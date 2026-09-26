# 20260926-03 via-ir 默认 + 多字返回 / SSA 槽 / CALL 家族 escape

## 0. 一句话

先按决定把 compile net 切到 `--via-ir`（stack-too-deep 是 EVM 栈只能 DUP/SWAP 到 16 个槽造成的，
不是 solc 的怪癖；via-ir 的 codegen 会自动溢出到 memory，前端检查集完全不变）。约束解除后一次做了三件
此前被栈限制挡住或缺失的事：**多字 helper 返回**（`(t0, t1) = f(...)`）、**demoteSSA 槽 → 局部变量**、
**CALL 家族 escape**（`__evm_call/callcode/delegatecall/staticcall`）。103 case：
condition TODO **3211 → 3159**、unresolved **155 → 139**、27 个 case 变好、只有 0448 的 real jump +2。

同时实测并**否决**了一条能大幅改指标但只搬移占位符的做法（见第 4 节），记录在案。

## 1. `--via-ir` 成为 compile suite 默认

`run_evm_solidity_pattern_compile_suite.py` 的 solc 调用现在默认带 `--via-ir`，可用
`--solc-arg` 覆盖（`--solc-arg=` 清空即回到 legacy）。理由与实测：

| | legacy | legacy+optimize | via-ir |
| --- | --- | --- | --- |
| 5 个 stack-too-deep case | 各 1 error | 各 1 error | **各 0 error** |
| 单文件耗时（24534） | 0.02 s | — | 0.54 s |

`Stack too deep` 的来源：EVM 的 `DUP1..DUP16`/`SWAP1..SWAP16` 只能触到栈顶 16 项，更深的局部变量必须
由编译器溢出到 memory。legacy codegen 把局部变量留在栈上，所以 >16 个同时存活就报错；via-ir 自带
stack-to-memory mover。**前端检查不变**：我们至今抓到的每一类真实错误（隐式转换、成员不存在、运算符类型
不匹配、`msg.value` in non-payable、pure/view 违规）都来自前端，via-ir 一样会报。变贵的只是编译时间。

## 2. 多字（struct）helper 返回

IR 形态：

```llvm
define internal { i256, i256 } @private__0x183_0x183(...)
  %ret.insert   = insertvalue { i256, i256 } poison, i256 %evm.mload, 0
  %ret.insert17 = insertvalue { i256, i256 } %ret.insert, i256 %_0x183arg0x0, 1
  ret { i256, i256 } %ret.insert17
...
%private.call = call { i256, i256 } @private__0x183_0x183(...)
%private.ret  = extractvalue { i256, i256 } %private.call, 0
```

实现：

1. `Reader::isHelperRenderCandidate` 原来只接受 `void`/i256 返回，struct-of-i256 的 helper 直接被排除
   —— 这是"多字返回没生效"的第一个原因；现在接受并用 `readHelperReturns` 生成 `(ret0, ret1)`。
2. `BodyBuilder::multiWordReturnExpr`：把 `insertvalue` 链折成 `return (a, b);`（从外向内走，
   同一 index 第一次见到的是最终值；base 必须是 undef/poison/zeroinitializer；任一元素未解析就整体放弃）。
3. 调用点：`readBody` 预扫描"返回 struct 且被 extractvalue 使用且 helper 可发射"的调用，为每个字预留
   `temp_N`；`collectStatements` 在调用点输出 `(t0, t1) = helper(args);`；`valueExpr` 的
   `extractvalue` 分支读回对应局部变量。Solidity 不能在表达式里投影 tuple，所以必须这样落地。
4. `valueNeedsMaterialization` 对 struct 值返回 false，避免和上面的解构重复。

## 3. demoteSSA 槽 → 局部变量 + CALL 家族 escape

- **槽**：候选 = i256 且只被 load/store 使用的 alloca；store → `name = v;`、load → 标识符、声明并入
  `HoistedNames`；名字取 alloca 名去掉 `.reg2mem` 后缀（可用时），否则 `temp_N`。
  接受条件是**该槽所有 store 的值都已解析**，并且迭代到不动点（槽之间互相引用）。语料里
  **14933 个 phi（77/103 case）** 都走这条路。
- **CALL 家族**：`__evm_call(gas, addr, value, inOffset, inSize, outOffset, outSize)`、
  `__evm_callcode`（7 参数）、`__evm_delegatecall`/`__evm_staticcall`（6 参数），函数体分别是
  Yul 的 `call/callcode/delegatecall/staticcall`（同名同参序，已对照 `InstructionLowerer` 的实参顺序）。
  都是 `internal`、**不带** mutability（状态交互）；结果没人用时按语句位置输出整条调用（否则副作用会被丢掉）。
  参数统一过 `wordOperandExpr`，任一实参未解析就放弃 escape（保持原来的 TODO，不放大占位符）。

## 4. 实测并否决：放宽槽接受条件

把"所有 store 都已解析"放宽成"无条件接受所有槽"（脏 store 照常输出 `name = 0 /* TODO: x */;`）：

| 指标 | 严格（保留） | 放宽（否决） |
| --- | --- | --- |
| condition TODO | 3211 → **3159** | 3211 → **2477**（−734） |
| unresolved | 155 → **139** | 155 → **970**（+815） |
| goto / real / dangling | 1192→1193 / 1110→1111 / 6→6 | 1192→1221 / 1110→1141 / 6→4 |
| 变好的 case | 27 | 62（同时约 60 个 case 的 unresolved 上升） |

−734 : +815 ≈ 1:1，说明绝大多数只是把占位符从条件搬到赋值：输出变成"看起来恢复了、其实读的是占位符"的
条件（`if (temp_5 == 0)` 而 `temp_5 = 0 /* TODO */`），比原来那条明确的 `false /* TODO: cond */`
更不诚实，指标也不再可比。0029 的实测比是 −140 : +170。所以保留严格策略；要真正吃掉 0029 那 792 条，
必须继续补缺失的表示，而不是放宽判定。

## 5. 剩余阻塞（新一轮 `NOTDEC_COND_DEBUG` 结果）

0029（792 条，占全语料 25%）的条件叶子仍然是槽 reload，而槽被拒的根因是链上的
`evm_call` / `and(1, evm.sload)` 这类值——注意 `%evm.and8 = and i256 1, %evm.sload7` 是**普通 LLVM and**，
它只是被 `evm_sload` 的操作数（另一个槽）拖住。也就是说 0029 是一条自引用链，需要同时补上多个缺失表示
才能整体解开。下一步优先级建议：

1. **COPY 家族语句 escape**：`evm_calldatacopy`/`codecopy`/`extcodecopy`/`returndatacopy`/`evm_mcopy`
   —— 今天全部**静默丢副作用**（和上一轮修掉的 `evm_sstore` 同类），机械且零局部变量。
2. 单目/位运算遗漏：`evm_not` 之类（`evm_and/or/xor` 在 frontend 里走 LLVM 指令，已覆盖；
   但 `evm_byte`/`evm_not` 等尚无渲染）。
3. 之后再用 `NOTDEC_COND_DEBUG` 重排长尾（1106 个名字的长尾里，`evm.lt.*`/`evm.iszero*` 系列仍占约 200 条）。

## 6. 验证

- `notdec.evm.solidity_pattern_compile`（默认 `--via-ir`）：103 case、0 失败，预算按下面的新锚点收紧。
- `notdec.evm.solidity_patterns`：102/103（唯一失败 24259 仍是另一会话 mlsub WIP 引起，与本轮无关）。
- `notdec.evm.solidity_source` / `notdec.evm.solidity_rewrite`：通过。
- 指标对比脚本：`/tmp/cmp_sstore.py`（todo/unres/goto/real/dang 逐 case diff）。

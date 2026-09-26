# 20260926-02 剩余 3211 条 TODO 的根因定位，以及一条被实测否决的修法

## 0. 一句话

condition TODO 的分布不是"每条都难"，而是少数根因；而且**只看输出计数会误判**：
`getCondition` 只要条件里有一个未解析叶子，就把整条条件塌成一条 `false /* TODO: name */`，
叶子名字根本不出现在输出里，所以 0029 这种 792 条 TODO 的 case 输出里 `unresolved = 0`。
新加 `NOTDEC_COND_DEBUG=1` 把阻塞叶子打出来后，根因收敛到两类**缺失的低层表示**：
`evm_call`/多字 helper 返回。同时实测否决了一条看起来自然的修法（把 SSA demotion 槽渲染成局部变量）——
Solidity 的 16 栈槽限制不允许。

## 1. 分布（103 case，3211 条）

- **1106 个不同名字**，长尾：`evm.lt.i.not` 38、`evm.lt.not` 32、`evm.lt.i.i.not` 28、
  `evm.iszero.i.i.not` 27 …… 名字里的 `.i.i.i` 是 inliner 的后缀，不是新指令。
- 集中度极高：**0029 792、0114 398、0726 398 = 49%**，前 12 个 case 占约 70%。
- 这三个头部 case 输出里 `unresolved value` 计数是 **0** —— 因为条件塌陷吃掉了叶子。
  结论：**排 TODO 优先级时必须用 `NOTDEC_COND_DEBUG`，不能看输出计数。**

## 2. 根因链（0029 实测）

```
[condition-todo] evm.eq.i.i.i.i <- _0x8f1_0x1.i.i.i.i.reload          (8 个副本，每个条目 8 次)
```

也就是说条件的叶子是**被 demote 的 SSA 槽的 reload**。往上追：

1. `demoteSSA` 对 Solidity 路径同样生效（`src/Passes/PassManager.cpp:211-213`）：全语料
   **14933 个 phi（77/103 case）** 被降级成 i256 alloca + `<name>.reload` load。所以"跨 merge 点的值"
   在今天完全没有表示，这是长尾 TODO 的主因之一。
2. 槽的 store 值里出现的阻塞叶子：`_0xbc1_0x0.*.reload`（另一个槽）、`evm.mload4`（其 offset 是
   `inttoptr %private.ret`）、`evm.div.*`（操作数不可解析）、`evm_call`。
3. 两个"上游缺失"因此暴露出来：
   - **多字（struct）helper 返回**：`%private.ret = extractvalue { i256, i256 } %private.call, 0`；
     `readHelperReturns` 对非 i256 返回直接放弃（注释："Multi-word (struct) and non-word helpers
     are not rendered yet"）。全语料 **425 个 struct 返回 helper / 70 个 case / 1973 处 extractvalue**。
     0029 只有 **2 个**这样的 helper，但它们的结果被当 memory offset（`inttoptr %private.ret`），
     于是那批 `evm.mload*` 无法 escape（escape 要求实参已解析）→ 依赖它们的槽 dirty → 条件塌陷。
   - **低层 CALL 结果**：`%evm.call = call i256 @evm_call(mem, returndata, env, gas, addr, value,
     inOff, inSize, outOff, outSize)`，后端完全没有渲染（0029 有 11 处）。同理还有
     `evm_delegatecall`/`evm_staticcall`，以及纯语句类的 `evm_calldatacopy`/`evm_codecopy`/
     `evm_extcodecopy`/`evm_returndatacopy`/`evm_mcopy`（这些今天是**静默丢副作用**，和上一轮
     `evm_sstore` 一样）。

## 3. 试过并放弃：把 demoted 槽渲染成 Solidity 局部变量

思路（实现过，已回滚）：候选 = 类型 i256 且只被 load/store 使用的 alloca；store → `name = v;`，
load → 标识符；声明并入 `HoistedNames`；名字取 alloca 名去掉 `.reg2mem`；接受条件 = 该槽**所有**
store 的值都已解析（固定点迭代，因为槽之间互相引用）。实测：

| 变体 | condition TODO | unresolved | solc |
| --- | --- | --- | --- |
| 不设上限 | 3211 → 3181 | 155 → 144 | **5 个 case `Stack too deep`** |
| 每函数上限 4（按读次数优先） | 3211 → 3184 | 155 → 145 | **4 个 case `Stack too deep`** |

原因：Solidity legacy codegen 的栈上限是 **16 个同时存活的局部变量**，而这些函数本来就贴着上限
（生成表达式嵌套很深，例如 25563 里 `if (!(f(g(temp_3, slot_6, 4499), h(f(...), 2, 4476), ...) == 0))`）。
`max_compile_failures: 0` 是硬约束，所以**任何"把值物化成函数级局部变量"的方案都必须先证明有栈余量**，
不能只在数量上加 cap。这条约束同样否决了"多字返回 → `(t0, t1) = f(...)`"的朴素做法
（0029 有 10 处、1485 有 134 处 extractvalue，每处 2 个局部变量必然爆栈）。

收益也只有 −30 条（约 1%），不值得为它做栈余量分析，已回滚。

## 4. 下一步候选（都已排除栈风险）

(a) **低层 CALL/COPY escape 家族**（推荐先做，机械且零局部变量）：
   `__evm_call(gas, addr, value, inOff, inSize, outOff, outSize)` → `assembly { ok := call(...) }`
   （Yul 内建同名同序），另有 `__evm_delegatecall`/`__evm_staticcall`（结果 = 成功标志，可直接进
   `require(...)`）；语句类的 `__evm_calldatacopy`/`__evm_codecopy`/`__evm_extcodecopy`/
   `__evm_returndatacopy`/`__evm_mcopy` 用同样的 `internal` + assembly 包装，顺带修掉"静默丢副作用"。
   参数卫生沿用上一轮的 `wordOperandExpr` 与"实参未解析就不 escape"。
(b) **多字 helper 返回**：不能用 per-call 局部变量，改用**按返回下标投影成单字函数**——
   对一个无副作用的 struct 返回 helper 生成 `f__0(...)`/`f__1(...)`，各自 `returns (uint256)`，
   `extractvalue %call, k` 渲染成对应调用；只投影实际用到的下标。body 侧把 `insertvalue` 链折叠成
   `return a;`（或 `return (a, b);` 的单字版本）。零新增局部变量。
(c) 上面两者落地后，再用 `NOTDEC_COND_DEBUG` 重新排序长尾。

## 5. 本轮保留的东西

`SolidityPayloadProvider::getCondition` 在 `NOTDEC_COND_DEBUG=1` 时向 stderr 打印每条被塌掉的条件
及其阻塞叶子（`collectUnresolvedTexts` 遍历表达式树）：

```
[condition-todo] evm.eq.i.i.i.i <- _0x8f1_0x1.i.i.i.i.reload
```

不设该环境变量时输出逐字节不变（103 case 指标对比：todo/unresolved/goto/real/dangling 全部相同）。

## 6. 验证

回滚后 `notdec.evm.solidity_pattern_compile`：103 case、`compile_failures=0`、
unresolved 155/155、condition TODO 3211/3211、goto 1192/1192、real 1110/1110、dangling 6/6、
helper 1548/1548、call sites 3125/3125（全部贴着上一轮锚定的棘轮）。

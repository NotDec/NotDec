# EVM IR 链路 + Solidity 后端现状复查

## 一、IR 结果

用 Gigahorse 缓存真实跑了一次 bytecode 到 LLVM IR（`simple-storage.hex`）：

```bash
python3 external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  /sn640/gigahorse-toolchain/tests/via-ir/simple-storage.hex \
  -o /tmp/ev2-storage.ll \
  --evm2llvm external/NotDec-evm2llvm/build/bin/evm2llvm \
  --work-dir /tmp/ev2-storage-work --timeout-secs 180 \
  --gigahorse-extra-arg=-i \
  --gigahorse-extra-arg=--restart \
  --gigahorse-extra-arg=--disable_inline \
  --gigahorse-extra-arg=--disable_scalable_fallback
```

结果：

- Gigahorse facts 正常产出，evm2llvm 生成 ~10KB IR；
- `llvm-as` 通过；
- `opt -mtriple=unknown-unknown-unknown -passes=verify` 通过；
- IR 保留了 selector dispatch、private function、calldata 读取、`evm_sload/sstore`、
  `evm_return`、每个 TAC 指令的 `!notdec.evm` metadata。

当前 IR 层面的判断：**结构和 verifier 质量已经够用**，不是后续主要瓶颈。需要注意的
是它仍然是“低层 helper IR”，SLOAD 还是 helper、dynamic ABI buffer 还是 calloc/loop，
后面靠 NotDec 的 EVM pattern pass + 类型恢复把语义抬高。

一个已知低层面问题：Gigahorse 对部分 `REVERT` 的栈恢复不精确时，TAC 会把函数
formal 当成 offset/size（例如 `simple-storage` 的 nonpayable guard 变成
`evm_revert(mem, %arg0, %arg0)`）。evm2llvm 忠实搬运了这个形状；后面应该由
high-level EVM guard pass 或 Solidity 后端识别 canonical nonpayable guard 后
归一化为 `revert()`，而不是在 evm2llvm 里硬猜。

## 二、Solidity 后端现状

当前后端位置：`external/NotDec-llvm2c/lib/Solidity/`，已经完成：

- 声明层：contract / public function / 参数 / 返回值 / state variable / event /
  error / empty payable fallback；
- AST + Printer：statement/expression 节点、缩进、优先级；
- body builder：typed statement payload，`if/else/while/do/for` 结构打印；
- 表达式层：常量、比较、bool and/or、算术/位运算、EVM div/mod/exp/shift/
  addmod/mulmod/signextend、`block.basefee`、`block.blobbasefee`、`gasleft()`、
  `msg.sender`；
- revert/require/emit、单 word return。

### 小 source suite

`notdec.evm.solidity_source` 当前 78/78 通过；其中 78/78 生成结果都能被
solc 0.8.25 编译（`--bin`），语法/类型层面已经站住。

### 复杂 pattern 生成的 Solidity 审计

对 `test/evm/solidity-patterns/cases/*.ll` 全量 103 个 case 跑：

```bash
notdec case.ll -o case.sol --tr-level=2
solc-0.8.25 --bin case.sol
```

本次修复前：103/103 能生成 `.sol`，25/103 能被 solc 编译。
本次修复后：29/103 能被 solc 编译。

两个低风险修复：

1. SSA 名字 sanitize
   `%private.call2` 这类 LLVM 名字原本直接打印成 `private.call2`，被 solc 解析成
   member access，报 `Expected primary expression`。现在统一 sanitize 成合法
   Solidity 标识符；只有 TODO/诊断注释仍保留原始 LLVM 名字。
2. dynamic ABI 参数 data location
   `string` / `bytes` 参数现在打印 `calldata`，修复 4 个 case 的
   `Data location must be "memory" or "calldata"`。

剩余 74 个编译失败全部是 `Undeclared identifier.`，按第一处出现分类：

| 类别 | 数量 | 来源 |
| --- | ---: | --- |
| `evm_sload` / `evm_sloadN` | 27 | 没走 high-level storage rewrite 的直接 storage load |
| `evm_and` / `evm_shl` 等 SSA 值 | 10+ | 实际是 `@evm.storage.*` helper 调用的结果，后端未识别 |
| `private_call` / `private_callN` / `private_ret` | 10+ | private function 调用结果未 inline / 未生成 helper |
| `evm_calldataload_*` / `evm_mload_*` / env builtin | 若干 | calldata/memory/环境 helper 未接 |
| 其它 SSA 名字 | 少量 | 泛化 fallback |

结论：**代码语法层已经没问题，主要缺口在“helper/SSA 值到 Solidity 表达式的语义映射”**。

## 三、下一步建议

优先级从高到低：

1. storage helper 表达式落地
   `evm.storage.slot` / `map.value` / `packed.load` / `static_array` 等映射回
   state variable / mapping access / packed field。预期直接清掉最大一类 undeclared。
2. private call 处理
   要么在 body builder 里把 `call @private__...` 恢复成函数调用并输出 private
   function，要么上游把 small private function inline；不能继续留 SSA 名。
3. CFG condition 恢复
   当前条件大多是 `if (false /* TODO: evm.branch.cond */)`。先接 `valueExpr`
   到 condition payload，让简单 guard / bool branch 能打印真实条件。
4. dynamic ABI return / memory buffer
   现在只有单 static word return 稳；dynamic bytes/array 还需要和 memory buffer
   helper 对接。
5. 测试
   给 pattern audit 加一个 CI job：抽样若干 `.sol` 必须能被 solc 编译；小 source
   suite 继续作为语义/文本 oracle。

## 四、本轮代码提交

- external/NotDec-llvm2c `a0a248d`
  `solidity: sanitize SSA identifiers and dynamic param locations`
- 主项目更新 gitlink 并提交本日志。

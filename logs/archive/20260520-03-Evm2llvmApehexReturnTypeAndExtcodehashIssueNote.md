# evm2llvm apehex `return type mismatch` / `EXTCODEHASH` 问题记录

## 背景

`more than two successors` 修完后，继续跑 `apehex_evm_contracts` 的 `batch009`，大部分样本都通过了。

这批里剩下的两个失败不是同一类问题：

1. 一个是 IR 返回类型不一致。
2. 一个是新的 opcode 覆盖缺口。

## 当前现象

`batch009` 结果是 `38/40` 通过，失败样本是：

- `0335_19494307_c365a11910_1354ce2e324d`
- `0351_19494408_834fc39f9b_5cebd5a0bfa1`

对应报错是：

```text
Function return type does not match operand type of return inst!
  ret void
 i256
```

以及：

```text
unsupported opcode EXTCODEHASH at 0xfc6
```

相关日志：

- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch009/logs/0335_19494307_c365a11910_1354ce2e324d.log`
- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch009/logs/0351_19494408_834fc39f9b_5cebd5a0bfa1.log`

## 说明

### 1. `0335` 是 IR 类型问题

这个失败不是 opcode 不认识，而是 `evm2llvm` 生成了一个返回类型和 `ret` 指令值类型不匹配的函数。

日志里 verifier 报得很直接：

```text
Function return type does not match operand type of return inst!
  ret void
 i256
```

这类问题要回到 lowering 里检查函数签名、return path 和 `ret` 值类型是不是一致。

### 2. `0351` 是 opcode 缺口

这条是纯粹的 lowering 覆盖问题：

```text
unsupported opcode EXTCODEHASH at 0xfc6
```

说明当前 `evm2llvm` 还没把 `EXTCODEHASH` 接进去。

## 判断

目前这批结果说明：

- CFG 结构问题已经收住了
- 旧的 `MULMOD`、`CREATE2`、`CHAINID` 问题没有回潮
- 现在剩下的是：
  - 一个返回类型一致性问题
  - 一个 opcode 覆盖缺口 `EXTCODEHASH`

后面继续扩批时，这两类问题还可能继续出现，分别要按 lowering 类型和 opcode 覆盖去处理。

## 备注

这次只做问题记录，没有改代码，也没有跑 fortune。

## 实现记录

已把这份记录里的两个问题一起收掉：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:94-137,364-422`
  - `terminalStatement()` 改为只识别真实控制流 terminator。
  - 修复 `RETURNPRIVATE` 后面还有 trailing `CONST` 时，被误判为“无 private return”的问题。
  - 非 `void` 函数如果真的没有 `RETURNPRIVATE`，现在直接报错，不再生成假返回。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:21-886`
  - 补了 `EXTCODEHASH`、`EXTCODECOPY`、`TLOAD`、`TSTORE`、`CREATE`、`CALLCODE`、`BLOCKHASH`、`COINBASE`、`GASPRICE`、`GASLIMIT`、`BASEFEE`、`BLOBHASH`、`BLOBBASEFEE`、`PC`、`KECCAK256`、`DIFFICULTY` / `PREVRANDAO`。
  - 处理了 `PUSH0` 和常见 `PUSHn`、`DUPn`、`SWAPn` 的 TAC 入口，避免这些原样落到 `unsupported opcode`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:9-98`
  - 增加了新 lowering 依赖的 runtime helper 声明。
- `external/NotDec-evm2llvm/test/CMakeLists.txt:36-44`
  - 新增 `opcode_coverage` fixture。
- `external/NotDec-evm2llvm/test/fixtures/opcode_coverage/*`
  - 新增覆盖新 opcode 路径的最小样本。

## 验证

- `cmake --build /sn640/NotDec/build-evm2llvm --target evm2llvm -j4`
- `ctest --test-dir /sn640/NotDec/build-evm2llvm -R '^evm2llvm\\.fixture\\.' --output-on-failure`
- 复跑两个真实样本：
  - `0335_19494307_c365a11910_1354ce2e324d`
  - `0351_19494408_834fc39f9b_5cebd5a0bfa1`
  - 都已经不再卡在原来的 verifier / unsupported opcode。

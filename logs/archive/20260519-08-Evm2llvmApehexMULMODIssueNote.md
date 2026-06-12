# evm2llvm apehex `MULMOD` 问题记录

## 背景

这次继续往 `apehex_evm_contracts` 往后推批次时，新的失败又稳定落到了同一个点：

- `unsupported opcode MULMOD`

它不是 `pilot` 阶段的问题，也不是 PHI 重复问题，而是 `evm2llvm` 对某些真实样本里的 opcode 覆盖还不完整。

## 当前现象

最近两批里都能复现：

- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch005/summary.csv`
- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch006/summary.csv`

命中的样本包括：

- `0010_19493030_17416f24c6_ea1ba3f21383`
- `0124_19493266_358e0dec95_ac8a34628401`
- `0143_19493361_674da545a4_938fcd843b18`
- `0205_19493685_dc49e7dfd9_528c6087ecaa`

对应日志里都能看到同样的报错：

```text
unsupported opcode MULMOD at 0x32e3
```

## 说明

这类失败的特征很清楚：

1. Gigahorse 侧 facts 已经生成完成，`errors=0`。
2. 失败发生在 `evm2llvm` lower 阶段。
3. 报错地址固定，说明不是随机数据噪声，而是稳定的 opcode 覆盖缺口。

所以这不是 PHI 校验问题，也不是 verifier 误报。

## 判断

目前可以先把它归成一类很直接的问题：

- `evm2llvm` 还没实现 `MULMOD`

后面如果要继续扩大 `apehex_evm_contracts` 覆盖面，这个 opcode 需要补上，否则类似样本还会继续失败。

## 备注

这次只做问题记录，没有改代码，也没有跑 fortune。

## 实现记录

这次检查发现当前 `external/NotDec-evm2llvm` 代码已经包含 `MULMOD` 支持：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:68`
  已声明 `evm_mulmod(i256, i256, i256) -> i256`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:219`
  `lowerTernary()` 已把 `MULMOD` lower 成 `evm_mulmod` 调用。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:642`
  主分发已把 `MULMOD` 放进三操作数表达式路径。

这次没有改 lowering 主体，只补了回归覆盖：

- `external/NotDec-evm2llvm/test/fixtures/arithmetic/TAC_Op.csv:6`
  新增一个 `MULMOD` TAC。
- `external/NotDec-evm2llvm/test/fixtures/arithmetic/TAC_Use.csv:3`
  给 `MULMOD` 连接三个操作数。
- `external/NotDec-evm2llvm/test/fixtures/arithmetic/TAC_Def.csv:4`
  记录 `MULMOD` 的结果变量。
- `external/NotDec-evm2llvm/test/fixtures/arithmetic/TAC_Variable_Value.csv:3`
  新增 fixture 常量。

判断：之前 batch005/batch006 的 `unsupported opcode MULMOD` 更可能是运行时用的
`evm2llvm` 二进制早于当前子模块代码，而不是当前源码仍缺实现。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
build-evm2llvm/bin/evm2llvm \
  --facts /sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch006/work/0205_19493685_dc49e7dfd9_528c6087ecaa/0205_19493685_dc49e7dfd9_528c6087ecaa/out \
  --output /tmp/evm2llvm-mulmod-0205.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/evm2llvm-mulmod-0205.ll \
  -o /tmp/evm2llvm-mulmod-0205.bc
llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/evm2llvm-mulmod-0205.bc
```

结果：

- `evm2llvm.fixture`：21/21 通过。
- `0205_19493685_dc49e7dfd9_528c6087ecaa`：当前 `evm2llvm`
  不再报 `unsupported opcode MULMOD`，`0x32e3` lower 为 `@evm_mulmod`。

性能：这次只改 evm2llvm fixture 和日志，不触碰 NotDec 主 pass pipeline、类型恢复、
结构体合并或 pointer analysis；不跑 fortune 性能对比。

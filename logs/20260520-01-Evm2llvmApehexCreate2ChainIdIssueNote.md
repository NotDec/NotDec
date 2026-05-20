# evm2llvm apehex `CREATE2` / `CHAINID` 问题记录

## 背景

`MULMOD` 修完后，继续跑 `apehex_evm_contracts` 的 `batch007`，大部分样本已经通过。

这批里最后留下的两个明确失败不是 PHI 问题，而是新的 opcode 覆盖缺口。

## 当前现象

`batch007` 结果是 `38/40` 通过，失败样本只有两个：

- `0266_19493909_db205f215f_d1d6b1bed833`
- `0280_19493924_b2910fc84f_9efc6f820e90`

对应报错是：

```text
unsupported opcode CREATE2 at 0xc8
unsupported opcode CHAINID at 0x392
```

相关日志：

- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch007/logs/0266_19493909_db205f215f_d1d6b1bed833.log`
- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch007/logs/0280_19493924_b2910fc84f_9efc6f820e90.log`

## 说明

这两个失败点都很直接：

1. Gigahorse facts 已经产出，`errors=0`。
2. 失败发生在 `evm2llvm` lowering。
3. 报错是固定 opcode 名称，不是 verifier 级的 CFG / PHI 语义错误。

所以当前可以先把它们归类成普通的 opcode 覆盖缺口。

## 判断

现在的剩余问题面已经比前面干净很多：

- `MULMOD` 已经修掉
- `PHIIncoming` 那类老问题没有回潮
- 目前新冒出来的是 `CREATE2` 和 `CHAINID`

后面如果继续扩批，这两个 opcode 需要补上，不然同类样本还会继续失败。

## 备注

这次只做问题记录，没有改代码，也没有跑 fortune。

## 2026-05-20 实现记录

用户原始需求：

```text
logs/20260520-01-Evm2llvmApehexCreate2ChainIdIssueNote.md 阅读这个文件，然后修复这两个不支持的指令
```

本次把 `CREATE2` 和 `CHAINID` 接进 evm2llvm lowering，没有改 PHI / CFG 逻辑。

修改点：

- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:60`，在
  `declareEvmRuntimeHelpers` 中声明 `evm_chainid(ptr) -> i256`。
- `external/NotDec-evm2llvm/lib/EvmRuntimeDecls.cpp:64`，在
  `declareEvmRuntimeHelpers` 中声明
  `evm_create2(ptr mem, ptr env, i256 value, i256 offset, i256 size, i256 salt) -> i256`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:256`，在
  `InstructionLowerer::lowerStateRead` 的无操作数环境读取集合中加入
  `CHAINID`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:289`，在
  `InstructionLowerer::lowerStateRead` 中把 `CHAINID` lowering 成
  `evm_chainid(env)`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:400`，在
  `InstructionLowerer::lowerStateWrite` 中把 `CREATE2` lowering 成
  `evm_create2(mem, env, value, offset, size, salt)`，返回创建地址。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:622` 和 `:674`，
  在 `InstructionLowerer::lower` 的分发中接入 `CREATE2` / `CHAINID`。
- `external/NotDec-evm2llvm/test/fixtures/state/TAC_Op.csv:7` 到 `:9`，
  `TAC_Def.csv:6` 到 `:8`，`TAC_Use.csv:6` 到 `:9`，给现有
  `state` fixture 增加 `CHAINID` 和 `CREATE2` 覆盖。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture.state --output-on-failure
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
build-evm2llvm/bin/evm2llvm --facts /sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch007/work/0266_19493909_db205f215f_d1d6b1bed833/0266_19493909_db205f215f_d1d6b1bed833/out --output /tmp/notdec-evm2llvm-0266.ll
build-evm2llvm/bin/evm2llvm --facts /sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch007/work/0280_19493924_b2910fc84f_9efc6f820e90/0280_19493924_b2910fc84f_9efc6f820e90/out --output /tmp/notdec-evm2llvm-0280.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-evm2llvm-0266.ll -o /tmp/notdec-evm2llvm-0266.bc
llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/notdec-evm2llvm-0266.bc
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-evm2llvm-0280.ll -o /tmp/notdec-evm2llvm-0280.bc
llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/notdec-evm2llvm-0280.bc
```

结果：

- evm2llvm fixture 全部通过：`21/21`。
- `0266_19493909_db205f215f_d1d6b1bed833` 不再停在
  `unsupported opcode CREATE2 at 0xc8`，生成 IR 后通过 LLVM verify。
- `0280_19493924_b2910fc84f_9efc6f820e90` 不再停在
  `unsupported opcode CHAINID at 0x392`，生成 IR 后通过 LLVM verify。

性能判断：

这次只增加两个 opcode 的直接分发和 runtime helper 声明，不进入类型恢复、
结构体合并、pointer analysis 或 pass pipeline。没有跑 fortune 计时；从改动路径看不会影响
当前 fortune 主链路性能。

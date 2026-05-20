# evm2llvm apehex `more than two successors` 问题记录

## 背景

`CREATE2` 和 `CHAINID` 修完后，继续跑 `apehex_evm_contracts` 的 `batch008`，大部分样本都已经通过。

这批里剩下的唯一失败不是 opcode 问题，而是一个 CFG 结构问题。

## 当前现象

`batch008` 结果是 `39/40` 通过，唯一失败样本是：

- `0283_19493931_6d84640bb6_449600e5884c`

对应报错是：

```text
block 0x4e62b has more than two successors
```

相关日志：

- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch008/logs/0283_19493931_6d84640bb6_449600e5884c.log`

## 说明

这类失败和前面的 opcode 缺口不一样：

1. Gigahorse 侧 facts 已经产出，`errors=0`。
2. 失败发生在 `evm2llvm` lowering。
3. 报错不是某个 opcode 名字，而是 block 出边数超过了当前 lowering 的预期。

所以它更像是 CFG 结构没有被当前 lowering 正确接受，或者 facts 里某个 block 的出口语义和 lowerer 假设不一致。

## 判断

目前可以先把它单独归成一类 CFG 问题：

- block 的 successor 数量超出当前处理逻辑

后面如果继续扩批，这类样本还可能继续出现，得回到 block 连接语义上查，而不是继续补 opcode。

## 备注

这次只做问题记录，没有改代码，也没有跑 fortune。

## 2026-05-20 修复记录

这次确认 `0x4e62b` 是动态 `JUMP` 的多目标 CFG，不是重复边。facts 里该块有 3
条本函数出边：

- `0x4e62b -> 0x4e524`
- `0x4e62b -> 0x4e549`
- `0x4e62b -> 0x4e56e`

修改：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:229` 增加
  `blockIdConstant`，把具体 block id 转成 LLVM `switch` case 常量。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:417` 修改
  `lowerTerminator`：当多出边终结语句是 `JUMP` 时，按跳转目标生成 LLVM
  `switch`；默认分支进入 `unreachable`。`JUMPI` 仍只接受两条边。
- `external/NotDec-evm2llvm/test/CMakeLists.txt:43` 增加 `jump_table`
  fixture。
- `external/NotDec-evm2llvm/test/fixtures/jump_table/*` 增加 3 目标动态
  `JUMP` 回归用例。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
build-evm2llvm/bin/evm2llvm \
  --facts /sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch008/work/0283_19493931_6d84640bb6_449600e5884c/0283_19493931_6d84640bb6_449600e5884c/out \
  --output /tmp/evm2llvm-0283-switch.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/evm2llvm-0283-switch.ll \
  -o /tmp/evm2llvm-0283-switch.bc
llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/evm2llvm-0283-switch.bc
```

结果：

- `evm2llvm.fixture`：24/24 通过。
- `0283_19493931_6d84640bb6_449600e5884c`：能生成 IR，且通过
  `llvm-as` 和 `opt -passes=verify`。
- 该样本里一共生成 3 个动态 `JUMP` switch：`0x4e62b`、`0x4e606`、
  `0x4e651`。

性能：

```bash
/usr/bin/time -p build-evm2llvm/bin/evm2llvm \
  --facts /sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch008/work/0283_19493931_6d84640bb6_449600e5884c/0283_19493931_6d84640bb6_449600e5884c/out \
  --output /tmp/evm2llvm-0283-switch-time.ll
```

结果：`real 0.29s`。这次只改 `evm2llvm` lowering，没有触碰 NotDec 主 pass
pipeline、类型恢复、结构体合并或 pointer analysis，所以没有重跑 fortune。

评分：

- 实现效果：8/10。能正确表达动态 `JUMP` 多目标 CFG，并保留原 predecessor 给
  PHI 使用。
- 复杂度：3/10。只在 terminator lowering 增加一条 `JUMP -> switch` 路径。
- 维护成本：3/10。后续如果 Gigahorse 导出非具体 block id，会明确报错，不会静默错分支。

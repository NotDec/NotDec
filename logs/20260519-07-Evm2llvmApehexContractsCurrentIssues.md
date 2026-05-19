# evm2llvm apehex contracts 当前问题简析

## 背景

这轮先确认两件事：

1. `evm2llvm_apehex_pilot` 已经是 `8/8` 全过。
2. `apehex_evm_contracts` 继续往后跑，前 160 个去重样本里，已知问题从最早的 PHI 重复，逐步收敛到更具体的 opcode / CFG 语义问题。

我这次主要看的是最近两批结果：

- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-train-batch005/summary.csv`
- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-knownfail-rerun/summary.csv`
- `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260519-evm2llvm-knownfail-rerun/logs/0146_19493376_503a732d50_138a4d77d617.log`

## 当前状态

### 1. `batch005` 已经全绿

`batch005` 是从 train 数据里继续抽出来的 40 个去重 runtime bytecode。

结果是：

- `40/40` `evm2llvm` 成功
- `40/40` `llvm-as` 成功
- `40/40` `opt -passes=verify` 成功

这说明当前的主 lowering 路径在这一段样本上是稳定的，不是“整体坏掉”。

### 2. 之前那批已知失败样本重新跑后，暴露出新问题

把老的失败样本单独重跑后，结果不再只是 PHI 问题。

现在看到两类明确失败：

1. `unsupported opcode MULMOD`
   - `0010_19493030_17416f24c6_ea1ba3f21383`
   - `0124_19493266_358e0dec95_ac8a34628401`
   - `0143_19493361_674da545a4_938fcd843b18`

2. verifier 级 CFG 问题
   - `0146_19493376_503a732d50_138a4d77d617`
   - 报错是 `Entry block to function must not have predecessors!`

## 现在怎么看

### `MULMOD` 是最直接的缺口

这类失败是实打实的 opcode lowering 缺失，不是 validator 放得太严。

它的特征很清楚：

- Gigahorse 侧 facts 已经能正常产出，`errors=0`
- `evm2llvm` 在读到 bytecode 里的 `MULMOD` 时直接停掉
- 报错点稳定，属于可复现的 opcode 覆盖缺口

所以这类问题优先级高，处理方式也简单：

- 先补 `MULMOD` 的 lowering
- 再把这几个样本重跑

### `0146` 更像 CFG / 导出语义问题

`0146` 不是 opcode 问题，而是 LLVM verifier 直接拒绝：

```text
Entry block to function must not have predecessors!
```

这说明当前生成的 IR 里，函数入口块被连上了前驱。

这类问题更像下面两种之一：

1. Gigahorse facts 本身把入口块前驱关系导错了。
2. `evm2llvm` lower 时把某条边接回了 entry block，但这个边不该存在。

这不是靠放宽 verifier 能解决的，应该回到 facts / CFG 语义上查。

## 结论

现在的情况可以概括成一句话：

`evm2llvm_apehex_pilot` 已经通了，但 `apehex_evm_contracts` 往后推，会继续冒出两类新问题：

- opcode 覆盖缺口，当前已经明确看到 `MULMOD`
- 少数样本还会暴露 CFG 结构问题，当前已经明确看到 entry block 前驱错误

当前这批结果说明，后面不能只盯 PHI 了，得把 opcode 覆盖和 CFG 导出一起补齐。

## 备注

这次最初只做数据集结果分析，没有改代码，也没有跑 fortune。

## 实现记录

本次继续修了两个点：

1. `MULMOD` / `ADDMOD` lowering 缺口。
2. Gigahorse 函数入口块同时是循环头时，LLVM entry block 不能有前驱的问题。

### 代码修改

都在 `external/NotDec-evm2llvm`：

- `lib/InstructionLowerer.cpp:197` 新增 `InstructionLowerer::lowerTernary`，检查三操作数并降低 `ADDMOD` / `MULMOD`。
- `lib/InstructionLowerer.cpp:642` 在 `InstructionLowerer::lower` 里接入 `ADDMOD` / `MULMOD`。
- `include/notdec-evm2llvm/InstructionLowerer.h:46` 声明 `lowerTernary`。
- `lib/EvmRuntimeDecls.cpp:67` 声明 `evm_addmod` / `evm_mulmod` runtime helper。这里不用 `add/mul` 后再 `mod`，避免 256 位中间结果溢出导致语义不对。
- `lib/LlvmLowerer.cpp:36` 新增 `hasFunctionPredecessor`，判断真实入口块是否有同函数前驱。
- `lib/LlvmLowerer.cpp:279` 新增 `entryPhiSeedValue`，从入口 PHI 的 uses 里找非回边初值。
- `lib/LlvmLowerer.cpp:307` 新增 `fillSyntheticEntryPhiIncoming`，给真实入口 PHI 补 synthetic entry incoming。
- `lib/LlvmLowerer.cpp:519` 在入口块有前驱时创建 LLVM-only synthetic entry，并跳到真实入口。
- `lib/LlvmLowerer.cpp:640` 在普通 PHI incoming 后补 synthetic entry 的 PHI incoming。
- `test/CMakeLists.txt:41` 新增 `entry_loop` fixture。
- `test/fixtures/entry_loop/` 新增最小回归样例：真实入口块 `0x0` 有回边 `0x10 -> 0x0`，入口 PHI 初值来自 formal，回边值来自 loop body。

### 验证

构建：

```bash
cmake --build /sn640/NotDec/build-evm2llvm --target evm2llvm -j4
```

fixture：

```bash
ctest --test-dir /sn640/NotDec/build-evm2llvm -R 'evm2llvm.fixture' --output-on-failure
```

结果：`21/21` 通过。

真实样本：

```bash
/usr/bin/python3 external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  <sample.hex> \
  --output /tmp/notdec-evm2llvm-fix-<id>.ll \
  --evm2llvm /sn640/NotDec/build-evm2llvm/bin/evm2llvm \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --work-dir /tmp/notdec-evm2llvm-fix-<id>.work \
  --timeout-secs 180 \
  --gigahorse-extra-arg=-i \
  --gigahorse-extra-arg=--restart \
  --gigahorse-extra-arg=--disable_inline \
  --gigahorse-extra-arg=--disable_scalable_fallback
```

已验证：

- `0010_19493030_17416f24c6_ea1ba3f21383`：通过，`real 84.90s`
- `0124_19493266_358e0dec95_ac8a34628401`：通过，`real 85.01s`
- `0143_19493361_674da545a4_938fcd843b18`：通过，`real 85.03s`
- `0146_19493376_503a732d50_138a4d77d617`：直接用已有 facts 跑 `evm2llvm`，通过

LLVM 22 验证：

```bash
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as <out.ll> -o <out.bc>
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output <out.bc>
```

结果：上面 4 个输出都通过 `llvm-as` 和 `opt -passes=verify`。

### 性能和风险

这次改动只在 `evm2llvm` lowering 阶段，不涉及类型恢复、结构体合并、pointer analysis 或 pass pipeline，所以没有跑 fortune 对比。

性能影响判断：正常路径只多一次入口前驱扫描，按函数块数线性；只有入口有前驱时才多一个 basic block 和少量 PHI incoming。对当前问题样本，主要时间仍在 Gigahorse，`evm2llvm` 本身没有看到可感知变慢。

评分：

- 实现效果：8/10，覆盖当前已知 `MULMOD` 和 entry predecessor 失败。
- 复杂度：6/10，比删边复杂，但保留了真实 CFG/SSA 语义。
- 维护成本：6/10，synthetic entry 的规则集中在 `LlvmLowerer`，后面如果 Gigahorse 改成导出专门 entry stub，可以再简化。

更好的方案：长期可以让 Gigahorse 直接导出 LLVM 友好的函数 entry stub，或者显式标出 PHI 初始 incoming；但当前 facts 已经能表达语义，先在 `evm2llvm` 适配 LLVM IR 约束更直接。

# Evm2llvm JUMPI 条件修复记录

## 背景

`evm2llvm` 现在能稳定跑通 `apehex_evm_contracts` 的一批样本，但 `JUMPI` 的条件选错了。具体表现是，有些 IR 里本来应该是基于条件跳转，最后却变成了 `br i1 true`，把跳转目标当成了分支条件。

我拿了两个样例确认这个问题：

- `external/NotDec-evm2llvm/test/fixtures/phi_branch`
  - 这里只给了 `JUMPI` 一个条件 use，旧逻辑会直接读这个值。
- `external/NotDec-evm2llvm/test/fixtures/jumpi_condition`
  - 这里同时给了跳转目标和条件，能直接暴露“把目标当条件”的错误。

## 修改内容

### `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp`

- `blockIdConstant()` 前后新增了 `isConcreteSuccessorValue()` 和 `jumpiConditionUseIndex()`。
- `lowerTerminator()` 里的 `JUMPI` 分支不再固定读 `Uses[0]`，改成：
  - 只有一个 use 时，直接把它当条件。
  - 有多个 use 时，优先找出看起来像 concrete successor block id 的那个，把剩下的 operand 当条件。
  - 看不出来时，退回到最后一个 use。
- 相关函数范围大概在 233-304、523-531 行。

### `external/NotDec-evm2llvm/test/fixtures/jumpi_condition/*`

- 新增了一个最小回归 fixture。
- 这个 fixture 里：
  - `0x0` 是 `CONST 0`
  - `0x1` 是 `CONST 0x10`
  - `0x2` 是 `JUMPI`
  - 两个 successor 分别是 `0x10` 和 `0x20`
- 目标是验证输出里必须是 `br i1 false, label %bb._0x10, label %bb._0x20`，不能再把目标常量当成条件。

### `external/NotDec-evm2llvm/test/CMakeLists.txt`

- 新增 `jumpi_condition` fixture。
- 新增 `check-branch` 断言，用 `grep` 直接检查生成的 `.ll`，确认分支条件是 `false`。

## 验证

- `cmake --build build-evm2llvm --target evm2llvm -j4`
- `ctest --test-dir build-evm2llvm -R 'evm2llvm.fixture.(phi_branch|jump_table|jumpi_condition)' --output-on-failure`
- 手工看输出：
  - `build-evm2llvm/bin/evm2llvm --facts external/NotDec-evm2llvm/test/fixtures/phi_branch --output /tmp/phi_branch.ll`
  - `build-evm2llvm/bin/evm2llvm --facts external/NotDec-evm2llvm/test/fixtures/jumpi_condition --output /tmp/jumpi_condition.ll`

结果：

- `phi_branch` 通过，说明单条件 `JUMPI` 还能正常跑。
- `jumpi_condition` 生成了 `br i1 false, label %bb._0x10, label %bb._0x20`，回归点修住了。

## 结论

这次修的是 `JUMPI` 的 operand 选择，不动其他 opcode lowering。
后面如果再看到 `br i1 true`，优先先看事实里 `JUMPI` 的 use 列表，而不是先怀疑 LLVM verifier。

## 补充 review：真实样例 abort

复查最新提交并用 `0394_19494617_261e203d6f_433b0912f7c8` 的真实 facts 重生成 IR 时，发现原修复还有一个真实样例会触发的问题：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:257`
  - `isConcreteSuccessorValue()` 会遍历 successors，并调用 `blockIdConstant()`。
  - 真实样例里有 `0xeb0x0`、`0xef0xf44` 这类带 context 后缀的 block id，不能被 `blockIdConstant()` 解析成纯地址。
  - 原代码遇到这个 error 后直接 `continue`，但没有 `takeError()` / `consumeError()`，LLVM 的 `Expected` 析构时会 abort。

修复：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:258`
  - 在跳过不可解析 successor 前调用 `llvm::consumeError(successorConstantOrError.takeError())`。
  - 语义不变：不可解析 successor 仍然不能用来判断 concrete target，只是把错误正确消费掉。

验证：

- `cmake --build build-evm2llvm --target evm2llvm -j4`
- 用已有 facts 更新：
  - `/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch010/outputs/0394_19494617_261e203d6f_433b0912f7c8.ll`
- LLVM 22 验证：
  - `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as ...0394...ll -o ...0394...bc`
  - `/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output ...0394...bc`
- 回归：
  - `ctest --test-dir build-evm2llvm -R 'evm2llvm.fixture.(phi_branch|jump_table|jumpi_condition)' --output-on-failure`

更新后的 0394 输出里，原来的 `br i1 true` 已经变成真实条件：

- selector dispatch 使用 `%evm.branch.cond`
- `callvalue == 0` 使用 `%evm.branch.cond`
- delegatecall 成败分支使用 `%evm.branch.cond`

## 补充调整：去掉 successor 启发式

继续确认 `TAC_Use.csv` 的第三列后，发现它在 `FactLoader` 里已经作为 operand position 排序：

- `external/NotDec-evm2llvm/lib/FactLoader.cpp:140`
  - 读取 `TAC_Use.csv` 第三列为 position。
- `external/NotDec-evm2llvm/lib/FactLoader.cpp:258`
  - 按 position 排序后写入 `TacStatement::Uses`。
- `external/NotDec-evm2llvm/include/notdec-evm2llvm/TacProgram.h:15`
  - 注释也说明 uses/defs 已按 position 排好。

因此对 Gigahorse 导出的两操作数 `JUMPI`，可以按 EVM 栈语义直接处理：

- `Uses[0]` 是 destination。
- `Uses[1]` 是 condition。

新的修复：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:249-256`
  - 删除 `isConcreteSuccessorValue()` 和“排除 concrete successor”的启发式。
  - `JUMPI` 两个 use 时固定用 `Uses[1]` 当 condition。
  - 其他 use 数量直接报错，不再猜。
- `external/NotDec-evm2llvm/test/fixtures/phi_branch/TAC_Use.csv`
  - 给原来单 use 的 `JUMPI` 补上 destination use。
- `external/NotDec-evm2llvm/test/fixtures/phi_branch/TAC_Variable_Value.csv`
  - 补上 destination 常量 `0x10target = 0x10`。

再次验证：

- `cmake --build build-evm2llvm --target evm2llvm -j4`
- 重新生成并更新 `0394_19494617_261e203d6f_433b0912f7c8.ll`
- `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as ...0394...ll -o ...0394...bc`
- `/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output ...0394...bc`
- `ctest --test-dir build-evm2llvm -R 'evm2llvm.fixture.(phi_branch|jump_table|jumpi_condition)' --output-on-failure`

结果：

- 0394 输出里不再有 `br i1 true`。
- `phi_branch` 已改成双 use，并通过。
- `jumpi_condition` 双 use 路径通过。

剩余风险：

- 现在不再按 successor 猜条件，主要风险变成：如果未来 Gigahorse 改变 `TAC_Use` position 对 `JUMPI` 的含义，需要同步更新这里的约定。

## 补充调整：删除单 use 兼容

进一步检查后，单 use `JUMPI` 只存在于旧 fixture，真实 0394 facts 里的 6 个 `JUMPI` 都是两个 use。为避免错误 facts 被默默降错，已删除单 use 兼容：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:249-256`
  - `jumpiConditionUseIndex()` 现在严格要求 `JUMPI` 有两个 use。
  - 两个 use 以外直接报错：`must have destination and condition uses`。

验证同上，`phi_branch`、`jump_table`、`jumpi_condition` 回归通过，0394 重新生成后仍通过 LLVM 22 `llvm-as` 和 `opt -passes=verify`。

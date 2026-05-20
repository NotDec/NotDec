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

## 补充 review

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

剩余风险：

- 当前条件选择还是启发式：多个 use 时排除 concrete successor，取剩下的 operand。它比旧逻辑正确，但如果某个条件值本身刚好等于 successor 地址，仍可能误判。后续最好让 Gigahorse facts 显式标出 `JUMPI` 的 destination 和 condition，或者在 loader 层按 opcode operand 角色拆开。

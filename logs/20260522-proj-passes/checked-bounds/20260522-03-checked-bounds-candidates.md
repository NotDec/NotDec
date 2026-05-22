# checked / bounds 候选标注

## 背景

checked arithmetic、array bounds、ABI bounds 等保护分支还没有单独 metadata。当前阶段先把“非 callvalue 条件跳到 empty revert”和 Panic revert 作为 compiler guard 候选标出来。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:17` 增加 `KIND_SOLIDITY_CHECKED_BOUNDS`。
- `include/notdec/Passes/evm/SolidityPatterns.h:94` 增加 `CheckedBoundsPass`。
- `src/Passes/evm/SolidityPatterns.cpp:26` 增加统计 `NumCheckedBounds`。
- `src/Passes/evm/SolidityPatterns.cpp:53` 增加 metadata 名 `notdec.solidity.checked_bounds`。
- `src/Passes/evm/SolidityPatterns.cpp:248` 增加 `dependsOnCallTo`，用来排除 callvalue guard。
- `src/Passes/evm/SolidityPatterns.cpp:575` 实现 `CheckedBoundsPass::run`：
  - 条件分支到 empty revert，且条件不依赖 `evm_callvalue`，标为 `guard_to_empty_revert_candidate`。
  - `Panic(uint256)` 形状的 revert 标为 `panic_guard_candidate`。
- `src/Passes/PassManager.cpp:294` 接入 pass。

## 通用性

这次没有按函数名或 PC 特判。规则会把不同来源的 compiler guard 先统一标候选，后续再由 ABI decode、array/storage pass 进一步细分。

## 验证

- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过。
- manifest 中 `checked_bounds` 覆盖 12 个 case。
- fortune 同口径运行退出码 0，耗时 16.66s。

## 评分

- 实现效果：6/10。能覆盖当前样例里的保护分支，但还没提取 panic code 和具体 bounds 类型。
- 复杂度：3/10。增加了小范围 use-def 查询。
- 维护成本：3/10。后续要减少 empty revert 的用户 require 误标。

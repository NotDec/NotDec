# Solidity pattern pass 文件拆分

## 原始 prompt

先拆开吧

## 背景

`src/Passes/evm/SolidityPatterns.cpp` 同时放了 selector、payability、ABI、revert、checked bounds、storage、event、external call 的 `run()` 实现。后续 memory-buffer、storage、external/event 并行开发都会改这个文件，容易产生无关 merge 冲突。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp`：保留共享 matcher、metadata helper、统计定义和 pass 公共 helper；文件末尾改为 include 各 pass 的实现片段。
- `src/Passes/evm/solidity-patterns/SelectorEntryOutliningPass.inc`：移动 `SelectorEntryOutliningPass::run`。
- `src/Passes/evm/solidity-patterns/PayabilityGuardPass.inc`：移动 `PayabilityGuardPass::run`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.inc`：移动 `AbiReturnPass::run`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.inc`：移动 `SolidityRevertPass::run`。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.inc`：移动 `CheckedBoundsPass::run`。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.inc`：移动 `StorageAddressingPass::run`。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.inc`：移动 `PackedStorageFieldPass::run`。
- `src/Passes/evm/solidity-patterns/EventLogPass.inc`：移动 `EventLogPass::run`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.inc`：移动 `ExternalCallPass::run`。

## 判断

这次只拆文件，不改 pass 顺序、matcher 逻辑、metadata 名字和统计行为。使用 `.inc` 是为了先避免大规模接口拆分；后续如果某条线稳定，再把共享 helper 提到正式头文件/源文件。

复杂度：2/10。include 片段不如正式 `.cpp` 干净，但改动小。
维护成本：3/10。后续各 pass 能先改独立文件，公共 helper 仍需在主文件里协调。
实现效果：6/10。能降低最直接的 merge 冲突，但还没有彻底拆开共享 helper。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，`1/1`，约 `97.24 sec`。

这次只移动代码，未做性能敏感逻辑改动。EVM pattern suite 同口径通过，说明 metadata 和 rewrite marker 行为没有变化。

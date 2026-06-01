# Solidity pattern pass 文件拆分

## 原始 prompt

先拆开吧

## 背景

`src/Passes/evm/SolidityPatterns.cpp` 同时放了 selector、payability、ABI、revert、checked bounds、storage、event、external call 的 `run()` 实现。后续 memory-buffer、storage、external/event 并行开发都会改这个文件，容易产生无关 merge 冲突。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp`：保留共享 matcher、metadata helper 和 pass 公共 helper；原匿名 namespace 改为 `notdec::passes::evm::detail`。
- `include/notdec/Passes/evm/SolidityPatternUtils.h`：新增内部共享声明，给各 pass `.cpp` 使用。
- `src/CMakeLists.txt`：把各 pass `.cpp` 加入 `notdec` 静态库。
- `src/Passes/evm/solidity-patterns/SelectorEntryOutliningPass.cpp`：移动 `SelectorEntryOutliningPass::run` 和 selector outline 统计。
- `src/Passes/evm/solidity-patterns/PayabilityGuardPass.cpp`：移动 `PayabilityGuardPass::run` 和 payability 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp`：移动 `AbiReturnPass::run` 和 ABI return 统计。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp`：移动 `SolidityRevertPass::run` 和 revert 统计。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp`：移动 `CheckedBoundsPass::run` 和 checked/bounds 统计。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp`：移动 `StorageAddressingPass::run` 和 storage addressing 统计。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp`：移动 `PackedStorageFieldPass::run` 和 packed storage 统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp`：移动 `EventLogPass::run` 和 event 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp`：移动 `ExternalCallPass::run` 和 external call 统计。

## 判断

这次只拆文件，不改 pass 顺序、matcher 逻辑、metadata 名字和统计行为。共享 helper 先统一放在 `detail` namespace，后续各分支可以先改自己的 pass `.cpp`，公共 helper 再单独协调。

复杂度：4/10。比 `.inc` 多了一个内部声明头和 CMake 源文件列表，但结构更清楚。
维护成本：3/10。后续各 pass 能先改独立文件，公共 helper 仍需在主文件里协调。
实现效果：7/10。能降低最直接的 merge 冲突，但还没有彻底拆开共享 helper 的实现文件。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，`1/1`，约 `98.47 sec`。

这次只移动代码，未做性能敏感逻辑改动。EVM pattern suite 同口径通过，说明 metadata 和 rewrite marker 行为没有变化。

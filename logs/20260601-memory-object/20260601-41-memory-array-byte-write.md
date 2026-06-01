# Memory array byte write marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮处理 `mstore8` 生成的 memory byte write。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2403`，memory array index access。
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:3210`，memory byte array element 写入。

关键逻辑：

- 对 memory array 取下标时，Solidity 先用 `memoryArrayIndexAccessFunction(arrayType)` 算元素地址。
- 如果 array 是 bytes/string，`IRLValue::Memory` 会标记 `byteArrayElement`。
- 对 `byteArrayElement` 写入时生成：
  - `mstore8(address, byte(0, value))`

真实 IR 里，这类地址常见形状是 `arrayBase + 32 + index`。此前 `MemoryBufferAnalysis` 只写出 `notdec_solidity_memory_byte_write(ptr, 0, value)`，base 是最终地址，不是 memory array base。这样后续 ABI return / event 如果想消费 bytes/string array，会缺少 array base 和 index。

## 实现

新增 marker：

- `notdec_solidity_memory_array_byte_write(arrayBase, index, value)`

改动：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:70`
  - 增加 `MemoryArrayByteWrite`，记录 array base、byte index 和写入值。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:92`
  - `MemoryBufferFacts` 增加 `ArrayByteWrites`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:20`
  - 增加 `NumMemoryArrayByteWrites` 统计。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:130`
  - 增加 `getArrayByteIndexFromPtr()`。
  - 只识别明确的 `arrayBase + (32 + index)` 或 `arrayBase + constant>=32`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:273`
  - 增加 `insertArrayByteWriteMarker()`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:394`
  - 只在 `evm_mstore8` 分支里生成 array byte write，避免把普通 `mstore(array + 32, word)` 误标成 byte write。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:566`
  - 在 rewrite pass 里插入新 marker。
- `test/evm/solidity-patterns/manifest.json:1253`
  - 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 `notdec_solidity_memory_array_byte_write: 3` oracle。

这轮是 rewrite surface 准备：把裸地址 `mstore8` 改写成 memory array base/index 语义。它还没有被 ABI return / event 消费，所以不能算完整闭环。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0450_19495071_6b6c9447e0_6344565f4b31`
    - `notdec_solidity_memory_array_byte_write`: 3
    - `notdec_solidity_memory_byte_write`: 3
    - `notdec_solidity_memory_read`: 3
    - `notdec_solidity_memory_write`: 18
    - `notdec_solidity_memory_allocation`: 6
    - `notdec_solidity_abi_return_memory_consumer`: 2
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，108.09s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_memory_array_byte_write`: 5
- `notdec_solidity_memory_byte_write`: 7
- `notdec_solidity_memory_read`: 1269
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_abi_return_memory_consumer`: 1420

新 marker 少于裸 `memory_byte_write`，因为这轮只接受明确的 bytes/string array data 地址形状，不处理普通裸指针或更复杂 symbolic pointer。

## 下一步

1. 让 ABI return 识别动态 bytes/string return 的 memory array source，消费 `memory_array_byte_write` 或至少绑定 array base。
2. 继续分析没有 `memory_array_byte_write` 的 `mstore8`，确认是更复杂的 array index 形状，还是非 bytes/string 用途。
3. Revert data allocation 仍需要跨 helper 处理，不能用同块策略硬接。

# Storage array data keccak marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

Solidity 的 storage 动态数组 data area 不是普通 memory allocation，而是 scratch memory 参与 storage addressing。

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2391`，`YulUtilFunctions::arrayDataAreaFunction()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2422`，`YulUtilFunctions::storageArrayIndexAccessFunction()`
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2381`，`IRGeneratorForStatements::endVisit(IndexAccess const&)`

关键形状：

- `arrayDataAreaFunction()` 对 storage dynamic array 生成 `mstore(0, ptr)` 和 `keccak256(0, 0x20)`。
- `storageArrayIndexAccessFunction()` 调用 data area helper，再对 data area 加 index 或 packed offset。
- `IRGeneratorForStatements::endVisit(IndexAccess)` 在 storage index access 时调用 `storageArrayIndexAccessFunction()`。

所以这轮要把 `mstore(0, baseSlot); sha3(0, 32)` 改写成明确的 IR marker，避免后续 storage pass 只靠 `sha3` metadata 猜这是动态数组 data slot。

## 实现

新增 marker：

- `notdec_solidity_storage_array_data_keccak(baseSlot, hash)`

改动：

- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:18`
  - 增加 `NumStorageArrayDataKeccak` 统计。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:28`
  - 增加 `ArrayDataKeccakMatch`，只记录写入 scratch offset 0 的 base slot。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:73`
  - 增加 `matchStorageArrayDataKeccak()`。
  - 只匹配 `evm_sha3(mem, 0, 32)`。
  - 同一 basic block 向前找最近的 `evm_mstore(mem, 0, baseSlot)`。
  - 遇到前一个 `evm_sha3` 或 external call 就停止，降低跨 scratch region 误配风险。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:125`
  - 增加 `insertStorageArrayDataKeccakMarker()`，在 `sha3` 后插入 marker，因为 marker 要引用 hash 结果。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:168`
  - 在 `sha3(0,64)` mapping scratch marker 之后，接入 `sha3(0,32)` dynamic array data marker。
- `test/evm/solidity-patterns/manifest.json:300`
  - 给 `checked_bounds_storage_array_01` 增加 `notdec_solidity_storage_array_data_keccak: 1` oracle。

这轮是 IR rewrite surface，不是 metadata-only。marker 还没有删除原始 `mstore/sha3`，但已经给后续 storage addressing pass 提供了稳定输入。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，101.88 秒。

Manifest oracle：

- `checked_bounds_storage_array_01`
  - `notdec_solidity_storage_array_data_keccak`: 1

## 固定 100 例 audit

输入：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

按 `.ll` 完整路径排序，取前 100 个对应 `.bc`，运行：

- `./build/bin/notdec <case>.bc -o /tmp/...ll --tr-level=0`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_storage_array_data_keccak`: 161
- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_copy_write`: 783
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_external_call_memory_consumer`: 1105
- `notdec_solidity_event_memory_consumer`: 439

## 下一步

1. 让 storage slot 恢复逻辑消费 `notdec_solidity_storage_array_data_keccak`，少从 `evm_sha3` metadata 反推动态数组 data area。
2. 继续补 storage mapping marker 的消费者闭环。
3. 再回到 allocation / return / revert 主线，优先把未消费的 memory marker 接到后续 pass。

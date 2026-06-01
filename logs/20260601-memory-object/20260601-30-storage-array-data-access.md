# Storage array data access marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮接上上一轮的 storage dynamic array data area marker。

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2391`，`YulUtilFunctions::arrayDataAreaFunction()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2422`，`YulUtilFunctions::storageArrayIndexAccessFunction()`
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2381`，`IRGeneratorForStatements::endVisit(IndexAccess const&)`

关键逻辑：

- storage dynamic array 的 data area 是 `mstore(0, ptr); data := keccak256(0, 0x20)`。
- `storageArrayIndexAccessFunction()` 再用 `dataArea` 计算实际 element slot。
- 对普通非 packed element，是 `slot := add(dataArea, mul(index, storageSize))`。
- 对 packed element，是 `slot := add(dataArea, div(index, itemsPerSlot))`，再算 bit offset。

所以这轮不是再标 `sha3`，而是消费上一轮的 `notdec_solidity_storage_array_data_keccak(baseSlot, hash)`，在后续 `sload/sstore` 前插入 storage array data access marker。

## 实现

新增 marker：

- `notdec_solidity_storage_array_data_access(baseSlot, dataHash, storageSlot, kind)`

`kind` 当前约定：

- `1`: `sload`
- `2`: `sstore`

改动：

- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:20`
  - 增加 `NumStorageArrayDataAccess` 统计。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:32`
  - 增加 `ArrayDataAccessMatch`。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:35`
  - 增加 `dependsOnValue()`，只做有深度上限的 use-def 递归。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:106`
  - 增加 `matchStorageArrayDataAccess()`。
  - 同一 basic block 向前找 `notdec_solidity_storage_array_data_keccak`。
  - 要求 `sload/sstore` 的 storage slot operand 依赖 marker 的 data hash。
  - 遇到前一个 `evm_sha3` 或 external call 停止，避免跨 scratch region 误配。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:176`
  - 增加 `insertStorageArrayDataAccessMarker()`。
  - marker 插在 `sload/sstore` 前，后续 pass 不需要再反查低层 `sha3`。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:212`
  - 在处理 `evm_sha3` 之前先处理 `evm_sload` / `evm_sstore`，消费已经插入的 data keccak marker。
- `test/evm/solidity-patterns/manifest.json:301`
  - 给 `checked_bounds_storage_array_01` 增加 `notdec_solidity_storage_array_data_access: 1` oracle。

这轮是 marker 消费闭环：`storage_array_data_keccak` 不再只是孤立 marker，后续 storage access 有了直接 IR rewrite surface。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/checked_bounds_storage_array_01.ll -o /tmp/notdec-storage-array-access.ll --tr-level=0`
  - 通过。
  - 输出里先出现 `notdec_solidity_storage_array_data_keccak(i256 0, i256 %evm.sha3)`，再在 `sload` 前出现 `notdec_solidity_storage_array_data_access(i256 0, i256 %evm.sha3, i256 %evm.add, i256 1)`。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，102.11 秒。

Manifest oracle：

- `checked_bounds_storage_array_01`
  - `notdec_solidity_storage_array_data_keccak`: 1
  - `notdec_solidity_storage_array_data_access`: 1

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
- `notdec_solidity_storage_array_data_access`: 79
- `notdec_solidity_storage_array_data_keccak`: 161
- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_rewrite_storage_addressing`: 1708
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_consumer`: 5736

## 下一步

1. 给 `notdec_solidity_storage_scratch_keccak(key, slot, hash)` 补 mapping `sload/sstore` access marker。
2. 将 packed storage field pass 逐步改成优先读取 storage access marker。
3. 回到 memory allocation / return / revert 主线，继续减少直接扫描原始 `mstore/mload/copy` 的逻辑。

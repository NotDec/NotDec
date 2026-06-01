# Storage mapping access marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮接上 `notdec_solidity_storage_scratch_keccak(key, slot, hash)`。

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2739`，`YulUtilFunctions::mappingIndexAccessFunction()`
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2340`，`IRGeneratorForStatements::endVisit(IndexAccess const&)`

关键逻辑：

- mapping index access 先通过 `mappingIndexAccessFunction()` 计算 data slot。
- 对非动态 key，Solidity 生成：
  - `mstore(0, convertedKey)`
  - `mstore(0x20, slot)`
  - `dataSlot := keccak256(0, 0x40)`
- `IRGeneratorForStatements::endVisit(IndexAccess)` 在 mapping index access 时调用这个 helper，并把返回 slot 作为 storage lvalue。

所以这轮不是再标 `sha3(0,64)`，而是消费已有的 `notdec_solidity_storage_scratch_keccak(key, slot, hash)`，在依赖该 hash 的 `sload/sstore` 前插入 mapping access marker。

## 实现

新增 marker：

- `notdec_solidity_storage_mapping_access(key, baseSlot, hash, storageSlot, kind)`

`kind` 当前约定：

- `1`: `sload`
- `2`: `sstore`

改动：

- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:22`
  - 增加 `NumStorageMappingAccess` 统计。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:40`
  - 增加 `MappingAccessMatch`。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:162`
  - 增加 `matchStorageMappingAccess()`。
  - 同一 basic block 向前找 `notdec_solidity_storage_scratch_keccak`。
  - 要求 `sload/sstore` 的 storage slot operand 依赖 scratch marker 的 hash 参数。
  - 遇到前一个 `evm_sha3` 或 external call 停止，避免跨 scratch region 误配。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:243`
  - 增加 `insertStorageMappingAccessMarker()`。
  - marker 插在 `sload/sstore` 前，参数保留 key、base slot、hash、最终 storage slot 和 access kind。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:284`
  - 在 `evm_sload` 处理里，先尝试 array data access，再尝试 mapping access。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:303`
  - 在 `evm_sstore` 处理里，先尝试 array data access，再尝试 mapping access。
- `test/evm/solidity-patterns/manifest.json:4167`
  - 给 `0147_19493380_262c2cb2b2_c24931e22d66` 增加 `notdec_solidity_storage_mapping_access: 31` oracle。

这轮是 marker 消费闭环：mapping scratch keccak 不再只是孤立 marker，后续 storage access 可以直接读 `notdec_solidity_storage_mapping_access`。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/0147_19493380_262c2cb2b2_c24931e22d66.ll -o /tmp/notdec-storage-mapping-access.ll --tr-level=0`
  - 通过。
  - `notdec_solidity_storage_scratch_keccak`: 29
  - `notdec_solidity_storage_mapping_access`: 31
  - 输出里 mapping access marker 插在对应 `sload/sstore` 前。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，101.79 秒。

Manifest oracle：

- `0147_19493380_262c2cb2b2_c24931e22d66`
  - `notdec_solidity_storage_scratch_keccak`: 29
  - `notdec_solidity_storage_mapping_access`: 31

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
- `notdec_solidity_storage_mapping_access`: 1473
- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_storage_array_data_access`: 79
- `notdec_solidity_storage_array_data_keccak`: 161
- `notdec_solidity_rewrite_storage_addressing`: 1708
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_consumer`: 5736

## 下一步

1. 让 packed storage field pass 优先读取 storage mapping / array data access marker，减少从 `sload/sstore` operand 递归猜 storage root。
2. 对 nested mapping / mapping-to-array 做更细的 access role marker。
3. 回到 memory allocation / ABI return / revert 主线，继续减少消费者直接扫原始 memory call 的逻辑。

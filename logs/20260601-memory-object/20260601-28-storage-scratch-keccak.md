# Storage scratch keccak marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

`YulUtilFunctions::mappingIndexAccessFunction()` 对静态 mapping key 生成的形状是：

- `mstore(0, convertedKey)`
- `mstore(0x20, slot)`
- `dataSlot := keccak256(0, 0x40)`

`IRGeneratorForStatements::endVisit(IndexAccess)` 里，mapping index access 调用这个 helper，返回的 slot 再作为 storage lvalue 的 slot。

这块 memory 是 scratch，不是 free memory allocation。计划里也明确提到 `mstore(0/32); sha3(0,64)` 不能误当作 allocation。之前 `StorageAddressingPass` 只给 `evm_sha3` 打 metadata，没有 IR rewrite surface。

## 本轮改动

新增 semantic marker：

- `notdec_solidity_storage_scratch_keccak(key, slot, hash)`

实现点：

- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:16`
  - 新增 `NumStorageScratchKeccak` 统计。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:21`
  - 新增 `ScratchKeccakMatch`，记录 `mstore(0, key)` 和 `mstore(32, slot)`。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:26`
  - 新增 `matchStorageScratchKeccak()`。
  - 只接受 `evm_sha3(mem, 0, 64)`。
  - 只向同一个 basic block 内回看最近的 `evm_mstore(0, key)` 和 `evm_mstore(32, slot)`。
  - 遇到前一个 `evm_sha3` 或 external call 就停止，避免跨 scratch buffer 误配。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:67`
  - 新增 `insertStorageScratchKeccakMarker()`。
  - marker 插在 `evm_sha3` 后，因为 marker 参数引用 hash 结果。
- `src/Passes/evm/solidity-patterns/StorageAddressingPass.cpp:106`
  - `StorageAddressingPass::run()` 在保留原 metadata 的同时插入 marker。
- `test/evm/solidity-patterns/manifest.json:4162`
  - `0147_19493380_262c2cb2b2_c24931e22d66` 增加 `notdec_solidity_storage_scratch_keccak: 29` oracle。

单 case 里，`0147_19493380_262c2cb2b2_c24931e22d66`：

- `notdec.solidity.storage_addressing`: 30
- `notdec_solidity_storage_scratch_keccak`: 29

剩下 1 个 storage addressing metadata 没有强行标成 scratch marker，因为没有完整的同 block `mstore(0)` + `mstore(32)` 形状。

这轮属于 IR rewrite：新增 marker 明确表达 storage addressing 消费了 scratch memory，后续 storage pass 可以直接读这个 marker，不需要重新从低层 `mstore/sha3` 猜。

## 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.23 秒。
- 固定 100 个 `.bc` 用当前 `build/bin/notdec --tr-level=0` 重新生成临时 `.ll`：100 个成功，0 个失败。

固定 100 个样例的 marker 统计：

- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_external_call_output_abi_decode_buffer`: 10
- `notdec_solidity_external_call_output_word_read`: 2
- `notdec_solidity_external_call_output_copy_write`: 8
- `notdec_solidity_abi_return_data_copy_write`: 22
- `notdec_solidity_abi_return_data_word_write`: 1422
- `notdec_solidity_event_data_copy_write`: 5
- `notdec_solidity_event_data_word_write`: 463
- `notdec_solidity_external_call_input_abi_head_write`: 290
- `notdec_solidity_external_call_input_word_write`: 308
- `notdec_solidity_external_call_input_copy_write`: 4
- `notdec_solidity_external_call_memory_consumer`: 1105
- `notdec_solidity_event_memory_consumer`: 439
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_revert_memory_write_match`: 1123
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_memory_copy_write`: 783
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_byte_write`: 7
- `notdec_solidity_memory_allocation`: 1041

## 下一轮候选

1. 让 `StorageAddressingPass` 或后续 storage pass 消费 `notdec_solidity_storage_scratch_keccak`，减少直接从 `evm_sha3` metadata 猜 storage slot 的逻辑。
2. 支持 `sha3(0,32)` 的动态 array data area marker，但要和 mapping slot marker 区分。
3. 检查 fixed 100 里 storage addressing metadata 和 scratch keccak marker 的差值，分类未覆盖形状。

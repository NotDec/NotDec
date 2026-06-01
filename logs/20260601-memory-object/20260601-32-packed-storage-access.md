# Packed storage access marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮接上 storage access marker，让 packed storage field pass 消费它。

源码：

- `/sn640/solidity/libsolidity/codegen/LValue.cpp:222`，`GenericStorageItem::retrieveValue()`
- `/sn640/solidity/libsolidity/codegen/LValue.cpp:287`，`GenericStorageItem::storeValue()`

关键逻辑：

- 如果 storage value 占 32 字节，Solidity 直接 `sload/sstore`。
- 如果小于 32 字节，读取时会 `sload` 整个 slot，再通过 shift/div/signextend/and 取出对应字节段。
- 写入时会先 `sload` 旧 slot，清掉目标字节段，再把新值移位后 `or` 回去，最后 `sstore`。

所以 packed field 的真正 storage root 仍然是前面的 `sload/sstore` slot。现在已经有：

- `notdec_solidity_storage_mapping_access(key, baseSlot, hash, storageSlot, kind)`
- `notdec_solidity_storage_array_data_access(baseSlot, dataHash, storageSlot, kind)`

这轮让 packed pass 在原有 packed 判定成立后，额外绑定这些 storage access marker。

## 实现

新增 marker：

- `notdec_solidity_packed_storage_access(storageSlot, kind, rootKind)`

`kind` 当前约定：

- `1`: packed load
- `2`: packed store

`rootKind` 当前约定：

- `1`: mapping access
- `2`: storage array data access

改动：

- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:4`
  - 增加 `IRBuilder` / `Module` 相关 include，用于插 marker。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:16`
  - 增加 `NumPackedStorageAccess` 统计。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:21`
  - 增加 `StorageAccessMarker`，记录已找到的 storage access marker 及 root kind。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:26`
  - 增加 `findStorageAccessMarker()`。
  - 同一 basic block 向前找 `notdec_solidity_storage_mapping_access` 或 `notdec_solidity_storage_array_data_access`。
  - 要求 marker 的 `storageSlot` 和 `kind` 与当前 packed `sload/sstore` 对上。
  - 遇到前一个 `sload/sstore/sha3` 或 external call 停止，避免跨 storage access 误配。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:58`
  - 增加 `insertPackedStorageAccessMarker()`，在 `sload/sstore` 前插入 packed storage access marker。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:82`
  - packed store 原有判定成立后，消费 storage access marker。
- `src/Passes/evm/solidity-patterns/PackedStorageFieldPass.cpp:101`
  - packed load 原有判定成立后，消费 storage access marker。
- `test/evm/solidity-patterns/manifest.json:4167`
  - 给 `0147_19493380_262c2cb2b2_c24931e22d66` 增加 `notdec_solidity_packed_storage_access: 21` oracle。

这轮没有放宽 packed storage 判断。只有原本已经是 packed load/store 的位置，才会额外插 marker；marker 的作用是把 packed field 和前面的 storage root rewrite surface 绑定起来。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/0147_19493380_262c2cb2b2_c24931e22d66.ll -o /tmp/notdec-packed-storage-access.ll --tr-level=0`
  - 通过。
  - `notdec_solidity_storage_mapping_access`: 31
  - `notdec_solidity_storage_array_data_access`: 0
  - `notdec_solidity_packed_storage_access`: 21
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，102.02 秒。

Manifest oracle：

- `0147_19493380_262c2cb2b2_c24931e22d66`
  - `notdec_solidity_storage_scratch_keccak`: 29
  - `notdec_solidity_storage_mapping_access`: 31
  - `notdec_solidity_packed_storage_access`: 21

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
- `notdec_solidity_packed_storage_access`: 639
- `notdec_solidity_storage_mapping_access`: 1473
- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_storage_array_data_access`: 79
- `notdec_solidity_storage_array_data_keccak`: 161
- `notdec_solidity_rewrite_packed_storage_field`: 3458
- `notdec_solidity_rewrite_storage_addressing`: 1708

## 下一步

1. 把 `notdec_solidity_packed_storage_access` 的参数扩展为可读的 byte offset / mask，减少后续 pass 重新解析 shift/mask。
2. 给 array data root 的 packed access 选更小的 oracle case。
3. 回到 ABI return / revert / event / external call 的 memory marker 消费闭环。

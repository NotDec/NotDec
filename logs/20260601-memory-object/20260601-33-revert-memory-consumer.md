# Revert memory consumer marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮补齐 revert consumer 的专门落点。

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:238`，`YulUtilFunctions::revertWithError()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4147`，`YulUtilFunctions::forwardingRevertFunction()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4598`，`YulUtilFunctions::revertReasonIfDebugBody()`

关键逻辑：

- `revertWithError()` 分配或使用 `pos`，写 error selector 和 ABI 参数，然后 `revert(pos, sub(end, pos))`。
- `forwardingRevertFunction()` 用 `allocateUnbounded()` 作为 `pos`，`returndatacopy(pos, 0, returndatasize())`，然后 `revert(pos, returndatasize())`。
- `revertReasonIfDebugBody()` 用 `start` 作为 buffer base，写 `Error(string)` ABI payload，最后 `revert(start, overallLength)`。

所以 revert 和 return/event/external call 一样，都是明确的 memory consumer。此前 `SolidityRevertPass` 已经能读 `notdec_solidity_memory_write`，但缺少类似 `abi_return_memory_consumer` / `event_memory_consumer` 的专门 marker。

## 实现

新增 marker：

- `notdec_solidity_revert_memory_consumer(base, size, kind)`

`kind` 当前约定：

- `1`: empty
- `2`: panic
- `3`: returndata_bubble
- `4`: error_string
- `5`: custom_error_candidate
- `6`: encoded_candidate

改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:4`
  - 增加 `Constants` / `IRBuilder` / `Module` include，用于插 marker。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:16`
  - 增加 `NumRevertMemoryConsumers` 统计。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:21`
  - 增加 `getRevertKindCode()`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:43`
  - 增加 `findRevertConsumerMarker()`。
  - 同一 basic block 内，在 `evm_revert` 前找 `notdec_solidity_memory_consumer(base, size, revert)`。
  - 目前要求 base 和 size 与 `evm_revert` 参数完全相同，不做 reload 猜测。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:63`
  - 增加 `insertRevertMemoryConsumerMarker()`，在 `evm_revert` 前插入专门 marker。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:99`
  - `matchSolidityRevert()` 成功后，消费 memory consumer marker 并插入 `notdec_solidity_revert_memory_consumer`。
- `test/evm/solidity-patterns/manifest.json:450`
  - 给 `revert_error_string_01` 增加 `notdec_solidity_revert_memory_consumer: 1` oracle。
- `test/evm/solidity-patterns/manifest.json:989`
  - 给 `0011_multi_public` 增加 `notdec_solidity_revert_memory_consumer: 5` oracle。

这轮是 IR rewrite 消费闭环：`notdec_solidity_memory_consumer(..., revert)` 被 `SolidityRevertPass` 读取，并落成更具体的 revert semantic marker。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，102.66s。
- `git diff --check`
  - 通过。

单例检查：

- `revert_error_string_01`
  - `notdec_solidity_memory_consumer`: 1
  - `notdec_solidity_revert_memory_consumer`: 1
  - `notdec_solidity_revert_memory_write_match`: 1
  - `notdec_solidity_rewrite_revert_error_string`: 1
- `0011_multi_public`
  - `notdec_solidity_memory_consumer`: 10
  - `notdec_solidity_revert_memory_consumer`: 5
  - `notdec_solidity_revert_memory_write_match`: 2
  - `notdec_solidity_rewrite_revert_error_string`: 2
  - `notdec_solidity_rewrite_revert_returndata_bubble`: 2
- `0014_proxy_like`
  - `notdec_solidity_revert_memory_consumer`: 0

## 固定 100 例 audit

固定输入来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_revert_memory_consumer`: 2772
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_revert_memory_write_match`: 1123
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_event_memory_consumer`: 439
- `notdec_solidity_external_call_memory_consumer`: 1105
- `notdec_solidity_packed_storage_access`: 639
- `notdec_solidity_storage_mapping_access`: 1473
- `notdec_solidity_storage_array_data_access`: 79
- `notdec_solidity_storage_scratch_keccak`: 1388
- `notdec_solidity_storage_array_data_keccak`: 161

这说明新 marker 在真实样例里不是 0 命中；它覆盖了 revert memory consumer 的主要路径。

## 下一步

1. 让 revert marker 进一步绑定 selector write / copy write，减少后续 pass 重扫 memory write。
2. 对 returndata bubble 的 scratch returndata copy 做更明确的 revert output marker。
3. 继续回到 ABI return / event / external call 的 payload marker，补动态 tail 相关语义。

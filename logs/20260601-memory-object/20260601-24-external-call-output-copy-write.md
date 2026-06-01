# External call output copy marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

`IRGeneratorForStatements::appendExternalFunctionCall()` 里，external call 的返回数据复用 call input 的 `pos`：

- 先 `pos := allocate_unbounded()`。
- `mstore(pos, shl28(selector))`，参数从 `add(pos, 4)` 开始 ABI encode。
- call 使用 `pos, sub(end, pos)` 作为 input buffer，也使用 `pos, staticReturndataSize` 作为 output buffer。
- 动态返回值时，成功后执行 `returndatacopy(pos, 0, returnDataSize)`。
- 然后 `finalizeAllocation(pos, returnDataSize)`，再对 `pos..pos+returnDataSize` 做 ABI decode。

所以 external call output 不只是一个 consumer。call 后的 `returndatacopy(pos, 0, returndatasize())` 也是同一个 output buffer 的写入，应该有独立 IR marker，给后续 ABI decode / returndata 恢复使用。

## 本轮改动

新增 semantic marker：

- `notdec_solidity_external_call_output_copy_write(base, sourceOffset, copySize, copyKind, callKind)`

实现点：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp`
  - 新增 `NumExternalCallOutputCopyWrites` 统计。
  - 新增 `findExternalCallOutputCopyWriteMarker()`。
  - matcher 先看 external call 后同 block，再看直接 successor block。Solidity 的失败分支和成功分支通常由 call result 分开，真实 `returndatacopy(pos, 0, size)` 在成功 successor 里。
  - 只接受 `notdec_solidity_memory_copy_write(base, 0, 0, size, ReturndataCopy)`，并且 base 必须和 external call output base 相同，或同为重新读取的 free memory pointer。
  - marker 插在 `memory_copy_write` 前，而不是 external call 前，因为 copy size 和 copy base 在 successor block 才定义。
- `src/Passes/evm/MemoryBufferAnalysis.cpp`
  - 给 write marker 的 base 选择增加 dominance 检查，避免跨 block 选到不支配 marker 插入点的 free pointer load。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h`
  - `analyzeMemoryBuffers()` 增加 `DominatorTree` 参数。
- `test/evm/solidity-patterns/manifest.json`
  - `0258_19493864_78edd9f88e_b098925cbd2d` 增加 `notdec_solidity_external_call_output_copy_write: 7` oracle。

## 验证结果

单 case：

- `0258_19493864_78edd9f88e_b098925cbd2d`
- `notdec_solidity_external_call_output_copy_write`: 6

完整验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.39 秒。
- 固定 100 个 `.bc` 用当前 `build/bin/notdec --tr-level=0` 重新生成临时 `.ll`：100 个成功，0 个失败。

固定 100 个样例的 marker 统计：

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

1. 看 external call output copy 后面的 ABI decode 是否能继续接到 output marker。
2. 看 returndata forwarding / revert bubble 是否还有只读低层 `returndatacopy`、没有消费 memory marker 的形状。
3. 再看是否需要把 external call input/output 的跨 block 扫描抽成共享 helper。

# External call output ABI decode buffer marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

`IRGeneratorForStatements::appendExternalFunctionCall()` 对动态 returndata 的路径是：

- external call 使用 `pos` 作为 output base，静态返回大小为 0。
- 成功后 `returnDataSize := returndatasize()`。
- `returndatacopy(pos, 0, returnDataSize)` 把 output 写入同一个 `pos`。
- `finalizeAllocation(pos, returnDataSize)` 更新 free memory pointer。
- `abiDecode(pos, add(pos, returnDataSize))` 再读取这段 output buffer。

上一轮已经给静态 output 的 word read 加了 `notdec_solidity_external_call_output_word_read`。这轮补动态路径：只要已经确认 `returndatacopy(pos, 0, size)` 是 external call output copy，就再给这段 buffer 插入 ABI decode 输入 marker。

## 本轮改动

新增 semantic marker：

- `notdec_solidity_external_call_output_abi_decode_buffer(base, size, callKind)`

实现点：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:25`
  - 新增 `NumExternalCallOutputAbiDecodeBuffers` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:408`
  - 新增 `insertExternalCallOutputAbiDecodeBufferMarker()`。
  - marker 参数来自已匹配的 `notdec_solidity_memory_copy_write(base, 0, 0, size, ReturndataCopy)`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:540`
  - 在 `findExternalCallOutputCopyWriteMarker()` 命中后，同时插入 output copy marker 和 ABI decode buffer marker。
- `test/evm/solidity-patterns/manifest.json:1890`
  - `0258_19493864_78edd9f88e_b098925cbd2d` 增加 `notdec_solidity_external_call_output_abi_decode_buffer: 6` oracle。

单 case 里，命中形状是：

- `notdec_solidity_external_call_output_copy_write(base, 0, returndatasize, ReturndataCopy, callKind)`
- `notdec_solidity_external_call_output_abi_decode_buffer(base, returndatasize, callKind)`

这轮属于 IR rewrite：新增 marker 绑定 dynamic returndata output buffer 和后续 ABI decode 输入语义。metadata 没有作为 pass 间接口。

## 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.25 秒。
- 固定 100 个 `.bc` 用当前 `build/bin/notdec --tr-level=0` 重新生成临时 `.ll`：100 个成功，0 个失败。

固定 100 个样例的 marker 统计：

- `notdec_solidity_external_call_output_abi_decode_buffer`: 8
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

1. 把 `getOffsetFromBase()` 迁到共享 helper，减少 ABI return、event、external call 之间的重复。
2. 看 `notdec_solidity_external_call_output_abi_decode_buffer` 后面的具体 `mload` 能否在更多动态返回 case 中绑定到 base/offset。
3. 继续检查 fixed 100 里 output copy 未命中的 external call，确认是无返回值、revert forwarding，还是 matcher 漏掉。

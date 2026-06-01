# External call static output decode buffer marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

`IRGeneratorForStatements::appendExternalFunctionCall()` 里，静态和动态返回值最后都会走同一个 ABI decode 入口：

- call 使用 `pos` 作为 output base。
- 静态返回值时，`staticReturndataSize` 直接传给 call opcode，返回数据由 call 写入 output area。
- 动态返回值时，`staticReturndataSize` 为 0，成功后显式 `returndatacopy(pos, 0, returnDataSize)`。
- 两条路径最后都是 `abiDecode(pos, add(pos, returnDataSize))`。

上一轮 `notdec_solidity_external_call_output_abi_decode_buffer` 只覆盖了动态 `returndatacopy` 路径。静态路径已经有 `notdec_solidity_external_call_output_word_read`，但还缺少同一层 ABI decode buffer 事实。这会让后续 pass 需要区分动态/静态两种接口。

## 本轮改动

补齐已有 semantic marker：

- `notdec_solidity_external_call_output_abi_decode_buffer(base, size, callKind)`

实现点：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:408`
  - 把 `insertExternalCallOutputAbiDecodeBufferMarker()` 改成通用版本，可以指定插入点、base 和 size。
  - 保留 `CopyWrite` 重载，动态路径继续在 `returndatacopy` marker 附近插入。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:549`
  - 记录当前 external call 是否已经通过 output copy 插入 decode buffer。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:564`
  - 如果没有 output copy，但找到了 output word read，就在第一个 word read 前插入静态 output decode buffer marker。
  - marker 使用 external call 的 `outBase` 和 `outSize`。
- `test/evm/solidity-patterns/manifest.json:4988`
  - `24574_19760246_e537c886f5_6e80990d311f` 增加 `notdec_solidity_external_call_output_abi_decode_buffer: 1` oracle。

单 case 里，命中形状是：

- `evm_staticcall(..., outBase=%evm.mload18, outSize=32)`
- `notdec_solidity_external_call_output_abi_decode_buffer(%evm.mload18, 32, staticcall)`
- `evm_mload(mem, %evm.mload18)`
- `notdec_solidity_external_call_output_word_read(%evm.mload18, 0, value, staticcall)`

这轮属于 IR rewrite：不是新增 metadata，而是补齐已有 output decode buffer marker 的静态语义，让动态 `returndatacopy` 和静态 word read 都落到同一个 ABI decode buffer 接口。

## 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.17 秒。
- 固定 100 个 `.bc` 用当前 `build/bin/notdec --tr-level=0` 重新生成临时 `.ll`：100 个成功，0 个失败。

固定 100 个样例的 marker 统计：

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

1. 把 base/offset helper 迁到共享 helper，减少 external call、ABI return、event 三处重复。
2. 看 static output decode buffer 后是否可以恢复多个返回值的连续 word read。
3. 检查 fixed 100 中仍未落到 output decode buffer 的 external call，确认是无返回值、失败转发，还是 matcher 漏掉。

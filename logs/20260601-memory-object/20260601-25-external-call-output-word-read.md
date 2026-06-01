# External call output word read marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

`IRGeneratorForStatements::appendExternalFunctionCall()` 里，external call 的 output buffer 和 input buffer 共用 `pos`：

- `pos := allocate_unbounded()`。
- call 使用 `pos, sub(end, pos)` 作为 input buffer。
- call 也使用 `pos, staticReturndataSize` 作为静态返回值 output buffer。
- 成功后设置 `returnDataSize`，必要时执行 `returndatacopy(pos, 0, returnDataSize)`。
- 然后 `finalizeAllocation(pos, returnDataSize)`。
- 最后执行 `abiDecode(pos, add(pos, returnDataSize))`。

所以静态返回值场景里，call 后的 `mload(pos + 32*n)` 是 ABI decode 对 external call output buffer 的读取。这个读取要有独立 IR marker，不能只靠 external call metadata 表示。

## 本轮改动

新增 semantic marker：

- `notdec_solidity_external_call_output_word_read(base, offset, value, callKind)`

实现点：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:25`
  - 新增 `NumExternalCallOutputWordReads` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:65`
  - 新增 `getOffsetFromBase()`，支持同一 SSA 值、常量 base 差值、`base + const`，以及重新读取 free memory pointer 的 base。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:92`
  - 新增 `outputSizeCoversWord()`，常量 `outSize` 必须覆盖完整 32-byte word，避免把 `outSize = 0` 的动态 returndata 路径误标成直接 output word read。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:299`
  - 新增 `collectExternalCallOutputWordReadMarkers()`。
  - 扫描 external call 后同 block 和直接 successor block。
  - 遇到 free memory pointer store 或新的 external call 就停止，避免跨 buffer 误配。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:406`
  - 新增 `insertExternalCallOutputWordReadMarker()`，marker 插在 `evm_mload` 后面，因为 marker 要引用读取结果。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:529`
  - `ExternalCallPass::run()` 在 output copy marker 后继续插入 output word read marker。
- `test/evm/solidity-patterns/manifest.json:4986`
  - `24574_19760246_e537c886f5_6e80990d311f` 增加 `notdec_solidity_external_call_output_word_read: 1` oracle。

单 case 里，命中形状是：

- `evm_staticcall(..., outBase=%evm.mload18, outSize=32)`
- 后续 `evm_mload(mem, %evm.mload18)`
- 插入 `notdec_solidity_external_call_output_word_read(%evm.mload18, 0, %evm.mload26, staticcall)`

这轮属于 IR rewrite：新增 marker 绑定 output base、word offset、读取值和 call kind。metadata 没有作为 pass 间接口。

## 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.39 秒。
- 固定 100 个 `.bc` 用当前 `build/bin/notdec --tr-level=0` 重新生成临时 `.ll`：100 个成功，0 个失败。

固定 100 个样例的 marker 统计：

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

1. 继续把 external call output 的 `returndatacopy` 和 word read 合并成更完整的 ABI decode 输入事实。
2. 看动态返回值路径里 `outSize = 0` 后的 `returndatacopy(pos, 0, returndatasize())` 是否还有未覆盖的 decode read。
3. 把 `getOffsetFromBase()` 这类 base/offset helper 迁到共享 memory helper，减少 ABI return、event、external call 之间的重复。

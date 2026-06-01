# Memory read marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮补通用 memory read marker，并让 external-call output word read 消费它。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2655`，`IRGeneratorForStatements::appendExternalFunctionCall()`

关键逻辑：

- external call 使用 `pos` 作为 output base。
- 静态返回值时，call opcode 直接把返回值写进 `pos`。
- 动态返回值时，成功后执行 `returndatacopy(pos, 0, returnDataSize)`。
- 两条路径最后都会执行 `abiDecode(pos, add(pos, returnDataSize))`。

所以 external call output 的 ABI decode 本质是从 output buffer 读取 word。此前 `ExternalCallPass` 的 output word read 仍然直接扫 `evm_mload`。这轮把 `mload(base + offset)` 先落成通用 memory read marker，再让 `ExternalCallPass` 读取这个 marker。

## 实现

新增 marker：

- `notdec_solidity_memory_read(base, offset, value)`

改动：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:54`
  - 增加 `MemoryRead`，记录 `mload`、base、offset 和读取结果。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:82`
  - `MemoryBufferFacts` 增加 `Reads`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:19`
  - 增加 `NumMemoryReads` 统计。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:219`
  - 增加 `insertReadMarker()`。
  - marker 插在 `evm_mload` 后面，因为参数要引用读取结果。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:322`
  - 收集 `evm_mload(base + const)`，排除 free memory pointer slot `0x40`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:501`
  - 在 rewrite pass 中插入 `notdec_solidity_memory_read`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:301`
  - `collectExternalCallOutputWordReadMarkers()` 改成读取 `notdec_solidity_memory_read`，不再直接匹配 `evm_mload`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:450`
  - `insertExternalCallOutputWordReadMarker()` 改成使用 memory read marker 的 base、offset 和 value。
- `test/evm/solidity-patterns/manifest.json:5002`
  - 给 `24574_19760246_e537c886f5_6e80990d311f` 增加 `notdec_solidity_memory_read: 35` oracle。

这轮是 IR rewrite 消费闭环：`MemoryBufferAnalysis` 产生通用 read marker，`ExternalCallPass` 消费它并继续生成 `notdec_solidity_external_call_output_word_read`。

实现里保留了一个低层 `evm_mload` fallback。原因是固定 100 例里有 2 个历史 `external_call_output_word_read` 命中暂时还没有被通用 `memory_read` 覆盖。fallback 只用于避免覆盖倒退；如果 `evm_mload` 后面已经有对应 `notdec_solidity_memory_read`，会跳过低层路径，避免重复插 marker。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `24574_19760246_e537c886f5_6e80990d311f`
    - `notdec_solidity_memory_read`: 35
    - `notdec_solidity_external_call_output_word_read`: 1
    - `notdec_solidity_external_call_output_abi_decode_buffer`: 1
    - `notdec_solidity_external_call_memory_consumer`: 7
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，103.34s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_memory_read`: 1175
- `notdec_solidity_external_call_output_word_read`: 2
- `notdec_solidity_external_call_output_abi_decode_buffer`: 10
- `notdec_solidity_external_call_output_copy_write`: 8
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_copy_write`: 783
- `notdec_solidity_revert_data_word_write`: 4071
- `notdec_solidity_revert_data_copy_write`: 499

新增 `memory_read` 在真实样例里有稳定命中，同时没有丢掉原有 external-call output word read 覆盖。

## 下一步

1. 继续让 ABI return / event / revert 中需要 read 的逻辑复用 `notdec_solidity_memory_read`。
2. 检查 `MemoryAllocation` marker 是否能被消费者读取，避免 allocation 只停留在 oracle。
3. 看 `MemoryBufferAnalysis` 是否需要支持跨 block 的 read/write 绑定，先从真实高频漏报 case 入手。

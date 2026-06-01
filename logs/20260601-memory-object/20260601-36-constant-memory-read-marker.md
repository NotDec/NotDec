# Constant memory read marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续补 `notdec_solidity_memory_read` 的覆盖。上一轮固定 100 例里还有 2 个 `notdec_solidity_external_call_output_word_read` 走低层 `evm_mload` fallback，原因是 output base 是常量地址，不是 free-memory base。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2655`，`IRGeneratorForStatements::appendExternalFunctionCall()`

关键逻辑：

- external call 使用 `pos` 作为 output base。
- 静态返回值时，call opcode 直接把返回值写入 output area。
- 成功后执行 `abiDecode(pos, add(pos, returnDataSize))`。

真实 IR 里不总是 `mload(0x40)` 作为 output base。固定 100 例里的 `25596_19770844_d480364206_1e55fe8eccf5` 有两个静态 output read：

- `mload(52)`
- `mload(0)`

这两个是 external call output decode，不是业务分支。之前 `ExternalCallPass` 可以靠 fallback 识别它们，但通用 `memory_read` 没有覆盖到，导致 pass 间接口还不完整。

## 实现

改动：

- `src/Passes/evm/MemoryBufferAnalysis.cpp:322`
  - 收集 `evm_mload` 时，先按已有 free-memory base 匹配。
  - 如果没有匹配到 base，但读取地址是常量，就插入 `notdec_solidity_memory_read(constAddr, 0, value)`。
- `test/evm/solidity-patterns/cases/25596_19770844_d480364206_1e55fe8eccf5.ll`
  - 新增固定 100 例里发现的真实 IR fixture。
- `test/evm/solidity-patterns/manifest.json`
  - 新增该 fixture 的 oracle：
    - `notdec_solidity_memory_read: 2`
    - `notdec_solidity_external_call_output_word_read: 2`
    - `notdec_solidity_external_call_output_abi_decode_buffer: 2`

这轮补齐的是已有 rewrite surface 的覆盖：`MemoryBufferAnalysis` 现在能把常量地址 read 也落到 `notdec_solidity_memory_read`，`ExternalCallPass` 随后消费这个 marker。低层 fallback 仍保留，但这两个真实命中已经不再依赖 fallback。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `25596_19770844_d480364206_1e55fe8eccf5`
    - `notdec_solidity_memory_read`: 2
    - `notdec_solidity_external_call_output_word_read`: 2
    - `notdec_solidity_external_call_output_abi_decode_buffer`: 2
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，103.96s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_memory_read`: 1269
- `notdec_solidity_external_call_output_word_read`: 2
- `notdec_solidity_external_call_output_abi_decode_buffer`: 10
- `notdec_solidity_external_call_output_copy_write`: 8
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_copy_write`: 783

对比上一轮，`notdec_solidity_memory_read` 从 1175 增到 1269；`notdec_solidity_external_call_output_word_read` 保持 2，没有覆盖倒退。

## 下一步

1. 继续检查还有哪些 external-call output read 只能靠 fallback，优先把真实高频形状补到 `memory_read`。
2. 看 `memory_allocation` marker 是否能被 return / external-call 的 buffer role 消费，避免 allocation 只做 oracle。
3. 对 `memory_byte_write` 找消费者，先确认 mstore8 在真实样例里主要服务哪个 ABI/event/revert payload。

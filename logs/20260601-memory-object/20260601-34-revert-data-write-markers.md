# Revert data write markers

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮补 revert payload 的专门写入 marker。上一轮已经有 `notdec_solidity_revert_memory_consumer(base, size, kind)`，但 revert payload 写入还没有和 return/event/external-call 一样落到专门 marker。

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:238`，`YulUtilFunctions::revertWithError()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4147`，`YulUtilFunctions::forwardingRevertFunction()`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4598`，`YulUtilFunctions::revertReasonIfDebugBody()`

关键逻辑：

- `revertWithError()` 会准备 error selector 和 ABI 参数，最后 `revert(pos, sub(end, pos))`。
- `revertReasonIfDebugBody()` 用 `start` 做 base，按 `Error(string)` ABI 布局写 selector、offset、length、string words，然后 `revert(start, overallLength)`。
- `forwardingRevertFunction()` 用 `returndatacopy(pos, 0, returndatasize())` 填 buffer，再 `revert(pos, returndatasize())`。

所以 revert payload 写入可以分成两类：word write 和 copy write。它们应该消费已有 `notdec_solidity_memory_write` / `notdec_solidity_memory_copy_write`，而不是再从原始 `mstore` / `returndatacopy` 猜一次。

## 实现

新增 marker：

- `notdec_solidity_revert_data_word_write(base, offset, value, revertKind)`
- `notdec_solidity_revert_data_copy_write(base, offset, sourceOffset, size, sourceKind, revertKind)`

改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:4`
  - 增加 `SmallVector` include。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:19`
  - 增加 `NumRevertDataWordWrites` / `NumRevertDataCopyWrites` 统计。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:82`
  - 增加 `collectRevertDataWordWriteMarkers()`，只读取 `notdec_solidity_memory_write`。
  - 遇到 free pointer store 清空候选，避免跨 allocation 误挂。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:119`
  - 增加 `collectRevertDataCopyWriteMarkers()`，只读取 `notdec_solidity_memory_copy_write`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:156`
  - 增加 `insertRevertDataWordWriteMarker()`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:174`
  - 增加 `insertRevertDataCopyWriteMarker()`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:203`
  - 在成功读到 `notdec_solidity_memory_consumer(..., revert)` 后，继续读取同 base 的 write/copy marker，并插入 revert 专用 payload marker。
- `test/evm/solidity-patterns/manifest.json:451`
  - 给 `revert_error_string_01` 增加 `notdec_solidity_revert_data_word_write: 4` oracle。
- `test/evm/solidity-patterns/manifest.json:997`
  - 给 `0011_multi_public` 增加 `notdec_solidity_revert_data_word_write: 8` 和 `notdec_solidity_revert_data_copy_write: 2` oracle。

这轮是 IR rewrite 消费闭环：`SolidityRevertPass` 读取通用 memory write/copy marker，并生成 revert 专用 payload marker。metadata 没有参与 pass 间接口。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例统计：
  - `revert_error_string_01`
    - `notdec_solidity_revert_memory_consumer`: 1
    - `notdec_solidity_revert_data_word_write`: 4
    - `notdec_solidity_revert_memory_write_match`: 1
    - `notdec_solidity_memory_write`: 4
    - `notdec_solidity_memory_consumer`: 1
  - `0011_multi_public`
    - `notdec_solidity_revert_memory_consumer`: 5
    - `notdec_solidity_revert_data_word_write`: 8
    - `notdec_solidity_revert_data_copy_write`: 2
    - `notdec_solidity_revert_memory_write_match`: 2
    - `notdec_solidity_memory_write`: 9
    - `notdec_solidity_memory_copy_write`: 2
    - `notdec_solidity_memory_consumer`: 10
  - `0014_proxy_like`
    - `notdec_solidity_revert_data_word_write`: 0
    - `notdec_solidity_revert_data_copy_write`: 0
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，103.07s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_revert_data_word_write`: 4071
- `notdec_solidity_revert_data_copy_write`: 499
- `notdec_solidity_revert_memory_consumer`: 2772
- `notdec_solidity_revert_memory_write_match`: 1123
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_copy_write`: 783
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_abi_return_data_word_write`: 1422
- `notdec_solidity_abi_return_data_copy_write`: 22
- `notdec_solidity_event_data_word_write`: 463
- `notdec_solidity_event_data_copy_write`: 5
- `notdec_solidity_external_call_input_word_write`: 308
- `notdec_solidity_external_call_input_copy_write`: 4
- `notdec_solidity_external_call_output_copy_write`: 8

新增 revert payload marker 在固定真实样例里不是 0 命中，说明这轮不是只服务小样例。

## 下一步

1. 让 revert selector / panic / error-string rewrite 优先读取 `notdec_solidity_revert_data_word_write`，减少对旧 `SolidityRevertMatch` 的依赖。
2. 补 ABI dynamic tail 的更细 marker，尤其 return/event/external-call 里的动态 bytes/string。
3. 看 `MemoryBufferAnalysis` 是否需要支持更远距离的 `allocate_unbounded` / `finalize_allocation` 绑定。

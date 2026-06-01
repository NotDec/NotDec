# External call output allocation marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮让 external-call output 消费 `notdec_solidity_memory_allocation`。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2655`，`IRGeneratorForStatements::appendExternalFunctionCall()`

关键逻辑：

- external call 使用 `pos` 作为 input base，也使用 `pos` 作为 output base。
- 动态返回值时，成功后执行 `returndatacopy(pos, 0, returnDataSize)`。
- 随后执行 `finalizeAllocation(pos, returnDataSize)`。
- 最后执行 `abiDecode(pos, add(pos, returnDataSize))`。

所以 output buffer 不只是被 copy/write/read 消费，还会驱动 free memory pointer 的 finalize。已有 `notdec_solidity_memory_allocation(base, size)` 能表达 finalize，但此前没有 external-call output 专门读取它，allocation 主要停在通用 marker / oracle。

## 实现

新增 marker：

- `notdec_solidity_external_call_output_allocation(base, allocSize, outputSize, callKind)`

改动：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:27`
  - 增加 `NumExternalCallOutputAllocations` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:371`
  - 增加 `findExternalCallOutputAllocationMarker()`。
  - 只在 external call 后扫描同 block 和直接 successor。
  - 遇到 free memory pointer store 或新的 external call 就停止。
  - 要求 allocation marker 的 base 与 output base 相同或是重新读取的 free memory base。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:502`
  - 增加 `insertExternalCallOutputAllocationMarker()`。
  - 参数来自已存在的 `notdec_solidity_memory_allocation`，并补上 external call 的 output size 和 call kind。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:676`
  - 只有当前 call 已经确认有 output decode buffer 时，才尝试绑定 allocation，避免把无返回值 call 后面的无关 allocation 误挂上。
- `test/evm/solidity-patterns/manifest.json:1901`
  - 给 `0258_19493864_78edd9f88e_b098925cbd2d` 增加 `notdec_solidity_external_call_output_allocation: 6` oracle。

这轮是 IR rewrite 消费闭环：`ExternalCallPass` 消费通用 allocation marker，生成 external-call output 专用 allocation marker。metadata 没有作为接口。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0258_19493864_78edd9f88e_b098925cbd2d`
    - `notdec_solidity_external_call_output_copy_write`: 6
    - `notdec_solidity_external_call_output_abi_decode_buffer`: 6
    - `notdec_solidity_external_call_output_allocation`: 6
    - `notdec_solidity_memory_allocation`: 58
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，104.54s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_external_call_output_allocation`: 8
- `notdec_solidity_external_call_output_copy_write`: 8
- `notdec_solidity_external_call_output_abi_decode_buffer`: 10
- `notdec_solidity_external_call_output_word_read`: 2
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_copy_write`: 783
- `notdec_solidity_memory_read`: 1269

新 marker 在固定 100 例里不是 0 命中，并且和 dynamic output copy 的 8 次命中对齐。

## 下一步

1. 看 ABI return / event / revert 是否也应该消费 `notdec_solidity_memory_allocation`。
2. 继续检查 `memory_byte_write` 的真实用途，确认是否应接入 event / return / revert payload。
3. 把 external call 里重复的 base/marker 扫描逻辑逐步收进共享 helper，前提是不会把简单逻辑抽得太重。

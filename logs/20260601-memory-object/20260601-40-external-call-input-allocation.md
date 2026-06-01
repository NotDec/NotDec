# External call input allocation marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮让 external call input 消费 `notdec_solidity_memory_allocation`。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2708`，普通 external call。
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:1707`，precompile call。

关键逻辑：

- 普通 external call：
  - `let pos := allocateUnbounded()`
  - `mstore(pos, shl28(selector))`
  - `let end := encodeArgs(add(pos, 4), args...)`
  - `call(..., pos, sub(end, pos), pos, staticReturndataSize)`
- precompile call：
  - `let pos := allocateUnbounded()`
  - `let end := encodeArgs(pos, args...)`
  - `staticcall(..., pos, sub(end, pos), 0, 32)`

所以 external call input buffer 的 allocation、ABI 写入和 call input consumer 是同一条链。此前 `ExternalCallPass` 已经消费了 input consumer、word write、ABI head write 和 copy write，但还没有消费 input allocation。

## 实现

新增 marker：

- `notdec_solidity_external_call_input_allocation(base, allocSize, inputSize, callKind)`

改动：

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:27`
  - 增加 `NumExternalCallInputAllocations` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:426`
  - 增加 `findExternalCallInputAllocationMarker()`。
  - allocation base 必须匹配已经确认的 input copy write、selector word write 或 ABI head write。
  - 匹配允许同一个 free pointer reload，沿用 external call input write 的现有判断。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:571`
  - 增加 `insertExternalCallInputAllocationMarker()`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:698`
  - 保存 input copy/word/head write 的结果，供 allocation 绑定使用。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:722`
  - input writes 收集后，消费 matching `notdec_solidity_memory_allocation` 并生成 external-call 专用 allocation marker。
- `test/evm/solidity-patterns/manifest.json:1181`
  - 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_external_call_input_allocation: 3` oracle。

这轮是 IR rewrite 消费闭环：`ExternalCallPass` 消费通用 allocation marker，生成 external call input 专用 allocation marker。metadata 没有作为接口。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0448_19495059_065877b669_4f138305be23`
    - `notdec_solidity_external_call_input_allocation`: 3
    - `notdec_solidity_external_call_input_word_write`: 6
    - `notdec_solidity_external_call_input_abi_head_write`: 10
    - `notdec_solidity_external_call_memory_consumer`: 16
    - `notdec_solidity_memory_allocation`: 13
  - `0258_19493864_78edd9f88e_b098925cbd2d`
    - `notdec_solidity_external_call_input_allocation`: 7
    - `notdec_solidity_external_call_output_allocation`: 6
    - `notdec_solidity_external_call_input_word_write`: 44
    - `notdec_solidity_external_call_input_abi_head_write`: 7
    - `notdec_solidity_external_call_memory_consumer`: 114
    - `notdec_solidity_memory_allocation`: 58
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，107.56s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_external_call_input_allocation`: 111
- `notdec_solidity_external_call_input_word_write`: 308
- `notdec_solidity_external_call_input_abi_head_write`: 290
- `notdec_solidity_external_call_input_copy_write`: 4
- `notdec_solidity_external_call_memory_consumer`: 1105
- `notdec_solidity_external_call_output_allocation`: 8
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_consumer`: 5736

新 marker 在固定 100 例里不是 0 命中，但数量明显少于 external call input consumer。原因是这轮要求 allocation base 先和已确认的 input write/copy/head write 绑定，不只靠 call 的 input base 猜。

## 下一步

1. Revert data allocation 需要单独处理跨 helper 形状：很多样例是在 helper 里分配和编码，caller 再 `revert(pos, size)`。
2. 继续看 external call input 未绑定 allocation 的样例，区分无 allocation marker、跨块、还是 input 只有常量 scratch。
3. 对 `memory_byte_write` 继续分类，优先看 bytes/string return 或 event payload 的真实 IR 形状。

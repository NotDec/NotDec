# Event data allocation marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮让 event data 消费 `notdec_solidity_memory_allocation`。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:1067`，event 分支。

关键逻辑：

- event 的 non-indexed 参数会收集到 `nonIndexedArgs` / `nonIndexedArgTypes`。
- 生成 Yul 时先 `let pos := allocateUnbounded()`。
- 然后 `let end := encode(pos, nonIndexedArgs...)`。
- 最后执行 `logN(pos, sub(end, pos), indexedArgs...)`。

所以 event data buffer 的 base、size、writes 和 allocation 是一条链。此前 `EventLogPass` 已经消费了 `memory_consumer`、`memory_write`、`memory_copy_write`，但还没有消费 `memory_allocation`。

## 实现

新增 marker：

- `notdec_solidity_event_data_allocation(base, allocSize, dataSize, topicCount)`

改动：

- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:20`
  - 增加 `NumEventDataAllocations` 统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:54`
  - 增加 `isSameEventDataBase()`。
  - 这里只允许同一个 SSA 值或同一个常量地址，不使用 free-pointer reload 的宽松匹配。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:131`
  - 增加 `findEventDataAllocationMarker()`。
  - 先用已有 event data word/copy writes 确认 allocation base 确实参与了 event payload。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:187`
  - 增加 `insertEventDataAllocationMarker()`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:277`
  - event consumer 命中后，在已收集 word/copy writes 的基础上，消费 matching `notdec_solidity_memory_allocation`。
- `test/evm/solidity-patterns/cases/25546_19769987_0d6f2c127b_61e5a573fda0.ll`
  - 新增固定 100 例里的真实 IR fixture。
- `test/evm/solidity-patterns/manifest.json:5224`
  - 给该 fixture 增加 `notdec_solidity_event_data_allocation: 1` oracle。

这轮是 IR rewrite 消费闭环：`EventLogPass` 消费通用 allocation marker，生成 event 专用 allocation marker。metadata 没有作为接口。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `25546_19769987_0d6f2c127b_61e5a573fda0`
    - `notdec_solidity_event_data_allocation`: 1
    - `notdec_solidity_event_data_copy_write`: 1
    - `notdec_solidity_event_data_word_write`: 4
    - `notdec_solidity_event_memory_consumer`: 8
    - `notdec_solidity_memory_allocation`: 11
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `git diff --check`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，106.26s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_event_data_allocation`: 9
- `notdec_solidity_event_data_word_write`: 463
- `notdec_solidity_event_data_copy_write`: 5
- `notdec_solidity_event_memory_consumer`: 439
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_external_call_output_allocation`: 8
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_write`: 8902

新 marker 在固定 100 例里不是 0 命中，但数量明显少于 event consumer。原因是这轮要求 allocation base 和 event data write/copy base 严格一致，避免误配。

## 下一步

1. 看 ABI return 是否也能保守消费 `memory_allocation`，但要避免跨函数和 helper encode 误配。
2. 对 `memory_byte_write` 继续按真实用途分类；目前多是循环写 bytes/string，不能简单挂到 event/revert/return。
3. 检查 `memory_read` 在 ABI decode 之外的使用，看看是否能支持 memory array length / element read 的专门 marker。

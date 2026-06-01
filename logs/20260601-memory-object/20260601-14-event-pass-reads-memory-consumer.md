# Event pass reads memory consumer marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮已经能给 event data buffer 插入 `notdec_solidity_memory_consumer(base, size, event_log)`，但 `EventLogPass` 仍然只看原始 `evm_logN` 并打 metadata。

本轮把 `EventLogPass` 接到 memory rewrite surface：

- 在同一 basic block 里查找 `notdec_solidity_memory_consumer(..., event_log)`。
- 要求 marker 的 base / size 和 `evm_logN` 的 data base / size 一致。
- 命中后在 `evm_logN` 前插入 `notdec_solidity_event_memory_consumer(base, size, topicCount)`。

这个 marker 是 proof marker，表示 event pass 确实读到了 memory consumer rewrite。原有 event metadata 保留。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp` 的 event 分支生成：

- `let pos := allocate_unbounded()`
- `let end := abi_encode_tuple_*(pos, ...)`
- `logN(pos, sub(end, pos), topics...)`

`/sn640/solidity/libsolidity/codegen/ExpressionCompiler.cpp` 的传统 codegen 也是先取 free memory pointer，再 ABI encode non-indexed 参数，最后 `LOGn` 消费 `(memstart, memsize)`。

判断：`evm_logN` 的 data base / size 是明确的 memory consumer。event pass 不应该只重新看原始 `evm_logN`，也应该读取 memory pass 已经插入的 consumer marker。

## 实现记录

- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:16` 增加 `NumEventMemoryConsumers` 统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:21` 增加 `findEventConsumerMarker()`，只匹配同 block 内、位于 `evm_logN` 前、kind 为 event log 且 base / size 相同的 memory consumer marker。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:41` 增加 `insertEventMemoryConsumerMarker()`，插入 `notdec_solidity_event_memory_consumer(base, size, topicCount)`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:71` 在处理 `evm_log0` 到 `evm_log4` 时读取 memory marker，命中后插入 proof marker。
- `test/evm/solidity-patterns/manifest.json:922` 给 `0014_proxy_like` 增加 `notdec_solidity_event_memory_consumer = 1` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.08s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-event-marker-consumer`。
  - 100/100 成功。
  - `notdec_solidity_event_memory_consumer`：439。
  - `notdec_solidity_memory_consumer`：5736，其中 event_log consumer 439。

## 当前限制

- 只读取同一 basic block 内、位于 `evm_logN` 前面的 consumer marker。
- 只证明 event data buffer 被消费，不恢复 event topic 和 non-indexed payload 字段。
- 如果后续 `MemoryBufferRewritePass` 能识别更多 event consumer，这个 pass 会自然跟进；但跨块 marker 还不会匹配。

## 下一轮候选

1. 让 `ExternalCallPass` 读取 input / output memory consumer marker，并插入 proof marker。
2. 审计 100-case 中 event / external call payload 的 write marker 覆盖，看看是否值得开始恢复 ABI head。
3. 改进 base 识别，处理 helper 参数或 helper 返回值作为 buffer base 的 return / event / call。

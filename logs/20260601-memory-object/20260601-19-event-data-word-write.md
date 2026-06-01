# Event data word write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

`EventLogPass` 已经能读取 `notdec_solidity_memory_consumer(..., event_log)`，但还没有读取 event data payload 的 writes。

本轮接 event data 的 word write：

- 在 `evm_logN` 前查找 `notdec_solidity_memory_write(base, offset, value)`。
- 只接受常量 offset，且 `offset % 32 == 0`。
- 要求 write 和 `evm_logN` 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_event_data_word_write(base, offset, value, topicCount)`。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp` 的 event 分支：

- indexed 参数进入 topics。
- non-indexed 参数通过 `abi_encode_tuple_*(pos, ...)` 写入 memory。
- 最后生成 `logN(pos, sub(end, pos), topics...)`。

传统 codegen 的 `ExpressionCompiler.cpp` 也是先取 free memory pointer，再 ABI encode non-indexed 参数，最后 `LOGn` 消费 `(memstart, memsize)`。

判断：event data 的 non-indexed 参数 ABI head 从 offset 0 开始，间隔 32 字节。`notdec_solidity_memory_write(base, 32*n, value)` 是明确的 event data payload write，应该被 `EventLogPass` 消费。

## 实现记录

- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:3` 增加 `SmallVector` include。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:20` 增加 `NumEventDataWordWrites` 统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:45` 增加 free pointer load/store helper。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:57` 增加 `getUInt64Constant()`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:65` 增加 `isEventAbiHeadOffset()`，只接受 `32 * n` offset。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:70` 增加 `collectEventDataWordWriteMarkers()`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:121` 增加 `insertEventDataWordWriteMarker()`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:161` 在 event consumer 命中后读取 event data word writes。
- `test/evm/solidity-patterns/manifest.json:1171` 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_event_data_word_write = 8` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.79s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-event-data-word`。
  - 100/100 成功。
  - `notdec_solidity_event_data_word_write`：463。
  - offset 分布：0 有 252 个，32 有 113 个，64 有 43 个，96 有 31 个，128 有 12 个，160 有 6 个，192 有 6 个。

## 当前限制

- 只接常量 offset 的 word write。
- 只在同一 basic block 内查找。
- 不解析 event 参数类型，也不恢复 dynamic tail。
- free pointer reload 等价仍在 `EventLogPass` 内部实现，后续应该下沉到 `MemoryBufferAnalysis`。

## 下一轮候选

1. 把 free pointer reload 等价和 marker 查找逻辑下沉到 `MemoryBufferAnalysis`。
2. 审计 ABI return payload 的 write marker，找能稳定接入的 return word/copy proof。
3. 继续接 event data 的 copy write，用于 bytes/string 或 indexed reference hash 前的 packed encode。

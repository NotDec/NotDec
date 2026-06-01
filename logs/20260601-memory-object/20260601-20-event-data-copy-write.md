# Event data copy write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮 `EventLogPass` 已经读取 event data 的 word writes。本轮补 copy write：

- 在 `evm_logN` 前查找 `notdec_solidity_memory_copy_write(base, offset, sourceOffset, size, copyKind)`。
- 只接受常量 offset，且 `offset % 32 == 0`。
- 要求 copy write 和 `evm_logN` 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_event_data_copy_write(base, offset, sourceOffset, size, copyKind, topicCount)`。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp` 的 event 分支把 non-indexed 参数 ABI encode 到 memory，再执行 `logN(pos, sub(end, pos), topics...)`。

当 event data 里有 bytes/string 或来自 code/calldata/returndata 的数据块时，payload 不一定全是 `mstore`，也可能是 `calldatacopy`、`returndatacopy`、`codecopy` 或 `mcopy`。这些已经由 memory pass 转成 `notdec_solidity_memory_copy_write`，EventLogPass 应该消费它。

## 实现记录

- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:22` 增加 `NumEventDataCopyWrites` 统计。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:108` 增加 `collectEventDataCopyWriteMarkers()`，查找同 block 内 event log 前的 copy write marker。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:176` 增加 `insertEventDataCopyWriteMarker()`。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:224` 在 event consumer 命中后读取 event data copy writes。
- `test/evm/solidity-patterns/manifest.json:1240` 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 `notdec_solidity_event_data_copy_write = 2` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，100.88s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-event-data-copy`。
  - 100/100 成功。
  - `notdec_solidity_event_data_copy_write`：5。
  - offset 分布：32 有 3 个，64 有 2 个。
  - copy kind 全部是 `codecopy`。

## 当前限制

- 只接常量 offset 且 `offset % 32 == 0` 的 copy write。
- 只在同一 basic block 内查找。
- 目前 100-case 命中很少，说明 event copy payload 不是最高频问题。
- free pointer reload 等价仍在 `EventLogPass` 内部实现，后续应该下沉到 `MemoryBufferAnalysis`。

## 下一轮候选

1. 把 free pointer reload 等价和 marker 查找逻辑下沉到 `MemoryBufferAnalysis`。
2. 审计 ABI return payload 的 write/copy marker，找能稳定接入的 return proof。
3. 继续外部调用 input 的动态 tail / copy write，覆盖更复杂 ABI 参数。

# Memory event log consumer rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

在上一轮 return / revert consumer 基础上，接入 Solidity event log 的 data buffer consumer。

本轮只处理确定性高的形状：

- `evm_log0` 到 `evm_log4`。
- log 参数中的 `dataBase` 是 `evm_mload(mem, 64)`。
- `dataSize` 原样保留，常见是 `sub(end, base)` 或 `0`。
- 插入 `notdec_solidity_memory_consumer(base, size, kind)`，其中 event log kind 单独编号。

不处理 indexed topic 的含义，也不迁移 `EventLogPass` 的 metadata 逻辑。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp` 的 event 分支：

- 非 anonymous event 先把 event signature hash 放进 indexed topic。
- indexed 参数直接进 topic；引用类型 indexed 参数先 packed encode 到 memory，再取 keccak。
- non-indexed 参数统一 ABI encode 到 memory。
- Yul 模板是：
  - `let pos := allocate_unbounded()`
  - `let end := abi_encode_tuple_*(pos, ...)`
  - `logN(pos, sub(end, pos), topics...)`

`/sn640/solidity/libsolidity/codegen/ExpressionCompiler.cpp` 的传统 codegen 也类似：

- 先处理 indexed 参数。
- 对 non-indexed 参数执行 `fetchFreeMemoryPointer()` + `abiEncode()`。
- `toSizeAfterFreeMemoryPointer()` 后调用 `LOGn`。

判断：`logN(base, size, topics...)` 里的 `(base, size)` 是明确的 memory buffer consumer。它不是业务分支，也不只是 metadata，应该进入 memory rewrite surface。

## 计划

1. 给 `MemoryConsumerKind` 增加 event log kind。
2. `analyzeMemoryBuffers()` 识别 `evm_log0` 到 `evm_log4` 的 data base / size。
3. 复用已有 `notdec_solidity_memory_consumer` marker，不新增另一套 marker。
4. runner 支持 event consumer kind oracle。
5. 选一个已有 Solidity-generated event case 加 oracle。

## 风险

- `logN(..., mload(0x40), 0, topics...)` 是合法 event，无 data write；consumer marker 仍应存在。
- indexed reference 参数也可能临时使用 free pointer 做 packed encode + keccak，这一轮只标最终 `logN` data buffer，不把 topic hash 临时 buffer当 consumer。
- 真实 IR 里有些 log data size 是 `add` 或 `sub` 表达式，本轮只绑定 base，不判断 size 表达式含义。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0014_proxy_like.ll -o /tmp/notdec-memory-event-0014.ll --tr-level=0`。
  - 输出 3 个 `notdec_solidity_memory_consumer`：2 个 return，1 个 event_log。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:22` 给 `MemoryConsumerKind` 增加 `EventLog = 3`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:49` 更新 consumer 注释，明确 return / revert / event data 先支持，external call 后续再做。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:43` 增加 `isEvmLogCall()`，只接受 `evm_log0` 到 `evm_log4`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:233` 在 `analyzeMemoryBuffers()` 中识别 log data buffer，并插入 `MemoryConsumerKind::EventLog`。
- `test/run_evm_solidity_patterns_suite.py:296` 给 memory consumer kind oracle 增加 `event_log`。
- `test/evm/solidity-patterns/manifest.json:920` 给 `0014_proxy_like` 增加 memory consumer oracle，覆盖 2 个 return consumer 和 1 个 event log consumer。

## 当前限制

- event topic 只保留在原始 `evm_logN` 参数里，本轮不恢复 topic 语义。
- indexed reference 参数内部的 packed encode + keccak 临时 buffer 没有单独建模。
- 只在 log data base 直接是 `mload(0x40)` 时标 consumer；等价 base、PHI base、跨块 base 暂不追。
- 仍是 marker rewrite surface，还没有把 `EventLogPass` 改成直接消费 memory facts。

## 评分

- 实现效果：6/10。event data consumer 已进入 memory rewrite surface，并有 Solidity-generated oracle；但还没有恢复 event payload 结构。
- 理解成本：4/10。新增逻辑很窄，沿用已有 consumer marker，没有引入新表。
- 维护成本：4/10。规则保守，误判风险低；后续扩 topic / payload 时需要更多 oracle。

## 下一轮候选

1. external call input/output consumer。它在真实样例里频率高，也符合 PLAN 里“同 base 多 role”的核心问题。
2. copy write：接 `evm_returndatacopy` / `evm_calldatacopy(base + C, ..., size)`，否则 bytes/string/event 动态数据只能看到部分 `mstore`。
3. allocation finalize 归一：处理 `base + roundUp(size)` 和 checked-bounds 识别到的 Panic `0x41` guard。
4. 把 `EventLogPass` / `SolidityRevertPass` 的一部分重复 memory 解析迁到 `MemoryBufferAnalysis`。

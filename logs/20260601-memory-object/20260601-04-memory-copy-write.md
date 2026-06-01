# Memory copy write rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

补齐 memory write 里的 copy 写入：

- `evm_calldatacopy(mem, calldata, base + C, src, size)`。
- `evm_returndatacopy(mem, returndata, base + C, src, size)`。

本轮仍只处理 `base` 是 `evm_mload(mem, 64)` 且 offset 是常量的形状。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()`：

- external call 用 `pos := allocate_unbounded()` 构造 input buffer。
- 动态返回时成功块里生成 `returndatacopy(pos, 0, returnDataSize)`。
- 后续再 `finalizeAllocation(pos, returnDataSize)` 并从 `pos` ABI decode。

`/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp::returnDataToArray()`：

- `v := mload(0x40)`。
- `mstore(v, returndatasize())`。
- `returndatacopy(add(v, 0x20), 0, returndatasize())`。

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- forwarding revert：`pos := allocate_unbounded(); returndatacopy(pos, 0, returndatasize()); revert(pos, returndatasize())`。
- calldata arg copy：`memoryDataOffset := allocate(argSize); calldatacopy(memoryDataOffset, 0, argSize)`。

判断：copy 指令是编译器生成的 buffer write。只看 `mstore` 会漏掉 calldata forwarding、dynamic bytes/string、returndata forwarding 这些高频形状。

## 计划

1. 给 `MemoryWriteKind` 增加 calldata / returndata copy。
2. `MemoryWrite` 记录 copy 的 source offset。
3. 继续保留原有 `notdec_solidity_memory_write(base, offset, value)` 作为 word write marker。
4. 新增 `notdec_solidity_memory_copy_write(base, offset, src, size, kind)` 表达 copy write。
5. 在 `0014_proxy_like` 上加 oracle，覆盖 1 个 calldata copy 和 1 个 returndata copy。

## 风险

- copy marker 是新 surface，后续消费者要明确区分 word write 和 copy write。
- 当前只支持常量 offset；`base + symbolic` 的 copy 暂时不标。
- `calldatacopy` 也可能用于初始化/清零，本轮只挂到 free-memory base，不处理 scratch。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0014_proxy_like.ll -o /tmp/notdec-memory-copy-0014.ll --tr-level=0`。
  - 输出 2 个 `notdec_solidity_memory_copy_write`：1 个 calldata copy，1 个 returndata copy。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:18` 给 `MemoryWriteKind` 增加 `CalldataCopy = 2` 和 `ReturndataCopy = 3`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:43` 更新 `MemoryWrite` 注释；`include/notdec/Passes/evm/MemoryBufferAnalysis.h:51` 增加 `SourceOffset`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:148` 扩展 `insertWriteMarker()`，保留原 `notdec_solidity_memory_write(base, offset, value)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:169` 新增 `notdec_solidity_memory_copy_write(base, offset, src, size, kind)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:257` 在 `analyzeMemoryBuffers()` 中识别 `evm_calldatacopy` / `evm_returndatacopy`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:266` 根据 opcode 区分 calldata / returndata copy；`src/Passes/evm/MemoryBufferAnalysis.cpp:269` 记录 dst offset、src offset 和 size。
- `test/run_evm_solidity_patterns_suite.py:318` 增加 copy write kind 计数。
- `test/run_evm_solidity_patterns_suite.py:742` 和 `test/run_evm_solidity_patterns_suite.py:884` 接入 expected / actual oracle。
- `test/evm/solidity-patterns/manifest.json:920` 给 `0014_proxy_like` 增加 copy write marker 和 kind oracle。

## 当前限制

- 只处理 `dst = base` 或 `dst = base + const`。复杂 symbolic offset 暂时不标。
- copy marker 还没有被 ABI return / revert / external call pass 消费，只是 rewrite surface。
- `codecopy` 还没接入。当前先覆盖 calldata / returndata，因为它们直接对应 ABI input/output 和 revert forwarding。
- 还没有对固定 100 个 apehex 样例做覆盖率 audit。

## 评分

- 实现效果：7/10。copy write 已进入 IR rewrite surface，覆盖 proxy fallback 和 returndata array 的关键形状。
- 理解成本：5/10。新增一个 copy marker，语义比复用 word write marker 更清楚。
- 维护成本：5/10。参数固定，误判风险低；后续要让消费者真正使用这些 facts。

## 下一轮候选

1. 做固定 100 个 apehex 样例 memory marker audit，找最高频漏掉形状，避免只围着 manifest 小样例转。
2. allocation finalize 归一，处理 `base + roundUp(size)` 和 Panic `0x41` guard 后写回 `0x40`。
3. 让 `SolidityRevertPass` / `ExternalCallPass` 读取 memory facts，先迁一小块重复解析逻辑。
4. 接 `evm_codecopy`，覆盖 embedded literal string / metadata copy 到 memory 的形状。

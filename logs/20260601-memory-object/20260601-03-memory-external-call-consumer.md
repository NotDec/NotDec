# Memory external call consumer rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

接入 external call 的 input / output memory buffer consumer。

本轮只处理 EVM call opcode wrapper 的确定性参数：

- `evm_call(mem, returndata, env, gas, addr, value, inBase, inSize, outBase, outSize)`。
- `evm_callcode(mem, returndata, env, gas, addr, value, inBase, inSize, outBase, outSize)`。
- `evm_delegatecall(mem, returndata, env, gas, addr, inBase, inSize, outBase, outSize)`。
- `evm_staticcall(mem, returndata, env, gas, addr, inBase, inSize, outBase, outSize)`。

只在 `inBase` / `outBase` 直接是 `evm_mload(mem, 64)` 时插入 consumer marker。`outSize = 0` 也保留，因为这正是 Solidity 常见的“先用同一个 pos，动态 returndata 后续再 copy”的形状。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()`：

- `let pos := allocate_unbounded()`。
- `mstore(pos, shl28(funSel))`。
- `let end := encodeArgs(add(pos, 4), args...)`。
- 非 EOF call 形状是 `call(gas, address, value, pos, sub(end, pos), pos, staticReturndataSize)`。
- delegatecall / staticcall 没有 value 参数，但同样传 `pos, sub(end,pos), pos, staticReturndataSize`。
- 动态 returndata 时，成功后 `returndatacopy(pos, 0, returnDataSize)`，再 `finalizeAllocation(pos, returnDataSize)`，然后 ABI decode。

判断：external call 是同一个 memory base 多 role 的核心样例。`pos` 先是 call input buffer，又常被复用作 output / returndata buffer。这里只插 marker，不删除原始 call。

## 计划

1. 给 `MemoryConsumerKind` 增加 external call input / output。
2. 按 call opcode 类型读取正确参数位置。
3. 对 input 和 output 分别插入 `notdec_solidity_memory_consumer(base, size, kind)`。
4. runner 增加 kind 名称。
5. 在 `0014_proxy_like` 上加 oracle，覆盖 delegatecall input/output 和已有 return/event consumer。

## 风险

- `outSize = 0` 不是“没有 output role”，可能表示动态 returndata 之后显式 `returndatacopy`，所以不能简单跳过。
- 只看 base 直接是 `mload(0x40)`，会漏掉等价 SSA value，但避免 dominance 和误绑问题。
- external call 后续的 `returndatacopy` / `finalizeAllocation` 还没建模，input/output marker 只是第一步。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0014_proxy_like.ll -o /tmp/notdec-memory-call-0014.ll --tr-level=0`。
  - 输出 5 个 `notdec_solidity_memory_consumer`：2 个 return、1 个 event_log、1 个 external_call_input、1 个 external_call_output。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:22` 给 `MemoryConsumerKind` 增加 `ExternalCallInput = 4` 和 `ExternalCallOutput = 5`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:51` 更新 consumer 注释，明确同一个 base 可以有多个 role。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:52` 增加 `ExternalCallMemoryArgs`，保存 call input/output base 和 size。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:59` 增加 `getExternalCallMemoryArgs()`，按 `evm_call` / `evm_callcode` 与 `evm_delegatecall` / `evm_staticcall` 的参数布局取 input/output。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:268` 在 `analyzeMemoryBuffers()` 中插入 external call input consumer。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:276` 在 `analyzeMemoryBuffers()` 中插入 external call output consumer。
- `test/run_evm_solidity_patterns_suite.py:296` 给 memory consumer kind oracle 增加 `external_call_input` / `external_call_output`。
- `test/evm/solidity-patterns/manifest.json:920` 更新 `0014_proxy_like` oracle，检查 5 个 consumer marker。

## 当前限制

- 只绑定 `inBase` / `outBase` 直接为 `mload(0x40)` 的形状。
- 只标 call 的 input/output role，还没有把后续 `returndatacopy(pos, 0, size)` 建成 copy write。
- `outSize = 0` 会保留 output consumer，这对动态 returndata 是有用信息，但后续需要结合 `returndatacopy` 判断真实返回长度。
- 还没有让 `ExternalCallPass` 消费 memory facts。

## 评分

- 实现效果：7/10。已经覆盖 external call 最关键的同 base input/output role，并用真实 proxy-like case 验证。
- 理解成本：5/10。参数位置按 opcode 分支写清楚了，但后续 EOF call / extcall 如果进入 IR 需要再扩。
- 维护成本：5/10。保守规则稳定；下一步接 copy write 后，consumer 和 returndata 会更完整。

## 下一轮候选

1. copy write：接 `evm_calldatacopy` / `evm_returndatacopy(base + C, ..., size)`，这是 external call 和 bytes/string/event payload 的明显缺口。
2. 在固定 100 个 apehex 样例上做 memory marker audit，统计 return/revert/event/call/copy 的命中和漏掉形状。
3. allocation finalize 归一：处理 `roundUp(size)` 和 Panic `0x41` guard 后写回 `0x40`。
4. 让 `ExternalCallPass` 从 `MemoryBufferAnalysis` 读取 input/output facts，减少重复猜 opcode 参数。

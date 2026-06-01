# External call pass reads memory consumer marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

前面已经能给 external call 的 input / output buffer 插入 `notdec_solidity_memory_consumer`，但 `ExternalCallPass` 仍然只看原始 `evm_call` / `evm_staticcall` / `evm_delegatecall` / `evm_callcode` 并打 metadata。

本轮把 `ExternalCallPass` 接到 memory rewrite surface：

- 读取同一 basic block 内、位于 call 前面的 memory consumer marker。
- input 要匹配 `notdec_solidity_memory_consumer(inBase, inSize, external_call_input)`。
- output 要匹配 `notdec_solidity_memory_consumer(outBase, outSize, external_call_output)`。
- 命中后插入 `notdec_solidity_external_call_memory_consumer(base, size, role, callKind)`。

其中 `role = 1` 表示 input，`role = 2` 表示 output；`callKind = 1/2/3/4` 分别表示 call / staticcall / delegatecall / callcode。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()` 生成的核心形状是：

- `let pos := allocate_unbounded()`
- `mstore(pos, shl28(funSel))`
- `let end := encodeArgs(add(pos, 4), args...)`
- 非 EOF call：`call(gas, address, value, pos, sub(end, pos), pos, staticReturndataSize)`
- delegatecall / staticcall 没有 value 参数，但同样传 `pos, sub(end, pos), pos, staticReturndataSize`
- 动态 returndata 时，成功后 `returndatacopy(pos, 0, returnDataSize)`，再 `finalizeAllocation(pos, returnDataSize)`

判断：同一个 `pos` 同时是 call input 和 output buffer，是 memory object 计划里最重要的多 role 例子之一。external call pass 应该读 memory pass 插入的 input/output role marker，而不是只在原始 call 上打 metadata。

## 实现记录

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:17` 增加 `NumExternalCallMemoryConsumers` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:22` 增加 `ExternalCallMemoryArgs`，记录 input/output base 和 size。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:29` 增加 `getExternalCallMemoryArgs()`，按 `evm_call` / `evm_callcode` 和 `evm_delegatecall` / `evm_staticcall` 的参数布局取 memory 参数。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:64` 增加 `findExternalCallConsumerMarker()`，只匹配同 block 内、位于 call 前、kind 和 base / size 都一致的 memory consumer marker。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:85` 增加 `insertExternalCallMemoryConsumerMarker()`，插入 proof marker。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:119` 在处理 external call 时分别读取 input / output marker。
- `test/evm/solidity-patterns/manifest.json:922` 给 `0014_proxy_like` 增加 `notdec_solidity_external_call_memory_consumer = 2` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，100.85s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-external-marker-consumer`。
  - 100/100 成功。
  - `notdec_solidity_external_call_memory_consumer`：1105。
  - external input role：558。
  - external output role：547。
  - 对应原始 memory consumer：`external_call_input = 558`，`external_call_output = 547`。

## 当前限制

- 只读取同一 basic block 内、位于 call 前面的 consumer marker。
- 只证明 input/output buffer role 已经被 external call pass 消费，不恢复 call payload 的 ABI head / tail。
- EOF `extcall` / `extdelegatecall` / `extstaticcall` 当前没有进入 NotDec wrapper 名称，本轮不处理。

## 下一轮候选

1. 审计 100-case 里 return / event / external call payload 的 write marker 覆盖，找最高频 base 识别缺口。
2. 改进 base 识别，处理 helper 参数或 helper 返回值作为 buffer base 的 return / event / call。
3. 开始让 ABI return 或 external call payload 读取 `notdec_solidity_memory_write`，恢复简单的一词 ABI head。

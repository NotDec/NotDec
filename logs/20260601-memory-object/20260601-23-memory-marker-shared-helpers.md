# 共享 memory marker helper

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

这轮不新增 marker，先整理已有消费者 pass 的公共判断。依据仍然是 Solidity 的 free memory pointer 形状：

- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp` 里 `fetchFreeMemoryPointer()` 生成 `mload(0x40)`，`storeFreeMemoryPointer()` 生成 `mstore(0x40, newPtr)`。
- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp` 的 public wrapper 用 `memPos := allocate_unbounded()` 后再 `return(memPos, sub(memEnd, memPos))`。
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp` 的 event 和 external call 也会围绕 `allocate_unbounded()` 返回的同一个 free pointer base 写 buffer。

所以 ABI return、event data、external call input 都需要同一套判断：两个 base 可以是同一个 SSA value，也可以是同一 block 里重新读出来的 `mload(0x40)`。遇到 `mstore(0x40, ...)` 后，之前候选必须清掉。

## 本轮改动

本轮只做维护性重构，不改变 marker 语义，不新增 metadata。

- `include/notdec/Passes/evm/SolidityPatternUtils.h`：导出 `getUInt64Constant`、`isFreeMemoryPointerLoad`、`isFreeMemoryPointerStore`、`isSameOrReloadedFreeMemoryBase`。
- `src/Passes/evm/SolidityPatterns.cpp`：把上面 helper 放到共享实现里，删除后面多余的本地前置声明。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp`：删除本地重复 helper，ABI return data word/copy marker 继续用同样的 base 等价判断。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp`：删除本地重复 helper，event data word/copy marker 继续用同样的 base 等价判断。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp`：删除本地重复 helper，external call input copy/word/head marker 继续用同样的 base 等价判断。

## 判断

这轮不是新的功能完成点，而是给已有 IR rewrite surface 降低维护成本。后续继续扩展 return / event / external call marker 时，不需要在三个 pass 里同步改同一套 free pointer 判断。

本轮预期行为不变，重点看这些已有 marker：

- `notdec_solidity_abi_return_data_word_write`
- `notdec_solidity_abi_return_data_copy_write`
- `notdec_solidity_event_data_word_write`
- `notdec_solidity_event_data_copy_write`
- `notdec_solidity_external_call_input_copy_write`
- `notdec_solidity_external_call_input_word_write`
- `notdec_solidity_external_call_input_abi_head_write`

固定 100 个样例验证时，不直接读取旧 `outputs/*.ll`，而是用当前 `build/bin/notdec` 从对应 `.bc` 重新生成临时 `.ll` 后统计。结果 100 个样例全部成功，命中如下：

- `notdec_solidity_abi_return_data_copy_write`: 43
- `notdec_solidity_abi_return_data_word_write`: 1493
- `notdec_solidity_event_data_copy_write`: 9
- `notdec_solidity_event_data_word_write`: 513
- `notdec_solidity_external_call_input_abi_head_write`: 321
- `notdec_solidity_external_call_input_word_write`: 358
- `notdec_solidity_external_call_input_copy_write`: 7
- `notdec_solidity_external_call_memory_consumer`: 1176
- `notdec_solidity_event_memory_consumer`: 513
- `notdec_solidity_memory_consumer`: 5830
- `notdec_solidity_revert_memory_write_match`: 1174
- `notdec_solidity_abi_return_memory_consumer`: 1513
- `notdec_solidity_memory_copy_write`: 867
- `notdec_solidity_memory_write`: 8994
- `notdec_solidity_memory_byte_write`: 11
- `notdec_solidity_memory_allocation`: 1131

旧批次目录里的 `outputs/*.ll` 不一定是当前 build 重新跑出的 pass 后 IR，不能直接当成当前 marker 统计口径。

## 下一轮候选

1. 优先看 ABI return / event / external call 是否还漏掉跨 block 的直接 marker 消费。
2. 再看 memory consumer 对 `finalize_allocation` 形状的绑定是否太弱。
3. 最后再考虑把同 block 扫描循环抽成共享 helper；这部分容易改过头，先不动。

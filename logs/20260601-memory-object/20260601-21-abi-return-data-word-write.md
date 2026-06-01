# ABI return data word write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

`AbiReturnPass` 已经读取 `notdec_solidity_memory_consumer(..., return)`，但还没有读取 return buffer 里的 payload writes。

本轮接 ABI return data 的 word write：

- 在 `evm_return` 前查找 `notdec_solidity_memory_write(base, offset, value)`。
- 只接受常量 offset，且 `offset % 32 == 0`。
- 要求 word write 和 `evm_return` 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_abi_return_data_word_write(base, offset, value, returnKind)`。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp` 的 public wrapper 返回路径：

- `memPos := allocate_unbounded()`
- `memEnd := abiEncode(memPos, returns...)`
- `return(memPos, sub(memEnd, memPos))`

ABI return 的 head 从 `memPos` 开始，常量 offset 是 `32 * n`。这些 `mstore` 已经由 memory pass 转成 `notdec_solidity_memory_write`，`AbiReturnPass` 应该消费它，而不是重新猜原始 memory 指令。

## 实现记录

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:3` 增加 `SmallVector` include。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:18` 增加 `NumAbiReturnDataWordWrites` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:63` 增加 free pointer load/store helper。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:75` 增加 `getUInt64Constant()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:83` 增加 `isAbiHeadOffset()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:88` 增加 `collectAbiReturnDataWordWriteMarkers()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:137` 增加 `insertAbiReturnDataWordWriteMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:171` 在 return consumer 命中后读取 ABI return data word writes。
- `test/evm/solidity-patterns/manifest.json:985` 给 `0011_multi_public` 增加 `notdec_solidity_abi_return_data_word_write = 1` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.70s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-abi-return-word`。
  - 100/100 成功。
  - `notdec_solidity_abi_return_data_word_write`：1422。
  - offset 分布：0 有 1064 个，32 有 187 个，64 有 67 个，96 有 39 个，128 有 27 个，160 有 15 个，192 有 14 个，224 有 8 个，256 有 1 个。

## 当前限制

- 只接常量 offset 的 word write。
- 只在同一 basic block 内查找。
- 不解析 return 类型，也不恢复 dynamic tail。
- free pointer reload 等价仍在 `AbiReturnPass` 内部实现，后续应该下沉到 `MemoryBufferAnalysis`。

## 下一轮候选

1. 接 ABI return data 的 copy write，用于 bytes/string 或 returndata forward。
2. 把 free pointer reload 等价和 marker 查找逻辑下沉到 `MemoryBufferAnalysis`。
3. 审计剩余 consumer 中没有 payload marker 的主要形状，决定是否继续补 event/external/return。

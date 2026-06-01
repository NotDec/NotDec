# ABI return data copy write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮 `AbiReturnPass` 已经读取 ABI return data 的 word writes。本轮补 copy write：

- 在 `evm_return` 前查找 `notdec_solidity_memory_copy_write(base, offset, sourceOffset, size, copyKind)`。
- 只接受常量 offset，且 `offset % 32 == 0`。
- 要求 copy write 和 `evm_return` 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_abi_return_data_copy_write(base, offset, sourceOffset, size, copyKind, returnKind)`。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp` 的 public wrapper 返回路径：

- `memPos := allocate_unbounded()`
- `memEnd := abiEncode(memPos, returns...)`
- `return(memPos, sub(memEnd, memPos))`

动态 bytes/string 或来自 code/calldata/returndata 的返回数据不一定只通过 `mstore` 生成，也可能通过 `calldatacopy`、`returndatacopy`、`codecopy` 或 `mcopy` 写入 return buffer。这些 copy 已经由 memory pass 转成 `notdec_solidity_memory_copy_write`，`AbiReturnPass` 应该消费它。

## 实现记录

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:20` 增加 `NumAbiReturnDataCopyWrites` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:125` 增加 `collectAbiReturnDataCopyWriteMarkers()`，查找同 block 内 return 前的 copy write marker。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:190` 增加 `insertAbiReturnDataCopyWriteMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:233` 在 return consumer 命中后读取 ABI return data copy writes。
- `test/evm/solidity-patterns/manifest.json:1172` 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_abi_return_data_copy_write = 1` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.33s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-abi-return-copy`。
  - 100/100 成功。
  - `notdec_solidity_abi_return_data_copy_write`：22。
  - offset 分布：32 有 20 个，64 有 2 个。
  - copy kind：`codecopy` 19 个，`mcopy` 2 个，`calldatacopy` 1 个。

## 当前限制

- 只接常量 offset 且 `offset % 32 == 0` 的 copy write。
- 只在同一 basic block 内查找。
- 目前 100-case 命中不高，但覆盖了明确的 dynamic return data copy surface。
- free pointer reload 等价仍在 `AbiReturnPass` 内部实现，后续应该下沉到 `MemoryBufferAnalysis`。

## 下一轮候选

1. 把 free pointer reload 等价和 marker 查找逻辑下沉到 `MemoryBufferAnalysis`。
2. 审计剩余没有 payload marker 的 return/event/external consumer，决定是否还要补细分 marker。
3. 开始把重复的 marker 搜索逻辑从 pass 内迁到共享 helper，降低维护成本。

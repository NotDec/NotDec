# ABI return memory dynamic array rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:766` 到 `:768` 的 public ABI wrapper 形状是：

- `memPos := allocateUnbounded()`
- `memEnd := abiEncode(memPos, returns...)`
- `return(memPos, sub(memEnd, memPos))`

动态 memory array 作为返回值时，ABI encoder 会从已有 memory array 读取 length 和元素，再写入 return buffer。`ABIFunctions.cpp:1120` 到 `:1127` 说明 `_fromMemory` decode / read 会用 `mload(offset)`；真实 IR 中会表现为从 source array 的 offset 0 读 length，再从 source array data 区域读元素。

这和上一轮的 `bytes_concat` builder 不同。`YulUtilFunctions.cpp:2675` 以后生成的是 builder helper，`outPtr := allocateUnbounded()` 后写 length 并 finalize。本轮处理的是 ABI wrapper 直接把已有 memory array 重编码到 return buffer，不把它误归为 concat builder。

## 问题

expanded200 里仍有 dynamic ABI return source 没有 ABI return 级别 rewrite。典型真实样例是 `25928_19774281_d048a8d52d_2758caa02f46`：

- `/tmp/notdec-memory-expand200/out/25928_19774281_d048a8d52d_2758caa02f46.ll:3524` 已有 `notdec_solidity_abi_return_dynamic_array_source`。
- `:3525` 已有 `notdec_solidity_abi_return_dynamic_array_memory_source`。
- 但它不是 literal source，也不是 storage source 或 memory builder，所以没有 `notdec_solidity_rewrite_abi_return_*` 级别 marker。

这不符合 PLAN 里的闭环要求：`memory_source` 只是 memory fact，消费者还需要产出 ABI return 级别 rewrite。

## 实现

本轮属于 `consumer rewrite`。

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:41` 增加统计 `NumAbiReturnMemoryDynamicArrayRewrites`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:897` 新增 `insertAbiReturnMemoryDynamicArrayRewriteMarker()`，插入：
  - `notdec_solidity_rewrite_abi_return_memory_dynamic_array(returnBase, sourceArray, length, sourceLengthWord, dataOffset, returnSize, copyKind, kind)`
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1248` 到 `:1289` 在已有 `AbiReturnDynamicArrayMemorySource` 分支里区分 literal / non-literal：
  - literal 仍走原来的 literal bytes rewrite。
  - non-literal 插入新的 memory dynamic array ABI return rewrite。

这个 marker 是 ABI return 级别的 rewrite，不是 metadata。它读取已有的 memory source 事实，并把 source array、length、data offset、return buffer size 和 copy kind 一起落到 IR 中。

## Oracle

新增真实 IR case：

- `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll`
- `test/evm/solidity-patterns/manifest.json:5268` 增加 oracle：
  - `notdec_solidity_abi_return_dynamic_array_source`: 4
  - `notdec_solidity_abi_return_dynamic_array_memory_source`: 2
  - `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 2

## 验证

已跑：

- `cmake --build ./build --target all -j4`
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-memory-round/25928.out.ll --tr-level=3`

单例输出确认：

- `--tr-level=3` 单例：
  - `notdec_solidity_abi_return_dynamic_array_source`: 4
  - `notdec_solidity_abi_return_dynamic_array_memory_source`: 2
  - `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 2
- suite 默认 `--tr-level=0` 单例 oracle：
  - `notdec_solidity_abi_return_dynamic_array_source`: 3
  - `notdec_solidity_abi_return_dynamic_array_memory_source`: 1
  - `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 1

完整验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 98 个 case 全部通过。
  - 总耗时 115.34 秒。

fixed100 audit，输入来自 batch639-641 的前 100 个 `.bc`，`--tr-level=3`：

- `notdec_solidity_abi_return_dynamic_array_source`: 31
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 14
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 14
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 3
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 14
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 14
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 14
- `notdec_solidity_rewrite_abi_return_memory_builder`: 3
- `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 0

expanded200 audit，`--tr-level=3`：

- `notdec_solidity_abi_return_dynamic_array_source`: 37
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 29
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 0
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 4
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 2
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 29
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 0
- `notdec_solidity_rewrite_abi_return_memory_builder`: 4
- `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 2

本轮没有删除低层 `mload/mstore/return`，只是补 ABI return 级别 semantic rewrite，不应改变既有执行语义。suite 耗时主要来自新增真实大 case，后续如果 suite 变慢明显，可以把这个 case 拆成更小 oracle。

## 下一轮候选

1. `25730_19772202_c569d7f701_8b569cf7465a` 里还有 helper-returned dynamic source，source array 来自 private helper 返回值，需要继续对照 ABI decode / helper 生成逻辑。
2. `25928` 里 `10658`、`10981` 这种 passthrough return 没有 `memory_source`，需要建模 loop copy 中的结构体 / array-of-struct 写入。
3. 把后续 lowering 接到 `notdec_solidity_rewrite_abi_return_memory_dynamic_array`，让 C 输出也能使用这个高层语义。

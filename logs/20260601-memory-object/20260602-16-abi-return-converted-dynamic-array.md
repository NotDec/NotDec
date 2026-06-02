# ABI return converted dynamic array rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:766` 到 `:768` 的 public ABI wrapper 仍然是本轮入口：先拿 `memPos`，再 `abiEncode(memPos, returns...)`，最后 `return(memPos, sub(memEnd, memPos))`。

本轮重点看 `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:525` 以后的 `abiEncodingFunctionSimpleArray()`。它对 memory array 的非 bytes/string 编码形状是：

- `pos := storeLength(pos, length)`。
- `baseRef := dataAreaFun(value)`。
- `srcPtr := baseRef`。
- `for { let i := 0 } lt(i, length) { i := add(i, 1) }`。
- loop 内读取 `arrayElementAccess`，再 `pos := encodeToMemoryFun(elementValues, pos)`。

所以真实 IR 里不一定是原样 `mload(source + index)` 直接 `mstore(return + index)`。中间可能有 `and`、`icmp`、`zext` 等清理 / 转换。这是 ABI encoder 的元素清理，不是业务分支。

## 问题

上一轮 expanded200 里 `25928_19774281_d048a8d52d_2758caa02f46` 还有两个 dynamic ABI return source 没有高层 rewrite。

典型形状：

- `/tmp/notdec-memory-expand200-r15/out/25928_19774281_d048a8d52d_2758caa02f46.ll:10659` 有 `notdec_solidity_abi_return_dynamic_array_source`。
- loop 从 source array data 区逐项 `mload`。
- 写 return buffer 时，元素先经过 mask / bool conversion，再 `mstore` 到 return buffer 的 PHI cursor。

旧 `copy_loop` 只识别纯 copy：store value 必须直接是 source `evm_mload`。它不适合这个 ABI encoder 清理元素的情况。

## 实现

本轮属于 `consumer rewrite`。

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:31` 增加 `NumAbiReturnDynamicArrayConvertedCopyLoops`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:33` 增加 `NumAbiReturnConvertedDynamicArrayRewrites`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:245` 增加 `AbiReturnDynamicArrayConvertedCopyLoop`，单独表示带清理 / 转换的逐元素 copy。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:489` 到 `:588` 增加 PHI cursor 匹配：
  - return cursor 必须从 `returnBase + 64` 开始，按 32 / 64 / 96 前进。
  - source cursor 必须从 `sourceArray + 32` 开始，按 32 前进。
  - store 的 value 里必须能追到这个 source cursor 上的 `evm_mload`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:916` 增加 `notdec_solidity_abi_return_dynamic_array_converted_copy_loop` marker。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:936` 增加 `notdec_solidity_rewrite_abi_return_converted_dynamic_array` marker。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1365` 到 `:1400` 在 dynamic source 分支里接入这个识别，并在非 storage source 时产出 ABI return 级 rewrite。

这个 rewrite 不推断元素类型，只表达“ABI return 正在把 memory dynamic array 逐元素清理 / 转换后编码出去”。它比纯 copy loop 更宽，但仍绑定在已识别的 `DynamicSource`、source cursor 和 return cursor 上。

## Oracle

复用上一轮新增的真实 IR case：

- `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll`
- `test/evm/solidity-patterns/manifest.json:5276` 增加：
  - `notdec_solidity_abi_return_dynamic_array_converted_copy_loop`: 1
  - `notdec_solidity_rewrite_abi_return_converted_dynamic_array`: 1

suite 默认 `--tr-level=0` 下，`25928` 的 call 计数为：

- `notdec_solidity_abi_return_dynamic_array_source`: 3
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 1
- `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 1
- `notdec_solidity_abi_return_dynamic_array_converted_copy_loop`: 1
- `notdec_solidity_rewrite_abi_return_converted_dynamic_array`: 1

`--tr-level=3` 单例下，新增 converted marker 是 2 个，正好覆盖 expanded200 里 `25928` 剩余两个 dynamic source。

## 验证

已跑：

- `cmake --build ./build --target all -j4`
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-memory-round/25928-converted.out.ll --tr-level=3`
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-memory-round/25928-converted-tr0.out.ll --tr-level=0`
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 98 个 case 全部通过。
  - 总耗时 115.32 秒。

fixed100 audit，输入来自 batch639-641 的前 100 个 `.bc`，`--tr-level=3`：

- `notdec_solidity_abi_return_dynamic_array_source`: 31
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 14
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 14
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 3
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 14
- `notdec_solidity_abi_return_dynamic_array_converted_copy_loop`: 0
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 14
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 14
- `notdec_solidity_rewrite_abi_return_memory_builder`: 3
- `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 0
- `notdec_solidity_rewrite_abi_return_converted_dynamic_array`: 0

expanded200 audit，`--tr-level=3`：

- `notdec_solidity_abi_return_dynamic_array_source`: 37
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 29
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 0
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 4
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 2
- `notdec_solidity_abi_return_dynamic_array_converted_copy_loop`: 2
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 29
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 0
- `notdec_solidity_rewrite_abi_return_memory_builder`: 4
- `notdec_solidity_rewrite_abi_return_memory_dynamic_array`: 2
- `notdec_solidity_rewrite_abi_return_converted_dynamic_array`: 2

expanded200 中 `notdec_solidity_abi_return_dynamic_array_source` 共有 37 个；storage / memory builder / memory dynamic array / converted dynamic array 的高层 rewrite 合计也是 37 个，本轮后该扩展样本里没有剩余 dynamic source 未闭环。

## 下一轮候选

1. 继续看 expanded200 里是否还有 dynamic source 没有对应 rewrite，优先找出现频率最高的形状。
2. 对 `25730_19772202_c569d7f701_8b569cf7465a` 的 helper-returned source 做源码对照，确认是 calldata decode helper、storage helper，还是 memory passthrough helper。
3. 后续 lowering 接入 `notdec_solidity_rewrite_abi_return_converted_dynamic_array`，让 C 输出不再重新扫描低层 loop。

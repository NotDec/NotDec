# ABI return dynamic array mcopy marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮处理新版 EVM 下动态 bytes/string ABI return 的 `mcopy` 形状。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:760`
  - public wrapper 仍然是 `allocateUnbounded()`、`abiEncode()`、`return(memPos, sub(memEnd, memPos))`。
- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:450`
  - 动态数组编码写 length 后调用 `copyFun(start, pos, length)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:103`
  - `copyToMemoryFunction(false, cleanup)` 在支持 `mcopy` 的 EVM 上生成：
    - `mcopy(dst, src, length)`
    - cleanup 时再 `mstore(add(dst, length), 0)`

上一轮只识别旧 EVM 展开的 loop：

- `mstore(returnBase + 64 + i, mload(sourceArray + 32 + i))`

fixed100 里还有一种真实形状：

- `memory_copy_write(returnBase, 64, sourceArray + 32, length, MemoryCopy)`
- 底层是 `evm_mcopy(returnBase + 64, sourceArray + 32, length)`

这同样是 ABI dynamic data 从 source array 复制到 return buffer，应该有 ABI return 专用 marker。

## 实现

新增 marker：

- `notdec_solidity_abi_return_dynamic_array_mcopy(returnBase, sourceArray, length, returnSize, kindCode)`

改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:29`
  - 增加 `NumAbiReturnDynamicArrayMCopies` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:211`
  - 增加 `AbiReturnDynamicArrayMCopy`，记录 return base、source array、length 和 copy marker。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:357`
  - 增加 `isSourceArrayDataStart()`，只接受 `sourceArray + 32`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:371`
  - 增加 `findAbiReturnDynamicArrayMCopy()`。
  - 消费已有 `notdec_solidity_memory_copy_write`，要求：
    - base 等于 return base；
    - offset 为 64；
    - source offset 为 `sourceArray + 32`；
    - size 等于动态数组 length；
    - copy kind 为 `MemoryCopy`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:446`
  - 增加 `insertAbiReturnDynamicArrayMCopyMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:569`
  - 在已有 dynamic source 命中后继续找 `mcopy` 形状。
- `test/evm/solidity-patterns/cases/25619_19770992_6b2850de20_21a0b617860a.ll`
  - 增加 fixed100 里的真实 IR fixture。
- `test/evm/solidity-patterns/manifest.json:5244`
  - 增加 25619 的 oracle，只检查本轮相关 marker：
    - `notdec_solidity_abi_return_dynamic_array_source: 2`
    - `notdec_solidity_abi_return_dynamic_array_mcopy: 2`
    - `notdec_solidity_abi_return_data_copy_write: 2`

这轮继续消费 `MemoryBufferRewritePass` 的 IR marker，不重新扫裸 `evm_mcopy`。它补上新版 EVM 的 memory-to-memory copy 路径，但 helper 包裹的 copy 仍未处理。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0450_19495071_6b6c9447e0_6344565f4b31`
    - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_dynamic_array_mcopy`: 0
  - `25619_19770992_6b2850de20_21a0b617860a`
    - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_data_copy_write`: 2
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，108.88s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- outputs: 100
- `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 14
- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_data_copy_write`: 22
- `notdec_solidity_memory_copy_write`: 783

`mcopy` 当前只补上 2 个直接形状。剩下 source 有但 copy marker 没有的样例主要是 helper 调用包住了 copy，需要跨 helper 处理。

## 下一步

1. 处理 helper 形状，例如 `private__0xf32(src, dst, length, ...)` 里封装的 memory copy。
2. 把 `memory_array_byte_write` 接到 `abi_return_dynamic_array_source`，建立 source array 内容来源。
3. 等 direct loop、mcopy、helper copy 都稳定后，再讨论隐藏低层 copy 指令和 cleanup store 的条件。

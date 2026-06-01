# ABI return dynamic array copy loop marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮处理动态 bytes/string ABI return 的 memory-to-memory copy loop。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:760`
  - public wrapper 使用 `allocateUnbounded()` 得到 return buffer。
  - `abiEncode(memPos, retParams...)` 写 ABI head/tail。
  - `return(memPos, sub(memEnd, memPos))` 消费该 buffer。
- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:450`
  - 动态数组编码写 length 后调用 `copyFun(start, pos, length)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:103`
  - 旧 EVM 没有 `mcopy` 时，`copy_memory_to_memory_with_cleanup` 展开为：
    - `for { } lt(i, length) { i := add(i, 32) }`
    - `mstore(add(dst, i), mload(add(src, i)))`
    - cleanup 时再 `mstore(add(dst, length), 0)`

真实 IR 里，`0450_19495071_6b6c9447e0_6344565f4b31` 的两个 public wrapper 都有：

- `returnBase + 64` 作为 ABI dynamic data 目的地址。
- `sourceArray + 32` 作为 memory bytes/string data 源地址。
- 循环体里 `mstore(returnBase + 64 + i, mload(sourceArray + 32 + i))`。
- 后续按 32 字节对齐并 `evm_return(returnBaseReload, returnSize)`。

这说明上一轮的 `abi_return_dynamic_array_source` 还不够，必须把实际 copy loop 也写成 IR marker，后续才能隐藏或替换低层 `mload/mstore` loop。

## 实现

新增 marker：

- `notdec_solidity_abi_return_dynamic_array_copy_loop(returnBase, sourceArray, length, returnSize, kindCode)`

改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:24`
  - 增加 `NumAbiReturnDynamicArrayCopyLoops` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:198`
  - 增加 `AbiReturnDynamicArrayCopyLoop`，记录 return base、source array、length、load、store。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:280`
  - 增加 `stripAddConstant()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:293`
  - 增加 `matchBasePlusConstantPlusIndex()`。
  - 只接受明确的 `base + constant + index` 形状。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:315`
  - 增加 `findAbiReturnDynamicArrayCopyLoop()`。
  - 匹配 `evm_mstore(returnBase + 64 + i, evm_mload(sourceArray + 32 + i))`。
  - 源和目的必须使用同一个 index SSA value。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:377`
  - 增加 `insertAbiReturnDynamicArrayCopyLoopMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:493`
  - 在已有 dynamic source marker 命中后继续找 copy loop，命中才插入新 marker。
- `test/evm/solidity-patterns/manifest.json:1258`
  - 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 `notdec_solidity_abi_return_dynamic_array_copy_loop: 2` oracle。

这轮比上一轮更接近闭环：ABI return pass 不只知道动态源数组，还确认了源数组 data 被复制到 return buffer 的动态 data 区。但它还没有隐藏/删除原始 loop，也还没把 `memory_array_byte_write` 接进返回值来源，所以仍不是完整最终形态。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0450_19495071_6b6c9447e0_6344565f4b31`
    - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_memory_consumer`: 2
    - `notdec_solidity_memory_array_byte_write`: 3
  - `0448_19495059_065877b669_4f138305be23`
    - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 0
    - `notdec_solidity_abi_return_dynamic_array_source`: 0
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，106.85s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- outputs: 100
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 14
- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_memory_array_byte_write`: 5
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_consumer`: 5736

`copy_loop` 少于 `dynamic_array_source`，因为这轮只接受旧 EVM 展开的明确 loop。`mcopy`、拆分 helper 或更复杂 index 形状暂时不接。

## 下一步

1. 把 `memory_array_byte_write` 接到 `abi_return_dynamic_array_source`，建立 source array 内容来源。
2. 处理 `mcopy(dst, src, length)` 形状，让新版 EVM 的 memory-to-memory copy 也能命中。
3. 等动态 return 的 head、source、copy loop 都稳定后，再讨论隐藏低层 copy loop 的条件。

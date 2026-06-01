# ABI return dynamic array source marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮处理 public wrapper 返回动态 bytes/string 时的 ABI head。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:760`
  - public wrapper 调用目标函数后：
    - `let memPos := allocateUnbounded()`
    - `let memEnd := abiEncode(memPos, retParams...)`
    - `return(memPos, sub(memEnd, memPos))`
- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:37`
  - `tupleEncoder()` 对动态返回值先写 head offset：
    - `mstore(add(headStart, pos), sub(tail, headStart))`
    - 然后从 tail 编码动态数据。
- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:450`
  - 动态 bytes/string 编码会先写 length，再复制 data。

真实 IR 里，`0450_19495071_6b6c9447e0_6344565f4b31` 的 public wrapper 是：

- `memory_write(returnBase, 0, 32)`
- `length := evm_mload(sourceArray)`
- `memory_write(returnBase, 32, length)`
- 后续循环把 `sourceArray + 32 + i` 复制到 `returnBase + 64 + i`
- 最后 `evm_return(returnBaseReload, returnSize)`

所以 ABI return pass 不能只标 return buffer，还要把动态源数组显式写成 IR marker，供后续 copy loop / byte write rewrite 消费。

## 实现

新增 marker：

- `notdec_solidity_abi_return_dynamic_array_source(returnBase, sourceArray, length, returnSize, kindCode)`

改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:4`
  - 引入 `llvm/IR/Dominators.h`，跨块检查 `memory_write` 是否支配 `evm_return`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:22`
  - 增加 `NumAbiReturnDynamicArraySources` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:188`
  - 增加 `AbiReturnDynamicArraySource`，记录 return base、source array、length。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:197`
  - 增加 `getMemoryLoadPointer()`，只接受 `evm_mload(sourceArray)` 作为 length 来源。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:210`
  - 增加 `valueAvailableAt()`，避免 source array 在 return 点不可用。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:223`
  - 增加 `findAbiReturnDynamicArraySource()`。
  - 匹配支配 return 的 `notdec_solidity_memory_write`：
    - offset 0 写常量 32；
    - offset 32 写 `evm_mload(sourceArray)`；
    - base 必须和 return consumer base 等价。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:284`
  - 增加 `insertAbiReturnDynamicArraySourceMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:393`
  - ABI return pass 在已有 `memory_consumer` / `memory_write` 基础上插入新 marker。
- `test/evm/solidity-patterns/manifest.json:1257`
  - 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 `notdec_solidity_abi_return_dynamic_array_source: 2` oracle。

这轮是消费者侧 rewrite surface：`AbiReturnPass` 消费 `MemoryBufferRewritePass` 写出的 `memory_write` / `memory_consumer`，产出 ABI return 专用 marker。它还没有消费上一轮的 `memory_array_byte_write`，也还没有隐藏动态 copy loop，所以不是完整闭环。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0450_19495071_6b6c9447e0_6344565f4b31`
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_memory_consumer`: 2
    - `notdec_solidity_memory_array_byte_write`: 3
    - `notdec_solidity_memory_write`: 18
    - `notdec_solidity_memory_consumer`: 10
  - `0448_19495059_065877b669_4f138305be23`
    - `notdec_solidity_abi_return_dynamic_array_source`: 0
    - 保持 `abi_return_data_allocation`: 3。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，106.71s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- outputs: 100
- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_abi_return_data_allocation`: 94
- `notdec_solidity_memory_array_byte_write`: 5
- `notdec_solidity_memory_write`: 8902
- `notdec_solidity_memory_consumer`: 5736

新 marker 在真实样例里有命中，不是单个 fixture 特例。数量远小于 ABI return consumer，因为这里只接受动态 bytes/string 单返回的明确 head 形状。

## 下一步

1. 继续把 `notdec_solidity_abi_return_dynamic_array_source` 和 copy loop 绑定起来，识别 `sourceArray + 32 + i` 到 `returnBase + 64 + i` 的动态数据复制。
2. 再把上一轮的 `notdec_solidity_memory_array_byte_write` 接进 ABI return，形成 source array byte writes -> ABI return dynamic source -> return buffer 的链条。
3. Revert 的动态 bytes/string 也有类似 helper/cross-block 问题，不能只用同块扫描。

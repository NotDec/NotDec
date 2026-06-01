# ABI return data allocation marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮让 ABI return 消费 `notdec_solidity_memory_allocation`。

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:760`，`IRGenerator::generateExternalFunction()`。

关键逻辑：

- public ABI wrapper 先调用目标函数，得到 `retParams`。
- 然后 `let memPos := allocateUnbounded()`。
- `let memEnd := abiEncode(memPos, retParams...)` 写返回数据。
- 最后 `return(memPos, sub(memEnd, memPos))`。

所以 ABI return data buffer 的 allocation、writes 和 return consumer 是同一条链。此前 `AbiReturnPass` 已经消费了 `memory_consumer`、`memory_write`、`memory_copy_write`，但还没有消费 `memory_allocation`。

## 实现

新增 marker：

- `notdec_solidity_abi_return_data_allocation(base, allocSize, returnSize, kindCode)`

改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:22`
  - 增加 `NumAbiReturnDataAllocations` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:72`
  - 增加 `isSameAbiReturnDataBase()`。
  - 这里只允许同一个 SSA 值或同一个常量地址，不用 free-pointer reload 的宽松匹配。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:147`
  - 增加 `findAbiReturnDataAllocationMarker()`。
  - 先要求 allocation base 匹配已经确认的 ABI return word/copy write base，再绑定 allocation。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:202`
  - 增加 `insertAbiReturnDataAllocationMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:286`
  - ABI return consumer 命中后，在已收集 word/copy writes 的基础上消费 matching `notdec_solidity_memory_allocation`。
- `test/evm/solidity-patterns/manifest.json:1181`
  - 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_abi_return_data_allocation: 3` oracle。

这轮是 IR rewrite 消费闭环：`AbiReturnPass` 消费通用 allocation marker，生成 ABI return 专用 allocation marker。metadata 没有作为接口。

## 验证

- `cmake --build ./build --target all -j4`
  - 通过。
- 单例检查：
  - `0448_19495059_065877b669_4f138305be23`
    - `notdec_solidity_abi_return_data_allocation`: 3
    - `notdec_solidity_abi_return_data_copy_write`: 1
    - `notdec_solidity_abi_return_data_word_write`: 17
    - `notdec_solidity_abi_return_memory_consumer`: 14
    - `notdec_solidity_memory_allocation`: 13
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
  - 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 通过，107.01s。

## 固定 100 例 audit

固定输入仍来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`

结果：

- inputs: 100
- failed: 0
- `notdec_solidity_abi_return_data_allocation`: 94
- `notdec_solidity_abi_return_data_word_write`: 1422
- `notdec_solidity_abi_return_data_copy_write`: 22
- `notdec_solidity_abi_return_memory_consumer`: 1420
- `notdec_solidity_memory_allocation`: 1041
- `notdec_solidity_memory_consumer`: 5736
- `notdec_solidity_memory_write`: 8902

新 marker 在固定 100 例里不是 0 命中，但明显少于 ABI return consumer。原因是这轮要求 allocation base 和 ABI return data write/copy base 严格一致，避免把别的 allocation 误接到 return。

## 下一步

1. 继续看 ABI return 里未绑定 allocation 的样例，区分是 free-pointer reload、helper encode 跨块，还是没有明确 allocation marker。
2. 看 revert data allocation 是否能用同样方式消费 `memory_allocation`，但要注意 returndata bubble 不是 ABI encode allocation。
3. 对 `memory_byte_write` 继续分类，优先看 bytes/string return 或 event payload 的真实 IR 形状。

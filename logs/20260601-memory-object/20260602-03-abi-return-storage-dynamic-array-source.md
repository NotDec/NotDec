# ABI return storage dynamic array source

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2422`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2611`

相关逻辑：

- `storageArrayIndexAccessFunction()` 对 storage bytes/string 区分短数组和长数组。长数组会通过 data area 访问元素。
- `copyArrayFromStorageToMemoryFunction()` 对 storage array -> memory array 会先分配 memory，再从 storage data area 读数据并写入 memory。
- 当前真实 IR 里 storage bytes/string getter 常见形状是：
  - 先 `sload(slot)` 读长度/短数组编码。
  - 长数组路径写 `mstore(0, slot)` 后 `sha3(0, 32)` 得到 data area。
  - 再从 data area 派生地址 `sload`，把 storage bytes/string 拷贝到 memory array。
  - public wrapper 再把这个 memory array 复制到 ABI return buffer。

## 问题

上一轮的下一步本来想把 `memory_array_byte_write` 接到 `abi_return_dynamic_array_source`。检查 fixed100 后发现不能硬接：

- fixed100 里 `memory_array_byte_write` 只有 5 个。
- 与 `abi_return_dynamic_array_source` 同时出现的主要是 `25580`。
- 但 `25580` 里 `memory_array_byte_write` 写的是另一个 helper 临时构造的 bytes，不是 public getter 最后 return 的 source array。

所以这轮不把 `memory_array_byte_write` 强接到 ABI return。改为处理更真实的来源：dynamic ABI return 的 source array 来自 storage bytes/string decode helper。

## 实现

文件：`src/Passes/evm/solidity-patterns/AbiReturnPass.cpp`

- `:34` 增加 `NumAbiReturnDynamicArrayStorageSources` 统计。
- `:232` 增加 `AbiReturnDynamicArrayStorageSource`。
- `:519` 增加 `getSourceArrayProducerCall()`：
  - 支持 source array 直接是 call 返回值。
  - 支持 source array 是 `extractvalue {i256, i256} call, 0`，这是 public getter helper 返回 `{memPtr, end/aux}` 时的常见形状。
- `:531` 增加 `functionHasStorageBytesSource()`：
  - 不依赖后续 `StorageAddressingPass` 的 marker，因为 pipeline 里 `AbiReturnPass` 在它之前。
  - 直接识别 helper 内的原始 storage bytes/string 形状：常量 slot `sload`、`evm_sha3(0, 32)`、非字面 slot 的 data `sload`。
- `:560` 增加 `findAbiReturnDynamicArrayStorageSource()`，只在 producer helper dominate return 时标记。
- `:663` 插入 `notdec_solidity_abi_return_dynamic_array_storage_source(returnBase, sourceArray, length, returnSize, kindCode)`。
- `:801` 在 dynamic source/copy/mcopy/helper copy 之后接入 storage source marker。

这轮是 ABI return 消费者侧的 IR rewrite marker，不是 metadata。它把 dynamic return source 的来源从“一个 memory array”推进到“这个 memory array 来自 storage bytes/string decode helper”。

测试更新：

- `test/evm/solidity-patterns/manifest.json:1262`
  - 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 `notdec_solidity_abi_return_dynamic_array_storage_source: 2` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单例：
  - `0450_19495071_6b6c9447e0_6344565f4b31`
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2
    - `notdec_solidity_abi_return_dynamic_array_storage_source`: 2
  - `25580_19770476_5bc11a3f17_8208dc777fbf`
    - `notdec_solidity_abi_return_dynamic_array_source`: 2
    - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2
    - `notdec_solidity_abi_return_dynamic_array_storage_source`: 2
  - 两个输出都能被 `llvm-22.1.0.obj/bin/llvm-as` 接受。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，110.46s。

fixed100 audit：

- 输入：`20260601-evm2llvm-train-batch639/640/641` 的前 100 个 `.bc`。
- 输出：100 / 100。
- 精确统计只数 `call void @marker(...)`。
- 相关动态 return 统计：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1
  - `notdec_solidity_abi_return_dynamic_array_storage_source`: 10

## 下一轮候选

1. 继续区分 dynamic return source 的来源：storage bytes/string、calldata decode、memory builder、literal builder。
2. 对 `memory_array_byte_write` 不硬接；先找真实同 base 样例，再插 `abi_return_dynamic_array_byte_write`。
3. storage source marker 稳定后，再考虑让后续 C 输出或隐藏逻辑消费它。

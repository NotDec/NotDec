# ABI return dynamic array helper copy

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:103`

`YulUtilFunctions::copyToMemoryFunction(false, cleanup)` 生成 memory 到 memory 的复制 helper：

- 新 EVM：`mcopy(dst, src, length)`，cleanup 时再 `mstore(add(dst, length), 0)`。
- 旧 EVM：循环 `i += 32`，执行 `mstore(add(dst, i), mload(add(src, i)))`。
- cleanup 版本会在 `dst + length` 写 0。

这说明真实 IR 里动态数组 return 的数据复制不一定内联在 public entry wrapper 里，也可能保留成 helper 调用。这个 helper 是编译器生成的 memory copy，不是业务逻辑。

## 问题

fixed100 里 `25524_19769741_8c9517a9e9_3555426790bd.ll` 已经能识别 dynamic array source，但没有识别复制关系：

- return 站点调用 outlined helper。
- helper 实参里 `src = sourceArray + 32`。
- helper 实参里 `dst = returnBase + 64`。
- helper 实参里 `length = mload(sourceArray)`。
- helper 函数体里有 `mstore(dst + i, mload(src + i))`，并在 `dst + length` 写 0。

原有 copy-loop matcher 只看当前函数里的内联循环，mcopy matcher 只看 `notdec_solidity_memory_copy_write`，所以漏掉这个 outlined helper。

## 实现

文件：`src/Passes/evm/solidity-patterns/AbiReturnPass.cpp`

- `:31` 增加 `NumAbiReturnDynamicArrayHelperCopies` 统计。
- `:220` 增加 `AbiReturnDynamicArrayHelperCopy`，记录 return base、source array、length 和 helper call。
- `:382` 到 `:447` 增加 helper 识别逻辑：
  - 约定 outlined helper 的 `src/dst/length` 是 lowered IR 里的第 4/5/6 个参数。
  - 要求函数体存在 `mstore(dst + i, mload(src + i))`。
  - 要求存在 cleanup `mstore(dst + length, 0)`。
- `:475` 到 `:498` 增加 call-site 绑定：
  - `arg4 == sourceArray + 32`。
  - `arg5 == returnBase + 64`，允许 return base 重新从 free pointer reload。
  - `arg6 == source length`。
  - helper call 必须 dominate `return`。
- `:568` 到 `:584` 插入 `notdec_solidity_abi_return_dynamic_array_helper_copy(returnBase, sourceArray, length, returnSize, kindCode)`。
- `:698` 到 `:704` 在 ABI return pass 里接入 helper-copy matcher。

这个 marker 是 ABI return 消费者侧的 rewrite surface。它不是 metadata，也不是最终隐藏低层 helper 的完整闭环；下一步还要把 source array 的 byte/word write 和这个 return copy 关系接起来。

测试新增：

- `test/evm/solidity-patterns/cases/25524_19769741_8c9517a9e9_3555426790bd.ll`
- `test/evm/solidity-patterns/manifest.json:5260` 增加 oracle：
  - `notdec_solidity_abi_return_dynamic_array_source`: 1
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1
  - `notdec_solidity_abi_return_data_allocation`: 1

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，108.46s。

fixed100 audit：

- 输入：`20260601-evm2llvm-train-batch639/640/641` 的前 100 个 `.bc`。
- 输出：100 / 100。
- 新增 `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1。
- 相关动态 return 统计：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 14
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1

## 下一轮候选

1. 继续看 source 多于 copy marker 的样例，优先 `25544` 和 `25563`，确认是不是 helper 参数位置不同、base reload 形状不同，还是 source/copy 本来不是同一个 return。
2. 把 `memory_array_byte_write` / source array 内容写入和 `abi_return_dynamic_array_source` 接起来，让 return dynamic array 不只知道复制了哪个 array，还知道 array 数据从哪里来。
3. 在 helper/copy_loop/mcopy marker 足够稳定后，再评估能否 hide 低层 helper call 或低层 loop。当前不能删，因为后续仍可能依赖低层 IR。

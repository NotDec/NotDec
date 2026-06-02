# ABI return copy loop address order

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

源码：`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:103`

`copyToMemoryFunction(false, cleanup)` 在旧 EVM 上生成：

- `for { } lt(i, length) { i := add(i, 32) }`
- `mstore(add(dst, i), mload(add(src, i)))`
- cleanup 时 `mstore(add(dst, length), 0)`

编译到当前 IR 后，地址可能是 `add(add(base, i), const)`，也可能是 `add(add(i, base), const)`。这两个都是同一个 compiler copy loop，不是业务分支。

## 问题

fixed100 里 `25544` 和 `25563` 仍有 dynamic array source，但缺 copy marker。检查真实 IR 后，缺口不是 outlined helper，而是内联 copy loop 的地址顺序不同：

- source data：`(i + sourceArray) + 32`
- return data：`(i + returnBaseReload) + 64`

原 matcher 只稳定处理 `(base + const) + i`，没有覆盖 `(base + i) + const`，也没有把 return 前 reload 的 free memory pointer 当作同一个 return base。

## 实现

文件：`src/Passes/evm/solidity-patterns/AbiReturnPass.cpp`

- `:316` 增加 `matchBasePlusIndex` 的 reload base 判断，允许 loop 使用早一点的 `mload(0x40)`，return 使用 return 前重新 reload 的 `mload(0x40)`。
- `:330` 扩展 `matchBasePlusConstantPlusIndex`：
  - 先识别 `(base + index) + constant`。
  - 保留原有 `(base + constant) + index` 逻辑。

这轮仍然产出已有的 `notdec_solidity_abi_return_dynamic_array_copy_loop(...)` marker，没有新增 metadata。它补的是同一个 IR rewrite surface 的覆盖面。

测试新增：

- `test/evm/solidity-patterns/cases/25544_19769956_6dc86f13a0_e354b4c44525.ll`
- `test/evm/solidity-patterns/cases/25563_19770118_498bb800eb_8e79ddef1b42.ll`
- `test/evm/solidity-patterns/manifest.json:5275`、`:5289` 增加 oracle：
  - `notdec_solidity_abi_return_dynamic_array_source`: 2
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，110.90s。

fixed100 audit：

- 输入：`20260601-evm2llvm-train-batch639/640/641` 的前 100 个 `.bc`。
- 输出：100 / 100。
- 精确统计只数 `call void @marker(...)`，不数 declaration。
- 相关动态 return 统计：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1

这说明 fixed100 里 dynamic array source 和 copy/mcopy/helper copy 已经对齐：`18 + 2 + 1 = 21`。

## 下一轮候选

1. 把 `memory_array_byte_write` / source array 内容写入和 `abi_return_dynamic_array_source` 接起来，让 dynamic return 知道数组数据的来源。
2. 检查 return dynamic array 以外的 ABI return 动态类型，例如 bytes/string 或多返回值里的嵌套动态结构。
3. 在 copy_loop/mcopy/helper copy 都稳定后，再考虑 hide 低层 loop/helper。当前只加 semantic marker，不删除低层 IR。

# Checked bounds small unsigned bounded arithmetic

## 问题

上一轮后 pattern suite 还有 32 个 `checked_bounds_skip`，panic `0x11`
还有 10 个。继续看 `0x11` 后，部分 case 是 Solidity 小整数 checked arithmetic：

- 操作数先用同一个 `2^n - 1` mask 做 unsigned cleanup；
- `add/sub` 后用同一个 max 做 `result > max` guard；
- `mul` 后先 cleanup product，再检查 `product == cleanup(product)`；
- 失败分支都是 `panic(0x11)`。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:720`
的 `overflowCheckedIntAddFunction`、`:780` 的
`overflowCheckedIntMulFunction`、`:922` 的
`overflowCheckedIntSubFunction` 都是这个语义：先 cleanup，再用类型 max
判断 overflow。

## 本轮选择

只处理 unsigned 小整数 bounded arithmetic。收益是 4 个 pattern skip，语义明确，
且不需要跨函数猜测。

不做：

- 不处理 signed checked arithmetic 的组合条件；
- 不处理 full `uint256` 的 `max / x` 乘法上界；
- 不处理 batch589 剩余两个 `panic 0x11`，它们当前 IR 里已经缺少稳定原始操作数；
- 不把没有 cleanup mask 的普通 `panic(0x11)` 分支当作 checked arithmetic。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:292` 新增 `isPowerOfTwoMinusOne`，
  识别常量或 `evm_shl(bits, 1) - 1` 形式的 unsigned max。
- `src/Passes/evm/SolidityPatterns.cpp:325` 新增
  `isUnsignedCleanupToMaxValue`，要求 operand 被同一个 max mask cleanup。
- `src/Passes/evm/SolidityPatterns.cpp:2062` 新增
  `matchUnsignedBoundedResult`，匹配 `add/sub` 的 `result > max` guard。
- `src/Passes/evm/SolidityPatterns.cpp:2101` 新增
  `matchUnsignedBoundedMulCleanup`，匹配 `mul == (mul & max)` 的 success guard。
- `src/Passes/evm/SolidityPatterns.cpp:2194` 在 checked arithmetic matcher 中接入
  `checked_add_bound`、`checked_sub_bound`、`checked_mul_bound` rewrite。
- `test/evm/solidity-patterns/manifest.json:1512` 更新 `2001...` oracle：
  `checked_add_bound` `1 -> 2`，`panic_checked_arithmetic` `2 -> 1`，
  `unrecognized_operands` `3 -> 2`，CFG rewrite `33 -> 34`。
- `test/evm/solidity-patterns/manifest.json:1679` 给 `0258...` 补强
  checked-bounds oracle，记录新增 `checked_mul_bound` 和剩余
  `operand_not_available_at_guard`。
- `test/evm/solidity-patterns/manifest.json:2262` 更新 `1775...` oracle：
  新增 `checked_add_bound` 和 `checked_sub_bound`，skip 清零，CFG rewrite
  `13 -> 15`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：32 -> 28。
- pattern panic `0x11` skip：10 -> 6。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，170.23s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.79s。

复杂度评分：

- 实现效果：6/10。减少 4 个 `0x11` skip，并补强了 `0258` oracle。
- 理解成本：4/10。新增的是专用 bounded arithmetic matcher，条件集中在 cleanup mask 和 guard。
- 维护成本：3/10。没有按地址或 helper 名匹配，只要求同一个 max 同时出现在 cleanup 和 guard。

## 剩余 skip 和下一轮

pattern suite 当前剩余 28 个 skip：

- panic `0x0`: 14
- panic `0x11`: 6
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 剩余 panic `0x11`：还有 6 个，但形状分成 signed 条件、full `uint256`
   乘法上界和已丢操作数增量。下一轮只能挑能从当前 IR 证明的子类。
2. pattern panic `0x0` / `0x1`：数量最高，但要先区分 compiler assert、user assert 和不可达分支。
3. pattern panic `0x32`：剩 2 个，语义明确，适合从具体 case 继续看。
4. batch589 两个 panic `0x11`：真实数据还剩两个，但当前 pass 位置缺少操作数，优先调查是否能从更早 IR 保留证据。

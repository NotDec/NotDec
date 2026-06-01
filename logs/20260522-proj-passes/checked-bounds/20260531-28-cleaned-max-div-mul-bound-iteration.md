# Checked bounds cleaned max-div multiply

## 问题

上一轮后 pattern suite 还有 28 个 `checked_bounds_skip`，panic `0x11`
还有 6 个。继续看 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 后，剩下一个
`0x11` 是 Solidity checked unsigned multiplication 的 max/div guard：

- `max := shl(80, 1) - 1`；
- 两个操作数都先 `and max` cleanup；
- guard 成功条件是 `x == 0 || y <= max / x`；
- success block 返回 `x * y`；
- 失败分支是 `panic(0x11)`。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:780`
的 `overflowCheckedIntMulFunction` 对较宽 unsigned integer 使用同类逻辑：
先 cleanup，再用 `x == 0 || y == product / x` 或等价的 max/div 上界判断 overflow。

## 本轮选择

只补 cleaned `max / x` multiplication guard。这个 case 的 operand cleanup、max、
除法上界和 success block product 都在当前 IR 里，能直接证明。

不做：

- 不处理 signed checked arithmetic；
- 不处理常量乘法上界，比如 `max / x > 1000000000000`；
- 不处理 `0x32` loop index guard，因为当前 IR 下直接删异常边仍缺少更强证明；
- 不处理 batch589 剩余两个 `0x11`，它们还是缺少原始操作数。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:1885` 新增
  `matchMulByCleanedMaxDivBound`，只接受 `Factor <= max / Divisor` 且
  `Factor`、`Divisor` 都被同一个 `max` cleanup。
- `src/Passes/evm/SolidityPatterns.cpp:1968` 新增
  `matchCheckedMulCleanedMaxDivSuccessCondition`，匹配
  `x == 0 || y <= max / x` 成功条件。
- `src/Passes/evm/SolidityPatterns.cpp:2035` 在 `matchCheckedMulGuard`
  里优先返回 `checked_mul_bound`。
- `src/Passes/evm/SolidityPatterns.cpp:3835`、`:3874` 扩展 bound arithmetic
  marker 插入：如果 product 在 success block，guard 前重建同一个 `mul` 作为
  semantic marker 参数，避免 metadata-only。
- `test/evm/solidity-patterns/manifest.json:1512` 更新 `2001...` oracle：
  新增 `checked_mul_bound`，`panic_checked_arithmetic` `1 -> 0`，
  `unsupported_condition` `1 -> 0`，CFG rewrite `34 -> 35`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：28 -> 27。
- pattern panic `0x11` skip：6 -> 5。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，171.63s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.80s。

复杂度评分：

- 实现效果：4/10。只减少 1 个 pattern skip，但把一个明确的 full checked-mul guard rewrite 了。
- 理解成本：4/10。新增 matcher 只围绕 `max / x` 和同 max cleanup。
- 维护成本：3/10。没有按 helper 地址匹配，条件比较窄。

## 剩余 skip 和下一轮

pattern suite 当前剩余 27 个 skip：

- panic `0x0`: 14
- panic `0x11`: 5
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 剩余 panic `0x11`：还剩 5 个，里面有 signed 条件、常量乘法上界和已丢操作数增量。只能继续挑能证明的子类。
2. pattern panic `0x32`：剩 2 个，语义明确但当前看起来是 loop/index guard，rewrite 前要证明异常边不可达或是 compiler-only。
3. pattern panic `0x0` / `0x1`：数量最高，但需要先区分 compiler assert、user assert 和不可达分支。
4. batch589 两个 panic `0x11`：真实数据还剩两个，下一步仍要调查是否能从更早 IR 保留证据。

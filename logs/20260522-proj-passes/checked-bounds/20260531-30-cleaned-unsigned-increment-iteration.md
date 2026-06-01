# Checked bounds cleaned unsigned increment

## 问题

上一轮后 pattern suite 还有 26 个 `checked_bounds_skip`，panic `0x11` 还有 5 个。
继续看 `1991_19510015_7f60edd6aa_6188d2031ddf`，其中一个 skip 是 cleaned
unsigned increment：

- `max := shl(128, 1) - 1`；
- `value := and input, max`；
- `if value == max panic(0x11)`；
- success block 返回 `value + 1`。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4216`
的 `incrementCheckedFunction` 就是这个形状：先 cleanup，再检查
`eq(value, maxval)`，最后 `ret := add(value, 1)`。

## 本轮选择

只补 cleaned unsigned increment。这个形状同时有同一个 cleaned value、`2^n-1`
上界、固定 `+ 1` 结果，能证明是编译器 checked increment guard。

不做：

- 不处理 signed add/sub；
- 不处理 `1991` 里另一个动态 packed-field 加法上界；
- 不处理 `0334` 的 signed arithmetic；
- 不处理 `0x32` loop/index guard。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2244` 新增
  `matchCleanedUnsignedIncrement`，只接受 `icmp eq cleaned, max`，其中
  `cleaned` 必须是对同一个 `max` 的 unsigned cleanup，`max` 必须是 `2^n-1`。
- `src/Passes/evm/SolidityPatterns.cpp:2264` 要求 success block 或 guard block
  里存在同一个 cleaned value 的 `+ 1`。
- `src/Passes/evm/SolidityPatterns.cpp:2357` 在 `matchCheckedArithmetic` 中返回
  `checked_add_bound`，marker 参数固定为 `cleaned, 1, result, max`。
- `test/evm/solidity-patterns/manifest.json:2058` 更新 `1991...` oracle：
  `panic_checked_arithmetic` `2 -> 1`，新增 `checked_add_bound` 和
  `notdec_solidity_rewrite_checked_add_bound`，`unrecognized_operands`
  `2 -> 1`，CFG rewrite `73 -> 74`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：pattern runner 内 78/78 passed。
- pattern skip：26 -> 25。
- pattern panic `0x11` skip：5 -> 4。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，172.31s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，17.07s。

复杂度评分：

- 实现效果：3/10。只减少 1 个 pattern skip，但这是一个明确的 Solidity helper 形状。
- 理解成本：3/10。新增 matcher 很窄，只围绕 cleaned value 的 `+ 1`。
- 维护成本：2/10。不按地址或函数名匹配，条件都来自当前 IR 数据流。

## 剩余 skip 和下一轮

pattern suite 当前剩余 25 个 skip：

- panic `0x0`: 14
- panic `0x11`: 4
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：剩 2 个，集中在 array/loop index guard。语义比较明确，但必须先证明异常边是 compiler bounds guard。
2. pattern 剩余 panic `0x11`：还剩 4 个。`1991` 另一个像 packed-field 加法上界，`0334` 有 signed arithmetic，`1485` 是常量乘法上界；要继续逐个证明。
3. batch589 两个 panic `0x11`：真实数据仍剩两个。当前 pass 位置缺少原始操作数，下一步要看更早 IR 是否保留证据。
4. pattern panic `0x0` / `0x1`：数量最高，但先要区分 compiler assert、user assert 和不可达分支。

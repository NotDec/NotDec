# Checked bounds const max-div multiply

## 问题

上一轮后 pattern suite 还有 25 个 `checked_bounds_skip`，panic `0x11` 还有 4 个。
`1485_19504919_9deb0fc809_62afe2b29485` 里剩下一个 `0x11` 是常量乘法上界：

- `x` 先被清成 `uint64`；
- guard 成功条件是 `x == 0 || (-1 / x) > 999999999999`；
- success block 返回 `x * 1000000000000`；
- 失败分支是 `panic(0x11)`。

这是 `y <= max / x` 的常量版，其中 `y = 1000000000000`，
`max / x > y - 1` 等价于 `y <= max / x`。Solidity
`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:780`
的 `overflowCheckedIntMulFunction` 对 unsigned multiplication 使用同类
`x == 0 || y == div(product, x)` overflow guard；当前 IR 是优化后的 max-div
上界形式。

## 本轮选择

只补 `div(-1, x) > const_minus_one` 且 success block 存在
`x * (const_minus_one + 1)` 的形状。

不做：

- 不处理任意 `div > const`；
- 不处理 signed multiplication；
- 不处理 `const < div`，当前真实 IR 没出现这个形状；
- 不处理 `0x32` loop/index guard，因为还缺少局部 CFG 之外的证明。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:1885` 新增
  `matchMulByStrictConstMaxDivBound`，只接受 `evm_div(-1, x) > C`。
- `src/Passes/evm/SolidityPatterns.cpp:1910` 用 `C + 1` 找 success block 里的
  `mul x, C+1`，找不到就不 rewrite。
- `src/Passes/evm/SolidityPatterns.cpp:1985` 在已有 max-div multiplication
  matcher 失败后，再尝试这个常量版。
- `test/evm/solidity-patterns/manifest.json:2630` 给 `1485...` 补强 oracle：
  检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG
  rewrite。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：25 -> 24。
- pattern panic `0x11` skip：4 -> 3。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，171.89s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，17.10s。

复杂度评分：

- 实现效果：3/10。只减少 1 个 pattern skip，但把 `1485` 的 checked-bounds oracle 补强了。
- 理解成本：3/10。matcher 只处理一个常量 max-div 形状。
- 维护成本：2/10。必须能在 success block 找到对应乘法，否则保留 skip。

## 剩余 skip 和下一轮

pattern suite 当前剩余 24 个 skip：

- panic `0x0`: 14
- panic `0x11`: 3
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：剩 2 个，都是 loop/index 附近的 array bounds guard。要先证明异常边是 compiler guard，而不是用户可触发的 bounds check。
2. pattern 剩余 panic `0x11`：还剩 3 个。`0334` 里有 signed arithmetic，`1991` 里还有 packed-field 加法上界；需要继续逐个证明。
3. batch589 两个 panic `0x11`：真实数据仍剩两个。当前 pass 位置缺少原始操作数，下一步看更早 IR 或 helper 结构。
4. pattern panic `0x0` / `0x1`：数量最高，但必须先分类 assert 来源。

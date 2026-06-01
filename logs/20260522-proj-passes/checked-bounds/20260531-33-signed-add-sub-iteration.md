# Checked bounds signed add sub

## 问题

上一轮后 pattern suite 还有 23 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x11`: 3
- panic `0x21`: 2
- panic `0x32`: 2
- panic `0x41`: 1

其中 `0334_19494307_668d201319_1354ce2e324d` 有两个 panic `0x11`
skip。真实 IR 里它们是 Solidity signed 256-bit `add` / `sub` 的溢出检查：

- signed sub：先算 `sub(x, y)`，成功条件等价于
  `(y < 0 || sub <= x) && (y >= 0 || sub >= x)`。
- signed add：先算 `add(x, y)`，失败条件来自
  `(x >= 0 && add < y)` 和符号位选择，优化后外层变成成功条件。

Solidity 对应 codegen 在
`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:720`
`overflowCheckedIntAddFunction`，以及同文件 `:922`
`overflowCheckedIntSubFunction`。

## 本轮选择

选择 `0334...` 的两个 signed arithmetic guard，因为：

- 都是 panic `0x11`；
- 条件和 Solidity codegen 能对上；
- rewrite 后可以直接输出 `checked_add` / `checked_sub` semantic helper，并改 CFG；
- 不需要跨 block memory 证明。

不做：

- 不处理 `2001...` 的两个 panic `0x32`。它们需要证明
  `mstore(base, len)` 到后续 `mload(base)` 中间没有覆盖。
- 不把其他 signed add/sub 条件泛化成通用规则。
- 不用 metadata-only 代替 rewrite。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2309` 新增 signed add/sub guard
  matcher，匹配 Solidity signed 256-bit success-condition 形状。
- `src/Passes/evm/SolidityPatterns.cpp:2393` 新增
  `matchSignedSubSuccessCondition`，识别 signed sub 的两个 OR 条件。
- `src/Passes/evm/SolidityPatterns.cpp:2460` 新增
  `matchSignedAddSuccessCondition`，识别当前真实 IR 里的 signed add 条件。
- `src/Passes/evm/SolidityPatterns.cpp:2485` 新增
  `matchSignedCheckedArithmeticGuard`，只接受 panic `0x11` 且分支条件表示成功。
- `src/Passes/evm/SolidityPatterns.cpp:3974` 在 checked mul 之后接入 signed
  arithmetic matcher。
- `test/evm/solidity-patterns/manifest.json:1918` 更新 `0334...` oracle：
  `checked_add 2 -> 3`，`checked_sub 1 -> 2`，移除
  `panic_checked_arithmetic`，skip 从 3 降到 1，CFG rewrite `50 -> 52`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：23 -> 21。
- pattern panic `0x11` skip：3 -> 1。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.42s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，17.00s。

复杂度评分：

- 实现效果：4/10。只减少 2 个 pattern skip，但消掉了 `0334...` 的 signed add/sub。
- 理解成本：4/10。matcher 较长，但每个 helper 都绑定到明确的 Solidity guard 形状。
- 维护成本：3/10。规则保守，后续如果支持更多 signed arithmetic 形状，需要逐个用真实 IR 证明。

## 剩余 skip 和下一轮

pattern suite 当前剩余 21 个 skip：

- `0651_19497235_18ca6cbfa7_017843325734`: 9 个，`{0: 7, 33: 1, 65: 1}`
- `0648_19497230_998d150449_ea367f7fc1ac`: 8 个，`{0: 7, 33: 1}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 2 个，`{50: 2}`
- `1991_19510015_7f60edd6aa_6188d2031ddf`: 1 个，`{17: 1}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：`2001...` 两个真实 case。语义可能是 array bounds，
   但必须先证明 memory length load 没有被覆盖。
2. pattern panic `0x11`：`1991...` 一个 packed-field add bound。数量少，但语义
   比较像 compiler guard。
3. batch589 panic `0x11`：`23523...` 和 `23545...` 各一个。真实数据价值高，
   需要先分类 IR 形状。
4. pattern panic `0x0` / `0x1`：数量高，但要先区分 assert、unreachable 和业务
   分支，不能直接 rewrite。

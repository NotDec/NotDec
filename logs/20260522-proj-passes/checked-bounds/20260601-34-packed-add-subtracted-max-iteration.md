# Checked bounds packed add subtracted max

## 问题

上一轮后 pattern suite 还有 21 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x11`: 1
- panic `0x21`: 2
- panic `0x32`: 2
- panic `0x41`: 1

唯一的 pattern panic `0x11` 在
`1991_19510015_7f60edd6aa_6188d2031ddf`。真实 IR 形状是 packed field
加法上界：

- `old = and(..., max128)`
- `delta = and(input, max128)`
- `limit = max128 - delta`
- `old > limit` 时 panic `0x11`
- success block 里执行 `old + delta`

这等价于 unsigned small integer checked add 的 `old + delta <= max128`。Solidity
codegen 的 checked unsigned add 在
`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:720`
`overflowCheckedIntAddFunction`，非 256-bit unsigned 分支检查 `sum > maxValue`
时 panic。

## 本轮选择

选择 `1991...` 这个 case，因为：

- 它是 pattern suite 里最后一个 panic `0x11` skip；
- guard 条件和 success block 的 `add` 能直接对应；
- `old` 和 `delta` 都被同一个 `2^n - 1` mask 清理；
- rewrite 可以输出 `checked_add_bound` semantic helper 并改 CFG。

不做：

- 不处理 batch589 里的两个 panic `0x11`。它们当前是常量 `shl(255, 1)` 比较，
  不是清楚的 arithmetic helper。
- 不把 `max - delta` 放宽到任意 sub 条件。
- 不在找不到 success block add 时输出 marker。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2226` 新增
  `matchUnsignedAddWithSubtractedMax`，只匹配 `lhs > max - rhs` / `lhs < max - rhs`
  的等价方向。
- `src/Passes/evm/SolidityPatterns.cpp:2252` 要求 `max` 是 `2^n - 1`，且
  `lhs`、`rhs` 都是同一个 max 的 unsigned cleanup。
- `src/Passes/evm/SolidityPatterns.cpp:2260` 要求 success block 中存在
  `lhs + rhs`，否则不 rewrite。
- `src/Passes/evm/SolidityPatterns.cpp:2628` 在 checked arithmetic 里接入该
  matcher，输出 `checked_add_bound`。
- `test/evm/solidity-patterns/manifest.json:2055` 更新 `1991...` oracle：
  `checked_add_bound 1 -> 2`，移除 `panic_checked_arithmetic`，skip 清零，CFG
  rewrite `74 -> 75`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：21 -> 20。
- pattern panic `0x11` skip：1 -> 0。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.54s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.30s。

复杂度评分：

- 实现效果：3/10。减少 1 个 pattern skip，但清掉了 pattern suite 最后一个 panic `0x11`。
- 理解成本：3/10。条件窄，只处理 `max - rhs` 这一种等价上界。
- 维护成本：3/10。后续扩展必须继续要求 max cleanup 和 success block arithmetic。

## 剩余 skip 和下一轮

pattern suite 当前剩余 20 个 skip：

- `0651_19497235_18ca6cbfa7_017843325734`: 9 个，`{0: 7, 33: 1, 65: 1}`
- `0648_19497230_998d150449_ea367f7fc1ac`: 8 个，`{0: 7, 33: 1}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 2 个，`{50: 2}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：`2001...` 两个真实 case。可能是 array bounds，但需要先
   证明 memory length load 没有被覆盖。
2. pattern panic `0x21` / `0x41`：`0648...`、`0651...` 各有 enum/resource 类
   guard，需要分别看是否已有语义 helper 可 rewrite。
3. batch589 panic `0x11`：`23523...` 和 `23545...` 各一个。真实数据价值高，
   但当前形状像常量退化 guard，优先级低于 pattern suite 明确剩余项。
4. pattern panic `0x0` / `0x1`：数量高，但需要先区分 assert、unreachable 和业务
   分支。

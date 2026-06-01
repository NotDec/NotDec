# Checked bounds enum max gt

## 问题

上一轮后 pattern suite 还有 20 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x32`: 2
- panic `0x41`: 1
- panic `0x21`: 2

两个 panic `0x21` 分别在：

- `0648_19497230_998d150449_ea367f7fc1ac`
- `0651_19497235_18ca6cbfa7_017843325734`

真实 IR 都是 `value > 2` 后 panic `0x21`，success 分支继续使用这个值。这是 enum
conversion guard，`2` 是最大 enum member，member count 是 `3`。

Solidity legacy codegen 在
`/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp:851` 和 `:891`
用 `numberOfMembers() - 1`、`GT`、`EnumConversionError` 做 enum 转换检查。
Yul validator 在
`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4089`
用 `value < members` 表达同一个条件。

## 本轮选择

选择这两个 `0x21` case，因为：

- 都是 pattern suite 真实 case；
- panic code 和 Solidity codegen 能直接对上；
- 已有 `enum_conversion` semantic helper，只需要补等价条件方向；
- rewrite 后能改 CFG，不是 metadata-only。

不做：

- 不处理 panic `0x41`。它是 resource/memory 类，需要单独确认。
- 不处理 panic `0x32`。`2001...` 仍需要 memory length load 无覆盖证明。
- 不把非小常量的 `>` 条件当作 enum conversion。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2869` 新增
  `getEnumMemberCountFromMax`，只接受小常量 max member，并转换为 member count。
- `src/Passes/evm/SolidityPatterns.cpp:2898` 扩展 `matchEnumConversion`，支持
  `value > maxMember` 和等价反向 `maxMember < value`。
- `test/evm/solidity-patterns/manifest.json:1842` 更新 `0651...` oracle：
  `panic_enum_conversion -> enum_conversion`，skip `9 -> 8`，CFG rewrite `21 -> 22`。
- `test/evm/solidity-patterns/manifest.json:2337` 更新 `0648...` oracle：
  `panic_enum_conversion -> enum_conversion`，skip `8 -> 7`，CFG rewrite `22 -> 23`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：20 -> 18。
- pattern panic `0x21` skip：2 -> 0。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.62s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.54s。

复杂度评分：

- 实现效果：4/10。减少 2 个 pattern skip，并清掉 pattern suite 的 panic `0x21`。
- 理解成本：2/10。只是补 enum conversion 的 `> maxMember` 等价方向。
- 维护成本：2/10。限制在小常量 max member，误匹配风险低。

## 剩余 skip 和下一轮

pattern suite 当前剩余 18 个 skip：

- `0651_19497235_18ca6cbfa7_017843325734`: 8 个，`{0: 7, 65: 1}`
- `0648_19497230_998d150449_ea367f7fc1ac`: 7 个，`{0: 7}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 2 个，`{50: 2}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：`2001...` 两个真实 case。可能是 array bounds，但还要
   证明 memory length load 没被覆盖。
2. pattern panic `0x41`：`0651...` 一个 resource/memory guard。数量少，但 panic
   code 明确，值得先看是否已有 helper。
3. batch589 panic `0x11`：`23523...` 和 `23545...` 各一个。真实数据价值高，
   但当前像常量退化 guard。
4. pattern panic `0x0` / `0x1`：数量高，但要先区分 assert、unreachable 和业务
   分支。

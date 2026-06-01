# Checked bounds panic code offset

## 问题

上一轮后 pattern suite 还有 16 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x32`: 1

其中 `0648_19497230_998d150449_ea367f7fc1ac` 和
`0651_19497235_18ca6cbfa7_017843325734` 各有 7 个 panic `0x0` skip。检查
真实 IR 后发现它们并不是 Solidity panic code 0，而是 panic code 没被
`CheckedBoundsPass` 读出来：

- panic selector 写在 revert base 上，例如 `evm_mstore(base, selector)`。
- panic code 仍写在 ABI 绝对偏移 `4`，例如 `evm_mstore(4, 65)` 或
  `evm_mstore(4, 17)`。
- 原来的 `matchSolidityRevert` 只用 `getOffsetFromBase(offset, base)` 找
  code，所以 `base` 不是常量 0 时会漏掉 `mstore(4, code)`。

这会让已知的 panic `0x11/0x41` 变成 `panic_unknown`，后续 checked add/sub 和
resource guard matcher 都不会运行。

## 本轮选择

选择修 panic code 识别，而不是直接放宽 checked-bounds matcher：

- 两个 pattern case 共 14 个 skip，数量最高。
- IR 里能直接看到 Solidity panic ABI 形状，语义明确。
- 修完后仍由现有 matcher 决定能不能 rewrite；不能证明的仍保留 skip。
- 这不是 metadata-only：能识别出 panic code 后，已有 rewrite 会输出 semantic
  helper 并改 CFG。

不做：

- 不把任意 `mstore(4, x)` 都当 panic code。
- 不处理 `0334...` 的 `panic(1)` assert。
- 不处理 `2001...` 剩余的 private helper `panic(0x32)`。
- 不新增跨函数推理。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:362` 新增 `getUInt64Metadata`，用于复用
  已有 panic metadata 的数字值。
- `src/Passes/evm/SolidityPatterns.cpp:1444` 在 `matchSolidityRevert` 扫同一
  block 的 `evm_mstore` 时，记录绝对偏移 `4` 上的常量 panic code。
- `src/Passes/evm/SolidityPatterns.cpp:1529` 只在已经确认 `Kind == "panic"`、
  selector store 存在、revert length 是 36，且正常 base-relative 方式没有拿到
  code 时，才使用这个绝对 `mstore(4, code)`。
- `test/run_evm_solidity_patterns_suite.py:372` 让 `expected_panic_codes` 优先按
  `notdec_solidity_rewrite_revert_panic(i256 code)` 统计，避免测试工具继续只认
  绝对 `mstore(0/4)` 形状。
- `test/evm/solidity-patterns/manifest.json:1875` 更新 `0651...` oracle：
  `panic_unknown 7 -> 0`，`unrecognized_operands 7 -> 3`，CFG rewrite `23 -> 27`。
- `test/evm/solidity-patterns/manifest.json:2372` 更新 `0648...` oracle：
  `panic_unknown 7 -> 0`，`unrecognized_operands 7 -> 3`，CFG rewrite `23 -> 27`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，
  168.73s。
- pattern skip：16 -> 8。
- pattern panic `0x0` skip：14 -> 0。
- `0648...` 和 `0651...` 各新增 4 个 CFG rewrite。
- apehex batch589 同口径重跑：38 个输出，bad logs 0，`llvm-as` 38/38 passed，
  剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，
  16.55s。上一轮同口径是 16.32s。

复杂度评分：

- 实现效果：6/10。一次消掉 8 个 pattern skip，且都是真实 panic code 漏识别。
- 理解成本：2/10。只补 panic ABI 的局部识别，不改 checked-bounds 主逻辑。
- 维护成本：2/10。限制条件很窄，后续如果出现更多 panic ABI 变体再按真实形状补。

## 剩余 skip 和下一轮

pattern suite 当前剩余 8 个 skip：

- `0648_19497230_998d150449_ea367f7fc1ac`: 3 个，`{65: 3}`
- `0651_19497235_18ca6cbfa7_017843325734`: 3 个，`{65: 3}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 1 个，`{50: 1}`

batch589 当前剩余 2 个 skip：

- `23523_19745765_952a215d44_d3d3b55f272b`: 1 个，`{17: 1}`
- `23545_19746035_86a085849a_4d774e840428`: 1 个，`{17: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x41`：`0648...`、`0651...` 共 6 个。真实 case 数量最高，
   但需要确认是内存分配/resource guard 还是其他 compiler guard。
2. batch589 panic `0x11`：两个真实 case。真实数据价值高，但当前看起来像常量退化
   guard，不能只按 panic code rewrite。
3. pattern panic `0x32`：`2001...` 一个。需要证明 private helper 返回值和数组
   index/length 的关系。
4. pattern panic `0x1`：`0334...` 一个。需要确认是 Solidity compiler assert
   还是业务 assert。

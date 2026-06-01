# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后剩余真实 `panic 0x11` skip：

- batch615 `24562_19760107_e25027c623_f371f33b9104`：1 个。
- batch615 `24574_19760246_e537c886f5_6e80990d311f`：1 个。
- batch616 `24612_19760427_dd0dd0b57c_933311393a34`：1 个。

## 本轮选择

本轮处理两个语义明确的 compiler guard：

- `24562...` 的 signed checked negation。
- `24612...` 的 signed narrow sub range check。

真实 IR 形状：

- `24562...`
  - guard 是 `value == int256_min`，失败报 `Panic(0x11)`。
  - 成功分支计算 `sub i256 0, value`。
- `24612...`
  - 先 `signextend` 两个 48-bit signed 参数。
  - 成功条件是 `(x - y + 2^47) < 2^48`。
  - 失败分支报 `Panic(0x11)`。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4258` 的 `negateNumberCheckedFunction`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:922` 的 `overflowCheckedIntSubFunction`，signed 非 256-bit 路径检查结果是否落在 `[min, max]`。

不做的内容：

- 不处理 `24574...` 的大常量边界分支。它虽然是 `panic 0x11`，但更像业务常量域运算，先不按 Solidity checked arithmetic rewrite。
- 不处理 `panic 0x01`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2550`
  - 增加 `isSignedInt256MinValue`，识别 `2^255` 以及 `sub(0, 2^255)` 的 int256 min 表达。
- `src/Passes/evm/SolidityPatterns.cpp:2588`
  - 增加 `matchSignedNegationGuard`，匹配 `value == min` 和成功分支 `sub(0, value)`。
- `src/Passes/evm/SolidityPatterns.cpp:2614`
  - 增加 `matchSignedNarrowSubRangeGuard`，匹配 `(diff - min) >= range` 这种 signed narrow sub 失败条件。
- `src/Passes/evm/SolidityPatterns.cpp:3028`
  - 将 checked negation rewrite 为 `checked_sub(0, value)`。
  - 将 signed narrow sub rewrite 为 `checked_sub_bound`。
- `test/evm/solidity-patterns/cases/24562_19760107_e25027c623_f371f33b9104.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/cases/24612_19760427_dd0dd0b57c_933311393a34.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4410`
  - 增加两个 oracle，检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24562...`：输出过 `llvm-as`，skip `0`，`checked_sub` marker 2 个。
- 单 case `24612...`：输出过 `llvm-as`，skip `0`，新增 `checked_sub_bound` marker 1 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round50`：85 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round50：85 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 85/85 通过。
- batch615 round50：36 个输出，bad log 0，skip `2 {1:1,17:1}`，`llvm-as` 36/36 通过。
- batch616 round50：40 个输出，bad log 0，skip `4 {1:4}`，`llvm-as` 40/40 通过。
- fortune smoke：`elapsed=16.74`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch615 `24574...` 的 `panic 0x11`，1 个真实 skip。需要先证明它是 Solidity checked arithmetic，不然保留 skip。
2. pattern `0334...`、batch615 `24561...`、batch616 `24615.../24618.../24620.../24626...` 的 `panic 0x01`。当前都不能证明是 compiler-generated guard，暂不 rewrite。
3. 继续抽样新的 apehex 批次，优先找 `panic 0x11/0x32/0x41/0x65` 中语义明确且数量更多的形状。

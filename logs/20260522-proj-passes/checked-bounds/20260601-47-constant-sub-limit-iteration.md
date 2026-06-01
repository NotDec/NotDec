# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 batch615 只剩 2 个 skip：

- `24561_19760106_df158f473c_d42cbc9a7845`：`panic 0x01` 1 个。
- `24574_19760246_e537c886f5_6e80990d311f`：`panic 0x11` 1 个。

batch616 只剩 4 个 `panic 0x01` skip。pattern suite 只剩 `0334...` 的 `panic 0x01`。

## 本轮选择

本轮只处理 `24574...` 的 `panic 0x11`。

真实 IR 形状：

- 前面先用自定义错误限制两个坐标不大于 BN254 域值 `p - 1`。
- 后面有 guard `y < p + 1`，失败报 `Panic(0x11)`。
- 成功分支计算 `sub p, y`。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:922` 的 `overflowCheckedIntSubFunction`。
- 对 unsigned 256-bit sub，Solidity 生成 `diff := sub(x, y)`，再检查 underflow。这里 `x` 是常量 `p`，优化后留下 `y < p + 1`。

这是 checked unsigned sub 的 compiler guard，不是业务分支。

不做的内容：

- 不把任意常量边界比较都 rewrite。
- 只接受 failure 条件是 `rhs >= limit`，且成功分支确实有 `sub(limit - 1, rhs)`。
- 不处理 `panic 0x01`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2113`
  - 增加 `matchCheckedConstSubLimitGuard`，匹配 `rhs >= limit` / `rhs <= limit` 的失败条件和成功分支 `sub(limit - 1, rhs)`。
- `src/Passes/evm/SolidityPatterns.cpp:4832`
  - 在 checked arithmetic 通用 matcher 前调用该 matcher，rewrite 为 `checked_sub`。
- `test/evm/solidity-patterns/cases/24574_19760246_e537c886f5_6e80990d311f.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4517`
  - 增加 `24574...` oracle，检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24574...`：输出过 `llvm-as`，skip `0`，新增 `checked_sub` marker 1 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round51`：86 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round51：86 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 86/86 通过。
- batch615 round51：36 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 36/36 通过。
- fortune smoke：`elapsed=16.78`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. pattern `0334...`、batch615 `24561...`、batch616 `24615.../24618.../24620.../24626...` 的 `panic 0x01`。当前不能证明是 compiler-generated guard，优先保留 skip。
2. 继续抽样新的 apehex 批次。优先找 `panic 0x11/0x32/0x41/0x65` 中语义明确且数量更多的形状。

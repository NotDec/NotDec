# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 pattern suite 只剩 1 个 `panic 0x01` skip：

- `0334_19494307_668d201319_1354ce2e324d`: 1 个

batch617 抽样还有 1 个真实 case 有 checked-bounds skip：

- `24663_19760902_bbe5fabbb5_b34e557279fe`: `panic 0x11` 3 个

## 本轮选择

本轮只处理 `24663...` 的 3 个真实 `panic 0x11`。

真实 IR 形状：

- `private__0x9b1_0x9b1` 里有 signed mul：
  - 先算 `mul(x, y)`。
  - 第一段 guard 是 `x >= 0 || y != int_min`，对应 Solidity signed 256-bit mul 的 special case。
  - 第二段 guard 是 `x == 0 || y == sdiv(product, x)`，对应 signed mul overflow check。
- `private__0x9e0_0x9e0` 里有 signed div：
  - 已有除零 guard。
  - 后面还有 `x != int_min || y != -1`，对应 signed div 的 `min / -1` overflow guard。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:780` 的 `overflowCheckedIntMulFunction`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:850` 的 `overflowCheckedIntDivFunction`。

这些都是 compiler checked arithmetic guard，不是业务分支。

不做的内容：

- 不把任意 `panic 0x11` 的 `or` 条件都当作 signed arithmetic。
- 不处理 pattern suite 剩余的 `panic 0x01`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2672`
  - 增加 `matchSignedMulMinValueSpecialCase`，匹配 `x >= 0 || y != int_min`，并要求同 block 里存在 `mul(x, y)`。
  - 增加 `matchSignedMulDivisionCheck`，匹配 `x == 0 || y == sdiv(product, x)`，并要求 `product` 是同一组 `x/y` 的 `mul`。
  - 增加 `matchSignedDivMinValueOverflow`，匹配 `x != int_min || y != -1`。
- `src/Passes/evm/SolidityPatterns.cpp:2766`
  - 扩展 `matchSignedCheckedArithmeticGuard`，将上述形状 rewrite 为 `checked_mul` / `checked_div`。
- `test/evm/solidity-patterns/cases/24663_19760902_bbe5fabbb5_b34e557279fe.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4237`
  - 增加 `24663...` oracle，检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round46`：81 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round46：81 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 81/81 通过。
- batch617 round46：38 个输出，bad log 0，skip `0 {}`，`llvm-as` 38/38 通过。
- fortune smoke：`elapsed=17.21`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. pattern `0334...` 的 `panic 0x01`，1 个 case。需要先确认是 compiler assert 还是业务 assert；如果不能证明，就保留 skip。
2. 继续抽样更新的 apehex 批次，找新的真实 checked-bounds skip。batch589 和 batch617 当前都已清零。

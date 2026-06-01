# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 batch615 还有 7 个 skip：

- `24562_19760107_e25027c623_f371f33b9104`：`panic 0x11` 1 个。
- `24574_19760246_e537c886f5_6e80990d311f`：`panic 0x11` 1 个。
- `24590_19760322_8a0f96cd7a_ecc07eeb03e3`：`panic 0x11` 2 个。
- `24561_19760106_df158f473c_d42cbc9a7845`：`panic 0x01` 1 个。
- `24576_19760248_3fe89a8330_a66660aa706c`：`panic 0x32` 2 个。

## 本轮选择

本轮只处理 `24590...` 里的 2 个 `panic 0x11`。

真实 IR 形状：

- packed `uint16` 自增：
  - guard 是 `and(sload, 65535) == 65535`，失败报 `Panic(0x11)`。
  - 成功分支先对原 storage word 做 `add sload, 1`，再 `and ..., 65535` 写回低 16 位。
- packed `uint16` 自减：
  - guard 是 `and(sload, 65535) == 0`，失败报 `Panic(0x11)`。
  - 成功分支先对原 storage word 做 `add sload, 65535`，等价于低 16 位减 1，再 mask 写回。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4174` 的 `decrementCheckedFunction`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4216` 的 `incrementCheckedFunction`。

这些是 checked integer inc/dec 的 compiler guard，不是业务分支。

不做的内容：

- 不把任意 `and(x, 65535) == const` 都当作 checked arithmetic。
- 不处理 `24562...` 的 signed min negation、`24574...` 的常量域运算、`panic 0x01`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2363`
  - 增加 `getUnsignedCleanupSource`，识别 `and(raw, max)` 的原始值。
- `src/Passes/evm/SolidityPatterns.cpp:2377`
  - 增加 `findCheckedCleanedStepResult`，让 cleaned inc/dec 能匹配 packed offset 0 的 `raw +/- 1` 写回形状。
- `src/Passes/evm/SolidityPatterns.cpp:2425`
  - 增加 `matchCleanedUnsignedDecrement`，匹配 cleaned unsigned decrement guard。
- `src/Passes/evm/SolidityPatterns.cpp:3004`
  - 将 cleaned decrement rewrite 为 `checked_sub_bound`。
- `test/evm/solidity-patterns/cases/24590_19760322_8a0f96cd7a_ecc07eeb03e3.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4342`
  - 增加 `24590...` oracle，检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24590...`：输出过 `llvm-as`，skip `0`，新增 `checked_add_bound` 1 个、`checked_sub_bound` 1 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round49`：83 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round49：83 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 83/83 通过。
- batch615 round49：36 个输出，bad log 0，skip `3 {17:2,1:1}`，`llvm-as` 36/36 通过。
- fortune smoke：`elapsed=16.72`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch615 `24576...` 的 `panic 0x32`，2 个真实 skip。先确认是否是 storage/memory array bounds 的新形状。
2. batch615 `24562...` 的 `panic 0x11`，1 个真实 skip。看起来像 signed min negation guard，需对照 Solidity checked negation / sub codegen。
3. batch615 `24574...` 的 `panic 0x11`，1 个真实 skip。存在大常量边界，先确认是否是 compiler arithmetic guard，不能直接按业务常量分支 rewrite。
4. pattern `0334...`、batch615 `24561...`、batch616 多个 `panic 0x01`。语义不够确定，除非能证明是 compiler-generated assert，否则继续保留 skip。

# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 pattern suite 只剩 2 个 skip：

- `0334_19494307_668d201319_1354ce2e324d`: `panic 0x01` 1 个
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: `panic 0x32` 1 个

batch589 真实 IR 输出还剩 2 个 `panic 0x11` skip：

- `23523_19745765_952a215d44_d3d3b55f272b`: 1 个
- `23545_19746035_86a085849a_4d774e840428`: 1 个

## 本轮选择

本轮只处理 batch589 的两个真实 `panic 0x11`。这两个 case 数量最多，而且形状完全一致。

真实 IR 原始形状：

- 循环头从 0 开始，条件是 `i < 1`。
- 循环 latch 处有 Solidity checked increment：先检查 `i != 2^256 - 1`，成功后计算 `i + 1` 并回到循环头。
- 因为循环上界是 1，后续清理把 `i` 折成常量，最终剩下 `evm_shl(255, 1) == -1` 或 `evm_shl(255, 1) == 1` 这种恒假的 panic 条件。

Solidity codegen 对应 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:720` 的 `overflowCheckedIntAddFunction`：`uint256` 加法用 `if gt(x, sum) { panic(0x11) }` 检查溢出。这里是编译器 checked add guard，被常量传播清理后才变成恒假条件，不是业务分支。

不做的内容：

- 不把普通恒假 `panic 0x11` 分支都 rewrite。
- 不处理 pattern suite 剩余的 `panic 0x01` / `panic 0x32`。
- 不尝试修复更宽泛的 loop induction 识别。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2395`
  - 增加 `isShiftLeft255One` 和 `isCleanedConstantFalseIncrementOverflowCondition`，只识别 `evm_shl(255, 1)` 与 `1/-1` 的相等比较。
- `src/Passes/evm/SolidityPatterns.cpp:2425`
  - 增加 `hasSingleIterationLoopHeader`，要求成功边是单次循环头：header 分支条件是 i1 PHI，guard block 是 header 的一个 successor，guard 回来后 header 退出，入口从 false 开始进入循环。
- `src/Passes/evm/SolidityPatterns.cpp:2462`
  - 增加 `matchCleanedConstantFalseIncrementGuard`，只在 `panic 0x11`、失败边为真、恒假 increment guard、单次循环头同时成立时 rewrite 为 `checked_add`。
- `src/Passes/evm/SolidityPatterns.cpp:4459`
  - 在 checked-bounds 匹配链路中接入这个 matcher。
- `test/evm/solidity-patterns/cases/23523_19745765_952a215d44_d3d3b55f272b.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/cases/23545_19746035_86a085849a_4d774e840428.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4162`
  - 增加两个真实 case 的 oracle，检查 checked-bounds kind、panic code、`notdec_solidity_rewrite_checked_add` marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round43`：80 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round43：80 个输出，bad log 0，skip `2 {1:1, 50:1}`，`llvm-as` 80/80 通过。
- batch589 round44：38 个输出，bad log 0，skip `0 {}`，`llvm-as` 38/38 通过。
- fortune smoke：`elapsed=16.81`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. pattern `2001...` 的 `panic 0x32`，1 个 case。可能是数组下标/长度 helper 关系，需先确认不是业务越界检查。
2. pattern `0334...` 的 `panic 0x01`，1 个 case。要先区分 compiler assert 和业务 assert。
3. 更大 batch 继续抽样统计。batch589 当前已无 checked-bounds skip，下一步应从更多 apehex 批次里重新找真实高频 skip。

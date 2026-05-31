# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

`/tmp/notdec-patterns-cb-run3` 基于 `test/evm/solidity-patterns/manifest.json` 的输出里，checked-bounds skip 共 281 个：

- `unrecognized_operands`: 216
- `unsupported_condition`: 64
- `operand_not_available_at_guard`: 1

其中 `panic_checked_arithmetic` / panic code 17 的 `unrecognized_operands` 有 86 个，是这轮里语义最清楚、数量也最高的候选。

# 本轮目标

只扩展 panic 0x11 的 checked arithmetic 识别，不碰资源错误、enum、pop 和 storage encoding：

- 识别 `x == UINT_MAX` 后成功块里执行 `x + 1`。
- 识别 `x > ~y` 后成功块里执行 `x + y`。
- 识别 `x < y` 后成功块里执行 `x - y`。

这些都是 Solidity checked arithmetic guard。只有成功块里能找到对应算术操作时才 rewrite，否则继续保留 skip。

# 判断标准

- pattern suite 中对应 skip 数下降，并出现新的 `checked_add` / `checked_sub` rewrite。
- runner 继续检查 semantic marker、skip reason、CFG rewrite。
- 输出过 `llvm-as`，`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 通过。

# 实现记录

- `src/Passes/evm/SolidityPatterns.cpp:149` 新增 `isAllOnes`，用于识别 `UINT_MAX` 常量。
- `src/Passes/evm/SolidityPatterns.cpp:1563` 新增 `matchBitwiseNot`，只接受 `xor x, -1` 形式。
- `src/Passes/evm/SolidityPatterns.cpp:1687` 扩展 `matchCheckedArithmetic`，传入 success block，并识别：
  - `x == -1` 后 success block 里的 `x + 1`。
  - `x > ~y` 后 success block 里的 `x + y`。
  - `x < y` 后 success block 里的 `x - y`。
- `src/Passes/evm/SolidityPatterns.cpp:1987` 允许 `checked_add` / `checked_sub` 的 result 在 success block。
- `src/Passes/evm/SolidityPatterns.cpp:2018` 对 success block 里的 add/sub result 在 guard 前重建 semantic marker 参数，然后再 rewrite CFG。
- `test/evm/solidity-patterns/manifest.json:1740` 和 `test/evm/solidity-patterns/manifest.json:2099` 更新两个真实合约 oracle：`checked_add` 从 1 增到 6，CFG rewrite 从 3 增到 8，`unrecognized_operands` 从 25 降到 20。

# 验证结果

- `cmake --build ./build --target all -j4` 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null` 通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py` 通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round4`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure` 通过。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round4.c --tr-level=2` 通过，elapsed=16.31。

# 效果

`test/evm/solidity-patterns/manifest.json` 全量输出对比：

- skip 总数从 281 降到 240。
- `unrecognized_operands` 从 216 降到 175。
- panic 17 的 `unrecognized_operands` 从 86 降到 45。
- `checked_add` marker 从 25 增到 59。
- `checked_sub` marker 从 25 增到 32。

apehex batch589 的 38 个输出重新跑 `--tr-level=0` 后：

- `checked_add`: 11
- `checked_sub`: 8
- `checked_div`: 7
- `checked_mul`: 5
- skip: `unrecognized_operands` 18，`unsupported_condition` 10。

# 下一轮候选

1. panic 0x32 array out of bounds：pattern suite 剩 28 个、apehex batch589 剩 6 个，真实数量较高，语义确定性高；风险在于要确认 `lt.not` / 零长度形状是不是数组下标 guard，不要把业务长度判断 rewrite。
2. panic 0x11 剩余复杂 arithmetic：pattern suite 还有 45 个 `unrecognized_operands`，apehex batch589 还有 6 个；可能包含 masked-width mul/add，语义确定性中等，rewrite 风险高于本轮。
3. panic 0x65 resource error：pattern suite 93 个、apehex batch589 11 个；数量高，但不是 checked bounds，应该单独定义 resource guard，不应该混进 array/arithmetic rewrite。
4. panic 0x33 / 0x49：数量较小，语义明确但不是本轮 checked arithmetic/bounds 主路径，测试成本低，收益低。

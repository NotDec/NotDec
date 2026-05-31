# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，`test/evm/solidity-patterns/manifest.json` 全量输出仍有 panic 0x11 的算术 skip：

- `unrecognized_operands`: 45
- `unsupported_condition`: 11

apehex batch589 中也有 panic 0x11：

- `unrecognized_operands`: 6
- `unsupported_condition`: 1

抽样看到两类 Solidity checked multiplication 形状：

- `y == 0 || x <= UINT_MAX / y` 成功，成功块里执行 `x * y`。
- 已有 `y != 0` 前置后，`x > UINT_MAX / y` 失败，成功块里执行 `x * y`。

# 本轮目标

只处理 `UINT_MAX / y` 形式的 checked multiplication guard：

- panic code 必须是 0x11。
- bound 必须是 `evm_div(-1, y)`。
- 对 `or` 条件，必须同时看到 `y == 0` 和 `x <= UINT_MAX / y`。
- 成功块里必须有 `x * y`。
- 不能证明这些条件时继续 skip。

# 判断标准

- pattern suite 中 panic 0x11 的 skip 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_checked_mul` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

- `src/Passes/evm/SolidityPatterns.cpp:1563` 新增 `findCommutativeBinaryOpInBlock`，用于在 success block 里找 `x * y`，不要求操作数顺序。
- `src/Passes/evm/SolidityPatterns.cpp:1633` 新增 `matchMulByMaxDivBound`，只接受 `evm_div(-1, y)` 作为乘法上界。
- `src/Passes/evm/SolidityPatterns.cpp:1665` 新增 `matchCheckedMulMaxDivSuccessCondition`，识别 `y == 0 || x <= UINT_MAX / y`。
- `src/Passes/evm/SolidityPatterns.cpp:1731` 扩展 `matchCheckedMulGuard`，在 success 条件分支里识别 max-div 乘法 guard。
- `src/Passes/evm/SolidityPatterns.cpp:1838` 扩展 `matchCheckedArithmetic`，识别直接失败条件 `x > UINT_MAX / y`。
- `src/Passes/evm/SolidityPatterns.cpp:2091` 和 `src/Passes/evm/SolidityPatterns.cpp:2123` 允许 `checked_mul` 的 result 在 success block，并在 semantic marker 前重建 `mul` 参数。
- `test/evm/solidity-patterns/manifest.json:3323` 更新真实 case `0147_19493380_262c2cb2b2_c24931e22d66` 的 checked-bounds oracle，新增 `checked_mul` marker 和 CFG rewrite 检查。

# 验证结果

- `cmake --build ./build --target all -j4` 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null` 通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py` 通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round6`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure` 通过。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round6.c --tr-level=2` 通过，elapsed=16.40。

# 效果

`test/evm/solidity-patterns/manifest.json` 全量输出对比上一轮：

- skip 总数从 214 降到 194。
- `unrecognized_operands` 从 149 降到 137。
- `unsupported_condition` 从 64 降到 56。
- panic 0x11 的 `unrecognized_operands` 从 45 降到 33。
- panic 0x11 的 `unsupported_condition` 从 11 降到 3。
- `checked_mul` marker 从 22 增到 42。

apehex batch589 的 38 个输出重新跑 `--tr-level=0` 后：

- panic 0x11 skip 从 7 降到 3。
- `checked_mul` marker 从 5 增到 9。
- skip 总数从 22 降到 18。

# 剩余问题和下一轮候选

1. panic 0x65 resource error：pattern suite 仍有 93 个相关 skip，apehex batch589 有 11 个。真实 case 数最高，但不是 checked arithmetic / array bounds，需要先单独定义 resource guard 的接口和 rewrite 语义。
2. panic 0x11 剩余 arithmetic：pattern suite 还有 33 个 `unrecognized_operands` 和 3 个 `unsupported_condition`，apehex batch589 还有 3 个 `unrecognized_operands`。下一步看 masked-width mul、storage decrement 和 shift/exponent overflow，rewrite 风险中等。
3. panic 0x34 storage encoding：pattern suite 25 个、apehex batch589 4 个，语义偏 storage byte array 编码，不应混进 arithmetic matcher。
4. panic 0x32 剩余 2 个：仍在 `2001_19510128_72aa4f70b3_e9ee7c9c0e79`，需要单独确认 calldata loop 的 index/length 来源。

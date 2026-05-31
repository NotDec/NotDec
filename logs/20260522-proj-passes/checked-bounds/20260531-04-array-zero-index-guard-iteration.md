# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，`test/evm/solidity-patterns/manifest.json` 全量输出仍有 28 个 panic 0x32 的 `unrecognized_operands`。apehex batch589 中也有 6 个同类 case。

抽样看 IR 后，这批形状一致：

- 读取 memory array length：`%len = call i256 @evm_mload(...)`
- guard 是 `icmp eq i256 %len, 0`
- true 分支进入 `Panic(0x32)`
- false 分支继续写入或读取数组首元素

这对应 Solidity 对 `arr[0]` 的 bounds guard：`0 < arr.length`。因为下标是常量 0，提升后条件会变成 `length != 0`。

# 本轮目标

只处理 panic 0x32 下的零下标数组越界 guard：

- 只接受 `length == 0` 失败条件。
- `length` 必须能分类成已有 array bounds length 来源，例如 `evm_mload`。
- marker 使用 `index = 0` 和真实 `length`。
- 不能识别来源时继续 skip。

# 判断标准

- pattern suite 中 panic 0x32 的 `unrecognized_operands` 明显下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_array_bounds_memory` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

- `src/Passes/evm/SolidityPatterns.cpp:1841` 扩展 `matchArrayBounds`：当 panic code 是 0x32 且失败条件是 `length == 0` 时，把它识别为 `index = 0, length = length` 的数组越界 guard。
- `src/Passes/evm/SolidityPatterns.cpp:1850` 仍复用原有 length 来源分类；只有 `evm_mload` / `evm_calldataload` / `evm_sload` 等已支持来源才 rewrite，否则继续 skip。
- `test/evm/solidity-patterns/manifest.json:3323` 给真实 case `0147_19493380_262c2cb2b2_c24931e22d66` 增加 checked-bounds oracle，检查 `array_bounds_memory` marker、panic code、skip reason 和 CFG rewrite。

# 验证结果

- `cmake --build ./build --target all -j4` 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null` 通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py` 通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round5`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure` 通过。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round5.c --tr-level=2` 通过，elapsed=16.33。

# 效果

`test/evm/solidity-patterns/manifest.json` 全量输出对比上一轮：

- skip 总数从 240 降到 214。
- `unrecognized_operands` 从 175 降到 149。
- panic 0x32 的 `unrecognized_operands` 从 28 降到 2。
- `array_bounds_memory` marker 从 187 增到 213。

apehex batch589 的 38 个输出重新跑 `--tr-level=0` 后：

- panic 0x32 的 `unrecognized_operands` 从 6 降到 0。
- `array_bounds_memory` marker 从 13 增到 19。
- skip 总数从 28 降到 22。

# 剩余问题和下一轮候选

1. panic 0x11 剩余 arithmetic：pattern suite 还有 45 个 `unrecognized_operands` 和 11 个 `unsupported_condition`，apehex batch589 还有 6 个 `unrecognized_operands` 和 1 个 `unsupported_condition`。数量最高，下一轮优先看 masked-width mul/add 和 `and/or` 条件。
2. panic 0x65 resource error：pattern suite 仍有 93 个相关 skip，apehex batch589 有 11 个。数量高，但不是 array bounds，应该单独定义 resource guard，不要塞进 checked array/arithmetic。
3. panic 0x32 剩余 2 个：都在 `2001_19510128_72aa4f70b3_e9ee7c9c0e79`，是 `icmp ult index, length` 的循环/calldata 形状；语义看起来合理，但需要进一步确认 length 来源和 success block 使用，不能本轮顺手放开。
4. panic 0x33 / 0x49：数量小，语义较明确，收益低于前两项。

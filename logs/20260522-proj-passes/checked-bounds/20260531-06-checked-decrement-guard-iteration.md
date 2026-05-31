# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，panic 0x11 仍有若干 skip。按真实 IR 抽样分类：

- `x == 0` 后 success block 做 `x - 1`：8 个。
- packed / masked width 加法：约 24 个，但涉及 storage 位宽 mask。
- masked multiply：1 个。
- 其它 signed / shift / exponent 相关形状少量。

本轮只处理 `x == 0 -> x - 1`。这是 Solidity checked decrement，语义明确，rewrite 风险低。

# 本轮目标

识别 panic 0x11 下的 checked decrement：

- 失败条件必须是 `x == 0`。
- success block 里必须有 `x - 1`，IR 中通常是 `add x, -1`。
- marker 输出 `checked_sub(x, 1, x - 1)`。
- 找不到 success block 算术结果时继续 skip。

# 判断标准

- pattern suite 中 panic 0x11 的 `unrecognized_operands` 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_checked_sub` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

本轮已完成。

改动：

- `src/Passes/evm/SolidityPatterns.cpp:1814` 的 `matchCheckedArithmetic` 中，在 panic 0x11 / `icmp eq` 分支下新增 `x == 0` 识别。
- `src/Passes/evm/SolidityPatterns.cpp:1843` 要求 success block 里存在 `add x, -1` 或 `add -1, x`，才 rewrite 为 `checked_sub(x, 1, x - 1)`。
- `test/evm/solidity-patterns/manifest.json:1740` 更新真实 case `0651_19497235_18ca6cbfa7_017843325734` 的 oracle：`checked_sub` 从 1 增到 6，CFG rewrite 从 8 增到 13，`unrecognized_operands` 从 20 降到 15。
- `test/evm/solidity-patterns/manifest.json:2099` 更新真实 case `0648_19497230_998d150449_ea367f7fc1ac` 的同类 oracle。

不做：

- 不处理 packed / masked width 加法。那些形状和 storage mask 混在一起，不能只看局部 `and` 就 rewrite。
- 不处理 signed / shift / exponent overflow。数量少，且需要单独确认 Solidity codegen。

# 验证结果

命令：

```bash
cmake --build ./build --target all -j4
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null
python3 -m py_compile test/run_evm_solidity_patterns_suite.py
python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round7
ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round7.c --tr-level=2
```

结果：

- pattern suite：78 passed, 0 failed。
- ctest：`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 通过。
- fortune smoke：通过，`elapsed=16.50`。

效果：

- pattern suite skip 总数：194 -> 183。
- pattern suite panic 0x11 skip：36 -> 25。
- pattern suite `checked_sub` marker：64 -> 75。
- apehex batch589 重新过 `notdec --tr-level=0` 后没有变化：skip 总数仍 18，panic 0x11 skip 仍 3，说明这批样本没有命中本轮 decrement 形状。

# 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic 0x65 resource error：pattern suite 仍有 93 个，apehex batch589 有 11 个。数量最高，但不是 checked arithmetic / array bounds，需要先定义清楚 semantic helper，不应继续塞进 checked arithmetic。
2. panic 0x34 storage encoding：pattern suite 25 个，apehex batch589 有 4 个。语义比较固定，但更接近 storage byte array 编码，应该单独看 Solidity codegen。
3. panic 0x11 剩余 arithmetic：pattern suite 还有 25 个 skip，apehex batch589 还有 3 个。里面主要是 masked-width add / mul、signed overflow、shift / exponent，数量还值得看，但 rewrite 风险高于本轮。
4. panic 0x50 剩余 array bounds：pattern suite 还有 2 个，apehex batch589 为 0。数量少，适合后面补尾。

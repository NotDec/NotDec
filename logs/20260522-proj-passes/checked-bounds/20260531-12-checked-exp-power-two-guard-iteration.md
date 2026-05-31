# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后：

- pattern suite：122 个 checked-bounds skip，其中 panic `0x11` 有 25 个。
- apehex batch589：4 个 skip，其中 panic `0x11` 有 3 个。

apehex 有一个清楚的 `2 ** exponent` guard：

```llvm
%ok = icmp ult i256 %exponent, 256
br i1 %ok, label %success, label %panic_0x11
success:
  %power = call i256 @evm_shl(i256 %exponent, i256 1)
```

Solidity `checked_exp_unsigned` 对 base 2 也有同类检查：`exponent > 255` 时触发 `Panic(0x11)`。

# 本轮目标

只识别 base 为 2 的 checked exponent guard：

- panic code 必须是 `0x11`。
- 失败条件必须等价于 `exponent >= 256` 或 `exponent > 255`。
- success block 必须计算 `evm_shl(exponent, 1)` 或 `evm_exp(2, exponent)`。
- rewrite 后输出 `notdec_solidity_rewrite_checked_exp(base, exponent)` 并改 CFG。

# 不做

- 不处理常量 `power > max` 这类后置检查。
- 不处理 signed exponentiation 的复杂溢出分支。
- 不把小位宽 add/sub/mul 混进本轮。

# 判断标准

- pattern suite 的 panic `0x11` skip 下降。
- apehex batch589 的 panic `0x11` skip 至少下降一个。
- 强 oracle 检查 `checked_exp` kind、semantic marker、skip reason 和 CFG rewrite。
- pattern suite / rewrite suite / `llvm-as` / fortune smoke 通过。

# 实现记录

## 代码

- `src/Passes/evm/SolidityPatterns.cpp:2202` 新增 `hasPowerOfTwoExpComputation`，只接受 success block 里的 `evm_shl(exponent, 1)` 或 `evm_exp(2, exponent)`。
- `src/Passes/evm/SolidityPatterns.cpp:2226` 新增 `matchPowerOfTwoExpGuard`，只接受 panic `0x11` 且失败条件等价于 `exponent >= 256` / `exponent > 255`。
- `src/Passes/evm/SolidityPatterns.cpp:2739` 在 checked arithmetic 后尝试该 matcher。
- `src/Passes/evm/SolidityPatterns.cpp:2796` 新增 semantic marker `notdec_solidity_rewrite_checked_exp`。

## 测试和文档

- `test/evm/solidity-patterns/manifest.json:1910` 更新 `1991_19510015_7f60edd6aa_6188d2031ddf` 的强 oracle，新增 `checked_exp`、semantic marker 和 CFG rewrite 计数。
- `test/evm/solidity-patterns/manifest.json:2084` 更新 `1775_19507435_1be1a16c5d_eb304133e7c9` 的强 oracle，新增 `checked_exp`、semantic marker 和 CFG rewrite 计数。
- `docs/evm/entry-control/checked-bounds.md:60` 补充 base 2 exponent guard 形状。

# 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `git diff --check`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round13`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589：38 个 `.ll`，全部通过，输出在 `/tmp/notdec-apehex-cb-589-round13`。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 round13 的 116 个输出：0 bad。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round13.c --tr-level=2`，通过，`elapsed=16.17`。

# 效果

pattern suite：

- skip 总数：122 -> 118。
- panic `0x11` skip：25 -> 21。
- `checked_exp` marker：0 -> 4。

apehex batch589：

- skip 总数：4 -> 3。
- panic `0x11` skip：3 -> 2。
- `checked_exp` call：0 -> 1。

# 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic `0x41` 剩余资源检查：pattern suite 还有 56 个，apehex batch589 还有 1 个。数量最高，但要继续按真实 IR 聚类，不能把业务上限判断混进去。
2. panic `0x11` 小位宽 add/sub/mul：pattern suite 剩 21 个，apehex batch589 剩 2 个。语义较清楚，但需要确认 helper 是否要带位宽，不能直接当 256-bit 运算。
3. panic `0x33` enum conversion：pattern suite 剩 11 个。语义明确，apehex 当前没有，测试成本低。
4. panic `0x49` empty array pop：pattern suite 剩 11 个。语义明确，apehex 当前没有。

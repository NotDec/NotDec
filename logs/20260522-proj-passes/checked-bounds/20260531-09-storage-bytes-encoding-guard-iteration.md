# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，panic 0x34 在 pattern suite 和 apehex batch589 中仍然稳定出现。

抽样后，主要形状是 Solidity storage bytes/string 编码检查：

```llvm
%shr = evm_shr(1, %slot)
%low = and %slot, 1
%is_short = icmp eq %low, 0
%short_len = and %shr, 127
%len = select %is_short, %short_len, %shr
%low_bit = trunc %slot to i1
%too_long_for_short = icmp ugt %len, 31
%bad = xor %too_long_for_short, %low_bit
br i1 %bad, label %panic_0x22, label %success
```

含义是：

- short encoding：最低位是 0，长度必须小于 32。
- long encoding：最低位是 1，长度必须大于等于 32。

数量：

- pattern suite：24 个。
- apehex batch589：4 个。

# 本轮目标

识别并 rewrite 这类 storage bytes/string encoding guard：

- panic code 必须是 0x22。
- 条件必须来自 `xor`，两侧分别是 slot 低位和 length 与 31/32 的比较。
- length 必须由同一个 slot 经 `shr 1`、`and 127`、`select` 计算出来。
- marker 输出 `notdec_solidity_rewrite_storage_bytes_encoding(slot, length)`。

# 不做

- 不处理不符合该 shape 的唯一 0x34 other case。
- 不推断业务里的普通 bit check。
- 不把 storage encoding 混进 array bounds 或 arithmetic。

# 判断标准

- pattern suite 的 panic 0x34 skip 下降。
- apehex batch589 的 panic 0x34 skip 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_storage_bytes_encoding` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

## 代码

- `src/Passes/evm/SolidityPatterns.cpp:2099` 新增 `matchStorageBytesLength`，只接受同一个 slot 产生的 `shr 1`、`and 127`、`select` length。
- `src/Passes/evm/SolidityPatterns.cpp:2175` 新增 `matchStorageBytesEncoding`，只匹配 panic `0x22`、`xor`、slot 低位和 `length > 31` / `length < 32` 这两种 guard 形状。
- `src/Passes/evm/SolidityPatterns.cpp:2351` 在普通 normalized condition 之前尝试该 matcher，因为这类 `xor i1` 不是普通 `icmp`。
- `src/Passes/evm/SolidityPatterns.cpp:2447` 增加 marker `notdec_solidity_rewrite_storage_bytes_encoding`。

## 测试和文档

- `test/evm/solidity-patterns/manifest.json:1740`、`test/evm/solidity-patterns/manifest.json:2100` 更新两个真实 IR oracle：各新增 1 个 `storage_bytes_encoding`、1 个 semantic marker，CFG rewrite 从 13 到 14，`unsupported_condition` 从 2 到 1。
- `docs/evm/entry-control/checked-bounds.md:81` 简要补充 storage bytes/string 编码检查形状。

# 验证结果

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round10`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260531-evm2llvm-train-batch589/outputs` 下 38 个 `.ll`，全部通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round10.c --tr-level=2`，通过，`elapsed=16.40`。

# 效果

pattern suite：

- skip 总数：167 -> 143。
- panic 0x34 skip：25 -> 1。
- 新 marker 引用：48。

apehex batch589：

- skip 总数：10 -> 6。
- panic 0x34 skip：4 -> 0。
- 新 marker 引用：8。

# 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic `0x41` resource guard：pattern suite 剩 77 个，apehex batch589 剩 3 个。数量最多，但要继续区分 memory allocation size、free pointer 和其他资源检查，不能把业务上限判断混进去。
2. panic `0x11` checked arithmetic：pattern suite 剩 25 个，apehex batch589 剩 3 个。语义明确，但形状分散，需要先按真实 IR 聚类。
3. panic `0x33` enum conversion：pattern suite 剩 11 个。语义较明确，测试成本低；收益小于 `0x41` 和 `0x11`。
4. 剩余 panic `0x34`：pattern suite 只剩 1 个，apehex batch589 已清零。先留作低优先级，避免为单例放宽 matcher。

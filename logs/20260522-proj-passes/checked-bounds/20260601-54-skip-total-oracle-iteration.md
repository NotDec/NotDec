# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 pattern suite checked-bounds 剩余 skip：

- `0334_19494307_668d201319_1354ce2e324d`：`unrecognized_operands` 1 个，`panic 0x01`。

当前 apehex 最新 batch620-624：

- outputs 合计 190 个左右。
- checked-bounds skipped metadata：0。

`0334...` 的剩余 `panic 0x01` 在 helper `private__0x2dad_0x2dad` 里：

```llvm
%cond = icmp ugt i256 %arg0, %arg1
br i1 %cond, label %panic_0x01, label %success
```

success 只继续调用另一个 helper。当前只能确认这是 assert 形状，不能证明是 bounds/compiler guard，所以本轮不 rewrite。

同时发现一个真实弱 oracle：`manifest.json` 里大量 case 写了
`"expected_checked_bounds_skip_reasons": {}`，但 runner 只遍历字典里的 reason。空字典不会检查“实际 skip 总数为 0”，可能漏掉新增 skip。

## 本轮选择

修 runner 的 skip 总数 oracle：

- 如果 case 显式写了 `expected_checked_bounds_skip_reasons`，就额外检查 `checked_bounds_skip_total`。
- 空字典代表期望实际 skip 总数为 0。

不做：

- 不把 `panic 0x01` 改成 semantic helper。
- 不放宽 checked-bounds matcher。
- 不修改 manifest 的语义计数。

## 修改

- `test/run_evm_solidity_patterns_suite.py:547`
  - 根据 `expected_checked_bounds_skip_reasons` 计算 `checked_bounds_skip_total` 期望值。
- `test/run_evm_solidity_patterns_suite.py:662`
  - 对输出里的 `notdec.solidity_checked_bounds.skipped` metadata 求总数，并和期望比较。

## 验证

- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-skip-total`：90 passed, 0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-skip-total`：65 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

本轮没有改 pass，不需要 fortune 性能对比。

## 剩余和下一轮候选

当前 manifest 剩余 checked-bounds skip：

- `0334_19494307_668d201319_1354ce2e324d`：`unrecognized_operands` 1 个，`panic 0x01`。

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 最新 apehex batch 继续抽样统计。真实 case 数量高，当前 checked-bounds skip 为 0，但能发现新形状；语义确定性取决于样本；测试成本中等。
2. `0334...` 的 `panic 0x01`。真实 case 1 个，语义不确定，rewrite 风险高，测试成本低。除非能找到 Solidity 源或更明确 helper 语义，否则继续保留 skip。
3. batch612 历史 `0x11/0x12`。真实 case 数量较高，但可能已被后续迭代覆盖；需要用当前 HEAD 重跑或复查旧输出，测试成本中等。

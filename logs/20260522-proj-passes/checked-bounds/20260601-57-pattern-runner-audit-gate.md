# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 checked-bounds 状态：

- pattern 输出：775 个 checked-bounds metadata，1 个保留 skip，774 个 semantic marker，774 个 CFG rewrite。
- apehex batch620-625：229 个 `.ll`，checked-bounds metadata 和 skip 都是 0。
- `0334_19494307_668d201319_1354ce2e324d` 的 `panic 0x01` 仍不能证明是 bounds/compiler guard，不 rewrite。

上一轮加了 `scripts/audit-checked-bounds.py --fail-on-mismatch`，但仍需要人工单独运行。这样 CTest 只跑 pattern suite 时，不能自动防止 metadata-only 回归。

## 本轮选择

把 checked-bounds audit gate 接入 `run_evm_solidity_patterns_suite.py`：

- pattern suite 生成全部输出后，自动运行
  `scripts/audit-checked-bounds.py <workdir> --fail-on-mismatch`。
- audit 失败时，pattern suite 直接失败。

不做：

- 不改 CheckedBoundsPass matcher。
- 不处理 `panic 0x01`。
- 不把 apehex 无 skip 当作完成。

## 修改

- `test/run_evm_solidity_patterns_suite.py`
  - 在所有 case 跑完后调用 checked-bounds audit 脚本。
  - 将审计输出写到 `checked-bounds-audit.log`。
  - 审计失败时增加失败计数并报告日志路径。

## 验证

- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-audit-integrated`：90 passed, 0 failed。
  - 结尾自动执行 `[PASS ] checked_bounds_audit`。
  - audit 结果：`rewrite_expected: 774`，`rewrite_markers: 774`，`cfg_rewrites: 774`。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

本轮没有改 C++ pass，不需要 build / fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续扫后续 apehex batch。真实 case 数量最高；当前 batch620-625 没有 checked-bounds skip。
2. 若后续 batch 仍无 skip，考虑把 apehex audit 结果汇总成长期 CSV，便于看趋势。语义确定，风险低，测试成本低。
3. `0334...` 的 `panic 0x01`。真实 case 1 个，语义不确定，rewrite 风险高；没有更多证据前继续保留 skip。

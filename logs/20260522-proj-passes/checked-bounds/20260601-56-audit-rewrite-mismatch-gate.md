# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

使用上一轮新增的 `scripts/audit-checked-bounds.py` 审计当前 pattern 输出：

- files：90。
- checked_bounds_kinds：775。
- semantic_markers：774。
- skip_reasons：1，`unrecognized_operands: 1`。
- cfg_rewrites：774。

这说明当前非 skip 的 checked-bounds 都有 marker 和 CFG rewrite。问题是脚本只展示统计，不会在 mismatch 时失败。这样如果后面出现 metadata-only 回归，人工可能漏看。

apehex batch620-625：

- files：229。
- checked_bounds_kinds：0。
- skip_reasons：0。

没有新的真实 matcher 候选。`0334...` 的 `panic 0x01` 仍不能证明是 bounds/compiler guard，本轮不 rewrite。

## 本轮选择

给审计脚本增加 `--fail-on-mismatch`：

- 计算 `rewrite_expected = checked_bounds_kinds - skip_reasons`。
- 要求 `rewrite_expected == semantic_markers == cfg_rewrites`。
- 不满足时返回非 0。

这直接覆盖“不能 metadata-only 当作对外完成”的检查。

不做：

- 不改 CheckedBoundsPass。
- 不处理 `panic 0x01`。
- 不把 apehex 无 skip 当作完成条件。

## 修改

- `scripts/audit-checked-bounds.py`
  - 增加 `--fail-on-mismatch` 参数。
  - 输出 `rewrite_expected` 和 `rewrite_markers`。
  - mismatch 时打印错误并返回 1。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-audit-script --list-skips --fail-on-mismatch`：通过。
  - `rewrite_expected: 774`
  - `rewrite_markers: 774`
  - `cfg_rewrites: 774`
- `python3 scripts/audit-checked-bounds.py ...batch620/outputs ...batch625/outputs --fail-on-mismatch`：通过。
  - `rewrite_expected: 0`
  - `rewrite_markers: 0`
  - `cfg_rewrites: 0`
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-audit-mismatch`：90 passed, 0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-audit-mismatch`：65 passed, 0 failed。

本轮没有改 C++ pass，不需要 build / fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续用 `scripts/audit-checked-bounds.py --fail-on-mismatch` 扫后续 apehex batch。真实 case 数量最高，当前 batch620-625 没有 skip。
2. 如果后续仍无 skip，把 `--fail-on-mismatch` 接进 CTest 或批量 runner，减少人工执行遗漏。语义确定，风险低，测试成本低。
3. `0334...` 的 `panic 0x01`。真实 case 1 个，语义不确定，rewrite 风险高；没有更多证据前继续保留 skip。

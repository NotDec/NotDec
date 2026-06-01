# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 pattern suite：

- checked-bounds case：78 个。
- 剩余 skip：`0334_19494307_668d201319_1354ce2e324d` 里 1 个 `unrecognized_operands`，panic code 是 `0x01`。
- 该 helper 只是 `arg0 > arg1` 后走 `panic(0x01)`，success 继续调用另一个 helper；不能证明是 bounds/compiler guard。

当前 apehex：

- batch625：39 个 `.ll`，checked-bounds skip 0。
- batch620-625 合计 229 个 `.ll`，checked-bounds skip 0。
- 历史 batch612 当前输出也已经没有 checked-bounds skip，说明旧的 `0x11/0x12` 已被后续迭代覆盖。

问题是这些统计此前靠一次性 Python 片段，不适合作为每轮筛选依据。

## 本轮选择

增加可复用审计脚本：

- 扫描一个或多个输出目录 / `.ll` 文件。
- 汇总 checked-bounds kind、panic code、semantic marker、skip reason、CFG rewrite。
- 可列出仍有 skip 的文件。

不做：

- 不 rewrite `panic 0x01`。
- 不扩展 CheckedBoundsPass matcher。
- 不把 metadata-only 当作语义完成。

## 修改

- `scripts/audit-checked-bounds.py`
  - 新增 checked-bounds 输出审计脚本。
  - 只统计 checked-bounds 相关 semantic marker，避免把 revert/payability/storage 等 marker 混进结果。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-skip-total --list-skips`：
  - files：90。
  - checked_bounds_kinds：775。
  - semantic_markers：774。
  - skip_reasons：1，`unrecognized_operands: 1`。
  - cfg_rewrites：774。
  - skip file：`0334_19494307_668d201319_1354ce2e324d/out.ll`。
- `python3 scripts/audit-checked-bounds.py ...batch620/outputs ...batch625/outputs --list-skips`：
  - files：229。
  - checked_bounds_kinds：0。
  - skip_reasons：0。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-audit-script`：90 passed, 0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-audit-script`：65 passed, 0 failed。

本轮没有改 C++ pass，不需要 build / fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 用 `scripts/audit-checked-bounds.py` 持续扫后续 apehex batch。真实 case 数量最高，能直接发现新 skip；当前 batch620-625 没有 skip。
2. `0334...` 的 `panic 0x01`。真实 case 1 个，语义不确定，rewrite 风险高。除非找到 Solidity 源或更明确 helper 语义，否则保留 skip。
3. 若后续 apehex 仍无 skip，转向 checked-bounds oracle 质量审计，例如 marker 数和 CFG rewrite 数是否对齐，避免 metadata-only 回归。

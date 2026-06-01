# CheckedBoundsPass audit skipped kind column

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- 唯一 skip：`0334_19494307_668d201319_1354ce2e324d/out.ll`，skip reason 是 `unrecognized_operands`，skipped kind 是 `panic_assert`。
- 新增 batch632：13 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch629-632 合计 127 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择给 audit 输出增加 skipped kind：

- 真实 case 数量：直接覆盖当前唯一 skip，并能用于后续 apehex batch 扫描。
- 语义确定性：只读取同一条 skipped branch 上已有的 `!notdec.solidity.checked_bounds` metadata。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 CFG。
- 测试成本低：pattern 输出和 batch632 输出都能直接验证。

不做的事：

- 不 rewrite `panic 0x01`。
- 不扩展 matcher。
- 不修改 manifest oracle。

## 改动

- `scripts/audit-checked-bounds.py:89`
  - 新增 `skipped_kinds_by_branch`，统计带 `!notdec.solidity_checked_bounds.skipped` 的 branch 上的 checked-bounds kind。
- `scripts/audit-checked-bounds.py:113`
  - `audit_file` 返回 `skipped_kinds`。
- `scripts/audit-checked-bounds.py:127`
  - 新增 `format_counter_csv`，把 counter 写成 `kind:count` 形式。
- `scripts/audit-checked-bounds.py:157`
  - summary CSV 增加 `skipped_kinds` 列。
- `scripts/audit-checked-bounds.py:199`
  - by-path / by-file CSV 增加 `skipped_kinds` 列。
- `scripts/audit-checked-bounds.py:235`
  - `summarize_files` 汇总 `skipped_kinds`。
- `scripts/audit-checked-bounds.py:327`
  - stdout 打印 `skipped_kinds`。
- `scripts/audit-checked-bounds.py:340`
  - summary CSV 写入 `skipped_kinds`。
- `scripts/audit-checked-bounds.py:359`
  - by-path CSV 写入 `skipped_kinds`。
- `scripts/audit-checked-bounds.py:388`
  - by-file CSV 写入 `skipped_kinds`。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- pattern audit：
  - `skipped_kinds: panic_assert:1`。
  - summary CSV：`90,775,1,panic_assert:1,774,774,774,matched`。
  - 0334 by-file 行：`checked_bounds_total=53`，`skip_total=1`，`skipped_kinds=panic_assert:1`，`semantic_marker_total=52`，`cfg_rewrites=52`。
- batch632 audit：
  - 13 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
  - summary CSV 的 `skipped_kinds` 为空。
- batch629-632 audit：
  - 127 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round69-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round69-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 如果持续没有新 skip，审计 manifest 中 checked-bounds case 是否还有弱 oracle，例如缺少 `expected_checked_bounds_skipped_kinds` 的非零 skip。真实 case 中等；语义确定；rewrite 风险低；测试成本低。

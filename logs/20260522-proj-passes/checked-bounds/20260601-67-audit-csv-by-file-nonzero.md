# CheckedBoundsPass audit CSV by-file nonzero

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 最近输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- 唯一 skip：`0334_19494307_668d201319_1354ce2e324d/out.ll`，`panic_assert` / panic `0x01`，skip reason 是 `unrecognized_operands`。
- 该 skip 仍没有 Solidity codegen 或真实 IR 证据证明是 compiler bounds guard；本轮不放宽 matcher。
- apehex batch631：30 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择改进审计脚本的 per-file CSV：

- 真实 case 数量：apehex 当前输出多数是 0，per-file CSV 会产生大量无效行，影响继续找非零 case。
- 语义明确：只过滤 CSV 行，不改变 CheckedBoundsPass、不改变 IR。
- rewrite 风险：无 matcher / CFG 改动。
- 测试成本低：可直接用 pattern 输出和 batch631 输出验证。

不做的事：

- 不处理剩余 `panic 0x01` skip。
- 不把 metadata-only 作为完成。
- 不改 manifest oracle。

## 改动

- `scripts/audit-checked-bounds.py:262`
  - 在 `main` 中新增 `--csv-by-file-nonzero`。
- `scripts/audit-checked-bounds.py:347`
  - 在 `main` 的 `--csv-by-file` 写行前过滤 `checked_bounds_total == 0` 且 `skip_total == 0` 的文件。
  - 默认 `--csv-by-file` 行为不变。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- pattern 输出审计：
  - 默认 `--csv-by-file`：91 行。
  - `--csv-by-file --csv-by-file-nonzero`：60 行。
  - 剩余 skip 文件仍保留：`0334_19494307_668d201319_1354ce2e324d/out.ll`，`checked_bounds_total=53`，`skip_total=1`，`semantic_marker_total=52`，`cfg_rewrites=52`。
- apehex batch631 审计：
  - 30 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
  - `--csv-by-file-nonzero` 输出只剩 header。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round67-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round67-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对唯一 `panic 0x01` skip 做 Solidity codegen / 原始 IR 归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 如果后续 batch 仍长期全 0，考虑把多 batch 非零文件汇总脚本接入固定命令或日志模板。真实 case 间接受益；语义确定；rewrite 风险低；测试成本低。

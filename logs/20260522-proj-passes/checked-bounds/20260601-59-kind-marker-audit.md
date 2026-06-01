# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 pattern suite 重新生成输出后：

- `.ll` 文件：90。
- checked-bounds metadata：775。
- semantic marker：774。
- CFG rewrite：774。
- skip：1，仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。
- 新增逐 kind 审计后，`expected_semantic_markers` 和实际 `semantic_markers` 完全一致。

当前 apehex 最新真实输出：

- batch627：37 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch628：40 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch629：6 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

没有新的安全 rewrite case。剩余 `panic 0x01` 仍只证明 assert-shaped guard，不足以确认是 Solidity compiler bounds guard。

## 本轮选择

选择补强 audit oracle：原来的 `--fail-on-mismatch` 只检查总数：

- `checked_bounds_total - skip_total == semantic_marker_total`
- `checked_bounds_total - skip_total == cfg_rewrites`

这个检查不能发现 kind 和 marker 名错配，例如 `checked_add` 被错误打成 `checked_sub`，只要总数相同就会漏掉。

本轮只做逐 kind marker 校验，不做：

- 不改 CheckedBoundsPass matcher。
- 不 rewrite `panic 0x01`。
- 不把当前 apehex 无 checked-bounds 输出当作完成。

## 修改

- `scripts/audit-checked-bounds.py:26`
  - 新增 `KIND_TO_MARKER`，和 `src/Passes/evm/SolidityPatterns.cpp` 里的 `getCheckedBoundsRewriteMarkerName` 对齐。
- `scripts/audit-checked-bounds.py:63`
  - 新增 `expected_markers_by_kind`，从真实 IR 行上读取 `notdec.solidity.checked_bounds` 和 `notdec.solidity_checked_bounds.skipped`。
  - 对未 skip 的 kind 计算应有 semantic marker；skip 的 case 不要求 marker。
- `scripts/audit-checked-bounds.py:80`
  - `audit_file` 返回 `expected_markers` 和 `unmapped_rewrite_kinds`。
- `scripts/audit-checked-bounds.py:173`
  - 汇总并打印 `expected_semantic_markers`、`unmapped_rewrite_kinds`。
- `scripts/audit-checked-bounds.py:226`
  - `--fail-on-mismatch` 增加逐 marker 比较；如果 kind 没有映射或 marker 数错配，直接失败并打印差异。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- apehex batch627：
  - `python3 scripts/audit-checked-bounds.py ...batch627/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch627-kind.csv`
  - CSV：`37,0,0,0,0,0`
- apehex batch628：
  - `python3 scripts/audit-checked-bounds.py ...batch628/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch628-kind.csv`
  - CSV：`40,0,0,0,0,0`
- apehex batch629：
  - `python3 scripts/audit-checked-bounds.py ...batch629/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch629-kind.csv`
  - CSV：`6,0,0,0,0,0`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-kind-audit`
  - 90 passed, 0 failed。
  - audit：775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites，0 unmapped kind。
  - CSV：`90,775,1,774,774,774`
- build：
  - `cmake --build ./build --target all`
  - 通过。
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-kind-audit`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

第一次 rewrite suite 和 build 并行运行时，`notdec` 被 relink，runner 打开可执行文件时报 `Text file busy`。这是验证调度问题，不是测试失败；串行重跑后通过。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找是否重新出现 checked-bounds metadata 或 skip。真实 case 数量最高，语义判断成本低，rewrite 风险取决于新 case，测试成本低。
2. 给 pattern manifest 增加更细的 per-case kind-to-marker oracle。当前全局 audit 已能抓错配，但 per-case 失败定位还可以更直接；语义确定，rewrite 风险低，测试成本中等。
3. 审计 `KIND_TO_MARKER` 与 C++ `getCheckedBoundsRewriteMarkerName` 是否能由单一来源生成或自动比对。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留并观察 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高，除非找到 Solidity codegen 证据，否则不做 matcher。

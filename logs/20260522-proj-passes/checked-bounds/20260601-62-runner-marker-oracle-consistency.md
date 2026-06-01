# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新仍是 batch629。
- batch629 当前 38 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。

检查 manifest 自一致性时发现 3 个真实 case 的 `expected_checked_bounds_kinds` 和 `expected_checked_bounds_semantic_markers` 不一致：

- `0679_19497465_c2187cbc73_f22fac5262f8` 少了 `storage_bytes_encoding: 1` 对应 marker。
- `23523_19745765_952a215d44_d3d3b55f272b` 只检查了 `checked_add: 2` 的 marker，漏了 array bounds / sub / div / mul。
- `23545_19746035_86a085849a_4d774e840428` 同上。

对这 3 个真实输出单独跑 `scripts/audit-checked-bounds.py`，确认 IR 里 marker、kind、CFG rewrite 本身都对齐；问题是 manifest oracle 弱，不是 pass rewrite 缺失。

## 本轮选择

选择修正这 3 个 manifest marker oracle，并让 pattern runner 在运行前检查 kind/marker oracle 自一致。

选择原因：

- 真实 case 数量：3 个 manifest 弱 oracle。
- 语义明确：marker 名由 checked-bounds kind 一一派生。
- rewrite 风险低：不改 C++ pass，不改 matcher。
- 测试收益明确：以后 manifest 少写 marker 会直接失败。

不做：

- 不 rewrite `panic 0x01`。
- 不改 CheckedBoundsPass。
- 不把 apehex 当前无 checked-bounds 输出当作完成。

## 修改

- `test/evm/solidity-patterns/manifest.json:1840`
  - `0679_...` 补 `notdec_solidity_rewrite_storage_bytes_encoding: 1`。
- `test/evm/solidity-patterns/manifest.json:4197`
  - `23523_...` 补全 `array_bounds_memory: 2`、`checked_sub: 1`、`checked_div: 1`、`checked_mul: 1` 对应 marker。
- `test/evm/solidity-patterns/manifest.json:4238`
  - `23545_...` 补全同一组 marker。
- `test/run_evm_solidity_patterns_suite.py:38`
  - 新增 `CHECKED_BOUNDS_KIND_TO_MARKER`。
- `test/run_evm_solidity_patterns_suite.py:139`
  - 新增 `expected_checked_bounds_markers_from_kinds`。
- `test/run_evm_solidity_patterns_suite.py:149`
  - 新增 `validate_checked_bounds_marker_oracles`。
- `test/run_evm_solidity_patterns_suite.py:494`
  - runner 在执行 case 前先检查 manifest 自一致；不一致时直接失败。

## 验证

- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-manifest-json-check`：通过。
- manifest 自一致检查：
  - `validate_checked_bounds_marker_oracles(manifest)` 返回 0 个错误。
- apehex batch629 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch629/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch629-runner-consistency-final.csv`
  - 38 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-runner-consistency`
  - 90 passed, 0 failed。
  - audit：775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites。
  - CSV：`90,775,1,774,774,774`
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-runner-consistency`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 扩展 audit CSV，加上 `cpp_marker_mapping` 状态，方便批量趋势表里看到 audit 契约是否一致。真实 case 间接受益，语义确定，rewrite 风险低，测试成本低。
3. 把 runner 的 checked-bounds kind-to-marker 映射也和 C++ `getCheckedBoundsRewriteMarkerName` 自动比对，避免 runner 映射漂移。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

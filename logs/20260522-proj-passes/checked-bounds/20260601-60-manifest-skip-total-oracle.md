# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- pattern suite：90 个 `.ll`。
- checked-bounds metadata：775。
- semantic marker：774。
- CFG rewrite：774。
- skip：1，仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。
- apehex batch629 当前已有 20 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

manifest oracle 检查发现：

- 39 个 checked-bounds case 都有 kind / marker / CFG rewrite oracle。
- 7 个 Solidity-generated 基础 case 没有 `expected_checked_bounds_skip_reasons`。
- runner 只有看到这个字段时才会检查 `checked_bounds_skip_total`，所以这 7 个 case 没有显式保证 skip 总数为 0。

## 本轮选择

选择补齐这 7 个基础 Solidity-generated case 的空 skip oracle：

- 语义明确：这些 case 当前都已经要求 marker 和 CFG rewrite 为 1，没有 skip 才符合预期。
- 风险低：只收紧测试，不改 matcher，不改输出。
- 收益明确：所有 checked-bounds manifest case 都会检查 skip 总数。

不做：

- 不改 CheckedBoundsPass。
- 不 rewrite `panic 0x01`。
- 不调整 apehex 当前无 checked-bounds 输出的处理。

## 修改

- `test/evm/solidity-patterns/manifest.json:43`
  - `checked_bounds_arithmetic_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:80`
  - `checked_bounds_sub_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:117`
  - `checked_bounds_mul_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:154`
  - `checked_bounds_division_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:191`
  - `checked_bounds_mod_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:271`
  - `checked_bounds_calldata_array_01` 增加 `expected_checked_bounds_skip_reasons: {}`。
- `test/evm/solidity-patterns/manifest.json:308`
  - `checked_bounds_storage_array_01` 增加 `expected_checked_bounds_skip_reasons: {}`。

修改后检查：

- checked-bounds case：39。
- 缺少 skip total oracle：0。

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-manifest-json-check`：通过。
- `cmake --build ./build --target all`：`ninja: no work to do.`。
- apehex batch629 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch629/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch629-next.csv`
  - 20 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-skip-total-all`
  - 90 passed, 0 failed。
  - audit：775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites。
  - CSV：`90,775,1,774,774,774`
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-skip-total-all`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 自动比对 Python `KIND_TO_MARKER` 与 C++ `getCheckedBoundsRewriteMarkerName`，防止 audit 映射漂移。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
3. 给 runner 增加 per-case “expected marker 从 expected kind 派生”的一致性检查，避免 manifest 里 kind 和 marker oracle 自身互相矛盾。真实 case 覆盖 39 个，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

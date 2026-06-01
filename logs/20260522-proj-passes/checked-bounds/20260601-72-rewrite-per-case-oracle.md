# CheckedBoundsPass rewrite per-case oracle

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- rewrite suite 输出：65 个 `.ll`，626 个 checked-bounds metadata，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
- rewrite suite 中两个 skip：
  - `0046_19493140_f89daa079d_8aad1d5bf751/out.ll`：`checked_bounds_total=2`，`skip_total=1`，`skipped_kinds=panic_resource_error:1`，`semantic_marker_total=1`，`cfg_rewrites=1`。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`checked_bounds_total=53`，`skip_total=1`，`skipped_kinds=panic_assert:1`，`semantic_marker_total=52`，`cfg_rewrites=52`。
- `0046` 的真实 IR skip 形状：
  - `%evm.gt13 = icmp ugt i256 %evm.add12, 18446744073709551615`
  - `%evm.lt = icmp ugt i256 %evm.add, -129`
  - `%evm.or2 = or i1 %evm.lt, %evm.gt13`
  - branch 带 `!notdec.solidity.checked_bounds`、panic code `65`、skip reason `unsupported_condition`。
- batch633：27 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择给 rewrite suite 增加 per-case checked-bounds oracle：

- 真实 case 数量：覆盖 rewrite suite 当前 2 个真实 skip。
- 语义确定性：只检查真实输出里已经存在的 metadata、semantic marker 和 CFG rewrite 数量。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 IR。
- 测试成本低：rewrite suite 每轮必跑，输出也过 `llvm-as`。

不做的事：

- 不 rewrite `panic_resource_error` 或 `panic_assert`。
- 不扩展 matcher。
- 不把 aggregate audit 代替 per-case oracle。

## 改动

- `test/run_evm_solidity_rewrite_suite.py:15`
  - 新增 `CHECKED_BOUNDS_MARKER_RE`，统计 checked-bounds semantic marker。
- `test/run_evm_solidity_rewrite_suite.py:58`
  - 新增 `count_metadata_string_values`，读取 metadata 字符串值。
- `test/run_evm_solidity_rewrite_suite.py:67`
  - 新增 `count_checked_bounds_skipped_kinds`，统计 skipped branch 上的 checked-bounds kind。
- `test/run_evm_solidity_rewrite_suite.py:81`
  - 新增 `count_checked_bounds_cfg_rewrites`。
- `test/run_evm_solidity_rewrite_suite.py:176`
  - per-case checks 增加 checked-bounds total、skip total、semantic marker total、CFG rewrite、skip reason、skipped kind。
- `test/evm/solidity-rewrite/manifest.json:75`
  - 给 `0046_19493140_f89daa079d_8aad1d5bf751` 增加 checked-bounds oracle。
- `test/evm/solidity-rewrite/manifest.json:281`
  - 给 `0334_19494307_668d201319_1354ce2e324d` 增加 checked-bounds oracle。

## 验证

- `python3 -m py_compile test/run_evm_solidity_rewrite_suite.py`：通过。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round72-suite`：65 passed，0 failed，`checked_bounds_audit` passed。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round72-suite`：90 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- batch633 audit：
  - 27 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 rewrite suite 的 `panic_resource_error` / panic `0x41` skip 做 Solidity codegen 或真实 IR 归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 对 pattern/rewrite 的 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。

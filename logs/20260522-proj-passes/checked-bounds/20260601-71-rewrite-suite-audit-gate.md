# CheckedBoundsPass rewrite suite audit gate

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- pattern 唯一 skip：`0334_19494307_668d201319_1354ce2e324d/out.ll`，skip reason 是 `unrecognized_operands`，skipped kind 是 `panic_assert`。
- rewrite suite 输出：65 个 `.ll`，626 个 checked-bounds metadata，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
- rewrite suite skip：
  - `0046_19493140_f89daa079d_8aad1d5bf751/out.ll`：`unsupported_condition`，skipped kind 是 `panic_resource_error`。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands`，skipped kind 是 `panic_assert`。
- batch632：34 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择给 rewrite suite 接入 checked-bounds audit gate：

- 真实 case 数量：rewrite suite 当前有 626 个 checked-bounds metadata 和 2 个 skip，之前 runner 没把这些作为 gate。
- 语义确定性：复用已验证的 `scripts/audit-checked-bounds.py --fail-on-mismatch`，检查 marker、CFG rewrite、skip reason、skipped kind 和 C++ marker 映射。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 IR。
- 测试成本低：rewrite suite 本来就在每轮验证里跑。

不做的事：

- 不 rewrite `panic_resource_error` 或 `panic_assert`。
- 不扩展 matcher。
- 不改 rewrite manifest 计数。

## 改动

- `test/run_evm_solidity_rewrite_suite.py:152`
  - suite 跑完所有 case 后查找 `scripts/audit-checked-bounds.py`。
- `test/run_evm_solidity_rewrite_suite.py:154`
  - 调用 audit 脚本扫描当前 rewrite workdir，并传入 `--fail-on-mismatch`。
- `test/run_evm_solidity_rewrite_suite.py:164`
  - 写 `checked-bounds-audit.log`。
- `test/run_evm_solidity_rewrite_suite.py:167`
  - audit 通过时输出 `[PASS ] checked_bounds_audit`。
- `test/run_evm_solidity_rewrite_suite.py:169`
  - audit 失败时计入 suite failure。

## 验证

- `python3 -m py_compile test/run_evm_solidity_rewrite_suite.py`：通过。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round71-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round71-suite`：65 passed，0 failed，`checked_bounds_audit` passed。
- rewrite suite audit 输出：
  - files：65。
  - checked_bounds_kinds：626。
  - semantic_markers：624。
  - skip_reasons：2，`unsupported_condition:1`，`unrecognized_operands:1`。
  - skipped_kinds：2，`panic_resource_error:1`，`panic_assert:1`。
  - cfg_rewrites / rewrite_expected / rewrite_markers：624。
  - `cpp_marker_mapping: matched`。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 rewrite suite 的 `panic_resource_error` / panic `0x41` skip 做 Solidity codegen 或真实 IR 归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 对 pattern/rewrite 的 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。

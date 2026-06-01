# CheckedBoundsPass rewrite oracle completeness gate

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- rewrite suite 输出：65 个 `.ll`，626 个 checked-bounds metadata，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
- rewrite suite 两个 per-case checked-bounds oracle 已覆盖：
  - `0046_19493140_f89daa079d_8aad1d5bf751`：`panic_resource_error`，`unsupported_condition`。
  - `0334_19494307_668d201319_1354ce2e324d`：`panic_assert`，`unrecognized_operands`。
- batch633：30 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- `0046` 的 skip 是 `or` 组合资源检查，目前没有证据证明能安全 rewrite。

## 选择

本轮选择给 rewrite suite 增加 checked-bounds per-case oracle 完整性 gate：

- 真实 case 数量：覆盖当前 rewrite suite 两个 checked-bounds skip oracle。
- 语义确定性：只检查 manifest 的字段完整性和数量自洽。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 IR。
- 测试成本低：rewrite suite 启动时即可检查。

不做的事：

- 不 rewrite `panic_resource_error` 或 `panic_assert`。
- 不扩展 matcher。
- 不改 manifest 中已记录的真实数量。

## 改动

- `test/run_evm_solidity_rewrite_suite.py:108`
  - 新增 `validate_checked_bounds_oracles`。
- `test/run_evm_solidity_rewrite_suite.py:121`
  - 如果某个 case 写了任意 `checked_bounds*` oracle，要求同时写 total、skip total、semantic marker total、CFG rewrite。
- `test/run_evm_solidity_rewrite_suite.py:128`
  - 检查 semantic marker total 和 CFG rewrite 都等于 `checked_bounds_total - checked_bounds_skip_total`。
- `test/run_evm_solidity_rewrite_suite.py:141`
  - 如果 skip total 非零，要求 skip reason 总数和 skipped kind 总数都等于 skip total。
- `test/run_evm_solidity_rewrite_suite.py:185`
  - suite 启动时先运行 manifest 预检查，失败则直接返回。

## 验证

- `validate_checked_bounds_oracles(...)`：0 errors。
- `python3 -m py_compile test/run_evm_solidity_rewrite_suite.py`：通过。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round73-suite`：65 passed，0 failed，`checked_bounds_audit` passed。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round73-suite`：90 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 rewrite suite 的 `panic_resource_error` / panic `0x41` skip 做 Solidity codegen 或真实 IR 归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 对 pattern/rewrite 的 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。

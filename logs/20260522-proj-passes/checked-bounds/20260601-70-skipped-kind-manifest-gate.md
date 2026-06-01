# CheckedBoundsPass skipped kind manifest gate

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- 唯一 skip：`0334_19494307_668d201319_1354ce2e324d/out.ll`，skip reason 是 `unrecognized_operands`，skipped kind 是 `panic_assert`。
- manifest 当前只有 `0334_19494307_668d201319_1354ce2e324d` 有非零 `expected_checked_bounds_skip_reasons`，且已有 `expected_checked_bounds_skipped_kinds: {"panic_assert": 1}`。
- batch632：24 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch629-632 合计 140 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择把 manifest oracle 规则固化到 runner：

- 真实 case 数量：覆盖当前唯一 skip，并防止后续新增 skip oracle 时只写 reason、不写 skipped kind。
- 语义确定性：只比较 manifest 里 skip reason 总数和 skipped kind 总数。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 IR，不改 CFG。
- 测试成本低：pattern suite 启动时会跑 manifest 预检查。

不做的事：

- 不 rewrite `panic_assert` / panic `0x01`。
- 不扩展 matcher。
- 不改现有 manifest 数量。

## 改动

- `test/run_evm_solidity_patterns_suite.py:203`
  - 在 `validate_checked_bounds_marker_oracles` 中读取 `expected_checked_bounds_skip_reasons`。
- `test/run_evm_solidity_patterns_suite.py:205`
  - 如果 skip reason 总数非零，要求 `expected_checked_bounds_skipped_kinds` 总数相同。
- `test/run_evm_solidity_patterns_suite.py:209`
  - 总数不一致时报 manifest oracle 错误，阻止 suite 继续跑。

## 验证

- manifest 预检查：`validate_checked_bounds_marker_oracles(...)` 返回 0 个错误。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- pattern audit：
  - `skipped_kinds: panic_assert:1`。
  - `skip_reasons: unrecognized_operands:1`。
- batch632 audit：
  - 24 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch629-632 audit：
  - 140 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round70-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round70-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 如果持续没有新 skip，检查 rewrite suite 是否也需要 checked-bounds skipped-kind manifest gate。真实 case 中等；语义确定；rewrite 风险低；测试成本低。

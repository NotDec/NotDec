# CheckedBoundsPass skipped kind oracle

原始需求：

> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

## 本轮证据

- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
- 唯一 skip：`0334_19494307_668d201319_1354ce2e324d/out.ll`。
- 真实 IR 形状：
  - `private__0x2dad_0x2dad` 里 `%evm.gt.not = icmp ugt i256 %_0x2dadarg0x0, %_0x2dadarg0x1`。
  - branch 带 `!notdec.solidity.checked_bounds !1514`，`!1514 = !{!"panic_assert"}`。
  - 同一 branch 带 `!notdec.solidity_checked_bounds.panic_code !1515`，`!1515 = !{!"1"}`。
  - 同一 branch 带 `!notdec.solidity_checked_bounds.skipped !1516`，`!1516 = !{!"unrecognized_operands"}`。
- batch629-631 合计 116 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。

## 选择

本轮选择补强 runner oracle：检查 skipped checked-bounds 的 kind。

- 真实 case 数量：直接覆盖当前唯一 skip。
- 语义确定性：只读取同一条 skipped branch 上已有 metadata，确认它是 `panic_assert`，不推导业务语义。
- rewrite 风险：不改 `CheckedBoundsPass`，不改 CFG。
- 测试成本低：0334 已在 pattern suite 和 rewrite suite 中。

不做的事：

- 不把 `panic 0x01` rewrite 成 semantic helper。
- 不放宽 `unrecognized_operands` matcher。
- 不把“有 metadata”当成 rewrite 完成。

## 改动

- `test/run_evm_solidity_patterns_suite.py:234`
  - 新增 `count_checked_bounds_skipped_kinds`，统计带 `!notdec.solidity_checked_bounds.skipped` 的 branch 上的 checked-bounds kind。
- `test/run_evm_solidity_patterns_suite.py:671`
  - 接入 `expected_checked_bounds_skipped_kinds` 的 expected 侧。
- `test/run_evm_solidity_patterns_suite.py:797`
  - 接入 `expected_checked_bounds_skipped_kinds` 的 actual 侧。
- `test/evm/solidity-patterns/manifest.json:2012`
  - 给 `0334_19494307_668d201319_1354ce2e324d` 增加 `expected_checked_bounds_skipped_kinds: {"panic_assert": 1}`。

## 验证

- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `cmake --build ./build --target all`：通过，`ninja: no work to do`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round68-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round68-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。

## 剩余问题和下一轮候选

1. 继续扫 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高；语义确定性取决于新 case；rewrite 风险未知；测试成本低。
2. 对 `panic_assert` / panic `0x01` 做 Solidity codegen 或原始合约归档。真实 case 数量 1；语义确定性当前低；rewrite 风险高；测试成本中。
3. 如果后续真实 batch 仍全 0，考虑给 audit 增加 skipped-kind 统计列，方便非 runner 场景定位同类 skip。真实 case 间接受益；语义确定；rewrite 风险低；测试成本低。

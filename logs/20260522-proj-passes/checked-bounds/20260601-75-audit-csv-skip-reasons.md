# 原始 prompt

> <goal_context>
> Continue working toward the active thread goal.
> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进
> </goal_context>

# 本轮问题

当前 checked-bounds rewrite 覆盖稳定，剩余 skip 数量很少。审计 stdout 已经能同时看到 skip reason 和 skipped kind，但 CSV 只有 `skipped_kinds`，没有 `skip_reasons`。批量看 pattern、rewrite、apehex 趋势时，还要回 stdout 才能知道为什么跳过。

本轮统计：

- pattern suite `/tmp/notdec-patterns-cb-round74-suite`：90 个 `.ll`，775 个 checked-bounds，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- rewrite suite `/tmp/notdec-rewrite-cb-round74-suite`：65 个 `.ll`，626 个 checked-bounds，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
  - `0046_19493140_f89daa079d_8aad1d5bf751/out.ll`：`unsupported_condition:1`，`panic_resource_error:1`。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- apehex `20260601-evm2llvm-train-batch634/outputs`：18 个 `.ll`，checked-bounds 为 0。

# 选择

做 CSV oracle 改进：给 summary、by-path、by-file CSV 都增加 `skip_reasons` 列。

不做 matcher 改动：

- `panic_assert` 仍不能证明是 compiler guard，不消掉。
- `panic_resource_error` 复合条件只有 1 个真实 case，尚未对照 Solidity codegen 证明可安全 rewrite。

# 修改

- `scripts/audit-checked-bounds.py:157` 的 `write_csv_summary` 增加 `skip_reasons` 参数和 CSV 字段。
- `scripts/audit-checked-bounds.py:202` 的 `write_csv_rows` 增加 `skip_reasons` 字段。
- `scripts/audit-checked-bounds.py:343` 的 summary CSV 写入 `format_counter_csv(summary["skip_reasons"])`。
- `scripts/audit-checked-bounds.py:357` 的 by-path CSV 写入每个输入 path 的 skip reason。
- `scripts/audit-checked-bounds.py:383` 的 by-file CSV 写入每个 `.ll` 文件的 skip reason。

这轮没有改 `CheckedBoundsPass` matcher，也没有改 CFG rewrite 逻辑。

# 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-round74-suite --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-round75-pattern.csv --csv-by-file /tmp/notdec-cb-round75-pattern-nonzero.csv --csv-by-file-nonzero`：通过。
  - summary CSV 有 `skip_reasons=unrecognized_operands:1`。
  - 0334 per-file CSV 有 `skip_reasons=unrecognized_operands:1`，`skipped_kinds=panic_assert:1`。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-rewrite-cb-round74-suite --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-round75-rewrite.csv --csv-by-file /tmp/notdec-cb-round75-rewrite-nonzero.csv --csv-by-file-nonzero`：通过。
  - summary CSV 有 `skip_reasons=unsupported_condition:1;unrecognized_operands:1`。
  - 0046 per-file CSV 有 `skip_reasons=unsupported_condition:1`，`skipped_kinds=panic_resource_error:1`。
  - 0334 per-file CSV 有 `skip_reasons=unrecognized_operands:1`，`skipped_kinds=panic_assert:1`。
- `python3 scripts/audit-checked-bounds.py /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch634/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-round75-apehex634.csv --csv-by-file /tmp/notdec-cb-round75-apehex634-nonzero.csv --csv-by-file-nonzero`：通过，18 个 `.ll`，checked-bounds 为 0。
- `cmake --build ./build --target all`：通过，`ninja: no work to do.`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round75-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round75-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed，180.25 秒。

# 当前状态

- pattern suite：775 个 checked-bounds，774 个已 rewrite，1 个保留 skip。
- rewrite suite：626 个 checked-bounds，624 个已 rewrite，2 个保留 skip。
- semantic marker、expected marker、CFG rewrite 三者计数匹配。
- C++ marker mapping 和 Python audit mapping 匹配。

# 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续扫 apehex 新 batch 的 checked-bounds 命中。真实 case 数量可能增加；语义确定性取决于命中形状；rewrite 风险未知；测试成本低。
2. 对 `panic_resource_error` 0x41 复合条件做 Solidity codegen 对照。当前真实 case 1 个；语义确定性中等；rewrite 风险中等；测试成本中等。
3. 对 `panic_assert` 0x01 做 Solidity codegen 和业务 assert 区分。当前真实 case 1 个；语义确定性低；rewrite 风险高；测试成本中等。

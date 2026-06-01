# 原始 prompt

> 当前 CheckedBoundsPass完成得怎么样了？还有什么值得改进的地方吗
>
> /goal 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进

# 本轮问题

当前 matcher 覆盖已经比较稳定，pattern suite 和 rewrite suite 只剩少量 skipped checked-bounds。继续放宽 matcher 前，需要先能从审计输出直接看出每个 skip 文件对应的语义 kind，否则定位剩余 case 时还要回查 CSV 或 IR。

本轮统计：

- pattern suite `/tmp/notdec-patterns-cb-round73-suite`：90 个 `.ll`，775 个 checked-bounds，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- rewrite suite `/tmp/notdec-rewrite-cb-round73-suite`：65 个 `.ll`，626 个 checked-bounds，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
  - `0046_19493140_f89daa079d_8aad1d5bf751/out.ll`：`unsupported_condition:1`，`panic_resource_error:1`。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- apehex `20260601-evm2llvm-train-batch634/outputs`：7 个 `.ll`，checked-bounds 为 0。

# 选择

做审计输出改进：`--list-skips` 的文件行同时输出 skip reason 和 skipped kind。

不做 matcher 改动：

- `panic_assert` 仍是 assert 形状，不能证明是可消掉的 compiler guard。
- `panic_resource_error` 只看到一个复合条件真实 case，尚未对照 Solidity codegen 证明语义可安全重写。

# 修改

- `scripts/audit-checked-bounds.py:228` 的 `summarize_files` 返回 skipped 文件时增加 per-file skipped kind。
- `scripts/audit-checked-bounds.py:251` 读取 `skipped_kinds`，把它和 skip reason 一起保存在 `skipped_files`。
- `scripts/audit-checked-bounds.py:404` 的 `--list-skips` 输出改为：
  - `reasons=...; skipped_kinds=...`

这轮没有改 `CheckedBoundsPass` matcher，也没有改 CFG rewrite 逻辑。

# 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-round73-suite --list-skips --fail-on-mismatch ...`：通过，skip 文件输出 `skipped_kinds=panic_assert:1`。
- `python3 scripts/audit-checked-bounds.py /tmp/notdec-rewrite-cb-round73-suite --list-skips --fail-on-mismatch ...`：通过，skip 文件输出 `skipped_kinds=panic_resource_error:1` 和 `panic_assert:1`。
- `python3 scripts/audit-checked-bounds.py /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch634/outputs --list-skips --fail-on-mismatch ...`：通过，checked-bounds 为 0。
- `cmake --build ./build --target all`：通过，`ninja: no work to do.`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round74-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round74-suite`：65 passed，0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed，179.49 秒。

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

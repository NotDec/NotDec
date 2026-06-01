# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新到了 batch630。
- batch630 当前 13 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。
- audit 输出显示 `cpp_marker_mapping: matched`，但 CSV 里没有这个状态。

没有新的安全 rewrite case。`panic 0x01` 仍没有 Solidity codegen 证据证明是 bounds compiler guard。

## 本轮选择

选择扩展 `scripts/audit-checked-bounds.py --csv`，把 `cpp_marker_mapping` 状态写入 CSV。

选择原因：

- 真实 case 数量：覆盖 pattern 输出和 apehex batch 输出的批量审计。
- 语义明确：这是 audit 契约状态，不改变 matcher 或 IR 输出。
- rewrite 风险低：不改 C++ pass。
- 测试成本低：可以用现有 pattern 输出和 batch630 输出直接验证。

不做：

- 不改 CheckedBoundsPass。
- 不 rewrite `panic 0x01`。
- 不把 apehex 当前无 checked-bounds 输出当作完成。

## 修改

- `scripts/audit-checked-bounds.py:149`
  - `write_csv_summary` 增加 `cpp_marker_mapping` 参数。
- `scripts/audit-checked-bounds.py:162`
  - CSV header 增加 `cpp_marker_mapping`。
- `scripts/audit-checked-bounds.py:174`
  - CSV row 写入 `matched` 或 `mismatch`。
- `scripts/audit-checked-bounds.py:260`
  - 调用 `write_csv_summary` 时传入当前 C++/Python marker 映射状态。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- apehex batch630 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch630/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch630-csv-status.csv`
  - 13 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
  - CSV：`13,0,0,0,0,0,matched`
- pattern 输出 audit：
  - `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-runner-consistency --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-pattern-csv-status.csv`
  - 775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites。
  - CSV：`90,775,1,774,774,774,matched`
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-csv-status`
  - 90 passed, 0 failed。
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-csv-status`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 把 runner 的 checked-bounds kind-to-marker 映射也和 C++ `getCheckedBoundsRewriteMarkerName` 自动比对，避免 runner 映射漂移。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
3. 给 `audit-checked-bounds.py` 增加 CSV 多路径汇总模式，方便一次审多个 batch 并比较趋势。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新到了 batch631。
- batch631 当前 15 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。

没有新的安全 rewrite case。`panic 0x01` 仍没有 Solidity codegen 证据证明是 bounds compiler guard。

现有 `--csv-by-path` 可以按 batch 汇总，但不能在一个 batch 内定位具体 `.ll`。后续如果 apehex batch 里出现 checked-bounds metadata 或 skip，需要直接知道是哪一个输出文件。

## 本轮选择

选择给 `scripts/audit-checked-bounds.py` 增加 `--csv-by-file`：

- 每个 `.ll` 输出一行。
- 字段和 `--csv-by-path` 对齐。
- 保留 `--csv` 和 `--csv-by-path` 旧行为。

不做：

- 不改 CheckedBoundsPass。
- 不 rewrite `panic 0x01`。
- 不改变 pattern runner 的集成 audit gate。

## 修改

- `scripts/audit-checked-bounds.py:258`
  - 新增命令行参数 `--csv-by-file`。
- `scripts/audit-checked-bounds.py:338`
  - 新增 `--csv-by-file` 输出逻辑，对收集到的每个 `.ll` 单独调用 `summarize_files([file])` 并写 CSV。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- batch631 file CSV：
  - `python3 scripts/audit-checked-bounds.py ...batch631/outputs --fail-on-mismatch --csv /tmp/notdec-cb-batch631-file-total.csv --csv-by-file /tmp/notdec-cb-batch631-by-file.csv`
  - 合计 CSV：`15,0,0,0,0,0,matched`
  - 每个文件行都是 `checked_bounds_total=0, skip_total=0, semantic_marker_total=0, cfg_rewrites=0, rewrite_expected=0, cpp_marker_mapping=matched`。
- pattern 输出 file CSV：
  - `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-file-csv --list-skips --fail-on-mismatch --csv-by-file /tmp/notdec-cb-pattern-file-csv-final.csv`
  - 90 行。
  - 59 行有 checked-bounds。
  - 唯一 skip 行：`0334_19494307_668d201319_1354ce2e324d/out.ll`，`checked_bounds_total=53, skip_total=1, semantic_marker_total=52, cfg_rewrites=52, rewrite_expected=52, cpp_marker_mapping=matched`。
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-file-csv`
  - 90 passed, 0 failed。
  - 集成 audit 通过。
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-file-csv`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 减少 audit 脚本和 runner 里 kind-to-marker 映射重复，例如让两边都解析 C++ 映射再做派生。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
3. 给 `audit-checked-bounds.py --csv-by-file` 增加按 `checked_bounds_total` / `skip_total` 过滤输出的选项，减少大 batch CSV 噪音。真实 case 间接受益，语义确定，rewrite 风险低，测试成本低。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

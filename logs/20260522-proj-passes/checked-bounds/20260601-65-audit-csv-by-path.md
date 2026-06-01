# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新仍是 batch630。
- batch629 当前 38 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch630 当前 38 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。

没有新的安全 rewrite case。`panic 0x01` 仍没有 Solidity codegen 证据证明是 bounds compiler guard。

现有 `--csv` 只能把多个路径合并成一行。后续要持续看多个 apehex batch 时，还需要每个输入路径一行的趋势表。

## 本轮选择

选择给 `scripts/audit-checked-bounds.py` 增加 `--csv-by-path`：

- 保留 `--csv` 旧语义：所有输入合并成一行。
- 新增 `--csv-by-path`：每个输入路径单独一行。

不做：

- 不改 CheckedBoundsPass。
- 不 rewrite `panic 0x01`。
- 不改变 pattern runner 的集成 audit gate。

## 修改

- `scripts/audit-checked-bounds.py:179`
  - 新增 `write_csv_rows`，写多行 CSV。
- `scripts/audit-checked-bounds.py:199`
  - 新增 `collect_files`，统一处理目录和单个 `.ll`。
- `scripts/audit-checked-bounds.py:207`
  - 新增 `summarize_files`，复用单路径、多路径汇总逻辑。
- `scripts/audit-checked-bounds.py:254`
  - 新增命令行参数 `--csv-by-path`。
- `scripts/audit-checked-bounds.py:279`
  - 保留每个原始输入路径及其收集到的 `.ll`。
- `scripts/audit-checked-bounds.py:314`
  - `--csv-by-path` 输出每个输入路径一行。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- batch629 + batch630 多路径 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch629/outputs ...batch630/outputs --fail-on-mismatch --csv /tmp/notdec-cb-batches-total.csv --csv-by-path /tmp/notdec-cb-batches-by-path.csv`
  - 合计 CSV：`76,0,0,0,0,0,matched`
  - 分路径 CSV：
    - batch629：`38,0,0,0,0,0,matched`
    - batch630：`38,0,0,0,0,0,matched`
- pattern 输出 audit：
  - `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-runner-cpp-map --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-pattern-multipath-total.csv --csv-by-path /tmp/notdec-cb-pattern-multipath-by-path.csv`
  - 合计 CSV：`90,775,1,774,774,774,matched`
  - 分路径 CSV：`90,775,1,774,774,774,matched`
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-multipath-csv`
  - 90 passed, 0 failed。
  - 集成 audit 通过。
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-multipath-csv`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 减少 audit 脚本和 runner 里 kind-to-marker 映射重复，例如让两边都解析 C++ 映射再做派生。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
3. 给 `audit-checked-bounds.py` 增加可选的 per-file CSV，定位某个 batch 内具体文件的 checked-bounds 分布。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

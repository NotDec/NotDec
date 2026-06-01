# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新仍是 batch630。
- batch630 当前 25 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。

上一轮 runner 已经用 `CHECKED_BOUNDS_KIND_TO_MARKER` 检查 manifest 的 kind/marker oracle 自一致。但这个 runner 映射是第三份手写映射，除了 C++ pass 和 audit 脚本之外，还可能单独漂移。

当前直接比对结果：

- runner 映射：19 对。
- C++ `getCheckedBoundsRewriteMarkerName` 映射：19 对。
- 当前一致。

没有新的安全 rewrite case。`panic 0x01` 仍没有 Solidity codegen 证据证明是 bounds compiler guard。

## 本轮选择

选择给 pattern runner 增加 runner/C++ marker 映射一致性检查。

选择原因：

- 真实 case 数量：覆盖全部 39 个 checked-bounds manifest case 的 oracle 派生逻辑。
- 语义明确：runner 只是测试 oracle，必须和 C++ pass 的 marker contract 一致。
- rewrite 风险低：不改 CheckedBoundsPass，不改 IR 输出。
- 测试收益明确：以后 C++ 新增 kind 或改 marker 名，runner 会在 suite 开头失败。

不做：

- 不改 matcher。
- 不 rewrite `panic 0x01`。
- 不把 apehex 当前无 checked-bounds 输出当作完成。

## 修改

- `test/run_evm_solidity_patterns_suite.py:36`
  - 新增 `CPP_CHECKED_BOUNDS_MARKER_FUNCTION_RE`。
- `test/run_evm_solidity_patterns_suite.py:40`
  - 新增 `CPP_CHECKED_BOUNDS_MARKER_PAIR_RE`。
- `test/run_evm_solidity_patterns_suite.py:156`
  - 新增 `load_cpp_checked_bounds_marker_mapping`，从 `src/Passes/evm/SolidityPatterns.cpp` 解析 C++ marker 映射。
- `test/run_evm_solidity_patterns_suite.py:165`
  - 新增 `format_checked_bounds_marker_mapping_errors`，输出 runner/C++ 映射差异。
- `test/run_evm_solidity_patterns_suite.py:180`
  - `validate_checked_bounds_marker_oracles` 增加 `project_root` 参数，并先检查 runner 映射和 C++ 映射是否一致。
- `test/run_evm_solidity_patterns_suite.py:535`
  - 调用 manifest oracle 检查时传入 `project_root`。

## 验证

- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- 直接 validator：
  - `validate_checked_bounds_marker_oracles(manifest, Path('.').resolve())` 返回 0 个错误。
  - C++ 映射 19 对，runner 映射 19 对。
- apehex batch630 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch630/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch630-runner-map.csv`
  - 25 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
  - CSV：`25,0,0,0,0,0,matched`
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-runner-cpp-map`
  - 90 passed, 0 failed。
  - audit：775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites。
  - CSV：`90,775,1,774,774,774,matched`
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-runner-cpp-map`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 给 `audit-checked-bounds.py` 增加多路径 CSV 汇总模式，方便一次审多个 batch 并比较趋势。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
3. 减少 audit 脚本和 runner 里 kind-to-marker 映射重复，例如让两边都解析 C++ 映射再做派生。真实 case 间接受益，语义确定，rewrite 风险低，测试成本中等。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

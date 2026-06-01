# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前工作树里只有既有的 `external/NotDec-bin2llvm` 本地改动，未触碰。

重新取证：

- apehex 最新仍是 batch629。
- batch629 当前已有 32 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- pattern suite 输出：90 个 `.ll`，775 个 checked-bounds metadata，1 个 skip，774 个 semantic marker，774 个 CFG rewrite。
- 剩余 skip 仍是 `0334_19494307_668d201319_1354ce2e324d/out.ll` 的 `unrecognized_operands:1`。

审计脚本里有一份 `KIND_TO_MARKER`，C++ pass 里 `getCheckedBoundsRewriteMarkerName` 也维护同样映射。上一轮已经用 Python 映射检查 kind 和 marker 是否匹配，但还不能发现 Python 映射和 C++ 映射本身发生漂移。

## 本轮选择

选择补强 audit：在 `--fail-on-mismatch` 下自动解析 C++ `getCheckedBoundsRewriteMarkerName`，和 Python `KIND_TO_MARKER` 比对。

选择原因：

- 语义明确：这是 audit 映射和 pass 输出契约的一致性检查。
- 风险低：不改 CheckedBoundsPass matcher，不改 IR 输出。
- 收益明确：以后 C++ 新增 kind 或改 marker 名，Python audit 不会静默过期。

不做：

- 不 rewrite `panic 0x01`。
- 不改 manifest。
- 不把 apehex 当前无 checked-bounds 输出当作完成。

## 修改

- `scripts/audit-checked-bounds.py:13`
  - 新增 `CPP_MARKER_FUNCTION_RE`，定位 `getCheckedBoundsRewriteMarkerName` 函数体。
- `scripts/audit-checked-bounds.py:17`
  - 新增 `CPP_MARKER_PAIR_RE`，解析 `if (Kind == "...") return "...";`。
- `scripts/audit-checked-bounds.py:114`
  - 新增 `load_cpp_marker_mapping`，从 `src/Passes/evm/SolidityPatterns.cpp` 读取 C++ marker 映射。
- `scripts/audit-checked-bounds.py:126`
  - 新增 `print_marker_mapping_mismatch`，失败时打印 Python/C++ 差异。
- `scripts/audit-checked-bounds.py:241`
  - audit 输出 `cpp_marker_mapping: matched|mismatch`。
- `scripts/audit-checked-bounds.py:266`
  - `--fail-on-mismatch` 增加 C++/Python marker 映射一致性 gate。

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- apehex batch629 audit：
  - `python3 scripts/audit-checked-bounds.py ...batch629/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-batch629-cpp-map.csv`
  - 32 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
  - `cpp_marker_mapping: matched`。
  - CSV：`32,0,0,0,0,0`
- pattern 输出 audit：
  - `python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-skip-total-all --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-pattern-cpp-map.csv`
  - 775 metadata，1 skip，774 expected markers，774 actual markers，774 CFG rewrites。
  - `cpp_marker_mapping: matched`。
  - CSV：`90,775,1,774,774,774`
- build：
  - `cmake --build ./build --target all`
  - `ninja: no work to do.`
- pattern suite：
  - `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-cpp-map`
  - 90 passed, 0 failed。
  - 集成 audit 通过，输出 `cpp_marker_mapping: matched`。
- rewrite suite：
  - `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-cpp-map`
  - 65 passed, 0 failed。
- CTest：
  - `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`
  - 2/2 passed。

本轮没有改 C++ pass，不需要 fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续审计 apehex 后续 batch，找新的 checked-bounds metadata 或 skip。真实 case 数量最高，语义确定性取决于新 case，rewrite 风险未知，测试成本低。
2. 给 runner 增加 per-case “expected marker 从 expected kind 派生”的一致性检查，避免 manifest 里 kind 和 marker oracle 自身互相矛盾。真实 case 覆盖 39 个，语义确定，rewrite 风险低，测试成本中等。
3. 扩展 audit CSV，加上 `cpp_marker_mapping` 状态，方便批量趋势表里看到 audit 契约是否一致。真实 case 间接受益，语义确定，rewrite 风险低，测试成本低。
4. 继续保留 `0334...` 的 `panic 0x01` / `unrecognized_operands`。真实 case 1 个，语义不确定，rewrite 风险高；没有 Solidity codegen 证据前不做 matcher。

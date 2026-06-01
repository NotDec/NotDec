# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 pattern suite：

- checked-bounds metadata：775。
- skip：1，仍是 `0334_19494307_668d201319_1354ce2e324d` 的 `panic 0x01`。
- semantic marker：774。
- CFG rewrite：774。

当前 apehex：

- batch626 已生成 `summary.csv`。
- batch626 输出 39 个 `.ll`，checked-bounds metadata / skip / marker / CFG rewrite 都是 0。
- batch620-625 之前也都是 0。

没有新的可安全 rewrite case。`panic 0x01` 仍不能证明是 bounds/compiler guard。

## 本轮选择

给 `scripts/audit-checked-bounds.py` 增加一行 CSV 汇总输出：

- 方便把后续多个 batch 的审计结果落盘。
- 字段只保留筛选 checked-bounds 候选需要的核心数值。

不做：

- 不改 CheckedBoundsPass matcher。
- 不 rewrite `panic 0x01`。
- 不把 apehex 当前无 skip 当作完成。

## 修改

- `scripts/audit-checked-bounds.py`
  - 新增 `--csv <path>` 参数。
  - 输出字段：
    - `files`
    - `checked_bounds_total`
    - `skip_total`
    - `semantic_marker_total`
    - `cfg_rewrites`
    - `rewrite_expected`

## 验证

- `python3 -m py_compile scripts/audit-checked-bounds.py`：通过。
- pattern CSV：
  - 命令：`python3 scripts/audit-checked-bounds.py /tmp/notdec-patterns-cb-audit-integrated --fail-on-mismatch --csv /tmp/notdec-cb-pattern-audit.csv`
  - CSV：`90,775,1,774,774,774`
- batch626 CSV：
  - 命令：`python3 scripts/audit-checked-bounds.py /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch626/outputs --fail-on-mismatch --csv /tmp/notdec-cb-batch626-audit.csv`
  - CSV：`39,0,0,0,0,0`
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-audit-csv`：90 passed, 0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-audit-csv`：65 passed, 0 failed。

本轮没有改 C++ pass，不需要 build / fortune 性能对比。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 用 `--csv` 对后续 apehex batch 持续落盘，找 checked-bounds skip 是否重新出现。真实 case 数量最高，风险低。
2. 若长期无 skip，审计 marker 类型和 kind 类型是否一一对应，防止 marker 名错误但总数对齐。语义明确，风险低。
3. `0334...` 的 `panic 0x01`。真实 case 1 个，语义不确定，rewrite 风险高；继续保留 skip，除非找到更多证据。

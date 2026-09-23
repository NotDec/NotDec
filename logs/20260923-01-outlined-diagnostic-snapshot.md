# 20260923-01 outlined 口径复测与重锚：为 pre-inline 形状建独立预算

结论：P1/P2（`fabd348` + `0f3c38f`）之后复测全语料，**outlined（`NOTDEC_EVM_INLINE=0`）的 goto 洪峰同样消失**
（goto 424→140、real jump 343→100、dangling 69→19、condition TODO 729→558），并且它**不能**复用
扁平口径的 `compile-budget.json`（helper 覆盖 floor 是扁平值 2408，outlined 只有 1394）。
因此新增 `test/evm/solidity-patterns/compile-budget-outlined.json` 与
`notdec.evm.solidity_pattern_compile_outlined`（`ctest` 里 119s 通过），把 outlined 从
"只记在日志里的数字" 变成有门禁的棘轮。

## 1. 复测方法与可复现性

```
BIN=/sn640/NotDec/build-notdec-nothreads/bin/notdec   # sha256 前 16 位 06b0a271e408dcfb
python3 test/run_evm_solidity_pattern_compile_suite.py \
  --binary "$BIN" --cases test/evm/solidity-patterns/cases \
  --budget test/evm/solidity-patterns/compile-budget.json \
  --project-root /sn640/NotDec --workdir <scratch> --solc /sn640/EthIR/source/solcv8.25
# outlined 口径加 NOTDEC_EVM_INLINE=0
```

**扁平口径先做了对照复跑**，逐项等于 `51dc15a7` 交接文档里的数字
（goto 1787 / real 1695 / dangling 58 / cond_todo 4078 / unresolved 297 / solc 0 失败，
helper 1313 / 2442），说明这台机器上的基线与交接文档一致、可比。

## 2. outlined 指标

| 指标 | 会话前（20260920-02 §11.11） | 现在 | 预算 |
| --- | --- | --- | --- |
| compile_failures | 0 | **0** | 0 |
| unresolved_value | 264 | 267 | 267（+3，理由同扁平） |
| condition_todo | 729 | **558** | 558 |
| goto | 424 | **140** | 140 |
| real_jump | 343 | **100** | 100 |
| dangling_goto | 69 | 19 | 19 |
| helper_functions（floor） | 1339 | 1383 | 1383 |
| helper_call_sites（floor） | 1301 | 1394 | 1394 |

- unresolved 从 264 涨到 267 与扁平口径 289→297 同因：入口节点不再被丢，入口块里的
  calldata 边界检查语句重新渲染出来（其中少量值仍未恢复）。
- helper 覆盖两项都上升，说明现在渲染出来的 helper 反而更多（此前有被丢弃的）。

## 3. 为什么必须独立预算文件

`compile-budget.json` 的 floor 在 2026-09-22 已经按扁平口径重锚（2408），
outlined 的真实值是 1394，共用预算时 floor 必然误报（本次实测确实 FAIL）。
两个形状的 floor 语义不同：扁平把共享 helper 复制到每个调用点，outlined 保留
"一层调用"。所以拆成两份预算文件，各自棘轮。

## 4. 本会话改动

- 新增 `test/evm/solidity-patterns/compile-budget-outlined.json`。
- `test/CMakeLists.txt`：新增 `add_evm_solidity_pattern_compile_outlined_suite`
  （`${CMAKE_COMMAND} -E env NOTDEC_EVM_INLINE=0` 包一层，与 `notdec.evm.solidity_patterns_outlined`
  同法），注册 `notdec.evm.solidity_pattern_compile_outlined`。

这样结构器/内联两条线的改动都会同时被扁平与 outlined 两个棘轮约束，
不会再出现 "outlined 基线看起来干净" 的幸存者偏差（root cause 见 20260922-01）。

# 20260923-01 pre-inline（outlined）形状的诊断快照

> **【2026-09-23 更正】** 本文原来的结论是“outlined 应当有自己的预算棘轮”（标题里的“重锚”）。
> 该结论是错的：pre-inline 不是产品形状，而且已知会把不同入口的 calldata 类型合在一起
> （模块级 calldata 记录并集，见 `20260920-01` §3）；当前二进制实测 record==annotated
> 1858/2316 vs 内联 2264/2316（`agg_abi_params.py`）。`NOTDEC_EVM_INLINE` 开关、
> `compile-budget-outlined.json` 和 `notdec.evm.solidity_pattern_compile_outlined` 都已在
> `20260923-04-remove-evm-inline-switch.md` 删除。本文只保留**诊断数据**：把“结构器质量”
> 和“内联 scope”分开看这个视角，是当初发现“outlined 结构化得干净”属于幸存者偏差的关键。

## 1. 当初为什么要测这个形状

会话中期测过 pre-inline 口径 goto 424→233、cond_todo 729→658；P1/P2（`fabd348` + `0f3c38f`）
之后需要判断结构器修复是否只在扁平形状上有效。这是诊断问题，不是产品口径问题。

## 2. 测量方法（历史命令，开关已删除，无法再复现 pre-inline 形状）

```
# 当时的命令：给下面的 runner 加 NOTDEC_EVM_INLINE=0
python3 test/run_evm_solidity_pattern_compile_suite.py \
  --binary build-notdec-nothreads/bin/notdec \
  --cases test/evm/solidity-patterns/cases \
  --budget test/evm/solidity-patterns/compile-budget.json \
  --project-root /sn640/NotDec --workdir <scratch> --solc /sn640/EthIR/source/solcv8.25
```

扁平口径先做了对照复跑，逐项等于 `51dc15a7` 交接文档的数字（goto 1787 / real 1695 /
dangling 58 / cond_todo 4078 / unresolved 297 / solc 0 失败，helper 1313 / 2442），
说明基线与交接文档一致、可比。

## 3. pre-inline 指标（历史快照）

| 指标 | 会话前（20260920-02 §11.11） | 2026-09-23 快照 |
| --- | --- | --- |
| compile_failures | 0 | 0 |
| unresolved_value | 264 | 267 |
| condition_todo | 729 | 558 |
| goto | 424 | 140 |
| real_jump | 343 | 100 |
| dangling_goto | 69 | 19 |
| helper_functions | 1339 | 1383 |
| helper_call_sites | 1301 | 1394 |

读法（诊断价值）：结构器修复在**没有内联**的函数体上同样把 goto 洪峰压掉了（424→140），
说明 P1/P2 修的是结构器本身，不是“内联暴露出来的形状问题”。

unresolved 264→267 与扁平口径 289→297 同因：入口节点不再被丢，入口块里的 calldata 边界检查
语句重新渲染出来（其中少量值仍未恢复）。

## 4. 为什么这个形状不能当 CI 口径

| 形状 | record==annotated | too_few |
| --- | --- | --- |
| 内联（产品） | 2264 / 2316 (97.8%) | 2 |
| pre-inline | 1858 / 2316 (80.2%) | 308 |

原因见 `20260920-01` §3：共享 outline 解码 helper 的调用约束把各调用点的常量偏移汇进同一个
形参，calldata 记录退化成模块级并集。给它加 goto/TODO 棘轮会把优化压力引到一个不出、且
ABI 参数恢复更差的形状上。真正要解的是 per-call-site 实例化的收敛性（`20260923-04` §1）。

# tmux simplify 共享查重优化与 PNDiff 耗时误判纠正

## 用户原始 prompt

> 背景：项目 NotDec……任务起点：解决 lighttpd 内存爆炸；后扩展为 Bench2 大项目（tmux 39MB/redis 55MB）的 scalability。评估链路 `--tr-level=2 --merge-struct-ptr-load-store --gen-work-dir --fast-work-dir --merge-eval-dir --frozen-tr-input-ir`，8 线程；类型推理结果看 HType 文件。
> 下一步建议第 1 条：PNDiff 性能/死循环排查（当前焦点）：perf 采样 PNDiff 热点……确认是 O(n²) 还是死循环。
> 继续看看PNDiff为什么耗时这么久

## 背景

tmux（17 万 root 源码级 IR）此前 62GB OOM。hash-consing（40979be）+ per-group（8d29998）修复后内存可控，但 simplify 阶段单 group 展开 13+ 分钟不返回；用户观察到约束生成后"PNDiff solve 单核跑 12+ 分钟"。

## 关键结论（过程纠偏）

1. **PNDiff 不是瓶颈**：`PNDiff.warn.txt` 是 MLsubGenerator（MLsubGenerator.cpp:79）apply override 时写的时间戳，不是 solve 完成标记。perf 实测单核 13+ 分钟的热点是 ConstraintsGenerator 约束生成+实时求解（`resolve_variable` 19%、`addUpperBound` 16%、`constrain_impl`/`constrain_worklist_only` 16%、`collectMaxDirectFieldAccessSizeBytes` 9.5%），PNDiff 相关只占 ~4%（`unifyPNDiffValueGroups` 2%、`getPNIValue` 2.4%）。
2. **simplify 单 group 卡死根因**（perf 95%）：CoalesceMemo 桶扫描 `*Entry.Ty == *Ty` 全量结构比较在全局互斥锁内；`operator==` 用两次 `operator<`（每次 tie 先全量走 var set 再 record map 逐字段递归比较），每次共享命中 = O(整棵子树) 验证，巨型 SCC 命中常态 → 二次方，且 8 线程被锁串行化。
3. **小组扫描慢**：`CompactType::check`（每次 arena make 全量递归验证）37% + occurrence 交集双向子集预检（`occurrence_var_list_is_subset`）62% → 17 万组预计 13 小时。

## 实现（binarysub，7 个 commit）

| commit | 内容 | 涉及位置 |
|---|---|---|
| 389b4a2 | **PNDiff merge 重入过滤**：`mergePNINodes` 仅 `PNTy::merge` 返回 Updated 时 `markChanged`；unify 分支不再 push Merged | PNDiff.cpp:566-580、315-327、433-444 |
| f6827a0 | **CoalesceMemo 移锁**：lookup/store 锁内只做 O(bucket) 浅拷贝快照，O(subtree) 验证锁外；store 竞态用 arena hash-cons"同构同指针"做 O(1) 指针查重；**share_mu 默认开**；**operator== 单遍**（binarysub.h:606，逐字段 == 替代 2×`operator<`） | binarysub.cpp:4283-4360、binarysub.h:606-615 |
| 8b30d82 | **交集单遍**：子集判定与交集收集合并为一遍双指针遍历，删 `occurrence_var_list_is_subset` | binarysub.cpp:3564-3616 |
| b123d22 | **check 开关**：`NOTDEC_CHECK_COMPACT=1` 才执行，默认关 | binarysub.h:697-707 |
| fc8f33f | **μ 确定性命名默认开**（`NOTDEC_MU_DETERMINISTIC=0` 回退） | binarysub.cpp:4421-4437 |
| 9b7c246 | **simplify 展开诊断**：`[simplify-expanding]`（每 20 万 go 调用）、`[simplify-slow-go]`（单次 >300ms，最多 20 次） | binarysub.cpp:4258-4270、4370-4390、4578-4600 |
| 77c229a | 效率提升.md 追加 tmux-scale 一节 | doc/效率提升.md |

## 验证

- fortune/memcached：eval 指标与历史一致（fortune bad_unions=0 frag=8 cov 2.85%；memcached bad_unions=1 frag=133 cov 1.77%），ValueTypes diff 与既有 run-to-run 并行共享噪声同级
- tmux 8 线程实测：simplify 从 13 小时级 → ~1 小时级（check 开关后 200→550-1600 组/分钟）；RSS 稳定 ~3.2GB；原 920 万 go 调用的大组 ~10 分钟跑完不再卡死；PNDiff 优化对 ~13 分钟约束生成阶段无感（PNDiff 只占 4%）

## 未解决 / 下一步

- 约束生成阶段 ~13 分钟单核（resolve_variable/addUpperBound/constrain_impl 系列）：下一个优化目标，需先 perf 定位生成-求解循环
- tmux 全量最终确认（tmux-final 跑批进行中，simplify 阶段）

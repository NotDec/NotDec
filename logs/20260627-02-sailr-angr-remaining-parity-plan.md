# 原始 prompt

> logs/20260627-01-sailr-angr-p0-copied-prefix-tail.md 这里不是分析了一部分，难道是里面分析的还没修完，还是说之前有没发现完全的？可能得写一个计划继续完整修复所有差距

# 和 20260627-01 的关系

`logs/20260627-01-sailr-angr-p0-copied-prefix-tail.md` 只完成了 P0 里的一小块：
`DuplicationReverter` 的 shared-tail 候选允许已经 materialized 的 copied region
prefix 参与合并。

它没有表示 P0 已完成，也没有表示 SAILR 已经和 Angr 完全对齐。现在的情况是：

- 已知的大差距还在，尤其是完整 merge graph、一般 return region、真正 lowered
  switch 识别。
- 也可能还有没暴露出来的细差距，需要继续对照 Angr 源码和迁移测试来发现。
- 之前的实现记录是“补了一条确定安全的边界”，不是“完整修复所有差距”。

# 背景

当前 llvm2c 的 SAILR 已经有完整框架：SAILR structurer、deoptimization pipeline、
trial / rollback / quality guard，以及 `SwitchDefaultCaseDuplicator`、
`DuplicationReverter`、`SwitchReusedEntryRewriter`、`LoweredSwitchSimplifier`、
`ReturnDuplicatorLow`、`CrossJumpReverter`。

这说明现在不是“有没有 SAILR”的问题，而是每个 pass 还只覆盖了 Angr 语义的一部分。
后续要做的不是继续堆零散 case，而是把差距按影响排序补完，并给每个保守差异留
明确记录。

# 目标

目标是把当前实现从“接近 Angr 的框架和常见子集”推进到“已知 Angr SAILR 语义都
有对应实现或明确说明为什么不做”。

完成后应该能做到：

1. 常见重复 region、shared tail、return tail、switch default / reused-entry 的输出
   不再明显比 Angr 差。
2. copied / virtual block 的 body、变量来源、edge、switch case/default target 都在
   shared CFG 里表达，不靠 C 或 Solidity renderer 猜。
3. 每个 Angr SAILR 相关 pass 都有 NotDec 对应状态：完成、刻意保守、暂不适用、
   或还没做。
4. 新发现的差距先归类，再决定是否实现，不再混进已有实现日志里。

# 当前已完成的部分

这些能力已经有基础，不需要重做：

- SAILR pass pipeline 和 shared optimization wrapper。
- pass 失败后的 rollback 和基本 quality guard。
- copied / synthetic block 的独立 `BlockId` 和来源身份。
- `CrossJumpReverter` 的 shared CFG 子集。
- `ReturnDuplicatorLow` 的线性 return tail、部分 branch / diamond / grouped
  predecessor 复制。
- `DuplicationReverter` 的 exact merge、common statement tail、linear region tail、
  copied-prefix shared tail。
- 已有 switch 的部分 default 复用和 case entry 复用处理。
- angr dephication 模式的最小入口、shared Phi edge payload、部分 vvar / copied
  payload 回归。

这些是后续继续补的基础，不等于完整对齐。

# 剩余差距和处理顺序

## P0：先建立 Angr 对照清单（已完成 2026-06-27）

先把“到底差什么”列清楚。否则容易继续修一个小 case，就误以为对应 pass 完成。

要做：

- 对照 Angr 当前 SAILR / optimization pass 源码，列出每个类和关键方法。
- 给 NotDec 现有实现标状态：已覆盖、部分覆盖、没覆盖、NotDec 暂不适用。
- 把 Angr SAILR 相关测试分成三类：可直接迁移、需要 IR/payload 代理、不适用。
- 每次实现后更新这个清单，避免日志分散后看不出总进度。

判断标准：

- 有一张 pass 级别的差距表。
- `20260627-01` 这种小实现会被标成某个 pass 的子项完成，而不是 pass 完成。
- 后续发现的新差距有入口记录。

完成记录见文末“2026-06-27 P0 实现记录”。P0 只建立清单，不表示后面的 P1-P7
已经完成。

## P1：补 `DuplicationReverter` 的完整 merge graph 语义

这是对输出影响最大的差距。当前 NotDec 还是保守 tail merge；Angr 会找相似子图，
拆出公共部分，再把图重接回去。

要做：

- 候选仍先围绕 goto 和已有 quality guard，不做全图激进搜索。
- 支持小型 single-entry / single-exit DAG region，不先碰循环 region。
- 用 payload source identity、normalized statement、读写副作用信息做相似判断。
- 支持 prefix / common / suffix 拆分，不只合并完全相同尾部。
- 构造 shared merge graph，把公共主体变成显式 shared block 或 region。
- 边、case target、default target、dephication context 都按 shared CFG 身份重写。
- 失败时必须完整回滚。

暂时不做：

- 不做很宽的表达式等价证明。
- 不移动有副作用或依赖不清楚的 statement。
- 不为了减少 goto 破坏 payload 来源。

判断标准：

- 能合并“前缀不同、中间或尾部相同、后缀可重接”的两个相似 region。
- 至少有一个非 exact-match、非纯 tail 的 shared merge 测试。
- 不能合并的相似 region 有明确 skip 原因。
- 合并后 goto 数或结构质量改善，且语义不靠 renderer 特判。

## P2：补 `ReturnDuplicatorLow` 的一般 return region

这是第二大差距。现在已经覆盖不少 return tail，但还不是 Angr 那种从 end node 往前找
single-entry return region 的完整做法。

要做：

- 从所有 end node 反推 return region，而不是只从当前可识别 tail 形状出发。
- 支持更一般的 branch / switch / 多 block return region。
- 按 connected predecessor component 分组复制，避免复制过多。
- 复制时处理 copied payload、Phi incoming、vvar 映射和 dephication context。
- 删除原 region 时同步更新剩余 incoming 和 edge 元数据。
- 保留 block / statement / call 成本限制。

判断标准：

- 一般 single-entry return region 能被复制，不只线性或 diamond。
- 同一个 return region 被不同 predecessor 复制后，payload 可以有不同值来源。
- Phi / vvar 不需要 renderer 猜。
- 失败和部分复制都能回滚。

## P3：补 shared Phi / vvar / copied payload 的全量消费

这不是单独为了好看，而是 P1、P2 继续扩张的前提。没有这层，复制 region 很容易
结构看起来对，但值来源错。

要做：

- 收口 legacy demote 和 angr dephication 两条路线的边界，默认先不急着切换。
- 把 copied block 的 payload materialize 统一到 shared 层。
- copied vvar、`vvar_to_vvar`、Phi incoming 删除/重定向都能被 pass 使用。
- C 和 Solidity 消费同一份 shared 结果。

判断标准：

- copied return region 和 copied switch region 都有 predecessor-sensitive payload。
- shared CFG 能解释每个 copied value 的来源。
- legacy / angr 模式差异可解释。
- 默认切换前有明确对照测试。

## P4：实现真正的 `LoweredSwitchSimplifier`

当前这个 pass 主要处理已有 switch case target 复用，还没有从 if-chain 恢复 switch。

要做：

- 先支持 `x == const` / `x != const` 链。
- 再支持简单范围比较形成的 case/default 拆分。
- 识别同一个 switch variable，收集 case value 和 default target。
- 在 shared CFG 里生成 switch 语义，而不是 renderer 侧把 if-chain 打印成 switch。
- 和 default / reused-entry pass 共用 case/default 表示。

判断标准：

- lowered if-chain 能恢复成 switch。
- 普通 if-else 不被误识别。
- case value、default target、fallthrough 都稳定。
- 后续 case 复制仍走 shared CFG。

## P5：补 switch default / reused-entry 的剩余语义

这部分影响比 P1/P2/P4 小，但复杂 switch 会明显受影响。

要做：

- default 复用不只看 `Switch` terminator，还要结合 switch-case construct。
- reused-entry 优先表达 virtual goto 语义，只有必要时复制 region。
- default-only、case-only、case/default 交叉复用要分开处理。
- jump-table 或 recovered switch 元数据进入 shared switch 表示。

判断标准：

- 多个 switch 共用 default 不再缠成 goto 网。
- 多个 case 共用 entry 时不会错误 fallthrough。
- default 复用不会被误当普通 case 复用。

## P6：对齐 pass options、顺序和质量判断

这部分不一定直接改变单个样例输出，但会影响稳定性。

要做：

- 重新核对 pass 顺序。
- 核对 `RequireGotos`、`PreventNewGotos`、`StrictlyLessGotos`、
  `MustImproveRelativeQuality`、`MaxOptIters`。
- 给每个 pass 的 block / statement / call limit 留记录。
- 质量判断对 copied target、virtual goto、normalized goto target 保持一致。

判断标准：

- 每个默认 option 都有 Angr 对照或 NotDec 保守理由。
- 不接受 goto 更多、结构更差的候选。
- 不因为 copied block id 不同误判质量退化。

## P7：迁移 Angr 测试和真实样例分类

最后用测试确认不是只修了手写 proxy。

要做：

- 把 Angr SAILR 相关测试继续迁到 shared regression。
- 对不能直接迁移的测试，说明是 IR 表达差异、payload 差异，还是 NotDec 暂不适用。
- 用真实样例分类：pass 没触发、trial 拒绝、结构改善、结构退化、payload 问题。
- 每次扩大算法后做性能 smoke。

判断标准：

- `structuring-analysis-test` 覆盖每个关键差距。
- `run_sailr_bench2_migration.py` 不再主要依赖 proxy 名字撑覆盖。
- fortune 或当前关注样例同口径时间没有明显退化。

# 总完成条件

只有同时满足下面条件，才算“完整修复已知差距”：

1. 有 Angr SAILR pass 对照清单，每个差异都有状态。
2. `DuplicationReverter` 不再停留在 exact/common-tail/linear-tail 子集，至少覆盖
   保守 merge graph。
3. `ReturnDuplicatorLow` 覆盖一般 single-entry return region，并正确处理 copied
   payload / Phi / vvar。
4. `LoweredSwitchSimplifier` 能从常见 lowered if-chain 恢复 switch。
5. switch default / reused-entry 的复杂复用有 shared CFG 表达。
6. copied / virtual block 的变量和值来源不依赖 C 或 Solidity renderer。
7. pass options 和 quality guard 差异都有记录。
8. Angr 相关测试迁移或明确归类完成。
9. `structuring-analysis-test`、`run_structuring_smoke.py`、
   `run_sailr_bench2_migration.py` 通过。
10. fortune 或同等级真实样例性能 smoke 没有明显退化。

# 风险

最大风险是把“结构看起来更像 Angr”和“值来源真的正确”混在一起。后续只要涉及
copied region，就必须先看 payload / vvar / dephication context 是否能表达。

第二个风险是把 Angr 的 AIL 语义硬套到 LLVM payload 上。能对齐的要对齐，不能直接
对齐的要写清楚差异，不能静默假装一样。

第三个风险是测试继续停留在 proxy。proxy 可以帮助落地 shared CFG 行为，但最终
必须用真实 Angr 测试和真实样例分类校验。

# 2026-06-27 P0 实现记录

本次只完成对照清单，没有改 C++ 算法。

Angr 对照源码使用本机 `/sn640/angr`，提交 `63c05f1d4`。NotDec 对照范围是
`external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h`
和 `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`。

## Pass 对照表

| Angr pass / 文件 | Angr 关键语义 | NotDec 当前状态 | 下一步 |
| --- | --- | --- | --- |
| `StructuringOptimizationPass`，`optimization_pass.py:443-654` | SAILR during-region pass wrapper，负责初始 structuring、goto guard、固定点、失败回滚、相对质量检查。 | 基本覆盖。NotDec 在 `StructuringOptimizationPass.h:16-24` 保存同类 option，在 `StructuringOptimizationPass.cpp:62-130` 做 evaluate、rollback、goto/quality guard。后续又补了 Angr stage 顺序、filtered goto 统计、label/goto 清理和 helper children 去重。 | P6 剩余重点是继续用真实样例审 copied target、virtual goto、case/default edge kind 的质量统计。 |
| `SwitchDefaultCaseDuplicator`，`switch_default_case_duplicator.py:20-166` | 基于 jump table 找 default case 复用；多 switch 共享 default 时写成 goto；外部 predecessor 复用时复制 default block。 | 部分覆盖。NotDec 已处理 shared switch default、synthetic goto / forwarder、线性 default region、case/default overlap 拆边、shared default 后 case 边保留和 C renderer 输出。还没有 Angr 那种直接消费 CFG jump table metadata 的入口。 | P5：接入 recovered switch / jump-table 元数据，继续区分 default-only、case-only、case/default 交叉复用。 |
| `DuplicationReverter`，`duplication_reverter.py:36-180`、`655-730`、`1056-1175` | 从 goto 周边找候选，构造 `AILMergeGraph`，按相似语句/子图拆分公共部分，再重接 predecessor、successor、jump target。 | 部分覆盖。NotDec 覆盖 exact duplicate、common statement tail、linear region tail、copied-prefix shared tail、switch predecessor redirect 和 copied goto target 归一化；但还没有通用 merge graph、条件重建和非 tail 拆分。`20260627-01` 属于这个 pass 的 copied-prefix tail 子项。 | P1：先补 shared 条件/guard 表达，或者只做不需要条件重接的更窄 merge graph。 |
| `SwitchReusedEntryRewriter`，`switch_reused_entry_rewriter.py:20-132` | 基于 jump table entry，发现多个 switch head 复用同一 case entry 时，为后续 head 建 virtual goto，不复制 entry。 | 部分覆盖。NotDec 基于 shared switch case edge 建 synthetic goto，保留最低 id 的 entry，已有 reuse limit，并补了 default-only skip、case/default overlap 和真实 C 输出边界。缺口仍是 jump-table/recovered switch metadata 和复杂 default/case 混用。 | P5：和 default 复用共用 recovered switch 表示，明确 default-only 不进 case-entry 逻辑。 |
| `LoweredSwitchSimplifier`，`lowered_switch_simplifier.py:143-260`、`413-939` | 识别 `==` / `!=` 链和范围比较树，收集 case/default，生成 incomplete switch head，并处理 shared case node。 | 部分覆盖。NotDec 现在有 condition compare metadata，能消费 `==` / `!=` if-chain、一层和线性 nested range guard、左右 range-tree，并能保守合并 RetDup 产生的 duplicated default return；同时过滤 shared case/default target、连续 case、distinct target、default 回流、重复 case value 和 all-ones sentinel，也补了结构树 `DefaultTarget` 和脚本 smoke。剩余主要是 Angr 更完整的 range-tree 细节、shared case node 复杂复制，以及 jump-table/recovered switch metadata。 | P4：继续审 Angr 的 range-tree 细节和 shared case node 复制；P5：和 recovered switch metadata 共用 case/default 表示。 |
| `ReturnDuplicatorLow` / `ReturnDuplicatorBase`，`return_duplicator_low.py:18-171`、`return_duplicator_base.py:69-220`、`219-660` | 从 end node 反推 single-entry return region，按 goto edge 和 connected predecessor component 复制，复制时处理 Phi、fresh vvar、label、删除原 region。 | 部分覆盖。NotDec 能复制线性 return tail、unreachable tail、terminal fork 默认管线、nested/diamond/joined-diamond、direct-return side、branch/switch wrapper、switch return tail、grouped predecessor，并覆盖多 vvar、dephication incoming 删除/复制和 copied payload 的多个代理形状；也按调用数和语句数限制复制。差距仍是 Angr 的通用 single-entry region 枚举和 Phi/vvar 全量消费。 | P2 + P3：继续补更一般的 endnode region 枚举，并扩大 copied payload / vvar 的真实输出覆盖。 |
| `CrossJumpReverter`，`cross_jump_reverter.py:15-107` | 最后运行；对只有一个 goto 的块，复制目标的单 successor 线性块；限制调用数，要求 goto 数下降。 | 部分覆盖但方向接近。NotDec 复制线性 region，支持 switch case/default edge kind 拆分、grouped predecessor、Angr call-count 成本 guard、single-switch case/default both-edge 和 edge kind 传播。 | P7：继续核对真实样例行为和测试迁移；如果真实样例显示质量判断仍偏离，再回到 P6。 |
| `ConstPropOptReverter`，`const_prop_reverter.py` | SAILR/DREAM 共享的前置去常量传播 pass，用于让后续相似性更容易成立。 | 暂未实现，且不在当前 shared CFG deoptimization pipeline。 | 暂不放 P1-P5 主线；P7 真实样例如果显示它是主因，再单独写计划。 |
| `ReturnDuplicatorHigh`、`ReturnDeduplicator` | SAILR/DREAM 共享外围 pass；`ReturnDeduplicator` 在 full preset 里紧跟 `ReturnDuplicatorLow`，把两侧相同 return 收回到共享 return。 | `ReturnDeduplicator` 已补 shared CFG 窄子集：两个私有 branch return arm 的末尾 return payload 相同时共享一个 return block。`ReturnDuplicatorHigh` 仍暂不适用。 | `ReturnDeduplicator` 后续只按真实 shared CFG 阻塞继续扩；`ReturnDuplicatorHigh` 暂不放 P1-P5 主线。 |

## 当前测试迁移状态

Angr 侧相关测试主要在：

- `/sn640/angr/tests/analyses/decompiler/test_decompiler.py`
  - `test_sailr_motivating_example`
  - `test_fmt_deduplication`
  - `test_true_a_graph_deduplication`
  - `test_deduplication_too_sensitive_split_3`
  - `test_reverting_switch_lowering_*`
  - `test_decompiling_reused_entries_between_switch_cases`
  - `test_decompiling_abnormal_switch_case_*`
- `/sn640/angr/tests/analyses/decompiler/test_switch_default_case_dup.py`
  - `test_switch_case_header_mismatch_caused_by_cmovs`
- `/sn640/angr/tests/analyses/decompiler/test_dogbolt_regressions.py`
  - LoweredSwitchSimplifier infinite-loop regression。

NotDec 当前已有覆盖：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  - `DuplicationReverter` 覆盖 exact、payload origin、common statement tail、
    linear tail、copied-prefix tail、switch predecessor redirect 和 copied goto
    target 归一化。
  - `ReturnDuplicatorLow` 覆盖 goto return target、grouped predecessor、
    parent goto source、nested/diamond/joined-diamond/fork/branch/switch return
    region 的一部分，以及 dephication vvar / incoming 的多个复制和删除场景。
  - `SwitchDefaultCaseDuplicator` 覆盖 shared default goto、forwarder、linear tail、
    grouped predecessor、rollback、case/default overlap 和 shared default 后 case 边保留。
  - `SwitchReusedEntryRewriter` 覆盖 reused case entry、limit、default-only skip。
    后续又补了 case/default overlap 的 shared CFG 和 renderer 覆盖。
  - `LoweredSwitchSimplifier` 覆盖已有 switch case target 复用、`==` / `!=`
    if-chain、简单 range guard、nested range guard、左右 range-tree、duplicated default
    return 等价、shared target 过滤、连续 case / distinct target 启发式、default 回流、
    duplicate case value 和 all-ones sentinel；仍不覆盖 recovered switch / jump-table
    metadata 和 Angr 更完整的 shared case node 复制。
  - `CrossJumpReverter` 覆盖 linear goto target、grouped predecessor、case/default
    edge kind、call-count 成本和 single-switch case/default both-edge。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  当前有 18 个分类样例；本机缺 3 个外部真实输入，剩余 15 个 proxy/fixture 样例通过。
  覆盖 `ReturnDuplicatorLow`、`DuplicationReverter`、`LoweredSwitchSimplifier`、
  `SwitchDefaultCaseDuplicator`、`SwitchReusedEntryRewriter` 和部分 switch overlap 输出。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py`
  覆盖 angr dephication Phi、multi-Phi、copied switch/return region、return region
  多种代理形状、lowered switch 安全边界，以及 SAILR/Phoenix 对比样例。

测试分类：

| 类别 | 当前结论 |
| --- | --- |
| 可直接迁移 | Switch default/reused-entry 的小图行为、CrossJumpReverter 线性目标、ReturnDuplicatorLow 简单 return tail。已有较多在 `structuring_analysis_test.cpp` 和 `run_structuring_smoke.py`。 |
| 需要 IR/payload 代理 | `DuplicationReverter` merge graph、ReturnDuplicatorLow Phi/vvar、LoweredSwitchSimplifier if-chain/range-tree。Angr 测试基于 AIL 和真实 binary，NotDec 需要 shared CFG proxy 或 Bench2 IR。 |
| 暂不适用 | Angr `ConstPropOptReverter`、`ReturnDuplicatorHigh` 的非 shared-CFG 主线测试，以及 `ReturnDeduplicator` 超出两个私有 branch return arm 的形状。除非真实样例证明它们阻塞 P1-P5，否则先不算本计划完成条件。 |

## 这次修改的文件和行

- `logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  - 将 P0 标题改成已完成。
  - 在文末追加本节，记录 Angr 源码版本、NotDec 对照文件、pass 差距表、测试迁移状态。

## 验证

这次只改文档，不改反编译代码路径，所以没有跑性能 smoke。完成后检查：

- `git diff -- logs/20260627-02-sailr-angr-remaining-parity-plan.md`
- `git status --short`

# 2026-06-27 P1 路线检查

检查 `StructuredCFG::materializeBlockBody()`、`duplicateRegion()`、`redirectPredecessors()`
后确认：当前 shared CFG 能复制 region、重写 successor/case target、materialize payload，
但还没有 Angr `AILMergeGraph` 那种“两个不同前缀合并到一个公共主体后，用恢复出的条件
选择不同后缀”的 shared 条件表达。

因此，P1 不能直接做“中间公共段 + 两边后缀重接”的 merge graph，否则会把 incoming-
specific successor 语义藏到普通 goto 或 renderer 行为里。后续 P1 要先补 shared
guard/condition 表达，或者先限定到不需要条件重接的更窄形状。

本次没有改算法，继续转向 P2 的一般 return region；P2 已经有 `duplicateRegion()`、
payload materialize 和 predecessor rewrite 基础，能更直接推进。

# 2026-06-27 P2 实现记录

本次只补 `ReturnDuplicatorLow` 的复制成本限制，还没有推进 Phi / vvar 的完整消费。
Angr 用 call 数限制 return region；shared CFG 当前没有 call counter，所以先用语句数做
保守上限，避免大 return region 被复制后放大 CFG。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:34`
  - `ReturnDuplicatorLow` 构造函数增加 `MaxDuplicatedStatements` 参数。
  - `ReturnDuplicatorLow` 增加同名成员，默认值是 16。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1699`
  - 新增 `statementCountInRegion(const StructuredCFG &, const ReturnRegion &)`，
    用 return region 里的 block statement 数估算复制成本。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2196`
  - `ReturnDuplicatorLow::runOnGraph()` 在复制前跳过超过上限的 region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4400`
  - 新增 `testReturnDuplicatorLowSkipsLargeReturnRegion()`，覆盖多 block return
    region 超限时不复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9974`
  - 在测试入口注册新用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-backends/Structuring/SAILRDeoptimization.h lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retlimit.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=197.80 user=220.01 sys=1.57 maxrss=1263732`。过程中仍有既有 codegen
  warning / error 日志，但最终生成输出并正常退出。

## 影响判断

- 实现效果：3/5。补上了 Angr return duplication 成本保护的一部分，但还没有补通用
  return region 和 Phi / vvar。
- 复杂度：1/5。只增加一个计数 helper 和一个早退出判断，不改变 CFG 重写路径。
- 维护成本：1/5。参数默认值集中在 pass 构造函数，测试覆盖超限行为。

后续 P2 仍要继续补 endnode region 枚举和更完整的 copied payload 处理；这次只是先把
复制放大风险压住。

# 2026-06-27 P2 unreachable end node 实现记录

本次继续推进 P2 的 end node 枚举。Angr `ReturnDuplicatorBase._find_endnode_regions()`
从所有无 successor 的 end node 反推 region；NotDec 之前只从 `Return` 终点进入，
会漏掉 `Unreachable` 结尾的闭合 return/trap tail。shared CFG 已有
`isClosedTerminal()`，所以这次只把终点判断改成复用它，不改变复制和 payload 重写。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:390`
  - `collectDiamondReturnRegion()` 的 terminal 判断从只认 `Return` 放宽为
    `isClosedTerminal()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:513`
  - `findLinearReturnRegion()` 的入口判断同样改为 `isClosedTerminal()`，使
    `Unreachable` tail 也能作为 end node 被反推。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5459`
  - 新增 `testReturnDuplicatorLowCopiesUnreachableTailRegion()`，覆盖 goto 指向
    trap tail 时复制 copied tail 和 copied unreachable block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10034`
  - 在测试入口注册新用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-unreachable-retregion.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=196.49 user=219.18 sys=1.65 maxrss=1266724`。和上一轮
  `197.80s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补上 end node 枚举里的一个明确缺口，但还没有完成 Angr 的全部
  `_single_entry_region()` 语义。
- 复杂度：1/5。只复用已有闭合终点判断。
- 维护成本：1/5。测试覆盖了新增 reachable 行为。

后续 P2 仍剩：更完整的 single-entry region 枚举、删除原 region 后的 Phi/vvar
incoming 处理，以及 P3 的 copied payload 全量消费。

# 2026-06-27 P6 pass option 对齐记录

本次核对 Angr `/sn640/angr` 当前 pass 默认参数后，补齐 shared pipeline 里
`RequireStructurableGraph` 的差异。Angr 的 `LoweredSwitchSimplifier` 明确
`require_structurable_graph=False`；`SwitchDefaultCaseDuplicator` 和
`SwitchReusedEntryRewriter` 在 Angr 属于 AIL graph 创建后 / jump-table 相关 pass，
不是 during-region structuring wrapper。NotDec 把这三个 pass 放进 shared pipeline
时，之前仍继承了初始图必须 structurable 的默认值，会在 pass 有机会修复 switch
形状前先被初始 structuring gate 拦住。

这次只取消这三个 pass 的初始 structurable gate。修改后的候选仍会走
`StructuringOptimizationPass::analyze()` 的最终 structuring 评估，失败不会提交。
`DuplicationReverter`、`ReturnDuplicatorLow`、`CrossJumpReverter` 仍保持 Angr
during-region pass 的默认 structurable/goto/quality gate。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1712`
  - `SwitchReusedEntryRewriter::defaultOptions()` 设置
    `RequireStructurableGraph = false`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1801`
  - `LoweredSwitchSimplifier::defaultOptions()` 设置
    `RequireStructurableGraph = false`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1906`
  - `SwitchDefaultCaseDuplicator::defaultOptions()` 设置
    `RequireStructurableGraph = false`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:349`
  - 新增 `AddFirstSuccessorNoInitialStructuringPass` 测试 helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7331`
  - 新增 `testStructuringOptimizationPassCanSkipInitialStructurableGraph()`，
    覆盖初始 structuring 可跳过，但最终 structuring 失败仍拒绝。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7563`
  - `testSAILRDeoptimizationDefaultOptionsMatchAngr()` 补齐
    `RequireStructurableGraph` 字段检查。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-options.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=196.30 user=218.41 sys=1.70 maxrss=1272048`。和上一轮
  `196.49s` 同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。P6 的默认 option 又少了一个和 Angr 不一致的 gate，但 pass
  顺序和质量统计仍需要真实样例继续审。
- 复杂度：1/5。只改默认选项和测试，不改具体 CFG rewrite。
- 维护成本：1/5。默认字段已有集中测试。

P6 剩余工作：继续核对 copied target / virtual goto 的质量统计在真实样例里的效果；
如果没有新差异，本项后续可以转成文档归档，而不是继续改默认值。

# 2026-06-27 P4 路线检查

本次检查了 NotDec 和 Angr 的 `LoweredSwitchSimplifier`。结论是：当前不能用一个小
CFG 重写直接实现“if-chain 恢复 switch”，因为 shared CFG 还没有表达分支条件的语义。

NotDec 现在的条件只是一层 payload 引用：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h`
  里的 `CFGBlock::Condition` 是 `PayloadRef`。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp` 里
  `LLVMFunctionCFGBuilder::build()` 只通过 `PayloadProvider::getCondition()`
  记录条件 payload，不保存 `x == const`、`x != const`、范围比较、变量身份、
  true/false 目标这些语义。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp` 里的
  `LoweredSwitchSimplifier::runOnGraph()` 目前只扫描已有 `Switch` terminator 的
  case target 复用，并复制共享 case 线性 region；它没有解析 `Branch` 条件。

Angr 的实现不同：

- `/sn640/angr/angr/analyses/decompiler/optimization_passes/lowered_switch_simplifier.py`
  的 `_find_switch_variable_comparison_type_a()`、`_type_b()`、`_type_c()` 直接解析
  AIL `ConditionalJump` 里的 `BinaryOp(VirtualVariable, Const)`。
- `StableVarExprHasher` 用 AIL variable map 判断多条比较是否来自同一个 switch
  variable。
- `_find_cascading_switch_variable_comparisons()` 再基于 `eq` / `gt`、case value、
  default target、reachability 和 case 数过滤，最后生成 switch head。

所以 P4 后续要先补 shared 条件模型，至少能表达：

- 条件种类：`==`、`!=`、`>`、`>=`、`<`、`<=`。
- 被比较的稳定变量身份。
- 常量 case value。
- 条件为 true / false 时分别对应哪个 successor。
- payload 只负责打印，不参与语义判断。

在这层补齐前，不应该用 payload 字符串解析或者 renderer 特判来恢复 switch。这样会把
语义藏到后端里，也无法保证 C 和 Solidity 走同一份 shared CFG 结果。

本次没有改算法，P4 暂时记录为需要先设计 shared condition metadata。后续可以先做
LLVM `icmp` + branch 的最小模型，再迁移 Angr lowered switch 的 if-chain 测试。

## 修改位置

- `logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  - 追加本节，记录 P4 的 blocker、涉及文件和后续路线。

## 验证

这次只改文档，不改代码路径，所以没有跑性能 smoke。检查命令：

- `git diff -- logs/20260627-02-sailr-angr-remaining-parity-plan.md`
- `git status --short`

# 2026-06-27 P5 case/default overlap 实现记录

本次继续推进 P5，只处理一个保守边界：`SwitchDefaultCaseDuplicator` 在复制 default
region 给外部 predecessor 时，如果某个 switch 的 default target 同时也是 case
target，不能把这个 switch 当普通 default predecessor 复制。shared CFG 当前还没有
jump-table/recovered switch metadata 来标明“这次重写的是 default 边还是 case 边”，
而通用 `redirectPredecessors()` 会同时改 `Successors` 和 `Cases`。所以这里先跳过
这种重叠 switch，只复制真正外部 predecessor。

shared-default goto 阶段没有改。已有 `replaceDefaultSwitchSuccessor()` 只更新
`Successors.front()`，不会改 case target，继续保留原行为。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1083`
  - 新增 `switchDefaultAlsoCaseTarget()`，判断 switch default 是否同时出现在 case
    target 里。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2011`
  - `SwitchDefaultCaseDuplicator::runOnGraph()` 在 default-region copy 的
    `PredsToUpdate` 收集阶段跳过 case/default 重叠的 switch predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6106`
  - 新增 `testSwitchDefaultCaseDuplicatorSkipsCaseDefaultOverlap()`，覆盖重叠 switch
    保持 default/case 都指向原块，同时外部 predecessor 仍复制 default region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10157`
  - 在测试入口注册新用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p5-case-default-overlap.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=193.36 user=222.26 sys=1.67 maxrss=1271568`。和上一轮
  `196.30s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了 P5 的一个 case/default 交叉复用边界，但还没有接入
  recovered switch / jump-table metadata。
- 复杂度：1/5。只加一个判断和一个收集阶段 skip，不改 CFG copy 机制。
- 维护成本：1/5。测试覆盖了保留重叠 switch 与复制外部 predecessor 两件事。

P5 剩余工作仍是：default / reused-entry 共用 recovered switch 表示，default-only、
case-only、case/default 交叉复用有更明确的 shared edge kind。

# 2026-06-27 P3/P7 copied payload 覆盖记录

本次没有改算法，只补一个 shared structuring 回归测试。目标是把
`ReturnDuplicatorLow` 的 switch return region 复制、dephication vvar 复制、
payload materialize 三件事放在同一个用例里验证，避免后续只覆盖 branch return
region 或普通 switch copy 时漏掉组合路径。

这个测试不能代表 P3/P7 已完成。它只确认当前 shared CFG copy 机制已经能把 copied
switch 的 condition、case value、return payload、dephication assignment 和 copied
merge vvar context 一起传给 materialize hook。P3/P7 剩余的真实差异仍要靠更多 Angr
迁移用例和真实样例继续看。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5777`
  - 新增 `testReturnDuplicatorLowCopiesSwitchReturnRegionWithDephicationVVars()`。
    用例构造两个外部 predecessor 进入同一个 switch，其中一个 predecessor 触发
    `ReturnDuplicatorLow` 复制完整 switch return region；同时给 default return merge
    block 添加 dephication vvar。
  - 测试里的 materialize hook 检查 copied vvar、dephication assignment、
    switch condition、case value 和 copied merge return payload 都带着正确 context，
    并把 payload id 改写为 `+1000`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10270`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p3-switch-dephi-copy.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=197.89 user=219.93 sys=1.69 maxrss=1271600`。和上一轮
  `193.36s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了 copied switch return region 与 dephication vvar 的组合覆盖，
  但没有新增 Angr 迁移样例，也没有解决 P1/P4 需要 shared condition metadata 的问题。
- 复杂度：1/5。只加测试，不改 shared CFG copy 代码。
- 维护成本：1/5。测试是局部构图，失败时能直接定位到 payload/context copy 路径。

# 2026-06-27 P2/P3 return end-node dephication 覆盖记录

本次继续补 `ReturnDuplicatorLow` 的组合回归测试，没有改算法。Angr 的
`ReturnDuplicatorBase._find_endnode_regions()` 对多个 predecessor 指向同一个 end
node 的情况，会把 end node 本身作为 return region。NotDec 已经有普通 return target
复制测试，但还缺一个直接 return merge block 带 dephication vvar 的覆盖。

新增测试用 LLVM builder 同类形状建图：两个 predecessor 先进入 dephication edge
block，再进入同一个 return merge block。只复制带 goto 的那条 edge，确认 copied
return merge 拿到 copied vvar，copied edge assignment 改写到 copied vvar，未复制的
原 edge 和原 merge 仍保持原 vvar。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4393`
  - 新增 `testReturnDuplicatorLowCopiesReturnEndNodeWithDephicationVVars()`。
    用例覆盖直接 return end node 作为复制 region 时，dephication edge block 的
    assignment、copied merge vvar、原 edge incoming、原 merge vvar 都保持正确。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10384`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-endnode-dephi-copy.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=197.66 user=219.95 sys=1.71 maxrss=1272132`。和上一轮
  `197.89s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了 direct end-node return region 和 dephication edge block 的组合
  覆盖，但 P2 的更完整 endnode region 枚举、P3 的全量 copied payload 消费还没完成。
- 复杂度：1/5。只加测试，不改 CFG 复制和 dephication 表。
- 维护成本：1/5。测试局部、边界明确，失败时能直接定位到 return end-node copy 或
  dephication edge assignment materialize。

# 2026-06-27 P7 copied return end smoke 记录

本次把上一轮 direct return end-node + dephication 的 C++ 回归，补到
`notdec-llvm2c` 脚本层 smoke。目的不是证明 P2/P3 完成，而是让真实
LLVMFunctionCFGBuilder、SAILR dephication mode、shared structuring 和 C backend
一起覆盖这个代理形状。

新增 IR 用一个 switch 的两个 case 共享带 Phi 的 return block。`structured-sailr`
在 `--sailr-dephication-mode=angr` 下会复制其中一条 case 到 copied return end，
预期输出同时出现 copied vvar 返回和原 vvar 返回，并且不能泄漏 `phi` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:366`
  - 新增 `sailr_angr_dephication_copied_return_end` case，覆盖 copied return end 通过
    `notdec-llvm2c` 输出 `p_copy1 = a; return p_copy1;`，原共享分支输出
    `p = b; return p;`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_structuring_smoke.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p7-return-end-smoke.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=197.80 user=219.84 sys=1.83 maxrss=1271852`。和上一轮
  `197.66s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。把一个 P2/P3 代理形状提升到了脚本层 smoke，但仍不是 Angr 真实
  binary 测试的完整迁移。
- 复杂度：1/5。只加一个脚本 case。
- 维护成本：1/5。输入 IR 短，断言直接对应 copied/original dephication 输出。

# 2026-06-27 P6 infinite-loop quality parity 记录

本次继续推进 P6。Angr 的 `StructuringOptimizationPass._improves_relative_quality()`
只比较 `for`、`while`、`do-while` 三类 loop trade；`ControlFlowStructureCounter`
没有 infinite-loop 计数。NotDec 之前在 `totalLoops()` 里把 `InfiniteLoop` 算进总数，
会让 “for 变少但同时出现 infinite loop” 被当成同总数 loop trade 拒绝，和 Angr
质量门槛不一致。

这次只改相对质量比较，不改 `ControlFlowStructureCounter` 的采集字段。`InfiniteLoop`
仍然可统计，但不参与 Angr parity 的 loop trade 总数。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:60`
  - 修改 `totalLoops()`，只返回 `WhileLoops + DoWhileLoops + ForLoops`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7586`
  - 新增 `testRelativeQualityIgnoresInfiniteLoopsForAngrParity()`，覆盖 `ForLoops`
    变少但 `InfiniteLoops` 增加时不被相对质量检查拒绝。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10454`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/StructuringQuality.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-quality-loop-parity.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=155.76 user=177.84 sys=1.44 maxrss=1269144`。和前两轮
  `197.80s`、`197.66s` 同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。补齐一个 P6 质量门槛差异，但 P6 仍剩 copied target / virtual goto
  的真实样例效果核对。
- 复杂度：1/5。只改质量统计 helper 和一个单元测试。
- 维护成本：1/5。行为直接对应 Angr counter 字段，后续如果 NotDec 引入 for-loop
  节点也不需要改这条规则。

# 2026-06-27 P6 goto label quality parity 记录

本次继续推进 P6，补齐 Angr `ControlFlowStructureCounter` 的 label/goto 清理规则。
Angr 在遍历后会丢掉没有输出 label 的 goto target，也会丢掉没有被 goto 用到的 label。
NotDec 之前把这两类都留在质量计数里，可能让相对质量判断受未输出标签或未使用标签影响。

这次只在 `ControlFlowStructureCounter::collect()` 的最终结果上做清理。没有把同样逻辑放进
`qualityWithGotos()`，因为 `ReturnDuplicatorLow` 的 `getNewGotos()` 会把 copied target
映射回原始 source target 给质量过滤使用；此前验证显示在那里清理会拒绝合法的 copied
switch return region 重写。这个边界先保守保留，避免用 Angr 没有的 copied-target
重映射细节硬套质量规则。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringQuality.h:27`
  - 为 `ControlFlowStructureCounter` 新增 `normalizeGotoLabels()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:78`
  - `ControlFlowStructureCounter::collect()` 在遍历后调用 `normalizeGotoLabels()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:85`
  - 新增 `ControlFlowStructureCounter::normalizeGotoLabels()`，先删除没有对应 output label
    的 `GotoTargets`，再删除没有被 goto 使用的 `OrderedLabels`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1005`
  - 更新 `testStructuringEvaluatorCollectsGotoSummary()`，确认 goto edge 仍被记录，但没有
    output label 的目标不进入质量计数。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7528`
  - 扩展 `testControlFlowStructureCounterCollectsSharedQuality()`，覆盖未使用 label、有效
    goto label、无 label 的 goto target 三种情况。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-backends/Structuring/StructuringQuality.h lib/Structuring/StructuringQuality.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-quality-label-normalize.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=155.94 user=178.32 sys=1.64 maxrss=1266176`。和上一轮
  `155.76s` 同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。补齐 P6 的 label/goto 质量计数差异，但 copied target / virtual
  goto 的真实样例效果还需要继续核对。
- 复杂度：1/5。只增加一次采集后清理，不改 structuring 和 CFG copy 流程。
- 维护成本：1/5。规则直接对应 Angr counter 后处理，边界也通过测试固定。

# 2026-06-27 P2 prefixed diamond return region 记录

本次继续推进 P2 的一般 return region 枚举。当前 NotDec 已能识别 diamond return
region，但入口直接停在 diamond branch。如果 diamond 前还有一个简单 fallthrough
wrapper，那么外部 predecessor 只会落在 wrapper 上，`ReturnDuplicatorLow` 看不到
多个外部 predecessor，结果不会复制这个 return region。

这次只处理保守情况：diamond region 前面的单前驱、单后继、fallthrough wrapper。
不处理带条件、switch、循环或多 successor 的前缀，也不引入新的 shared 条件模型。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:441`
  - 新增 `prependFallthroughPrefix()`，在 diamond region 识别后，把简单 fallthrough
    wrapper 放进同一个 `ReturnRegion`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:545`
  - `findLinearReturnRegion()` 在 `collectDiamondReturnRegion()` 成功后调用
    `prependFallthroughPrefix()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5471`
  - 新增 `testReturnDuplicatorLowCopiesPrefixedDiamondReturnRegion()`，覆盖
    `fallthrough -> diamond -> shared return` 形状，只复制有 goto 的 predecessor，
    原 region 保留给另一条 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10507`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-prefixed-diamond-return.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.36 user=179.63 sys=1.69 maxrss=1272084`。和上一轮
  `155.94s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了 P2 的一个具体 end-node return region 缺口，但还没有完成
  Angr 的全部 single-entry region 枚举。
- 复杂度：1/5。只扩展 diamond region 的简单前缀，不改复制流程。
- 维护成本：1/5。规则保守，测试直接覆盖复制后的原 region 保留和 copied region 形状。

# 2026-06-27 P7 prefixed diamond return smoke 记录

本次没有改算法，只把上一轮 P2 的 prefixed diamond return region 覆盖提升到
`notdec-llvm2c` 脚本层 smoke。目标是让 LLVMFunctionCFGBuilder、SAILR structuring
和 C backend 一起覆盖这个代理形状，避免 C++ 局部构图通过但真实输出又退回到共享
return region 的 goto。

这个用例仍是 P2/P7 的代理覆盖，不表示一般 single-entry return region 已完成。真实
Angr binary 用例迁移和更多 end-node region 形状还要继续补。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:400`
  - 新增 `sailr_prefixed_diamond_return_region` case，构造两个 switch case 共享
    `prefix -> diamond -> ret` 的 IR。
  - 断言输出仍是 `switch (x)`、两个 case 都有 `return 7;`，且不出现 `goto prefix`
    或 `goto ret`，同时不泄漏 `phi` / `reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_structuring_smoke.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 第一次和
  `notdec-llvm2c` 并行构建时链接失败，表现为静态库符号缺失；随后顺序重跑通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 顺序重跑通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p7-prefixed-diamond-smoke.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=155.87 user=177.86 sys=1.60 maxrss=1269376`。和上一轮
  `157.36s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。把 P2 prefixed diamond return region 的代理形状提升到了脚本层
  smoke，但仍没有迁移真实 Angr binary 用例。
- 复杂度：1/5。只加一个短 IR smoke case。
- 维护成本：1/5。断言只检查核心结构和禁止的 goto，不依赖完整格式细节。

# 2026-06-27 P2/P3 delete original dephication return end 覆盖记录

本次没有改算法，只补一个 C++ regression。它覆盖 `ReturnDuplicatorLow` 在所有
dephication incoming 边都被复制后，删除原 return merge block 的情况。

这个用例固定几个关键状态：两条 synthetic dephication edge 分别复制到不同的
return copy；原 block 2 被删除；原 vvar 被标记 retired；原 merge block 的 vvar
上下文被清空；剩下的 incoming 都落在复制后的 edge context 上，并且指向不同的 copied
vvar，同时保留 `SourceTarget` 指回原 vvar。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4528`
  - 新增
    `testReturnDuplicatorLowDeletesOriginalReturnEndNodeWithDephicationVVars()`，
    构造两个 predecessor 都通过 synthetic dephication edge 跳到同一个 return
    merge 的形状，断言复制后原 return merge 删除、payload rewrite 生效、copied
    vvar 和 copied incoming 都保持独立。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10615`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-delete-original-dephi.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=160.55 user=183.56 sys=1.62 maxrss=1271640`。和最近几轮
  `155.87s`、`157.36s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补住 copied dephication return end 全覆盖后删除原 merge 的测试缺口，
  但不新增 Angr 行为。
- 复杂度：1/5。只加一个局部 regression。
- 维护成本：1/5。断言直接固定 CFG、vvar 和 incoming 状态，后续失败原因比较明确。

# 2026-06-27 P7 branch return region smoke 记录

本次没有改算法，只把 `ReturnDuplicatorLow` 的 branch return region 覆盖提升到
`notdec-llvm2c` 脚本层 smoke。目标是确认 LLVM IR 提升、SAILR structuring 和 C 输出
都能稳定处理“两条 switch case 共享一个 `branch -> then/else return`”的形状。

这个 smoke 仍是 P2/P7 的代理覆盖，不表示 Angr 的一般 return region 枚举已经完全迁移。
它只固定一个具体形状：两个 switch case 都进入同一个 branch return region，输出里要保留
两个 case，各自内联 `if (a == b)` 和两条 return，不允许回退成 `goto branch` 或
`goto then/else`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:435`
  - 新增 `sailr_branch_return_region` case，构造两个 switch case 共享一个 branch
    return region 的 IR。
  - 断言输出包含 `switch (x)`、`case 1:`、`case 2:`、`if (a == b)`、
    `return 7;` 和 `return 8;`，并且不出现 `goto branch`、`goto then`、
    `goto else`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_structuring_smoke.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p7-branch-return-smoke.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.95 user=179.35 sys=1.45 maxrss=1270196`。和上一轮
  `160.55s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了一个真实提升链路上的 branch return region 脚本 smoke，但仍只是
  P2/P7 代理，不是 Angr 全量迁移。
- 复杂度：1/5。只加一个短 IR case。
- 维护成本：1/5。断言围绕核心结构和禁止的 goto，后续诊断成本低。

# 2026-06-27 branch return dephication 缺口检查记录

这次顺着 P2/P3 再检查了一次 branch return region 的删原 region 方向，确认了一个点：
`ReturnDuplicatorLow` 现有的 branch return payload rewrite 测试已经能覆盖条件、then/else
和 grouped predecessor 复制，但 branch 这条线并没有一条稳的、可直接迁移的
dephication/delete-original 回归，不适合硬塞一个不清楚 payload 位置的测试。

我检查后没有保留新的代码改动，也没有改算法。这个结果的价值是把 branch 方向从
“看起来像有缺口”收回到“当前证据不足，不继续硬补”，避免把 P2/P3 的工作拧到
不稳的测试语义上。

## 结论

- `ReturnDuplicatorLow` 的 return/switch dephication delete-original 回归已经有明确
  覆盖。
- branch return region 这条线，当前缺的不是一个已经清楚的最小回归，而是更稳的
  payload/edge 语义样例。
- 这次没有新增文件修改，不进入提交。

# 2026-06-27 P3 multi-vvar return end 覆盖记录

本次没有改算法，只补一个 C++ regression，推进 P3 的 shared Phi / vvar / copied
payload 消费覆盖。现有脚本层已有 `multi_phi_same_edge`，但 C++ 的
`ReturnDuplicatorLow` return-end 复制回归主要还是单 vvar。这里补上“同一条 copied
dephication edge 上有两个 vvar incoming”的形状。

这个测试固定几件事：复制 return end 后原 merge 被删除；同一条 edge 的两个 assignment
都被 hook 按各自 vvar 重写；每条 copied edge 都拿到两个 copied vvar；copied incoming
都指向 copied return merge，并保留 assignment 的重写结果。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4651`
  - 新增 `testReturnDuplicatorLowCopiesReturnEndNodeWithMultipleDephicationVVars()`。
  - 构造两个 synthetic dephication edge，每条 edge 同时给 `x` 和 `y` 两个 vvar 赋值，
    然后复制共享 return end，断言 copied edge、copied vvar、assignment rewrite 和
    incoming metadata 都一致。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10755`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p3-multi-vvar-return-end.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.73 user=179.08 sys=1.66 maxrss=1269212`。和上一轮
  `156.95s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补住 P3 多 vvar copied payload 的一个关键 C++ 回归，但还没有把
  P3 的所有 renderer 消费路径都覆盖完。
- 复杂度：1/5。只加测试，不改 shared CFG 或 deoptimization 逻辑。
- 维护成本：1/5。测试直接检查 vvar copy map、incoming 和 assignment rewrite，失败点清楚。

# 2026-06-27 P5 reused-entry case/default overlap 覆盖记录

本次没有改算法，只补一个 C++ regression。目标是固定
`SwitchReusedEntryRewriter` 在 case/default 交叉复用时的 shared CFG 语义：如果同一个
switch 的 default 和 case 都指向同一个 reused entry，reused-entry pass 只虚拟化
case 边，default 边仍然留在原 entry。这样 P5 后续接入 recovered switch / jump-table
metadata 时，不会把 default-only、case-only 和 case/default overlap 混成一种边。

这个测试不表示 P5 完成。P5 的核心缺口仍是 default / reused-entry 共用 recovered
switch 表示，以及从 jump-table metadata 判断 default-only、case-only、交叉复用。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7239`
  - 新增 `testSwitchReusedEntryRewriterSplitsCaseDefaultOverlap()`。
  - 构造两个 switch 共享同一个 case entry，其中第二个 switch 的 default 和 case
    都指向该 entry；断言重写后第二个 switch 的 case target 变成 synthetic goto，
    default successor 仍指向原 entry，原 entry 的 tail 不被复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10819`
  - 在测试入口注册该用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `git diff --check -- logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过，ninja 无需重建。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过，ninja 无需重建。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p5-reused-case-default-overlap.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.91 user=179.36 sys=1.62 maxrss=1267956`。和上一轮
  `156.73s` 同口径，没有明显退化。

## 影响判断

- 实现效果：1/5。只补一个 P5 边界覆盖，不新增 switch metadata 能力。
- 复杂度：1/5。只加测试，不改 CFG rewrite。
- 维护成本：1/5。测试形状小，断言直接固定 case/default overlap 的边语义。

# 2026-06-27 P5 case/default overlap renderer 记录

本次继续推进 P5，修一个脚本层暴露出来的输出问题：shared CFG 里同一个 switch 的
case 和 default 可以指向同一个 target。之前 `StructuredGoto` 渲染 semantic switch
时，会把 case body 和 default body 各渲染一份。如果两者最后都是同一个 shared block，
C 输出里会出现重复的 `structured_block_N:` label。

这次不改 shared CFG pass，只改 C renderer 对 semantic switch 的输出：只在同一个
switch 内发现 case/default target overlap 时，把这些标签串成连续 `case/default`
标签，最后一层挂同一份 body。普通多个 case 的 copied body 仍按各自 `NodeId` 分开，
避免破坏 `ReturnDuplicatorLow` 的 copied return region 输出。

这不表示 P5 完成。P5 仍缺 recovered switch / jump-table metadata，以及 default-only、
case-only、case/default overlap 的 shared switch 来源分类。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:38`
  - 新增 `SwitchBodyKey` 和 `SwitchBodyLabel`，用于在同一个 semantic switch 里识别
    case/default target overlap。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:558`
  - `StructuredGotoAdapter::renderSwitch(const StructuredTree &, const StructuredNode &)`
    在 case/default overlap 时复用同一个 body，只输出一份 shared label/body。
  - 合并条件只看当前 switch 的 default target 和 case target 是否重叠；其他 case
    body 不按 target 合并，避免多个 copied case 被误合并。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:470`
  - 新增 `sailr_switch_case_default_overlap_body_once`，用两个可达 switch 共享同一个
    target，其中一个 switch 的 case/default 都指向 shared block。
  - 断言输出包含 `switch (y)`、`case 2:`、`default:`、`b();`、`return 0;`，并且
    `structured_block_5:` 只出现一次。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuredGoto.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过，ninja 无需重建。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p5-switch-overlap-render.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.25 user=179.50 sys=1.58 maxrss=1269844`。和上一轮
  `156.91s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。修了 P5 case/default overlap 的真实 C 输出问题，但没有补 recovered
  switch metadata。
- 复杂度：2/5。renderer 增加了一个小型 case/default label 合并规则，范围只限 semantic
  switch。
- 维护成本：2/5。规则依赖 shared CFG 的 target 信息，测试覆盖了不要重复 label，也通过
  既有 copied return smoke 防止误合并 copied case。

# 2026-06-27 P6 pass stage order 对齐记录

本次继续推进 P6，修正 shared SAILR pipeline 的 pass 顺序。之前测试名写的是
`MatchesAngrOrder`，但实际顺序把 `DuplicationReverter` 放在
`SwitchReusedEntryRewriter` 前面，不符合 Angr 的阶段调度。

对照 `/sn640/angr` 当前代码后确认：

- `SwitchDefaultCaseDuplicator` 和 `SwitchReusedEntryRewriter` 的 `STAGE` 都是
  `AFTER_AIL_GRAPH_CREATION`。
- `DuplicationReverter`、`LoweredSwitchSimplifier`、`ReturnDuplicatorLow`、
  `CrossJumpReverter` 是 `DURING_REGION_IDENTIFICATION` 或继承这个阶段。
- NotDec 现在没有 Angr 的完整 AIL stage，所以把这些 pass 折进 shared CFG pipeline
  时，应该先跑两个 jump-table graph-creation pass，再跑 during-region pass。

这次只调整顺序和测试命名，不改变任何单个 pass 的 CFG rewrite 规则。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2481`
  - `buildSAILRDeoptimizationPipeline()` 里把 `SwitchReusedEntryRewriter` 移到
    `DuplicationReverter` 前面。
  - 新增短注释，说明 NotDec 是把 Angr 的两个阶段折到 shared CFG pipeline 里，但保留
    stage 顺序。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8237`
  - 将测试改名为 `testSAILRDeoptimizationPipelineMatchesAngrStageOrder()`。
  - 断言顺序改为 `SwitchDefaultCaseDuplicator`、
    `SwitchReusedEntryRewriter`、`DuplicationReverter`、
    `LoweredSwitchSimplifier`、`ReturnDuplicatorLow`、`CrossJumpReverter`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10866`
  - 更新测试入口调用新名字。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-stage-order.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.19 user=179.45 sys=1.75 maxrss=1269440`。和上一轮
  `157.25s` 同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。P6 的 pass 顺序现在按 Angr stage 折叠后对齐，但
  `ConstPropOptReverter`、`ReturnDeduplicator` 等暂不适用项仍只记录不实现。
- 复杂度：1/5。只调整 pipeline 添加顺序和测试断言，不改具体 pass。
- 维护成本：1/5。测试固定了 stage-order 语义，后续新增 pass 时需要同时说明属于哪个
  Angr 阶段。

# 2026-06-27 P2/P7 prefixed switch return region 覆盖记录

本次没有改算法，只补 P2/P7 覆盖。已有 `ReturnDuplicatorLow` 覆盖了 prefixed diamond
return region、branch return region 和 switch return region，但少了
`prefix -> switch -> return tails` 这个组合形状。

这个形状当前 shared CFG 已能表达：prefix 是简单 fallthrough，后面是 switch，每个
default/case tail 都闭合到 return。新增测试确认 `ReturnDuplicatorLow` 复制 region 时，
prefix、switch condition、case value、default/case tail 和 return 都随同 copied
region 一起保留，不需要 renderer 猜。

脚本层 IR 一开始用单 case inner switch，会触发 `CFGBuilder::visitSwitchInst()` 对
default+1 case 的断言。这个属于 LLVM CFG builder 的单 case switch 限制，不是本次
return-region 目标，所以 smoke 改成 default+2 cases，继续覆盖 prefixed switch return
region。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5820`
  - 新增 `testReturnDuplicatorLowCopiesPrefixedSwitchReturnRegion()`。
  - 构造两个 predecessor 共享 `prefix -> switch -> default/case return tail` 的形状，
    只复制带 goto 的 predecessor，原 region 保留给另一条 predecessor。
  - 断言 copied prefix、copied switch、case target、default/case tail 和 return 都
    复制完整。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10893`
  - 在测试入口注册新用例。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:439`
  - 新增 `sailr_prefixed_switch_return_region`，通过 `notdec-llvm2c` 覆盖真实
    LLVM CFG builder、SAILR structuring 和 C 输出。
  - 断言输出包含外层 `switch (x)`、两个 case、内层 `switch (a + 1)`、
    `case 10:`、`case 11:` 和三条 return；禁止 `goto prefix`、
    `goto inner_default`、`goto inner_case`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过，ninja 无需重建。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  先因 inner switch 只有一个 case 触发 `CFGBuilder::visitSwitchInst()` 断言；改成两个
  case 后通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过，ninja 无需重建。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-prefixed-switch-smoke.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.31 user=179.53 sys=1.52 maxrss=1265740`。和上一轮
  `157.19s` 同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。补了一个 P2/P7 组合覆盖，但没有新增 Angr 的完整 single-entry
  region 枚举。
- 复杂度：1/5。只加 C++ 和脚本测试，不改 CFG rewrite。
- 维护成本：1/5。测试形状明确，脚本层也固定了真实输出不能退回 goto。

# 2026-06-27 P2 branch-prefixed diamond return region 实现记录

本次继续推进 P2 的一般 return region 支持。已有 diamond return region 收集成功后会提前
返回，只额外吸收简单 fallthrough prefix；如果 diamond 上方还有一个外层 branch，且
另一个分支也是 return tail，`ReturnDuplicatorLow` 不能把这一层一起复制。

这次没有新增一套 region 枚举逻辑，而是让 diamond 结果继续走已有的 prepend loop。这样
外层 branch、switch、terminal fork 等已有保守规则可以复用，避免两个收集路径语义分叉。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:513`
  - 修改 `findLinearReturnRegion()`。
  - `collectDiamondReturnRegion()` 成功后不再提前返回，而是先反转
    `Region.Blocks`，再继续走原来的 prepend loop。
  - 删除只服务 diamond 的 `prependFallthroughPrefix()`，避免 fallthrough prefix 和
    一般 prepend 规则分开维护。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5820`
  - 新增 `testReturnDuplicatorLowCopiesBranchPrefixedDiamondReturnRegion()`。
  - 构造 `branch -> diamond return region / return tail`，两个 predecessor 共享外层
    branch，只复制带 goto 的 predecessor。
  - 断言 copied outer branch、diamond head、左右路径、join、diamond return 和另一条
    return tail 都保留 payload 与 terminator。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10991`
  - 在 `main()` 注册新测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-branch-prefixed-diamond.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.91 user=180.21 sys=1.67 maxrss=1271744`。和近期
  `157s` 左右的同口径结果一致，没有明显退化。

## 影响判断

- 实现效果：3/5。覆盖了 diamond return region 上方还有 branch wrapper 的常见形状，
  但还不是 Angr `_single_entry_region()` 的完整 parity。
- 复杂度：2/5。复用现有 prepend loop，删除一段专用 prefix helper，逻辑更集中。
- 维护成本：2/5。新增测试固定组合形状；后续如果扩展 return region，只需要继续看
  `findLinearReturnRegion()` 的统一 prepend 路径。

# 2026-06-27 P2 diamond return tail 实现记录

本次继续推进 P2 的一般 return region。上一轮已经能把 diamond return region 上方的
branch wrapper 一起复制，但 wrapper 的其它分支仍只能是线性 return tail。遇到
`branch -> linear return tail / diamond return tail` 时，`ReturnDuplicatorLow` 还是不能把
外层 branch 作为一个完整 region 复制。

这次没有做任意 DAG 枚举，只给 closed return tail 增加一个保守 diamond 形状：两条
fallthrough 路径必须各自只有单 predecessor，最后汇到同一个 closed terminal，terminal
也只能有这两个 predecessor。然后 branch/switch prepend 继续复用同一个
`collectClosedReturnTail()`。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:349`
  - 新增 `collectFallthroughPathToClosedTerminal()`，按单 predecessor fallthrough 链向后
    收集到 return/unreachable。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:384`
  - 新增 `collectClosedDiamondReturnTail()`，只接受两条路径汇到同一个 closed terminal
    的 diamond tail。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:467`
  - 新增 `collectClosedReturnTail()`，先试原有线性 tail，再试 diamond tail，失败时不污染
    `Seen`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:604`
  - `prependBranchReturnRegion()` 改用 `collectClosedReturnTail()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:639`
  - `prependSwitchReturnRegion()` 改用 `collectClosedReturnTail()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5918`
  - 新增 `testReturnDuplicatorLowCopiesBranchWithDiamondReturnTail()`。
  - 构造外层 branch 的一侧是普通 return tail，另一侧是 diamond return tail；只复制带
    goto 的 predecessor。
  - 断言 copied outer branch、普通 tail、diamond head、左右路径、join terminal 都保留
    source block、payload 和 terminator。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11101`
  - 在 `main()` 注册新测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-diamond-tail.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.99 user=179.52 sys=1.69 maxrss=1268212`。和近期
  `157s` 左右同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。补上 branch/switch wrapper 其它分支为 diamond tail 的保守形状，但
  仍不是 Angr `_single_entry_region()` 的完整枚举。
- 复杂度：2/5。新增一个小型 tail shape helper，复用现有 prepend 和复制流程。
- 维护成本：2/5。规则比较保守，限制在 single-predecessor fallthrough diamond tail；
  后续扩展其它 DAG 形状时继续收敛到 `collectClosedReturnTail()`。

# 2026-06-27 SAILRDeoptimization 编译歧义修复记录

本次继续收口 `SAILRDeoptimization.cpp` 的一个编译问题，不改算法语义。前一轮新增了
`hasInitialSourceGoto()` 的外层定义后，文件里还残留了一条匿名命名空间里的前置
声明，导致 `DuplicationReverter::getNewGotos()` 和 `ReturnDuplicatorLow::getNewGotos()`
都出现同名可见候选。这个问题只影响编译，不影响 P1-P7 的结构目标。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:498`
  - 删除匿名命名空间里的旧 `hasInitialSourceGoto()` 前置声明。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1874`
  - 在匿名命名空间外补一个 `hasInitialSourceGoto()` 前置声明，供
    `DuplicationReverter::getNewGotos()` 和 `ReturnDuplicatorLow::getNewGotos()` 共用。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
  - 这轮修复前失败，报 `hasInitialSourceGoto` 未声明和随后的一处重载歧义。
  - 修复后通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-parity.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.65 user=180.22 sys=1.72 maxrss=1270540`。和近期
  `157s` 左右同口径，没有明显退化。

## 影响判断

- 实现效果：1/5。只修了编译可见性问题，没有动任何 structuring 语义。
- 复杂度：1/5。只是删掉一个重复声明。
- 维护成本：1/5。后续这类 helper 统一放在一个可见命名空间里，避免匿名 namespace
  和外层 namespace 重名。

# 2026-06-27 P6/P7 记录：默认选项对齐和真实样例边界

这轮补了两件事，都是收边界，不是扩算法。

1. `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8720-8759`
   里的 `testSAILRDeoptimizationDefaultOptionsMatchAngr()`，现在也检查
   `ReturnDuplicatorLow` 和 `CrossJumpReverter` 的 `MustImproveRelativeQuality`。
   这两个 pass 之前只核了 `RequireGotos`、`PreventNewGotos`、`StrictlyLessGotos`，
   现在把最后一条质量门也补上了。
2. 这轮扫真实 Bench2 `module-all.ll` 时，下面这些样例都还是先被前端挡住，不适合拿来
   替换迁移脚本里的 proxy：
   - `selected-targets-native/lighttpd/executable/module-all.ll`
   - `selected-targets-native/redis/server-symlink/module-all.ll`
   - `selected-targets-native/fortune/executable/module-all.ll`
   - `hexx64/java/module-all.ll`

   它们都先撞到 `SAContext::getIntrinsic(): unhandled intrinsic`。
   `hexx64/native/module-all.ll` 不是 intrinsic 问题，而是先撞到
   `pred_iterator out of range!`。

现在的结论是：这些 `module-all.ll` 还在当前 frontend 的支持边界外，先不要继续拿它们
当 migration 基线。当前还能稳定保留的非平凡真实样例，还是
`lighttpd/1-main_init_once.ll`，它已经在 `run_sailr_bench2_migration.py` 里。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8720-8759`
  - `testSAILRDeoptimizationDefaultOptionsMatchAngr()` 新增两个
    `MustImproveRelativeQuality` 断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `ctest --test-dir build -R '^structuring-analysis$' --output-on-failure` 通过。

# 2026-06-27 P6 filtered goto quality 记录

本次继续收口 P6 的质量判断。`StructuringOptimizationPass` 会让具体 pass 通过
`getNewGotos()` 过滤最终 goto，再用过滤后的 goto 重新计算相对质量。之前重算时只替换
`GotoTargets`，没有同步处理 label，导致一个被 pass 保留下来但当前结构里没有输出 label 的
目标，也会参与 `totalGotos()` 和 target 数量判断。

这次改成：过滤后的 goto target 如果没有当前 label，只有它本来就是初始 goto target 时才
保留。这样能忽略真正没有输出 label 的临时目标，同时不破坏
`ReturnDuplicatorLow` / `DuplicationReverter` 把 copied target 归一回原始 target 的行为。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:8-38`
  - `qualityWithGotos()` 传入初始质量结果，重建过滤后的 `GotoTargets` 后，删除既没有
    当前 label、也不是初始 goto target 的目标，并同步删除未使用 label。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:73-75`
  - `acceptsFinalEvaluation()` 调用新的 `qualityWithGotos()` 参数。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:395-425`
  - 新增 `DropSecondSuccessorWithUnlabeledFilteredGotoPass`，构造一个 filtered goto 指向
    无 label 目标的测试 pass。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8564-8583`
  - 新增 `testStructuringOptimizationPassNormalizesFilteredGotoQuality()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11351`
  - 注册新测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `ctest --test-dir build -R '^structuring-analysis$' --output-on-failure` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-filtered-goto-quality.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.59 user=179.89 sys=1.50 maxrss=1268400`。和近期
  `157s` 左右同口径，没有明显退化。

## 影响判断

- 实现效果：2/5。修了 P6 里 filtered goto 质量统计的一条边界，但没有改变 pass
  顺序或更大的质量模型。
- 复杂度：1/5。只是在已有 `qualityWithGotos()` 里补 label/target 同步规则。
- 维护成本：1/5。测试直接覆盖误判条件，同时现有 copied target 归一化用例继续通过。

# 2026-06-27 P2 direct-return-side diamond 记录

本次继续推进 P2 的一般 return region。已有 closed diamond return tail 只接受两侧都有
私有 fallthrough path 的形状，会漏掉一侧直接跳到共同 return、另一侧经过私有 tail
再到同一个 return 的单入口闭合 region。

这次只放宽这个保守形状，不做任意 DAG 枚举。直接 return 侧没有私有 block，所以 terminal
predecessor 记为 branch head；同时 end-node 反推时要求 direct-return-side 的 diamond
head 不能是组件入口，避免把 connected predecessor 误吃进 return region。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:384-391`
  - 新增 `terminalPredecessorForReturnPath()`，处理直接 branch-to-return 侧的
    terminal predecessor。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:426-440`
  - `collectClosedDiamondReturnTail()` 允许一侧 path 只有 terminal，并用新 helper
    计算 terminal predecessor。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:523-603`
  - `collectDiamondReturnRegion()` 允许一侧没有私有 block，但遇到 direct side 时拒绝
    吸收没有 predecessor 的组件入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5791-5861`
  - 新增 `testReturnDuplicatorLowCopiesDiamondReturnRegionWithDirectReturnSide()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11366`
  - 注册新测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `ctest --test-dir build -R '^structuring-analysis$' --output-on-failure` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-direct-diamond-return.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.99 user=179.17 sys=1.60 maxrss=1271616`。和近期
  `157s` 左右同口径，没有明显退化。

## 影响判断

- 实现效果：3/5。补上 direct-return-side diamond return region，但仍不是 Angr 的完整
  `_single_entry_region()`。
- 复杂度：2/5。只扩展现有 diamond 收集路径，另加组件入口保护。
- 维护成本：2/5。规则仍是保守 shape 匹配，测试覆盖了误判风险最相关的复制结果。

# 2026-06-27 P7 direct-return-side diamond smoke 记录

本次没有改算法，只把上一轮 P2 的 direct-return-side diamond return region 覆盖提升到
`notdec-llvm2c` 脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享
`head`，`head` 一侧直接到共同 `ret`，另一侧经过私有 `tail` 再到 `ret`。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region
已经完成。它只保证当前真实 `notdec-llvm2c` 链路不会把这个 direct-return-side
diamond 退回成 `goto head/tail/ret` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:439-472`
  - 在 `CASES` 里新增 `sailr_direct_diamond_return_region`。
  - 构造两个 switch case 共享 `head -> ret/tail -> ret` 的 IR。
  - 断言输出保留 `switch (x)`、两个 case、`if (a == b)`、`a + 1;`
    和 4 个 `return 7;`，同时不出现 `goto head`、`goto tail`、`goto ret`、
    `phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 P2 direct-return-side diamond 的脚本级回归覆盖，但没有新增
  Angr 的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言直接对应当前输出形状；本次没有改运行时代码，不涉及性能路径变化。

# 2026-06-27 P7 diamond return tail smoke 记录

本次没有改算法，只把 P2 的 diamond return tail 覆盖提升到 `notdec-llvm2c`
脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享外层 branch，
外层一侧是普通 return tail，另一侧是 diamond return tail。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region
已经完成。它只保证当前真实 `notdec-llvm2c` 链路不会把
`branch -> linear return tail / diamond return tail` 退回成 `goto` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:553-610`
  - 在 `CASES` 里新增 `sailr_branch_diamond_return_tail`。
  - 构造两个 switch case 共享 `outer`，`outer` 一侧进入普通 `return 7`
    tail，另一侧进入 `diamond_head -> left/right -> diamond_ret`。
  - 断言输出保留 `switch (x)`、两个 case、外层 `if (a == b)`、内层
    `if (a > b)`、2 个 `return 7;` 和 4 个 `return 9;`，同时不出现
    `goto outer`、`goto plain_tail`、`goto plain_ret`、`goto diamond_head`、
    `goto diamond_ret`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 diamond return tail 的脚本级回归覆盖，但没有新增 Angr
  的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言只固定结构关键词和 goto/reg2mem 退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P7 switch diamond return tail smoke 记录

本次没有改算法，只把 P2 的 switch wrapper + diamond return tail 覆盖提升到
`notdec-llvm2c` 脚本层 smoke。这个用例固定一个具体形状：两个外层 switch case
共享内层 switch，内层 default 和一个 case 是普通 return tail，另一个 case 是
diamond return tail。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region
已经完成。它只保证当前真实 `notdec-llvm2c` 链路不会把
`switch -> linear return tail / diamond return tail` 退回成 `goto` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:613-682`
  - 在 `CASES` 里新增 `sailr_switch_diamond_return_tail`。
  - 构造两个外层 switch case 共享 `inner_switch`，内层 `case 10` 进入
    `diamond_head -> left/right -> diamond_ret`，`case 11` 和 default 走普通
    return tail。
  - 断言输出保留外层 `switch (x)`、两个外层 case、2 个内层 `switch (a)`、
    `case 10:`、`case 11:`、内层 `if (a > b)`、2 个 `return 7;`、
    2 个 `return 8;` 和 4 个 `return 9;`，同时不出现 `goto inner_switch`、
    `goto plain_tail`、`goto plain_ret`、`goto case_tail`、`goto case_ret`、
    `goto diamond_head`、`goto diamond_ret`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 switch wrapper + diamond return tail 的脚本级回归覆盖，
  但没有新增 Angr 的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定结构关键词和 goto/reg2mem 退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P7 unreachable tail smoke 记录

本次没有改算法，只把 P2 的 unreachable end-node 覆盖提升到 `notdec-llvm2c`
脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享一个普通 tail，
tail 后面是 `unreachable` 终点。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 end-node region 枚举已经完成。
它只保证当前真实 `notdec-llvm2c` 链路不会把 `tail -> unreachable` 共享区域退回成
`goto tail/trap` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:400-429`
  - 在 `CASES` 里新增 `sailr_unreachable_tail_region`。
  - 构造两个 switch case 共享 `tail -> trap`，其中 `trap` 是 `unreachable`。
  - 断言输出保留 `switch (x)`、两个 case、2 个 `a + 1;` 和 `return 0;`，
    同时不出现 `goto tail`、`goto trap`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 unreachable end-node tail 的脚本级回归覆盖，但没有新增
  Angr 的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定结构关键词和 goto/reg2mem 退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P7 branch-prefixed diamond smoke 记录

本次没有改算法，只把 P2 的 branch-prefixed diamond return region 覆盖提升到
`notdec-llvm2c` 脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享
外层 branch，外层一侧进入 diamond return region，另一侧直接 return。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region
已经完成。它只保证当前真实 `notdec-llvm2c` 链路不会把
`branch -> diamond return region / return tail` 退回成 `goto` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:585-639`
  - 在 `CASES` 里新增 `sailr_branch_prefixed_diamond_return_region`。
  - 构造两个 switch case 共享 `outer`，`outer` 一侧进入
    `diamond_head -> left/right -> diamond_ret`，另一侧进入 `other_ret`。
  - 断言输出保留 `switch (x)`、两个 case、外层 `if (a == b)`、内层
    `if (a > b)`、2 个 `return 8;` 和 4 个 `return 9;`，同时不出现
    `goto outer`、`goto diamond_head`、`goto left_join`、`goto right_join`、
    `goto diamond_ret`、`goto other_ret`、`phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 branch-prefixed diamond return region 的脚本级回归覆盖，
  但没有新增 Angr 的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定结构关键词和 goto/reg2mem 退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P7 nested return region smoke 记录

本次没有改算法，只把 P2 的 nested return region 覆盖提升到 `notdec-llvm2c`
脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享外层 branch，
外层一侧进入带 tail 的内层 branch，另一侧直接 return。

这个 smoke 仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region
已经完成。它只保证当前真实 `notdec-llvm2c` 链路不会把
`branch -> nested branch return region / return tail` 退回成 `goto` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:585-632`
  - 在 `CASES` 里新增 `sailr_nested_return_region`。
  - 构造两个 switch case 共享 `head`，`head` 一侧进入
    `left_tail -> left_branch -> left_ret/left_ret2`，另一侧进入 `right_ret`。
  - 断言输出保留 `switch (x)`、两个 case、外层 `if (a == b)`、内层
    `if (a > b)`、2 个 `a + 1;`、2 个 `return 7;`、2 个 `return 8;`
    和 2 个 `return 9;`，同时不出现 `goto head`、`goto left_tail`、
    `goto left_branch`、`goto right_ret`、`goto left_ret`、`goto left_ret2`、
    `phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 nested return region 的脚本级回归覆盖，但没有新增
  Angr 的通用 region 枚举能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定结构关键词和 goto/reg2mem 退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P3/P7 copied return tail smoke 记录

本次没有改算法，只把 copied return tail + dephication vvar 的真实 `notdec-llvm2c`
链路补到脚本层 smoke。这个用例固定一个具体形状：两个 switch case 共享
`shared_tail -> shared_ret`，`shared_tail` 里有 Phi 和 add，`shared_ret` 负责 return。

这个 smoke 仍是 P3/P7 代理覆盖，不表示 copied payload / Phi / vvar 全量消费已经完成。
它只保证当前 `--sailr-dephication-mode=angr` 下，复制两段 return tail 时不会退回
`phi`、`reg2mem`，并且 copied vvar assignment 能落到 copied tail 上。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:400-438`
  - 在 `CASES` 里新增 `sailr_angr_dephication_copied_return_tail`。
  - 构造两个 switch case 共享 `shared_tail -> shared_ret` 的 IR，其中
    `shared_tail` 含 `%p = phi` 和 `%r = add`。
  - 使用 `--sailr-dephication-mode=angr`，断言输出保留 `int r;`、copied
    `p_copy`、`p_copy1 = a;`、`r = p_copy1 + 1;`、`p = b;`、`r = p + 1;`
    和 2 个 `return r;`，同时不出现 `phi`、`reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 copied return tail 和 dephication vvar 的脚本级回归覆盖，
  但没有新增 Angr 的全量 copied payload 消费能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定 copied vvar、tail assignment 和 return 输出；本次没有改
  运行时代码，不涉及性能路径变化。

# 2026-06-27 P3/P7 switch return dephication smoke 记录

本次没有改算法，只把 switch return region + dephication vvar 的真实 `notdec-llvm2c`
链路补到脚本层 smoke。这个用例固定一个具体形状：两个外层 switch case 共享
内层 switch，内层各分支汇到一个带 Phi 的 return block。

这个 smoke 仍是 P3/P7 代理覆盖，不表示 copied payload / Phi / vvar 全量消费已经完成。
它只保证当前 `--sailr-dephication-mode=angr` 下，复制 switch return region 时
switch condition、case value 和 dephication assignment 能一起落到输出里，不退回
`phi` 或 `reg2mem`。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:366-420`
  - 在 `CASES` 里新增 `sailr_angr_dephication_switch_return_region`。
  - 构造两个外层 switch case 共享 `inner_switch`，内层 default、`case 10`、
    `case 11` 都汇到 `inner_ret`，`inner_ret` 里含 `%p = phi` 和 `%r = add`。
  - 使用 `--sailr-dephication-mode=angr`，断言输出保留 `int p;`、外层
    `switch (x)`、2 个内层 `switch (a)`、`case 10:`、`case 11:`、
    `p = a;`、`p = x;`、`p = b;` 和 2 个 `return p + 1;`，同时不出现
    `phi`、`reg2mem`、`goto inner_switch`、`goto inner_ret`。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 switch return region 和 dephication vvar 的脚本级组合覆盖，
  但没有新增 Angr 的全量 copied payload 消费能力。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言固定 switch/dephication 输出和退化点；本次没有改运行时代码，
  不涉及性能路径变化。

# 2026-06-27 P7 fork return region smoke 边界记录

本次没有改代码，只确认 `TerminalForkRegion` 和 `ReturnTailForkRegion` 暂时不适合提升到
`notdec-llvm2c` 脚本层 smoke。对应 C++ 回归里的 pass options 会放宽质量限制：
`PreventNewGotos=false`、`MustImproveRelativeQuality=false`。真实 CLI 默认 SAILR
没有这些测试专用选项。

用临时 LLVM IR 验证 `return tail fork` 形状时，默认
`./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr`
输出仍包含多处 `goto structured_block_*`，不是一个适合固定成脚本 smoke 的干净输出。
上一轮 `terminal fork` 临时验证也有同样问题。

## 判断

- `terminal fork` / `return tail fork` 继续保留在 C++ 回归层，覆盖底层 region copy
  能力。
- 脚本层 P7 暂时不加这两个 case，避免把“测试专用放宽质量选项下成立”的行为伪装成
  默认真实链路行为。
- 后续如果要脚本化，需要先让默认质量判断能接受该输出，或给 CLI 暴露明确的测试选项；
  这不属于本次 P7 smoke 补覆盖范围。

## 验证

- `return tail fork` 临时 IR：默认 `notdec-llvm2c --algo=structured-sailr` 输出仍有
  `goto structured_block_*`，没有形成干净的两个 copied fork region。
- 本次只更新计划日志，没有改运行时代码，不涉及性能路径变化。

# 2026-06-27 P3/P7 dephication contrast smoke 记录

本次没有改算法，只把上一轮两个 `--sailr-dephication-mode=angr` smoke 补成
legacy/angr 对比。目的不是新增恢复能力，而是固定真实 CLI 上的模式差异：legacy
仍会走 `p_reg2mem`，angr 模式必须输出普通 vvar assignment，不能漏回 `reg2mem`。

覆盖两个形状：

- copied return tail：两个 switch case 共享 `shared_tail -> shared_ret`。
- switch return region：两个外层 switch case 共享内层 switch，内层各分支汇到带 Phi
  的 return block。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1277-1325`
  - 新增 `run_sailr_dephication_copied_return_tail_contrast_case()`。
  - legacy 断言 `p_reg2mem` 存在，angr 断言 copied vvar、`r = p_copy1 + 1;`
    和 `return r;` 存在，且不泄漏 `p_reg2mem`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1328-1378`
  - 新增 `run_sailr_dephication_switch_return_contrast_case()`。
  - legacy 断言 `p_reg2mem` 和 `return *(int *)&p_reg2mem + 1;` 存在，
    angr 断言 `int p;`、`p = a;`、`p = b;`、`p = x;` 和
    `return p + 1;` 存在，且不泄漏 `p_reg2mem`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1399-1404`
  - 在 `main()` 里调用两个新增 contrast case。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：2/5。补了 copied return tail 和 switch return region 的 legacy/angr
  对比覆盖，但没有新增 Angr 的全量 copied payload 消费能力。
- 复杂度：1/5。只新增两个脚本 contrast case。
- 维护成本：1/5。断言固定模式差异和 reg2mem 退化点；本次没有改运行时代码，不涉及
  性能路径变化。

# 2026-06-27 P6/P7 CrossJump 成本边界回归记录

本次没有改算法，只补 `CrossJumpReverter` 的复制成本边界测试。Angr 的
`CrossJumpReverter` 用 `max_call_duplications` 限制目标块里可复制的调用数；NotDec
当前还没有 call counter，只用 `MaxDuplicatedStatements` 限制 copied linear region
的语句数。这个测试只固定当前 NotDec 的保守上限行为，不表示 P6 的 call-counter
成本模型已经和 Angr 对齐。

脚本层继续不新增单独的 CrossJump smoke。默认 CLI 的 condensing 样例已经能覆盖真实
结果，但很难证明某个小 IR 是由 `CrossJumpReverter` 单独触发，而不是前面的
`ReturnDuplicatorLow` 或 structurer 自身消掉了 goto。这里先保留 C++ 级直接覆盖。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3130`
  - 新增 `testCrossJumpReverterSkipsLargeLinearGotoTarget()`。
  - 构造 `0 -> 1 -> 2`，并给 target block 1 放两条 payload statement。
  - 直接喂 `StructuredGoto{0, 1}`，用 `MaxDuplicatedStatements=1` 运行
    `CrossJumpReverter::runOnGraph()`，断言没有复制和改边，原 target 保留。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11372`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：1/5。只固定当前 NotDec 的 statement 上限边界，没有实现 Angr call
  counter。
- 复杂度：1/5。只新增一个 C++ 回归测试。
- 维护成本：1/5。测试直接调用 pass 入口，失败时能定位到 CrossJump 成本 gate；本次
  没有改运行时代码，不涉及性能路径变化。

# 2026-06-27 P2 joined diamond return region 实现记录

本次继续推进 P2 的一般 return region 枚举。之前 `ReturnDuplicatorLow` 能处理两侧直接
汇到 return 的 diamond，也能处理一些 branch/switch wrapper，但还不能处理一个常见的
single-entry 形状：`branch -> left/right -> join -> return`。这个形状的两侧先汇到一个
普通 fallthrough join，再由 join 线性走到 closed terminal。

这次只补这个保守子集，不做 Angr 的完整 single-entry region 搜索。新逻辑要求：

- terminal 是 return 或 unreachable。
- shared tail 从 join 到 terminal 必须是 fallthrough 单后继链。
- 两条 private side 必须都是 fallthrough 单后继链，并回溯到同一个 branch head。
- 不接受 loop、direct-to-join side、nested branch side、非线性 shared tail。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:480`
  - 新增 `collectReverseFallthroughSideToBranch()`。
  - 从 join 的两个 predecessor 向上回溯 private side，要求 side block 本身是
    fallthrough 单后继，并最终回到同一个 branch head。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:536`
  - 新增 `collectJoinedDiamondReturnRegion()`。
  - 从 terminal 反推 shared tail，找到两前驱 join，再收集两个 private side，最后生成
    `Head, left side, right side, shared tail` 的 region block 顺序。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:861`
  - 在 `findLinearReturnRegion()` 中，当普通 diamond return region 不匹配时，再尝试
    joined-diamond region，并保持已有 tail-to-head prepend 约定。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5895`
  - 新增 `testReturnDuplicatorLowCopiesJoinedDiamondReturnRegion()`。
  - 构造两个 predecessor 共享 `branch -> left/right -> join -> ret`，确认原 region 被删除，
    两个 predecessor 分别得到完整 copied region，且 copied left/right 仍汇到同一 copied join。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11479`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过，`elapsed 0.98`。上一轮同口径 CrossJump 边界验证约 `elapsed 1.00`，没有明显退化。

## 影响判断

- 实现效果：3/5。补了一个真实的 P2 single-entry return region 缺口，但仍不是 Angr
  的完整 end-node region 搜索。
- 复杂度：2/5。新增一个保守收集 helper，约束写在代码里；没有改 renderer，也没有动
  shared CFG copy API。
- 维护成本：2/5。逻辑比原来的 direct diamond 多一层 join/tail 回溯，但边界清楚，
  回归测试能覆盖误删原 region 和 copied join 重接。

# 2026-06-27 P2 direct-to-join joined diamond return region 实现记录

本次继续推进 P2 的一般 return region 枚举。上一轮 joined diamond 要求 branch 的两侧
都先走 private fallthrough side 再到 shared join；这次只放开其中一侧直接从 branch
连到 join 的形状：`branch -> left -> join -> return`，另一条边是 `branch -> join`。

这个放开仍然很保守：允许一侧 direct-to-join，但不接受两侧都是 direct-to-join 的
all-direct join。后者和普通 direct diamond 的边界容易混在一起，先保留原行为，避免
ReturnDuplicatorLow 复制过宽 region。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:480-539`
  - 调整 `collectReverseFallthroughSideToBranch()`。
  - 新增 `TailHead` 参数，初始期望后继改为 join。
  - 当当前 block 是 branch，且 side 为空、branch successor 包含 join 时，把它识别为
    单侧 direct-to-join，并返回同一个 branch head。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:541-618`
  - 调整 `collectJoinedDiamondReturnRegion()`。
  - 调用 side 回溯时传入 join；检查 head successor 时允许单侧 side 为空。
  - 仍保留 `(LeftReverse.empty() && RightReverse.empty())` 拒绝 all-direct join。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5974-6049`
  - 新增 `testReturnDuplicatorLowCopiesJoinedDiamondWithDirectJoinSide()`。
  - 覆盖 `0,1 -> branch -> {left, join} -> return`，断言原 region 删除，两份 copied
    region 都保留 copied left、copied join 和 copied return payload。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6051-6083`
  - 新增 `testReturnDuplicatorLowSkipsAllDirectJoinedDiamond()`。
  - 覆盖 `{join, join}` 的 all-direct join，断言 pass 不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11590-11592`
  - 在 `main()` 中调用两个新增回归测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过，`elapsed 0.97`。上一轮同口径 joined diamond 验证约 `elapsed 0.98`，没有明显退化。
- `git -C external/NotDec-llvm2c diff --check` 通过。

## 影响判断

- 实现效果：2/5。补了 direct-to-join side 这个小缺口，但还不是 Angr 的完整
  single-entry return region 搜索。
- 复杂度：2/5。只是在已有 joined diamond helper 上增加一个直接到 join 的边界分支。
- 维护成本：2/5。新增正反两个 C++ 回归测试，能固定“单侧允许、两侧拒绝”的边界。

# 2026-06-27 P2 branch/switch wrapper joined diamond tail 实现记录

本次继续补 P2 的一般 return region 枚举。上一轮已经能从 terminal 反推
`branch -> left/right -> join -> return`，但如果这个 joined diamond 只是外层
branch/switch 的一条 closed side，`prependBranchReturnRegion()` 和
`prependSwitchReturnRegion()` 仍只能吸收线性 tail 或 direct diamond tail。

这次只补 wrapper 侧的保守适配：复用已有 `collectJoinedDiamondReturnRegion()` 识别
body 形状，再额外确认它就是当前 wrapper side 的 head，且入口 predecessor 是外层
branch/switch。没有新增更宽的 region 搜索，也没有碰 P1 merge graph 或 P4 lowered
switch 条件模型。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:175-177`
  - 前置声明 `collectJoinedDiamondReturnRegion()`，让 wrapper-tail helper 复用同一套
    joined diamond 识别逻辑。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:484-526`
  - 新增 `collectClosedJoinedDiamondReturnTail()`。
  - 扫描 closed terminal，调用 `collectJoinedDiamondReturnRegion()`，只接受
    `Region.Head == Head` 且 `Head` 的唯一 predecessor 是 `ExpectedPred` 的情况。
  - 用 `Seen` 拒绝和当前 region 已有 block 重叠的候选。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:730-738`
  - 在 `collectClosedReturnTail()` 里追加 joined-diamond tail 候选。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6379-6482`
  - 新增 `testReturnDuplicatorLowCopiesBranchWithJoinedDiamondReturnTail()`。
  - 覆盖外层 branch 的一条 side 是普通 return tail，另一条 side 是 joined diamond
    return tail，断言 copied join 和 copied return 正确重接。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6597-6706`
  - 新增 `testReturnDuplicatorLowCopiesSwitchWithJoinedDiamondReturnTail()`。
  - 覆盖外层 switch case 指向 joined diamond return tail，断言 copied switch case target
    指向 copied joined head。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11811-11814`
  - 在 `main()` 中调用两个新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过，`elapsed 0.98`。上一轮同口径 direct-to-join joined diamond 验证约 `elapsed 0.97`，
  没有明显退化。

## 影响判断

- 实现效果：2/5。补了 branch/switch wrapper 下 joined diamond return tail 的缺口，
  但仍不是 Angr 的完整 end-node region 搜索。
- 复杂度：1/5。复用已有 joined diamond region 收集，只新增 wrapper side 的入口锚定。
- 维护成本：1/5。正向回归覆盖 branch 和 switch 两个入口，逻辑不引入新的复制机制。

# 2026-06-27 P3 copied dephication 原 region 删除后再复制回归记录

本次补 P3 的 shared Phi / vvar / copied payload 覆盖，没有改运行时代码。目标是固定一个
容易退化的链路：dephication region 被复制后删除原 region，只留下 copied region；
后续再复制这份 copied region 时，shared CFG 仍要保留 copied vvar、incoming 来源链和
materialize context。

测试通过说明当前 `StructuredCFG::duplicateRegion()`、`removeBlocks()` 和
`materializeBlockBody()` 已经能支撑这个子场景。它不表示 P3 全量完成；P1/P2 继续扩大
region 时，仍要继续补更多 copied payload / Phi / vvar 组合回归。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2616-2738`
  - 新增 `testStructuredCFGDuplicateCopiedDephicationEdgeAfterOriginalRemoval()`。
  - 构造 `incoming -> dephication edge -> merge`，先复制完整 region，再删除原
    `{incoming, edge, merge}`。
  - 断言原 vvar retired、第一份 copied vvar 和 incoming 仍有效。
  - 再复制第一份 copied region，断言第二份 copied incoming 的 `SourceIncomingBlock`、
    `SourceMergeBlock`、`SourceEdgeBlock` 和 `SourceTarget` 保留上一层来源。
  - materialize 第二份 copied edge / merge，确认 dephication assignment 和 copied merge
    vvar context 都能被 hook 读到。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:11850`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。

本次只新增 C++ 回归测试，没有改运行时代码，不涉及性能路径变化。

## 影响判断

- 实现效果：1/5。补了 P3 的一个具体覆盖点，但没有新增 runtime 能力。
- 复杂度：1/5。只新增一个 shared CFG 层测试。
- 维护成本：1/5。测试直接固定 dephication 元数据链，失败时能定位到复制或删除逻辑。

# 2026-06-27 P5 shared default 后 case 边保留记录

本次继续推进 P5，只修一个边界：`SwitchDefaultCaseDuplicator` 第一阶段把 shared
default 改成 synthetic goto 后，原 switch 的 case 边可能仍然指向原 default block。
第二阶段复制 default region 时，如果把这种 switch 当普通 predecessor，会把 case
target 也重定向到 copied default，混淆 case/default 身份。

这次把第二阶段的 skip 条件改成：只要 switch 的 case 指向当前 default target，就不让
它进入 default-region copy。这样 shared default rewrite 仍然保留，case 边继续指向原
case/default overlap block；普通外部 predecessor 仍会复制 default region。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1492`
  - 复用 `switchCaseReachesBlock()` 判断 case 边是否指向 default target，删除只服务于
    旧窄条件的 `switchDefaultAlsoCaseTarget()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2427`
  - `SwitchDefaultCaseDuplicator::runOnGraph()` 在收集 `PredsToUpdate` 时，跳过 case
    指向当前 default target 的 switch predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7874`
  - 新增 `testSwitchDefaultCaseDuplicatorKeepsCaseAfterSharedDefaultRewrite()`。
  - 覆盖 shared default 先改写为 synthetic goto 后，case/default overlap switch 的
    case target 仍指向原 default block，外部普通 predecessor 仍复制 default region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12030`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p5-shared-default-case-skip.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.07 user=178.11 sys=1.54 maxrss=1270428`。和前几轮同口径
  `193.36s`、`196.30s` 相比没有退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：2/5。补了 shared default rewrite 后 case/default overlap 的一个真实边界，
  但 P5 仍没有 recovered switch / jump-table metadata。
- 复杂度：1/5。只收紧 default-region copy 的 predecessor 选择，不改复制机制。
- 维护成本：1/5。新增回归直接覆盖 case 边是否被误重定向。

# 2026-06-27 P6 copied goto edge kind 归一化记录

本次推进 P6 的质量判断一致性。`DuplicationReverter` 和 `ReturnDuplicatorLow` 会把
current goto 的 copied target 归一化到原 target，避免 copied block id 变化导致质量
判断误判。但旧逻辑只看 source 和 target，没有看 `StructuredGotoEdgeKind`。如果一个
goto 从 switch case 语义变成 default 语义，仍可能被当成同一条初始 goto。

这次只收紧 copied-target 归一化条件：source、原 target、edge kind 三者都一致时才把
copied target 还原为原 target。`NodeId` 不参与匹配，因为 structuring 后节点号本来不
稳定。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2576`
  - `hasInitialSourceGoto()` 增加 `InitialGoto.EdgeKind == Goto.EdgeKind` 判断。
  - 影响 `DuplicationReverter::getNewGotos()` 和
    `ReturnDuplicatorLow::getNewGotos()` 的 copied target 归一化。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4185`
  - 新增 `testDuplicationReverterKeepsCopiedGotoTargetsWhenEdgeKindChanges()`。
  - 覆盖初始 `SwitchCase` goto 和当前 copied target `SwitchDefault` goto 不会被归一化。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5248`
  - 新增 `testReturnDuplicatorLowKeepsCopiedGotoTargetsWhenEdgeKindChanges()`。
  - 覆盖同样的 edge kind 不一致场景。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12008`
  和 `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12036`
  - 在 `main()` 中调用两个新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-goto-edge-kind-normalize.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=155.96 user=177.31 sys=1.57 maxrss=1270180`。和上一轮
  `156.07s` 同口径，没有明显退化；过程中的 global/type warning 仍是 fortune 既有
  codegen 输出。

## 影响判断

- 实现效果：2/5。补了 copied target 质量判断里的 case/default edge kind 身份，但
  P6 仍要继续看真实样例里的 pass option 和 quality 细节。
- 复杂度：1/5。只加一个匹配条件和两个回归测试。
- 维护成本：1/5。逻辑集中在 `hasInitialSourceGoto()`，调用方无需额外分支。

# 2026-06-27 P2/P5 return switch case/default overlap 拆边记录

本次补 `ReturnDuplicatorLow` 的一个 switch 边界：同一个 switch predecessor 既通过
case 边又通过 default/non-case 边指向同一个 return region 时，旧逻辑会把整个
predecessor 当成一类边复制，容易把 case 和 default 的身份混在一起。

这次只在已有 goto 质量判断能明确区分 `SwitchCase` / `SwitchDefault` 时拆开处理：
case 只重定向 case target，default/non-case 只重定向普通 successor。缺少 edge kind
或还是 `Unknown` 时直接跳过，不猜。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2105-2176`
  - 新增 `gotoEdgeKindsFor()`，读取当前 source 到 target 的 goto edge kind 集合。
  - 新增 `copyRegionForNonCasePredecessors()`，复制 return region 后只重定向
    non-case successor。
  - 新增 `copyRegionForSwitchCases()`，复制 return region 后只重定向 switch case。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2753-2827`
  - `ReturnDuplicatorLow::runOnGraph()` 在 component 内把普通 predecessor、
    switch-case predecessor、switch-default/non-case predecessor 分开复制。
  - 同一个 switch 同时命中 case 和 non-case 时，依赖 `StructuredGotoEdgeKind`
    决定该复制哪条边；edge kind 不明确时跳过该 component。
  - 删除原 return region 的条件改为 `Region.Head` 已经没有 predecessor，避免只拆走
    其中一类边时误删原 region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5223-5322`
  - 新增 `testReturnDuplicatorLowSplitsSwitchCaseDefaultOverlapCaseOnly()`，
    覆盖只复制并重定向 case target。
  - 新增 `testReturnDuplicatorLowSplitsSwitchCaseDefaultOverlapDefaultOnly()`，
    覆盖只复制并重定向 default/non-case successor。
  - 新增 `testReturnDuplicatorLowSkipsAmbiguousSwitchCaseDefaultOverlap()`，
    覆盖 edge kind 不明确时保持原 CFG。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12136-12138`
  - 在 `main()` 中调用这三个回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-switch-overlap-return.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=159.25 user=181.76 sys=1.59 maxrss=1273400`。和上一轮
  `155.96s`、`156.07s` 同口径接近，没有明显退化；过程中的 global/type warning
  仍是 fortune 既有 codegen 输出。

## 影响判断

- 实现效果：2/5。补了 return region 在 switch case/default overlap 下的边身份保留，
  但 P2 的一般 return region 和 P5 的 switch metadata 还没完整补齐。
- 复杂度：2/5。新增了两个 return-region 专用复制 helper，但没有改通用复制入口。
- 维护成本：2/5。逻辑依赖已有 `StructuredGotoEdgeKind`，不明确时保守跳过；后续如果补
  merge graph / switch metadata，可以继续收窄或复用这条拆边路径。

# 2026-06-27 P5/P6 switch 线性 body goto edge kind 记录

本次补 `GotoManager` 的一个质量判断缺口。switch case/default 的 body 可能是
`Sequence -> BasicBlock -> Goto` 这种线性外壳，之前只有直接挂在 case/default 节点上的
goto 能保留 `SwitchCase` / `SwitchDefault`，线性 body 里的 synthetic goto 会退成
`Unknown`。这会让后续 `ReturnDuplicatorLow`、`CrossJumpReverter` 这类按 edge kind 拆边的
逻辑缺依据。

这次只让 switch edge kind 穿过 `Sequence` 下的 `BasicBlock`、`Label`、`Goto` 等线性节点；
遇到 `If`、`Switch`、loop、return/unreachable 就停止传递，避免把嵌套控制流里的普通 goto
误标成 case/default。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:28-60`
  - 新增 `preservesSwitchEdgeContext()` 和 `childEdgeKind()`。
  - 只在线性 wrapper 内保留当前 switch edge kind。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:76-79`
  - `collectGotos()` 遍历 `Children` 时使用 `childEdgeKind()`，不再无条件清成
    `Unknown`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1028-1079`
  - 新增 `testGotoManagerKeepsSwitchEdgeKindsThroughLinearBodies()`，覆盖 default body 和
    case body 都包一层 `Sequence + BasicBlock` 后，goto 仍保留 `SwitchDefault` /
    `SwitchCase`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12091`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/GotoManager.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-goto-edge-kind-linear-body.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.61 user=179.83 sys=1.51 maxrss=1269764`。和上一轮
  `159.25s` 同口径接近，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：2/5。补了 P5/P6 的一条 edge kind 传播缺口，让线性 case/default body 里的
  synthetic goto 能参与后续质量判断和拆边。
- 复杂度：1/5。只改 `GotoManager` 的上下文传递规则，没有改变 structurer 或 pass 顺序。
- 维护成本：1/5。规则只穿过线性 wrapper，边界清楚；后续如果补真正 switch metadata，
  也能继续复用这份 goto edge kind。

# 2026-06-27 P6 switch helper children 重复统计记录

本次继续补 P6 的 goto / quality 统计一致性。`GotoStructurer` 为了调试和线性展开，会把
`If` / `Switch` 的 transfer 同时放进专用字段和 `Children`。`GotoManager` 和
`ControlFlowStructureCounter` 之前会遍历两边，导致 switch case/default 的 goto 被额外算一
次，甚至多出 `Unknown` edge kind。

这次不改 structurer 输出，只收紧收集器：只有 `Sequence.Children` 参与 goto / quality
收集；`If`、`Switch`、loop 等节点走 `Then`、`Else`、`Body`、`Default` 和
`StructuredCases` 专用字段。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:62-89`
  - 新增 `collectsChildList()`。
  - `collectGotos()` 只在 `Sequence` 节点遍历 `Children`，避免 switch helper children
    和 `StructuredCases` / `Default` 重复收集。
- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:8-10`
  和 `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:52-56`
  - `ControlFlowStructureCounter` 使用同样的 `Sequence.Children` 规则，避免相对质量判断
    把同一条 goto 算两次。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1081-1116`
  - 新增 `testGotoManagerDoesNotDoubleCountSwitchHelperChildren()`。
  - 覆盖 switch helper `Children` 和 `StructuredCases` / `Default` 同时存在时，只收一次
    default goto 和一次 case goto，并保留 edge kind。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9357-9398`
  - 新增 `testControlFlowStructureCounterDoesNotDoubleCountSwitchHelperChildren()`。
  - 覆盖质量统计只给 default / case 两个目标各计一次。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12172`
  和 `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12340`
  - 在 `main()` 中调用两个新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/GotoManager.cpp lib/Structuring/StructuringQuality.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-switch-helper-child-quality.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.22 user=180.08 sys=1.47 maxrss=1274692`。和上一轮
  `157.61s` 同口径接近，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：2/5。修掉 switch helper children 导致的 goto / quality 重复统计，减少
  P6 相对质量判断里的假噪声。
- 复杂度：1/5。只改两个收集器的遍历规则，不改变 structured tree 构造。
- 维护成本：1/5。规则清楚：`Children` 只作为 `Sequence` 的顺序容器，控制节点用专用字段。

# 2026-06-27 P2/P5 single-switch case/default overlap 拆边记录

本次继续补 `ReturnDuplicatorLow` 的 switch case/default overlap。上一轮已经处理了
“同一个 switch 加另一个外部 predecessor”时的拆边，但 `StructuredCFG::predecessorsOf()`
按 block 去重；如果只有一个 switch block 同时用 default 和 case 指向同一个 return
region，旧逻辑会因为只有一个 predecessor block 直接跳过。

这次只把这种 switch 当成两条逻辑 incoming edge。后续复制和重定向仍复用已有
`StructuredGotoEdgeKind` 拆边逻辑：明确是 `SwitchCase` 时只改 case target，明确是
`SwitchDefault` 时只改 default successor，`Unknown` 仍跳过。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2116-2139`
  - 新增 `hasCaseDefaultOverlapPredecessor()` 和
    `hasMultipleLogicalPredecessors()`。
  - 说明 `predecessorsOf()` 会按 block 去重，但 case/default 同目标仍是两条逻辑边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2725-2728`
  和 `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2739-2741`
  - `ReturnDuplicatorLow::runOnGraph()` 在 region 收集和提交前检查逻辑 predecessor，
    不再只用 block 数判断是否可复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5414-5506`
  - 新增 `testReturnDuplicatorLowSplitsSingleSwitchCaseDefaultOverlapCaseOnly()`。
  - 新增 `testReturnDuplicatorLowSplitsSingleSwitchCaseDefaultOverlapDefaultOnly()`。
  - 新增 `testReturnDuplicatorLowSkipsAmbiguousSingleSwitchCaseDefaultOverlap()`。
  - 覆盖单个 switch 的 case/default 同 target 时，明确 edge kind 可拆，未知 edge kind
    保守不动。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12368-12370`
  - 在 `main()` 中调用三个新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  第一次和二进制链接并行执行，报 `Text file busy`；顺序重跑通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-single-switch-overlap-return.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.46 user=179.60 sys=1.54 maxrss=1270364`。和上一轮
  `157.22s` 同口径接近，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：2/5。补了 `ReturnDuplicatorLow` 对单 switch case/default 同 target 的拆边
  能力，但 P2 的一般 return region 和 P5 的 recovered switch metadata 仍未完成。
- 复杂度：1/5。只改 predecessor 数量判断，不改变 region copy 或 edge rewrite helper。
- 维护成本：1/5。新增逻辑只识别已有 shared CFG 能表达的 case/default overlap，未知
  edge kind 继续保守跳过。

# 2026-06-27 P2/P5 single-switch case/default both-edge 覆盖记录

本次没有改运行时代码，只补 `ReturnDuplicatorLow` 的回归覆盖。上一节实现已经覆盖单个
switch 的 case-only、default-only 和 unknown edge kind，但还缺“case 和 default 都明确
是 goto”时的断言。这个场景应该复制两份 return region，并在原 region 没有 predecessor
后删除原块。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5508-5547`
  - 新增 `testReturnDuplicatorLowSplitsSingleSwitchCaseDefaultOverlapBothEdges()`。
  - 构造单个 switch 的 default 和 case 都指向同一个 return block，同时提供
    `SwitchCase` 和 `SwitchDefault` 两条 goto。
  - 断言 default/case 分别指向不同 copied return，原 return block 被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12412`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- 本次只改测试，不改 `lib/`、`include/` 或 codegen 运行路径，所以没有重新跑 fortune
  性能 smoke；上一节 runtime 改动已经跑过同口径 fortune。

## 影响判断

- 实现效果：1/5。补齐上一节 single-switch overlap 的 both-edge 回归，不新增能力。
- 复杂度：1/5。只新增一个 C++ 测试。
- 维护成本：1/5。断言直接固定 default copy、case copy 和原 return 删除状态。

# 2026-06-27 P5/P6 CrossJump single-switch case/default both-edge 记录

本次继续补 switch case/default edge kind 在 `CrossJumpReverter` 里的消费。此前
`ReturnDuplicatorLow` 已经能在单个 switch 的 case/default 都指向同一 target 时同时拆
两条逻辑边，但 `CrossJumpReverter` 仍用 `if / else if`，同一 switch 同时带
`SwitchCase` 和 `SwitchDefault` goto 时只会复制 case 边，default 边还留在原 target。

这次只修明确 edge kind 的场景：没有 `Unknown`，且至少有 `SwitchCase` 或
`SwitchDefault` 时，case/default 可以同时进入对应复制路径；仍有 `Unknown` 时继续保守
跳过。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2979-2988`
  - `CrossJumpReverter::runOnGraph()` 在 case/default overlap 分支里允许同一个 switch
    同时加入 `CasePreds` 和 `NonCasePreds`。
  - `Unknown` 或没有明确 case/default edge kind 时仍设置 `HasAmbiguousSwitchPred` 并跳过。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3806-3859`
  - 新增 `testCrossJumpReverterSplitsSingleSwitchCaseDefaultBothEdges()`。
  - 覆盖单个 switch 的 case/default 都指向同一线性 target，且同时有
    `SwitchCase` / `SwitchDefault` goto 时，两条边分别复制，原 target/tail/exit 被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12452`
  - 在 `main()` 中调用新增回归测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p6-crossjump-single-switch-both-edge.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=155.54 user=178.43 sys=1.59 maxrss=1274612`。和上一轮
  `156.46s` 同口径接近，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：2/5。补齐 CrossJump 对 single-switch case/default both-edge 的消费，减少
  P5/P6 edge kind 已存在但 pass 没完整使用的缺口。
- 复杂度：1/5。只把互斥分支改成可同时收集 case/default，不改 copy helper。
- 维护成本：1/5。边界清楚：只接受明确 edge kind，Unknown 继续跳过。

# 2026-06-27 P2/P7 switch joined-diamond return tail smoke 记录

本次没有改运行时代码，只把 `ReturnDuplicatorLow` 已有的 switch wrapper + joined-diamond
return tail C++ 覆盖提升到 `notdec-llvm2c` 脚本层 smoke。这个形状是外层 switch 的两个
case 共享内层 switch，内层一个 case 进入 joined diamond tail，另一个 case 和 default
走普通 return tail。

这个用例仍是 P2/P7 代理覆盖，不表示 Angr 的一般 single-entry return region 枚举已经
完成。它只固定当前 shared CFG 复制能力在真实输出层不会退回 goto、phi 或 reg2mem。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:922-987`
  - 新增 `sailr_switch_joined_diamond_return_tail`。
  - 输入 IR 覆盖外层 switch 共享内层 switch，内层 switch 的 case 10 进入 joined
    diamond return tail，case 11 和 default 进入普通 return tail。
  - 断言输出包含两份内层 `switch (a)`、case 10/11、`if (a > b)` 和三个 return 值。
  - 断言不残留 `goto inner_switch`、`goto joined_head`、`goto join`、`goto joined_ret`、
    `phi` 或 `reg2mem`。

## 验证

- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改脚本测试，不改 `lib/`、`include/` 或 codegen 运行路径，所以没有重新跑 fortune
性能 smoke。

## 影响判断

- 实现效果：2/5。把 P2 的 switch joined-diamond return tail 代理形状提升到真实输出层。
- 复杂度：1/5。只新增一个 smoke case。
- 维护成本：1/5。断言集中检查 switch、return 数量和不应出现的 goto/phi/reg2mem。

# 2026-06-27 P2 branch wrapper switch return tail 实现记录

本次继续补 `ReturnDuplicatorLow` 的一般 return region 枚举。之前
`collectClosedReturnTail()` 可以把 branch、diamond、joined-diamond 当成 wrapper 里的
closed return tail，但不能把一个内层 switch 当成 closed tail。这样会漏掉：
外层 branch/switch 的一侧是普通 return tail，另一侧是一个所有分支都闭合到 return /
unreachable 的 switch return tail。

这次只补这个保守形状：switch 自身必须只有一个外部 predecessor，且 `Graph.successorsOf()`
里的每个唯一 successor 都能被现有 `collectClosedReturnTail()` 证明为闭合 return tail。
不解析 switch 条件，不做 lowered switch 恢复，也不碰 P1 merge graph。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:178-180`
  - 为后面的递归调用补 `collectClosedReturnTail()` 前置声明。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:533-570`
  - 新增 `collectClosedSwitchReturnTail()`。
  - 要求 switch 是单前驱、唯一 successor 集合非空，每个 successor 都能收成 closed
    return tail。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:783-790`
  - `collectClosedReturnTail()` 在 linear、diamond、joined-diamond 之后尝试 switch tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7049-7159`
  - 新增 `testReturnDuplicatorLowCopiesBranchWithSwitchReturnTail()`。
  - 构造外层 branch，一侧普通 return tail，另一侧内层 switch return tail，断言复制后
    copied branch、copied switch、default/case return tail 都保留来源和 payload。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12600`
  - 在 `main()` 中调用新增 C++ 回归。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:850-902`
  - 新增 `sailr_branch_switch_return_tail`。
  - 输出层断言外层 switch 两个 case 共享的 branch 被复制，内层 switch 出现两份，且不残留
    `goto outer`、`goto inner_switch`、`phi` 或 `reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p2-branch-switch-return-tail.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.71 user=179.77 sys=1.68 maxrss=1270412`。和前几轮
  `155-157s` 同口径接近，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有 codegen 输出。

## 影响判断

- 实现效果：3/5。补了一个真实 P2 single-entry return region 缺口，让 branch/switch
  wrapper 能吸收闭合 switch return tail。
- 复杂度：2/5。新增一个递归 helper，但条件保守，复用现有 `collectClosedReturnTail()`。
- 维护成本：2/5。递归路径需要注意互相调用，不过单前驱、Seen 和 closed-tail 条件限制了
  扩张范围。

# 2026-06-27 P2/P7 switch wrapper switch return tail 覆盖记录

本次没有改运行时代码，只补上一节 `collectClosedSwitchReturnTail()` 的另一条调用路径覆盖。
上一节 C++ 和 smoke 已经覆盖 branch wrapper 里吸收内层 switch return tail；但同一个 helper
也会被 `prependSwitchReturnRegion()` 使用，需要固定“外层 switch 的一个 case 进入内层
switch return tail”这个形状。

这个用例仍是 P2/P7 代理覆盖，不表示一般 single-entry return region 已完成。它只确认刚
新增的 switch-tail helper 在 switch wrapper 路径下也能被消费。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7161-7277`
  - 新增 `testReturnDuplicatorLowCopiesSwitchWithSwitchReturnTail()`。
  - 构造外层 switch，一侧普通 return tail，另一侧内层 switch return tail。
  - 断言 copied outer switch、copied inner switch、default/case return tail 都保留
    `SourceBlock` 和 payload。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12719`
  - 在 `main()` 中调用新增 C++ 回归。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:905-969`
  - 新增 `sailr_switch_switch_return_tail`。
  - 输出层断言外层 switch 共享内层 switch，内层 switch 再进入 nested switch return tail；
    输出包含两份 `switch (a)` 和两份 `switch (b)`，且不残留相关 goto、`phi` 或 `reg2mem`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/structuring_analysis_test.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改测试覆盖，不改 `lib/`、`include/` 或 codegen 运行路径，所以没有重新跑 fortune
性能 smoke。

## 影响判断

- 实现效果：2/5。补齐上一节 switch return tail helper 的 switch-wrapper 路径覆盖。
- 复杂度：1/5。只新增 C++ 和脚本 smoke。
- 维护成本：1/5。断言直接固定 copied switch、nested switch 和无 goto/phi/reg2mem 输出。

# 2026-06-28 P7 eager returns 边界检查

我单独核了 Angr 的 `test_eager_returns_simplifier_no_duplication_of_default_case`。
这条测试依赖 `ls_ubuntu_2004` 这个本地没有落盘的真实样本；在当前 workspace 里，
我只找到了 `stat.o`、`uname.o`、`fmtmsg.o` 这类 glibc 对象，没有找到同名二进制。
同时我试了几个很小的 switch proxy。它们在 NotDec 里仍会把 `50/51/52` 这类分支
打印出来，说明它们不是 Angr 这条测试的等价代理，不能拿来冒充迁移完成。

这条测试现在继续归到“需要真实样本，暂不迁移”的一类。后面如果补到同源样本，
再回头对齐 default case 的 eager returns 收口；现在不把它算进已完成项。

## 修改位置

- `logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  - 追加本节，记录 `eager_returns` 的迁移边界和当前不适合直接 proxy 的原因。

## 验证

- `rg` / `find` 检查本地样本路径。
- `notdec-llvm2c --algo=structured-sailr` 在几个临时 IR 上的输出对比。

## 影响判断

- 实现效果：1/5。只是把 Angr 测试的迁移边界写清楚。
- 复杂度：1/5。只补计划记录。
- 维护成本：1/5。后续如果补到真实样本，可以直接把本节改成实现记录。

# 2026-06-28 P4 condition compare metadata 实现记录

本次做的是 P4 `LoweredSwitchSimplifier` 的前置补强，没有实现 if-chain-to-switch。
之前 shared CFG 只有不透明 payload，后续 pass 如果想识别 `x == const` 链，只能猜
payload 文本或 payload id，不稳。现在把“条件比较了哪个值和哪个常量”记录到 shared
CFG 里，渲染仍由 payload 负责，结构恢复 pass 只读语义元数据。

这个元数据只覆盖 `==` 和 `!=`，且一边必须是整数常量。更复杂表达式先不做，避免把
P4 的范围提前扩大。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:81-93`
  - 新增 `ConditionCompareKind` 和 `ConditionCompare`，记录 compared value、constant
    value 和等于/不等于。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:247-249`
  - 新增 `StructuredCFG::conditionCompare()` 查询接口。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:267-269`
  - 新增 `StructuredCFG::setConditionCompare()` 写入接口。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:330-331`
  - 新增 `ConditionCompares` 表，按 condition payload origin 存储。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:290-316`
  - 实现查询和写入；查询时走 `payloadOrigin()`，所以复制或 materialize 后的新
    condition payload 也能查回原始比较元数据。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:38-73`
  - 新增 `conditionCompareFromICmp()`，从 LLVM `icmp eq/ne` 中提取 value/constant。
  - 支持常量在左边或右边两种写法。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:95-101`
  - 构建 conditional branch 时写入 condition compare metadata。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:173-203`
  - 从 Clang AST branch terminator 的 `BinaryOperator` 中提取 `==` / `!=` 和
    `IntegerLiteral`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:392-399`
  - C/Clang CFG 转 shared CFG 时写入 condition compare metadata。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1754-1803`
  - 新增 `testLLVMFunctionCFGBuilderRecordsConditionCompare()`。
  - 构造 `x == 7`，断言元数据存在，再复制并 materialize block，断言 copied condition
    仍能通过 origin 查到同一组比较信息。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12661`
  - 在 `main()` 中调用新增 C++ 回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-backends/Structuring/StructuredCFG.h lib/Structuring/LLVMFunctionCFGBuilder.cpp lib/Structuring/StructuredCFG.cpp lib/notdec-llvm2c/StructuredGoto.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p4-condition-compare.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=161.29 user=184.36 sys=1.61 maxrss=1276296`。和前几轮
  `155-157s` 相比略高，但仍在 fortune 这类样例的波动范围内；过程中的 global/type
  warning 仍是既有输出。

## 影响判断

- 实现效果：2/5。给 P4 降级 switch 恢复补了必要语义入口，但还没有真正合并 if-chain。
- 复杂度：2/5。新增一个小元数据表和两个 builder 提取点，没有改变 structuring 主流程。
- 维护成本：2/5。后续 P4 pass 消费这个接口即可；需要注意 copied payload 继续维护
  origin 链。

# 2026-06-28 P4 if-chain 到 shared switch 的消费实现记录

本次把上一节记录的 `ConditionCompare` 真正用起来了。目标不是做范围树，也不是把
所有 lowered branch 都强行折成 switch，而是先吃最常见的 `x == const -> x == const`
链，生成一个 shared switch，给 P4 的主线恢复补上第一条真实消费路径。

为了避免误判，这次没有碰 `!=`、循环、分支外的额外 stmt，也没有尝试回收 case/default
复用。实现只接受 `Equal`，并要求 compared value 一致、每个 compare block 只有一个
前驱、没有自己的 statements，最后一跳是 default。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:89-93`
  - `ConditionCompare` 增加 `EqualTargetIndex`，用来区分 equal 边在 branch successor
    里的位置。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:38-101`
  - 给 LLVM 侧 condition / switch case 加了本地 payload cache，避免同一个 `llvm::Value*`
    或 `ConstantInt*` 每次构建出不同 payload id。
  - `icmp eq` 记录 `EqualTargetIndex = 1`，`icmp ne` 记录 `EqualTargetIndex = 0`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:135-235`
  - 给 Clang 侧 branch 条件也加了简单复用缓存，`DeclRefExpr` 和 `IntegerLiteral`
    走稳定 payload。
  - 同步写入 `EqualTargetIndex`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1699-1815`
  - 新增 `LoweredSwitchIfCase` / `LoweredSwitchIfChain`。
  - 新增 `equalConditionCompare()`、`canConsumeLoweredSwitchIfNode()`、
    `collectLoweredSwitchIfChain()`、`rewriteLoweredSwitchIfChain()`。
  - `LoweredSwitchSimplifier::runOnGraph()` 现在先消费 branch 链，再做原有 switch case
    复用，生成的 switch 仍保持 shared CFG 约定：`Successors.front()` 是 default。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9778-9868`
  - 新增 `testLoweredSwitchSimplifierBuildsSwitchFromIfChain()`。
  - 真实用 LLVM IR 构造 `x == 7 -> x == 9` 链，断言被收成 `switch (x)`。
  - 新增 `testLoweredSwitchSimplifierSkipsIfChainWithDifferentComparedValue()`，
    断言 compared value 不一致时不会误折叠。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12880-12882`
  - 在 `main()` 中调用新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-backends/Structuring/StructuredCFG.h lib/Structuring/LLVMFunctionCFGBuilder.cpp lib/Structuring/SAILRDeoptimization.cpp lib/notdec-llvm2c/StructuredGoto.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p4-condition-compare.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.56 user=180.06 sys=1.63 maxrss=1272652`。和前几轮
  `155-157s` 基本一致，没有明显退化；过程中的 global/type warning 还是 fortune 既有输出。

## 影响判断

- 实现效果：3/5。P4 终于从“有元数据”变成“能把最常见的 if-chain 折成 switch”。
- 复杂度：3/5。多了两个小缓存和一个新收集路径，但范围还算窄。
- 维护成本：3/5。后续如果继续扩 `!=`、range tree、shared default 复用，要继续守住
  `EqualTargetIndex` 和 copied payload 的 origin 规则。

# 2026-06-28 P4/P5 `!=` if-chain 与 switch 边分类记录

本次继续推进 P4 和 P5 的 shared CFG 语义。P4 方面，`LoweredSwitchSimplifier`
之前虽然有 `ConditionCompareKind::NotEqual` 元数据，但实际 if-chain 消费路径只接受
`Equal`，所以 `x != c ? next : case` 这种常见 lowered switch 形状仍不会恢复成
shared switch。现在改成按 `EqualTargetIndex` 选择 case 边，不再限制条件种类。

P5 方面，之前各 pass 分别用 `Cases`、`Successors.front()` 和局部 overlap 判断来区分
case/default 边。现在在 `StructuredCFG` 增加统一的 `SwitchEdgeKind` 查询入口，能明确
返回 default-only、case-only、case/default overlap 或 unknown。当前先把
`ReturnDuplicatorLow` / `CrossJumpReverter` 相关的共享 helper 收到这个入口上，还没有接入
Angr 那种真实 jump-table metadata。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:69-77`
  - 新增 `SwitchEdgeKind`，表达 default-only、case-only、default/case overlap 和
    unknown。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:102-105`
  - 补充 `ConditionCompare::EqualTargetIndex` 注释，说明 `ne` 时该字段指 false/equal
    successor。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:296`
  - 新增 `StructuredCFG::switchEdgeKind()` 查询接口。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:17-34`
  - 新增 `switchEdgeKindForBlock()`，统一从 `Successors.front()` 和 `Cases` 判断 switch
    边身份。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:617-623`
  - 实现 `StructuredCFG::switchEdgeKind()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1574-1586`
  - 新增 `hasSwitchCaseEdge()` / `hasSwitchDefaultEdge()`，作为 SAILR pass 内部共享入口。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1713-1810`
  - `LoweredSwitchSimplifier` 的 condition 收集从 `equalConditionCompare()` 改为
    `matchedConditionCompare()`，不再拒绝 `ConditionCompareKind::NotEqual`。
  - case target 仍统一使用 `EqualTargetIndex`，重复 case value 仍按 payload origin
    去重。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2314-2355`
  - `blockUsesSwitchCaseEdge()`、`blockUsesNonSwitchCaseEdge()` 和
    `hasCaseDefaultOverlapPredecessor()` 改为使用 `switchEdgeKind()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2339-2352`
  - 新增 `testStructuredCFGClassifiesSwitchEdges()`，覆盖 case-only、case/default
    overlap 和 unknown。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9858-9920`
  - 新增 `testLoweredSwitchSimplifierBuildsSwitchFromNotEqualIfChain()`，用真实 LLVM IR
    构造 `x != 7 -> x != 9` 链，断言恢复成 shared switch。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12870`
  和 `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12984`
  - 在测试入口注册新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-backends/Structuring/StructuredCFG.h lib/Structuring/StructuredCFG.cpp lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-p4-ne-switch-edge.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=156.90 user=179.48 sys=1.80 maxrss=1272392`。和前几轮
  `155-157s` 同口径基本一致，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有输出。

## 影响判断

- 实现效果：3/5。补上了 P4 的 `!=` if-chain 消费，也给 P5 的 default/case/overlap
  判断提供了共享入口；但 range tree 和 recovered switch / jump-table metadata 仍未完成。
- 复杂度：2/5。新增一个小枚举和查询接口，`LoweredSwitchSimplifier` 只放开已有
  `ConditionCompare` 的消费范围。
- 维护成本：2/5。后续 switch pass 可以复用 `switchEdgeKind()`，但如果接入真实
  jump-table metadata，需要继续确认它和现有 `Successors/Cases` 约定是否一致。

# 2026-06-28 P4 range condition metadata 记录

本次只补 P4 的前置元数据，让 shared CFG 能记录 `> >= < <=` 这类 range compare。
`LoweredSwitchSimplifier` 仍只消费 `==` / `!=` if-chain，不把 range compare 折成
switch。range-tree lowered switch 恢复还没实现。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:91-114`
  - `ConditionCompareKind` 增加 `GreaterThan`、`GreaterEqual`、`LessThan`、
    `LessEqual`。
  - `ConditionCompare` 增加 `TrueTargetIndex`，range compare 的 `EqualTargetIndex`
    暂时等于 true successor。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:66-148`
  - `conditionCompareFromICmp()` 支持 LLVM signed/unsigned range predicate。
  - 常量在左侧时通过 `swappedConditionCompareKind()` 翻转比较方向。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:206-284`
  - `conditionCompareFromBranchCondition()` 支持 Clang AST 的 `>`、`>=`、`<`、`<=`。
  - 保持 Clang CFG true edge 为 successor 0 的约定。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1721-1733`
  - `matchedConditionCompare()` 显式拒绝 range compare，避免当前 switch 恢复误吃。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1754-1863`
  - 补 `TrueTargetIndex` 断言。
  - 新增 `testLLVMFunctionCFGBuilderRecordsRangeConditionCompare()`，覆盖 range
    compare 和常量左侧时的方向翻转。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10025-10069`
  - 新增 `testLoweredSwitchSimplifierSkipsRangeConditionCompare()`，确认 range
    compare 不会被当前 lowered-switch 恢复消费。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- include/notdec-backends/Structuring/StructuredCFG.h lib/Structuring/LLVMFunctionCFGBuilder.cpp lib/Structuring/SAILRDeoptimization.cpp lib/notdec-llvm2c/StructuredGoto.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-range-condition.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=163.14 user=185.07 sys=1.59 maxrss=1272840`。和前次
  `elapsed=156.90 user=179.48 sys=1.80 maxrss=1272392` 同口径接近，没有明显退化。

## 影响判断

- 实现效果：2/5。只补 range compare 元数据，为后续 range-tree switch 恢复铺路。
- 复杂度：2/5。新增枚举值和少量 predicate 映射，没有改恢复算法。
- 维护成本：2/5。后续实现 range-tree 时要继续复用 `TrueTargetIndex`，不要从 payload
  文本猜分支方向。

# 2026-06-28 P4 range guard if-chain 消费记录

本次只处理一层简单 range guard 包住 equality/inequality if-chain 的形状，例如
`x <= 9 ? (x == 7 ? case7 : x == 9 ? case9 : default) : default`。只有能证明所有
case 常量都落在 guard 允许范围内时，才把 guard 和内部 compare 链一起改成 shared
switch。case 超出 guard 时整体跳过，不做半截恢复。没有实现一般 range-tree switch
恢复。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:100-112`
  - `ConditionCompare` 增加 64 位以内整数常量值和 signed/unsigned predicate 标记，
    供 range guard 判断使用；payload 仍负责渲染。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:107-159`
  - LLVM IR builder 记录 signed/unsigned predicate 和整数常量值，宽度超过 64 位时
    不给 range 消费。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:273-287`
  - Clang CFG 入口同步填充整数常量值，保持 true edge 为 successor 0 的约定。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:177-180`
  - `LoweredSwitchIfCase` 保存原始 `ConditionCompare`，后续用它判断 case 常量是否
    符合 range guard。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1742-1841`
  - 新增 range compare 匹配和 case-in-range 判断，按 guard 的 signed/unsigned
    语义比较。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1843-1962`
  - 普通 if-chain 收集遇到外层 range guard 时默认跳过，避免把受 guard 限制的链条
    单独恢复成可能误导后续 pass 的 switch。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1965-2011`
  - 新增 `collectRangeGuardedLoweredSwitchIfChain()`，只消费 default 边一致且全部 case
    都满足 guard 的一层形状。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2753-2758`
  - `LoweredSwitchSimplifier` 先尝试 range-guarded 形状，再回到普通 if-chain。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1818-1895`
  - range metadata 测试补 unsigned predicate 和整数值断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10016-10076`
  - 新增正例，确认 `x <= 9` 包住 `x == 7 / x == 9` 能恢复成 switch。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10175-10227`
  - 新增反例，`x <= 8` 包住 `x == 7 / x == 9` 时不消费 guard，也不折内部链。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13252-13257`
  - 注册新增回归。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:259-286`
  - Clang AST 分支条件的整数元数据也改成只接受 64 位以内常量，避免宽整数直接
    进入 `getSExtValue()` / `getZExtValue()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1807-1908`
  - `testLLVMFunctionCFGBuilderRecordsRangeConditionCompare()` 补宽整数 case，确认 128 位
    常量不会写入可比较整数值。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- include/notdec-backends/Structuring/StructuredCFG.h lib/Structuring/LLVMFunctionCFGBuilder.cpp lib/notdec-llvm2c/StructuredGoto.cpp lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- 这次补丁后又重新跑了一次 `cmake --build ./build --target structuring-analysis-test -j4`
  和 `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`，都通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-range-guard.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=160.53 user=182.55 sys=1.52 maxrss=1271812`。和前两轮
  `156.90s`、`163.14s` 同口径接近，没有明显退化；过程中的 global/type warning
  仍是 fortune 既有输出。
- 宽整数元数据补丁后又跑了一次 fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-range-metadata-width.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=158.55 user=181.10 sys=1.65 maxrss=1269316`，仍在近期
  `156-163s` 范围内。

## 影响判断

- 实现效果：3/5。补上一个常见 guarded lowered-switch 形状，但还不是一般 range-tree
  恢复。
- 复杂度：3/5。新增整数元数据和一层 guard 收集逻辑，范围仍控制在
  `LoweredSwitchSimplifier` 内。
- 维护成本：2/5。规则保守，后续如果要做更完整 range-tree，需要重新设计多 guard
  合取和 case 覆盖判断。

# 2026-06-28 P2/P6 调用数复制成本实现记录

本次把 Angr 的 call-count 复制成本 guard 接到 shared CFG。之前只用 statement 数限制
复制，`ReturnDuplicatorLow` 会漏掉 Angr 的 `max_calls_in_regions=2`，`CrossJumpReverter`
也只补过 statement 边界测试，没有真正实现 `max_call_duplications=1`。这次不改复制策略，
只让已有 return-region 和 cross-jump 复制在调用数过高时跳过。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:225-228`
  - `CFGBlock` 新增 `CallCount`，记录 backend-neutral 的调用成本。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:181-189`
  - `LLVMFunctionCFGBuilder::build()` 统计 LLVM `CallBase` 指令。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:97-115`
  - 新增 `CallExprCounter` 和 `callExprCount()`，递归统计 Clang AST 的 `CallExpr`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:475-482`
  - `StructuredGotoAdapter::buildCFG()` 写入每个 shared block 的 `CallCount`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:431-435` 和 `:554-559`
  - copied block materialize 时同步复制 body block 的 `CallCount`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:34-56`
  - `ReturnDuplicatorLow` 增加 `MaxDuplicatedCalls=2`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:129-145`
  - `CrossJumpReverter` 增加 `MaxDuplicatedCalls=1`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2636-2660`
  - 新增 `callCountInRegion()`，分别统计 `LinearRegion` 和 `ReturnRegion`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:3163-3192`
  - `ReturnDuplicatorLow::runOnGraph()` 超过调用数上限时跳过复制。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:3332-3363`
  - `CrossJumpReverter::runOnGraph()` 超过调用数上限时跳过复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1929-1962`
  - 新增 `testLLVMFunctionCFGBuilderRecordsCallCount()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3600-3630`
  - 新增 `testCrossJumpReverterSkipsCallHeavyLinearGotoTarget()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5493-5525`
  - 新增 `testReturnDuplicatorLowSkipsCallHeavyReturnRegion()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13275-13320`
  - 在 `main()` 中注册三条新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过，ninja 显示 no work to do。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-call-count.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=161.03 user=183.38 sys=1.54 maxrss=1268336`。仍在近期
  `156-163s` 范围内，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有输出。

## 影响判断

- 实现效果：3/5。补齐两个 Angr 成本 guard，但还没有把 pass 选项完整暴露到 CLI。
- 复杂度：2/5。只新增一个 shared block 计数字段和两个 pass gate，不改复制流程。
- 维护成本：2/5。后续新 backend 如果走 shared CFG，需要记得填 `CallCount`，否则会
  回到只按 statement 数估算。

# 2026-06-28 P4 嵌套 range guard if-chain 消费记录

本次继续推进 P4，但仍不做完整 range-tree。只补一个保守形状：多个空语句 range guard
线性包住同一条 `==` / `!=` if-chain，并且每层 guard 的非 chain 分支都指向同一个
default。所有 case 都必须落在每个 guard 允许的分支里；default 不一致时只允许内层
guard 自己恢复成 switch，不跨外层 guard。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1784-1788`
  - 新增 `hasOnlyPredecessor()`，避免消费仍被别的入口使用的内层 guard 或 if-chain head。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1881-1896`
  - 新增 `loweredSwitchChainFitsRangeGuard()`，统一校验 compared value、default target
    和 guard 覆盖关系。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1989-2048`
  - `collectRangeGuardedLoweredSwitchIfChainFrom()` 递归消费线性 range guard 链；遇到
    不同 default、重复 guard、非单前驱头或 case 不在范围内时保守跳过。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10207-10274`
  - 新增 `testLoweredSwitchSimplifierBuildsSwitchFromNestedRangeGuardedIfChain()`，覆盖
    `x >= 7 && x <= 9` 包住 `x == 7 / x == 9` 的恢复。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10427-10495`
  - 新增 `testLoweredSwitchSimplifierKeepsOuterRangeGuardWithDifferentDefault()`，确认
    default 不一致时不跨外层 guard。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13524-13531`
  - 注册新增回归。
- `logs/20260627-02-sailr-angr-remaining-parity-plan.md:266-270`
  - 更新 P0 对照表里 `ReturnDuplicatorLow` 和 `CrossJumpReverter` 已有 call-count guard
    的状态，避免继续把它当未实现项。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j1` 通过，显示 no work to do。
  注意：此前和 `notdec` 并行构建时，`notdec-llvm2c-exe -j4` 出过一次
  `StructuredGoto::execute()` 未定义链接错误；`notdec` 完成后顺序重跑正常，判断为
  既有并行构建顺序问题，不是本次算法改动。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-nested-range-guard.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=160.63 user=182.81 sys=1.50 maxrss=1269300`。仍在近期
  `156-163s` 范围内，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有输出。

## 影响判断

- 实现效果：3/5。补上常见双边界 guard 包住 equality chain 的形状，但还不是 Angr
  的完整 range-tree。
- 复杂度：2/5。只把一层 guard 收集扩展成线性 guard 链，不改 shared switch 表示。
- 维护成本：2/5。规则保守，后续如果做一般 range-tree，需要重新设计多分支 guard
  合取和 default 覆盖判断。

# 2026-06-28 P4 lowered switch 共享 target 过滤记录

本次继续推进 P4 的安全边界，不扩展完整 range-tree。Angr 的
`LoweredSwitchSimplifier` 会过滤 case/default target 还被链外节点进入的候选，避免把普通
共享块误当 switch case。NotDec 之前从 lowered if-chain 恢复 switch 时缺这个过滤。

同时补一个 nested range guard 的候选顺序保护：内层 equality if-chain 如果已经被外层
range guard 覆盖，就不能先单独恢复成 switch；否则外层 guard 没机会整体消费。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1867-1908`
  - 新增 `rangeGuardCoversChainCases()`，并让 `loweredSwitchIfChainHasRangeGuard()`
    沿前驱向上检查 range guard，避免内层 chain 抢先恢复。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1911-1948`
  - 新增 `loweredSwitchTargetsOnlyReachedFromChain()`，要求 case/default target 只由
    当前 comparison chain 进入；只放行 source 在 chain 内的 synthetic goto/forwarder。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2033-2040`
  - 普通 if-chain 恢复前应用共享 target 过滤。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2103-2108`
  - range-guarded if-chain 最终恢复前也应用同一过滤。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10276-10337`
  - 新增 `testLoweredSwitchSimplifierSkipsInnerChainWhenRangeGuardDefaultDiffers()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10380-10485`
  - 新增共享 case target 和共享 default target 两个负例。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13698-13701`
  - 注册新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp lib/Structuring/SAILRStructurer.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-shared-target.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=161.65 user=183.49 sys=1.62 maxrss=1266544`。仍在近期
  `156-163s` 范围内，没有明显退化；过程中的 global/type warning 仍是 fortune
  既有输出。

## 影响判断

- 实现效果：2/5。补了 Angr lowered switch 候选过滤的一条安全边界，但没有解决
  duplicated default terminal 的等价判断，也不是完整 range-tree。
- 复杂度：2/5。只新增 target 前驱过滤和 range guard 祖先检查，不改 switch rewrite。
- 维护成本：2/5。规则保守；后续如果要接受 duplicated default，需要先让 shared CFG
  能可靠表达 return value 或 default body 等价。

# 2026-06-28 P4/P5 lowered switch 启发式和 default target 记录

本次继续推进 P4，并顺手修掉 smoke 暴露出的 P5/C 输出语义问题。

P4 方面，对齐 Angr `LoweredSwitchSimplifier` 的低误判过滤：如果 case 常量都有整数
元数据，就统计连续 case run。连续 run 达到 6 个时不恢复；所有 case 都连续且当前图里
没有已有 switch 时也不恢复；case body distinct target 少于 2 且没有已有 switch 时跳过。
缺整数元数据时保留旧行为，避免靠 payload 文本猜。

P5/renderer 方面，发现 lowered if-chain 被恢复成 shared switch 后，结构树里的 default
target 来自重写后的候选 CFG，但 C renderer 仍从原始 shared block 猜 default target。
这会把 `case 7` 和 `default` 误串成同一个 body，输出 `case 7: default: return 0;`。
本次给 `StructuredNode` 增加 `DefaultTarget`，由 structurer 构造 switch 节点时写入，
renderer 优先消费这个字段。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:400-403`
  - `StructuredNode` 新增 `DefaultTarget`，保存结构树层的 switch default CFG target。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:97`
  - fallback switch 节点写入 `DefaultTarget`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:576,1487,1898`
  - Phoenix switch reduction、virtualized switch source、fallback switch 节点都写入
    `DefaultTarget`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:53-57`
  - `SwitchBodyLabel` 明确只保存当前 body 组最后一个 label，后面再绑定 body。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:706-767`
  - C renderer 渲染 structured switch 时优先使用 `Node.DefaultTarget`，只在旧树没有
    该字段时回退到原始 shared block successor；同时按出现顺序记录 case/default
    body 组，避免再用 `std::map` 顺序绑定 label body。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:192`
  - 新增 `MaxLoweredSwitchContinuousCases = 6`，对应 Angr 的默认阈值。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1953-2038`
  - 新增 lowered switch case 整数收集、连续 run 统计和 Angr 启发式过滤。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2959-2979`
  - `LoweredSwitchSimplifier::runOnGraph()` 在 if-chain rewrite 前应用启发式过滤。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:143-167`
  - 新增 `lowered_if_chain_switch` 脚本级 smoke，要求 case 7、case 9 和 default 都输出
    正确 body。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:185-214`
  - 新增 `findFirstNodeKind()`，用于检查结构树中的 switch 节点。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10113-10160`
  - 新增 `testSAILRStructurerKeepsLoweredSwitchDefaultTarget()`，固定 lowered switch
    default target 不再从原始 block 猜。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10226-10273`
  - 新增 `testLoweredSwitchSimplifierSkipsContinuousIfChainWithoutSwitchHint()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10275-10343`
  - 新增 `testLoweredSwitchSimplifierAcceptsContinuousIfChainWithSwitchHint()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13895-13898`
  - 注册新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j1` 通过。期间遇到一次本地
  build 目录里的 `libnotdec-llvm2c.a` 只剩 `ASTManager.cpp.o` 的损坏产物；确认
  `StructuralAnalysis.cpp.o` 仍有 `decompileModule` 符号后，删除该 `.a` 让 ninja
  重新归档，随后通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-render.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=161.87 user=184.81 sys=1.60 maxrss=1267276`。和上一轮
  `161.65s` 同口径，没有明显退化；过程中的 global/type warning 仍是 fortune 既有输出。

## 影响判断

- 实现效果：3/5。P4 增加了 Angr 的关键误判过滤，P5 修掉 lowered switch default
  target 在 renderer 侧被猜错的问题；但仍没有完整 range-tree 和 recovered switch metadata。
- 复杂度：2/5。新增一个结构树字段和几个小 helper，不改变 CFG copy 主流程。
- 维护成本：2/5。后续所有新 switch structurer 分支需要写 `DefaultTarget`；已有分支已覆盖。

# 2026-06-28 P4 lowered switch default 回流过滤记录

本次继续补 Angr `LoweredSwitchSimplifier` 的保守过滤。Angr 会拒绝 default case 在同一
region 里回到其他 case/comparison 节点的候选；NotDec 之前只检查 case/default target
是否被链外节点进入，没有检查 default target 自己能否沿 CFG 回到 comparison chain。

这次只补这条过滤：如果 default target 能到达当前 lowered if-chain 的 head 或被移除的
comparison block，就不把这条 if-chain 恢复成 switch。这样避免 rewrite 删除 comparison
block 后改变循环/回流语义。仍不处理 duplicated default 等价和完整 range-tree。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1953-1974`
  - 新增 `loweredSwitchDefaultReachesChain()`，从 default target 做 reachability，命中
    `Chain.Head` 或 `RemovedBlocks` 时拒绝候选。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2150-2156`
  - 普通 lowered if-chain 收集完成后应用 default 回流过滤。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2219-2224`
  - range-guarded lowered if-chain 也应用同一过滤。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10622-10661`
  - 新增 `testLoweredSwitchSimplifierSkipsDefaultThatReachesIfChain()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13943-13946`
  - 注册新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-default-cycle.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=165.63 user=188.74 sys=1.77 maxrss=1273132`。和上一轮
  `161.87s` 同口径，没有明显退化；过程中的 global/type warning 仍是 fortune 既有输出。

## 影响判断

- 实现效果：2/5。补上 Angr lowered switch default 回流过滤，但 P4 仍缺完整 range-tree
  和 duplicated default 等价。
- 复杂度：1/5。只新增一个 reachability 检查和一个负例。
- 维护成本：1/5。规则保守；如果后续要允许某些结构化循环回流，需要先证明 rewrite 后
  不会删除仍需要的 comparison block。

# 2026-06-28 P4 lowered switch all-ones sentinel 过滤记录

本次继续补 Angr `LoweredSwitchSimplifier` 的低误判过滤。Angr 在 `eq` 链收集时遇到
`0xffffffff` 或 `0xffffffffffffffff` 会放弃候选，因为这常见于先判断函数返回值 `-1`
再进入其他控制流的形状，不一定是 switch case。

NotDec 已有 `ConditionCompare::UnsignedIntegerValue`，所以这次不新增 bitwidth 或 payload
解析，只按 Angr 的两个 unsigned sentinel 值过滤。缺整数元数据的 case 仍保持之前的
保守行为，不靠文本猜。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:193-196`
  - 新增 i32/i64 all-ones sentinel 常量。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2019-2032`
  - 新增 `hasAllOnesSwitchSentinelCase()`，扫描 lowered switch case 的整数元数据。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2052-2058`
  - `loweredSwitchChainPassesAngrHeuristics()` 在连续 case 和 distinct target 过滤前
    先拒绝 all-ones sentinel。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10226-10283`
  - 新增 `testLoweredSwitchSimplifierSkipsAllOnesSentinelIfChain()`，用真实 LLVM i32
    `-1` 比较确认 `UnsignedIntegerValue == 0xffffffff`，并确认 if-chain 不被改写。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:13994-13998`
  - 注册新增回归。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-sentinel.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=162.97 user=185.16 sys=1.70 maxrss=1269056`。和上一轮
  `165.63s` 同口径，没有明显退化；过程中的 global/type warning 仍是 fortune 既有输出。

## 影响判断

- 实现效果：2/5。补上 Angr 的 all-ones sentinel 误判过滤；P4 仍缺完整 range-tree 和
  duplicated default 等价。
- 复杂度：1/5。只增加一个常量过滤 helper 和一个 LLVM IR 回归。
- 维护成本：1/5。规则很窄；如果后续发现非 i32/i64 sentinel，需要先对照 Angr 再扩展。

# 2026-06-28 P7 lowered switch default 回流 smoke 记录

本次只把上一节 P4 的 default 回流过滤提升到 `notdec-llvm2c` 脚本层 smoke，确认真实
IR 输入里 default 分支回到 comparison chain 时，不会被输出成 `switch`。算法代码和
C++ 单测已在前一节记录，这里不重复算一次实现。

这个用例不表示 P4 或 P7 完成。P4 仍缺完整 range-tree、duplicated default 等价和
recovered switch / jump-table metadata；P7 仍要继续迁移更多 Angr 真实样例。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:166-191`
  - 新增 `lowered_if_chain_default_cycle`，构造 `default -> check9` 的回流形状。
  - 断言输出包含两个真实 case return，同时不包含 `switch (x)`、`case 7:`、`case 9:`。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- test/structuring/run_structuring_smoke.py`
  通过。
- `git diff --check -- logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：1/5。只把 default 回流边界升到输出层 smoke；P4/P7 仍未完成。
- 复杂度：1/5。只新增一个内联 IR smoke case。
- 维护成本：1/5。规则保守；smoke 只看“不恢复成 switch”的核心边界，后续输出局部
  排版变化不应影响。

# 2026-06-28 P7 lowered switch sentinel smoke 记录

本次不改算法，只把上一节 P4 的 all-ones sentinel 过滤提升到 `notdec-llvm2c` 脚本层
smoke。用例构造 `x == -1` 后继续检查 `x == 7` 的 if-chain，确认真实 IR 输出不会
把这个错误/特殊返回值检查恢复成 `switch`。

这个用例不表示 P4 或 P7 完成。P4 仍缺完整 range-tree、duplicated default 等价和
recovered switch / jump-table metadata；P7 仍要继续迁移更多 Angr 真实样例。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:195-217`
  - 新增 `lowered_if_chain_all_ones_sentinel`，断言输出保留 `return -1;`、
    `return 7;`、`return 0;`，并且不出现 `switch (x)`、`case -1:`、`case 7:`。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- test/structuring/run_structuring_smoke.py`
  通过。
- `git diff --check -- logs/20260627-02-sailr-angr-remaining-parity-plan.md`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：1/5。只把 sentinel 安全边界升到输出层 smoke；P4/P7 仍未完成。
- 复杂度：1/5。只新增一个内联 IR smoke case。
- 维护成本：1/5。断言只看“不恢复成 switch”的核心边界，后续输出局部排版变化不应影响。

# 2026-06-28 P7 migration 覆盖补充记录

本次不改算法，只扩展 `run_sailr_bench2_migration.py` 的迁移覆盖。之前脚本只有 5 个
真实/半真实样例，P2/P3 的 copied Phi/vvar return region 主要还停在 smoke 脚本里。
这次把已有稳定 proxy 升到 migration 层，让这个脚本同时覆盖：

- `ReturnDuplicatorLow` 复制 branch return region。
- `ReturnDuplicatorLow` 在 angr dephication 模式下复制带 Phi/vvar payload 的 return tail。
- `LoweredSwitchSimplifier` 对 default 回流 comparison chain 的安全过滤。

这些仍是 proxy，不表示 P2/P3/P4/P7 完成。P7 后续还要继续补真实样例分类，尤其是
`DuplicationReverter` merge graph 和 recovered switch / jump-table metadata。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:61`
  - 新增 `branch_return_region_proxy`，代理 Angr abnormal switch case 里
    `ReturnDuplicatorLow` 复制 branch return region 后不能泄漏 `goto` / `phi` /
    `reg2mem` 的要求。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:105`
  - 新增 `copied_return_tail_dephication_proxy`，用
    `--sailr-dephication-mode=angr` 覆盖 copied Phi/vvar payload 输出。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:147`
  - 新增 `lowered_switch_default_cycle_regression`，代理 Angr Dogbolt lowered switch
    无限循环回归，确认 default 回流时不恢复成 switch。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:188`
  - `run_case()` 支持 per-case `args` 和 `counts` 断言。

## 验证

- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

## 影响判断

- 实现效果：1/5。迁移脚本覆盖从 5 个样例扩到 8 个样例，P2/P3/P4 的代表性更好；
  但还没有增加新的算法能力。
- 复杂度：1/5。只给脚本增加 per-case 参数和计数断言。
- 维护成本：1/5。新增用例复用已有 smoke 中稳定形状，断言只看核心结构和 payload。

# 2026-06-28 P4 copied default range guard 实现记录

本次补 `LoweredSwitchSimplifier` 的一条窄边界：range guard 的 default 边和内层
if-chain 的 default 边如果不是同一个 block id，但二者是 copied/original 同源块，仍然
可以按 Angr 的 duplicated default 形状恢复成 switch。之前 NotDec 要求两个 default
target 的 id 完全相同，会漏掉前置 pass 已经复制 default block 的情况。

这不是完整 range-tree，也没有接入 recovered switch / jump-table metadata。只复用 shared
CFG 已有的 copied block identity，并且只有 copied default 没有链外前驱时才删除它。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1854`
  - `loweredSwitchChainFitsRangeGuard()` 用 `sameBlockReference()` 判断 guard default
    和 chain default 是否同源，不再只比较 block id。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1940`
  - 新增 `blockOnlyReachedFrom()`，用于确认要删除的 copied default 只被当前 guard /
    comparison chain 使用。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2237`
  - `collectRangeGuardedLoweredSwitchIfChainFrom()` 在 default 同源但 id 不同时，把最终
    switch default 改成 guard default，并把无链外前驱的 chain default 记入删除列表。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:580`
  - 新增 `copiedBlock()` 测试 helper，构造 shared CFG copied/original 同源块。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10609`
  - 新增 `testLoweredSwitchSimplifierMergesCopiedRangeGuardDefault()`，确认 copied default
    被合并并删除，最终 switch 使用 guard default。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10681`
  - 新增 `testLoweredSwitchSimplifierSkipsSharedCopiedRangeGuardDefault()`，确认 copied
    default 有链外前驱时不重写，避免删共享块。

## 验证

- `git -C external/NotDec-llvm2c diff --check` 通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-copied-default.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=157.90 user=180.07 sys=1.60 maxrss=1269496`。和近期
  `158s-163s` 同口径，没有明显退化；输出中的 global/type warning 仍是 fortune 既有输出。

## 影响判断

- 实现效果：2/5。补上 range-guarded lowered switch 的 copied default 等价边界；
  P4 仍缺完整 range-tree，P5 仍缺 recovered switch / jump-table metadata。
- 复杂度：1/5。只复用已有 copied block identity，删除前加链外前驱保护。
- 维护成本：1/5。规则很窄，测试同时覆盖可合并和必须跳过两种情况。

# 2026-06-28 P4 duplicate case value 过滤记录

本次继续补 `LoweredSwitchSimplifier` 的误判过滤。Angr 在收集 lowered switch case 后会
拒绝重复 case value；NotDec 之前只按 payload origin 去重。两个不同 payload id 如果
都代表整数 `7`，仍可能被误恢复成带重复 case 的 switch。

这次在已有 `ConditionCompare` 整数元数据存在时优先按 unsigned integer value 去重；
缺整数元数据时继续沿用 payload origin 去重。该改动只发生在候选收集阶段，不改变
switch rewrite 路径。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2120`
  - `collectLoweredSwitchIfChain()` 增加 `IntegerCaseValues`，有整数元数据时按整数值
    拒绝重复 case。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2147`
  - case 收集时先检查整数重复；没有整数值时才回退到原来的 payload origin 去重。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10787`
  - 新增 `testLoweredSwitchSimplifierSkipsDuplicateIntegerCaseValue()`，构造两个不同
    case payload 但整数值都为 `7` 的 if-chain，确认不恢复 switch。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14200`
  - 注册新增测试。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `cmake --build ./build --target notdec-llvm2c -j4` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `cmake --build ./build --target notdec -j4` 通过。
- fortune 性能 smoke：
  `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-duplicate-case.c --tr-level=2 --algo=structured-sailr`
  退出码 0，`elapsed=158.57 user=180.78 sys=1.58 maxrss=1270984`。和近期
  `157s-163s` 同口径，没有明显退化；输出中的 global/type warning 仍是 fortune 既有输出。

## 影响判断

- 实现效果：2/5。补上 Angr duplicate case value 过滤；P4 仍缺完整 range-tree，
  P5 仍缺 recovered switch / jump-table metadata。
- 复杂度：1/5。只增加一个收集期去重集合。
- 维护成本：1/5。整数元数据来自 LLVM builder，缺失时仍走原保守路径。

# 2026-06-28 P7 switch default/reused-entry migration 覆盖记录

本次不改算法，只把 P5 的两个 shared CFG 行为补到 `run_sailr_bench2_migration.py`：
shared default 和 reused case entry。之前 migration 脚本主要覆盖 ReturnDuplicatorLow、
CrossJumpReverter 和 LoweredSwitchSimplifier，P5 只在 C++ 单测和 smoke 里有覆盖。

这两个用例仍是 proxy，不表示 P5 完成。P5 仍缺 recovered switch / jump-table metadata，
以及真实样例分类。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:174`
  - 新增 `switch_shared_default_proxy`，代理 Angr
    `test_switch_case_header_mismatch_caused_by_cmovs` 类的 shared default 行为。
    断言输出保留两个 `switch (x)`，并通过 `goto structured_block_` 表达共享 default。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:236`
  - 新增 `switch_reused_entry_proxy`，代理 Angr
    `test_decompiling_reused_entries_between_switch_cases` 类的 reused case entry 行为。
    断言输出保留两个 `switch (x)`，并对共享 case entry 生成 goto。

## 验证

- `git -C external/NotDec-llvm2c diff --check -- test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 8 个样例扩到 10 个样例，并补上 P5 代表行为；
  但没有新增算法能力。
- 复杂度：1/5。只新增两个内联 IR case。
- 维护成本：1/5。断言集中在 switch 数量、case label 和 goto 形状，避免依赖局部块编号。

# 2026-06-28 P7 lowered switch sentinel migration 覆盖记录

本次不改算法，只把 P4 已有的 all-ones sentinel 安全边界补到
`run_sailr_bench2_migration.py`。之前这个边界只在 C++ 和 smoke 层覆盖；
migration 脚本还缺一个对应 Angr lowered switch 误识别保护的代理样例。

这不是完整 range-tree 恢复。今天尝试过 range-tree 测试形状，但
`testLoweredSwitchSimplifierBuildsSwitchFromRangeTree()` 仍不能稳定触发
`Changed`，所以没有保留相关改动。P4 仍缺完整 range-tree、duplicated default
等价和 recovered switch / jump-table metadata。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:62`
  - 新增 `lowered_switch_sentinel_proxy`，代理 Angr lowered switch 中 all-ones
    sentinel 不能被误恢复成 switch 的安全边界。
  - 断言输出保留 `return -1;`、`return 7;`、`return 0;`，并且不出现
    `switch (x)`、`case -1:`、`case 7:`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 10 个样例扩到 11 个样例，并补上 P4 的一个
  安全边界代理；但没有新增算法能力。
- 复杂度：1/5。只新增一个内联 IR case。
- 维护成本：1/5。断言只检查关键输出和误识别的 switch/case 文本。

# 2026-06-28 P7 migration 分类报告记录

本次不改算法，只增强 `run_sailr_bench2_migration.py` 的可审计性。P7 的目标不是只跑
proxy，而是要能看清每个样例对应哪个 Angr 测试、属于真实样例还是 proxy、覆盖哪条
SAILR 语义。之前这些信息只隐含在脚本 case 字段里，不方便后续做真实样例分类。

这次增加可选 CSV 报告输出，默认跑法不变。报告只记录当前脚本样例的分类和结果，
不表示 P7 迁移完成；P7 仍缺更多真实样例，尤其是 `DuplicationReverter` merge graph
和 recovered switch / jump-table metadata。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:4`
  - 引入标准库 `csv`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:339`
  - 新增 `case_kind()`，默认把带 `input` 的样例标成 `real`，带内联 `ir` 的样例标成
    `proxy`，也允许 case 显式覆盖。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:386`
  - 新增 `write_report()`，输出 `name,kind,angr_test,semantic,status,failures`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:396`
  - CLI 增加 `--report-csv`，每个 case 跑完后记录 pass/fail 和失败摘要。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-report.csv`
  通过；报告显示 5 个 `real`、6 个 `proxy`，全部 `pass`。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。让 P7 样例分类可输出、可检查，但没有新增算法能力。
- 复杂度：1/5。只加一个可选 CSV 输出路径。
- 维护成本：1/5。默认脚本行为不变，报告字段稳定且少。

# 2026-06-28 P7 switch overlap migration 覆盖记录

本次不改算法，只把 P5 已有的 switch case/default overlap 输出边界补到
`run_sailr_bench2_migration.py`。这个形状之前在 C++ 和 smoke 里有覆盖，但 migration
分类报告里还没有单独代表 P5 overlap 的样例。

这仍是 proxy，不表示 P5 完成。P5 的核心缺口还是 recovered switch / jump-table metadata，
以及真实样例分类。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:326`
  - 新增 `switch_case_default_overlap_proxy`，代理同一个 switch 里 case/default 都落到
    shared body 的输出边界。
  - 断言 `switch (y)`、`case 2:`、`default:`、`b();`、`return 0;` 都存在，
    并确认 `structured_block_5:` 只定义一次。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-overlap-report.csv`
  通过；报告显示 5 个 `real`、7 个 `proxy`，全部 `pass`。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 11 个样例扩到 12 个样例，并补上 P5 overlap
  代表行为；但没有新增算法能力。
- 复杂度：1/5。只新增一个内联 IR case。
- 维护成本：1/5。断言关注共享 body 不重复和关键 switch 文本。

# 2026-06-28 P7 migration 指标报告记录

本次不改算法，只继续增强 `run_sailr_bench2_migration.py` 的报告。上一版 CSV 能看
case 分类和 pass/fail，但还不能快速比较结构化输出质量，比如 switch、case、goto、
return 数量。P7 后续要迁移更多真实样例，这些粗指标能帮助先发现明显退化。

这次也把本地缺失真实输入的情况改成 `skip`。之前真实 Bench2 文件不在当前机器时，
脚本会把它当成失败；这会混淆“样例缺失”和“结构化行为失败”。现在缺输入只写入报告，
不会让整批脚本失败。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:397`
  - 新增 `output_metrics()`，统计输出里的 `switch_count`、`case_count`、
    `goto_count`、`return_count`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:406`
  - `run_case()` 返回值改成 `status, failures, metrics`。
  - 缺少真实输入文件时返回 `skip`，不再记入全局 failure。
  - notdec-llvm2c 成功返回但没有输出文件时仍记为 `fail`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:454`
  - CSV 字段增加 `switch_count`、`case_count`、`goto_count`、`return_count`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:487`
  - 只有 `fail` 才合入全局失败列表，`skip` 只写报告。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-metrics-report.csv`
  通过；报告显示 1 个 `real` pass、4 个 `real` skip、7 个 `proxy` pass。
  当前 skip 的真实输入分别缺
  `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/hexx64/function-0x1156e0/native/function-0x1156e0.ll`、
  `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/python/one-_PyPegen_fill_token.cold.ll` 和
  `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/lighttpd/1-main_init_once.ll`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  未通过：当前机器在 `real_condensing_fixture.c` 处没有生成输出文件，脚本读取该文件时
  抛出 `FileNotFoundError`。这条失败和本次 migration 报告脚本改动无关，不能作为算法
  回归证据。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。报告能同时看分类、状态和粗结构指标，但没有新增算法能力。
- 复杂度：1/5。只增加一个输出计数 helper，并让缺本地真实输入进入 `skip`。
- 维护成本：1/5。默认脚本行为保持简单，CSV 字段仍是固定少量字段。

# 2026-06-28 P7 structuring smoke 外部样例 skip 记录

本次不改算法，只修 `run_structuring_smoke.py` 的本地数据缺失行为。这个脚本里有一个
Bench2 真实 lighttpd 输入使用绝对路径。当前机器缺这个文件时，notdec-llvm2c 不会生成
输出文件，脚本随后直接 `read_text()`，最后变成 `FileNotFoundError`。这会让 P7 smoke
结果不清楚：看起来像脚本崩溃，而不是“外部真实样例缺失”。

处理方式和 migration 脚本一致：只有显式标记的外部真实输入可以缺失时跳过；仓库内
fixture 或内联 IR 缺输出仍然是失败。这样本地可以继续验证内联 proxy 和仓库内 fixture，
但不会把缺 Bench2 文件伪装成算法回归。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:257`
  - 给 `real_condensing_fixture` 增加 `skip_if_missing`，说明这个 case 依赖外部 Bench2
    数据，本地缺失时可以跳过。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1364`
  - `run_case()` 在输入文件不存在时先检查 `skip_if_missing`。
  - 可跳过 case 打印 skip 信息并返回空 failure；不可跳过 case 返回明确 failure。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1386`
  - notdec-llvm2c 成功返回但没有输出文件时，返回明确 failure，不再抛异常。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_structuring_smoke.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；输出
  `real_condensing_fixture: skipped missing input file /sn640/NotDec-Exp/Bench2/bin2llvm-ir/lighttpd/1-main_init_once.ll`。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-after-smoke-skip.csv`
  通过。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。修清楚 P7 smoke 的本地缺数据行为，但没有新增算法能力。
- 复杂度：1/5。只给一个外部真实样例加 skip 标记，并补缺输出检查。
- 维护成本：1/5。skip 必须显式标记，不会静默跳过仓库内回归。

# 2026-06-28 P7 continuous lowered switch migration 覆盖记录

本次不改算法，只把 P4 已有的 continuous if-chain 安全边界补到
`run_sailr_bench2_migration.py`。Angr 的 lowered switch 恢复有保守过滤：没有 switch
hint 时，连续小整数比较链不能只凭形状恢复成 switch。NotDec C++ 回归已有这个边界，
但 migration 报告里还缺对应样例。

这个用例仍是 proxy，不表示 P4 完成。P4 仍缺完整 range-tree、duplicated default 等价
和 recovered switch / jump-table metadata。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:89`
  - 新增 `lowered_switch_continuous_no_hint_proxy`，代理 continuous lowered if-chain
    没有 switch hint 时不能恢复成 switch 的边界。
  - 断言输出保留 `return 7;`、`return 8;`、`return 0;`，并且不出现
    `switch (x)`、`case 7:`、`case 8:`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-continuous-report.csv`
  通过；报告显示 1 个 `real` pass、4 个 `real` skip、8 个 `proxy` pass。新增 case
  的指标是 `switch_count=0`、`case_count=0`、`goto_count=0`、`return_count=3`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 12 个样例扩到 13 个样例，并补上 P4 的一个
  安全边界代理；但没有新增算法能力。
- 复杂度：1/5。只新增一个内联 IR case。
- 维护成本：1/5。断言只检查关键 return 和误识别的 switch/case 文本。

# 2026-06-28 P4 range compare migration 覆盖记录

本次不改算法，只把 `LoweredSwitchSimplifier` 的 range compare 保守边界补到
`run_sailr_bench2_migration.py`。当前 shared CFG 里 `>` / `>=` / `<` / `<=`
还不应该被恢复成 switch；它们只是 P4 的前置元数据，不代表 range-tree 已完成。

这条代理只是确认：连续 range compare 在 C 输出层仍保留 if-chain，不会误变成
`switch` / `case`。这和前面 `testLoweredSwitchSimplifierSkipsRangeConditionCompare()`
的 C++ shared-CFG 负例是一致的。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:117`
  - 新增 `lowered_switch_range_compare_proxy`，代理 range compare 还未恢复成 switch
    的保守边界。
  - 断言输出保留 `if (x > 7)`、`if (x > 9)`、`return 7;`、`return 9;`、
    `return 0;`，并且不出现 `switch (x)`、`case 7:`、`case 9:`。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-range-compare-report.csv`
  通过；新增 case 的指标是 `switch_count=0`、`case_count=0`、`goto_count=0`、
  `return_count=3`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 13 个样例扩到 14 个样例，并补上 P4 的一个
  range compare 保守边界；但没有新增算法能力。
- 复杂度：1/5。只新增一个内联 IR case。
- 维护成本：1/5。断言只检查关键 if-chain 文本和 switch 误识别文本。

# 2026-06-28 P7 fmt deduplication migration 覆盖记录

本次不改算法，只把 Angr 的 `test_fmt_deduplication` 对应输出层代理补到
`run_sailr_bench2_migration.py`。这个样例覆盖 `DuplicationReverter` 相关的重复调用
尾部形状：两个分支都调用 `xdectoumax()`，再汇合到同一个 `return 0;`。

当前覆盖仍是 proxy，只说明 C 输出层不会生成明显的 `goto left` / `goto right`，并不表示
P1 的 merge graph 一般能力已经完成。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:143`
  - 新增 `fmt_deduplication_proxy`，关联 Angr `test_fmt_deduplication`。
  - 内联 IR 构造左右分支重复调用 `xdectoumax()` 后汇合返回的形状。
  - 断言输出包含 `xdectoumax()` 和 `return 0;`，不出现 `goto left` / `goto right`，
    并检查 `return 0;` 出现 2 次。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-fmt-dedup-report.csv`
  通过；新增 case 的指标是 `switch_count=0`、`case_count=0`、`goto_count=0`、
  `return_count=2`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 14 个样例扩到 15 个样例，并补上
  `DuplicationReverter` 的一个输出层代理；但没有新增算法能力。
- 复杂度：1/5。只新增一个内联 IR case。
- 维护成本：1/5。断言只检查关键调用、返回和明显错误 goto。

# 2026-06-28 P7 range-tree migration xfail 记录

本次不改算法，只把 `LoweredSwitchSimplifier` 的 range-tree 输出层缺口登记到
`run_sailr_bench2_migration.py`。当前 C++ shared-CFG 回归
`testLoweredSwitchSimplifierBuildsSwitchFromRangeTree()` 已经能把左右两条 equality chain
合成 switch，但同形状 LLVM IR 走 `notdec-llvm2c --algo=structured-sailr` 时仍输出嵌套
`if`，没有生成 `switch (x)`。

因此这次给 migration 脚本加 `xfail`，让报告能明确显示“这是已知未完成项”，而不是继续
只靠已经通过的 proxy 撑覆盖。这个记录不表示 P4 完成，反而把 P4 的真实输出层缺口固定
下来，后续要继续查主链路 CFG 到 shared CFG 的条件元数据、候选顺序或 rewrite 后的
structuring 结果。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:143`
  - 新增 `lowered_switch_range_tree_output_gap`，关联 Angr
    `test_reverting_switch_lowering_range_tree`。
  - 期望输出包含 `switch (x)`、`case 7:`、`case 9:`、`case 11:`、`case 13:` 和
    `default:`；并且不再保留外层 `if (x <= 9)`、低半区 `if (x == 7)`、高半区
    `if (x == 11)`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:625`
  - `main()` 支持 case 标记 `expected_failure`。
  - 预期失败写成 `xfail` 并进入 CSV；如果未来意外通过则写成 `xpass`，并让脚本失败，
    逼后续把 xfail 改成正常 pass。

## 验证

- 手工用同形状临时 IR 跑
  `/sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr`
  确认当前输出仍是嵌套 `if`，没有 `switch`。
- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-range-tree-xfail-report.csv`
  通过；新增 case 状态为 `xfail`，指标是 `switch_count=0`、`case_count=0`、
  `goto_count=0`、`return_count=6`。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次只改测试脚本，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。migration 脚本覆盖从 15 个样例扩到 16 个样例，其中新增 1 个
  `xfail`，让 P4 输出层缺口可见。
- 复杂度：1/5。只给脚本增加预期失败状态处理和一个内联 IR case。
- 维护成本：1/5。`xpass` 会让脚本失败，避免缺口修好后继续被当成预期失败。

# 2026-06-28 P4 range-tree lowered switch 修复记录

本次修复上面登记的 range-tree 输出层缺口。真实路径里 `RetDupPass` 会把同一个
default `return 0;` 拆成两个 AST payload，之前 shared-CFG 的 range-tree 合并要求左右
default 完全同一目标，导致 `notdec-llvm2c --algo=structured-sailr` 仍输出嵌套 `if`。

修复后，左右 equality chain 仍必须共享同一个比较变量、case 值互不重复、case 都符合
range guard；只有两个 default 都是 closed terminal、语句 payload origin 相同、且只被
当前链路进入时，才会把 duplicated default 合并为一个 switch default。adapter 侧只给
简单整数字面量 `return` 建 shared payload origin，不复用 AST 节点。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:155`
  - 增加 `blockOnlyReachedFrom()` 前置声明，供 range-tree default 合并做链内 predecessor
    检查。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1901`
  - 新增 `sameClosedDefaultBlock()`，只接受 closed terminal 且 statement origin 相同的
    duplicated default。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1917`
  - 新增 `mergeEquivalentRangeTreeDefaults()`，把链私有 duplicated default 合并到第一个
    default，并把第二个 default 加入 removed block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1943`
  - `mergeRangeTreeLoweredSwitchIfChains()` 不再要求 default target id 完全相同，改为走
    上面的保守等价 default 检查。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:131`
  - 新增简单 return payload origin 缓存，记录整数字面量 return 的 canonical payload。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:170`
  - 新增 `setSimpleReturnOrigin()`，让 duplicated `return 0;` 共享 payload origin。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:518`
  - `buildCFG()` 在收集 block statement 时调用 `setSimpleReturnOrigin()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10849`
  - 新增 `testLoweredSwitchSimplifierMergesRangeTreeDuplicatedDefaultReturns()`，覆盖左右
    range-tree 分支各自有 duplicated default return 的 shared-CFG 合并。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:144`
  - `lowered_switch_range_tree_output_gap` 改为 `lowered_switch_range_tree_proxy`，去掉
    `expected_failure`，现在要求真实 `notdec-llvm2c` 输出 `switch (x)` 和四个 case。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuredGoto.cpp lib/Structuring/SAILRDeoptimization.cpp test/structuring/structuring_analysis_test.cpp test/structuring/run_sailr_bench2_migration.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `/sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /tmp/notdec-sailr-debug-range-tree/range_tree.ll -o /tmp/notdec-sailr-debug-range-tree/range_tree.originfix.c`
  输出包含 `switch (x)`、`case 7:`、`case 9:`、`case 11:`、`case 13:`。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-originfix.csv`
  初次按旧 xfail 运行得到 `xpass`，确认缺口已修复；更新预期后重新验证见最终记录。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-range-tree-fixed.csv`
  通过；`lowered_switch_range_tree_proxy` 指标是 `switch_count=1`、`case_count=4`、
  `goto_count=0`、`return_count=5`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次改动只影响 llvm2c 的 structuring / adapter 路径，不影响主 NotDec 类型恢复和 pass
pipeline。性能 smoke 用 migration 和 structuring smoke 观察同类链路；没有跑 fortune，
因为 fortune 不能代表这条 llvm2c range-tree 输出路径。

## 影响判断

- 实现效果：3/5。修掉 P4 里一个真实输出层缺口，range-tree lowered switch 能从 nested
  if 恢复成 switch。
- 复杂度：2/5。新增一个很窄的 default 等价判断和一个 adapter payload origin cache。
- 维护成本：2/5。判断条件偏保守，只覆盖简单 literal return duplicated default；后续如果
  要支持复杂 default，需要继续扩展 payload 等价来源。

# 2026-06-28 P4 range-tree return-var default 修复记录

继续收窄 P4 的 range-tree 输出缺口。上一轮只让 `return 0;` 这类整数字面量 default
共享 payload origin；同形状 IR 如果 default 是 `return x;`，`RetDupPass` 仍会生成两个
不同 AST return payload，range-tree 合并会因为 duplicated default 语句 origin 不同而跳过。

本次只扩展 adapter 侧的简单 return origin：整数字面量仍按值复用，`return <DeclRef>;`
按同一个 `ValueDecl*` 复用。复杂表达式仍不做等价判断。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:131`
  - 将简单 return origin 缓存拆成 `SimpleReturnDeclPayloads` 和
    `SimpleReturnIntegerPayloads`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:171`
  - `setSimpleReturnOrigin()` 除整数字面量外，也支持 `DeclRefExpr` 返回值。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:196`
  - 新增 `lowered_switch_range_tree_return_var_proxy`，固定 range-tree duplicated default
    为 `return x;` 时也能输出 `switch (x)`。
- `logs/20260627-02-sailr-angr-remaining-parity-plan.md:268`
  - 同步更新 P0 对照表里 `LoweredSwitchSimplifier` 的当前状态：range-tree 和
    duplicated default 已有保守覆盖，剩余转向更完整 range-tree 细节、shared case node
    复杂复制和 recovered switch metadata。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuredGoto.cpp test/structuring/run_sailr_bench2_migration.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `/sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /tmp/notdec-sailr-debug-range-tree/range_tree_retvar.ll -o /tmp/notdec-sailr-debug-range-tree/range_tree_retvar.fixed.c`
  输出包含 `switch (x)`、四个 case 和 `return x;`。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-migration-return-var-final.csv`
  通过；`lowered_switch_range_tree_return_var_proxy` 指标是 `switch_count=1`、
  `case_count=4`、`goto_count=0`、`return_count=5`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次仍只影响 llvm2c structuring / adapter 链路，没有跑 fortune；migration 和 structuring
smoke 覆盖了这条输出路径。

## 影响判断

- 实现效果：2/5。补上 range-tree duplicated default 的一个常见非字面量返回形状。
- 复杂度：1/5。只扩展 adapter 的 simple return origin key，不改 range-tree 合并规则。
- 维护成本：1/5。只接受 `DeclRefExpr`，没有引入任意表达式等价。

# 2026-06-28 P3/P7 multi-vvar copied return migration 覆盖记录

本次不改运行时代码，补一条输出层 migration proxy，并同步 P0 测试迁移清单。已有
`copied_return_tail_dephication_proxy` 只覆盖单个 Phi/vvar 的 copied return tail；这次新增
两个 Phi/vvar 同时进入 shared tail 的形状，确认 copied payload materialize 能同时处理
`p` 和 `q` 两个 incoming。

这仍是 proxy，不表示 P3 的 shared Phi / vvar / copied payload 全量消费完成；它只把已有
C++ 多 vvar 覆盖提升到 `notdec-llvm2c` 输出层。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:362`
  - 新增 `copied_return_tail_multi_vvar_proxy`，使用 `--sailr-dephication-mode=angr`。
  - 内联 IR 构造两个 switch case 进入 shared tail，tail 内有 `%p` 和 `%q` 两个 Phi，
    返回 `%p + %q`。
  - 断言输出包含 `p_copy`、`q_copy`、`p = b;`、`q = d;` 和两处 `return s;`，并且没有
    `phi`、`reg2mem`、`p_reg2mem`、`q_reg2mem`。
- `logs/20260627-02-sailr-angr-remaining-parity-plan.md:304`
  - 同步“当前测试迁移状态”：migration 脚本现在有 18 个分类样例，本机 15 个通过、
    3 个因外部真实输入缺失跳过。

## 验证

- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-multi-vvar-final.csv`
  通过；`copied_return_tail_multi_vvar_proxy` 指标是 `switch_count=1`、`case_count=2`、
  `goto_count=0`、`return_count=3`。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。

本次只是测试覆盖和日志同步，不改反编译算法路径，所以没有跑 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。把 P3 的 multi-vvar copied payload 覆盖推进到 migration 输出层。
- 复杂度：1/5。只新增一个内联 IR proxy。
- 维护成本：1/5。断言只看关键 copied 变量、Phi 消除和 return 数量。

# 2026-06-28 P7 fortune smoke 前端阻塞收窄记录

本次尝试用当前 Bench2 布局里的
`selected-targets-native/fortune/executable/module-all.ll` 做真实样例 smoke。旧的函数级
lighttpd / hexx64 / python 输入在本机不存在；直接跑 fortune 时，`notdec-llvm2c` 先卡在
未处理的 LLVM overflow intrinsic，不到 SAILR structuring 阶段。

这次只补 `llvm2c` 对 `sadd/uadd/ssub/smul/umul.with.overflow` 的保守处理：`extractvalue 0`
降成普通 `add/sub/mul` 表达式，`extractvalue 1` 仍调用 fake helper
`llvm_*_is_overflow_iN()`。这不是 SAILR pass parity 的完成项，只是把 P7 真实样例 smoke
的第一个前端断言移开。重新跑 fortune 后，新的阻塞变成普通/native 多返回值函数的
`extractvalue`，需要后续单独处理。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:108`
  - 新增 overflow intrinsic helper，统一识别 add/sub/mul overflow intrinsic 和对应 fake
    helper 名。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:942`
  - `CFGBuilder::visitCallInst()` 对这些 overflow intrinsic 不再生成普通 call stmt，交给
    `extractvalue` 消费。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1072`
  - `CFGBuilder::visitExtractValueInst()` 支持 `sadd/uadd/ssub/usub/smul/umul.with.overflow`。
  - 顺手修正 overflow helper 第二个实参从误用 `Op0` 改为 `Op1`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2004`
  - `SAContext::getIntrinsic(std::string)` 支持所有 `llvm_*_is_overflow_iN` fake helper。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2186`
  - `SAContext::createDecls()` 跳过已由 `extractvalue` 处理的 overflow intrinsic。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:354`
  - 新增 `llvm_overflow_intrinsics_extractvalue`，覆盖 add/sub/mul overflow intrinsic 输出。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过，目标已是最新。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-overflow-migration.csv`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过旧路径缺失的 lighttpd 输入。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-module-all.c`
  仍失败，但已经越过 overflow intrinsic 声明断言；新失败是普通/native 多返回值
  `extractvalue`，`elapsed=0.32 user=0.29 sys=0.03 maxrss=199096`。

本次只影响 `llvm2c` 前端表达式构造和输出层 smoke，不影响主 NotDec pass pipeline。

## 影响判断

- 实现效果：1/5。真实 fortune smoke 少一个前端断言阻塞，但还没跑到完整真实样例输出。
- 复杂度：1/5。复用已有 `umul` 处理思路，扩成同类 overflow intrinsic。
- 维护成本：1/5。只处理 LLVM 标准 overflow intrinsic，不引入泛化聚合返回处理。

# 2026-06-28 P7 fortune smoke 聚合返回 extractvalue 收窄记录

继续推进上一节的 fortune smoke。overflow intrinsic 修掉后，fortune 卡在普通/native 多返回值
函数的 `extractvalue`，形状是 `%x = extractvalue (call { i64, ... } @foo()), N`。这类不是
SAILR 本身的问题，但会阻止真实样例跑到 structuring 阶段。

本次只支持 call 返回 LLVM struct 后立刻取字段的窄形状：先把 call materialize 到一个局部
struct 临时变量，再把 `extractvalue` 降成 `tmp.field_N`。不处理 `insertvalue` 链、不处理
任意聚合表达式，也不修改匿名 struct 的打印格式。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1090`
  - 新增 `materializeAggregateCallField()`，为 aggregate-return call 生成局部临时变量和
    `tmp = call(...)`，并把 call 的表达式映射改成这个临时变量，避免多个字段访问重复调用。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1182`
  - `CFGBuilder::visitExtractValueInst()` 在 overflow intrinsic 分支之后，支持单层 index 的
    aggregate-return call 字段访问。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:398`
  - 新增 `aggregate_return_extractvalue`，覆盖 `{ i64, i64 }` 返回值同时取两个字段时输出
    `p = pair(a, b);` 和 `return p.field_0 + p.field_1;`。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过旧路径缺失的 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-aggregate-extract-migration.csv`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-module-all.c`
  仍失败，但已经越过普通/native 多返回值 `extractvalue`；新失败是 `void*` deref 断言，
  `elapsed=5.11 user=4.96 sys=0.14 maxrss=355948`。

本次只影响 `llvm2c` 前端表达式构造和输出层 smoke，不影响主 NotDec pass pipeline。

## 影响判断

- 实现效果：1/5。真实 fortune smoke 又向前推进一段，但仍未跑到完整输出。
- 复杂度：2/5。新增了 call materialize 和字段访问映射，范围仍限定在 call-return struct。
- 维护成本：2/5。后续若要支持 `insertvalue` 或更复杂聚合，需要单独扩展，不能复用这段假装通用。

# 2026-06-28 P7 fortune smoke opaque GEP 收窄记录

继续推进 fortune smoke。上一节修掉 aggregate-return `extractvalue` 后，fortune 卡在
`handleGEP()` 对 LLVM 22 opaque pointer 的 `void*` 解引用。IR 的 GEP 本身仍带
`source element type`，所以这次只在 GEP 入口使用这个类型把 `void*` cast 成
`source_element_type*`，再走已有 pointer arithmetic / deref 逻辑。不修改 `deref()` 的
全局行为。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:442`
  - `handleGEP(Ctx, EB, TB, GEPOperator&)` 在 pointer operand 是 `void*` 时，用
    `GEPOperator::getSourceElementType()` 生成 typed pointer cast。
  - 同步把常量表达式 GEP 调用点也传入 `TypeBuilder`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:415`
  - 新增 `opaque_pointer_gep_source_type`，覆盖 `getelementptr i64, ptr %p, i64 1`
    能输出 `*((long long *)p + 1LL)`，不再直接对 `void*` 做 GEP / deref。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过旧路径缺失的 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-opaque-gep-migration.csv`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-module-all.c`
  仍失败，但已经越过 `void*` GEP / deref 断言；新失败是 `CFGBlock::updateStmt` slot
  断言，`elapsed=5.17 user=5.07 sys=0.09 maxrss=356208`。

本次只影响 `llvm2c` GEP 表达式构造和输出层 smoke，不影响主 NotDec pass pipeline。

## 影响判断

- 实现效果：1/5。真实 fortune smoke 继续推进到下一个前端阻塞。
- 复杂度：1/5。只利用 GEP 自带 source element type 做 cast。
- 维护成本：1/5。不改变通用 `void*` 解引用策略。

# 2026-06-28 P7 fortune smoke lazy load slot fallback 记录

fortune 继续往前跑后，前一个 blocker 已经从 `void*` GEP / deref 断言移开，新的停点是
`CFGBlock::updateStmt()` 断言。这里的场景不是 slot 本身失效，而是 lazy load 先放进去的
`NullStmt` 占位后来已经被别的语句填掉；这时再尝试把 load 临时变量塞回原 slot 会撞上
旧实现的强断言。  

这次把 `updateStmt()` 改成“占位还是 `NullStmt` 就原地替换；否则返回 `false`”，
`SAFuncContext::addStmt()` 在 slot 已被占用时退回到正常 `appendStmt()`。这样不改 lazy load
的主路径，只让已经被别的语句占住的 slot 不再把 fortune 卡死。fortune 重新跑后，确实越过
了这个断言，新的 blocker 变成 `getLLVMTypeSize()` 的零尺寸断言，说明这次 fallback 是有效的。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:508`
  - `CFGBlock::updateStmt()` 改成返回 `bool`，slot 还是 `NullStmt` 时原地替换，否则返回
    `false`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1271`
  - `SAFuncContext::addStmt()` 在 `Slot` 已被占用时不再强行写回原位，改为追加语句。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:507`
  - 补了一句短注释，说明这个返回值就是给 lazy load 占位冲突兜底用的。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-llvm2c/CFG.h lib/notdec-llvm2c/StructuralAnalysis.cpp`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-slot-fallback-migration.csv`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-module-all.c`
  这次已经越过 `CFGBlock::updateStmt` slot 断言，新的 blocker 是 `Utils.cpp:70` 的
  `getLLVMTypeSize()` 零尺寸断言；`elapsed=22.50 user=22.19 sys=0.31 maxrss=644452`。

本次还是只影响 `llvm2c` structuring / adapter 链路，不改 SAILR pass 选择和排序。

## 影响判断

- 实现效果：2/5。把 fortune 的一个真实前端断言收窄掉了，确实推进到下一层。
- 复杂度：1/5。只是在已有 lazy load slot 机制上加了一个退路。
- 维护成本：1/5。行为很局部，只有 slot 冲突时才走 fallback。

# 2026-06-28 P7 fortune smoke aggregate load/store 尺寸收窄记录

继续推进 fortune smoke。上一节的 lazy load slot fallback 之后，fortune 卡在
`getLLVMTypeSize()` 对聚合类型的断言：先是 aggregate store，修掉后又落到 aggregate load。
这次只处理 load/store 的类型选择：LLVM aggregate 没有标量 bitwidth，所以直接用
`TypeBuilder::visitType()` 保留结构体类型；标量路径仍走原来的大小匹配和整数兜底。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:503`
  - `CFGBuilder::visitStoreInst()` 遇到 aggregate store value 时直接恢复 record type，不再调用
    `getLLVMTypeSize()`；后续大小比较只在 `StoreSize` 存在时执行。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:582`
  - `CFGBuilder::visitLoadInst()` 遇到 aggregate load result 时直接恢复 record type，不再调用
    `getLLVMTypeSize()`；标量 load 行为不变。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:428`
  - 新增 `aggregate_store_type`，覆盖 `{ i64, i64 }` call 返回值直接 store 到指针。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:443`
  - 新增 `aggregate_load_type`，覆盖 `{ i64, i64 }` load 后再 store 的 lazy load 表达式。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-aggregate-load-migration.csv`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-aggregate-load.c`
  仍失败，但已经越过 aggregate load/store 的 `getLLVMTypeSize()` 断言；新 blocker 是
  `CFGBuilder::visitExtractValueInst()` 对 `insertvalue` 链后续 `extractvalue` 的未处理断言。
  本次同口径结果：`elapsed=22.30 user=22.04 sys=0.26 maxrss=645824`。

本次只影响 `llvm2c` 的 load/store 表达式类型选择和 structuring smoke，不改 SAILR pass
排序，也不展开做完整 `insertvalue` 聚合构造支持。

## 影响判断

- 实现效果：2/5。真实 fortune smoke 从聚合 load/store 尺寸断言推进到下一类聚合表达式问题。
- 复杂度：1/5。只把聚合类型从标量 size 路径分出来。
- 维护成本：1/5。行为限定在 load/store aggregate 类型，不改变现有标量推断。

# 2026-06-28 P7 fortune smoke extractvalue 聚合来源收窄记录

继续推进 fortune smoke。上一节之后，fortune 卡在 `CFGBuilder::visitExtractValueInst()`：
一种来源是 `extractvalue (insertvalue chain), N`，另一种来源是 reg2mem 后的 aggregate load
再取字段。这次只处理这两种单层字段访问：`insertvalue` 链按字段号倒查最近一次写入并转发
operand；已经能作为 C record 表达式的聚合值则生成 `field_N` 访问。嵌套字段和完整聚合构造
仍不在本次范围内。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1167`
  - 新增 `materializeAggregateExprField()`，把 aggregate load 这类已可表达的 record value 的
    `extractvalue` 降成字段访问。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1193`
  - 新增 `findInsertValueField()`，沿单层 `InsertValueInst` 链倒查目标字段的 inserted operand。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1210`
  - `CFGBuilder::visitExtractValueInst()` 在已有 overflow intrinsic 和 aggregate-return call 逻辑后，
    支持上述两类单层聚合字段访问。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:460`
  - 新增 `insertvalue_extractvalue_forward`，覆盖从 `{ i64, i64 }` insertvalue 链取两个字段后相加。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:476`
  - 新增 `aggregate_load_extractvalue`，覆盖 `{ i64, i64 }` load 后取字段。

## 验证

- `git -C external/NotDec-llvm2c diff --check lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-extractvalue-aggregate-migration.csv`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-extractvalue-aggregate.c`
  仍失败，但已经越过 `visitExtractValueInst()` 的聚合来源断言；新 blocker 是
  `ExprBuilder::getUndef()` 对 `visitReturnInst()` 里 `ret undef` 的非标量类型未处理。
  本次同口径结果：`elapsed=22.48 user=22.20 sys=0.28 maxrss=643392`。

本次只影响 `llvm2c` 的 `extractvalue` 表达式构造和 smoke 用例，不改 SAILR pass 排序，
也不实现完整 `insertvalue` 聚合对象。

## 影响判断

- 实现效果：2/5。fortune 从 `extractvalue` 聚合来源断言推进到下一类 `undef` 表达式问题。
- 复杂度：2/5。新增两个小 helper，但只处理单层字段访问。
- 维护成本：2/5。后续完整聚合构造仍要单独实现，不能把这里当通用 aggregate builder。

# 2026-06-28 P7 fortune smoke aggregate undef 收窄记录

继续推进 fortune smoke。上一节之后，fortune 卡在 `ExprBuilder::getUndef()` 对非标量
`ret poison` / `ret undef` 的断言。这次只处理 record 和 array 形状的 undef：record 按字段
生成已有的 `llvm_undef_*()` 表达式，再包成 compound literal；array 先用 Clang 的
implicit value init 表达式兜底。这样避免把 record undef 静默降成全零。

这次还补了 `StmtTransform` 对 compound literal / init list 的递归处理，因为 lazy load
重写路径会遍历 return 表达式；没有这层会在新生成的 record literal 上断言。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2804`
  - `ExprBuilder::getUndef()` 支持 record undef，逐字段递归生成 undef initializer，并返回
    `CompoundLiteralExpr`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2815`
  - `ExprBuilder::getUndef()` 支持 array undef，返回 `ImplicitValueInitExpr`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CCodeTransform.h:332`
  - `StmtTransform::TransformCompoundLiteralExpr()` 递归 transform initializer，必要时重建节点。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CCodeTransform.h:346`
  - `StmtTransform::TransformInitListExpr()` 递归 transform 字段 initializer 和 array filler。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CCodeTransform.h:393`
  - `StmtTransform::TransformImplicitValueInitExpr()` 把 implicit value init 当叶子表达式放行。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:489`
  - 新增 `aggregate_undef_return`，覆盖 `{ i64, i64 }` 函数直接返回 `poison` 的输出。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-llvm2c/CCodeTransform.h lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-aggregate-undef-migration.csv`
  通过；CSV 18 个样例里真实输入缺失的 case 仍按 skip 记录。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-aggregate-undef.c`
  通过并产出 C；同口径结果：`elapsed=54.52 user=54.10 sys=0.42 maxrss=667764`。
  输出里仍有多处 `insertvalue` 链未降级警告，说明下一类聚合构造还没处理。

本次只影响 `llvm2c` 的 undef 表达式构造、通用 AST transform 子集和 structuring smoke，
不改 SAILR pass 排序，也不实现完整 `insertvalue` 聚合对象。

## 影响判断

- 实现效果：2/5。fortune 从 `getUndef()` 非标量断言推进到可以完整跑出 C，但仍有
  `insertvalue` 聚合构造未降级。
- 复杂度：2/5。record undef 和 AST transform 都是局部补齐，没有引入通用 aggregate builder。
- 维护成本：2/5。后续如果实现完整 `insertvalue`，需要复用或替换这里的 record literal 构造。

# 2026-06-28 P7 fortune smoke insertvalue 聚合返回收窄记录

继续推进 fortune smoke。上一节之后，fortune 已经能跑出 C，但输出里还有大量
`insertvalue` 链未处理。实际问题是 native summary 常把多寄存器返回写成
`insertvalue ... undef` 链，然后直接 `ret { ... } %v`。之前 `extractvalue` 可以从这种链
转发字段，但完整 aggregate return 会在 `ExprBuilder` 里拿不到表达式，容易退成空 return
或只留下 warning。

这次只补单层 struct 的 `insertvalue` 链：从最终 `InsertValueInst` 往前扫，收集每个字段
最后一次写入；base 只接受 `undef` / `poison`，未写字段继续用上一节的字段级 undef。嵌套
aggregate、array、非 undef base 仍不处理，避免把这里变成不完整的通用 aggregate builder。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:327`
  - 给 `ExprBuilder` 声明 `visitInsertValueInst()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2558`
  - 新增 `ExprBuilder::visitInsertValueInst()`，把单层 struct `insertvalue` 链降成
    `CompoundLiteralExpr`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:725`
  - `CFGBuilder::visitInsertValueInst()` 对纯 SSA 聚合构造做 no-op，实际表达式在使用点物化，
    不再打印误导性的 `Cannot handle insertvalue`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:476`
  - 新增 `insertvalue_aggregate_return`，覆盖 `{ i64, i64 }` insertvalue 链直接返回 struct。

## 验证

- `git -C external/NotDec-llvm2c diff --check include/notdec-llvm2c/StructuralAnalysis.h lib/notdec-llvm2c/StructuralAnalysis.cpp test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-insertvalue-return-migration.csv`
  通过；CSV 18 个样例里真实输入缺失的 case 仍按 skip 记录。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-insertvalue-return.c`
  通过并产出 C；同口径结果：`elapsed=54.81 user=54.38 sys=0.42 maxrss=666332`。
  输出中不再出现 `CFGBuilder: Cannot handle insertvalue`，并能看到多处
  `return (struct (unnamed)){...};`，例如 `/tmp/notdec-sailr-fortune-insertvalue-return.c:1568`。

本次只影响 `llvm2c` 的单层 struct 聚合返回表达式和日志噪声，不改 SAILR pass 排序，
也不处理嵌套 aggregate / array / 非 undef base。

## 影响判断

- 实现效果：2/5。fortune 的 insertvalue 聚合返回能落成 C 表达式，真实样例输出更完整。
- 复杂度：2/5。新增一条保守的单层 struct 构造路径。
- 维护成本：2/5。后续如果要支持嵌套 aggregate，需要单独设计，不能继续在这里硬扩。

# 2026-06-28 P7 迁移报告分类记录

继续推进 P7 的真实样例分类。现有 `run_sailr_bench2_migration.py` 只输出 pass/fail/skip
和基础计数，看不出失败到底是输入缺失、runner 崩溃、仍有 goto、缺 switch/case，还是普通
输出不匹配。这样不利于判断后续该回到 P1-P5 的哪条线。

这次只扩展 CSV 报告，不改变任何 case 的通过标准。新增 `classification` 字段，把当前
结果粗分为 `pass`、`missing-input`、`runner-failure`、`unexpected-goto`、
`missing-structure`、`output-mismatch`，以及 xfail/xpass 的对应分类。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:637`
  - 新增 `classify_failures()`，按状态和失败文本给迁移样例分类。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:654`
  - `run_case()` 额外返回 `classification`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:712`
  - CSV `fieldnames` 增加 `classification`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:736`
  - 主循环把分类写入每行报告，并保留原有 fail/xpass 判定。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-classified-migration.csv`
  通过。
- `/tmp/notdec-sailr-classified-migration.csv` 当前统计是 14 个 `pass/pass`、4 个
  `skip/missing-input`。缺失的真实输入仍是 hexx64、python、lighttpd 两个路径；代理样例
  全部保持通过。

本次只影响 P7 报告脚本，不影响 `llvm2c` 或 SAILR pass 执行；因此不需要 fortune 性能
smoke。

## 影响判断

- 实现效果：1/5。没有新增算法能力，但后续真实样例分类更清楚。
- 复杂度：1/5。只增加一个报告字段和简单分类函数。
- 维护成本：1/5。分类基于现有失败文本，后续可按需要继续细分。

# 2026-06-28 P7 lighttpd 真实样例路径分类记录

继续推进 P7 的真实样例分类。上一节报告里 lighttpd 仍显示为 `missing-input`，但当前 Bench2
已经没有旧的 `lighttpd/1-main_init_once.ll` per-function 布局，只有
`selected-targets-native/lighttpd/executable/module-all.ll`。手动跑现有 module-all 后确认它能
进入 `notdec-llvm2c`，但目前会在 Clang `QualType` null 断言处 abort。因此这里应该归类为
真实 runner blocker，而不是路径缺失。

这次把两个 lighttpd real case 指到现有 module-all，并标成 expected failure；同时把 CSV
失败详情压成单行并截断，避免 runner stderr 把一条 case 拆成多行，影响后续统计。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:11`
  - 新增 `MAX_FAILURE_DETAIL`，限制 CSV 失败详情长度。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:42`
  - `goto_condensing_chain` 改用
    `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/lighttpd/executable/module-all.ll`，
    并标记当前 `QualType` runner blocker 为 expected failure。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:617`
  - `condensing_real_lighttpd` 同步改用现有 module-all，并标记同一个 expected failure。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:657`
  - 新增 `summarize_failure()`，把失败信息转成单行并截断。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:778`
  - 写 CSV 时使用压缩后的失败详情。

## 验证

- `git -C external/NotDec-llvm2c diff --check test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-lighttpd-xfail-migration.csv`
  通过。
- `/tmp/notdec-sailr-lighttpd-xfail-migration.csv` 当前统计是：14 个 `pass/pass`，2 个
  `skip/missing-input`，2 个 `xfail/expected-runner-failure`。剩余 missing input 是
  hexx64 和 python；lighttpd 已经从 missing-input 升级为真实 runner blocker。

本次只改 P7 迁移报告和真实样例路径，不改 SAILR pass，也不影响 `llvm2c` 性能。

## 影响判断

- 实现效果：1/5。没有新增算法能力，但真实样例分类更准确。
- 复杂度：1/5。只更新路径、expected failure 和 CSV 摘要。
- 维护成本：1/5。等 lighttpd runner blocker 修掉后，去掉 expected failure 即可。

# 2026-06-28 P7 wide integer runner blocker 记录

继续推进 P7 的 lighttpd runner blocker。`selected-targets-native/lighttpd/executable/module-all.ll`
先在 `i512` 负常量和宽 load 上触发 Clang `QualType` null / 打印器断言。这个问题不是 SAILR
CFG 结构本身，而是 `llvm2c` 表达式构造和打印对 `_BitInt(N)` 支持不完整。

这次只补宽整数 fallback，不改变类型恢复策略：LLVM 宽整数仍通过 `TypeBuilder::visitType()`
落到 `_BitInt(N)`；负数宽常量创建 literal 时如果没有内建有符号整数类型，就用有符号
`_BitInt(N)`；打印 `_BitInt(N)` integer literal 时不再强转 `BuiltinType`。lighttpd 修过这两处后
不再快速 abort，但 90 秒内不能完成，因此迁移报告把它从 expected runner-failure 更新为
expected-timeout。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:617`
  - `CFGBuilder::visitLoadInst()` 的未知 load 类型 fallback 改用
    `getTypeBuilder().visitType(*LoadTy)`，让 `load i512` 能得到 `_BitInt(512)`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:3019`
  - `ExprBuilder::visitConstant()` 的负数无符号整数字面量路径增加 `SignedTy` fallback；
    `getIntTypeForBitwidth(512, true)` 为空时改用 `Ctx.getBitIntType(false, 512)`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/ASTPrinter/StmtPrinter.cpp:1079`
  - `StmtPrinter::VisitIntegerLiteral()` 支持非 `BuiltinType` integer literal，打印数值后直接返回，
    避免 `_BitInt(N)` literal 触发 `castAs<BuiltinType>()` 断言。
- `external/NotDec-llvm2c/test/structuring/fixtures/wide_negative_integer_literal.ll:1`
  - 新增 `and i512 %x, -1` 回归输入。
- `external/NotDec-llvm2c/test/structuring/fixtures/wide_integer_load.ll:1`
  - 新增 `load i512` 回归输入。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:512`
  - 新增 `wide_negative_integer_literal` 和 `wide_integer_load` 两个 smoke case。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:44`
  - `goto_condensing_chain` 的 lighttpd xfail 文案改成 timeout，并设置 `timeout: 90`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:620`
  - `condensing_real_lighttpd` 同步设置 90 秒 timeout。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:680`
  - `run_case()` 给 `subprocess.run()` 传入 per-case timeout，并把超时分类为 `timeout`。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的旧 lighttpd per-function 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-wide-int-migration.csv`
  通过；CSV 统计为 14 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr external/NotDec-llvm2c/test/structuring/fixtures/wide_negative_integer_literal.ll -o /tmp/notdec-wide-negative.c`
  通过，输出包含 `unsigned _BitInt(512)` 和 `return (int)(x & ...);`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr external/NotDec-llvm2c/test/structuring/fixtures/wide_integer_load.ll -o /tmp/notdec-wide-load.c`
  通过，输出包含 `return (int)*(unsigned _BitInt(512) *)p;`。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-wide-int.c`
  通过；同口径结果：`elapsed=53.68 user=53.23 sys=0.43 maxrss=672812`。

## 影响判断

- 实现效果：2/5。修掉宽整数导致的 runner abort，lighttpd 前进到长时间运行 blocker。
- 复杂度：1/5。只补已有 `_BitInt(N)` fallback 的两个漏点和打印器兼容。
- 维护成本：1/5。后续 lighttpd 仍需单独看长跑原因；本次不扩大到类型恢复策略。

# 2026-06-28 P7 fmt dedup proxy 重新分类记录

继续核对 P7 migration 报告时发现 `fmt_deduplication_proxy` 的原检查太弱。当前输出虽然没有
`goto`，但仍然是：

- `if` 分支里一份 `xdectoumax(); return 0;`
- `else` 分支里一份 `xdectoumax(); return 0;`

这不代表 P1 `DuplicationReverter` 已经覆盖 Angr `test_fmt_deduplication` 的 dedup 语义。
真正期望是重复调用/return 被抽成一份公共结构。当前 shared CFG 还没有通用 merge graph，
直接实现需要新的相似子图和条件重接语义；这次先把报告改准，避免把 P1 缺口误算成 pass。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:255`
  - `fmt_deduplication_proxy` 增加 `expected_failure`，明确这是 P1 output gap。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:278`
  - 期望计数改为 `xdectoumax();` 出现 2 次（函数声明 1 次 + 实际调用 1 次），`return 0;`
    出现 1 次。当前输出仍是声明 1 次 + 调用 2 次、`return 0;` 2 次，因此稳定 xfail。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- 单独调用 `fmt_deduplication_proxy` 的 `run_case()`，结果为 `fail/output-mismatch`：
  `xdectoumax();` 期望 2 次，当前 3 次；`return 0;` 期望 1 次，当前 2 次。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-fmt-gap-migration.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，1 个 `xfail/expected-output-mismatch`。

本次只改 P7 报告分类，不改 `llvm2c` 或 SAILR pass；因此不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有新增算法能力，但把 P1 的真实输出缺口暴露出来。
- 复杂度：1/5。只收紧一个 migration proxy 的期望。
- 维护成本：1/5。后续实现 P1 merge graph 后，这个 xfail 应该转成 pass。

# 2026-06-28 P1 fmt dedup 相似语句边界记录

继续看 `fmt_deduplication_proxy` 时确认，当前缺口不是普通 exact duplicate block。
Angr `test_fmt_deduplication` 依赖 `DuplicationReverter` 里的 `is_similar()`、
`longest_ail_graph_subseq()` 和 shared conditional dominator；NotDec shared CFG 现在只有
payload origin 相等，没有“两个不同 call 语句语义相同”的共享接口。直接按渲染文本或按 CFG
形状合并会绕过 payload 语义，容易误合并手写重复分支。

本次不改运行时代码，只把这个边界固定下来：即使当前结构化结果有 goto hint，只要两边 payload
不同，`DuplicationReverter` 也不能把它当成 P1 修复。后续真正修 `fmt` 需要先补 shared
payload similarity / merge graph，而不是扩大 exact duplicate 规则。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4733`
  - 新增 `testDuplicationReverterKeepsDivergentBranchPayloadsWithGotoHint()`。
  - 覆盖两条 branch arm 都汇入同一 successor，且当前 goto 指向其中一边，但两边 statement
    payload 不同；期望 `DuplicationReverter` 不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14456`
  - 在测试入口调用新增 regression。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:255`
  - `fmt_deduplication_proxy` 的 expected failure 文案改成缺 Angr-style similar-statement
    merge graph。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `git diff --check`
  通过。
- `cmake --build ./build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-sim-boundary.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，1 个 `xfail/expected-output-mismatch`。

本次只改测试和报告文案，不改 `llvm2c` 运行时代码；因此不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有新增 merge graph 能力，但把 P1 的前置语义缺口说清楚，并防止后续用
  过宽 exact merge 冒充修复。
- 复杂度：1/5。只新增一个负例和一处报告文案。
- 维护成本：1/5。后续补 shared payload similarity 后，需要重新评估这个负例是否仍应保持。

# 2026-06-28 P1 简单 call payload origin 记录

上一节确认 `fmt_deduplication_proxy` 不能靠扩大 exact duplicate 规则修。继续往前推一小步：
给 C adapter 里非常简单的相同 call 语句建立 shared payload origin。范围只限直接 callee、
零参数的 `CallExpr`，不处理间接调用、带参数调用、表达式等价、返回值赋值，也不改变
`DuplicationReverter` 的候选搜索和 branch-arm 合并规则。

这样做的意义是给 P1 后续 merge graph / similar-statement 判断准备最小共享身份。当前
`fmt_deduplication_proxy` 仍然 xfail，因为它还需要 Angr 那类 shared conditional dominator
和 merge graph 重接；本次只补 payload 层材料。

## 修改位置

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:120`
  - 新增 `simpleCallExpr()`，只识别 `CallExpr` 和外层 `ExprWithCleanups` 包住的 call。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:130`
  - 新增 `directZeroArgCallee()`，只接受零参数、callee 去 cast 后是 `FunctionDecl` 的直接 call。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:162`
  - 新增 `SimpleCallPayloads`，按 `FunctionDecl*` 记录简单 call 的首个 payload。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:240`
  - 新增 `setSimpleCallOrigin()`，重复简单 call 复用首个 payload origin。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:581`
  - `buildCFG()` 收集 statement payload 后调用 `setSimpleCallOrigin()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4803`
  - 新增 `testDuplicationReverterExtractsCommonStatementTailByPayloadOrigin()`，
    覆盖不同 payload id 但同 origin 的 statement tail 能被抽成 shared tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14499`
  - 在测试入口调用新增 regression。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd per-function 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-call-origin.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，1 个 `xfail/expected-output-mismatch`。`fmt_deduplication_proxy`
  仍是预期的 P1 output gap。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-call-origin.c`
  通过；同口径结果：`elapsed=53.76 user=53.37 sys=0.38 maxrss=674044`。

## 影响判断

- 实现效果：2/5。补了 P1 相似语句前置材料，但没有实现 merge graph，所以 `fmt` 仍不通过。
- 复杂度：1/5。规则很窄，只按直接零参数 callee 复用 origin。
- 维护成本：1/5。后续如果扩展到带参数 call，必须先有参数 payload 等价和副作用边界。

# 2026-06-28 P1 branch common-tail 抽取记录

继续推进 P1 的保守 merge graph 子集。之前 common statement tail 只接受两个候选 block
的 predecessor set 不相交，因此同一个 branch predecessor 的 then/else 两边即使有相同
后缀，也不会抽成 shared tail。本次只放开一个窄形状：当前 goto source 本身是二分支，
goto target 是其中一边，另一边是唯一 sibling successor；两边 block 仍必须是 fallthrough、
同 control shape、无 dephication context、只有同一个 branch predecessor，且只抽共同
statement 后缀，不合并前缀和 branch 条件。

这仍不是完整 Angr merge graph。`fmt_deduplication_proxy` 的整 arm 合并还缺 shared
conditional dominator 和更一般的相似子图重接，所以本次后它仍保持 xfail。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1088`
  - 新增 `sameGotoBranchPredecessorPair()`，确认两个候选 block 只有同一个 branch
    predecessor，且这个 predecessor 就是当前 goto source。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1190`
  - 新增 `commonBranchStatementTailCandidate()`，复用 common statement tail 的 payload
    origin 比较和 block 安全条件，但允许上述同 branch predecessor 形状。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1487`
  - `revertGotoRelatedCommonStatementTail()` 先找 goto target 的 branch sibling，只对这个
    sibling 额外尝试 branch common-tail，避免对全图 block 做额外搜索。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4845`
  - 新增 `testDuplicationReverterExtractsGotoBranchCommonStatementTail()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4886`
  - 新增 `testDuplicationReverterKeepsBranchCommonStatementTailWithoutGotoHint()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14570`
  - 在测试入口调用两个新增 regression。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd per-function 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-branch-tail-tight.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，1 个 `xfail/expected-output-mismatch`。`fmt_deduplication_proxy`
  仍是预期的 P1 output gap。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-branch-tail-tight.c`
  通过；同口径结果：`elapsed=53.60 user=53.16 sys=0.43 maxrss=674584`。

## 影响判断

- 实现效果：2/5。补了同 branch predecessor 的 common-tail 子集，但仍不是完整 merge graph。
- 复杂度：2/5。新增一个额外候选形状，并把搜索限制到 goto target 的 sibling 以控制性能。
- 维护成本：2/5。后续如果扩到非 sibling 或整 arm 合并，必须先处理 condition/guard 重接。

# 2026-06-28 P7 branch common-tail 管线分类记录

继续把 P1 的最近改动放到 P7 migration 分类里核对。结论是：`DuplicationReverter` 的
branch common-tail 子能力已经有 pass 级 regression，但一个最小 LLVM IR 经过完整
`notdec-llvm2c --algo=structured-sailr` 时，还不会自然产生这个 pass 需要的 goto hint。
当前输出仍会在 then/else 两边各保留一份共同 call tail。

因此本次不把它写成通过的 smoke，而是加一个 expected-failure proxy，避免以后看报告时误以为
这个能力已经覆盖到真实管线。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:281`
  - 新增 `branch_common_tail_pipeline_proxy`。
  - IR 构造一个二分支，两边分别调用 `a()` / `b()` 后都有 `c()`，再回到 merge。
  - 期望 `c();` 只出现声明加一处公共调用；当前管线输出仍是声明加两处分支调用，所以该 case
    稳定归类为 `xfail/expected-output-mismatch`。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-branch-tail-pipeline.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，2 个 `xfail/expected-output-mismatch`。新增的
  `branch_common_tail_pipeline_proxy` 稳定归类为预期 output mismatch：`c();`
  期望 2 次，当前 3 次。

本次只改 P7 报告分类，不改 `llvm2c` 运行时代码；因此不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有新增算法能力，但把 pass 级覆盖和管线级覆盖的差距暴露出来。
- 复杂度：1/5。只新增一个 migration proxy。
- 维护成本：1/5。后续如果 structurer 能稳定提供该 goto hint，应把这个 xfail 转成 pass。

# 2026-06-28 P1 branch arm goto hint 消费记录

继续看上一个 `branch_common_tail_pipeline_proxy`。原因不是 common-tail 抽取本身完全不可用，
而是 pass 之前只接受 `goto source` 正好是 branch header 的形状。真实结构里更常见的是
goto 出现在某个 branch arm 的尾部，例如 arm -> merge/loop head。这次只补这一层消费能力：
当当前 goto source 是 branch 的私有 arm，且 source 确实有 CFG 边到 goto target 时，
`DuplicationReverter` 可以拿这个 arm 和 sibling arm 做 common statement tail 抽取。

这仍不是管线级修复。完整 `notdec-llvm2c` 的 migration proxy 仍保持 xfail，因为当前
structurer / goto 摘要还没有稳定把这个形状送到 pass，或者最终质量门没有接受该候选。
后续要继续看的是 goto 摘要生成和质量判断，而不是继续扩大 tail merge 的匹配范围。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1088`
  - 将 `sameGotoBranchPredecessorPair()` 收窄改名为 `sameBranchPredecessorPair()`，
    不再要求调用方传入的 goto source 就是 branch header。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1108`
  - 新增 `privateBranchSibling()`，只在目标 arm 有唯一 branch predecessor、且 sibling
    是同一 branch 的另一条 successor 时返回 sibling。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1507`
  - `revertGotoRelatedCommonStatementTail()` 新增 arm->target 形状：如果 `Goto.Source`
    到 `Goto.Target` 是真实 CFG 边，就以 `Goto.Source` 作为待拆 tail 的 base block，
    sibling 只来自 `privateBranchSibling()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4886`
  - 新增 `testDuplicationReverterExtractsBranchCommonTailFromArmGotoHint()`，覆盖
    `StructuredGoto{arm, merge}` 也能抽出 sibling arm 的共同 statement tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14645`
  - 在测试入口调用新增 regression。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的外部 lighttpd per-function 输入。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-arm-goto-tail.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，2 个 `xfail/expected-output-mismatch`。`branch_common_tail_pipeline_proxy`
  仍是预期 xfail。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-fortune-arm-goto-tail.c`
  通过；同口径结果：`elapsed=54.83 user=54.43 sys=0.39 maxrss=666332`。

本次改了 `llvm2c` 运行时代码，需要做 fortune 同口径性能 smoke。

## 影响判断

- 实现效果：2/5。pass 现在能消费更接近真实结构的 arm goto hint，但完整 pipeline xfail 还没解除。
- 复杂度：2/5。新增一个很窄的 sibling 推导，不引入条件重接或全图 merge graph。
- 维护成本：2/5。后续如果 pipeline 开始稳定触发这个形状，应把 migration xfail 转成 pass。

# 2026-06-28 P1 branch common-tail 回边边界记录

继续尝试把 `branch_common_tail_pipeline_proxy` 从 xfail 推到 pass 时，确认了一个更具体的边界：
proxy 里的 `merge` 会回到 `then` arm，导致待合并 arm 不是只有共同 branch predecessor。
如果简单放开这个多 predecessor 限制，会同时碰到 `reachesBlock()` 的循环可达性检查；这已经不再是
普通 common-tail split，而是需要 loop/backedge、condition 和质量判断一起参与的更大形状。

因此本次不继续扩大 `DuplicationReverter` 的 branch common-tail safety gate。当前保留的能力是：
没有回边额外 predecessor 时，pass 可以消费 `StructuredGoto{arm, merge}` hint；带回边的管线 proxy
仍归在 P1 merge graph / condition 重接未完成范围内。

## 修改位置

本次没有保留新的代码改动，只补日志分类。尝试过的回边扩展没有提交，因为它需要放开多 predecessor
和循环可达性两个安全门，已经超出当前窄 common-tail 修复。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `cmake --build ./build --target notdec-llvm2c structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。

本次没有保留运行时代码改动，不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有新增能力，但把 `branch_common_tail_pipeline_proxy` 的真实 blocker 进一步收窄。
- 复杂度：1/5。只更新日志。
- 维护成本：1/5。后续应从 loop/backedge 条件重接和质量门入手，不应直接放宽 common-tail 判断。

# 2026-06-28 P2 terminal fork 管线分类记录

转回 P2 的 `ReturnDuplicatorLow`。C++ 里已有 terminal fork return region 覆盖：
`branch -> return / unreachable` 可以作为 return region 被复制。但用一个最小 LLVM IR 经过
完整 `notdec-llvm2c --algo=structured-sailr` 时，默认管线仍会把两个 switch case 共同指到
同一个 fork，并留下 `goto structured_block_1` 一类共享 fork goto。

这说明 pass-level 能力和默认管线输出之间还有差距，可能和默认 quality gate、goto 摘要或
unreachable 渲染有关。本次先把它加入 migration 的 expected-failure 分类，不把它误算成
P2 已完成。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:319`
  - 新增 `terminal_fork_return_region_proxy`。
  - IR 构造两个 switch case 共享 `fork -> return / unreachable`。
  - 期望默认管线最终复制 `return 7;` 到两个 case，不留下 `goto fork` / `goto ret` /
    `goto trap`。当前输出仍只有一份 `return 7;`，因此归类为 expected output mismatch。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- 单独运行 `terminal_fork_return_region_proxy` 的 `run_case()`，结果为
  `fail/output-mismatch`：`return 7;` 期望 2 次，当前 1 次。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-terminal-fork-gap.csv`
  通过；CSV 统计为 13 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，3 个 `xfail/expected-output-mismatch`。新增的
  `terminal_fork_return_region_proxy` 稳定归类为预期 output mismatch。

本次只改 P7 报告分类，不改运行时代码；因此不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有新增算法能力，但把 P2 的 pass-level 和 pipeline-level 差距暴露出来。
- 复杂度：1/5。只新增一个 migration proxy。
- 维护成本：1/5。后续修默认管线后，应把这个 xfail 转成 pass。

# 2026-06-29 P2 terminal fork 默认管线修复

继续处理上面的 `terminal_fork_return_region_proxy`。问题不是单纯 pass-level
缺能力：`ReturnDuplicatorLow` 能复制 `branch -> return / unreachable`，但 C backend
给 LLVM `unreachable` 降出来的是空的无后继 CFG block，shared CFG 里没有被当作闭合终端；
另外 Phoenix 初始 goto 可能只落在一个 switch case 跳板上，另一个同 switch case 跳板也跳到同一个
fork，但不会被 `PredsToUpdate` 自动扩进去。

本次做的是窄修复：

- 只把 C backend 中“无后继、无语句、无 terminator”的空 block 标为 shared CFG 的
  `Unreachable`，对应 LLVM `unreachable` 降出的 trap 空块。
- `ReturnDuplicatorLow` 对 terminal fork leaf 增加同样的空 leaf 识别，复制这种 leaf 时把副本规范成
  `Unreachable`，让后续 structuring 仍把它视为闭合终端。
- 当 return region 包含 unreachable terminal，且已选 goto predecessor 是某个 switch case 的私有
  fallthrough 跳板时，把同一个 switch 下、同样私有跳到 region head 的 sibling case 跳板一起复制。
  这只覆盖当前 abnormal terminal fork 形状，不放宽普通 switch case return duplication。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:235`
  - 新增 `isClosedTerminalForkLeaf()`，让 terminal fork 收集能识别空的 closed leaf。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:242`
  - `prependTerminalForkRegion()` 改用 `isClosedTerminalForkLeaf()` 判断 fork 两侧。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1677`
  - 新增 `privateSwitchParentForJumpTo()`、`returnRegionContainsUnreachableTerminal()` 和
    `expandToSiblingSwitchJumpPredecessors()`，只在 unreachable terminal region 下扩展同 switch 私有
    case 跳板。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2748`
  - `materializeDuplicatedRegion()` 复制空 terminal fork leaf 时，把副本标成 `Unreachable`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:3801`
  - `ReturnDuplicatorLow::runOnGraph()` 在 connected predecessor component 后追加 sibling switch
    case jump 扩展。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:588`
  - C backend shared CFG adapter 只把空的无后继无 terminator block 标成 `Unreachable`，避免扩大到普通
    无后继 block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:779`
  - 新增 `makeSwitchCaseTerminalForkFunction()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8238`
  - 新增 `testReturnDuplicatorLowAcceptsSwitchCaseTerminalForkByDefault()`，覆盖默认 options 下的
    switch case terminal fork 复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8301`
  - 新增 `testSAILRDeoptimizationPipelineCopiesSwitchCaseTerminalFork()`，覆盖完整 deopt pipeline。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:360`
  - `terminal_fork_return_region_proxy` 去掉 expected failure，转成正常 pass 用例。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test notdec-llvm2c -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- 单独运行 `terminal_fork_return_region_proxy` 的 `run_case()`：
  `status=pass`，`classification=pass`，输出指标为 `switch_count=1`、`case_count=2`、
  `goto_count=0`、`return_count=3`。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的 lighttpd per-function fixture。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-terminal-fork-narrow.csv`
  通过；CSV 统计为 14 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，2 个 `xfail/expected-output-mismatch`。
  `terminal_fork_return_region_proxy` 为 `pass/pass`。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-terminal-fork-narrow.c`
  通过；同口径结果：`elapsed=52.40 user=52.00 sys=0.39 maxrss=672512`。

本次改了 SAILR 运行时代码和 C backend adapter，fortune smoke 没看到同口径退化；上次同类记录为
`elapsed=54.83 user=54.43 sys=0.39 maxrss=666332`。

## 影响判断

- 实现效果：3/5。P2 terminal fork 的默认管线 proxy 已从 xfail 转成 pass，但一般
  `_single_entry_region()` 枚举仍未完整复刻。
- 复杂度：2/5。只新增 terminal fork 和 switch case 私有跳板的窄规则，没有引入通用 region graph。
- 维护成本：2/5。后续如果要覆盖普通多入口 return region，应该继续补 `_single_entry_region()`，不要把本次
  unreachable terminal fork 规则扩成通用 predecessor 合并。

# 2026-06-29 P1 duplicate branch arm 窄合并

本次继续推进 P1，但没有放宽到完整 Angr `AILMergeGraph`。先补一个安全子集：
同一个 branch 的两个私有 successor 在当前结构化结果里已经和 goto 相关，且两个 arm 的控制形状、
payload origin、successor 都相同时，`DuplicationReverter` 可以删掉其中一个 arm，把 branch 改成
fallthrough 到保留 arm。这样覆盖“结构化已经暴露复制痕迹”的重复分支，不处理 no-goto 的普通源码重复。

`fmt_deduplication_proxy` 这次重新确认后仍保留 xfail：这个代理没有初始 goto，而 Angr
`DuplicationReverter` 默认 `require_gotos=True`；真实 `fmt` 还涉及前置 `ConstPropOptReverter`、
candidate search 和 `ReturnDeduplicator` 的配合。这里不为了代理把无 goto 分支合并放开。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1137`
  - 新增 `hasGotoTouchingBranchPair()`，要求本次重复 arm 合并必须和当前 goto 结果相关。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1153`
  - 新增 `canMergePrivateBranchArm()`，只接受同一 branch 的两个私有 successor，并复用
    `sameBlockShapeByReference()` 比较 payload origin 和控制形状。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1162`
  - 新增 `canDiscardBranchCondition()`，只允许空条件或已有 shared compare metadata 的条件被删除，避免丢掉
    不透明条件副作用。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1170`
  - 新增 `revertGotoRelatedDuplicateBranchArms()`，把重复 branch arm 合并成一个 fallthrough arm。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:3714`
  - `DuplicationReverter::runOnGraph()` 在 common-tail 规则前先尝试这个更窄的 exact branch arm 合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4812`
  - 新增 `testDuplicationReverterMergesGotoRelatedDuplicateBranchArms()`，覆盖有 goto 触发、payload origin
    相同的两个私有 arm 合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14782`
  - 把新回归接入 `structuring-analysis-test`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:255`
  - 更新 `fmt_deduplication_proxy` 的 xfail 理由，明确剩余是 no-goto candidate search 和
    `ReturnDeduplicator` 交互，不再简单写成“缺 merge graph”。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test notdec-llvm2c -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的 lighttpd per-function fixture。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-branch-arm-exact.csv`
  通过；CSV 统计为 14 个 `pass/pass`，2 个 `skip/missing-input`，2 个
  `xfail/expected-timeout`，2 个 `xfail/expected-output-mismatch`。
  `fmt_deduplication_proxy` 仍为预期 output mismatch。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-branch-arm-exact.c`
  通过；同口径结果：`elapsed=52.50 user=52.08 sys=0.40 maxrss=674708`。

本次改了 SAILR 运行时代码，fortune smoke 没看到同口径退化；上一轮记录为
`elapsed=52.40 user=52.00 sys=0.39 maxrss=672512`。

## 影响判断

- 实现效果：2/5。补了 P1 一个可验证子集，但 `fmt_deduplication_proxy` 和完整 Angr merge graph 仍未完成。
- 复杂度：2/5。只新增一个 exact branch arm 合并入口，没有引入 merge graph。
- 维护成本：2/5。规则依赖 goto 触发、私有 arm 和 shared compare metadata；后续扩展 no-goto
  candidate search 时需要先重新评估 `DuplicationReverter` 的 `RequireGotos` 语义。

# 2026-06-29 P1 ReturnDeduplicator 窄子集

继续看 `fmt_deduplication_proxy` 时确认，上一节解决的不是当前输出里的重复 return。
Angr full preset 在 `ReturnDuplicatorLow` 后还有 `ReturnDeduplicator`，会把两个分支末尾相同的
return 收回到共享 return。NotDec 之前没有这个 pass，导致 `ReturnDuplicatorLow` 复制出的
`return 0;` 留在两个 arm 里。

本次补 shared CFG 的窄子集：只处理同一个 branch 的两个私有 successor，二者都是 terminal return
block，且最后一条 return payload 的 origin 相同。实现把两边末尾 return payload 移到一个 synthetic
return block，两边 arm 保留自己的前缀语句并 fallthrough 到这个共享 return。这个规则不处理普通
multi-block return dedup，也不处理 void return。

为避免默认管线在大量无匹配函数上多做一次 structuring 评估，pass wrapper 加了两个保守选项：
`MaxInputBlocks` 在大图上直接跳过，`EvaluateInputBeforeRun=false` 只让能先做便宜图匹配的 pass
延迟输入评估。`ReturnDeduplicator` 真正改图后仍会评估改前和改后 CFG，并继续执行不新增 goto
检查。

`fmt_deduplication_proxy` 现在从“重复 call + 重复 return”缩到只剩重复 call；因此 xfail 继续保留，
但理由改成缺 Angr-style no-goto candidate search。

## 修改位置

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:59`
  - 新增 `ReturnDeduplicator` pass 声明。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:23`
  - 新增 `EvaluateInputBeforeRun`，让便宜图匹配 pass 可以先判断是否有改动。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:27`
  - 新增 `MaxInputBlocks`，大图在评估前跳过小图 cleanup pass。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:51`
  - `needsInitialEvaluation()` 尊重 `EvaluateInputBeforeRun`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:87`
  - `analyze()` 在 `MaxInputBlocks` 超限时直接返回；延迟输入评估的 pass 只有改图后才补评估原图。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:98`
  - 新增 `HasInitialEvaluation` 状态，避免延迟输入评估时拿空的初始 goto 集做验收。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:3998`
  - 新增 `ReturnDeduplicator::defaultOptions()`，不要求已有 goto，不做相对质量 gate，延迟输入评估，
    但仍禁止新增 goto，并限制 500 个 block 以内。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4009`
  - 新增 `canDeduplicateReturnArm()`，只接受 branch 私有 terminal return arm。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4021`
  - 新增 `sameTailReturnPayload()`，按 payload origin 判断两个 return payload 是否相同。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4029`
  - 新增 `ReturnDeduplicator::runOnGraph()`，抽出两个 arm 的末尾 return 到共享 synthetic return block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4282`
  - `buildSAILRDeoptimizationPipeline()` 在 `ReturnDuplicatorLow` 后、`CrossJumpReverter` 前加入
    `ReturnDeduplicator`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:114`
  - 新增 `TestReturnDeduplicator` 测试 wrapper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8962`
  - 新增 `testReturnDeduplicatorSharesDuplicateBranchReturns()`，覆盖相同 return payload 的共享。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9002`
  - 新增 `testReturnDeduplicatorKeepsDivergentBranchReturns()`，覆盖不同 return 不合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12072`
  - 新增 `testStructuringOptimizationPassSkipsLargeInputBeforeRunning()`，覆盖 `MaxInputBlocks` 在
    `runOnGraph()` 前生效。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12108`
  - 新增 `testStructuringOptimizationPassDelayedInputEvaluationKeepsInitialGotos()`，覆盖延迟输入评估仍使用
    真实初始 goto 集。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12325`
  - 更新 SAILR pipeline 顺序断言，加入 `ReturnDeduplicator`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:12412`
  - 更新默认 option 对齐测试，覆盖 `ReturnDeduplicator`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:255`
  - 更新 `fmt_deduplication_proxy` 的 xfail 理由。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:845`
  - 更新 direct diamond return region 的 `return 7;` 计数，接受共享 return 后的输出。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:1286`
  - 更新 switch diamond return tail 的 `return 9;` 计数，接受共享 return 后的输出。

## 验证

- `git -C external/NotDec-llvm2c diff --check`
  通过。
- `git diff --check`
  通过。
- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py`
  通过。
- `cmake --build ./build --target structuring-analysis-test notdec-llvm2c -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过；仍有现有的 `--target=wasm32-wasi` deprecated 警告。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；当前机器仍跳过缺失的 lighttpd per-function fixture。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-return-dedup.csv`
  通过；CSV 统计为 14 个 `pass`，4 个 `xfail`，2 个 `skip`。
  `fmt_deduplication_proxy` 现在 `return_count=1`，只剩 `xdectoumax();` 次数 mismatch。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-return-dedup-lazy.c`
  通过；同口径结果：`elapsed=53.97 user=53.60 sys=0.36 maxrss=666064`。

## 影响判断

- 实现效果：2/5。补齐 Angr full preset 里的一个窄 `ReturnDeduplicator` 子集，`fmt` proxy 只剩重复
  call，但完整 P1 candidate search 仍未完成。
- 复杂度：3/5。pass 本身只处理两个私有 return arm，但为了避免 fortune 退化，wrapper 增加了延迟
  输入评估和 block 上限两个选项。
- 维护成本：3/5。规则依赖 shared payload origin；`EvaluateInputBeforeRun=false` 只能给先做便宜图
  匹配的 pass 用，后续新增使用者必须保证改图后仍补原图和候选图评估。

# 2026-06-29 P7 fmt proxy 计数修正

继续核对 `fmt_deduplication_proxy` 时发现，它在上一节后生成的函数体已经只有两个
`xdectoumax();` 调用，失败原因是 migration 脚本的普通 `counts` 把函数声明
`extern void xdectoumax();` 也算进去了。这不是 P1 算法缺口。

本次不改 SAILR 算法，只给 migration case 增加函数体范围计数，让这个 proxy 按真实函数体输出
计数，并移除它的 `expected_failure`。P1 剩余缺口仍是 `branch_common_tail_pipeline_proxy`
这一类需要 full pipeline 产生 goto hint 或更完整 merge graph 的形状。

同类检查也覆盖了 `branch_common_tail_pipeline_proxy`。它的 `c();` 计数同样会被函数声明干扰，
但真实问题不是调用次数，而是输出仍有 7 个 `goto structured_block_*`。因此这个 case 继续
保留 xfail，但失败分类改为 `expected-unexpected-goto`，更准确地指向 P1 的 pipeline 结构缺口。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:252`
  - `fmt_deduplication_proxy` 移除 `expected_failure`，把 `xdectoumax();` 改成 `body_counts`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:281`
  - `branch_common_tail_pipeline_proxy` 把 `c();` 改成 `body_counts`，并明确要求无
    `goto structured_block_`，让 xfail 分类落到真实 goto 缺口。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:716`
  - 新增 `function_body()`，从 `Function Definitions` marker 后开始计数。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:808`
  - `run_case()` 支持 `body_counts`，用于避免函数声明干扰输出计数。

## 验证

- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- 单独运行 `fmt_deduplication_proxy` 的 `run_case()`：
  通过；`status=pass`，`return_count=1`。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-fmt-oracle.csv`
  通过；CSV 统计为 15 个 `pass`，3 个 `xfail`，2 个 `skip`。
  `fmt_deduplication_proxy` 为 `pass/pass`，`branch_common_tail_pipeline_proxy` 仍是 P1 预期
  `expected-unexpected-goto`。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-branch-common-tail-oracle.csv`
  通过；CSV 统计为 15 个 `pass`，3 个 `xfail`，2 个 `skip`。
  `branch_common_tail_pipeline_proxy` 为 `xfail/expected-unexpected-goto`。
- 单独运行 `branch_common_tail_pipeline_proxy` 的 `run_case()`：
  预期 xfail；分类为 `expected-unexpected-goto`，当前输出仍有 7 个 goto。

## 影响判断

- 实现效果：1/5。修正 P7 分类，确认 `fmt_deduplication_proxy` 不再是当前缺口。
- 复杂度：1/5。只扩展 migration oracle 的计数范围。
- 维护成本：1/5。`body_counts` 只影响显式使用它的 case，普通 `counts` 不变。

# 2026-06-29 P1 branch common-tail 无 hint 路线排除记录

继续检查 `branch_common_tail_pipeline_proxy`。真实 C CFG 是：

- `B0` 按 `x == 0` 分到 `B2(a(); c())` 和 `B1(b(); c())`。
- `B1`、`B2` 都到 `B3`。
- `B3` 按 `y == 0` 回到 `B2` 或到 `B4(return 0)`。

也就是说，`B2` 多了来自 `B3` 的回边。尝试把 `B1/B2` 的共同语句尾 `c();` 在没有
goto hint 的情况下抽成 shared synthetic tail，pass 级 `runOnGraph()` 能改图，但放进正式
`DuplicationReverter::analyze()` 后，结构化结果的 goto 数从 7 增到 9，被
`PreventNewGotos` 正确拒绝。这个方向不能作为修复提交。

后续不应直接放宽 common-tail safety gate，也不应给 loop 回边形状硬开 no-hint 抽尾。
`branch_common_tail_pipeline_proxy` 的真实缺口仍是 P1 的 loop/backedge 条件重接、
virtual edge 选择或更完整 merge graph 语义；修复标准仍是完整 pipeline 输出减少 goto，
而不是只让 `DuplicationReverter::runOnGraph()` 局部改图。

# 2026-06-29 P7 migration timeout 复用记录

继续跑 `run_sailr_bench2_migration.py` 时发现两个 lighttpd 真实样例当前指向同一个
`module-all.ll`，并且都是预期 90s timeout。脚本之前会重复跑两次，导致一次完整 migration
报告要多等 90s。

本次只给脚本增加同输入、同参数、同 timeout 的 runner failure 缓存。缓存只保存 timeout /
command failed / missing output 这类 runner 结果；成功输出不缓存，仍让每个 case 独立检查
自己的 `contains`、`absent`、`counts` 和 `body_counts`。这样不会把一个 case 的 oracle 误用到
另一个 case。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:747`
  - `run_case()` 增加 `run_cache` 参数。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:763`
  - 用 `(input_path, args, timeout)` 作为 runner 缓存 key；命中时返回 cached runner failure，
    并在失败信息里标出 `reused cached result from same input`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:793`
  - timeout、非零退出和缺输出文件写入缓存；普通输出 mismatch 不缓存。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:870`
  - `main()` 为本次临时工作目录创建一次 `run_cache`。

## 验证

- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-cache2.csv`
  通过；CSV 统计保持 `15 pass`、`2 skip`、`3 xfail`。
  第二个 lighttpd case `condensing_real_lighttpd` 复用第一个 lighttpd timeout 结果，失败信息包含
  `reused cached result from same input`。

本次只改 P7 报告脚本，不改 `llvm2c` 运行时代码；不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。减少重复 timeout 等待，让 P7 报告更快出结果，但不增加算法能力。
- 复杂度：1/5。缓存只在单次脚本进程内生效，key 只覆盖 runner 命令输入。
- 维护成本：1/5。成功输出不缓存，避免不同 case oracle 相互污染。

# 2026-06-29 P1 branch common-tail goto 数量基线记录

继续检查 `branch_common_tail_pipeline_proxy` 时试过在 Phoenix cleanup 里删除无条件
控制转移后的不可达 sequence 节点。这个方向可以把该 proxy 的输出从 7 个 goto 降到
5 个 goto，但规则过宽，会删掉 `linear_while_break`、`shared_synthetic_goto_switch_reuse`
和 `root_cycle_follow` 需要的 fallback 控制流，`run_structuring_smoke.py` 会失败。因此本次不提交
这条 runtime 清理。

本次只把当前缺口的 goto 数量写进 migration oracle，避免后续只看到
`unexpected-goto`，看不出是否有局部进展。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:316`
  - 给 `branch_common_tail_pipeline_proxy` 增加 `expected_metrics: {"goto_count": 7}`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:834`
  - `run_case()` 先计算一次 `metrics`，再按 case 里的 `expected_metrics` 校验具体指标。

## 验证

- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-expected-metrics.csv`
  通过；CSV 统计为 `15 pass`、`2 skip`、`3 xfail`。
  `branch_common_tail_pipeline_proxy` 仍为 `xfail/expected-unexpected-goto`，`goto_count=7`。

本次只改 migration 报告脚本，不改 `llvm2c` runtime；不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。没有增加算法能力，但把 P1 当前输出质量固定成可比较指标。
- 复杂度：1/5。只给显式 opt-in 的 case 校验现有 metrics。
- 维护成本：1/5。默认 case 不受影响；以后若算法减少 goto，报告会直接暴露基线变化。

# 2026-06-29 P7 缺失真实输入的 proxy fallback

`run_sailr_bench2_migration.py` 里 `return_tail_cleanup` 和 `early_exit_chain`
依赖本机当前没有的真实 IR，之前完整报告会留下 2 个 `skip/missing-input`。这会让
ReturnDuplicatorLow 的两个早期覆盖点在当前机器上完全不跑。

本次不替代真实样例判断，只给这两个 case 加 proxy fallback：真实输入存在时仍跑真实
IR；真实输入缺失时才写临时 IR，并在 CSV 的 `kind` 标成 `proxy-fallback`，同时在
`failures` 字段记录原真实路径缺失。这样当前报告能实际跑完这些语义代理，也不会把
proxy 误标成 real。

## 修改位置

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:22`
  - `return_tail_cleanup` 增加早退 + tail return 的 fallback IR，并给 fallback 设置独立
    `fallback_contains`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:48`
  - `early_exit_chain` 增加两个条件共用失败 return 的 fallback IR。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:737`
  - `case_kind()` 支持 `proxy-fallback`。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:784`
  - `run_case()` 在真实输入缺失且 case 有 `fallback_ir` 时写临时 fallback IR，并返回
    `used_fallback` 和 note。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:858`
  - fallback 运行时优先使用 `fallback_contains`，避免真实 IR oracle 和 proxy oracle 混用。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:933`
  - CSV 行使用 `case_kind(case, used_fallback)`，并把 fallback note 写进 `failures` 字段。

## 验证

- `python3 -m py_compile external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-fallback.csv`
  通过；CSV 统计为 `17 pass`、`3 xfail`、`0 skip`。
  `return_tail_cleanup` 和 `early_exit_chain` 在当前机器上为 `pass/pass/proxy-fallback`，
  并记录真实输入缺失路径。

本次只改 migration 报告脚本，不改 `llvm2c` runtime；不需要 fortune 性能 smoke。

## 影响判断

- 实现效果：1/5。补齐当前机器上的 P7 报告覆盖，但不增加算法能力，也不替代真实样例。
- 复杂度：1/5。fallback 只在显式提供 `fallback_ir` 且真实输入缺失时启用。
- 维护成本：1/5。CSV 明确区分 `real` 和 `proxy-fallback`，后续真实输入出现时自动回到 real。

# 2026-06-29 P2/P3 switch return shared return 收口

这次继续补 `ReturnDeduplicator` 的一个窄子集。之前它只接受一个 branch 的两侧
return arm。Angr full preset 里 `ReturnDeduplicator` 还能把 switch 下多个私有 return
arm 的相同尾 return 收成一个共享 return。本次只补这个同类共享，不碰更宽的 return
region 枚举，也不改 `ReturnDuplicatorLow` 的复制逻辑。

约束仍然保守：

- 只接受同一父块下的 private return arm。
- arm 必须只有该父块一个前驱。
- arm 末尾 return payload origin 必须相同。
- 继续跳过 case/default overlap 和重复 successor target。

这样能把一部分 `ReturnDeduplicator` 的 full preset 子集补上，同时不把 switch 复用
和 P1 common-tail 之类的结构混进去。

## 修改位置

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4026`
  - 新增 `deduplicateReturnArmGroup()`，把一组 return arm 抽成共享 synthetic return。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:4058`
  - `ReturnDeduplicator::runOnGraph()` 先保留 branch 两臂的旧路径，再补 switch 多臂返回
    的窄共享。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9044`
  - 新增 `testReturnDeduplicatorSharesDuplicateSwitchCaseReturns()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:15000`
  - 在测试入口注册新用例。

## 验证

- `cmake --build ./build --target structuring-analysis-test notdec-llvm2c -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
  通过；仍只跳过机器缺失的 lighttpd fixture。
- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --report-csv /tmp/notdec-sailr-switch-return-dedup.csv`
  通过；统计保持 `17 pass`、`3 xfail`、`0 skip`，未引入新失败。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/selected-targets-native/fortune/executable/module-all.ll -o /tmp/notdec-sailr-switch-return-dedup.c`
  通过；`elapsed=52.71 user=52.25 sys=0.45 maxrss=675440`，和上一轮同口径没有明显退化。

本次是 runtime structuring 改动，保留了 `DuplicateBranchReturns` 的旧行为，只扩了 switch
窄子集。

## 影响判断

- 实现效果：2/5。补了 `ReturnDeduplicator` 的一个 switch return 收口子集。
- 复杂度：2/5。只抽了一个小 helper，还是偏窄。
- 维护成本：2/5。增加了一个 switch-case return 回归，但规则还是保守。

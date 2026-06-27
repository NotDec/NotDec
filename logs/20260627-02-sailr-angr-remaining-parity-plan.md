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
| `StructuringOptimizationPass`，`optimization_pass.py:443-654` | SAILR during-region pass wrapper，负责初始 structuring、goto guard、固定点、失败回滚、相对质量检查。 | 基本覆盖。NotDec 在 `StructuringOptimizationPass.h:16-24` 保存同类 option，在 `StructuringOptimizationPass.cpp:62-130` 做 evaluate、rollback、goto/quality guard。 | P6 继续核对 quality 细节，特别是 copied block 和 virtual goto 的统计是否和 Angr 一致。 |
| `SwitchDefaultCaseDuplicator`，`switch_default_case_duplicator.py:20-166` | 基于 jump table 找 default case 复用；多 switch 共享 default 时写成 goto；外部 predecessor 复用时复制 default block。 | 部分覆盖。NotDec 在 `SAILRDeoptimization.cpp:1893-2042` 处理已有 shared switch default，支持 synthetic goto / forwarder，也能复制线性 default region。还没有 Angr 那种直接消费 CFG jump table metadata 的入口。 | P5：接入 recovered switch / jump-table 元数据，区分 default-only、case-only、交叉复用。 |
| `DuplicationReverter`，`duplication_reverter.py:36-180`、`655-730`、`1056-1175` | 从 goto 周边找候选，构造 `AILMergeGraph`，按相似语句/子图拆分公共部分，再重接 predecessor、successor、jump target。 | 部分覆盖。NotDec 在 `SAILRDeoptimization.cpp:2044-2125` 覆盖 exact duplicate、common statement tail、linear region tail、copied-prefix shared tail，但还没有通用 merge graph、条件重建和非 tail 拆分。`20260627-01` 属于这个 pass 的 copied-prefix tail 子项。 | P1：先做保守 single-entry / single-exit DAG merge graph，不碰循环和宽表达式等价。 |
| `SwitchReusedEntryRewriter`，`switch_reused_entry_rewriter.py:20-132` | 基于 jump table entry，发现多个 switch head 复用同一 case entry 时，为后续 head 建 virtual goto，不复制 entry。 | 部分覆盖。NotDec 在 `SAILRDeoptimization.cpp:1701-1787` 基于 shared switch case edge 建 synthetic goto，保留最低 id 的 entry，已有 reuse limit。缺口仍是 jump-table/recovered switch metadata 和复杂 default/case 混用。 | P5：和 default 复用共用 shared switch 表示，明确 default-only 不进 case-entry 逻辑。 |
| `LoweredSwitchSimplifier`，`lowered_switch_simplifier.py:143-260`、`413-939` | 识别 `==` / `!=` 链和范围比较树，收集 case/default，生成 incomplete switch head，并处理 shared case node。 | 名称相同但语义差距大。NotDec 在 `SAILRDeoptimization.cpp:1789-1891` 现在主要复制已有 switch 的共享 case 线性 region，并不是从 lowered if-chain 恢复 switch。 | P4：实现真正 if-chain/range-tree 到 shared switch 的恢复。 |
| `ReturnDuplicatorLow` / `ReturnDuplicatorBase`，`return_duplicator_low.py:18-171`、`return_duplicator_base.py:69-220`、`219-660` | 从 end node 反推 single-entry return region，按 goto edge 和 connected predecessor component 复制，复制时处理 Phi、fresh vvar、label、删除原 region。 | 部分覆盖。NotDec 在 `SAILRDeoptimization.cpp:2137-2252` 能复制线性 return tail、branch/diamond/fork 的一部分、grouped predecessor 和部分 payload。差距是 Angr 的通用 endnode region 枚举、call limit、Phi/vvar 全量处理。 | P2 + P3：先补一般 single-entry return region，再补 Phi/vvar/copied payload 全量消费。 |
| `CrossJumpReverter`，`cross_jump_reverter.py:15-107` | 最后运行；对只有一个 goto 的块，复制目标的单 successor 线性块；限制调用数，要求 goto 数下降。 | 部分覆盖但方向接近。NotDec 在 `SAILRDeoptimization.cpp:2254-2414` 复制线性 region，支持 switch case/default edge kind 拆分和 grouped predecessor，但限制用 statement 数，不是 Angr 的 call counter。 | P6/P7：核对成本限制和真实样例行为。 |
| `ConstPropOptReverter`，`const_prop_reverter.py` | SAILR/DREAM 共享的前置去常量传播 pass，用于让后续相似性更容易成立。 | 暂未实现，且不在当前 shared CFG deoptimization pipeline。 | 暂不放 P1-P5 主线；P7 真实样例如果显示它是主因，再单独写计划。 |
| `ReturnDuplicatorHigh`、`ReturnDeduplicator` | SAILR/DREAM 共享外围 pass，不是当前 low-level SAILR deoptimization 主差距。 | 暂不适用。NotDec 当前目标是 shared CFG 级 pass parity。 | 只记录，不作为本计划完成条件。 |

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
    linear tail、copied-prefix tail、switch predecessor redirect。
  - `ReturnDuplicatorLow` 覆盖 goto return target、grouped predecessor、
    parent goto source、nested/diamond/fork/branch return region 的一部分。
  - `SwitchDefaultCaseDuplicator` 覆盖 shared default goto、forwarder、linear tail、
    grouped predecessor、rollback。
  - `SwitchReusedEntryRewriter` 覆盖 reused case entry、limit、default-only skip。
  - `LoweredSwitchSimplifier` 目前覆盖已有 switch case target 复用，不覆盖 if-chain
    switch recovery。
  - `CrossJumpReverter` 覆盖 linear goto target、grouped predecessor、case/default
    edge kind。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py`
  当前只有 5 个真实/半真实样例，分别代理 ReturnDuplicatorLow、CrossJumpReverter、
  LoweredSwitchSimplifier 的一部分。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py`
  覆盖 angr dephication Phi、multi-Phi、copied switch region，以及 SAILR/Phoenix
  对比样例。

测试分类：

| 类别 | 当前结论 |
| --- | --- |
| 可直接迁移 | Switch default/reused-entry 的小图行为、CrossJumpReverter 线性目标、ReturnDuplicatorLow 简单 return tail。已有一部分在 `structuring_analysis_test.cpp`。 |
| 需要 IR/payload 代理 | `DuplicationReverter` merge graph、ReturnDuplicatorLow Phi/vvar、LoweredSwitchSimplifier if-chain。Angr 测试基于 AIL 和真实 binary，NotDec 需要 shared CFG proxy 或 Bench2 IR。 |
| 暂不适用 | Angr `ConstPropOptReverter`、`ReturnDuplicatorHigh`、`ReturnDeduplicator` 的非 shared-CFG 主线测试。除非真实样例证明它们阻塞 P1-P5，否则先不算本计划完成条件。 |

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

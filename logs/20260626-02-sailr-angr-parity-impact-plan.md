# 原始 prompt

> 形成一个新的logs/文档，规划如何改进使得进一步接近SAILR的算法，按照对结果的影响程度排序，先处理影响最大的

# 背景

当前 llvm2c 里的 SAILR 已经不是空壳。它已经有 Phoenix 变体、recursive structuring、virtual edge heuristic、optimization pipeline、trial / quality guard，以及一组 SAILR deoptimization pass。

现在的主要问题不是“有没有 SAILR”，而是几个 pass 只实现了 Angr 语义的保守子集。影响最大的差距集中在：

- `DuplicationReverter` 只合并完全相同的单 block，Angr 会做相似子图合并。
- `ReturnDuplicatorLow` 主要覆盖线性 return region，Angr 会从 end node 往前找一般 single-entry return region，并处理 Phi / vvar。
- `LoweredSwitchSimplifier` 当前主要处理已经存在的 switch case 复用，Angr 会把 if-chain 重新识别成 switch。
- switch default / reused-entry 现在只覆盖已有 `Switch` terminator 的简单共享目标，Angr 还利用 switch-case 构造和 jump-table 语义做更多改写。

dephication 默认模式切换本身不是核心差距。它影响入口选择，但真正影响结果质量的是 copied region 的值来源、payload rewrite、Phi / vvar 映射能否支持上面这些 pass。

# 目标

按对反编译结果的影响程度推进 SAILR Angr 对齐，先处理最能减少 goto、重复代码、错误 switch 结构的部分。

目标不是一次性把 Angr 所有细节照搬，而是先补最影响输出质量的语义：

1. shared CFG 能表达“相似子图合并”和“复制后 payload 变化”。
2. `DuplicationReverter` 从完全相同 block 合并，升级到保守的相似 region / subgraph 合并。
3. `ReturnDuplicatorLow` 覆盖一般 return region，并正确处理 Phi / vvar / copied payload。
4. `LoweredSwitchSimplifier` 能识别 lowered if-chain switch，而不只是复制已有 switch 的共享 case。
5. switch default / reused-entry 的虚拟 goto、default 复用、case 复用更接近 Angr。

# 优先级排序

## P0：先补 DuplicationReverter 的相似子图合并

这是影响最大的差距。

当前 NotDec 的 `DuplicationReverter` 只处理两个 block 形状完全一样的情况。它不会把两个相似 region 拆开，也不会提取中间共同语句再合并。结果是很多 compiler cross-jump / shared-tail 结构仍会保留 goto，或者保留两份接近但不完全相同的代码。

Angr 的核心是：

- 围绕 goto 找候选。
- 在候选子图中找最长相似 AIL 序列。
- 把 block / subgraph 拆成不相同前缀、相同主体、不相同后缀。
- 构造一个 merge graph。
- 重新插入 merge graph，并修正 jump / condition / edge。
- 失败时回滚，不能破坏原图。

NotDec 第一阶段不需要一次做到 Angr 的全部表达式相似性。建议先做保守版：

1. 候选仍围绕当前 `GotoManager` 的 goto edge 找，不全图搜索。
2. 先支持单入口、单出口、无循环的小 subgraph。
3. 先用 payload 层的 statement identity / normalized text / stable hash 找最长公共序列。
4. 只在读写依赖简单、statement 不能移动也能匹配时合并。
5. 合并后的 CFG 改动必须走现有 optimization trial / quality guard。

判断标准：

- 能处理“两个 region 有共同中间 tail，但前后有少量差异”的样例。
- 合并后 goto 数减少或结构质量提升。
- 不因为相似判断过宽导致语义错。
- 失败候选能干净回滚。

## P1：完整化 ReturnDuplicatorLow 的 return region 复制

这是第二大影响点。return tail 共享非常常见，处理好以后会明显减少 goto return。

当前 NotDec 主要靠 `findLinearReturnRegion()` 找线性 region，并依赖当前 structuring 结果里的 goto edge。这个覆盖面比 Angr 小。

要改成更接近 Angr：

1. 从所有 end node 往前找 single-entry return region，而不是只找线性 tail。
2. 支持 branch return region 和多 block return region。
3. 按 connected predecessor component 分组复制，避免复制太多。
4. 复制 region head 时处理 Phi：按当前 predecessor 选择对应 incoming。
5. 从原 region 删除 predecessor 时，同步删掉原 Phi incoming。
6. copied block 使用 fresh vvar / fresh payload，不能复用原 block 的变量身份。
7. 保留 max block / max call / max statement 这类成本限制。

这一步依赖 `logs/20260626-01-sailr-angr-dephi-mode-plan.md` 里的 shared Phi / vvar / payload rewrite 能力，但不应被“默认模式切换”阻塞。可以先在旧 demote 路线下补 region 查找和复制边界，再把新 dephication 模式接进去。

判断标准：

- 能处理非线性 return tail。
- copied return region 的值来源正确。
- 同一个 return block 被不同 predecessor 复制后，payload 可以不同。
- 不需要 C / Solidity renderer 特判 Phi 或 copied block。

## P2：实现真正的 LoweredSwitchSimplifier

这是第三大影响点。它主要影响 switch 输出质量。

当前 NotDec 的 `LoweredSwitchSimplifier` 名字接近 Angr，但实际做的是已有 switch case 目标复用时复制线性 case region。它没有从 if-chain 反推 switch。

Angr 会识别这类 lowered switch：

```c
if (x == 1) goto case_1;
if (x == 5) goto case_5;
if (x > 10) goto default;
...
```

然后构造一个 switch head，把 case value、case target、default target 都挂回 switch-case 语义。

NotDec 建议分两步做：

1. 先支持最常见的 `x == const` / `x != const` 链。
2. 再支持 `x < const`、`x <= const`、`x > const`、`x >= const` 形成的范围拆分。

判断标准：

- 能把简单 if-chain 还原成 switch。
- case value、default target、fallthrough 不错。
- 不把普通 if-else 误识别成 switch。
- case/default 复制仍走 shared CFG，不放到 renderer 里补。

## P3：补 SwitchDefaultCaseDuplicator / SwitchReusedEntryRewriter 的剩余语义

这部分影响比前三项小，但会影响复杂 switch 的可读性。

当前 NotDec 已经能处理一部分已有 switch 的 shared default 和 reused entry。差距主要是：

- Angr 会结合 switch-case construct / jump-table 信息识别更多 default 复用。
- reused-entry 里 Angr 更偏 virtual goto 语义，不一定真的复制 entry。
- default 和普通 case 交叉复用时，NotDec 的分类还偏保守。

路线：

1. 明确 switch case construct 的 shared 表达，避免只看 `TerminatorKind::Switch`。
2. default 复用优先创建 synthetic goto / virtual goto，不急着复制大 region。
3. reused entry 保留“哪个 switch predecessor 应该直连，哪个应该虚拟跳转”的信息。
4. 和 P2 的 lowered switch 结果共用同一套 case/default 表示。

判断标准：

- 多个 switch 共用同一 default 时，输出不再互相缠绕。
- 多个 case 共用 entry 时，不产生错误 fallthrough。
- default-only 复用不会被误当成普通 reused case。

## P4：补 pass wrapper、options、质量评估对齐

这部分不会单独大幅改变输出，但会决定上面几个 pass 是否稳定。

要对齐：

- pass 顺序。
- `require_gotos`
- `prevent_new_gotos`
- `strictly_less_gotos`
- `must_improve_rel_quality`
- `max_opt_iters`
- function/block/call cost limit
- rollback 和 retry 规则

判断标准：

- 每个 SAILR pass 都能对应到 Angr 的类 / 方法。
- 每个默认 option 差异都有记录。
- 质量 guard 不会接受“goto 更多、结构更差”的候选。

## P5：dephication 默认模式切换

这个优先级最低，因为默认开关本身不是算法能力。

要做的不是简单把默认值改成 Angr mode，而是等 P1 需要的 Phi / vvar / copied payload 语义稳定后，再考虑默认切换。

判断标准：

- Angr dephication mode 至少覆盖 copied return region 和 switch copied region。
- 旧 demote 模式和新模式的差异可解释。
- 默认切换后现有主要 smoke 不退化。

# 总体实施顺序

1. 先做 P0 的保守相似子图合并，因为它最直接影响 SAILR 核心效果。
2. 再做 P1 的 return region 复制，因为 return tail 共享出现频率高。
3. 然后做 P2 的 lowered switch 识别，因为它能明显改善 switch 输出。
4. 接着补 P3 的复杂 switch default / reused-entry。
5. 最后统一 P4 options 和 P5 默认模式。

每一步都先用小样例锁语义，再上真实样例 smoke。不要直接靠大 corpus 跑通来证明正确。

# 测试和性能判断

每个阶段至少需要：

- 一个最小正例。
- 一个不应触发的反例。
- 一个 rollback / rejected candidate 样例。
- 一个 C backend smoke。
- 如果涉及 Solidity payload，也加 Solidity smoke。

性能上，涉及 structuring pass pipeline 的改动至少要对比当前关注的 fortune 用例同口径耗时。只影响 EVM Solidity/storage rewrite 时才改用 EVM smoke；本计划主要影响 llvm2c structuring，所以优先 fortune。

# 风险

最大风险是 P0 的相似判断过宽。只要 statement 相似但值来源不同，就可能合并错。第一版必须保守，宁可少合并，不要错合并。

第二个风险是 P1 的 copied payload。复制 CFG 节点容易，复制后的值来源才是关键。如果 copied block 仍复用原 payload，输出可能看起来更结构化，但语义已经错。

第三个风险是 P2 的 switch 误识别。普通 if-chain 和 lowered switch 很像，必须用 case 数、同一变量、default 可达性、连续范围等规则限制。

第四个风险是 pass 互相影响。P0 合并、P1 复制、P2 switch 还原都会改 CFG。每个 pass 必须用候选图提交，不能原地半改。

# 阶段完成条件

P0 完成：

- `DuplicationReverter` 能合并至少一种非完全相同的相似 subgraph。
- 新增测试覆盖成功合并、拒绝合并、rollback。
- 输出 goto 数或结构质量相对当前有明确改善。

P1 完成：

- `ReturnDuplicatorLow` 能处理一般 single-entry return region。
- copied region 的 Phi / vvar / payload 来源正确。
- 删除原 region 或保留原 region 时，边和 payload 都一致。

P2 完成：

- if-chain 能还原成 switch。
- case/default/fallthrough 语义稳定。
- 不误伤普通 if-else。

P3 完成：

- shared default 和 reused entry 的复杂样例能稳定输出。
- virtual goto / synthetic goto / copied region 的选择有明确规则。

P4/P5 完成：

- pass options 和 Angr 差异有清单。
- 默认 dephication 模式切换有测试支撑。
- fortune 同口径耗时无明显退化。

## 2026-06-26 实现记录：P0 保守公共 tail 提取

这轮先完成 P0 的一个保守子集，没有做 Angr 完整的 split / merge graph。
实现只处理“当前 structuring 结果已经有 goto 指向某个 block，且这个 block
和另一个 block 有相同 statement 后缀”的情况。两个 block 的控制流形状必须
一致，且必须是原始、已 materialize、无 dephication 上下文的 block。公共
后缀会被提成一个新的 shared tail block，两个原 block 保留各自不同前缀并
fallthrough 到 shared tail。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:54`：
  新增 `sameBlockControlShape()`，让公共 tail 提取只比较 terminator、
  condition、successor 和 switch case，不要求 statement 完全相同。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:376`：
  新增 `hasDephicationContext()`，避免在 Phi / vvar edge block 上移动 payload。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:383`：
  新增 `canSplitCommonStatementTailBlock()`，限制第一版只拆原始 materialized
  input block，不碰 copied / synthetic / dephication block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:398`：
  新增 `commonStatementSuffixLength()`，按 payload id 找公共 statement 后缀。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:412`：
  新增 `commonStatementTailCandidate()`，要求有非完整公共后缀、前驱集合不交叉、
  两个 block 之间没有直接边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:440`：
  新增 `extractCommonStatementTail()`，创建 SAILR deoptimization shared tail，
  并把两个原 block 改成各自前缀加 fallthrough。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:488`：
  新增 `revertGotoRelatedCommonStatementTail()`，只围绕 `Current.Gotos` 里的
  goto target 找候选，不做全图搜索。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1565`：
  `DuplicationReverter::runOnGraph()` 先尝试公共 tail 提取，失败后保留原来的
  exact duplicate block merge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3919`：
  新增 `testDuplicationReverterExtractsGotoRelatedCommonStatementTail()`，覆盖
  `101,201` 和 `102,201` 被拆成两个前缀加一个 shared `201` tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3961`：
  新增 `testDuplicationReverterKeepsCommonStatementTailWithoutGotoHint()`，确认
  没有 goto hint 时不触发，避免把普通手写重复代码也改掉。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9348`：
  注册两个新增测试。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
cmake --build build --target notdec-llvm2c-exe -j4
cmake --build build --target notdec -j4
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-sailr-dup-tail.c --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- structuring 相关 CTest 5 项通过。
- `run_structuring_smoke.py` 通过。
- fortune smoke 通过，当前本地 Debug + ASan 口径为 `elapsed=200.93`。近期
  同日志口径约 200-210 秒，本轮没有明显退化。

当前完成度：

- P0 已支持一种“非完全相同 block”的保守相似合并形状。
- 还没有实现 Angr 完整的 `AILMergeGraph`、statement move、子图 LCS、
  condition block 合成和复杂 split / reinsert。

## 2026-06-26 实现记录：P0 候选再收窄到线性无环 tail

上面那版公共 tail 提取再收窄了一次，避免把带回边或分叉的 shape 当成可分
shared tail。现在候选块必须满足：

- 原始 input block。
- `BodyMaterialized == true` 且 `BodyBlock == Id`。
- `Terminator == Fallthrough`。
- `Successors.size() == 1`。
- 不存在 dephication 上下文。
- 两个候选 block 之间没有可达关系，避免把同一条链上的不同位置误当成
  相似兄弟块。
- 不再“找到第一个公共后缀就合并”，而是从所有候选里挑公共后缀最长的
  那个。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:376`：
  让 `canSplitCommonStatementTailBlock()` 同时检查 linear tail 形状。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:412`：
  `commonStatementTailCandidate()` 额外拒绝互相可达的候选对。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:488`：
  `revertGotoRelatedCommonStatementTail()` 先遍历所有候选，再挑最长公共后缀。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3961`：
  新增 `testDuplicationReverterSkipsCommonTailWhenRegionIsNotLinear()`，用
  一个带回边的 branch 候选确认不触发。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9349`：
  注册新的反例测试。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：

- `structuring-analysis-test` 通过。
- `run_structuring_smoke.py` 通过。

当前完成度：

- 这个 P0 子集现在只覆盖“有 goto hint 的线性公共 tail”。
- 还没碰 Angr 那条真正的相似子图合并、LCS、语句移动和 merged condition
  graph。

## 2026-06-26 实现记录：P1 先放宽 ReturnDuplicatorLow 的 region gate

这轮先把 `ReturnDuplicatorLow` 从“必须有 predecessor rewrite hook 才看 branch/switch return region”改回按 region 直接识别。因为 Angr 的 low return duplicator 本来就是从 end node 往回找单入口 return region，再决定要不要复制，是否支持 payload rewrite 只影响复制时的材料化，不该挡住 region 发现。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1693-1700`：
  `ReturnDuplicatorLow::runOnGraph()` 里把 `AllowBranchReturnRegion` 改成始终开启，保留前驱筛选和 payload rewrite 判定，但不再用 hook 作为 region 发现前置条件。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4810-5065`：
  重新整理 `testReturnDuplicatorLowCopiesBranchReturnRegionWithPayloadRewrite()` 和 `testReturnDuplicatorLowCopiesSwitchReturnRegionWithoutPredecessorRewriteSupport()`，用一条有 predecessor rewrite hook 的 branch 正例和一条无 hook 的 switch 正例覆盖返回 region 复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4927-5066`：
  新增 `testReturnDuplicatorLowCopiesBranchReturnRegionWithoutPredecessorRewriteSupport()`，确认 branch return region 在没有 predecessor rewrite hook 时也会被复制。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-sailr-returndup.c --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- `run_structuring_smoke.py` 通过。
- fortune smoke 通过，当前本地 Debug + ASan 口径为 `elapsed=201.73`，和前一轮同口径基本持平，没有明显退化。

当前完成度：

- `ReturnDuplicatorLow` 现在按 Angr 更接近的方式先找 return region，再决定是否复制。
- 还没补 Phi / vvar 的更细复制语义，但 region 识别门槛已经比原来少了一层不必要的 hook 依赖。

## 2026-06-26 实现记录：P0 线性 tail 合并改成缓存版

前一版把 `findLinearRegionFromHead()` 直接放进候选双层循环后，`fortune` 同口径时间涨到
`391.39s`，回退太明显。这一轮把线性 tail 合并收回到缓存版，只在 `GotoManager`
命中的候选周围做一次 region 计算，再复用缓存结果找最长公共后缀，保留“有 goto hint
才动图”的语义，但不再每个候选都重复扫图。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:115-130`：
  新增 `MaxLinearRegionMergeBlocks` 和 `cachedLinearRegion()`，给线性 region 候选加本轮缓存。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:497-684`：
  `commonLinearRegionTailCandidate()`、`extractCommonLinearRegionTail()` 和
  `revertGotoRelatedCommonLinearRegionTail()` 改成缓存驱动；仍然只围绕 `Current.Gotos`
  的 goto target 找候选，并保留单入口、单出口、无环、小 region 和 rollback 限制。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1796-1808`：
  `DuplicationReverter::runOnGraph()` 继续优先尝试线性 tail 合并，再回到旧的
  statement tail / exact duplicate merge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4021-4130`：
  新增 `testDuplicationReverterMergesGotoRelatedLinearRegionTail()` 和
  `testDuplicationReverterSkipsGotoRelatedLinearTailWithoutHint()`，分别覆盖有 goto hint 的
 线性 tail 合并和无 hint 的反例。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9292-9308`：
  把新测试注册进 `main()` 固定回归入口。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-sailr-parity.c --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- `run_structuring_smoke.py` 通过。
- fortune smoke 通过，`elapsed=200.98`。

当前状态：

- 这版把之前的性能回退基本收回来了。
- 这仍然不是 Angr 的完整 merge graph，只是更保守的 goto 相关线性 tail 合并。

## 2026-06-26 实现记录：ReturnDuplicatorLow 补 parent-aware goto source 和 dephication return 回归

这轮没有继续扩大 P1 的 region 形状，只补了一层更贴近 Angr 的 goto 源点判断，并把
`ReturnDuplicatorLow` 的 copied return / dephication vvar 语义再钉了一条回归。
`gotoEdgeFromSourceOrParent()` 现在会先看当前 source 是否直接命中 goto，再回看一层
非 branch 前驱；这样 `DuplicationReverter` 和 `ReturnDuplicatorLow` 都能吃到“真实 goto
在上层块”的窄边界。与此同时，新测试确认 copied return region 在 materialize 时能拿到
fresh vvar / dephication incoming，上下文里会分别走 `DephicationAssignment` 和普通
`Statement` 路径。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:102-104`：
  新增 `gotoEdgeFromSourceOrParent()` 前置声明，给后面的 goto 相关 pass 共用。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:643-646`：
  `revertGotoRelatedCommonLinearRegionTail()` 改成用 parent-aware goto 判断筛候选。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:753-756`：
  `revertGotoRelatedCommonStatementTail()` 同步改用同一个 helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:949-990`：
  `gotoEdgeFromSourceOrParent()` 先看直连 goto，再看一层非 branch 前驱，最后再沿
  return tail 做线性回退。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1938-1990`：
  `ReturnDuplicatorLow::runOnGraph()` 改成先过 parent-aware goto 过滤，再做 predecessor
  分组和复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4783-4871`：
  新增 `testReturnDuplicatorLowCopiesReturnRegionWithDephicationVVars()`，覆盖 copied return
  region 的 dephication vvar 复制和 payload rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9534`：
  把新测试接入 `main()`。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-sailr-parent-goto.c --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- `run_structuring_smoke.py` 通过。
- fortune smoke 通过，当前本地 Debug + ASan 口径为 `elapsed=200.44`。

当前完成度：

- 这轮只补了 parent-aware goto 源点和一条 copied return / dephication 回归。
- `ReturnDuplicatorLow` 还没补 Angr 那层更完整的 return region / Phi / vvar 一般语义。

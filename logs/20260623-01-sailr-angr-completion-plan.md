# 原始 prompt

写成一个新的规划文件，同时给出明确的计划完成条件

# 背景

当前 SAILR 复刻已经过了搭框架阶段。shared structuring 的基础边界基本稳定：

- `OverlayManager` / `RegionOverlay`
- parent-visible graph
- checkpoint / rollback
- recursive structuring
- cyclic refinement
- virtual edge
- structuring trial
- goto quality guard
- optimization pipeline

Phoenix 已经迁到 shared reducer 路线。SAILR 已经作为 Phoenix 变体接入，并有 shared deoptimization pipeline。

已经落地的 SAILR deoptimization 子集包括：

- `ReturnDuplicatorLow`
- `CrossJumpReverter`
- `DuplicationReverter`
- `LoweredSwitchSimplifier`
- `SwitchDefaultCaseDuplicator`
- `SwitchReusedEntryRewriter`

最近几轮主要在补 shared CFG 的复制身份和回滚边界：

- copied / synthetic block 有独立 `BlockId`
- copied block 记录 `BodyBlock`、`Origin`、`SourceBlock`、`CopyKind`、`CreatedBy`
- copied block materialize 走 shared `StructuredCFG::materializeBlockBody()`
- deoptimization pass 统一成候选图提交
- copy helper 失败路径改成事务式回滚

现在的问题不是框架不够，而是 Angr SAILR 的完整语义还没补完。特别是 copied region 的 Phi / vvar / payload rewrite 还没有 shared 表达能力，所以不能继续靠 C 或 Solidity renderer 复用原 block body 来假装复制正确。

# 目标

把当前 SAILR 从“保守子集可跑”推进到“接近 Angr 完整语义”。

核心目标：

1. copied / virtual block 的身份、payload、successor / predecessor、switch case、goto source-target 都在 shared CFG / shared structured tree 里表达。
2. `ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、switch default / reused-entry 相关 pass 尽量对齐 Angr。
3. 所有 pass 都统一接入 `StructuringOptimizationPipeline`，用 shared structuring trial 和 quality guard 决定是否接受。
4. C 和 Solidity 后端只消费 shared 结果，不参与 structuring 算法决策。

非目标：

- 不为了短期输出好看，在 C renderer 或 Solidity renderer 里加 copied block 特判。
- 不在 Phi / vvar / payload 语义不清楚时扩大复杂 region copy。
- 不用 renderer fallback 掩盖 structuring 失败。

# 路线

## 阶段一：补 shared payload materialize 能力

先把 `StructuredCFG::materializeBlockBody()` 从简单复制 payload id，推进成 copied block payload rewrite 的统一入口。

要做：

- 定义 materialize context，至少包含 source block、copy block、original predecessor、new predecessor、copy kind、created-by pass。
- 明确 statement、branch condition、switch condition、switch case value 的 materialize 规则。
- 支持 payload clone / rewrite hook，但不把具体 C / Solidity AST 类型放进 structuring 层。
- 保证 materialize 不覆盖 copied block 的 CFG 身份：successor、predecessor、switch target、goto target 仍按 copy block 维护。

判断标准：

- copied block 的 body payload 可以按 copy 身份生成新 payload。
- 复制后 case target / default target / goto target 不退回原 block。
- C 和 Solidity 后端看到同一份 shared materialize 结果。

## 阶段二：补 Phi / vvar rewrite 的 shared 表达

这是扩 `ReturnDuplicatorLow` 的前置条件。

要做：

- 梳理当前 payload 里 Phi、recovered value、virtual variable 的表示方式。
- 如果 payload 仍是 opaque id，就给 structuring 层提供 backend-neutral rewrite callback。
- copied region 进入不同 predecessor component 时，可以生成不同 payload。
- 删除 original region 或 redirect predecessor 后，可以更新剩余 Phi incoming。

判断标准：

- 能构造一个最小分支 return-region：两个 predecessor 进入同一个 return tail，复制后两个 copy 的值来源不同。
- 这个差异由 shared materialize / rewrite 表达，不由 renderer 猜。
- 回滚失败时不会泄露已经 clone 的 payload 或半改 CFG。

## 阶段三：完整化 ReturnDuplicatorLow

在 payload rewrite 能力足够后，再扩 Angr `_single_entry_region()` 和相关 return duplication 语义。

要做：

- 补一般 branch return-region。
- 补 connected in-edge component 的完整分组逻辑。
- 补 copied region 内部 edge / external edge 的完整重写。
- 删除 original region 前继续使用候选图和 remove probe。
- 所有改图都由 optimization wrapper 做 trial / quality guard。

判断标准：

- `ReturnDuplicatorLow` 不再只覆盖 straight-line / terminal fork / goto tail 子集。
- 新增形状有 Phi / payload rewrite 测试。
- 未覆盖形状明确跳过，不产生半改图。

## 阶段四：完整化 switch deoptimization

优先顺序：

1. `LoweredSwitchSimplifier`
2. `SwitchDefaultCaseDuplicator`
3. `SwitchReusedEntryRewriter`

要做：

- 支持更长 case/default tail。
- 支持 case/default 交叉复用。
- 对齐 Angr reused-entry 中 virtual goto 相关语义。
- 继续区分 case target 和 default target，避免 default-only 误进 reused-entry。

判断标准：

- switch case value、case target、default target 在复制和重定向后都稳定。
- default forwarder / copied case region 是 shared CFG 的显式节点。
- 不需要 renderer 侧判断“这是 switch deopt 复制出来的块”。

## 阶段五：对齐 pass 顺序和 options

当前 pipeline 已经可跑，但还要继续和 Angr 对照。

要做：

- 对齐 pass 顺序。
- 对齐每个 pass 的默认 options。
- 对齐 block limit、max iteration、require gotos、prevent new gotos、strictly less gotos、relative quality guard。
- 每个差异都写清楚原因。

判断标准：

- `buildSAILRDeoptimizationPipeline()` 里的每个 pass 都能对应到 Angr 的类 / 方法。
- NotDec 保留的差异都能解释为 IR / payload 表达差异，而不是漏实现。

## 阶段六：真实样例分类

不要直接用大 corpus 判断成功。先选小批样例做分类。

分类维度：

- SAILR 比 Phoenix 好。
- SAILR 比 Phoenix 差。
- SAILR pass 没触发。
- SAILR trial 拒绝。
- copied block / return duplication / switch deoptimization 相关。

每个差异归因到：

- Angr 语义未复刻。
- shared CFG / payload 表达能力不足。
- renderer 能力不足，但算法结果已经正确。

判断标准：

- 能解释主要差异来源。
- 不用 renderer fallback 掩盖 structuring 失败。
- fortune 或更合适样例没有明显性能退化。

# 风险

最大风险仍然是 Phi / vvar / payload rewrite。

如果 copied block 只复用原 block body，小测试会容易通过，但复制后的值来源可能已经错了。这个问题必须在 shared 层解决。

第二个风险是 reused-entry 的语义取舍。当前 NotDec 有些实现用复制 entry region，Angr 有些路径更偏 virtual goto。这里如果影响输出质量，需要单独记录对比，不能静默改。

第三个风险是性能。SAILR deoptimization 会增加复制、trial 和 rollback。扩大 pass 前后要继续用同口径样例看耗时。

# 明确完成条件

只有同时满足下面条件，才算“接近 Angr 完整语义”的计划完成。

## 语义完成条件

1. copied / virtual block 在 `StructuredCFG` 中都有独立 `BlockId`。
2. copied block 明确记录 source、body-source、copy kind、created-by pass。
3. copied block 的 statements、condition、switch case value 可以通过 shared materialize / rewrite 生成。
4. copied block 的 successor、predecessor、switch case target、goto source-target 都按 copied `BlockId` 维护。
5. Phi / vvar / payload rewrite 不依赖 C 或 Solidity renderer。
6. `ReturnDuplicatorLow` 覆盖 Angr 主要 return duplication 形状，包括一般 branch return-region。
7. `CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、switch default / reused-entry 相关 pass 的主要 Angr 语义都落到 shared CFG。
8. 每个 pass 都通过 `StructuringOptimizationPipeline` 的 trial / quality guard 接受，不直接绕过 shared guard。

## 测试完成条件

1. `structuring-analysis-test` 覆盖 copied payload rewrite、Phi / vvar rewrite、return duplication、cross jump、duplication revert、lowered switch、default reuse、reused-entry。
2. 每个新增复杂形状至少有一个失败回滚测试。
3. C 和 Solidity 后端至少各有一个 copied / virtual block 渲染 smoke，证明它们消费同一份 shared tree。
4. CTest structuring subset 通过：

```bash
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

5. fortune 或同等级真实样例 smoke 通过，并记录同口径耗时。

## 对照完成条件

1. 日志中列出每个 SAILR deoptimization pass 对应的 Angr 类 / 方法。
2. 对每个和 Angr 不一致的行为，写清楚是 NotDec IR 差异、payload 表达不足、还是刻意保守。
3. 对不能复刻的语义点，有明确的暂停记录和后续方案。

## 工程完成条件

1. 算法层不包含 C renderer / Solidity renderer 特判。
2. renderer 不承担 structuring fallback 语义。
3. 所有 copied region 改图都具备候选图提交或事务式回滚。
4. 涉及 submodule 的改动先在 `external/NotDec-llvm2c` 内提交，再提交顶层指针和日志。
5. 顶层工作树除任务外的既有脏状态不被误改。

# 下一步建议

下一步不要先扩更多 matcher。先做一个最小 payload rewrite 设计和测试：

1. 构造一个 copied block 需要按 predecessor 生成不同 payload 的 shared 单测。
2. 给 `materializeBlockBody()` 增加 backend-neutral rewrite hook 或 materialize context。
3. 用这个能力补一个小的 branch return-region。
4. 再继续扩 `ReturnDuplicatorLow`。

这条路比直接堆 case 更慢一点，但更接近 Angr 的真实语义，也不会把 renderer fallback 当成算法成功。

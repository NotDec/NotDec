# 原始 prompt

写成一个新的规划文件，同时给出明确的计划完成条件

是的，Phi指令一定要按照这个方式处理，把我的原话放到logs/20260623-01-sailr-angr-completion-plan.md顶部。

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

# 2026-06-23 实现记录：阶段一 payload materialize 入口

本次完成了阶段一的 shared payload materialize 入口，并用一个受 hook 保护的 branch
return-region 测试验证扩展方向。总计划还没有完成，Phi / vvar rewrite、完整 Angr
pass 对照和真实样例分类仍需后续继续做。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:62`
  新增 `PayloadMaterializeKind`、`PayloadMaterializeContext` 和
  `PayloadMaterializeHook`。context 记录 source block、body block、copy block、
  original/new predecessor、copy kind、created-by pass，避免 structuring 层依赖
  C 或 Solidity AST 类型。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:145`
  给 `StructuredCFG` 增加 `setPayloadMaterializeHook()`、
  `hasPayloadMaterializeHook()` 和带 predecessor context 的
  `materializeBlockBody()` overload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:142`
  实现 hook 注册和查询。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:162`
  保留无 hook 快路径，生产路径行为不变；有 hook 时分别重写 statements、
  branch condition、switch case value，全部写入临时 buffer，成功后才提交，失败不留下半改 payload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:300`
  修正 `duplicateRegion()` 失败回滚：只删除本次新建 copy block，不再用
  `removeBlock()` 逐个删，避免 copy id 已经出现在原图边里时误改原图边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:189`
  新增 `collectClosedLinearReturnTail()` 和 `prependBranchReturnRegion()`，
  识别一个保守的一般 branch return-region：一个分支继续走原 return head，
  另一个分支是闭合线性 return tail。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1110`
  branch return-region 只在 `Graph.hasPayloadMaterializeHook()` 为真时启用。
  这样当前生产后端没有真实 payload rewrite 时不会扩大复制范围。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:500`
  和 `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:657`
  copy helper 调用新的 `materializeBlockBody()` overload，给 payload rewrite
  传入 predecessor context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:758`
  新增 copied payload rewrite 测试，覆盖 statements、condition、switch case value。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:819`
  新增 rewrite 失败原子性测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2203`
  新增带 payload rewrite hook 的 `ReturnDuplicatorLow` branch return-region 复制测试。

## 验证

构建和单测：

```bash
cmake --build ./build --target structuring-analysis-test notdec -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

结果：全部通过，CTest structuring subset 为 5/5 通过。

性能 smoke 使用同一个 fortune 用例：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-sailr-materialize-branch-fastpath.c \
  --tr-level=2 --algo=structured-sailr
```

本次改动后：`elapsed=196.55 user=218.80 sys=1.78 maxrss=1266720`。

stash 掉 `external/NotDec-llvm2c` 改动并重建后的同口径 baseline：
`elapsed=200.10 user=222.16 sys=1.67 maxrss=1264960`。

结论：这次 shared materialize 入口没有可见性能退化。当前环境下
`structured-sailr` fortune baseline 约 200 秒，和旧日志里的 84.64 秒不是同一口径。

## 当前判断

实现效果：7/10。阶段一的入口和回滚边界已经落到 shared CFG，测试覆盖了 payload
rewrite、失败原子性和一个 branch return-region 形状；但还没有接真实 C/Solidity payload
clone，也没有完成 Phi / vvar rewrite。

复杂度：6/10。新增 hook 和 context 会增加理解成本，但它把复制 payload 的问题放在
`StructuredCFG::materializeBlockBody()` 一个入口里，比在 renderer 里分散特判更可控。

维护成本：6/10。无 hook 快路径保持旧行为，branch return-region 也被 hook gate 保护；
后续主要维护点是定义真实 payload clone 的所有权和失败清理规则。

更好的方案暂时没有明显成立。直接扩大 return-region copy 会绕过 payload 语义，风险更高；
把 clone 逻辑放进 C/Solidity renderer 会让两个后端各自背 structuring 语义，也不符合目标。

# 2026-06-23 实现记录：真实后端接入 payload materialize hook

这轮把前一版的 shared payload materialize 入口接进了真实后端链路，并把
`ReturnDuplicatorLow` 的 branch return-region gate 拆成前驱感知能力位。目标还是
同一个：让 copied block 的 payload rewrite 真正落到 shared CFG，而不是只留在测试里。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:145`
  给 `setPayloadMaterializeHook()` 增加可选的
  `SupportsPredecessorRewrite` 标记，并新增
  `hasPredecessorRewritePayloadMaterializeHook()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:142`
  保存这个能力位。普通 payload clone 和 predecessor-aware rewrite 现在分开记。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1110`
  `ReturnDuplicatorLow` 只在 predecessor-aware hook 打开时才放行一般 branch
  return-region，避免普通 clone hook 误扩大复制形状。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:40`
  C 的 shared structuring 链路现在安装 payload materialize hook。这里先做保守的
  payload id 复制，让 copied block 在 shared CFG 里能拿到新 id。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:280`
  Solidity 链路也安装同类 hook，字符串 payload 会生成新的 id，后续 copied
  block 不再直接复用原 payload 位置。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2203`
  新增 predecessor-aware branch return-region 测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2368`
  新增负例测试，确认只有普通 clone hook 时 branch return-region 不会被打开。

## 验证

构建：

```bash
cmake --build ./build --target notdec structuring-analysis-test -j4
```

结果：通过。

测试：

```bash
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

结果：通过，CTest subset 5/5。

性能 smoke：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-sailr-stage2.c --tr-level=2 --algo=structured-sailr
```

结果：`elapsed=205.86 user=228.62 sys=1.66 maxrss=1260704`。和前一轮 196.55s、
200.10s 同一量级，没有看出这次接 hook 带来新的明显退化。

## 当前判断

实现效果：8/10。现在真实 C/Solidity 链路都能走 shared payload materialize hook，
branch return-region 也被更窄的能力位保护住了。copied block 的 payload 复制不再只靠测试桩。

复杂度：6/10。多了一个能力位，但逻辑仍在 `StructuredCFG` 和 `SAILRDeoptimization`
两处收口，没有把判断散到 renderer 里。

维护成本：6/10。后续如果真要做 predecessor 级 rewrite，只需要在 hook 里补具体规则；
目前普通 clone 和 predecessor-aware rewrite 已经分层。

仍未完成的部分没有变：Phi / vvar 的真实语义还没补，return duplication 也只是
一般 branch 形状的最小放行，不是完整 Angr 对齐。

# 2026-06-23 实现记录：Solidity 入口统一 Phi demote

这次把 Solidity 输出前的入口也接到了同一条 demoteSSA 路线上。这样 shared
structuring 还是保持“结构恢复前不处理 Phi”的旧约定，同时 HType 也会在
demote 前后按 Phi 名字迁到 `.reg2mem` alloca 上。

## 修改内容

- `src/Passes/PassManager.cpp:53`
  在 `#ifdef NOTDEC_ENABLE_LLVM2C` 下补入 `notdec-llvm2c/StructuralAnalysis.h`，
  让顶层 pass 可以直接复用 `demoteSSA()` / `demoteSSAFixHT()`。
- `src/Passes/PassManager.cpp:193`
  在 `MLsubNotdecSolidity::run()` 里，调用 Solidity backend 前先执行
  `demoteSSAFixHT()`；如果没有类型恢复结果，就退回 `demoteSSA()`。
  这样 Solidity 侧和 C 侧一样，结构恢复入口看到的都是已经去掉 Phi 的 IR。

## 验证

- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-sailr-phi-demote.sol --tr-level=2 --work-dir=/tmp/notdec-sailr-phi-demote-work --gen-work-dir`

结果：构建通过，smoke 通过，`llvm2c-before-demotessa.ll` 里还能看到 Phi，
`llvm2c-after-demotessa.ll` 里已没有 Phi，只剩 `.reg2mem` 相关的 load/store。

# 2026-06-23 实现记录：copied region 的 predecessor-aware materialize context

这次补的是 shared CFG 复制时的 context 语义。之前 copy helper 虽然会调用带
predecessor 参数的 `materializeBlockBody()`，但对 region 内所有 copy 都传同一个
外部 predecessor。这样后续做 predecessor-sensitive payload rewrite 时，内部 copied
block 不知道自己来自哪个原 predecessor，也不知道对应的新 copied predecessor。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:497`
  新增 `materializeDuplicatedRegion()`，统一给 copied region 内每个 block 计算
  `OriginalPredecessor` 和 `NewPredecessor`。region head 使用外部 predecessor；
  region 内部 block 使用原内部 predecessor 和对应 copy。多前驱或无法唯一判断时继续传
  `InvalidBlockId`，保持保守。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:545`
  `copyRegionForPredecessors()` 改用这个 helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:702`
  `copyLinearRegionForPredecessors()` 也改用同一 helper，避免 Return / switch deopt
  两条复制路径语义分叉。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2230`
  加强 `testReturnDuplicatorLowCopiesBranchReturnRegionWithPayloadRewrite()`：
  payload hook 现在要求 predecessor context 非空，并用 `NewPredecessor` 生成 payload，
  验证 region head、then/else tail、return block 都拿到正确的新前驱。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pred-context.c --tr-level=2 --algo=structured-sailr`

结果：构建和 structuring subset 都通过。fortune smoke 结果：
`elapsed=194.84 user=217.12 sys=1.71 maxrss=1257804`，和前面 196-205 秒同口径，
没有看到明显性能退化。

## 当前判断

实现效果：7/10。copied region 的 materialize context 已经能表达单前驱的真实
source-target 身份，后续做 payload rewrite 不再只能看到 copy block id。

复杂度：5/10。只多了一个 shared helper，两个 copy helper 复用同一套规则。

维护成本：5/10。多前驱仍保守传 `InvalidBlockId`，没有提前承诺复杂 Phi/vvar rewrite。

# 2026-06-24 实现记录：switch default-tail materialize context 测试

这次没有改算法，只补测试覆盖。上一轮已经让 `copyLinearRegionForPredecessors()` 走
predecessor-aware materialize context，但测试主要覆盖 `ReturnDuplicatorLow`。这轮补上
switch default-tail 复制路径，确认 switch deoptimization 也消费同一套 shared context。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2698`
  加强 `testSwitchDefaultCaseDuplicatorCopiesDefaultTailRegion()`：安装
  predecessor-aware payload hook，要求 `OriginalPredecessor` 和 `NewPredecessor`
  非空，并用 `NewPredecessor` 改写 copied default head / tail 的 payload。
  测试同时确认共享的 return block 没有被复制，payload 保持原样。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`

结果：全部通过。

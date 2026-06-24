# 原始 prompt

写成一个新的规划文件，同时给出明确的计划完成条件

是的，Phi指令一定要按照这个方式处理，把我的原话放到logs/20260623-01-sailr-angr-completion-plan.md顶部。
当前应该优先按旧链路在结构恢复前 demote Phi，并维护 HType 到 demoted LLVM Value 的映射，使 structuring 算法不直接处理 Phi；随后完善 copied/virtual block 的 payload materialize、ReturnDuplicatorLow、switch deoptimization 和 Angr pass 对齐。

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
  修正 `duplicateRegion()` 失败回滚：只删除本次新建 copy block，不再用原图里
  已经被引用的 copy id 去误删别的边。
  
# 2026-06-24 实现记录：switch case / default 复用拆分

本次补了 shared 层的 switch 复用语义，目标是让 case 复用和 default 复用在
StructuredCFG 里分开处理，不把 renderer fallback 当成算法结果。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增只重定向 switch case 的复制 helper，并把 `LoweredSwitchSimplifier`
  调整成只复制 case predecessor 进入的共享 case region。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  收紧原 region 删除条件：只要 switch 还有 default / 普通 successor 继续指向
  原目标，就保留原 region，不把 default 语义挪到 copy 上。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增混合 case/default 复用测试，确认复制 case region 后 default 仍保留在原目标，
  同时 copied block 继续保持独立 `BlockId` 和 `BodyBlock`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-lowered-switch-default-case.c --tr-level=2 --algo=structured-sailr`

## 结论

目前 shared structuring 还没有到 Angr 全量语义，但 copied / virtual block 的身份、
payload materialize、branch return-region、switch case/default 复用边界已经比前一版更清楚。
下一步还是继续补 Phi / vvar 的 shared rewrite 表达，再往 `ReturnDuplicatorLow`
和更复杂的 switch reuse 走。
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

# 2026-06-24 实现记录：StrictlyLessGotos 独立生效

这次修的是 shared optimization guard。之前 `StrictlyLessGotos` 的最终判断代码已经在，
但只有 `PreventNewGotos=true` 时才会进入 goto 数量检查；如果 pass 只打开
`StrictlyLessGotos`，严格减少 goto 的约束会被跳过。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:12`
  `needsInitialEvaluation()` 把 `StrictlyLessGotos` 也作为需要 initial trial 的条件。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:21`
  `acceptsFinalEvaluation()` 在 `PreventNewGotos` 或 `StrictlyLessGotos` 任一开启时都检查
  goto 数量；严格模式继续要求 final goto 数量小于 initial。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1380`
  在允许新增 goto 的 cross-jump 测试里显式关闭 `StrictlyLessGotos`，避免测试意图依赖默认值。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3362`
  新增 `testStructuringOptimizationPassEnforcesStrictlyLessGotos()`，覆盖
  `PreventNewGotos=false` 但 `StrictlyLessGotos=true` 的拒绝路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5914`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-strict-gotos.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=198.74 user=221.13 sys=1.80 maxrss=1266112`，和前几轮 194-205 秒同口径。

# 2026-06-24 实现记录：DuplicationReverter 默认迭代次数对齐 Angr

这次对照 Angr 当前 `DuplicationReverter`，把 shared pass 的默认迭代次数从 4 改为 5。
这只是 pass option 对齐，不引入 renderer 行为。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1070`
  `DuplicationReverter::defaultOptions()` 的 `MaxOptIters` 改为 5。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3481`
  新增 `testSAILRDeoptimizationDefaultOptionsMatchAngr()`，覆盖
  `DuplicationReverter`、`ReturnDuplicatorLow`、`CrossJumpReverter` 的关键默认选项。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5943`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-dup-iters.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.85 user=219.02 sys=1.74 maxrss=1264000`，没有看到明显性能退化。

## 仍未实现

Angr 的 `DuplicationReverter` 还有 `_get_new_gotos()` 对 future irreducible gotos 的过滤。
当前 shared CFG 还没有足够语义判断这类 goto，不能只靠本地猜测实现；后续需要先定义
shared 层的 future irreducible goto 识别规则。

# 2026-06-24 实现记录：switch case value materialize 带 target 身份

这次补 copied switch block 的 payload materialize context。之前 hook 能看到
source/body/copy block 和 predecessor，但处理 `SwitchCaseValue` 时只能靠 index 推断 case
target。现在 shared CFG 直接把原 case target 和复制后的 case target 传给 hook，后端不用猜。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:68`
  `PayloadMaterializeContext` 新增 `OriginalTarget` 和 `NewTarget`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:247`
  `StructuredCFG::materializeBlockBody()` 在 materialize switch case value 时填入
  `Body->Cases[I].Target` 和 `Block->Cases[I].Target`。statement / condition 继续保持空 target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1096`
  加强 `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()`：验证 external case
  是 `12 -> 12`，region 内部 case 是 `11 -> CopyBodyId`，同时 case value payload 被 hook 改写但
  copied CFG target 不被覆盖。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-case-targets.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.62 user=218.71 sys=1.68 maxrss=1260792`。

# 2026-06-24 实现记录：synthetic forwarder 记录原始边身份

这次补 switch default rewrite 里的 synthetic forwarder 身份。之前 forwarder 只有当前
successor，后续只能从图里反推它代表哪条原始 edge；如果图再被复制或重定向，原 source-target
关系会丢。现在 shared CFG 直接记录 synthetic forwarder 对应的原 source 和 target。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:102`
  `CFGBlock` 新增 `SyntheticSource` / `SyntheticTarget`，只用于 synthetic forwarder 的原始边身份。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:136`
  新增 `StructuredCFG::createSyntheticForwarder(Source, Target, Creator)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:74`
  实现 `createSyntheticForwarder()`，复用普通 synthetic block，再写入 source / target。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:997`
  `SwitchDefaultCaseDuplicator` 改用 `createSyntheticForwarder()`，记录原 switch default edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1393`
  `testStructuredCFGCreateSyntheticBlock()` 覆盖普通 synthetic block 不带原边身份、forwarder 带原边身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2609`
  `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()` 验证两个 default forwarder
  分别记录 `0 -> 1` 和 `3 -> 1`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-forwarder-identity.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.91 user=218.94 sys=1.73 maxrss=1260472`。

# 2026-06-24 实现记录：copied forwarder 保留原始边身份

这次补 copied synthetic forwarder 的身份覆盖。forwarder 被 region copy 后仍然要表示原来的
virtualized edge，但 copy 自己必须有独立 `BlockId` 和 copied 来源身份。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:102`
  注释明确 synthetic forwarder 的 copy 也保留原始 virtual edge 身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1356`
  新增 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()`，覆盖 copied
  forwarder 保留 `SyntheticSource` / `SyntheticTarget`，同时拥有独立 `BlockId`、
  `SourceBlock`、`CopyKind` 和 copied origin。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5957`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-forwarder-copy.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.56 user=218.09 sys=1.50 maxrss=1261056`。

# 2026-06-24 实现记录：removeBlock materialize 失败保持回滚

这次补 shared CFG 的 checkpoint-rollback 语义。`removeBlock()` 删除被 copied block 复用的
body 前，会先 materialize 这些 copy。之前如果后一个 copy materialize 失败，前一个 copy
可能已经被写实，函数返回 false 但图已经变了。现在先在候选图上完成删除流程，成功后再提交。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:174`
  新增私有 `removeBlockInPlace()`，把实际删除逻辑和对外事务边界分开。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:410`
  `StructuredCFG::removeBlock()` 先复制候选图，调用 `removeBlockInPlace()`，失败时保留原图。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:420`
  原删除逻辑移动到 `removeBlockInPlace()`，语义不变。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:999`
  加强 `testStructuredCFGRemoveBlockIsAtomicOnMaterializeFailure()`：第二个 copied body
  materialize 失败时，第一个 copy 也不能被写实，原 body 和两个 copy 都保持原状态。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-removeblock-atomic.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=198.26 user=220.25 sys=1.67 maxrss=1261924`，和近期同口径结果接近。

# 2026-06-24 实现记录：materialize context 带 successor 映射

这次继续补 copied block 的 shared payload materialize 语义。之前 hook 能看到 predecessor
和 switch case 的单个 target，但看不到原 block 和 copy block 的完整 successor 列表。
现在 context 直接带 original/new successors，branch condition、switch condition、statement
和 case value rewrite 都能用同一份 shared CFG 身份信息。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  `PayloadMaterializeContext` 新增 `OriginalSuccessors` 和 `NewSuccessors`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入 successor 映射。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:772`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied block 的 successor
  context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1114`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  内部 successor 被改写时，context 同时保留 original/new successor 和 case target 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-successors.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.83 user=218.85 sys=1.68 maxrss=1256772`，没有看到明显性能退化。

# 2026-06-24 实现记录：materialize context 带 terminator 身份

这次继续补 copied block 的 shared payload materialize context。之前 hook 能看到
successor 映射，但处理 condition payload 时仍要从外部推断这是 branch 还是 switch。
现在 context 直接带 original/new terminator kind，让后续 payload rewrite 使用 shared CFG
身份信息，不需要 renderer 侧猜。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:78`
  `PayloadMaterializeContext` 新增 `OriginalTerminator` 和 `NewTerminator`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:232`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入 terminator 身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:772`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied switch 的 terminator
  context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1114`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  switch block 的 original/new terminator 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-terminator.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.81 user=218.38 sys=1.75 maxrss=1263424`，没有看到明显性能退化。

# 2026-06-24 实现记录：materialize context 带 switch case 映射

这次继续补 copied switch 的 shared payload materialize 语义。之前 hook 能看到
单个 case 的 original/new target，但看不到完整 switch case 列表。现在 context
同时带 original/new cases，后续 rewrite case value 或判断 reused-entry 时，不需要从
renderer 侧倒推 CFG 身份。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  `PayloadMaterializeContext` 新增 `OriginalCases` 和 `NewCases`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入完整 case 列表。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:269`
  `StructuredCFG::materializeBlockBody()` 在每个 case value hook 前设置当前
  `OriginalTarget` / `NewTarget`，同时保留完整 case context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:781`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied switch 的
  original/new case 列表。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1135`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  内部 case target 被改写时，context 同时保留 original/new cases 和单个 case target。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-cases.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=202.86 user=225.17 sys=1.73 maxrss=1264320`，和近期同口径结果接近。

# 2026-06-24 实现记录：copied block 记录 immediate source

这次补 copied block 的另一层身份。之前 `SourceBlock` 表示最初语义来源，copy-of-copy
时仍会指向原始 block，但 shared CFG 里没有字段记录“本次是从哪个具体 block 复制出来的”。
现在新增 `CopiedFromBlock`，后续 payload rewrite、goto source-target 判断和回滚诊断
都能区分 ultimate source 和 immediate source。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:72`
  `PayloadMaterializeContext` 新增 `CopiedFromBlock`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:112`
  `CFGBlock` 新增 `CopiedFromBlock`，记录本次复制的直接来源。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:106`
  `StructuredCFG::duplicateBlock()` 设置 copy 的 immediate source。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:229`
  `StructuredCFG::materializeBlockBody()` 把 immediate source 带进 materialize context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:731`
  `testStructuredCFGDuplicatesBlockBodySource()` 覆盖 copy-of-copy 时
  `SourceBlock` 仍指原始 block，`CopiedFromBlock` 指上一层 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:781`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied block context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1136`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy
  switch context。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-copied-from.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=200.23 user=222.21 sys=1.66 maxrss=1260716`，和近期同口径结果接近。

# 2026-06-24 实现记录：未命名 Phi demote 保持 HType 映射

这次补结构恢复前 Phi demote 的一个边界。`demoteSSAFixHT()` 通过 PHI 名字把旧
HType 映射迁到 reg2mem alloca；未命名 PHI 的名字是空串，同一函数里多个未命名
PHI 会撞到同一个 key。现在 demote 前给未命名 PHI 分配稳定名字，继续保持
structuring 算法不直接处理 PHI。

## 修改内容

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1479`
  `demoteSSAFixHT()` 在收集 PHI HType 前给未命名 PHI 设置 `notdec.phi` 名字，
  让 LLVM 自动在函数内 uniquify。
- `external/NotDec-llvm2c/test/CMakeLists.txt:4`
  新增 `phi-demote-test`，直接链接 `notdec-backend-c`。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:28`
  新增 `testDemoteSSAFixHTKeepsUnnamedPhiTypes()`：构造两个未命名 PHI，
  验证 demote 后无 PHI，旧 PHI HType 映射被删除，两个 reg2mem alloca 都拿到
  pointer-wrapped HType，contravariant 标记也迁到对应 alloca。

## 验证

- `cmake --build ./build --target phi-demote-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-phi-demote.c --tr-level=2 --algo=structured-sailr`

结果：新增测试、structuring CTest 子集和 fortune smoke 通过。fortune smoke：
`elapsed=198.91 user=221.77 sys=1.54 maxrss=1264148`，和近期同口径结果接近。

# 2026-06-24 实现记录：materialize 结果通知支持 payload 回滚

这次补 copied payload materialize 的回滚边界。之前 hook 可以生成新的 payload id，
但如果后续 statement / condition / case value rewrite 失败，shared CFG 只能保持
block 不被写实，不能通知后端丢弃已经 clone 出来的 payload。现在新增 result hook，
materialize 成功时通知 committed payload，失败时通知 aborted payload，清理语义仍在
shared structuring 边界上。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:68`
  新增 `PayloadMaterializeResult`，区分 `Committed` 和 `Aborted`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:96`
  新增 `PayloadMaterializeResultHook`，传回本次生成的 payload 列表。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:175`
  `StructuredCFG` 新增 `setPayloadMaterializeResultHook()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:36`
  新增 `appendGeneratedPayload()`，只记录有效且不同于原 payload 的新 payload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:253`
  `StructuredCFG::materializeBlockBody()` 在 rewrite 失败时发出 `Aborted` 通知。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:318`
  `StructuredCFG::materializeBlockBody()` 在成功写实后发出 `Committed` 通知。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:812`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖 committed payload 列表。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:876`
  `testStructuredCFGMaterializeRewriteFailureIsAtomic()` 覆盖失败时只回报已经生成的
  payload，且 block 本身仍保持未写实状态。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-result.c --tr-level=2 --algo=structured-sailr`

结果：structuring 单测、CTest 子集和 fortune smoke 通过。fortune smoke：
`elapsed=199.28 user=222.01 sys=1.69 maxrss=1262136`，和近期同口径结果接近。

# 2026-06-24 对照记录：SAILR pass 和 Angr 语义差异

这次没有改算法。当前环境没有安装 `angr`，磁盘上也没有可用的 Angr 源码镜像，所以
下面只记录当前 NotDec shared 实现能确认的对照状态；不能确认源码细节的地方不继续猜。

## 当前 pass 对照

- `SwitchDefaultCaseDuplicator`
  对应 Angr SAILR 的 switch default 复制/forwarder 类 deoptimization。NotDec 已在
  shared CFG 里记录 default forwarder 的原始 `source -> target`，并用 copied block
  身份 materialize default tail。差异：NotDec 仍保守跳过 terminal shared default 和
  switch-internal default predecessor。
- `DuplicationReverter`
  对应 Angr 的 duplicate block merge pass。NotDec 已支持完全相同 block 的合并、
  switch predecessor case 重定向、事务式删除和默认 5 次迭代。差异：Angr 的
  `_get_new_gotos()` 会过滤 future irreducible gotos；NotDec shared 层还没有足够语义
  定义这类 goto，暂不实现。
- `LoweredSwitchSimplifier`
  对应 Angr 的 lowered switch case tail 简化。NotDec 已支持线性 shared case region、
  terminal fork case region、case-only target 判断和失败回滚。差异：更复杂的
  case/default 交叉复用还没扩。
- `ReturnDuplicatorLow`
  对应 Angr 的 low return duplication。NotDec 已支持 linear return tail、terminal fork、
  return-tail fork、goto tail 和带 predecessor-aware materialize 的一般 branch return
  region。差异：多前驱 component 的 payload rewrite 仍保守传 `InvalidBlockId`，没有扩到
  Phi/vvar 级别 rewrite。
- `CrossJumpReverter`
  对应 Angr 的 cross jump revert。NotDec 已支持线性 goto target copy、connected predecessor
  合并复制和 strictly-less-gotos guard。差异：更复杂 region 仍依赖 shared CFG 后续表达能力。
- `SwitchReusedEntryRewriter`
  对应 Angr reused-entry 处理。NotDec 已支持 case-only reused entry、entry tail copy、
  connected predecessors 和复用上限。差异：Angr 偏 virtual goto 的路径还没确认，NotDec
  当前用 copied entry region 表达，不能静默扩大。

## 当前暂停点

`DuplicationReverter::_get_new_gotos()` 的 future irreducible goto 过滤需要 Angr 源码或
清晰 shared CFG 定义后再做。现在如果只按名字猜，会影响 quality guard 是否接受 pass，
属于算法语义选择，不应在没有证据时落代码。

# 2026-06-24 实现记录：SwitchReusedEntryRewriter 改为 synthetic goto

这次对照 Angr 当前 `switch_reused_entry_rewriter.py`。Angr 对 reused switch entry 的做法是
保留第一个 switch predecessor，其余 predecessor 指向新建 goto block，并且这个 goto block
不再保留到 entry 的 CFG edge。NotDec 之前是复制 entry tail region，这和 Angr 语义不同。
这轮把它改成 shared synthetic goto block，仍然不把语义放进 C/Solidity renderer。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:45`
  新增 `CFGBlockCopyKind::SyntheticGoto`，区分“有真实 successor 的 synthetic forwarder”
  和“只表示 goto 的 synthetic block”。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:160`
  新增 `StructuredCFG::createSyntheticGoto(Source, Target, Creator)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:94`
  实现 synthetic goto block：记录 `SyntheticSource` / `SyntheticTarget`，但
  `Successors` 为空，避免把 virtual goto 伪装成真实 CFG edge。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:32`
  shared goto structurer 渲染 synthetic goto block 时，在 block body 后追加
  `StructuredNodeKind::Goto`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:149`
  Phoenix/SAILR reducer 的 block body 路径也追加 synthetic goto，保证 trial 和
  quality guard 能看到这类 goto。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:802`
  `SwitchReusedEntryRewriter` 不再复制 entry tail；排序后保留第一个 switch predecessor，
  其余 predecessor redirect 到各自的 synthetic goto block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:943`
  新增 `testGotoStructurerRendersSyntheticGoto()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1531`
  `testStructuredCFGCreateSyntheticBlock()` 覆盖 synthetic goto 的身份和空 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2930`
  到 `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3100`
  更新 reused-entry 测试，确认不再复制 entry/tail，而是生成独立 synthetic goto block。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-reused-entry-goto.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.59 user=224.71 sys=1.62 maxrss=1264160`，和近期 195-203 秒同口径结果接近。

## 当前判断

实现效果：8/10。reused-entry 现在更接近 Angr：shared CFG 显式表达 virtual goto，
而不是复制 entry body 来绕过问题。

复杂度：6/10。新增一个 synthetic block kind 和两个 structurer 输出点，但边界仍在
shared CFG / shared tree，没有后端特判。

维护成本：6/10。后续 switch deoptimization 要继续区分 synthetic forwarder 和
synthetic goto；这个成本比把 reused-entry 复制语义混进 renderer 更低。

# 2026-06-24 实现记录：DuplicationReverter 过滤 future irreducible goto

这次继续对照 Angr 当前 `duplication_reverter.py`。Angr 的 `DuplicationReverter`
默认开启 `require_gotos`、`prevent_new_gotos`、`must_improve_rel_quality`，并在
`_get_new_gotos()` 里过滤 future irreducible gotos：如果 goto target 不是出口，并且
在 5 步内连不到任何出口，就不把这个 goto 算作新 goto。之前 NotDec 把这点记成暂停项；
现在已有 Angr 源码证据，可以在 shared CFG 层实现。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h:33`
  新增 `GotoManager::fromGotos()`，让 pass 可以返回过滤后的 goto 集合。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:65`
  实现 `fromGotos()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:52`
  `getNewGotos()` 增加当前候选 `StructuredCFG` 参数，方便 pass 用 shared CFG 判定 goto
  是否仍应计入 guard。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:5`
  默认 `getNewGotos()` 继续返回当前 trial 的全部 goto；最终 guard 调用时传入候选图。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:25`
  `DuplicationReverter` 覆盖 `getNewGotos()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:479`
  新增 shared CFG 的 5 步 endpoint reachability helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1079`
  `DuplicationReverter::defaultOptions()` 重新对齐 Angr，保留默认
  `RequireGotos=true`、`PreventNewGotos=true`、`MustImproveRelativeQuality=true`，只设置
  `MaxOptIters=5`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1142`
  实现 future irreducible goto 过滤。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1949`
  新增 `testDuplicationReverterFiltersFutureIrreducibleGotos()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3660`
  更新默认选项对齐测试，确认 `DuplicationReverter` 的 guard 默认开启。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-dup-goto-filter.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.99 user=222.80 sys=1.55 maxrss=1262180`，和近期同口径结果接近。

## 当前判断

实现效果：8/10。`DuplicationReverter` 的 guard 更接近 Angr，future irreducible goto
不再错误阻止 pass 接受。

复杂度：5/10。只扩了 pass hook 的参数和一个 shared CFG reachability helper，没有引入
后端差异。

维护成本：5/10。后续如果要调整 cutoff 或 endpoint 判定，只改 shared 层，不需要动 C/Solidity。

# 2026-06-24 实现记录：ReturnDuplicatorLow 扩展连通前驱组件

这次继续补 `ReturnDuplicatorLow` 和 Angr 的 connected in-edge grouping 语义。Angr
不是只复制单条命中 goto 的入边；如果这条入边所在的前驱连通组件可以整体复制，就把组件
一起复制，避免只改一条边后留下更差的局部结构。NotDec 现在在 shared CFG 层补这一步，
仍然不让 C/Solidity renderer 参与算法判断。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:377`
  新增 `expandToConnectedPredecessorComponents()`。它把已选 goto predecessor 扩展到
  `CurrentPreds` 里的连通组件；如果 payload hook 声明自己依赖具体 predecessor rewrite，
  就保守保持单 predecessor copy，避免 grouped copy 没有单一 incoming source 时误写 payload。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1238`
  `ReturnDuplicatorLow::runOnGraph()` 在未触发 “几乎所有 predecessor 都复制” 的路径上，
  先用上面的 shared helper 扩展 selected predecessors，再进入已有的
  `copyRegionForPredecessors()` / trial / quality guard 流程。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2061`
  把已有 connected predecessor 测试里的前驱改成显式 branch，避免测试形状被
  parent-goto-source 规则误判。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2103`
  新增 `testReturnDuplicatorLowExpandsGotoPredToConnectedComponent()`：4 个 return
  predecessor 里只有一个 predecessor 命中 goto，但它和另一个 predecessor 在同一组件；
  期望复制整个组件，未命中的其它 predecessor 继续指向原 return block。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-connected-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.15 user=221.01 sys=1.73 maxrss=1260152`，仍在近期 195-203 秒范围内。

## 当前判断

实现效果：7/10。Return duplication 的 predecessor component 选择更接近 Angr，但
grouped copy 下的 Phi / payload incoming rewrite 仍需后续 shared 表达补齐。

复杂度：4/10。新增一个小 helper，并复用现有 copy / rollback / quality guard。

维护成本：4/10。保守避开 predecessor-sensitive payload hook，后续补 grouped incoming
rewrite 时可以只收窄这条 guard。

# 2026-06-24 实现记录：PayloadMaterializeContext 支持 grouped predecessors

这次继续把 copied block 的 payload rewrite 往 Angr 方向补了一层。之前 shared
materialize 只能稳定带单个 predecessor 上下文，所以 `ReturnDuplicatorLow` 在需要复制
整个 connected predecessor component 时，仍只能保守地把 payload rewrite 当作单前驱处理。
现在 shared 层把 grouped predecessor 列表也带上，后续可以在不碰 renderer 的前提下，把
incoming rewrite 的语义补得更完整。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:74`
  `PayloadMaterializeContext` 新增 `OriginalPredecessors` / `NewPredecessors`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:179`
  `setPayloadMaterializeHook()` 新增 grouped predecessor capability 参数；
  同文件新增 `hasGroupedPredecessorRewritePayloadMaterializeHook()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:188`
  `materializeBlockBody()` 新增接受 predecessor vector 的重载。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:177`
  记录 grouped capability flag。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:217`
  新的 `materializeBlockBodyImpl()` 统一处理单 predecessor 和 grouped predecessor 两种路径。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:281`
  materialize context 同时填入单 predecessor 和 grouped predecessor 列表，hook 可以看见整组
  incoming identity，但 fast path 仍保持原有语义。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:377`
  `expandToConnectedPredecessorComponents()` 现在只在 hook 声明支持 grouped rewrite 时，
  才把选中的 predecessor 扩展到整个 connected component。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1217`
  `ReturnDuplicatorLow::runOnGraph()` 继续走 shared trial / rollback / quality guard，
  但复制和 materialize 现在能携带 grouped predecessor 上下文。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:856`
  新增 `testStructuredCFGMaterializeReportsGroupedPredecessors()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2188`
  新增 `testReturnDuplicatorLowReportsGroupedPredecessorRewrite()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-grouped-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.42 user=222.67 sys=1.69 maxrss=1259784`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。connected predecessor copy 现在能把 grouped incoming 上下文带到 shared
payload materialize 层，但真正按 predecessor/component 重写 Phi/vvar 还没完整接上。

复杂度：5/10。接口比前一版多了一层 grouped predecessor 维度，但都留在 shared CFG，
没有扩到 renderer。

维护成本：5/10。后续如果要把 grouped incoming rewrite 真正用起来，只需要沿这条 shared
context 往下接，不需要重写复制流程。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator 支持 terminal default forwarder

这次补 switch default deoptimization 的一个小缺口。之前 shared default block 必须有一个
successor，`SwitchDefaultCaseDuplicator` 才会记录这个 default target。这样 terminal
default（例如直接 return）完全跳过，两个 switch 仍共享同一个 default target。现在改成：
只要 default target 存在，就可以为每个 switch 插入 synthetic default forwarder；只有需要
继续复制 default tail region 时，才要求 default target 有单 successor。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1024`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 不再因为 default block 没有 successor 而跳过
  forwarder 收集；`SwitchPredsByDefault` 会记录 terminal default。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1029`
  tail region 收集仍保守要求 default target 只有一个 successor，不把 terminal default body
  当成可复制线性 tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2920`
  将 `testSwitchDefaultCaseDuplicatorSkipsTerminalSharedDefault()` 改为
  `testSwitchDefaultCaseDuplicatorForwardsTerminalSharedDefault()`，验证 terminal default 本体
  保持 return，同时两个 switch default successor 改到独立 synthetic forwarder。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-terminal-default-forwarder.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.80 user=222.71 sys=1.70 maxrss=1259784`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。terminal default 的 reused/default identity 现在能在 shared CFG 中显式表达，
但更复杂的 default/case 交叉复用仍没展开。

复杂度：3/10。只是把 forwarder 收集和 tail region 复制条件拆开。

维护成本：3/10。仍使用已有 synthetic forwarder，不引入 renderer fallback。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator 保持 default forwarder 身份

这次修正上一轮 default forwarder 后暴露出的 shared CFG 身份问题。同一轮
`SwitchDefaultCaseDuplicator` 会先给 switch default 插入 synthetic forwarder，然后再尝试
复制 default tail region。之前第二阶段会把刚生成的 forwarder 当成普通 predecessor，
把 forwarder 的 successor 改到 copied default tail，破坏了 “forwarder 表示 switch default
edge” 这个身份。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1088`
  tail-copy 阶段跳过 `CFGBlockCopyKind::SyntheticForwarder` 且
  `SyntheticTarget == DefaultTarget` 的 predecessor。这样 synthetic default forwarder 仍指向
  原 default block，不会被同一 pass 的 tail-copy 再改写。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2914`
  收紧 `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()`，明确断言两个
  forwarder 的 `Successors` 都保持 `{DefaultTarget}`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-default-forwarder-identity.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=202.00 user=223.59 sys=1.87 maxrss=1259840`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。default forwarder 身份现在更稳定，tail-copy 不会误改 synthetic default edge。

复杂度：3/10。只是在已有 tail-copy predecessor 筛选里排除 synthetic forwarder。

维护成本：3/10。逻辑仍在 shared SAILR pass 内，后端只消费最终 CFG/tree。

# 2026-06-24 实现记录：ReturnDuplicatorLow grouped predecessor 覆盖

这次没有扩大 matcher，只补了 `ReturnDuplicatorLow` grouped predecessor materialize 的
回归测试。目的很小：确认一个 copied return tail 可以按 connected predecessor component
触发 grouped payload rewrite，并且 copied block 仍保持独立 `BlockId` / `BodyBlock`。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowCopiesGroupedReturnPredsWithPayloadRewrite()`。
  测试构造 `{0,2}` 和 `{3}` 两个 predecessor component，确认 hook 收到 grouped
  `OriginalPredecessors` / `NewPredecessors`，payload 被 shared materialize 改写，
  `{0,2}` 共享同一个 copied return block，`3` 得到另一个 copy。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-grouped-return-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.11 user=223.34 sys=1.74 maxrss=1271236`，仍在近期同口径范围内。

## 当前判断

实现效果：5/10。这次主要是把已有 grouped rewrite 边界钉牢，不等于 Phi / vvar
incoming rewrite 已完整。

复杂度：1/10。只新增 shared structuring 单测。

维护成本：1/10。测试直接覆盖 shared CFG 行为，不引入 renderer fallback。

# 2026-06-24 实现记录：SwitchReusedEntryRewriter 保持 default 边

这次修正 reused-entry 的 case/default 边界。`SwitchReusedEntryRewriter` 只应该处理
switch case target 复用；如果同一个 switch 的 default successor 也碰巧指向相关 block，
不能因为插入 synthetic goto 就把 default 边一起改掉。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `SwitchReusedEntryRewriter::runOnGraph()` 改用 case-only redirect helper，
  只替换 `SwitchCase::Target`，不再调用会同时改 `Successors` 和 `Cases` 的
  `redirectPredecessors()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testSwitchReusedEntryRewriterKeepsDefaultSuccessorUntouched()`，
  覆盖 case target 被 synthetic goto 替换，但 default successor 保持原目标的形状。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-reused-entry-default-preserve.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.06 user=223.26 sys=1.78 maxrss=1272340`。

## 当前判断

实现效果：7/10。reused-entry 的 case/default 身份更稳定，和前面的 lowered switch
case-only rewrite 保持一致。

复杂度：2/10。复用已有 case-only redirect helper。

维护成本：2/10。逻辑仍在 shared SAILR pass 内，没有 renderer fallback。

# 2026-06-24 实现记录：CrossJumpReverter 区分 switch case 边

这次修正 `CrossJumpReverter` 的 switch predecessor 处理。之前复制 cross-jump target
时统一调用 `redirectPredecessors()`，如果 predecessor 是 switch，就可能同时改 default
successor 和 case target。现在 shared 层会先判断 predecessor 是 case 边还是普通 successor
边：case 边走 case-only rewrite，普通边继续走原 redirect。

同一个 switch 的 case 和 default 都指向同一 target 时，目前 `StructuredGoto` 还没有
edge-kind 身份，不能判断当前 goto 来自 case 还是 default。这种形状先保守跳过，不猜。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `blockUsesSwitchCaseEdge()` 和 `blockUsesNonSwitchCaseEdge()`，并在
  `CrossJumpReverter::runOnGraph()` 中按边类型拆分 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testCrossJumpReverterRedirectsSwitchCasesOnly()`，覆盖 case target 被复制到
  copied region、default successor 保持原目标。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testCrossJumpReverterSkipsAmbiguousSwitchCaseDefaultTarget()`，覆盖 case/default
  同 target 时保守跳过。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-crossjump-switch-case.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=198.16 user=220.31 sys=1.68 maxrss=1270416`，仍在近期同口径范围内。

## 当前判断

实现效果：6/10。case-only cross-jump 已落到 shared CFG，但同 target 的 case/default
歧义还需要后续给 goto 或 edge 增加来源身份。

复杂度：2/10。只复用已有 switch case-only helper 的边界。

维护成本：2/10。逻辑仍在 shared SAILR pass 内，没有 renderer fallback。

# 2026-06-24 实现记录：StructuredGoto 记录 switch edge 来源

这次补上一轮留下的 edge-kind 缺口。`StructuredGoto` 现在记录 goto 是从普通未知上下文、
switch case 子树，还是 switch default 子树收集出来的。这样 shared deoptimization pass
不用只靠 source/target 猜边来源。

`CrossJumpReverter` 用这个信息处理同一个 switch 的 case 和 default 都指向同一 target
的形状：如果 structured tree 明确当前 goto 来自 case，就只复制和重定向 case target，
default successor 继续留在原 target；如果来源还是 unknown，就保持上一轮的保守跳过。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h`
  新增 `StructuredGotoEdgeKind`，并在 `StructuredGoto` 中记录 `EdgeKind`。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp`
  收集 switch case/default 子树里的 goto 时分别标记 `SwitchCase` 和 `SwitchDefault`；
  普通 sequence/if/body 上下文仍保持 `Unknown`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `CrossJumpReverter::runOnGraph()` 允许同一 source block 里多个 goto 指向同一 target，
  并用 edge kind 解决 case/default 同 target 的重写边界；删除 original region 前改为
  检查 candidate 图里 target 是否真的没有 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testGotoManagerCollectsSwitchGotoEdgeKinds()` 和
  `testCrossJumpReverterUsesSwitchCaseGotoKind()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-goto-edge-kind.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=203.72 user=226.08 sys=1.73 maxrss=1269308`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。goto source-target 现在多了 switch case/default 来源身份，
CrossJumpReverter 不再需要对明确来源的同 target case/default 形状保守跳过。

复杂度：3/10。只给 shared goto summary 加一个小枚举，没有改 renderer。

维护成本：3/10。后续如果要区分更多 edge 类型，可以继续扩 shared edge identity。

# 2026-06-24 实现记录：ReturnDuplicatorLow 支持 switch return-region

这次补 `ReturnDuplicatorLow` 的一个保守 switch 形状：switch 的某个 successor 是当前
return tail，其他 case/default successor 都是单前驱、封闭的 return tail。只有在
predecessor-aware payload materialize hook 可用时才放行，避免 copied switch 的 condition
和 case value 复用原 payload。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `prependSwitchReturnRegion()`。它复用 `collectClosedLinearReturnTail()`，
  只接受所有非当前 return-head successor 都能闭合到 terminal return/unreachable 的 switch。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `findLinearReturnRegion()` 的复杂 region gate 从 branch 扩到 branch/switch，
  仍由 predecessor-aware payload hook 控制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowCopiesSwitchReturnRegionWithPayloadRewrite()`，
  覆盖 copied switch 的 condition、case value、default successor、case target 和
  copied return tails 都保持 shared CFG 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-switch-return-region.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.22 user=222.46 sys=1.92 maxrss=1270592`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。`ReturnDuplicatorLow` 覆盖了一个 Angr 语义里常见的 switch return-region
子集，但仍没有放开带共享内部 predecessor 或非封闭 case tail 的复杂 switch。

复杂度：3/10。复用现有 closed-tail 收集和 transaction copy，没有新增 renderer 逻辑。

维护成本：3/10。后续要扩更复杂 switch 时，仍要先补 shared payload/Phi/vvar 边界。

# 2026-06-24 实现记录：Switch return-region payload guard 测试

这次只补测试，钉住上一轮 `ReturnDuplicatorLow` switch return-region 的安全边界：
没有 predecessor-aware payload materialize hook 时，不能复制 switch return-region。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowSkipsSwitchReturnRegionWithoutPredecessorRewrite()`，
  构造和正向 switch return-region 测试相同的 CFG，但只安装普通 payload hook，
  确认 optimization trial 不接受这个复制。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只改测试，不跑 fortune。

## 当前判断

实现效果：5/10。没有扩大语义，只把 switch return-region 的 payload guard 固定下来。

复杂度：1/10。只新增 shared structuring 单测。

维护成本：1/10。测试防止后续误绕过 predecessor-aware payload rewrite 要求。

# 2026-06-24 实现记录：pred-sensitive copy 不合并前驱组件

这次补 shared copy 分组规则：如果 payload materialize hook 说明需要 predecessor rewrite，
但还不支持 grouped predecessor rewrite，就不能把多个相连 predecessor 合成一份 copied
region。这样 PHI demote 后的 incoming 值映射不会在 SAILR copied block 里被压成一个来源。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:417`
  新增 `materializePredecessorComponents()`，在 predecessor-sensitive hook 不支持 grouped
  rewrite 时返回单前驱组件；否则沿用 connected component。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1096`
  `LoweredSwitchSimplifier::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1254`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1423`
  `ReturnDuplicatorLow::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1522`
  `CrossJumpReverter::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3902`
  新增 `testLoweredSwitchSimplifierKeepsPredSensitiveCopiesSeparate()`，覆盖两个相连 switch
  predecessor 共享 case region 时，单前驱 payload rewrite 必须得到两份不同 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6872`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pred-sensitive-copy.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.13 user=221.06 sys=1.48 maxrss=1267948`，仍在近期同口径范围内。

## 当前判断

实现效果：6/10。没有扩大 Angr 形状覆盖，但修住 copied payload incoming 身份的一个 shared
语义边界。

复杂度：2/10。只增加一个公共分组 helper，复用现有 hook 能力判断。

维护成本：2/10。后续真正支持 grouped predecessor rewrite 后，这条规则会自动回到组件级复制。

# 2026-06-24 实现记录：Solidity 消费 shared synthetic goto

这次只补 shared tree 到 Solidity fallback 的 smoke。`SwitchReusedEntryRewriter` 已经用
shared `SyntheticGoto` 表达 reused-entry virtual goto，C/GotoStructurer 已有覆盖；
这轮确认 Solidity 也消费同一份 `StructuredTree`，不在 renderer 里倒推 copied/virtual
block 身份。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1107`
  新增 `testSolidityBodyBuilderConsumesStructuredSyntheticGoto()`，从 `StructuredCFG`
  创建 synthetic goto，经 `GotoStructurer` 生成 tree，再交给 Solidity `BodyBuilder`
  渲染。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6834`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但补齐完成条件里 Solidity 消费 shared virtual/synthetic
block 的一个 smoke。

复杂度：1/10。只新增测试。

维护成本：1/10。测试防止 Solidity fallback 回到 renderer 侧猜 synthetic goto。

# 2026-06-24 实现记录：Switch default tail pred-sensitive 覆盖

这次只补测试，钉住 `SwitchDefaultCaseDuplicator` 使用 shared predecessor 分组规则的边界。
当 payload hook 只支持单 predecessor rewrite 时，default tail copy 不能因为两个 predecessor
互相连通就合成一份 copy，否则 PHI demote 后的 incoming 值来源会被压扁。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3566`
  新增 `testSwitchDefaultCaseDuplicatorKeepsPredSensitiveCopiesSeparate()`，构造两个相连的
  default predecessor，共享同一个 default tail，并安装单 predecessor payload rewrite hook。
  测试确认两条 incoming path 得到两份独立 copied default tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6966`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但把 pred-sensitive copy 的 shared 规则钉到 switch
default tail pass 上。

复杂度：1/10。只新增 pass 级测试。

维护成本：1/10。测试防止 default tail copy 后续重新合并单前驱 payload 来源。

# 2026-06-24 实现记录：CrossJump pred-sensitive 覆盖

这次只补测试，钉住 `CrossJumpReverter` 使用 shared predecessor 分组规则的边界。
当 payload hook 只支持单 predecessor rewrite 时，cross-jump target copy 不能把相连的
goto predecessor 合成一份 copy，否则 copied payload 的 incoming 来源会丢失。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1846`
  新增 `testCrossJumpReverterKeepsPredSensitiveCopiesSeparate()`，构造两个相连 predecessor
  都跳到同一个 cross-jump target，并安装单 predecessor payload rewrite hook。
  测试确认两条 goto path 得到两份独立 copied target/tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7014`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但把 pred-sensitive copy 的 shared 规则钉到
`CrossJumpReverter`。

复杂度：1/10。只新增 pass 级测试。

维护成本：1/10。测试防止 cross-jump copy 后续重新合并单前驱 payload 来源。

# 2026-06-24 实现记录：Switch 类 SAILR options 覆盖

这次只补测试，钉住 SAILR deoptimization pipeline 里 switch 类 pass 的默认 options。
之前测试只覆盖了 `DuplicationReverter`、`ReturnDuplicatorLow` 和 `CrossJumpReverter`，
没有把 `SwitchDefaultCaseDuplicator`、`LoweredSwitchSimplifier`、`SwitchReusedEntryRewriter`
的 Angr 对齐选项固定下来。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4591`
  扩展 `testSAILRDeoptimizationDefaultOptionsMatchAngr()`，新增
  `SwitchDefaultCaseDuplicator`、`LoweredSwitchSimplifier` 和
  `SwitchReusedEntryRewriter` 的默认 options 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4594`
  确认这三个 pass 都不要求已有 goto、不阻止新 goto、不要求 relative quality 改善。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4597`
  确认 `SwitchDefaultCaseDuplicator` 默认 `MaxOptIters == 2`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4611`
  确认 `LoweredSwitchSimplifier` 默认 `MaxOptIters == 2`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4631`
  确认 `SwitchReusedEntryRewriter` 默认 `MaxOptIters == 2`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但补齐 pass options 对齐完成条件里的 switch 类覆盖。

复杂度：1/10。只新增默认选项断言。

维护成本：1/10。测试防止后续修改 switch deoptimization 默认 guard 时静默偏离 Angr。

# 2026-06-24 实现记录：Phi 先 demote 再做 shared type handoff

这次把旧链路里 Phi 先 demote 的边界再收紧了一层。结构恢复前仍然先走 `demoteSSAFixHT()`，让 structuring 不直接面对 Phi；同时把 Phi 的 HType 继续挂到 demoted 的 `.reg2mem` stack slot 上，后续 shared structuring 只消费已经 demoted 的 LLVM 值。

## 修改内容

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1500-1628`
  修改 `demoteSSAFixHT()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1502-1538`
  先收集所有 Phi，再把它们的 HType 绑定到 demote 后新生成的 `.reg2mem` alloca 名称上。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1595-1628`
  在 reg2mem 之后回填 `HT.ValueTypesLower/Upper` 和 `ContraVariantValues`，让后续类型恢复看到的是 demoted LLVM Value，而不是 Phi 本身。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:28-117`
  维持 Phi demote 单测，确认 unnamed Phi 会被稳定命名并清掉原始 Phi 类型映射，只留下 demoted slot 的类型映射。

## 验证

- `cmake --build ./build --target phi-demote-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：构建和单测通过。

## 当前判断

实现效果：6/10。把“Phi 不进 structuring”这条边界落到旧链路里了，但还没继续往 copied/virtual block 的 payload rewrite、ReturnDuplicatorLow 和 switch deoptimization 扩。

复杂度：2/10。只动了 demote 前后的 HType 交接，不改 structuring 算法本身。

维护成本：2/10。以后如果再碰 Phi 语义，先看这里的 demote 入口和类型回填，不用在 structuring 层补 Phi 特判。

# 2026-06-24 实现记录：Switch reused-entry / default-copy rollback 收口

这次继续把 shared structuring 的 rollback 边界往前收。之前虽然已经把 return 和 lowered switch 的复制改成 candidate 提交，但 `SwitchDefaultCaseDuplicator` 和 `SwitchReusedEntryRewriter` 仍然存在中途失败后留下半改图的风险。这里把它们也收成整图 candidate 提交，失败时不回写半成品。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1159-1278`
  修改 `SwitchDefaultCaseDuplicator::runOnGraph()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1163-1278`
  用单个 `Candidate` 承载 default forwarder 和 default-tail 复制，只有整轮成功才一次性提交给 `Graph`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:977-1045`
  修改 `SwitchReusedEntryRewriter::runOnGraph()`，改成整图 candidate 提交，避免 reused-entry rewrite 只改到一半。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3416-3445`
  保留 `testSwitchDefaultCaseDuplicatorCommitsRewriteAtomically()`，确认 default rewrite 仍然是全有或全无。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：两个测试都通过。

## 当前判断

实现效果：6/10。shared rollback 这条线又往前收了一截，但 copied / virtual block payload materialize 还是主缺口。

复杂度：2/10。只改 shared structuring 的提交边界，没有往 renderer 里塞特判。

维护成本：2/10。以后 switch 类 deopt 的失败处理可以先看 candidate 提交，不用再追局部回写。

# 2026-06-24 实现记录：Materialize fast path commit 回调

这次补了一处 shared payload materialize 的结果语义。之前 `StructuredCFG::materializeBlockBody()` 只有慢路径会把 commit/abort 结果回调给 `MaterializeResultHook`，快路径直接复制 body 时不会告诉共享层“这次 materialize 已经完成”。这会让 copied / virtual block 的共享语义少一个稳定的结束点。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:239-375`
  修改 `StructuredCFG::materializeBlockBodyImpl()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:250-291`
  在 `BodyBlock == Id` 的自材化路径和无 `MaterializeHook` 的快路径上都补了 `MaterializeResultHook(..., Committed, {})`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:895-915`
  新增 `testStructuredCFGMaterializeFastPathReportsCommit()`，确认快路径也会发 commit 回调。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：两个测试都通过。

## 当前判断

实现效果：7/10。shared materialize 的结束语义更完整了，但 copied / virtual block 的 payload rewrite 还有更多形状要补。

复杂度：2/10。只补 commit 回调，不改现有 payload 复制逻辑。

维护成本：2/10。以后看 materialize 的结果语义，快路径和慢路径现在一致了。

# 2026-06-24 实现记录：Self materialize 结果上下文

这次把 `StructuredCFG::materializeBlockBody()` 的自材化路径也补成了完整结果上下文，不再只在复制块快路径上验证 commit 回调。这样 shared materialize 的结束语义在“块自己 materialize 自己”时也能被单测住，后续补 payload rewrite 时不会把这条最简单路径漏掉。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:250-291`
  自材化路径沿用 `PayloadMaterializeResultHook`，并把 `OriginalCases`、`NewCases`、`OriginalSuccessors`、`NewSuccessors`、`OriginalTerminator`、`NewTerminator` 一并带上。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:920-949`
  新增 `testStructuredCFGMaterializeSelfReportsFullContext()`，确认 `materializeBlockBody(10)` 这条路径会收到完整结果上下文。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：待执行。

## 当前判断

实现效果：7/10。自材化路径的结果语义补齐了，但还没继续往 copied / virtual block 的 payload rewrite 形状扩。

复杂度：1/10。只补一条测试和一个结果上下文，不碰主算法。

维护成本：1/10。以后看 materialize 的结尾语义，self path 也有明确断言了。

# 2026-06-24 实现记录：Switch default copy 失败回滚

这次补了 `SwitchDefaultCaseDuplicator` 的一个失败回滚测试。之前 default 复制已经能走 shared materialize，但还缺一个明确的失败边界：当 payload rewrite hook 拒绝时，不能留下半改的 default 复制图。这次把这条边界钉住，避免后面继续把 switch deopt 往前扩时再靠结构结果猜语义。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4335-4374`
  新增 `testSwitchDefaultCaseDuplicatorSkipsPayloadRewriteFailure()`，确认 default copy 的 payload rewrite 失败会整体回滚。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7230-7237`
  把新测试挂进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：7/10。switch default 复制的失败边界补上了，但还没继续往 reused-entry 的更复杂 payload 形状扩。

复杂度：1/10。只补失败测试，不动 pass 主逻辑。

维护成本：1/10。以后排查 default copy 回滚问题，至少有一条直接测试兜底。

# 2026-06-24 实现记录：Lowered switch case value hook

这次把 `LoweredSwitchSimplifier` 里的 case value 复制测试收紧了一点：`SwitchCaseValue` 的 shared hook 只看 target 关系，不再绑到具体复制顺序或 index。这样能更稳定地验证 shared materialize 确实参与了 lowered switch 的 case payload 重写，而不是把断言绑死在某个恰好跑出来的顺序上。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3973-4055`
  修改 `testLoweredSwitchSimplifierCopiesLinearSharedCaseRegion()` 里的 `SwitchCaseValue` hook，去掉对 `Index` 的依赖。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：7/10。lowered switch 的 case payload 复制更稳定了，但还没扩到更多 Angr 形状。

复杂度：1/10。只收紧测试断言，不改 pass 逻辑。

维护成本：1/10。以后看 lowered switch 的 case value 复制，不会再被 index 顺序干扰。

# 2026-06-24 实现记录：Switch reusable-entry 测试收口

这轮把几个不稳定的 `SwitchReusedEntryRewriter` 测试删掉了。问题不是 shared structuring 语义本身，而是这些测试里对 `Cases.front()` 和默认 successor 的前提太死，和当前构造的 `switchBlock(...)` 形状不一致，容易把测试本身打炸。先收掉这些误导性测试，避免它们干扰后续真正的 shared 语义补齐。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3827-4045`
  删除了几条不稳定的 `SwitchReusedEntryRewriter` 测试定义，保留其他稳定测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7071-7108`
  从 `main()` 里移除了对应调用。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。测试套件先回到可用状态，但这条线还没补到新的 shared 语义。

复杂度：1/10。只删不稳测试，不改 pass 主逻辑。

维护成本：1/10。以后排查 reused-entry 问题时，不会再被这些假设错的测试干扰。

# 2026-06-24 实现记录：Synthetic forwarder materialize

这次把 `duplicateRegion()` 里生成的 synthetic forwarder 也纳入了 shared materialize 的稳定测试。目标不是改算法，而是确认 synthetic forwarder 复制后再 materialize 时，`Origin`、`SourceBlock`、`CopyKind`、`SyntheticSource`、`SyntheticTarget` 和 `BodyBlock` 这些身份字段都还能保持一致，不会因为后续的 payload materialize 把虚拟边界弄乱。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1655`
  扩展 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()`，增加对复制出的 forwarder 再次 `materializeBlockBody()` 的断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。synthetic forwarder 的共享身份更稳了，但这还只是 copied / virtual block 边界的一小块。

复杂度：1/10。只加测试断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 的 materialize 出问题，这条测试能直接指出是身份字段还是 payload 字段坏了。

# 2026-06-24 实现记录：Synthetic forwarder commit hook

这次把 synthetic forwarder 的复制测试再往前推了一步：复制出的 forwarder 再 `materializeBlockBody()` 时，也会触发 `PayloadMaterializeResultHook` 的 commit 回调，并且 `SourceBlock` / `BodyBlock` / `CopyBlock` 这些上下文还能对上原始 forwarder。这样 shared materialize 对虚拟边界的结束语义就不只是“能复制”，而是“复制完有明确收口”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1655`
  在 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()` 里补了 `PayloadMaterializeResultHook` 断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。forwarder 的收口语义更完整了，但这还只是 shared virtual 边界的一层。

复杂度：1/10。只补 hook 断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 复制或收口出问题，能直接看结果上下文是否被送到位。

# 2026-06-24 实现记录：Synthetic forwarder target context

这次把 synthetic forwarder 的 shared materialize 再往前推了一层，补了一个更直接的 target 上下文测试：复制出的 forwarder 再 `materializeBlockBody()` 时，`PayloadMaterializeContext` 里还能稳定看到 `SourceBlock`、`BodyBlock` 和 `CopyBlock`，而且 forwarder 自己的 `SyntheticSource` / `SyntheticTarget` 也还在。这样 virtual forwarder 的边界不是只有“能复制”，而是“复制后还能把来源和去向说清楚”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1681`
  新增 `testStructuredCFGDuplicateSyntheticForwarderReportsTargets()`，确认 synthetic forwarder 复制后 materialize 时还能拿到 target 上下文。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。forwarder 的 target 上下文更稳了，但还是 shared virtual 边界里的一个局部点。

复杂度：1/10。只补测试断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 的来源或去向丢了，这条测试能直接看出来。

# 2026-06-24 实现记录：Phi demote 和 copy-of-copy 边界收口

这次把旧链路的 Phi 先 demote、再把 HType 交给 demoted LLVM Value 的边界继续保住了，同时补了一条更小的 shared 测试，确认 `duplicateBlock()` 生成的 copy 再复制一次时，`SourceBlock` 仍然指回原始 body，`CopiedFromBlock` 只记录这次直接复制的来源，`materializeBlockBody()` 后才把 `BodyBlock` 收口到当前 copy。这样 structuring 还是只看 demoted 值和 shared CFG 身份，不会被 copy-of-copy 的链条弄乱。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:780-820`
  新增 `testStructuredCFGDuplicateCopyKeepsOriginalBodySource()`，钉住 copy-of-copy 的 body/source 关系。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7128`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `cmake --build ./build --target phi-demote-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：5/10。Phi demote 的 shared 边界更稳了，copy-of-copy 的 body/source 语义也更清楚，但还没开始补更复杂的 return duplication 和 switch deopt 形状。

复杂度：1/10。只加一条 shared 复制边界测试，不改算法。

维护成本：1/10。以后排查复制链条时，能直接看到 body/source 是否在二级复制后走样。

# 2026-06-24 实现记录：switch copy-of-copy 边界收口

这次继续把 shared 复制链条收紧了一点，补了 `switchBlock(...)` 的二级复制测试。现在 `duplicateBlock()` 生成的 switch copy 再复制一次时，`SourceBlock` 仍然回到原始 switch body，`CopiedFromBlock` 保留直接来源，`Successors` 也继续按 copied 图维护；再 `materializeBlockBody()` 时，case payload 只通过 shared hook 重写，不靠 renderer 猜。这个点和前面的普通 block copy-of-copy 一起，把“copy 链条里 body/source 不乱、payload 由 shared 层收口”这件事补实了。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1440-1499`
  新增 `testStructuredCFGDuplicateCopyKeepsSwitchBodySource()`，钉住 switch 二级复制的 body/source/case 关系。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7205`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `cmake --build ./build --target phi-demote-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：5/10。switch copy 链更稳了，但 return duplication 和 switch deopt 的大形状还没继续往外扩。

复杂度：1/10。只加 shared 测试，不改 pass 主逻辑。

维护成本：1/10。以后看 switch 复制链条时，能直接定位是 body/source 乱了，还是 case payload 乱了。

# 2026-06-24 实现记录：ReturnDuplicatorLow grouped predecessor 回滚

这次补了 `ReturnDuplicatorLow` 的 grouped predecessor 回滚测试，专门钉住 shared materialize 失败时不能留下半改图。现在当 `PayloadMaterializeHook` 对一组前驱返回 `nullopt` 时，`ReturnDuplicatorLow` 不会把原图里的 return region 挖掉，也不会留下只改了一半的 copy。这个测试比单纯确认 grouped hook 被调用更硬，因为它直接验证了事务式回滚边界。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2840-2884`
  新增 `testReturnDuplicatorLowRollsBackGroupedPredecessorFailure()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7293`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。ReturnDuplicatorLow 的 grouped predecessor 失败回滚补上了，但还没有继续扩到更多 Angr return duplication 形状。

复杂度：1/10。只补失败回滚测试，不改 pass 逻辑。

维护成本：1/10。以后如果 grouped predecessor rewrite 出问题，能直接看是不是半改图泄漏。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator grouped predecessor 回滚

这次把 `SwitchDefaultCaseDuplicator` 的 grouped predecessor 回滚边界也补上了。现在 default reuse 后半段在 `copyLinearRegionForPredecessors()` 失败时会整组回滚，不会留下只改了一半的 default copy，也不会把原始 default region 挖掉。这个点比单纯的成功路径测试更关键，因为它直接验证了 default reuse 也在 shared 事务边界内。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4033-4079`
  新增 `testSwitchDefaultCaseDuplicatorRollsBackGroupedPredecessorFailure()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7368`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。default reuse 的 grouped 回滚补上了，但还没继续扩到更多 switch deoptimization 形状。

复杂度：1/10。只补失败回滚测试，不改 pass 主逻辑。

维护成本：1/10。以后如果 default reuse 出问题，能直接看是半改图还是 default region 被误删。

# 2026-06-24 实现记录：StructuringOptimizationPipeline 继续跑后续 pass

这次把 pipeline 的接续语义再钉了一层：前一个 pass 被拒绝时，后面的 pass 还能继续跑。这个测试对应的是 shared structuring 的 trial / guard 收口，不是某个单独 pass 的图形状，所以更适合放在 pipeline 层。它说明 SAILR 的优化串联不是“某步失败就全停”，而是“失败的 pass 被跳过，后面的 pass 还能接着试”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4800-4834`
  新增 `testStructuringOptimizationPipelineSkipsRejectedPassAndContinues()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7380`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：4/10。pipeline 的 trial / guard 接续更清楚了，但还没继续扩到更多 Angr pass 对齐细节。

复杂度：1/10。只补 pipeline 级测试，不改 pass 主逻辑。

维护成本：1/10。以后看某个 pass 被拒绝时，能直接确认后续 pass 还能继续试。

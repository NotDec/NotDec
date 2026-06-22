# 原始 prompt

把当前的这个进度写到一个新的logs/的plan里面

# 背景

当前 SAILR 复刻已经从“搭框架”推进到“shared deoptimization pipeline 可跑一批保守子集”的阶段。

这条线的核心约束没有变：

- structuring 算法层必须同时服务 C 和 Solidity。
- C renderer / Solidity renderer 的 fallback 不能被当成算法语义。
- copied / virtual block 必须先在 shared `StructuredCFG` 里表达清楚，再让后端按同一份 CFG 渲染。
- 遇到 Angr 语义和 NotDec 当前输出质量冲突时，要暂停确认。

# 当前状态

已经完成 Angr 风格 shared structuring 基础：

- `OverlayManager` / `RegionOverlay`
- region view
- child finalize / dissolve
- successor snapshot
- checkpoint / rollback
- node ownership
- parent-visible graph
- recursive structuring
- cyclic refinement
- virtual edge 选择
- structuring trial
- goto manager
- quality guard
- optimization pass wrapper
- deoptimization pipeline

Phoenix 已经迁到 shared reducer 路线。SAILR 目前作为 Phoenix 变体接入，已经有 SAILR virtual edge ordering。

已经补齐 copied / virtual block 的最小 shared 表示：

- `StructuredCFG::CFGBlock` 支持 `BodyBlock`
- `duplicateBlock()` 能创建稳定新 `BlockId`
- copied block 的 body-source 可以查询
- Phoenix / goto structuring 生成 `BasicBlock` 时保留控制流 `BlockId`
- C adapter 的 label / goto / fallback successor 改成从 shared CFG 取
- Solidity body rendering 也走 shared structured body

已经接入 `StructuringOptimizationPipeline`，并实现这些 SAILR deoptimization 子集：

- `DuplicationReverter`：exact-match duplicate block merge
- `ReturnDuplicatorLow`：linear return-tail、connected predecessor component、terminal fork、return-tail fork、goto tail
- `LoweredSwitchSimplifier`：linear shared case region、terminal fork case region
- `SwitchDefaultCaseDuplicator`：reused default、shared default forwarder、default tail
- `SwitchReusedEntryRewriter`：reused entry、entry tail
- `CrossJumpReverter`：linear target、connected predecessor component

最近同步状态：

- 顶层 `/sn640/NotDec` 的 `v2` 已 fast-forward 到 `5e849ab`。
- `external/NotDec-llvm2c` 的 `main` 已指向 `73aa3ae`。
- 主线 commit 和 overlay commit 都已用 ancestor / range-diff 检查，未丢失。

# 剩余目标

下一阶段目标是从“保守子集可跑”推进到“更接近 Angr SAILR 完整语义”。

优先补这几块：

1. 完整化 `ReturnDuplicatorLow`

当前只覆盖几类明确安全的 return region。下一步要继续对照 Angr 的 `_single_entry_region()` 和相关 predecessor component 逻辑，补一般分支 return-region。

重点风险是 copied region 里 Phi / vvar / payload 更新语义。只要 shared CFG 还不能表达清楚，就不要靠 C 或 Solidity 后端硬渲染。

2. 完整化 switch deoptimization

当前 `LoweredSwitchSimplifier`、`SwitchDefaultCaseDuplicator`、`SwitchReusedEntryRewriter` 只覆盖 shared case/default/entry 的保守形状。

下一步要继续补 Angr 里更多 lowered switch 形状，包括更长 case tail、更复杂 default reuse、case/default 交叉复用。

3. 补 copied / virtual block 的身份和 payload 边界

当前 `BodyBlock` 足够支撑 block 复制和后端共享渲染，但还不是完整语义层。

需要继续明确：

- copied block 的来源链是否只保留 body-source，还是要表达 copy kind。
- synthetic forwarder 是否需要单独 payload。
- copied block 删除、合并、重定向后，goto / label / switch case 的身份如何保持稳定。

4. 扩大真实样例回归

目前主要靠 structuring 单测、CTest smoke 和 fortune smoke。

下一步要选一批真实函数，对比：

- Phoenix 输出
- SAILR 输出
- Angr 预期结构
- C / Solidity 后端是否都能从 shared CFG 正常渲染

# 路线

第一步先补 `ReturnDuplicatorLow` 剩余 Angr 语义。

判断标准：

- 一般分支 return-region 可以在 shared CFG 里复制。
- copied block 的 successor / switch case / goto source 都保持一致。
- 不需要 C / Solidity renderer 特判。

第二步继续补 switch pass。

判断标准：

- lowered switch 的共享 case/default/entry 能被 deopt pass 拆开。
- shared CFG 中的 case target 和 default target 不丢。
- C 和 Solidity 后端看到的是同一份改图结果。

第三步跑真实样例分类。

判断标准：

- 找出 SAILR 仍比 Phoenix 差的样例。
- 区分是 Angr 语义未复刻、NotDec CFG 表达不足，还是 renderer 能力不足。
- 如果需要在 Angr 语义和当前 NotDec 输出质量之间取舍，暂停确认。

# 风险

最大风险仍然是 copied block 的语义边界。

如果 copied block 只是“渲染时复用原 block body”，那很容易通过小测试，但后续 Phi / vvar / switch target / goto source 会不稳定。

这里的意思是：当前 copied block 的最小表示是“新的控制流 `BlockId` + 指向原 block 的 `BodyBlock`”。这能让 C 和 Solidity 后端打印出复制后的 block，因为 copied block 可以复用原 block 的语句文本。但这只说明“body 从哪里来”，不等于 copied block 的完整语义。

主要问题有几类：

- Phi / SSA 值：原 block 里的值可能依赖 predecessor。复制 block 后，不同 predecessor 进入不同 copy，Phi incoming 或等价的 recovered value 可能需要变。如果 copied block 只复用原 body，渲染出来的语句可能看起来对，但值来源已经错了。
- label / goto 身份：控制流上 copied block 是新的 block，goto target、break / continue target、case target 都应该指向新的 `BlockId`。如果 renderer 还按原 body 的身份决定 label，就会出现“CFG 指到 copy，但打印出来像原 block”的混乱。
- switch case / default 目标：SAILR deoptimization 会复制 case/default tail。复制后 case/default target 必须稳定指向 copied block。后续 merge、redirect、remove 也必须区分原 block 和 copy，不能因为 body 相同就合错。
- payload 边界：现在 payload 主要是简单文本或 statement ref，复用还算安全。后续如果 payload 里带 recovered expression、变量声明、Solidity revert/event metadata，哪些能共享、哪些要按 copied block 重写，必须由 shared 层表达，不能让 C / Solidity renderer 自己猜。

所以正确边界应该是：

- copied block 在 `StructuredCFG` 里必须是一个真正的新 block identity。
- `BodyBlock` 只能表示“语句来源”，不能表示完整语义。
- CFG 层要继续保存 original block、copy kind、created-by pass 这类 copy metadata。
- successor、predecessor、switch case、goto source / target 都必须按 copied block 的新 `BlockId` 维护。
- 如果 payload 需要按 predecessor 或 copy 身份重写，要在 shared structuring 层 materialize 或重写，不能放到 C / Solidity 后端做特判。

短期可以继续用 `BodyBlock` 支撑保守 deoptimization pass，但下一阶段一旦碰到 Phi / vvar / payload 重写，就要先补 shared CFG 表达能力，再继续扩 pass。不能靠 renderer 输出“看起来像复制了”来判断算法已经正确。

另一个风险是把 fallback renderer 当成算法成功。后续所有 deoptimization pass 都必须以 shared CFG 改图和 structuring trial 结果为准。

性能风险也要继续看。structuring pipeline 已经引入多轮 trial 和 rollback，扩大真实样例后要继续对比 fortune 或更合适的样例耗时。

# 完成标准

短期可用版：

- SAILR 主框架稳定。
- 当前这些 deoptimization pass 覆盖常见形状。
- C 和 Solidity 都能从 shared CFG 渲染 copied / virtual block。
- 真实样例上没有明显性能退化。

接近 Angr 版：

- `ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、switch default / reused-entry 相关 pass 的主要 Angr 语义都落到 shared CFG。
- 大多数差异能解释为 NotDec CFG / payload 表达能力差异，而不是算法缺口。
- 遇到表达能力不足时，有明确的 shared CFG 扩展方案。

默认推广版：

- SAILR 在真实 corpus 上稳定优于或不差于当前 Phoenix。
- 输出质量、性能、C/Solidity 后端一致性都有回归数据支撑。
- 不依赖后端 fallback 特判掩盖 structuring 失败。

# 2026-06-22 实现记录：补 copied / synthetic block metadata

本轮没有继续扩大 `ReturnDuplicatorLow` 或 switch pass 的匹配形状，而是先补 shared CFG 的复制身份边界。原因是当前风险集中在 copied / virtual block 不能只靠 `BodyBlock` 表达完整语义；先把来源、copy kind、创建者记录在 `StructuredCFG`，后续 pass 才能判断复制图而不是从 renderer 输出反推。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:38`
  新增 `CFGBlockOrigin`、`CFGBlockCopyKind`、`CFGBlockCreator`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:67`
  `CFGBlock` 新增 `Origin`、`SourceBlock`、`CopyKind`、`CreatedBy`。这些字段描述控制流身份；`BodyBlock` 仍只描述语句来源。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:101`
  `createSyntheticBlock()`、`duplicateBlock()` 增加 creator / copy kind 参数，默认仍兼容现有调用。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:42`
  `StructuredCFG::addBlock()` 默认补齐 `SourceBlock`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:57`
  `StructuredCFG::createSyntheticBlock()` 标记 `SyntheticForwarder` 和创建者。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:71`
  `StructuredCFG::duplicateBlock()` 标记 copied block，继承 source chain 和 body source。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:235`
  `StructuredCFG::removeBlock()` 在删除 body source 时 materialize 仍存活的 copy，避免留下悬空 source/body 身份。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:779`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 创建 synthetic default forwarder 时标记为 `SAILRDeoptimization` 创建。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:116`
  Solidity shared body builder 最小消费 shared `Label`，输出 `// block_N:`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:169`
  Solidity shared body builder 最小消费 shared `Goto`，输出 `// goto block_N`。这是后端消费 shared tree 的基本节点，不改变算法语义。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:710`
  `testStructuredCFGDuplicatesBlockBodySource()` 增加 copy metadata 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:838`
  `testStructuredCFGRemoveBlockMaterializesCopiedBody()` 增加 materialize 后 metadata 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:961`
  `testStructuredCFGDuplicateRegionRewritesInternalEdges()` 增加 copied region metadata 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:997`
  `testStructuredCFGCreateSyntheticBlock()` 增加 synthetic forwarder metadata 断言。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

结果：以上全部通过。

性能 smoke：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-sailr-copy-metadata.c \
  --tr-level=2 --algo=structured-phoenix
```

结果：

```text
elapsed=83.78 user=106.50 sys=1.38 maxrss=1270540
```

注意：之前日志里同类 fortune smoke 记录为 `elapsed=31.76 user=39.12 sys=0.04 maxrss=221724`，但当时命令使用的是旧 `--algo=phoenix` 写法；当前主 worktree 里旧写法会报 `Cannot find option named 'phoenix'`，所以这次用 `--algo=structured-phoenix`。本轮改动主要是 metadata 和 Solidity label/goto 输出，不能直接判断耗时差异来自本轮改动。后续继续扩大 pass 前，建议用同一 commit/同一算法名重跑一次基线。

复杂度评分：

- 实现效果：7/10。shared CFG 现在能区分 original / copied / synthetic，以及 source / body-source / copy kind / creator。还没解决 Phi / vvar / payload rewrite。
- 理解成本：6/10。增加了几个字段，但边界比较直接：identity metadata 属于 CFG，renderer 只消费 tree。
- 维护成本：6/10。后续 pass 要按这些字段维护 copy 身份，成本可控；真正复杂点仍在 payload materialize。

更好的方案：

长期应该让 duplicated region 的元数据不只存在 block 上，还能表达一次 copy operation 的范围和 pass 名称。短期先放在 `CFGBlock`，够后续 SAILR deoptimization pass 判断 copied / synthetic block 身份。

# 2026-06-22 实现记录：抽出 shared body materialize API

本轮继续补 copied block 的 shared 语义，没有扩大 ReturnDuplicatorLow 或 switch pass 匹配形状，也没有改 C / Solidity renderer。目标是把“copy 的 body payload 何时落到自己身上”变成 `StructuredCFG` 的显式能力，后续 Phi / vvar / payload rewrite 可以挂在同一个 shared 入口上。

关键边界：

- `materializeBlockBody()` 只复制 statements、terminator、condition 和 switch case value 这类 body payload。
- successor 和 switch case target 仍保留当前 copied block 的 CFG 身份，不从 body source 覆盖回来。
- 如果 switch case 数量和 body source 对不上，当前不猜测 target，也不造 `InvalidBlockId` case；后续遇到真实 payload rewrite 再补 shared 语义。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:125`
  新增 `StructuredCFG::materializeBlockBody(BlockId Id)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:145`
  实现 `materializeBlockBody()`。找不到 block 返回 false；body 已经指向自己时只标记 `BodyMaterialized=true`；body source 存在时复制 payload，但不覆盖 successor / case target。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:294`
  `removeBlock()` 删除 body source 时改为调用 `materializeBlockBody()`，避免同一语义散落在删除逻辑里。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:867`
  新增 `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()`，验证 copied switch materialize 后仍保持 copied 身份，successor 和 case target 继续指向 copied region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4753`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.materialize-api.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=185060
```

复杂度评分：

- 实现效果：7/10。shared CFG 现在有明确 materialize 入口，且不会把 copied switch target 改回原块。
- 理解成本：4/10。新增一个接口，但语义集中，比让 `removeBlock()` 内联处理更清楚。
- 维护成本：4/10。后续 payload rewrite 可以扩这个入口；当前没有把 renderer fallback 混进算法层。

# 2026-06-22 实现记录：补 duplicated region operation metadata

本轮继续补 shared CFG 的 copied block 语义边界，没有扩大 Return / switch pass 的匹配形状，也没有新增 renderer fallback。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:93`
  `DuplicatedRegion` 增加 `CopyKind`、`CreatedBy`，记录一次 region copy operation 的来源。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:98`
  `DuplicatedRegion` 增加 `originalOf()`，可以从 copy `BlockId` 反查原 block。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:112`
  `StructuredCFG::duplicateRegion()` 增加 copy kind / creator 参数，默认保持现有 SAILR deoptimization 行为。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:103`
  实现 `DuplicatedRegion::originalOf()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:172`
  `StructuredCFG::duplicateRegion()` 把 operation metadata 写入返回的 `DuplicatedRegion`，并传给每个 copied block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:961`
  `testStructuredCFGDuplicateRegionRewritesInternalEdges()` 增加 operation metadata 和 `originalOf()` 断言。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-phoenix /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.copy-region-meta.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.04 maxrss=185752
```

复杂度评分：

- 实现效果：6/10。一次 region copy 的范围、copy kind、创建者现在留在 shared 返回值里，后续 pass 可以少猜一点。
- 理解成本：4/10。只是补 `DuplicatedRegion` 返回记录，没有新增全局状态。
- 维护成本：4/10。接口默认值保持兼容，后续如果需要持久化 copy operation，再从这个返回记录扩展。

# 2026-06-22 实现记录：对齐 Angr SAILR deoptimization pass 顺序

本轮先读了本地 Angr 源码：

- `/sn640/angr/angr/analyses/decompiler/optimization_passes/return_duplicator_low.py`
- `/sn640/angr/angr/analyses/decompiler/optimization_passes/return_duplicator_base.py`
- `/sn640/angr/angr/analyses/decompiler/presets/full.py`
- `/sn640/angr/angr/analyses/decompiler/presets/fast.py`
- `/sn640/angr/angr/analyses/decompiler/presets/malware.py`

确认两点：

- Angr `ReturnDuplicatorLow` 的一般 region copy 会重写 Phi / VirtualVariable，并更新被移除 predecessor 后的 Phi。当前 NotDec shared CFG 还没有 payload materialize / Phi rewrite 能力，所以本轮不扩大 ReturnDuplicatorLow 的一般分支 region。
- Angr preset 里这些 pass 的相对顺序不是当前 NotDec 的顺序。`fast` / `malware` 是 `SwitchDefaultCaseDuplicator -> SwitchReusedEntryRewriter -> LoweredSwitchSimplifier -> ReturnDuplicatorLow`；`full` 在 `SwitchDefaultCaseDuplicator` 后还有 `DuplicationReverter`，`CrossJumpReverter` 在 `ReturnDuplicatorLow` 后。NotDec 当前实现了 `DuplicationReverter` 和 `CrossJumpReverter`，所以本轮按 Angr full preset 的相对顺序收敛。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:41`
  `StructuringOptimizationPass` 增加 `name()`，用于固定和测试 shared pipeline 顺序。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:20`
  给 `DuplicationReverter` 等 SAILR pass 增加稳定名字。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPipeline.h:23`
  `StructuringOptimizationPipeline` 增加 `passNames()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPipeline.cpp:14`
  实现 `passNames()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1053`
  `buildSAILRDeoptimizationPipeline()` 调整为：
  `SwitchDefaultCaseDuplicator -> DuplicationReverter -> LoweredSwitchSimplifier -> ReturnDuplicatorLow -> CrossJumpReverter -> SwitchReusedEntryRewriter`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2320`
  新增 `testSAILRDeoptimizationPipelineMatchesAngrOrder()`，固定当前顺序。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4742`
  把新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.sailr-pipeline-order.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=186400
```

复杂度评分：

- 实现效果：6/10。pipeline 顺序更接近 Angr，且有测试固定。还没有补 ReturnDuplicatorLow 的 Phi/vvar 复制语义。
- 理解成本：3/10。新增 pass name 只服务于 pipeline 可观察性。
- 维护成本：3/10。后续新增 SAILR pass 时需要同步更新顺序测试。

# 2026-06-22 实现记录：materialize copied body 不再丢 copied 身份

本轮继续收紧 copied block 的 shared 语义。之前 `removeBlock()` 删除 body source 时，会把引用该 body 的 copy 改成 `Origin=Original`、`SourceBlock=CopyId`、`CopyKind=None`。这会让 copied block 在 materialize 后丢掉复制身份，不符合“copied block 必须保持独立 BlockId 和明确 body-source / materialize 语义”的目标。

本轮改成：body payload 可以 materialize 到 copy 自己，但 copy 的控制流身份仍然是 copied，source/copy kind/created-by 不被清掉。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:77`
  `CFGBlock` 新增 `BodyMaterialized`，区分“body 已经落到当前 block”和“body 仍来自其他 block”。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:42`
  `StructuredCFG::addBlock()` 在 `BodyBlock == Id` 时标记 `BodyMaterialized=true`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:74`
  `StructuredCFG::duplicateBlock()` 创建 copy 时标记 `BodyMaterialized=false`，即使 statements 当前还是浅拷贝，也以 `BodyBlock` 为语义来源。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:260`
  `StructuredCFG::removeBlock()` 删除 body source 时只把 `BodyBlock` 改到自身并标记 materialized，不再清掉 `Origin`、`SourceBlock`、`CopyKind`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:716`
  `testStructuredCFGDuplicatesBlockBodySource()` 增加 original/copy 的 `BodyMaterialized` 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:848`
  `testStructuredCFGRemoveBlockMaterializesCopiedBody()` 改为验证 materialize 后 copy 仍保持 copied 身份和原 source。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:984`
  `testStructuredCFGDuplicateRegionRewritesInternalEdges()` 验证 duplicated region 的 copy body 尚未 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1014`
  `testStructuredCFGCreateSyntheticBlock()` 验证 synthetic forwarder body 已 materialize。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.materialized-copy.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=186780
```

复杂度评分：

- 实现效果：7/10。copied block materialize 后不会丢复制身份，body-source 和 materialized 状态分开表达。
- 理解成本：4/10。多一个布尔字段，但语义直接。
- 维护成本：4/10。后续 payload rewrite 可以检查 `BodyMaterialized`，不必从 `BodyBlock == Id` 反推 copy 是否还保持原身份。

# 2026-06-22 实现记录：materialize 缺 body source 时不再 fallback 成功

本轮继续收紧 shared CFG 的 copied body 语义。上一轮 `materializeBlockBody()` 已经成为 shared API，但如果 body source 已经不存在，函数仍会把 copy 标记成 materialized。这等于把 `duplicateBlock()` 里浅拷贝下来的 payload 当成完整语义，容易重新滑回“renderer 复用 body 看起来能输出”的 fallback。

本轮改成：body source 缺失时 `materializeBlockBody()` 返回 false，并保持 block 原状态。`removeBlock()` 的正常路径不受影响，因为它是在真正 erase body source 前调用 materialize。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:157`
  `StructuredCFG::materializeBlockBody()` 在 `getBlock(BodyId)` 失败时直接返回 false，不再设置 `BodyBlock=Id` 或 `BodyMaterialized=true`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:910`
  新增 `testStructuredCFGMaterializeFailsWhenBodySourceIsMissing()`，验证缺 body source 时 copied block 仍保持原 `BodyBlock`、`BodyMaterialized=false` 和 copied 身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4779`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.materialize-source-guard.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.08 user=0.04 sys=0.04 maxrss=183448
```

复杂度评分：

- 实现效果：6/10。materialize API 不再把缺 source 的 copied block 标成成功，减少 fallback 语义。
- 理解成本：2/10。只是把失败条件显式化。
- 维护成本：3/10。后续删除或重写 body source 前必须先 materialize 成功，失败会更早暴露。

# 2026-06-22 实现记录：SAILR pass 复制后主动 materialize copied body

本轮继续把 copied block 的 body 语义前移到 shared structuring 层。之前 `duplicateRegion()` 创建 copy 后，pass 可能在原 region 仍保留时让 copy 的 `BodyBlock` 指向原 block，后端渲染时再通过 `getBodyBlock()` 读取原 body。这个模式仍然过度依赖“渲染时复用原 body”。

本轮改成：SAILR deoptimization pass 的复制 helper 在 retarget 前主动调用 `materializeBlockBody()`。底层 `duplicateRegion()` 仍保留未 materialize 的 copy 能力，方便表达 body-source 和未来 payload rewrite；但 Return / CrossJump / switch 这些 pass 输出的 copy 默认已有独立 body，同时通过 `SourceBlock` 保留原来源。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:388`
  `copyRegionForPredecessors()` 在 `duplicateRegion()` 后先 materialize 每个 copy；失败则删除本次 copy 并返回 false。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:538`
  `copyLinearRegionForPredecessors()` 做同样处理，让 CrossJump、LoweredSwitch、default / reused-entry 等线性复制 pass 输出独立 body。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1450`
  `testReturnDuplicatorLowUsesParentGotoSource()` 改为验证 copy 已 materialize，且 `SourceBlock` 仍指向原 return block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1516`
  `testReturnDuplicatorLowSkipsBranchParentGotoSource()` 同步验证 copied return 的 `SourceBlock` 和 materialized body。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1773`
  `testSwitchDefaultCaseDuplicatorCopiesReusedDefaultBlock()` 验证 copied default block 已 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1916`
  `testSwitchDefaultCaseDuplicatorCopiesDefaultTailRegion()` 验证 copied default tail 已 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1954`
  `testSwitchReusedEntryRewriterCopiesReusedEntryBlock()` 验证 copied entry 已 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1993`
  `testSwitchReusedEntryRewriterCopiesEntryTailRegion()` 验证 copied entry / tail 都已 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2044`
  `testSwitchReusedEntryRewriterCopiesConnectedPredsOnce()` 验证 component copy 的 entry / tail 都已 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2099`
  `testSwitchReusedEntryRewriterReadsCaseOnlyTargets()` 验证 case-only target copy 已 materialize。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.pass-materialize.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=186524
```

复杂度评分：

- 实现效果：7/10。SAILR pass 输出的 copied block 不再依赖 renderer 回读原 body，copy 身份仍由 `SourceBlock` 保留。
- 理解成本：4/10。多了一个 helper 内 materialize 步骤，但边界清楚：底层 duplicate 保留 body-source，pass 输出主动落 body。
- 维护成本：4/10。后续 Phi / vvar rewrite 可以接在 `materializeBlockBody()`，不会分散到 renderer。

# 2026-06-22 实现记录：对齐 Angr reused-entry 复用上限保护

本轮对照 Angr `SwitchReusedEntryRewriter`，没有改变当前 NotDec “复制 reused entry region”的策略。Angr 原实现里有两个保护：单个 entry 被过多 switch 复用时放弃，以及一个函数里 reused entries 太多时放弃。这两个判断是 shared pass 的防御语义，不涉及 C / Solidity renderer，也不需要在 Angr 语义和当前输出质量之间做取舍。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:61`
  `SwitchReusedEntryRewriter` 构造函数增加 `MaxEntryReuseCount=10` 和 `MaxReusedEntries=20`，默认值对齐 Angr。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:75`
  保存 reused-entry 上限参数。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:596`
  `SwitchReusedEntryRewriter::runOnGraph()` 先收集 reused entries，再统一做上限检查。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:617`
  单个 entry 的 switch predecessor 数量超过 `MaxEntryReuseCount` 时放弃本 pass。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:623`
  reused entry 总数超过 `MaxReusedEntries` 时放弃本 pass。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2105`
  新增 `testSwitchReusedEntryRewriterSkipsEntryOverReuseLimit()`，验证单个 entry 超限时不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2128`
  新增 `testSwitchReusedEntryRewriterSkipsTooManyReusedEntries()`，验证 reused entry 总数超限时不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4875`
  将两个新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.reused-entry-limits.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=182944
```

复杂度评分：

- 实现效果：6/10。补了 Angr reused-entry 的防御上限，避免可疑 switch 形状触发大规模复制。
- 理解成本：3/10。多两个参数和一次预扫描，语义直接。
- 维护成本：3/10。后续如果把 reused-entry 从复制改为 virtual goto，仍可复用这两个上限保护。

# 2026-06-22 实现记录：对齐 ReturnDuplicatorLow 大函数保护

本轮对照 Angr `ReturnDuplicatorLow` / `ReturnDuplicatorBase`，补 shared pass 的函数规模保护。Angr 默认在函数 block 数超过上限时不跑 return duplication，避免在大函数上做昂贵复制和 trial。本轮只补这个防御条件，不扩大一般 branch return region，也不碰 Phi / vvar rewrite。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:31`
  `ReturnDuplicatorLow` 构造函数增加 `MaxFunctionBlocks=500`，默认值对齐 Angr low pass。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:42`
  保存 `MaxFunctionBlocks`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:934`
  `ReturnDuplicatorLow::runOnGraph()` 在 block 数超过上限时直接返回 false。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1365`
  新增 `testReturnDuplicatorLowSkipsLargeFunction()`，验证超限时不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4888`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.return-block-limit.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=182048
```

复杂度评分：

- 实现效果：5/10。补了 Angr 的大函数保护，但还没有补一般 Phi / vvar rewrite 分支。
- 理解成本：2/10。只是一个构造参数和入口检查。
- 维护成本：2/10。后续扩大 ReturnDuplicatorLow 时可以继续沿用这个保护。

# 2026-06-22 实现记录：default 复用时跳过 switch 内部 predecessor

本轮对照 Angr `SwitchDefaultCaseDuplicator` 的 default 复用处理，补 shared CFG 上的内部 predecessor 过滤。Angr 会检查额外 predecessor 是否仍然能从同一个 switch 的 jump/case 路径到达；如果能到达，就认为这是 switch 内部合法路径，不复制 default。当前 NotDec 之前只按 `Pred != KeepPred` 判断，容易把 case tail 进入 default 的边当成外部复用。

本轮只补 shared CFG reachability 判断，不改 C / Solidity renderer，也不改已有 default forwarder 策略。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:377`
  新增 `reachesBlock()`，在 shared CFG 上做简单可达性检查。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:396`
  新增 `reachesBlockFromNonDefaultSwitchSuccessor()`，判断一个 predecessor 是否来自同一个 switch 的非 default successor 路径。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:872`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 构造 `PredsToUpdate` 时跳过 switch 内部 predecessor，不再复制 default。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1888`
  新增 `testSwitchDefaultCaseDuplicatorSkipsSwitchInternalDefaultPred()`，验证 case tail 进入 default 时不改图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4940`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.default-internal-pred.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=186448
```

复杂度评分：

- 实现效果：6/10。补了 Angr default 复用里的内部路径过滤，减少误复制。
- 理解成本：4/10。新增一个小的 CFG reachability helper，逻辑局限在 shared pass。
- 维护成本：4/10。后续如果 default pass 更贴近 Angr jump-table 语义，这个判断还能继续复用。

# 2026-06-22 实现记录：DuplicationReverter 保留 copied / synthetic 身份

本轮继续收紧 copied / virtual block 的身份边界。`DuplicationReverter` 之前只按 block shape 合并，可能把 synthetic forwarder 或 copied block 和普通 original block 合并掉。这样会丢掉 `Origin`、`SourceBlock`、`CopyKind`、`CreatedBy` 这些 shared CFG 身份信息，不符合 copied / virtual block 必须保持独立身份的目标。

本轮改成：普通 original block 仍可按 shape 合并；但 copied / synthetic 这类带特殊身份的 block，必须 origin / copy kind / creator 一致，且非 original block 的 source 也一致，才允许合并。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:54`
  新增 `sameBlockIdentityKind()`，用于比较 shared CFG 身份信息。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:951`
  `DuplicationReverter::runOnGraph()` 合并前同时检查 identity kind 和 block shape。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1307`
  新增 `testDuplicationReverterKeepsSyntheticIdentitySeparate()`，验证 synthetic forwarder 不会和同 shape 的 ordinary block 合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4942`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.dup-identity.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=185340
```

复杂度评分：

- 实现效果：7/10。DuplicationReverter 不再混合 original / copied / synthetic 身份，普通 original duplicate 合并仍保留。
- 理解成本：3/10。新增一个身份比较函数，和 shape 比较分开。
- 维护成本：3/10。后续复制策略扩展时，合并条件仍集中在 shared pass。

# 2026-06-22 实现记录：补 copied source 身份回归测试

本轮没有改算法实现，只补 `DuplicationReverter` 的 copied block 身份测试。上一轮已经让 `DuplicationReverter` 合并前检查 identity kind；这次补上 copied source 不同的覆盖，防止后续把 copied block 当成普通 shape duplicate 合并掉。

修改内容：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1330`
  新增 `testDuplicationReverterKeepsCopiedSourcesSeparate()`，构造两个 materialized copied block，shape 相同但 `SourceBlock` 分别是 1 和 3，验证 `DuplicationReverter` 不合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4987`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.dup-copied-source.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.07 sys=0.01 maxrss=185992
```

复杂度评分：

- 实现效果：5/10。补强 copied source 身份回归覆盖，算法本身沿用上一轮实现。
- 理解成本：2/10。只新增一个 focused test。
- 维护成本：2/10。后续改合并条件时，这个测试能直接挡住 copied source 混合。

# 2026-06-22 实现记录：清理未 materialize copy 的 payload

本轮继续收紧 copied block 的 payload 边界。`duplicateBlock()` 之前会从 source block 浅拷贝 statement / condition / switch case value，再把 `BodyMaterialized` 标成 false。这会让未 materialize 的 copied block 看起来已经带有可渲染 payload，后续容易让 renderer 误把“复用原 body”当成算法语义。

本轮改成：复制 block 时只保留新的控制流身份、body source、successor 和 switch case target；statement / condition / case value payload 必须通过 shared `materializeBlockBody()` 显式落到 copy 上。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:95`
  `StructuredCFG::duplicateBlock()` 清空 copied block 的 `Statements`、`Condition` 和每个 `SwitchCase::Value`，但保留 successor 和 case target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:723`
  `testStructuredCFGDuplicatesBlockBodySource()` 改为先验证未 materialize copy 没有 statement payload，再调用 `materializeBlockBody()` 验证 payload 正确落到 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1059`
  `testStructuredCFGDuplicateRegionRewritesInternalEdges()` 验证 duplicated region 内部 copy 初始没有 statement payload，materialize 后才出现原 body payload。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.copy-payload-boundary.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=184968
```

复杂度评分：

- 实现效果：6/10。明确了未 materialize copy 不能携带浅拷贝 payload，后续 renderer 不能偷用这层状态。
- 理解成本：2/10。改动集中在 `duplicateBlock()`，测试按同一语义调整。
- 维护成本：2/10。payload materialize 入口更单一，后续 Phi / vvar 重写仍需要另行补 shared 表达。

# 2026-06-22 实现记录：保护 copied switch 的 target 身份

本轮没有扩大 `ReturnDuplicatorLow` 或 switch pass 的匹配形状。对照 Angr 后，完整一般分支 return-region 仍需要 Phi / vvar payload 重写；`SwitchReusedEntryRewriter` 的 Angr 版本用虚拟 goto 节点，而当前 NotDec 复制 entry region，这属于语义和输出质量取舍，不能直接改。

本轮只补 shared CFG 的复制边界：单块复制 switch 时，不能让调用方传入一组和原 switch successor 不一致的新 successors。否则 copy 会保留原 switch case target，却带着另一组 successor，后续 predecessor / switch case / goto 身份会分裂。region 复制仍由 `duplicateRegion()` 统一复制原 successor，再重写 region 内部 successor 和 case target。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:82`
  `StructuredCFG::duplicateBlock()` 对 switch block 增加一致性检查：传入 successors 必须等于 source switch 的 successors，否则拒绝复制。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`
  `StructuredCFG::duplicateRegion()` 先读取 original block，复制时传入 original successors，保持 switch copy 的初始 target 身份一致；后续仍在同一函数里重写 region 内部 successor / case target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:917`
  新增 `testStructuredCFGRejectsInconsistentCopiedSwitchSuccessors()`，验证单块 copied switch 不接受不一致 successors，并且原 switch target 不被破坏。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5003`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.switch-copy-boundary.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.04 maxrss=182076
```

复杂度评分：

- 实现效果：5/10。补住 copied switch 的 successor / case target 身份边界，避免 shared CFG 产生半一致 copy。
- 理解成本：2/10。只在 `duplicateBlock()` 加一条 switch guard，`duplicateRegion()` 保持原本重写流程。
- 维护成本：2/10。后续如果需要单块复制 switch 并改 target，应该新增显式 shared API，而不是让调用方只改 successors。

# 2026-06-22 实现记录：收紧 switch default 候选

本轮对照 Angr 的 `SwitchDefaultCaseDuplicator` 后，只补一个 shared pass 语义约束：Angr 只把 out-degree 为 1 的 default-case node 纳入处理。当前 NotDec 之前会先按 default target 聚合，再检查 default block shape，这会让两个 switch 共享 terminal default 时也进入 shared-default forwarder 路径。

本轮改成：default target 必须存在且 `Graph.successorsOf(DefaultTarget).size() == 1`，才记录到 `SwitchPredsByDefault`。terminal default、多出口 default 暂时不做 rewrite/copy，避免把未覆盖的 default 形状误当成 Angr 语义。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:824`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 先检查 default block 是单后继，再记录 shared default candidate。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1945`
  新增 `testSwitchDefaultCaseDuplicatorSkipsTerminalSharedDefault()`，验证两个 switch 共享 terminal return default 时不插 synthetic forwarder。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5068`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.default-terminal-skip.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=183560
```

复杂度评分：

- 实现效果：5/10。减少一个和 Angr 不一致的 default rewrite 入口，避免 terminal default 被错误 forward。
- 理解成本：2/10。只是把候选记录放到 shape 检查之后。
- 维护成本：2/10。后续要支持 terminal default 时，需要先确认 Angr/NotDec 语义取舍，再单独补 shared 表达。

# 2026-06-22 实现记录：SwitchReusedEntryRewriter 只处理 case target

本轮继续收紧 switch deoptimization 的 pass 边界。Angr 的 `SwitchReusedEntryRewriter` 针对 jump table entry node；default case 复用由 `SwitchDefaultCaseDuplicator` 处理。当前 NotDec 之前用 `successorsOf()` 判断 switch 是否触达 entry，会把 default successor 也当成 reused entry，导致两个 pass 的语义边界混在一起。

本轮改成：`SwitchReusedEntryRewriter` 只扫描 `SwitchCase::Target`，不再把 default successor 当 entry。这样 default reuse 继续交给 default pass，reused-entry pass 只处理 case entry reuse。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:301`
  `switchReachesBlock()` 改为 `switchCaseReachesBlock()`，只检查 `CFGBlock::Cases`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:649`
  `SwitchReusedEntryRewriter::runOnGraph()` 使用 case-only helper 收集 switch predecessors。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2106`
  将 reused-entry 正向测试改成真实 case target 复用，而不是 default successor 复用。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2288`
  新增 `testSwitchReusedEntryRewriterSkipsDefaultOnlyTargets()`，验证 default-only 共享不会触发 reused-entry rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2310`
  limit 测试也改成 case target 复用，避免因为 default 不再扫描而失去覆盖。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.reused-entry-case-only.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=186480
```

复杂度评分：

- 实现效果：6/10。拆清 reused-entry 和 default reuse 的 shared pass 边界，减少错误 rewrite 入口。
- 理解成本：2/10。helper 从 all successors 改成 cases，测试数据同步改成 case target。
- 维护成本：2/10。后续如果要处理 default entry-like 形状，应在 default pass 或新 shared pass 里明确建模。

# 2026-06-22 实现记录：拒绝不完整的 copied switch materialize

本轮没有扩大 CrossJumpReverter 或 ReturnDuplicatorLow 的匹配范围。对照 Angr 后，更一般的复制仍会涉及 payload / vvar / Phi 重写，当前 shared CFG 还不能表达清楚。

本轮继续补 copied payload 边界：`materializeBlockBody()` 之前在 copied switch 的 case 数量和 body source 不一致时会静默成功，只是不复制 case value。这会留下一个 `BodyMaterialized=true` 但 payload 不完整的 copy。现在改成 case 数量不一致就直接失败，pass 不能把这种 copy 当成已 materialize。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:170`
  `StructuredCFG::materializeBlockBody()` 在 copied block 和 body source 的 `Cases.size()` 不一致时返回 false。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:178`
  case 数量一致时直接复制每个 `SwitchCase::Value`，不再静默跳过。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:917`
  新增 `testStructuredCFGMaterializeRejectsMismatchedSwitchCases()`，验证不完整 copied switch materialize 失败且 copy 仍保持未 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5085`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.materialize-case-count.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=185896
```

复杂度评分：

- 实现效果：6/10。避免 copied switch 在 case payload 不完整时被标记为 materialized。
- 理解成本：2/10。只收紧 `materializeBlockBody()` 的失败条件。
- 维护成本：2/10。后续若要支持 case count 改写，需要先新增 shared payload 重写语义。

# 2026-06-22 实现记录：修正 optimization trial rollback 的 changed 语义

本轮修 shared optimization wrapper 的回滚语义。`StructuringOptimizationPass::analyze()` 之前在 `runOnGraph()` 返回 true 后立刻把 `HadChanges` 置 true；如果随后 structuring trial 失败并 rollback，这次被回滚的改图仍会让 pass 最后可能返回成功。这不符合 checkpoint / rollback 的边界：只有最终保留下来的候选改图才算 pass changed。

本轮改成：candidate 通过 structuring trial 后才设置 `HadChanges`。如果一次改图被 rollback，且没有后续成功改图，pass 返回未成功，不把原图当成已优化结果。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:77`
  `StructuringOptimizationPass::analyze()` 把 `HadChanges = true` 移到 trial 成功之后。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2679`
  新增 `testStructuringOptimizationPassRejectsOnlyRolledBackChanges()`，验证只有 rollback 掉的改动时 pass 不成功。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5150`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.rollback-change-guard.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=185084
```

复杂度评分：

- 实现效果：6/10。rollback 掉的改图不再被当成成功优化，shared trial 边界更稳。
- 理解成本：2/10。只移动一个状态标记，并补 focused test。
- 维护成本：2/10。后续 pass 可以继续依赖 wrapper 做 trial/rollback 判定。

# 2026-06-22 实现记录：删除 body source 前保护 copied body

本轮继续收紧 copied block 的 body-source 生命周期。上一轮已经让 `materializeBlockBody()` 在 copied switch case payload 不完整时失败；但 `removeBlock()` 删除 body source 时仍会忽略 materialize 失败，可能删除 source 后留下 `BodyBlock` 指向已删除 block 的 copy。

本轮改成：`removeBlock()` 删除 block 前，先 materialize 所有依赖该 block 作为 body source 的其他 block。只要有一个 materialize 失败，就拒绝删除，并且不删边、不删 case target。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:313`
  `StructuredCFG::removeBlock()` 删除前先 materialize 依赖待删 block 的 copies；失败则返回 false。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:321`
  materialize 后再次确认没有其他 block 仍指向待删 body source，再执行删边和删 block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:874`
  新增 `testStructuredCFGRemoveBlockRejectsUnmaterializedCopy()`，验证 copied switch 无法 materialize 时 source 不被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5128`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.remove-body-source-guard.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=186240
```

复杂度评分：

- 实现效果：6/10。避免删除 body source 后留下悬空 copied body。
- 理解成本：2/10。删除前多一层 materialize 成功检查。
- 维护成本：2/10。后续 payload rewrite 变复杂时，删除 source 仍统一走 shared guard。

# 2026-06-22 实现记录：removeBlock materialize 失败保持原子性

本轮继续收紧 `removeBlock()` 的删除边界。上一轮删除 body source 前会尝试 materialize 所有依赖它的 copied block；但如果前面的 copy materialize 成功、后面的 copy 失败，函数会拒绝删除 source，却已经留下部分 copy 被 materialize 的副作用。

本轮改成：删除前先收集所有 body-user，并预检 copied switch 的 case payload 数量是否能 materialize。只有全部通过后才真正 materialize，避免失败路径改动 CFG。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:313`
  `StructuredCFG::removeBlock()` 先收集依赖待删 block 的 `BodyUsers`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:316`
  materialize 前预检 copied block 和 body source 的 `Cases.size()`，失败则直接返回 false。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:323`
  所有预检通过后再逐个调用 `materializeBlockBody()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:902`
  新增 `testStructuredCFGRemoveBlockIsAtomicOnMaterializeFailure()`，验证一个可 materialize copy 和一个不可 materialize copy 同时依赖 source 时，失败不会让前者被部分 materialize。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5170`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.remove-body-source-atomic.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=182152
```

复杂度评分：

- 实现效果：6/10。删除 body source 的失败路径不再留下部分 materialize 状态。
- 理解成本：2/10。只把已有 materialize 检查拆成预检和执行两步。
- 维护成本：2/10。后续如果 materialize 增加新的失败条件，需要同步补预检或改成事务式 rollback。

# 2026-06-22 实现记录：SAILR 删除 original region 前做原子预检

本轮继续处理 copied body-source 和 SAILR deoptimization 的交界。前面已经让 `removeBlock()` 在删除 body source 失败时保持原子，但多个 pass 删除 original region 时仍是逐个 `removeBlock()`，并且不检查失败结果。这样会有两个问题：后面的 block 删除失败时，前面的 block 可能已经被删；或者 source 删除被 shared CFG 拒绝，但 pass 仍继续把这次改图当成成功。

本轮改成：shared `StructuredCFG` 提供 `removeBlocks()`，先在临时 CFG 上按当前 `removeBlock()` 语义试删，全部成功才提交结果。`LoweredSwitchSimplifier`、`ReturnDuplicatorLow`、`CrossJumpReverter` 在准备删除 original region 前先用这个接口做预检；预检失败就跳过该 candidate，不复制和重定向。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:133`
  新增 `StructuredCFG::removeBlocks()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:348`
  新增 `StructuredCFG::removeBlocks()`，用临时 CFG 试删所有 block，成功后再整体提交。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:782`
  `LoweredSwitchSimplifier::runOnGraph()` 删除 original case region 前先预检 `removeBlocks()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1035`
  `ReturnDuplicatorLow::runOnGraph()` 删除 original return region 前先预检 `removeBlocks()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1117`
  `CrossJumpReverter::runOnGraph()` 删除 original copied region 前先预检 `removeBlocks()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:943`
  新增 `testStructuredCFGRemoveBlocksIsAtomicOnLaterFailure()`，验证批量删除后半段失败时前半段不会被删。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2559`
  新增 `testLoweredSwitchSimplifierSkipsUnsafeOriginalDeletion()`，验证 original region 不能安全删除时 pass 不复制、不重定向、不改图。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.atomic-region-removal.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.11 user=0.07 sys=0.03 maxrss=182560
```

复杂度评分：

- 实现效果：6/10。original region 删除失败时不再留下半删或半复制状态。
- 理解成本：2/10。新增一个 shared CFG 批量删除接口，pass 只做删除前预检。
- 维护成本：3/10。`removeBlocks()` 复制 CFG 做预检，简单但有额外成本；当前只在 deoptimization 删除 original region 前使用，范围可控。

# 2026-06-22 实现记录：DuplicationReverter merge 提交保持原子性

本轮继续收紧 shared deoptimization pass 的提交边界。`DuplicationReverter` 之前会先把 `DropId` 的 predecessor 重定向到 `Keep`，再调用 `removeBlock(DropId)`，并且不检查删除结果。现在 `removeBlock()` 会保护 copied body-source，如果 drop block 不能删，旧逻辑会留下 predecessor 已经改向、drop block 仍存在的半改图。

本轮改成：`DuplicationReverter` 在临时 `StructuredCFG` 上同时执行 redirect 和 remove。两步都成功后才把临时图提交回原图；任一步失败就跳过这个 candidate。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:973`
  `DuplicationReverter::runOnGraph()` 新建 `Candidate`，在候选图里执行 `redirectPredecessors()` 和 `removeBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:979`
  只有 redirect 和 remove 都成功后才 `Graph = std::move(Candidate)` 并返回 changed。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1466`
  新增 `testDuplicationReverterSkipsWhenDropCannotBeRemoved()`，验证 drop block 作为 copied body-source 且无法删除时，predecessor 不会被半重定向。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5308`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.duplication-reverter-atomic.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.06 sys=0.03 maxrss=185796
```

复杂度评分：

- 实现效果：6/10。DuplicationReverter 不再把失败删除留下的半重定向当成有效改图。
- 理解成本：2/10。局部引入临时 CFG 提交流程，没有改变匹配规则。
- 维护成本：2/10。后续如果 merge 逻辑扩展，仍可沿用这个 candidate 提交边界。

# 2026-06-22 实现记录：LoweredSwitchSimplifier 只看 case target

本轮继续收紧 switch deoptimization 的 shared 边界。`LoweredSwitchSimplifier` 之前用 `Graph.hasEdge()` 搜索候选 predecessor，这会把 switch 的 default successor 也算进去。这样 `switch case` 的简化和 `default` reuse 混在一起，和 `SwitchDefaultCaseDuplicator` 的 shared 边界不一致。

本轮改成：`LoweredSwitchSimplifier` 只把 `switchCaseReachesBlock()` 命中的 switch 作为候选 predecessor，也就是只看 `SwitchCase::Target`，不再把 default successor 当成 lowered case 处理。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:747`
  `LoweredSwitchSimplifier::runOnGraph()` 的 predecessor 收集从 `Graph.hasEdge()` 改成 `switchCaseReachesBlock()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2538`
  更新 `testLoweredSwitchSimplifierCopiesLinearSharedCaseRegion()`，改成真实的 case-target 共享形状。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2608`
  更新 `testLoweredSwitchSimplifierSkipsUnsafeOriginalDeletion()`，继续验证 unsafe 原始 region 不会被误删。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2651`
  新增 `testLoweredSwitchSimplifierSkipsDefaultOnlyTargets()`，只确认 default-only 目标不会被当成 lowered case 候选。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5365`
  将新测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.lowered-switch-case-only.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.04 sys=0.05 maxrss=181924
```

复杂度评分：

- 实现效果：5/10。`LoweredSwitchSimplifier` 不再把 default reuse 混进 case 简化。
- 理解成本：2/10。只收紧候选 predecessor 来源。
- 维护成本：2/10。`switchCaseReachesBlock()` 已经是 shared helper，后续默认/ case 边界更清楚。

# 2026-06-22 实现记录：SwitchDefaultCaseDuplicator 只处理 default reuse

本轮继续把 switch 相关 deoptimization 的共享边界拆清。前一轮已经把 `LoweredSwitchSimplifier` 限制为 case-target 语义；这轮主要确认 `SwitchDefaultCaseDuplicator` 仍然只负责 default reuse，不被 case 简化的搜索条件污染。

这次没有改 `SwitchDefaultCaseDuplicator` 的匹配规则本身，只是把 `LoweredSwitchSimplifier` 的候选 predecessor 收紧到 `SwitchCase::Target`，让 default-only 结构留给 default pass 处理。这样 case-only / default-only 的责任边界更清楚，避免两个 pass 都拿 `Graph.hasEdge()` 这种过宽条件去抢同一块图。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:747`
  `LoweredSwitchSimplifier::runOnGraph()` 继续使用 `switchCaseReachesBlock()`，只收 case target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2651`
  新增 `testLoweredSwitchSimplifierSkipsDefaultOnlyTargets()`，确认 default-only 目标不会被 case 简化误收。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5365`
  将测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.lowered-switch-case-only.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.04 sys=0.05 maxrss=181924
```

复杂度评分：

- 实现效果：5/10。case 简化和 default reuse 的边界更清楚。
- 理解成本：2/10。只是把候选源收紧到 case target。
- 维护成本：2/10。后续默认 reuse 再改时，不会撞到 case 简化的入口。

# 2026-06-22 实现记录：default-only 结构留给 SwitchDefaultCaseDuplicator

本轮继续把 switch 相关 pass 的责任边界收紧。前一轮把 `LoweredSwitchSimplifier` 限制成只收 case target，这一轮补了一个 default-only negative 测试，确认 default reuse 仍然不会被 case 简化入口误收。

这次没有改 `SwitchDefaultCaseDuplicator` 的核心逻辑，只是通过测试把边界钉死：default-only 的结构应该留在 default pass 里处理，不应该被 case 简化扫描到。这样后续如果再改 default reuse，不会误碰 lowered switch case 简化的入口。

修改内容：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2651`
  新增 `testLoweredSwitchSimplifierSkipsDefaultOnlyTargets()`，验证 default-only 目标块不会被 lowered switch case 简化误删。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2665`
  补齐测试所需的 default 结构 successor 块，保持图完整。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5368`
  将测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.switch-default-separate.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=186180
```

复杂度评分：

- 实现效果：4/10。默认分支和 case 简化的边界被测试钉住了。
- 理解成本：1/10。只是补了一个 negative test。
- 维护成本：1/10。边界清楚，后续 default reuse 调整不会误伤 case 简化。

# 2026-06-22 实现记录：SwitchReusedEntryRewriter 只看 case target

本轮继续收紧 switch 相关 pass 的共享边界。`SwitchReusedEntryRewriter` 之前在确认某个 predecessor component 还能到达 entry 时，用的是通用 `Graph.hasEdge()`。这样 default-only 复用也会被算进 reused-entry 逻辑，和前一轮已经收紧的 case/default 边界不一致。

本轮改成：`SwitchReusedEntryRewriter` 只把 `switchCaseReachesBlock()` 命中的 predecessor 当成候选，也就是只看 case target，不把 default successor 算进 reused-entry 的来源。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:700`
  `SwitchReusedEntryRewriter::runOnGraph()` 的 component 复核从 `Graph.hasEdge()` 改成 `switchCaseReachesBlock()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2465`
  更新 `testSwitchReusedEntryRewriterSkipsDefaultOnlyTargets()`，改成真正的 default-only 结构。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5364`
  将测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.switch-reused-entry-case-only.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=185744
```

复杂度评分：

- 实现效果：5/10。reused-entry 不再吞 default-only 结构。
- 理解成本：2/10。只把 reachability 复核收紧到 case target。
- 维护成本：2/10。default reuse 和 reused-entry 的边界更稳。

# 2026-06-22 实现记录：reused-entry 只看 case target

本轮继续收紧 switch 相关 pass 的共享边界。`SwitchReusedEntryRewriter` 之前在确认某个 predecessor component 还能到达 entry 时，用的是通用 `Graph.hasEdge()`。这样 default-only 复用也会被算进 reused-entry 逻辑，和前一轮已经收紧的 case/default 边界不一致。

本轮改成：`SwitchReusedEntryRewriter` 只把 `switchCaseReachesBlock()` 命中的 predecessor 当成候选，也就是只看 case target，不把 default successor 算进 reused-entry 的来源。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:700`
  `SwitchReusedEntryRewriter::runOnGraph()` 的 component 复核从 `Graph.hasEdge()` 改成 `switchCaseReachesBlock()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2465`
  更新 `testSwitchReusedEntryRewriterSkipsDefaultOnlyTargets()`，改成真正的 default-only 结构。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5364`
  将测试接入 `main()`。

验证：

```bash
cmake --build /sn640/NotDec/build --target structuring-analysis-test -j4
/sn640/NotDec/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --algo=structured-sailr /tmp/notdec-while-linear-body.ll \
  -o /tmp/notdec-while-linear-body.switch-reused-entry-case-only.c
```

结果：

```text
structuring-analysis-test: passed
CTest structuring subset: 100% passed
notdec-llvm2c smoke: elapsed=0.09 user=0.05 sys=0.03 maxrss=185744
```

复杂度评分：

- 实现效果：5/10。reused-entry 不再吞 default-only 结构。
- 理解成本：2/10。只把 reachability 复核收紧到 case target。
- 维护成本：2/10。default reuse 和 reused-entry 的边界更稳。

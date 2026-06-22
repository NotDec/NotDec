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

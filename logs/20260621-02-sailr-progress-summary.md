# SAILR structuring 当前进展总结

可以先读 `logs/20260621-01-sailr-overlay-manager-plan.md` 前 73 行了解之前的规划。73 行之后都是进度记录。

## 当前状态

当前 goal 还没完成。已经完成的是 Angr 风格 structuring 的公共基础层、copied / virtual block 的最小 shared 表示、shared region copy API、`CrossJumpReverter`、`DuplicationReverter` 的 exact-match shared 子集、`SwitchReusedEntryRewriter` 的 shared reused-entry / entry-tail region 子集、`SwitchDefaultCaseDuplicator` 的 shared default reuse / default-tail region 子集、`LoweredSwitchSimplifier` 的 shared case-region 复制子集，以及 `ReturnDuplicatorLow` 的线性 return-tail、连通前驱组件共享复制、terminal fork 和 return-tail 后续 goto edge shared 子集；还没完成的是完整 SAILR deoptimization 算法。

当前 goal 按下面这版执行：

```text
在 /sn640/NotDec2 这个 worktree 上，基于最新 v2 分支继续推进 llvm2c 的 structuring 重构，严格按 Angr 的结构恢复框架落地。先补齐 OverlayManager / RegionOverlay / 可回滚共享图这套基础架构，让 structuring 的 region view、child finalize / dissolve、successor snapshot、checkpoint / rollback、node ownership 和 parent-visible graph 语义尽量与 Angr 一致；再在同一套基础上继续实现 Angr 的 Phoenix / SAILR 后续部分，包括递归 structuring 流程、cyclic refinement、virtual edge 选择、deoptimization / last-resort refinement、以及后续可扩展的结构恢复算法接入口。只保留 NotDec 后端渲染真正需要的最小差异，避免把 fallback 渲染当成算法语义本身。遇到以下情况就暂停 goal：1) 需要在 Angr 语义和当前 NotDec 输出质量之间做明确取舍；2) 现有计划没覆盖到的结构边界、回滚语义或算法接口问题；3) 无法在代码或 Angr 对照里确认的技术决策点。
```

工作位置：

- 顶层 worktree：`/sn640/NotDec2`
- 顶层分支：`sailr-overlay-v2`
- 主要子模块：`external/NotDec-llvm2c`
- 子模块分支：`sailr-overlay-v2`

最近提交：

- 顶层：`3261e3e Record structuring optimization pipeline`
- llvm2c 子模块：`9b91479 Add shared structuring optimization pipeline`

## 已经实现的部分

已经有 shared structuring 骨架：

- `StructuredCFG`
- `StructuredTree`
- `Region`
- `RegionIdentifier`
- `RecursiveStructurer`
- `RegionStructurer`

已经有 Angr 风格 overlay 基础：

- `OverlayManager`
- `RegionOverlay`
- child region `finalize` / `dissolve`
- successor snapshot
- checkpoint / rollback
- owner / member 关系
- quotient / full / acyclic view
- overlay edge marks

Phoenix 已经迁到 shared reducer 路线：

- reducer collapse 会同步 overlay
- virtual edge 会同步 overlay
- cyclic refinement 会标记 break / continue / successor goto

SAILR 当前完成的是 Phoenix 变体部分：

- SAILR 的 virtual edge ordering heuristic
- improved Phoenix 开关

Angr 风格 structuring trial 已经实现：

- `StructuringEvaluator`
- trial 会做 region identify、recursive structuring、goto 收集、quality 统计
- 支持 `edges_to_remove`，对齐 Angr `_graph_is_structurable()`

Angr 风格 optimization pass wrapper 已经实现：

- 初始 structuring 检查
- require gotos
- prevent new gotos
- strictly-less-gotos
- relative quality check
- fixed-point retry
- 失败 rollback 后继续
- `_get_new_gotos()` 风格钩子

SAILR deoptimization pipeline 骨架已经实现：

- `StructuringOptimizationPipeline`
- pass 成功才更新当前 graph
- pass 失败或无变化就跳过
- `StructuredCFG::redirectPredecessors()` 支持原子 predecessor 重定向，避免 copied block pass 失败时留下半改图

第一个具体 SAILR deoptimization pass 已经实现：

- `CrossJumpReverter`
- `DuplicationReverter` 的 exact-match shared 子集
- `SwitchReusedEntryRewriter` 的 shared reused-entry 子集
- `SwitchDefaultCaseDuplicator` 的 shared default reuse 子集
- `ReturnDuplicatorLow` 的单 block / 线性 return-tail 复制子集
- `ReturnDuplicatorLow` 的连通前驱组件共享复制子集
- `ReturnDuplicatorLow` 的 terminal fork shared 子集
- 已接入 `buildSAILRDeoptimizationPipeline()`
- `SAILRStructurer::structure()` 会先跑 shared deoptimization pipeline，再走 Phoenix/SAILR structuring

## 当前实现方式

现在的核心链路是：

```text
StructuredCFG
  -> RegionIdentifier / OverlayManager
  -> RecursiveStructurer
  -> PhoenixStructurer / SAILRStructurer
  -> StructuredTree
  -> GotoManager / StructuringQuality
  -> StructuringOptimizationPass
  -> StructuringOptimizationPipeline
```

算法层只操作 shared CFG / region / structured tree，不直接碰 C renderer 或 Solidity renderer。

这和 Angr 的模型一致：SAILR 不是单独一个大 structurer，而是 Phoenix structurer 的变体，加更好的 virtual edge 选择，再加一组 deoptimization pass 和 shared structuring trial / goto / quality guard。

## 验证情况

最近一轮验证通过：

```bash
cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4
/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pipeline.c --tr-level=2 --algo=phoenix
```

fortune smoke 通过，耗时：

```text
elapsed=31.76 user=39.12 sys=0.04 maxrss=221724
```

## copied / virtual block 进展

已经补了 SAILR deoptimization 继续往下做需要的最小 shared block 表示：

- `StructuredCFG::CFGBlock` 增加 `BodyBlock`。
- `StructuredCFG::duplicateBlock()` 可以创建稳定新 `BlockId` 的 copied block。
- `StructuredCFG::bodyBlock()` / `getBodyBlock()` 提供 body-source 查询。
- Phoenix / Goto structurer 生成 `BasicBlock` 时保留控制流 `BlockId`，statements 从 `BodyBlock` 读取。
- C adapter 的 label / goto 改为按 shared `BlockId` 建 label，不再依赖原始 `CFGBlock *`。
- C adapter 的 fallback if / switch 改为从 shared `StructuredCFG` 读取 successor。
- C adapter 修了 condition payload 原地取反问题，避免同一个 condition payload 被多个 structured node 复用时互相改坏。

本轮又补了一个更窄的 shared switch pass，用来覆盖 Angr 里 reused entry 的最小安全子集：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:51`
  新增 `SwitchReusedEntryRewriter` pass 类。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:242`
  实现 `SwitchReusedEntryRewriter::defaultOptions()` 和 `SwitchReusedEntryRewriter::runOnGraph()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:619`
  `buildSAILRDeoptimizationPipeline()` 把这个 pass 接到 shared deoptimization pipeline 里。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:923`
  新增 `testSwitchReusedEntryRewriterCopiesReusedEntryBlock()`，验证一个 entry 被多个 switch 复用时会复制出新的 shared block，原 entry 仍保留给第一个 predecessor。

实现里特意先快照了 `EntryId` 列表，再做复制和 successor 重写，避免遍历 `Graph.blocks()` 时直接改图。这个 pass 只复制 block，不碰 renderer，也不把 switch lowering 的 if-chain 语义塞进算法层。

本轮继续把 `ReturnDuplicatorLow` 往 Angr 靠了一点，补了“连通前驱组件只复制一次”的 shared 子集：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:163`
  新增 `connectedPredecessorComponents()`，按前驱连通性分组。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:238`
  把单个 predecessor 复制 helper 改成 `copyRegionForPredecessors()`，一次复制整段 return region，再把同一组件里的 predecessor 都指向同一个 copied head。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:545`
  `ReturnDuplicatorLow::runOnGraph()` 先选出需要复制的 predecessor，再按连通组件分组后复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:885`
  新增 `testReturnDuplicatorLowCopiesConnectedPredsOnce()`，验证两个连通 predecessor 会共享同一个 return-tail copy，而不是复制两份。

这里仍然只做 shared CFG 级别的 region copy，不碰 Phi / vvar 重写，也不把 renderer 特判塞进算法层。

本轮又补了 Angr `_single_entry_region()` 里 terminal fork 的 shared 子集。只识别一个保守形状：

```text
branch -> return
       -> unreachable/return
```

两个 terminal successor 都必须没有后继，且都只能由这个 branch 到达。这个形状常见于 stack-canary 风格的 return fork，shared CFG 能直接复制，不需要解释 Phi / vvar，也不需要 renderer 介入。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:108`
  新增 `isClosedTerminal()`，只认闭合的 `Return` / `Unreachable` terminal block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:114`
  新增 `prependTerminalForkRegion()`，把 `branch -> {terminal, terminal}` 纳入 return region。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:152`
  `findLinearReturnRegion()` 在直线回溯前先尝试吸收 terminal fork；除此之外的一般分支 region 仍然不做。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:926`
  新增 `testReturnDuplicatorLowCopiesTerminalForkRegion()`，验证复制时会带上 fork head、return terminal 和 unreachable terminal，并删除原 region。

本轮仍未实现一般分支 return-region、Phi / vvar 刷新、connected in-edge component 的完整 Angr 语义。

本轮补了一处 shared CFG 改图原子性，给后续 copied block pass 复用：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  新增 `StructuredCFG::redirectPredecessors()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:90`
  实现两阶段重定向：先验证所有 predecessor 都有目标边，再统一替换 successor / switch case target。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:333`
  `copyRegionForPredecessors()` 改用 `redirectPredecessors()`，避免复制 return region 后半途中失败导致 predecessor 指向已删除 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:753`
  新增 `testStructuredCFGRedirectPredecessorsIsAtomic()`，验证失败时不会改任何 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:772`
  新增 `testStructuredCFGRedirectPredecessorsUpdatesSwitchCases()`，验证 switch successor 和 case target 会一起改。

这个改动是 shared graph 语义，不属于 C 或 Solidity renderer 特判。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-switchentry.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.86 user=39.11 sys=0.07 maxrss=220476`。

本轮验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retdup2.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.51 user=38.83 sys=0.06 maxrss=219448`。

本轮验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retfork.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.04 user=39.31 sys=0.04 maxrss=220412`。

本轮验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-redir.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.44 user=39.60 sys=0.06 maxrss=219352`。

验证通过：

```bash
cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4
/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-copied-block.c --tr-level=2 --algo=phoenix
```

fortune smoke 通过，耗时：

```text
elapsed=32.34 user=39.80 sys=0.11 maxrss=220248
```

本轮又补了一个更窄的 shared 渲染入口，让 copied / virtual block 的 shared CFG 表示能同时被 C 和 Solidity 复用：

- `external/NotDec-llvm2c/include/notdec-backends/Solidity/BodyBuilder.h:15-29`
  新增 `renderStructuredBody()` 声明，直接接 `StructuredTree` 和 payload 列表。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:204-221`
  `readBody()` 先走 shared `StructuredTree` 渲染，再补原有 fallback 注释；`renderStructuredBody()` 单独负责 tree 渲染，不把算法逻辑塞进 renderer。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:737-760`
  新增 `testSolidityBodyBuilderRendersVirtualBlockBodySource()`，验证 shared tree 里的 block label 和 copied body payload 会一起渲染出来。

这次只动了 renderer 接口，不改 structuring 算法本身，也没有把 Solidity 特判放回共享 structuring 里。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-solidity structuring-analysis-test -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pipeline.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.69 user=39.03 sys=0.04 maxrss=220780`。

本轮继续补 `ReturnDuplicatorLow`，对齐 Angr `ReturnDuplicatorLow._is_goto_edge()` 里“goto source 可能在相邻 parent block 上”的窄语义。没有推进 `LoweredSwitchSimplifier`，因为 Angr 那边依赖 AIL 表达式相似性、case cluster 和 block copy 的 payload 更新；当前 shared CFG 还没有足够语义直接照搬。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:280`
  新增 `gotoEdgeFromSourceOrParent()`，先查直接 goto edge，再查 source 的一层 predecessor 是否是 goto source。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:616`
  `ReturnDuplicatorLow::runOnGraph()` 用这个 helper 选择要复制 return region 的 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:84`
  新增 `TestReturnDuplicatorLow`，测试直接调用 shared pass 的 `runOnGraph()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:993`
  新增 `testReturnDuplicatorLowUsesParentGotoSource()`，手工构造 `GotoManager` 里的 parent goto，验证只复制对应 predecessor 的 return block，且 copy 继续通过 `BodyBlock` 指向原 return body。

这次仍然只改 shared deoptimization 判定，不碰 C / Solidity renderer。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retdup-parentgoto.c --tr-level=2 --algo=structured-phoenix`
  通过，`elapsed=36.30 user=43.69 sys=0.06 maxrss=219596`。当前 CLI 没有 `--algo=phoenix` literal，所以用 `structured-phoenix`。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retdup-parentgoto-sailr.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.76 user=80.06 sys=0.05 maxrss=220232`。

本轮补了一个更基础的 shared CFG 边识别细节，给 switch copied-block pass 兜底。之前 `SAILRDeoptimization.cpp` 里的本地 `hasSuccessor()` 只看 `CFGBlock::Successors`，但 shared CFG 的 switch case target 也保存在 `CFGBlock::Cases`。虽然当前 LLVM / C adapter 通常会把 case target 同步放进 `Successors`，算法层不应该依赖这个冗余不变量。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:12`
  `hasSuccessor()` 改为同时检查 `Successors` 和 `Cases`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1181`
  新增 `testSwitchReusedEntryRewriterReadsCaseOnlyTargets()`，构造 case target 只在 `Cases` 里的 switch，验证 `SwitchReusedEntryRewriter` 仍能复制 reused entry block，copy 的 `BodyBlock` 仍指向原 body。

这个改动仍只在 shared deoptimization 图语义里，不碰 C / Solidity renderer。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-switch-case-edge-sailr.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.85 user=80.11 sys=0.07 maxrss=222508`。

本轮把 shared CFG 的边查询从 SAILR pass 本地 helper 提到 `StructuredCFG`，避免后续 pass 各自重复定义“边”的含义。

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  新增 `StructuredCFG::hasEdge()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:77`
  新增 `StructuredCFG::predecessorsOf()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:90`
  `hasEdge()` 复用 shared edge 判断，同时覆盖普通 successor 和 switch case target。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:95`
  `predecessorsOf()` 统一返回 shared CFG predecessor。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:53`
  SAILR deoptimization 改用 `Graph.predecessorsOf()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:409`
  copied-block pass 改用 `Graph.hasEdge()` 判断 predecessor 是否仍指向目标。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:820`
  新增 `testStructuredCFGFindsCaseOnlyPredecessors()`，验证 case-only target 也能被 `hasEdge()` / `predecessorsOf()` 看见。

这个改动是 shared graph 基础，不涉及 C / Solidity renderer。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-shared-edge-api-sailr.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.54 user=79.88 sys=0.04 maxrss=220016`。

本轮继续把“单个 predecessor 替换一条 shared CFG 边”提到 `StructuredCFG`，让 copied-block pass 不再自己维护 `Successors` / `Cases` 的更新规则：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76-81`
  新增 `StructuredCFG::replaceEdge()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:90-127`
  实现 `replaceEdge()`，并让 `redirectPredecessors()` 复用同一套边替换逻辑。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:391-401,465-476,532-538,680-690`
  `SwitchReusedEntryRewriter`、`SwitchDefaultCaseDuplicator`、`DuplicationReverter`、`CrossJumpReverter` 改用 `Graph.replaceEdge()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:820-837`
  新增 `testStructuredCFGReplaceEdgeUpdatesSwitchCases()`，确认 `replaceEdge()` 会同时改普通 successor 和 case target。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-replaceedge.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=73.49 user=81.02 sys=0.10 maxrss=222828`。

本轮再收紧了一处 `ReturnDuplicatorLow` 的 shared 判定，补 Angr 里“紧挨着的 if-stmt 父块要跳过”的边界：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:253-273`
  `gotoEdgeFromSourceOrParent()` 现在会跳过 `TerminatorKind::Branch` 的直接父块，只接受更上层的 goto source。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1077-1132`
  新增 `testReturnDuplicatorLowSkipsBranchParentGotoSource()`，确认分支父块上的 goto 不会把旁边的 return predecessor 一起拖进复制。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-return-skip.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.87 user=80.03 sys=0.04 maxrss=218072`。

本轮把 shared CFG 的 successor snapshot 也提到 `StructuredCFG`，避免 deoptimization pass 继续自己拼 `Successors` / `Cases`：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76-79`
  新增 `StructuredCFG::successorsOf()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:90-109`
  `successorsOf()` 返回普通 successor 和 switch case target 的统一视图。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:217,233-278,368-385,430-465,651-674`
  `connectedPredecessorComponents()`、`switchReachesBlock()`、`defaultSwitchSuccessor()`、`SwitchReusedEntryRewriter`、`SwitchDefaultCaseDuplicator` 和 `CrossJumpReverter` 改用 shared successor 视图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:852-862`
  新增 `testStructuredCFGSuccessorsOfIncludesCaseTargets()`，确认 case-only target 也在 successor snapshot 里。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-successors.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.48 user=79.81 sys=0.06 maxrss=222304`。

本轮把 `successorsOf()` 再收紧了一层，避免 switch 的同一个 target 在 `Successors` 和 `Cases` 里同时出现时被重复算两次：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:8-30,95-109`
  新增 `appendUniqueTarget()`，让 `StructuredCFG::successorsOf()` 按出现顺序去重返回。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:852-875`
  新增 `testStructuredCFGSuccessorsOfDeduplicatesCaseTargets()`，确认 case target 重复存放时 `successorsOf()` 仍只返回一次。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:217,233-278,381-385,436-465,652-674`
  相关 shared successor 读取逻辑继续复用这个去重视图。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-successors-dedup.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.95 user=80.28 sys=0.06 maxrss=221380`。

本轮继续把 copied / virtual block 需要的 shared region copy 收口到 `StructuredCFG`，让 deoptimization pass 不再自己拼复制、重定向和回滚流程：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:64-78`
  新增 `DuplicatedRegion` 和 `StructuredCFG::duplicateRegion()` 声明，复制 bookkeeping 进入 shared CFG。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:70-183`
  实现 `DuplicatedRegion::copyOf()` 和 `StructuredCFG::duplicateRegion()`，先批量复制，再统一重写 region 内部 successor / case target；中途失败会删掉已建 copy。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:281-304`
  `copyRegionForPredecessors()` 改为直接复用 `duplicateRegion()`，只保留 predecessor redirect 和失败回滚。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:876-917`
  新增 `testStructuredCFGDuplicateRegionRewritesInternalEdges()` 和 `testStructuredCFGDuplicateRegionRollsBackOnMissingBlock()`，分别覆盖 region 内边重写和缺块回滚。

这次还是只做 shared CFG 语义，不把 renderer 特判塞回算法层。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-copy-region.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=72.01 user=79.38 sys=0.05 maxrss=217764`。

本轮又补了一个窄版 `LoweredSwitchSimplifier`，只处理“多个 switch 前驱共享同一段线性 case 链”的形状，整段用 `duplicateRegion()` 复制，再把各个前驱重定向到自己的 copy：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:64-75`
  新增 `LoweredSwitchSimplifier` pass 类。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:312-371`
  新增 `findLinearCopyRegion()` 和 `copyLinearRegionForPredecessors()`，把共享 case 链按 shared region 复制。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:449-525`
  实现 `LoweredSwitchSimplifier::defaultOptions()` / `runOnGraph()`，只在所有前驱都是 switch 且目标是线性链时才动手。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:826-833`
  把 `LoweredSwitchSimplifier` 接进 `buildSAILRDeoptimizationPipeline()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:84-88`
  新增 `TestLoweredSwitchSimplifier`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1392-1458`
  新增 `testLoweredSwitchSimplifierCopiesLinearSharedCaseRegion()`，验证一段三块线性 case 链会被整段复制，不再只复制入口块。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4010-4013`
  把新测试挂到主测试入口。

这版还不是 Angr 的完整 lowered-switch 重建，只是 shared CFG 上的保守复制子集。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=78.50 user=85.71 sys=0.08 maxrss=219508`。
- 同口径重复跑一次：
  `elapsed=77.69 user=85.07 sys=0.07 maxrss=221644`。

本轮继续扩了 `LoweredSwitchSimplifier` 的 shared case-region 复制范围。现在除了线性 case 链，也能把链尾的闭合 terminal fork 一起复制：

```text
case head -> branch -> return
                    -> unreachable/return
```

约束仍然很保守：fork head 必须是普通 branch，两个 successor 都必须是闭合 `Return` / `Unreachable`，且都只能由这个 branch 到达。这样只是在 shared CFG 里复制 block，不解释 renderer 语义，也不碰 Phi / vvar。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:312`
  新增 `appendTerminalForkRegion()`，把满足条件的 terminal fork 追加进 `LinearRegion`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:347`
  `findLinearCopyRegion()` 在继续走单 successor 链前先尝试吸收 terminal fork，命中后停止扩展。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1460`
  新增 `testLoweredSwitchSimplifierCopiesTerminalForkCaseRegion()`，验证两个 switch 共享同一个带 terminal fork 的 case region 时，会分别得到完整 copy，并删除原 region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4089`
  把新测试接入主测试入口。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-lowered-switch-terminal.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=77.98 user=85.38 sys=0.07 maxrss=221288`。
- 提交前同口径重复跑一次：
  `elapsed=77.40 user=84.63 sys=0.06 maxrss=221612`。

本轮对照 Angr `/sn640/angr/angr/analyses/decompiler/optimization_passes/return_duplicator_low.py` 的 `_is_goto_edge()`，补了 `ReturnDuplicatorLow` 的另一个 shared CFG stopgap：如果 tested edge 进入 return region，但真实 goto 标记在 region head 后面的单后继 tail 上，也允许复制这段 return region。这对应 Angr 注释里的 Phoenix loop-region / successor goto 情况。

修改仍然只读 shared CFG 和 `GotoManager`：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:251`
  `gotoEdgeFromSourceOrParent()` 在直接 source goto 和 parent goto 之后，沿 `Target` 的单 successor 链查找 `Gotos.isGotoEdge(Node, Succ)`；遇到分叉、多前驱或自环就停止。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1275`
  新增 `testReturnDuplicatorLowUsesGotoInReturnTail()`，构造 `pred -> head -> tail -> return`，goto 标记在 `head -> tail`，验证两个 predecessor 都会拿到完整 return region copy，原 region 被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4163`
  把新测试接入主测试入口。

这一步不处理一般分支 return-region，也不刷新 Phi / vvar；这些仍然需要更明确的 shared payload 语义。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retdup-tail-goto.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=77.86 user=85.10 sys=0.05 maxrss=217768`。

本轮继续补 `SwitchDefaultCaseDuplicator` 的 shared CFG 子集。之前只复制 reused default 的入口块；现在如果 default 后面是一段单 successor 链，会把整段 default-tail region 用 `duplicateRegion()` 一起复制，再把外部 predecessor 指向复制后的 head。

约束仍然保守：链路上每个后继只能有一个 predecessor，遇到分叉、环、缺块或 switch terminator 就停止。这里仍然不重建 switch，不新建 renderer 专用 goto block。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:374`
  新增 `findLinearRegionFromHead()`，从 reused default head 开始收集单 successor region。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:643`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 改为保存 default region，并复用 `copyLinearRegionForPredecessors()`；单块 default 仍走同一套路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1390`
  新增 `testSwitchDefaultCaseDuplicatorCopiesDefaultTailRegion()`，验证 `default -> tail -> return` 被外部 predecessor 复用时，复制的是 default head 和 tail，而不是只复制入口块。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4225`
  把新测试接入主测试入口。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-default-tail-copy.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=77.25 user=84.59 sys=0.06 maxrss=218796`。

本轮继续把 `SwitchReusedEntryRewriter` 从“只复制 entry block”扩成“复制 entry-tail region”。多个 switch 复用同一个 entry 时，除第一个 switch 继续保留原 entry，其他 switch 会拿到一份从 entry 开始的单 successor region copy。

这个改动复用 `findLinearRegionFromHead()` 和 `copyLinearRegionForPredecessors()`，所以边界和 default-tail 一样：只在 shared CFG 上复制线性 region，不重建 switch，也不碰 C / Solidity renderer。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:527`
  `SwitchReusedEntryRewriter::runOnGraph()` 改为先收集 entry-tail region，再对后续 switch predecessor 做整段 region copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1481`
  新增 `testSwitchReusedEntryRewriterCopiesEntryTailRegion()`，验证 `entry -> tail -> join` 被多个 switch 复用时，复制出来的 entry 会指向复制后的 tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4265`
  把新测试接入主测试入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1524`
  给 case-only target 测试增加一个外部 tail predecessor，使它继续只验证 case target 读取，不混入 tail region 复制语义。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-entry-tail-copy.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=78.09 user=85.38 sys=0.05 maxrss=218572`。

本轮继续补 `CrossJumpReverter` 的 shared CFG 子集。之前只复制 goto target 的单个 block；现在先从 target 收集一段保守的单 successor region，再用 `duplicateRegion()` 复制整段 region，region 内部边也随 copy 重写。

约束仍然保守：target 本身必须只有一个 successor，整段 region 的 statement 总数不能超过 `MaxDuplicatedStatements`，region 扩展仍复用 `findLinearRegionFromHead()` 的边界。这里不碰 C / Solidity renderer，也不把 fallback 渲染当算法语义。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:481`
  新增 `statementCountInRegion()`，按 shared `LinearRegion` 统计复制成本，缺块时返回最大值让 pass 放弃。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:872`
  `CrossJumpReverter::runOnGraph()` 改为保存 target 对应的 `LinearRegion`，并调用 `copyLinearRegionForPredecessors()` 复制整段 region；如果所有 predecessor 都被重定向，就删除原 region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:926`
  扩展 `testCrossJumpReverterDuplicatesLinearGotoTarget()`，把用例改成 `target -> tail -> exit`，验证两个 goto predecessor 会得到各自完整 copy，原 region 被删除，copy 能自持 payload。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-crossjump-region.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=77.84 user=85.12 sys=0.06 maxrss=220320`。

复杂度 / 维护判断：

- 实现效果：8/10。覆盖了 Angr cross-jump deopt 里更接近真实目标的“复制尾部 region”场景，但还不是完整 cross-jump region 搜索。
- 理解成本：3/10。只复用现有 `LinearRegion` 和 region copy helper，没有引入新图语义。
- 后期维护成本：3/10。边界保守，后续如果要做更完整 Angr cross-jump，可以继续替换 region 选择逻辑。

本轮对照 Angr `switch_default_case_duplicator.py` 里 default 被多个 switch head 复用时插入 goto block 的分支，补了 shared CFG 的最小 synthetic block 表示，并让 `SwitchDefaultCaseDuplicator` 用它插入 default forwarder。

NotDec 这里没有在算法层制造 C / Solidity 私有 goto 语句，而是新建一个无 payload 的 shared fallthrough block：`switch head -> synthetic forwarder -> original default`。C 和 Solidity 后端继续从同一棵 shared structured tree 渲染这个控制流块。

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  新增 `StructuredCFG::createSyntheticBlock()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:54`
  实现 `createSyntheticBlock()`，生成自持 `BodyBlock`、空 statements、`Fallthrough` terminator 和给定 successor 的 shared block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:661`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 先收集每个 default 的 switch predecessor。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:692`
  对多个 switch head 共享的 default，为每个 switch head 插入 synthetic forwarder，并跳过后续 default-tail copy 分支，贴近 Angr 的 goto-block 行为。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:926`
  新增 `testStructuredCFGCreateSyntheticBlock()`，锁住 synthetic block 的 shared 身份和 payload 边界。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1434`
  新增 `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()`，验证两个 switch 共享同一个 default 时，会插入两个无 payload forwarder，原 default 不被复制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4348`
  把新 synthetic block 测试接入主测试入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4363`
  把 default forwarder 测试接入主测试入口。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-default-forwarder.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=78.36 user=85.57 sys=0.07 maxrss=221548`。

复杂度 / 维护判断：

- 实现效果：8/10。补上了 Angr default 多 switch 复用场景里的 shared goto-block 等价物，也明确了 synthetic block 的 payload 边界。
- 理解成本：3/10。新增 API 很小，pass 仍只操作 `StructuredCFG`。
- 后期维护成本：3/10。后续如果需要真正带 payload 的 virtual block，需要另行定义 payload 来源；当前 forwarder 不需要这个语义。

本轮补了 synthetic forwarder 的 shared renderer 侧验证。没有改算法，只把之前新增的 synthetic block 通过 shared tree / Solidity renderer 的边界用测试锁住。

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:749`
  新增 `testGotoStructurerRendersSyntheticForwarder()`，验证 `GotoStructurer` 能把 synthetic block 结构化成无 statements 的 shared `BasicBlock`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:793`
  新增 `testSolidityBodyBuilderRendersSyntheticForwarder()`，验证 Solidity shared-tree renderer 能输出 synthetic forwarder 的 label / goto 注释，并且不会因为空 payload 输出 `unknown`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4387`
  把 synthetic `GotoStructurer` 测试接入主测试入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4389`
  把 Solidity synthetic renderer 测试接入主测试入口。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-synthetic-render.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=79.18 user=86.39 sys=0.10 maxrss=222180`。

本轮把 `CrossJumpReverter` 的 connected predecessor 复制策略补齐到和 `ReturnDuplicatorLow` 一致。之前同一个 goto target 的多个连通前驱会各自复制一份 target region；现在先按 shared CFG 连通性分组，同一组件只复制一次，然后把组件内所有仍然直达 target 的 predecessor 一起重定向到同一个 copied head。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:953`
  `CrossJumpReverter::runOnGraph()` 对 `PredsToUpdate` 调用 `connectedPredecessorComponents()`，逐组件检查 edge 仍存在。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:968`
  `CrossJumpReverter::runOnGraph()` 改为对整个组件调用 `copyLinearRegionForPredecessors()`，避免连通前驱重复复制同一段 linear region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:96`
  新增 `TestCrossJumpReverter`，让测试可以直接跑 protected `runOnGraph()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1068`
  新增 `testCrossJumpReverterCopiesConnectedPredsOnce()`，验证 `0 -> {3, target}` 且 `3 -> target` 时，`0` 和 `3` 会共享同一个 copied target region，原 target region 被删除。

这个改动仍只在 shared CFG / deoptimization pass 层处理 copied block，不碰 C renderer 或 Solidity renderer。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过，`ninja: no work to do`。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-crossjump-connected.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=78.74 user=86.05 sys=0.05 maxrss=221380`。

复杂度 / 维护判断：

- 实现效果：7/10。补上了 connected predecessor 共享 copied region 这一块，减少无意义复制，语义仍保守。
- 理解成本：2/10。复用已有 `connectedPredecessorComponents()` 和 `copyLinearRegionForPredecessors()`，没有新增算法接口。
- 后期维护成本：2/10。后续扩展完整 Angr cross-jump search 时，这里可以继续作为 predecessor 分组策略保留。

本轮补了 `DuplicationReverter` 的 shared graph 原子重定向。之前 exact-match merge 会逐个 predecessor 调 `replaceEdge()`，虽然当前常见用例没问题，但它绕开了已经实现的 `StructuredCFG::redirectPredecessors()`，也没有把 switch case target 的同步更新作为 pass 级行为锁住。现在 merge dropped block 时一次性验证并重定向所有 predecessor，和其他 copied-region pass 用同一套 shared CFG 语义。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:804`
  `DuplicationReverter::runOnGraph()` 改为调用 `Graph.redirectPredecessors(DropId, Keep->Id, DropPreds)`，成功后再删除 dropped block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1169`
  新增 `testDuplicationReverterRedirectsSwitchPredecessorCases()`，验证 switch predecessor 通过 case-only edge 指向 duplicate block 时，merge 后 case target 会指向 kept block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4509`
  把新测试接入 `structuring-analysis-test`。

这个改动没有改 renderer，也没有给 duplication merge 增加新的相似度语义；只是让当前 exact-match 子集复用 shared CFG 的原子 edge rewrite。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe notdec -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-dup-switch-redirect.c --tr-level=2 --algo=structured-sailr`
  通过，`elapsed=79.06 user=86.36 sys=0.06 maxrss=217996`。

复杂度 / 维护判断：

- 实现效果：6/10。没有扩展 Angr duplication search，但把现有 exact-match merge 接到 shared atomic rewrite 上，减少后续 pass 组合风险。
- 理解成本：1/10。删掉手写逐边重定向，复用已有 CFG API。
- 后期维护成本：1/10。后续如果补 similarity / merge graph，这里仍可作为最终 edge rewrite。

## 还差什么

还差真正的完整 Angr SAILR deoptimization pass：

- `LoweredSwitchSimplifier` 的完整 if-else 到 switch 重建
- 相关 switch / duplication 辅助逻辑

这些 pass 还没有完整实现。`CrossJumpReverter`、`DuplicationReverter`、`SwitchReusedEntryRewriter`、`SwitchDefaultCaseDuplicator` 和 `LoweredSwitchSimplifier` 都已经有最小 shared 版，`ReturnDuplicatorLow` 已经有单 block / 线性 return-tail 复制子集。

之前的主要卡点是：Angr 这些 pass 基本都会复制或新建 block。当前已经有最小 shared 表示，但具体 pass 还没有实现。

已处理 / 未处理例子：

- `CrossJumpReverter` 复制 goto target：已实现最小 shared 版
- `SwitchReusedEntryRewriter` 复制被多个 switch 复用的 entry block：已实现 shared reused-entry / entry-tail region 子集
- `ReturnDuplicatorLow` 复制 return block：已实现单 block / 线性 return-tail、连通前驱组件共享复制、terminal fork 和 return-tail 后续 goto edge 子集，缺一般分支 return-region / Phi / vvar 刷新
- `DuplicationReverter`：已实现 exact-match shared 合并子集，缺 similarity search / merge graph
- `SwitchDefaultCaseDuplicator`：已实现 shared default reuse / default-tail region 子集，缺 if-chain / lowering 回写
- `LoweredSwitchSimplifier`：已实现 shared case-region 复制子集，缺完整 if-else 到 switch 重建

当前 C renderer 的 label 身份已经改为 shared `BlockId`，body payload 通过 `BodyBlock` 走 shared CFG。后续 pass 仍要注意不要把新 block 的算法语义写到 C/Solidity renderer 里。

## 下一步建议

下一步可以继续补 `ReturnDuplicatorLow` 的分支 return-region / Phi 语义；如果这部分 shared payload 边界不清，再转向 `LoweredSwitchSimplifier` 里接口更窄的部分。

一句话总结：公共架构、Angr 风格执行框架、copied / virtual block 的 shared 表示、`CrossJumpReverter`、`DuplicationReverter` 的 exact-match 子集、`SwitchReusedEntryRewriter` 的 shared reused-entry / entry-tail region 子集、`SwitchDefaultCaseDuplicator` 的 shared default reuse / default-tail region 子集和 `ReturnDuplicatorLow` 的线性 return-tail / connected predecessor / terminal fork / tail-goto 子集已经搭好；完整 SAILR 还没完成。

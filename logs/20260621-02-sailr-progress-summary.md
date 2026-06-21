# SAILR structuring 当前进展总结

可以先读 `logs/20260621-01-sailr-overlay-manager-plan.md` 前 73 行了解之前的规划。73 行之后都是进度记录。

## 当前状态

当前 goal 还没完成。已经完成的是 Angr 风格 structuring 的公共基础层、copied / virtual block 的最小 shared 表示、`CrossJumpReverter`、`DuplicationReverter` 的 exact-match shared 子集、`SwitchReusedEntryRewriter` 的 shared reused-entry 子集、`SwitchDefaultCaseDuplicator` 的 shared default reuse 子集，以及 `ReturnDuplicatorLow` 的线性 return-tail、连通前驱组件共享复制和 terminal fork shared 子集；还没完成的是完整 SAILR deoptimization 算法。

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

## 还差什么

还差真正的完整 Angr SAILR deoptimization pass：

- `LoweredSwitchSimplifier`
- 相关 switch / duplication 辅助逻辑

这些 pass 还没有完整实现。`CrossJumpReverter`、`DuplicationReverter`、`SwitchReusedEntryRewriter` 和 `SwitchDefaultCaseDuplicator` 已经有最小 shared 版，`ReturnDuplicatorLow` 已经有单 block / 线性 return-tail 复制子集。

之前的主要卡点是：Angr 这些 pass 基本都会复制或新建 block。当前已经有最小 shared 表示，但具体 pass 还没有实现。

已处理 / 未处理例子：

- `CrossJumpReverter` 复制 goto target：已实现最小 shared 版
- `SwitchReusedEntryRewriter` 复制被多个 switch 复用的 entry block：已实现 shared reused-entry 子集
- `ReturnDuplicatorLow` 复制 return block：已实现单 block / 线性 return-tail、连通前驱组件共享复制和 terminal fork 子集，缺一般分支 return-region / Phi / vvar 刷新
- `DuplicationReverter`：已实现 exact-match shared 合并子集，缺 similarity search / merge graph
- `SwitchDefaultCaseDuplicator`：已实现 shared default reuse 子集，缺 if-chain / lowering 回写
- `LoweredSwitchSimplifier` 维护 block copies

当前 C renderer 的 label 身份已经改为 shared `BlockId`，body payload 通过 `BodyBlock` 走 shared CFG。后续 pass 仍要注意不要把新 block 的算法语义写到 C/Solidity renderer 里。

## 下一步建议

下一步可以继续补 `ReturnDuplicatorLow` 的分支 return-region / Phi 语义；如果这部分 shared payload 边界不清，再转向 `LoweredSwitchSimplifier` 里接口更窄的部分。

一句话总结：公共架构、Angr 风格执行框架、copied / virtual block 的 shared 表示、`CrossJumpReverter`、`DuplicationReverter` 的 exact-match 子集、`SwitchReusedEntryRewriter` 的 shared reused-entry 子集、`SwitchDefaultCaseDuplicator` 的 shared default reuse 子集和 `ReturnDuplicatorLow` 的线性 return-tail / connected predecessor / terminal fork 子集已经搭好；完整 SAILR 还没完成。

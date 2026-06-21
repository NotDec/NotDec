# 原始 prompt

直接启动这个goal开始开发吧

上文目标：在 `/sn640/NotDec2` 这个 worktree 上，基于最新 `v2` 分支继续推进 llvm2c 的 structuring 重构，严格按 Angr 的结构恢复框架落地。先补齐 `OverlayManager` / `RegionOverlay` / 可回滚共享图这套基础架构，让 structuring 的 region view、child finalize / dissolve、successor snapshot、checkpoint / rollback、node ownership 和 parent-visible graph 语义尽量与 Angr 一致；再在同一套基础上继续实现 Angr 的 Phoenix / SAILR 后续部分，包括递归 structuring 流程、cyclic refinement、virtual edge 选择、deoptimization / last-resort refinement、以及后续可扩展的结构恢复算法接入口。只保留 NotDec 后端渲染真正需要的最小差异，避免把 fallback 渲染当成算法语义本身。遇到以下情况就暂停 goal：1) 需要在 Angr 语义和当前 NotDec 输出质量之间做明确取舍；2) 现有计划没覆盖到的结构边界、回滚语义或算法接口问题；3) 无法在代码或 Angr 对照里确认的技术决策点。

# 背景

当前 llvm2c 已经有公共 structuring 骨架、Phoenix reducer、SAILR 的 edge ordering 和一层 `OverlayManager` / `RegionOverlay`。但这层 overlay 还不是 Angr 那种共享图：

- 当前 child region 结构化后，只记录 `StructuredRoot` 和 successor snapshot。
- parent region 每次从 `StructuredCFG + RegionTree + finalized child` 重新构造 `MutableRegionGraph`。
- Phoenix 的 cyclic refinement 失败时，NotDec 的 fallback renderer 还会利用 refinement 后的 partial graph 输出 loop；如果直接套 Angr 的 pre-refinement rollback，会让现有 smoke 输出退化。

这说明下一步不能只抄某个局部规则，要先把 overlay 的共享图、ownership 和 rollback 语义补起来。

# 目标

目标不是马上把所有 SAILR 优化一次性补完，而是先把 Angr 风格的基础架构落稳，再逐步接 Phoenix / SAILR：

- `OverlayManager` 拥有共享 region graph 状态，而不是只保存 finalized child 结果。
- `RegionOverlay` 能表达一个 region 在 parent 中的可见成员、child overlay、successor view 和 finalized/dissolved 状态。
- checkpoint / rollback 覆盖结构化过程中会改变的共享图、owner、members、finalized root 和 successor 信息。
- `MutableRegionGraph::build(Overlay)` 逐步从“重建图”变成“overlay view adapter”。
- 在这套基础上恢复 Angr 的 cyclic refinement checkpoint 语义，并继续实现 SAILR 的后续 deoptimization / last-resort refinement。

# 路线

第一阶段先只补数据模型，不改输出路径：

- 给 `OverlayManager` 增加共享图节点、边、block owner、region members 的状态。
- 初始化时从 `RegionTree` 和 `StructuredCFG` 派生 root/child membership。
- 给测试暴露只读查询，确认 parent view 里 child region 是一个 member，child blocks 归 child overlay。
- checkpoint / rollback 先覆盖新增状态。

第二阶段让现有 builder 读取 overlay state：

- 保留 `MutableRegionGraph` 这个 reducer 接口。
- 让 `MutableRegionGraph::build(Cfg, Overlay)` 优先从 overlay view 取 member/successor，而不是从 `RegionTree` 手工拼。
- 保持现有 structuring smoke 不退化。

第三阶段迁 finalize / dissolve：

- `finalize()` 不只记录 root，而是在 parent view 中用 structured result 替换 child overlay。
- `dissolve()` 把 child members 归还 parent。
- successor snapshot 用来恢复 parent-visible successor edge。
- rollback 必须能恢复这些操作。

第四阶段再打开 Angr 风格失败路径：

- cyclic refinement 前的 checkpoint 在失败时 rollback。
- child result 为 none 时走 dissolve，而不是依赖 NotDec fallback renderer 消化 partial graph。
- 这一步如果导致输出质量和 Angr 语义冲突，暂停确认。

第五阶段继续 SAILR：

- 在共享 overlay 基础上补 SAILR 的 deoptimization 接入口。
- 逐步对照 Angr 的 `sailr.py` 和相关 optimization pass，把能明确迁移的策略接到 Phoenix/SAILR 边界上。
- 保留 registry 里后续算法接入口，不伪实现未迁移算法。

# 风险

- 最大风险是 successor view：loop head backedge、break follow、external successor、goto target 都容易多边或少边。
- rollback 风险也很高：只恢复边不恢复 ownership，会产生 parent/child view 不一致。
- 短期可能出现“更贴近 Angr 但 NotDec 输出更差”的情况，这种情况必须暂停确认。
- 不应在共享 overlay 未落地前继续堆 fallback renderer 特例。

# 判断标准

- 现有 structuring 单测和 smoke 必须继续通过。
- 每一阶段都要有小测试覆盖具体语义，比如 owner、members、finalize、dissolve、rollback。
- 代码改动要能解释为 Angr 对齐或 NotDec 渲染必要差异。
- 涉及 structuring 输出变化时，用当前 structuring smoke 和至少一个 NotDec C 后端样例验证。

# 2026-06-21 实现记录：OverlayManager 初始成员和 owner 状态

本轮完成第一阶段的第一小步：`OverlayManager` 开始保存 Angr 风格 overlay view 的基础状态，但还没有让 Phoenix 直接使用它改图。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:27`
  新增 `OverlayMemberKind` 和 `OverlayMember`，表达一个 region view 里的 block、child region 或 structured result member。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:65`
  `OverlayManager` 增加 `parentOf()`、`ownerOf()`、`members()` 只读查询。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:79`
  checkpoint 状态增加 `Members` 和 `BlockOwners`，为后续 finalize / dissolve 回滚准备。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:9`
  实现 `OverlayMember::block()` / `region()` / `structured()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:37`
  新增 `OverlayManager::initializeOverlayState()`：child region 在 parent view 中作为一个 region member，child blocks 归 child overlay，非 child blocks 归当前 region。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:95`
  实现 parent、block owner 和 member 查询。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:159`
  checkpoint / rollback 覆盖新增的 member 和 owner 状态。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:402`
  新增 `testOverlayManagerInitialMembersMatchRegionTree()`，验证 parent、owner、root members、child members。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1113`
  将新测试接入 structuring analysis test。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这一步只增加状态和测试，没有改变 reducer 输入和 C 输出。
- 它把后续 shared overlay graph 的 owner / member 边界先固定住，避免后面直接改 finalize/dissolve 时没有可验证的基础。
- 实现效果：2/10。只是基础状态，还不是完整共享图。
- 复杂度：2/10。新增一个 member 表和 owner 表。
- 维护成本：2/10。后续 finalize/dissolve 会继续使用这份状态。

# 2026-06-21 实现记录：finalize / dissolve 维护 overlay member

本轮让 `finalize()` / `dissolve()` 开始维护上一轮新增的 member / owner 状态。现有 reducer 还没有消费这份状态，所以输出行为不变。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:79`
  允许 `RegionOverlay` 调用 `OverlayManager` 的 lifecycle helper。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:86`
  checkpoint 增加 `ParentRegions`，rollback 时能恢复 dissolve 造成的 child reparent。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:90`
  声明 `finalizeRegionMembers()` / `dissolveRegionMembers()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:111`
  `finalizeRegionMembers()` 把 parent view 里的 child region member 替换成 structured member。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:126`
  `dissolveRegionMembers()` 把 child members 插回 parent view，并更新 block owner / nested child parent。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:203`
  checkpoint / rollback 覆盖 `ParentRegions`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:231`
  `setStructuredRoot()` 调用 member finalize。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:319`
  `RegionOverlay::dissolve()` 调用 member dissolve。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:441`
  新增 `testOverlayManagerFinalizeAndDissolveUpdateMembers()`，验证 finalize、dissolve 和 rollback 后的 member / owner 状态。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1160`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这是 Angr lifecycle 语义的基础落地：child finalize / dissolve 已经开始改变 parent view。
- 还没有恢复 Angr 的 cyclic refinement rollback，因为 `MutableRegionGraph::build()` 仍主要看旧 `RegionTree + finalizedChildren()`。
- 实现效果：3/10。生命周期状态开始可回滚，但还不是共享 graph。
- 复杂度：2/10。只操作 member/owner，不碰 reducer。
- 维护成本：2/10。下一步要让 graph builder 消费这份 view。

# 2026-06-21 实现记录：MutableRegionGraph 读取 overlay member

本轮让 `MutableRegionGraph::build(Cfg, Overlay)` 开始读取 `OverlayManager::members()`。为了不改变现有输出，未 finalized 的 child 仍按普通 block 展开；finalized child 通过 structured member 建成 grouped node。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:44`
  `OverlayMember::structured()` 增加 source region id，记录这个 structured member 替换的是哪个 child region。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:23`
  structured member 保存 source region id。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:446`
  `MutableRegionGraph::build(Cfg, Overlay)` 改为先遍历 overlay members。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:448`
  block member 直接建普通 reducer node。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:455`
  structured member 使用 source child region 的 blocks/head 和 structured root 建 grouped node。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:472`
  继续从 finalized child snapshot 恢复 parent-visible successor。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:499`
  保留旧兼容行为：overlay members 没覆盖的 blocks 继续展开，避免未 finalized child 改变当前输出。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:557`
  新增 `testOverlayGraphUsesStructuredMemberSourceRegion()`，验证 parent graph 从 structured member 找回 source child region 的 blocks 和 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1163`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- graph builder 已经开始消费 overlay member，但仍保留未 finalized child 展开逻辑。
- 这是向 Angr parent overlay view 靠近的一步，不是完整 shared graph。
- 实现效果：4/10。parent view 的 finalized child 不再只靠旧 finalizedChildren 拼图。
- 复杂度：3/10。build 路径多了一层 member 解释。
- 维护成本：3/10。下一步应继续把 successor view 和未 finalized child view 从 fallback 逻辑里剥离出来。

# 2026-06-21 实现记录：保存共享 CFG successor

本轮给 `OverlayManager` 增加只读共享 CFG successor 状态。它还没有替代 `StructuredCFG` 输入，但后续 overlay view 的 successor 查询可以从这里开始迁移。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:56`
  新增 `OverlayManager(RegionTree, const StructuredCFG &)` 构造函数。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:70`
  新增 `sharedSuccessors(BlockId)` 查询。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:90`
  checkpoint 保存 `SharedSuccessors`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:38`
  新构造函数初始化 shared CFG successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:75`
  `initializeSharedGraph()` 从 `StructuredCFG` 保存 block successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:124`
  实现 `sharedSuccessors()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:222`
  checkpoint / rollback 覆盖 shared successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:337`
  `identifyOverlay()` 改为把 `StructuredCFG` 传入 `OverlayManager`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:490`
  新增 `testOverlayManagerKeepsSharedCFGSuccessors()`，验证共享 successor 查询。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1161`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这是 shared graph 的只读起点，暂不改变 reducer。
- 还没有实现 Angr 的 quotient view / successor view；只是把原始边放进 manager。
- 实现效果：2/10。基础状态。
- 复杂度：1/10。保存一份 successor 表。
- 维护成本：1/10。

# 2026-06-21 实现记录：只读 visible successor 查询

本轮增加 overlay 的只读 successor view 查询。它对照 Angr 的 `successor_nodes()` 思路，从共享 CFG successor 和当前 overlay members 推导 region 外部 successor。当前还没有替换 `snapshotSuccessors()`，避免改变 structuring 输出。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:71`
  新增 `OverlayManager::visibleSuccessors(RegionId)`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:11`
  新增本地 `appendUniqueBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:140`
  新增 `memberForBlock()`，在一个 region view 中找到 block 所属 member。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:160`
  实现 `visibleSuccessors()`：member 内部边不算 successor，指向 region view 外部 block 的边才算 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:503`
  新增 `testOverlayManagerDerivesVisibleSuccessors()`，验证 root 无外部 successor，loop child 的 successor 是 follow block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1162`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这是 successor view 的只读基础。
- 还不能直接替换 `RegionOverlay::snapshotSuccessors()`，因为现有 `Region.Successors` 和 derived view 在一些 fallback/loop case 上可能不完全一致，需要单独对比。
- 实现效果：3/10。
- 复杂度：2/10。
- 维护成本：2/10。

# 2026-06-21 实现记录：snapshotSuccessors 使用 visible successor

本轮把 `RegionOverlay::snapshotSuccessors()` 切到 derived visible successor。为了兼容手写 `RegionTree` 测试和没有 shared CFG 的调用，derived 结果为空时仍回退到旧 `Region.Successors`。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:383`
  `snapshotSuccessors()` 优先使用 `OverlayManager::visibleSuccessors()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:388`
  如果没有 shared CFG successor，回退到旧 `Region.Successors`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:518`
  新增 `testOverlayVisibleSuccessorsMatchIdentifiedLoopSuccessors()`，确认普通识别出的 loop successor 与 derived successor 一致。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:536`
  新增 `testSnapshotSuccessorsFallsBackWithoutSharedCFG()`，确认手写 `RegionTree` 仍使用旧 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1163`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- snapshot 已经开始来自 overlay view，贴近 Angr。
- fallback 只服务于旧构造路径，不是算法新语义。
- 实现效果：4/10。
- 复杂度：2/10。
- 维护成本：2/10。

# 2026-06-21 实现记录：新增 overlay quotient edge 查询

本轮增加只读 `quotientEdges()`，对齐 Angr `RegionOverlay._quotient_edges()` 的基础语义：在一个 overlay view 里，把 child region 折成一个 member，隐藏 child 内部边；需要 full view 时再返回 member 到外部 successor block 的边。当前还没有让 `MutableRegionGraph` 或 reducer 消费这个查询。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:48`
  新增 `OverlayViewEdge`，表示 member -> member 或 member -> external successor。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:84`
  新增 `OverlayManager::quotientEdges(RegionId, bool)`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:17`
  新增 `sameMember()` / `sameEdge()` / `appendUniqueEdge()`，用于边去重。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:221`
  实现 `quotientEdges()`，从 shared successor 和 overlay members 推导 view edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:560`
  新增 `testOverlayManagerDerivesQuotientEdges()`，验证父视图隐藏 child 内部边，child full view 暴露外部 follow。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:618`
  新增 `testOverlayManagerQuotientKeepsBlockSelfLoop()`，验证 block 自环不会被误删。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1372`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这是 parent-visible graph 的只读边视图，后续可以让 `MutableRegionGraph::build()` 从这里取边。
- 还没有实现 Angr cyclic full view 的 successor-to-successor edge，因为 NotDec 现在还没有完整 hidden edge / loop context 状态。
- 实现效果：5/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：MutableRegionGraph 使用 overlay quotient view

本轮把 `MutableRegionGraph::build(Cfg, RegionOverlay)` 的普通边生成切到 `OverlayManager::quotientEdges()`。同时把 parent view 里的未 finalized child region 也建成 grouped node，不再在父图里展开 child 内部 CFG 边。这一步更贴近 Angr：父 region 看到的是 child overlay/member，不直接看到 child 内部节点。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:455`
  `MutableRegionGraph::build()` 开始处理 `OverlayMemberKind::Region` 和 `OverlayMemberKind::Structured` 两类 grouped member。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:465`
  grouped child node 使用 child head 作为代表 block；finalized child 继续保留 `StructuredRoot`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:509`
  普通边生成改为遍历 `quotientEdges(..., true)`，member-to-member 生成 region-local edge，member-to-external successor 生成 placeholder edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:649`
  新增 `testOverlayGraphBuildsEdgesFromQuotientView()`，验证父图把 child region 当成一个节点，并隐藏 child 内部回边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1406`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- parent-visible graph 已经开始消费 overlay view，而不是自己扫 CFG。
- 这一步改变了父图里未 finalized child 的表现，但现有 structuring smoke 没有退化。
- 还缺 Angr 的 hidden edge、successor-to-successor edge、以及真正共享图 mutate/deopt。
- 实现效果：6/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：full quotient view 补 successor edge

本轮补 Angr full view 里的 successor-to-successor edge。NotDec 当前只有 `Root` / `NaturalLoop` 两种 region kind，没有 Angr `_in_loop` 的完整来源，所以这里按可确认语义实现：当前 region 是 `NaturalLoop`，或它有 `NaturalLoop` 祖先时，full quotient view 会补外部 successor block 之间的边。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:48`
  扩展 `OverlayViewEdge`，允许 source 是 external successor block。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:246`
  更新 member-to-member edge 初始化，适配新字段。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:258`
  在 `OverlayManager::quotientEdges()` 中判断当前 view 是否处在 loop context。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:272`
  为 full loop view 追加 successor-to-successor edge。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:514`
  `MutableRegionGraph::build()` 支持 external successor 作为 edge source，并创建 placeholder source node。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:688`
  新增 `testOverlayFullViewAddsLoopSuccessorEdges()`，验证 quotient view 和 mutable graph 都保留 successor-to-successor edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1441`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- full quotient view 的基础边语义更接近 Angr。
- loop context 目前用 `NaturalLoop` / 祖先 `NaturalLoop` 表示，后续如果引入更多 region kind，需要再对齐 Angr `_in_loop`。
- hidden edge 和真正 shared graph mutation 仍未实现。
- 实现效果：6/10。
- 复杂度：5/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：递归 structuring 从 overlay members 找 child

本轮把 recursive structuring 的 child 查找从静态 `Region.Children` 改成当前 overlay member view。这样更贴近 Angr：child `dissolve()` 或 `finalize()` 后，parent 后续看到的是当前 overlay 图里的 member，而不是旧 region tree 列表。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:22`
  `nextUnprocessedChild()` 改为遍历 `OverlayManager::members()` 中的 `OverlayMemberKind::Region`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:75`
  新增 `FailingRegionStructurer`，用于模拟 child structuring 失败。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:307`
  新增 `testRecursiveStructurerVisitsDissolvedChildMembers()`，验证 nested child 先处理，父 child 失败 dissolve 后，root 继续按当前 overlay member view 处理。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1489`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- recursive driver 更接近 Angr 的 “从当前 graph view 发现 child region”。
- 这一步没有引入新的 deoptimization 策略。
- 真正共享图 mutation / rollback 仍然是后续大块。
- 实现效果：6/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：补 view-only overlay mutation 状态

本轮补 Angr overlay mutation 里不需要立刻迁移 reducer 的部分：隐藏普通 view edge、只隐藏 full view edge、追加 full view extra edge。它们都是 view-only 状态，已经纳入 `checkpoint()` / `rollback()`，但还没有实现 Angr 的真实 shared graph node/edge mutation。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:61`
  新增 `OverlayEdgeEndpoint`，用于表示 member endpoint 或 external successor endpoint。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:106`
  新增 `hideEdge()`、`hideEdgeToSuccessor()`、`removeEdgeWithSuccessorsOnly()`、`addExtraFullEdge()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:128`
  checkpoint 状态加入 `HiddenEdges`、`HiddenFullEdges`、`ExtraFullEdges`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:218`
  新增 `underlyingBlocks()` 和 endpoint 到 view edge 的转换逻辑。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:291`
  `visibleSuccessors()` 开始过滤 hidden edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:323`
  `quotientEdges()` 开始过滤 hidden/full-only hidden edge，并追加 extra full edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:406`
  实现 view-only mutation API。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:785`
  新增 `testOverlayViewOnlyMutationsAffectQuotientEdges()`，验证 hidden/full-only/extra full edge 和 rollback。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1557`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- view-only mutation 已经有基础状态和 rollback。
- 还没有实现 Angr 的 `remove_node` / `replace_nodes` / `detach_edge` 这类真实 shared graph mutation。
- 这一步不改变 Phoenix reducer 路径，只是让后续迁移有可落地的 overlay API。
- 实现效果：6/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：RegionOverlay 暴露 view-only mutation 方法

本轮给 `RegionOverlay` 增加 view-only mutation 薄封装，让调用边界更接近 Angr：后续 reducer 迁移时可以直接对当前 overlay 调 `hideEdgeToSuccessor()` / `removeEdgeWithSuccessorsOnly()` / `addExtraFullEdge()`，而不是绕过 overlay 去调 manager。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:181`
  `RegionOverlay` 新增 `hideEdge()`、`hideEdgeToSuccessor()`、`removeEdgeWithSuccessorsOnly()`、`addExtraFullEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:652`
  实现这些方法，当前都是转发到 `OverlayManager`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:799`
  `testOverlayViewOnlyMutationsAffectQuotientEdges()` 改为通过 `RegionOverlay` 调用 mutation API。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 这是 API 边界对齐，不切换 reducer 行为。
- 将 `MutableRegionGraph::virtualizeEdge()` 同步回 overlay hidden edge 会改变 parent 后续看到的图，属于后续需要单独评估的行为切换。
- 实现效果：6/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：补 shared edge mutation API

本轮补真实 shared edge mutation 的最小子集：`addEdge()` 和 `detachEdge()`。它们直接修改 `SharedSuccessors`，因此会影响 `visibleSuccessors()` / `quotientEdges()` 派生出的 region view；同时 `checkpoint()` / `rollback()` 已经覆盖 `SharedSuccessors`，所以这部分 mutation 可回滚。当前仍没有把 Phoenix reducer 的 `virtualizeEdge()` 自动同步到 overlay。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:101`
  `OverlayManager` 新增 `addEdge(BlockId, BlockId)` 和 `detachEdge(BlockId, BlockId)`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:184`
  `RegionOverlay` 新增同名转发方法。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:291`
  新增 `clearHiddenEdge()`，真实 edge 重新加入或删除时清掉对应 hidden edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:417`
  实现 `OverlayManager::addEdge()` / `detachEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:678`
  实现 `RegionOverlay::addEdge()` / `detachEdge()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:838`
  新增 `testOverlaySharedEdgeMutationsUpdateViews()`，验证 add/detach 会更新 view，且 rollback 恢复。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1595`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- shared edge mutation 基础 API 已经存在。
- 这一步仍不改变 reducer 行为。
- 后续把 `MutableRegionGraph` 的虚拟化/折叠同步到 overlay，会改变 parent-visible graph，需要单独设计和验证。
- 实现效果：7/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：补 block member add/remove

本轮补 node mutation 的最小明确子集：block member 的 add/remove。它对齐 Angr `add_node()` / `_on_node_removed()` 的基础效果：更新当前 overlay member、owner 和 shared successor 状态，并纳入已有 checkpoint/rollback。当前只支持 block id，不实现 `replace_nodes()`、`collapse_to()` 或 structured result node 的 shared graph 表示。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:101`
  `OverlayManager` 新增 `addBlockMember()` / `removeBlockMember()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:187`
  `RegionOverlay` 新增同名转发方法。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:40`
  新增 block/member 和 edge 是否引用 block 的判断 helper。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:313`
  新增 `clearEdgeStateForBlock()`，删除 block 时清理 shared successors、hidden edge、full-only hidden edge、extra full edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:461`
  实现 `OverlayManager::addBlockMember()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:474`
  实现 `OverlayManager::removeBlockMember()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:750`
  实现 `RegionOverlay::addBlockMember()` / `removeBlockMember()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:873`
  新增 `testOverlayBlockMemberMutationsUpdateOwnersAndViews()`，验证 member/owner/view 更新和 rollback。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1632`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- block-level node add/remove 基础 API 已经存在。
- 还没有解决 structured result node 如何进入 shared graph；这是实现 Angr `replace_nodes()` / `collapse_to()` 前必须明确的表示问题。
- 实现效果：7/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：集中 overlay member representative 规则

本轮没有改变 reducer 行为，只把 overlay member 到 representative block 的规则集中到 `OverlayManager`。当前仍沿用现有表示：block member 代表自身，region/structured member 代表 source region head。这样后续如果引入真正的 shared graph node key，改动点会集中在 overlay 层。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:97`
  新增 `OverlayManager::representativeBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:203`
  实现 `representativeBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:467`
  grouped child node 使用 `representativeBlock()` 创建。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:515`
  quotient edge source member 使用 `representativeBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:537`
  quotient edge target member 使用 `representativeBlock()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1002`
  在 structured child 测试中直接确认 representative block 是 child head。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 代表节点规则已集中，后续替换成真正 shared node key 更可控。
- 这一步仍没有实现 Angr `replace_nodes()` / `collapse_to()`。
- 实现效果：7/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：引入 OverlayNodeKey

用户明确要求严格按 Angr 方式实现，不为了迁移旧版结构恢复保守折中。本轮先补 shared graph node 身份的基础表示：`OverlayNodeKey`。它能表示 block、child region overlay、structured result 三类 view node。当前还没有把 `SharedSuccessors` 从 `BlockId` map 迁到 `OverlayNodeKey` map，只先把 member 的身份查询落下来，作为后续实现 `replace_nodes()` / `collapse_to()` 的入口。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:33`
  新增 `OverlayNodeKind`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:43`
  新增 `OverlayNodeKey`，表示 block / region / structured 三类 overlay view node。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:123`
  新增 `OverlayManager::nodeKey()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:76`
  实现 `OverlayNodeKey` 的工厂方法。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:225`
  实现 `OverlayManager::nodeKey()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1002`
  在 finalized child 测试中验证 structured member 的 node key。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- shared graph node 的方向已经明确：后续按 Angr 走，structured result 会成为 overlay graph node，而不是只作为旧 renderer 附属信息。
- 下一步应逐步把 shared successor / hidden / extra full edge 从 `BlockId` 迁到 `OverlayNodeKey`，再实现 `replace_nodes()` / `collapse_to()`。
- 实现效果：7/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：shared successor 增加 OverlayNodeKey 存储

本轮开始把 shared graph 从纯 `BlockId` successor 图迁到 `OverlayNodeKey`。新增 `SharedNodeSuccessors` 作为真实方向，保留旧 `SharedSuccessors` 作为 block-only 兼容视图。这样 structured result node 已经可以拥有 outgoing edge，后续可以继续把 quotient view、hidden edge 和 reducer mutation 迁到 node-key 语义。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:57`
  为 `OverlayNodeKey` 增加比较和相等操作，允许作为 map key。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:127`
  新增 `sharedNodeSuccessors()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:165`
  checkpoint 状态新增 `SharedNodeSuccessors`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:99`
  实现 `OverlayNodeKey` 比较和相等。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:188`
  初始化 shared graph 时同时填充 node-key successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:269`
  实现 `sharedNodeSuccessors()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:579`
  实现 `addNodeEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:591`
  实现 `detachNodeEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:745`
  checkpoint/rollback 覆盖 `SharedNodeSuccessors`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:873`
  新增 `testOverlaySharedNodeSuccessorsCanTargetStructuredResults()`，验证 structured node 可以有 outgoing edge，且 rollback 生效。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1666`
  将新测试接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- shared graph 已经开始支持 structured result node。
- 旧 block-only API 仍保留，避免一次性改完 renderer 和现有 reducer。
- 下一步应让 `quotientEdges()` 从 `SharedNodeSuccessors` 派生，而不是从 block-only `SharedSuccessors` 派生。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：quotientEdges 使用 node-key successor

本轮让 `OverlayManager::quotientEdges()` 对 block/structured member 使用 `SharedNodeSuccessors` 派生 view edge。未 finalized child region member 暂时仍展开 underlying block，因为它在 Angr view 中还是 child `RegionOverlay`，不是 structured result node。这一步开始让 structured result 的 shared graph edge 进入 parent-visible view。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:471`
  `quotientEdges()` 新增 node-key successor 到 `OverlayViewEdge` 的转换逻辑。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:508`
  block/structured member 改为遍历 `sharedNodeSuccessors(nodeKey(Member))`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:520`
  未 finalized region member 保留 underlying block 展开路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1036`
  在 finalized child 测试中添加 structured node -> block edge，并验证 `quotientEdges()` 能看到这条 edge。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- structured result node 已经能通过 node-key shared successor 影响 parent-visible quotient view。
- `visibleSuccessors()` 和 loop successor-to-successor 仍是 block-only 兼容路径，后续需要继续迁移。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：visibleSuccessors 使用 node-key successor

本轮继续把 parent-visible successor 从 block-only 图迁到 node-key shared graph。`visibleSuccessors()` 现在对 block/structured member 直接读取 `SharedNodeSuccessors`，所以 finalized child 产生的 structured result node 也能贡献外部 successor。未 finalized region member 仍保留 block 展开路径，因为它还不是 structured result node。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:435`
  `OverlayManager::visibleSuccessors()` 对 block/structured member 改为遍历 `sharedNodeSuccessors(nodeKey(Member))`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:459`
  未 finalized region member 继续展开 region 内 block，并沿用现有 block successor 兼容路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1036`
  扩展 finalized child 测试，验证 structured node 指向 parent 内部 block 时不会暴露为外部 successor，指向 parent 外部 block 时会暴露出来。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- parent fallback / loop 后续逻辑已经能看到 structured result node 的外部 block successor。
- 返回类型仍是 `BlockId`，这是现有调用方的兼容限制；后面需要补 node-key successor API，再迁移 reducer 和 fallback renderer。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：hidden edge 改为 node-key 身份

本轮把 view-only hidden edge 从 `BlockId -> BlockId` 改成 `OverlayNodeKey -> OverlayNodeKey`。这样 finalized child 变成 structured result node 后，它的 outgoing edge 也能被当前 overlay view 隐藏。旧的 `hideEdge(BlockId, BlockId)` 仍保留为包装接口，现有 Phoenix 路径不用同步大改。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:98`
  `OverlayHiddenEdge` 改为保存 `OverlayNodeKey From/To`，并补充注释说明这是 Angr-style view edge 身份。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:142`
  新增 `OverlayManager::hideNodeEdge()`，作为 node-key hidden edge 的入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:346`
  `isHiddenEdge()` 改为比较 node-key。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:385`
  `clearHiddenEdge()` 改为按 node-key 清理。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:440`
  `visibleSuccessors()` 对 block/structured member 都用 node-key hidden edge 过滤。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:488`
  `quotientEdges()` 对 block/structured member 都用 node-key hidden edge 过滤。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:644`
  `addNodeEdge()` / `detachNodeEdge()` 统一清掉同一条 node-key hidden edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:676`
  实现 `hideNodeEdge()`，`hideEdge()` 保留为 block-only 包装。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:685`
  `hideEdgeToSuccessor()` 对 structured member 走 node-key successor，对未 finalized region member 仍展开 underlying block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1040`
  扩展 finalized child 测试，验证 structured node 的外部 successor 可以隐藏，重新添加同一条 node edge 后恢复可见。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- hidden edge 已经能覆盖 structured result node，和 Angr 的 overlay view 语义更接近。
- `removeEdgeWithSuccessorsOnly()` / `addExtraFullEdge()` 仍基于 `OverlayViewEdge`，下一步需要继续处理 external endpoint 的 node-key 化。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：新增 node-key visible successor

本轮新增 `visibleNodeSuccessors()`，对应 Angr `RegionOverlay.successor_nodes()` 的方向：先返回离开当前 region view 的 overlay node，再由旧 `visibleSuccessors()` 过滤出 block-only 兼容结果。这样后续 SAILR / deoptimization 可以逐步使用 node-key successor，不再被 `BlockId` 接口限制。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:133`
  新增 `OverlayManager::visibleNodeSuccessors()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:180`
  新增 `memberForNodeKey()`，用于判断 successor 是否已经在当前 view 内。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:18`
  新增 `appendUniqueNodeKey()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:303`
  实现 `memberForNodeKey()`，block key 复用现有 block-to-member 判断，非 block key 按 overlay node 身份匹配。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:463`
  实现 `visibleNodeSuccessors()`，对 block/structured member 走 node-key shared successor，对未 finalized region member 仍展开 underlying block。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:504`
  `visibleSuccessors()` 改为 `visibleNodeSuccessors()` 的 block-only 兼容视图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1044`
  扩展 finalized child 测试，验证 node-key successor 能保留外部 structured successor，同时旧 `visibleSuccessors()` 仍只返回 block。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- successor 语义已经有 Angr-style node-key API。
- 旧 `SuccessorSnapshot` 仍是 block-only，这是 renderer 兼容点；后续 child finalize / fallback renderer 要继续迁。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：SuccessorSnapshot 保留 node-key successor

本轮把 child finalize 的 successor snapshot 改成双轨：`Successors` 继续保留 block-only 结果给当前 `MutableRegionGraph` / renderer，新增 `NodeSuccessors` 保留 Angr-style overlay node successor。这样递归 structuring 边界不会再把非 block successor 丢掉。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:50`
  `SuccessorSnapshot` 新增 `NodeSuccessors`，并说明 block-only 字段只是当前 renderer 兼容。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:881`
  `OverlayManager::setStructuredRoot()` 存储 snapshot 前做双向补齐：旧 block-only snapshot 自动补 node-key，node-key snapshot 自动补 block-only block successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:968`
  `RegionOverlay::snapshotSuccessors()` 同时记录 `visibleNodeSuccessors()` 和旧 `visibleSuccessors()`；没有 manager-derived successor 时按 region 静态 successor 补 block key。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:386`
  空 successor snapshot 同时验证 block-only 和 node-key 字段为空。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:432`
  finalized child 测试验证 snapshot 和 finalized child record 都保留 node-key block successor。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- child finalize 已经不会丢 node-key successor。
- `MutableRegionGraph::build()` 仍只消费 `Successors`，这是下一层 renderer 兼容点。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：递归 structuring 默认使用 shared CFG graph

本轮修正 `RecursiveStructurer::structure(Cfg, Regions)` 的默认 overlay manager 构造方式。之前它只传 `RegionTree`，不会初始化 shared successor graph；这和 Angr 的 OverlayManager 作为共享图所有者不一致。现在默认入口也用 `OverlayManager(Regions, Cfg)`。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:82`
  `RecursiveStructurer::structure(Cfg, Regions, Structurer)` 改为构造带 CFG 的 `OverlayManager`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 默认递归 structuring 已经不再绕过 shared graph。
- 这会让后续 Phoenix / SAILR reducer mutation 更接近 Angr 的 overlay 流程。
- 实现效果：7/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：full-view 外部端点保留 node-key 身份

本轮把 `OverlayViewEdge` / `OverlayEdgeEndpoint` 的外部端点扩成双轨：block 外部端点继续填旧 `BlockId` 字段，非 block 外部端点用 `OverlayNodeKey` 保存。这样 full-view extra / hidden edge 已经能表达 structured result node 作为 successor/source 的情况，后续 SAILR 不需要把这些边降级成 block-only。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:83`
  `OverlayViewEdge` 增加 `HasExternalSourceNode` / `ExternalSourceNode` / `HasExternalSuccessorNode` / `ExternalSuccessorNode`，并新增 `sourceNode()` / `targetNode()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:103`
  `OverlayEdgeEndpoint` 增加 node-key external endpoint，并保留 `external(BlockId)` 兼容入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:41`
  `sameEdge()` 和 `edgeReferencesBlock()` 纳入 node-key 外部端点。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:148`
  实现 `OverlayViewEdge::sourceNode()` / `targetNode()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:376`
  `memberForEndpoint()` 对非 block external endpoint 不再误当 block 查找。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:391`
  `viewEdgeForEndpoints()` 能从 node-key external endpoint 构造 full-view edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:580`
  `quotientEdges()` 遇到不在当前 view 内的非 block successor 时，保留为 external node-key successor。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:510`
  当前 renderer-facing graph 只消费 block external endpoint，非 block external endpoint 暂时跳过。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:838`
  扩展 view-only mutation 测试，覆盖 structured node 作为 full-view external source/target，并验证 hidden-full 能隐藏这类边。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- full-view overlay edge 已经能保留 Angr-style node 身份。
- `MutableRegionGraph` 仍是 block renderer 兼容层，后续要么改成 node-key graph builder，要么把 renderer fallback 明确限制在 block-only 输出层。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：finalize 按 snapshot 重连 shared graph

本轮把 Angr `RegionOverlay.finalize()` 的 successor 重连语义迁到 shared graph。child region finalize 后，structured result node 会根据 `SuccessorSnapshot::NodeSuccessors` 重新建立 outgoing edge；指向自身的边和指向父层 loop head 的 continue 边跳过。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:955`
  `OverlayManager::setStructuredRoot()` 在保存 snapshot、替换 parent member 后，构造 structured result node key。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:979`
  清理 result node self-loop。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:982`
  识别父层 natural loop head，作为 continue edge skip 条件。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:988`
  遍历 `NodeSuccessors`，把合法 successor 重连到 structured result node。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1030`
  finalized child snapshot 测试新增 shared node successor 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1073`
  structured member source-region 测试删除手工补边假设，改为验证 finalize 自动重连 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1144`
  parent loop 测试验证 snapshot 里的 parent loop head 不会被重连，普通 follow 会保留。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- child finalize 的 successor 重连已经进入 shared graph，不再只靠 `MutableRegionGraph::build()` 的 snapshot 兼容层。
- `collapse_to()` / 通用 replace-nodes 还没实现；这仍是后续 SAILR / deoptimization 的关键缺口。
- 实现效果：8/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：新增 collapseRegionTo 基础语义

本轮补了 Angr `RegionOverlay.collapse_to()` 的基础 shared graph 版本。它把当前 region 成员对应的 shared graph node 收缩成一个 structured result node，保留跨入 / 跨出边，删除 region 内部边，并在 parent view 里用 structured member 替换 child region。checkpoint 已覆盖相关状态，所以 rollback 能恢复。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:181`
  新增 `OverlayManager::collapseRegionTo()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:216`
  新增 `memberNodeKeys()`，用于把 block / structured / region member 转成 shared graph node key 集合。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:264`
  新增 `RegionOverlay::collapseTo()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:527`
  实现 `memberNodeKeys()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1025`
  实现 `collapseRegionTo()`：收集 region 内节点、记录跨入 / 跨出边、删除内部节点、插入 result node、重连边、更新 structured root 和 parent members。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1198`
  实现 `RegionOverlay::collapseTo()` 包装入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:944`
  新增 `testOverlayCollapseRegionRewiresSharedGraph()`，验证 collapse 后 parent member、shared successor、owner、内部边删除和 rollback 恢复。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1775`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- 已有可回滚的 shared graph collapse 基础能力。
- 还没做 Angr `replace_nodes()` 的局部 reducer 收缩，也没把 Phoenix/SAILR reducer 改成直接用这个入口。
- 实现效果：7/10。
- 复杂度：6/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：新增 replaceNodes 基础语义

本轮补了 Angr `RegionOverlay.replace_nodes()` 的基础 node-key 版本。它在当前 region view 内把一个或多个 direct member node 替换成 structured result node，重连 shared graph 的跨入 / 跨出边，按 Angr 的规则保留 self-loop，并把 hidden edge / hidden-full edge / extra-full edge 里的旧 node 引用重映射到新 node。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:76`
  `OverlayMember::structured()` 增加 optional representative block。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:184`
  新增 `OverlayManager::replaceNodes()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:221`
  新增 bookkeeping remap helper 声明。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:274`
  新增 `RegionOverlay::replaceNodes()` 包装入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:140`
  structured member 保存 representative block；`representativeBlock()` 优先使用它。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:553`
  实现 `remapViewEdgeEndpoint()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:609`
  实现 `remapBookkeeping()`，覆盖 hidden / hidden-full / extra-full。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1195`
  实现 `replaceNodes()`：收集 crossing edges、删除旧 node、插入 structured result、重连边、更新 current view members、清理 block owner、重映射 bookkeeping。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1418`
  实现 `RegionOverlay::replaceNodes()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1000`
  新增 `testOverlayReplaceNodesRewiresSharedGraphAndBookkeeping()`，验证 shared graph 重连、representative block、hidden edge remap、extra-full edge remap 和 rollback。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1874`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- shared graph 已有 `collapse_to()` 和 `replace_nodes()` 的基础能力。
- Phoenix / SAILR 还在 `MutableRegionGraph` 上 reduce，下一步应开始把 reducer 的 collapse 调用迁到 overlay replace/collapse，或先做一层 adapter。
- 实现效果：7/10。
- 复杂度：7/10。
- 维护成本：6/10。

# 2026-06-21 实现记录：MutableRegionGraph 记录 overlay source nodes

本轮给 `MutableRegionGraph` 增加 overlay source node 记录。Phoenix reducer 目前还在 `MutableRegionGraph` 上执行；要把每次 `collapseNodes()` 映射到 Angr-style `Overlay.replaceNodes()`，必须先知道 graph node 来自哪些 overlay node。这一步只补数据通道，不改变 reducer 行为。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:44`
  `MutableRegionNode` 新增 `SourceNodes`，记录对应的 `OverlayNodeKey`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:143`
  新增 `appendUniqueNodeKey()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:378`
  `addNode()` 对普通 block node 初始化 block source key。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:482`
  overlay graph 构建时，child region / structured member 的 graph node source 记录为当前 overlay member 的 node key，而不是拆成内部 block。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:743`
  `collapseNodes()` 合并成员的 `SourceNodes`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:796`
  collapsed node 保存合并后的 `SourceNodes`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:171`
  checkpoint 测试中验证 collapse 后 source node 合并。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1253`
  finalized child overlay graph 测试验证 parent graph node 保留 structured member source key。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- reducer graph 已经能追踪 overlay node 来源。
- 下一步可以在 `PhoenixStructurer::structureRegion(RegionOverlay&)` 中用这些 source keys，把 reducer 的 collapse 结果同步回 overlay shared graph。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：Phoenix overlay path 同步最终 reducer collapse

本轮把 Phoenix overlay path 的成功收敛结果同步回 overlay shared graph。当 `MutableRegionGraph` 最终只剩一个带 `StructuredRoot` 的 active node 时，使用该 node 的 `SourceNodes` 调 `RegionOverlay::replaceNodes()`。这样 Phoenix reducer 虽然仍运行在 `MutableRegionGraph` 上，但成功 reduce 的最终结果已经会写回 Angr-style overlay graph。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2704`
  `PhoenixStructurer::structureRegion(Cfg, RegionOverlay&, Tree)` 在单 active structured node 路径中，调用 `Overlay.replaceNodes(Node->SourceNodes, RootId)`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1079`
  新增 `testPhoenixOverlayPathSyncsReducerCollapseToOverlay()`，验证简单 sequence reduce 后 overlay members 变成 structured member，shared graph 中原始 block edge 被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1914`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- Phoenix overlay path 的最终成功 reduce 已经会同步回 overlay shared graph。
- 还没有逐个 reducer 调用点同步，也没有同步 last-resort virtual edge 的 overlay mutation；这仍是 SAILR/deoptimization 继续推进前的缺口。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：Phoenix overlay path 同步 virtualized edge 标记

本轮在 Phoenix overlay path 的成功收敛路径里，同步 `MutableRegionGraph::virtualEdges()` 到 overlay view-only mutation。当前 `VirtualEdge` 只保存 block endpoint，所以这一步只同步 `FromBlock -> ToBlock` 能确认的边，写入 `removeEdgeWithSuccessorsOnly()`；随后最终 `Overlay.replaceNodes()` 会把这些 hidden-full edge remap 到 structured result node。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1111`
  新增 `syncVirtualEdgesToOverlay()`，把 block endpoint virtual edge 同步为 overlay hidden-full edge。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2721`
  在 overlay path 的单 active structured node 成功路径中，先同步 virtualized edge，再执行 `Overlay.replaceNodes()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1079`
  复用 Phoenix overlay path 同步测试，覆盖最终 reduce 后 overlay shared graph 的结果。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1914`
  该测试已接入 main。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。

当前判断：

- overlay path 已经能同步最终 reducer result 和 block-endpoint virtualized edge 标记。
- `VirtualEdge` 仍是 block-only，后续如果 SAILR 需要 node-key virtual edge，必须继续扩展 `VirtualEdge` 或新增 overlay-side virtual edge record。
- 实现效果：6/10。
- 复杂度：4/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：Phoenix reducer 中间 collapse 同步 overlay shared graph

本轮继续按 Angr 的 `replace_nodes()` 思路，把 Phoenix overlay path 从“最终结果同步”推进到“每次 reducer collapse 都同步”。这样 reducer 运行在 `MutableRegionGraph` 上时，每次形成 structured node，overlay shared graph 也会立刻把旧 source nodes 替换成新 structured node。后续 reducer 再 collapse 时，source node 会指向上一次的 structured overlay node，而不是已经被删掉的原始 block。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:30`
  给 `analyzeAcyclic()`、`analyzeCyclic()`、`refineCyclic()` 增加可选 `RegionOverlay *Overlay` 参数，旧调用默认仍走 graph-only 路径。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1162`
  新增 `collapseNodesAndSyncOverlay()`，封装 `MutableRegionGraph::collapseNodes()` + `RegionOverlay::replaceNodes()`；同步前会先把 virtualized edge 写入 overlay，替换后把 collapsed graph node 的 `SourceNodes` 改成新 structured overlay node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2576`
  `analyzeAcyclic()` 把 overlay 参数传给 switch / sequence / if reducer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2591`
  `analyzeCyclic()` 把 overlay 参数传给 while / do-while / self-loop reducer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2608`
  `refineCyclic()` 增加 overlay checkpoint / rollback，避免 cyclic refinement 试探失败后留下 shared graph mutation。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2787`
  overlay path 调用 reducer 阶段时传入当前 `RegionOverlay`；最终路径保留 wrapping 后的兜底替换，但避免同一个 structured node 自替换。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1112`
  新增 `testPhoenixOverlayPathSyncsRepeatedReducerCollapses()`，覆盖 0->1->2 连续 sequence collapse，确认第二次 collapse 使用 structured source node。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1949`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-overlay-sync.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.56 user=38.94 sys=0.04 maxrss=217124`。当前主程序只接受 `--algo=phoenix`，不接受旧日志里的 `--algo=structured-phoenix`。

当前判断：

- Phoenix/SAILR 共用 reducer 已经更接近 Angr 的共享 graph 语义：overlay path 不再等最终结果才替换节点。
- `lastResortRefinement()` 仍只 mutate `MutableRegionGraph`，没有 overlay checkpoint；当前它只 virtualize edge，不 collapse node，风险比 reducer collapse 小，但后续如果扩展 deoptimization，需要一起接入 overlay mutation。
- `VirtualEdge` 仍是 block endpoint，node-key virtual edge 还没做。
- 实现效果：7/10。
- 复杂度：5/10。
- 维护成本：5/10。

# 2026-06-21 实现记录：last-resort virtualized edge 对齐 Angr detach 语义

对照 Angr 后修正一处语义偏差：Angr `_virtualize_edge()` 会调用 `RegionOverlay.detach_edge()`，也就是从 shared graph 真实移除虚拟化边；之前 NotDec 只把 virtualized edge 放进 with-successors hidden-full 视图，shared graph 里边还在。本轮改成 overlay path 在 last-resort 成功虚拟化边后同步 detach shared edge，并用 overlay checkpoint 包住这一步。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:43`
  给 `lastResortRefinement()` 增加可选 `RegionOverlay *Overlay` 参数，旧 graph-only 调用不变。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1122`
  `syncVirtualEdgesToOverlay()` 从 `removeEdgeWithSuccessorsOnly()` 改为 `OverlayManager::detachNodeEdge()`，和 Angr virtualized edge 的 shared graph detach 语义一致。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2642`
  `lastResortRefinement()` 在 overlay path 成功虚拟化边后同步 overlay，并在失败时 rollback overlay checkpoint。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2814`
  overlay path 调用 `lastResortRefinement()` 时传入当前 overlay。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:22`
  测试用 `HintStructurer` 暴露 `lastResortRefinement()`，便于固定虚拟化边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1147`
  新增 `testPhoenixOverlayLastResortDetachesVirtualizedEdge()`，验证 last-resort 虚拟化 `0 -> 1` 后 shared graph 不再有该边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1987`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-virtual-edge-detach.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.64 user=38.94 sys=0.06 maxrss=219952`。

当前判断：

- last-resort virtualized edge 已经按 Angr 语义改为 shared graph detach。
- `removeEdgeWithSuccessorsOnly()` 仍保留给 switch-case 这类 Angr 里本来就是 view-only 的路径。
- 还没做 node-key virtual edge；当前同步仍依赖 `VirtualEdge.FromBlock/ToBlock`。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：补齐 overlay absorb successor 基础语义

本轮补 Angr `RegionOverlay.absorb_successor_into()` 对应的 view-only 语义。这个接口用于把当前 region full view 里的 successor 吸收到一个 structured member：successor 的 full-view 出边改挂到新 structured node 上，原成员到 successor 的边在当前 view 隐藏，shared graph 不改。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:169`
  新增 `OverlayManager::hideEdgeToNodeSuccessor()`，让隐藏 successor edge 支持 node-key，不只支持 block successor。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:175`
  新增 `OverlayManager::absorbSuccessorInto()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:279`
  新增 `RegionOverlay::absorbSuccessorInto()` 包装入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:902`
  `hideEdgeToSuccessor()` 改为调用 node-key 版本；node-key 版本按 member source nodes 扫 shared successors 并写 hidden edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:940`
  实现 `absorbSuccessorInto()`：扫描 `quotientEdges(..., true)`，把 successor 的 out-edge 变成 `NewNode -> target` extra-full edge，然后隐藏当前 view 中成员到 successor 的边。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1428`
  实现 `RegionOverlay` 包装方法。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1080`
  新增 `testOverlayAbsorbSuccessorIntoStructuredMember()`，验证 successor 出边被搬到 structured member，原 structured member 到 successor 的 full-view 边被隐藏。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2039`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-absorb-successor.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.57 user=38.99 sys=0.07 maxrss=221292`。

当前判断：

- overlay 已有 Angr `absorb_successor_into()` 的基础能力，后续 Phoenix 某些 `replace_nodes_both(old_node_1=successor)` 场景可以接这个入口。
- 当前实现只负责 view-only graph 语义，不处理 AIL/AST 里的条件改写；这部分仍属于后续 Phoenix/SAILR reducer 接入工作。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：补齐 overlay edge marks

本轮补 Angr `RegionOverlay.mark_edge()` / `drop_edge_marks_from()` 对应的 edge marks。它用于 cyclic refinement 这类试探性流程：边仍留在 shared graph 里，但在当前 region view 里临时不可见；replace/collapse 后 mark 要跟着新 structured node 走，checkpoint rollback 要能恢复。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:171`
  新增 `OverlayManager::markNodeEdge()`、`markEdge()`、`dropEdgeMarksFrom()`，并在 `RegionOverlay` 上提供包装入口。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:211`
  把 `EdgeMarks` 加入 checkpoint state；`external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:256` 保存运行时 edge marks。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:463`
  新增 `isMarkedEdge()`；`external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:481` 新增 `isMarkedViewEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:558`
  删除 block member 时清理引用该 block 的 edge marks。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:682`
  `remapBookkeeping()` 在 `replaceNodes()` 后 remap edge marks。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:708`
  `visibleNodeSuccessors()` 过滤 marked edge；`external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:770` 起 `quotientEdges()` 过滤 marked full-view edge。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1001`
  实现 mark/drop 逻辑；`external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1163` 起 checkpoint/rollback 保存并恢复 `EdgeMarks`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1516`
  实现 `RegionOverlay::markEdge()` / `dropEdgeMarksFrom()` 包装。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1134`
  新增 `testOverlayEdgeMarksFilterAndRemap()`，覆盖 mark 过滤、drop 恢复、replaceNodes remap 和 rollback。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2131`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-edge-marks.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.43 user=38.72 sys=0.04 maxrss=220116`。

当前判断：

- overlay 已经具备 Angr edge marks 的基础语义，后续可以把 Phoenix cyclic refinement 的 `cyclic_refinement_outgoing` 接到这里。
- 当前还没有把 Phoenix `refineCyclic()` 里的 cyclic refinement outgoing mark 真正改成 overlay mark；这属于下一步 reducer 接入。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：Phoenix collapse 后清理 refinement marks

本轮补 Angr `replace_nodes_both(..., drop_refinement_marks=True)` 的对应语义。循环类 reducer collapse 后，如果旧节点上有 `cyclic_refinement_outgoing` mark，被 `replaceNodes()` remap 到新 structured node 后要清掉，避免 structured loop 到 follow 的边继续被隐藏。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:50`
  给 `collapseNodesAndSyncOverlay()` 增加 `DropRefinementMarks` 参数。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:819`
  `reduceLinearWhileOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:890`
  `reduceLinearWhileWithBreakOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:996`
  `reduceLinearDoWhileOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1046`
  `reduceSelfLoopOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1174`
  `collapseNodesAndSyncOverlay()` 在 overlay replace 后调用 `dropEdgeMarksFrom(structured_node, "cyclic_refinement_outgoing")`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2470`
  `reduceGraphNaturalLoopOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2536`
  `reduceNaturalLoopFallbackOnce()` collapse 后清理 refinement marks。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1616`
  新增 `testRefineCyclicDropsOverlayRefinementMarksAfterCollapse()`：先构建 graph，再 mark `0 -> follow`，调用 `refineCyclic()` 后确认 structured loop 到 follow 的边仍可见。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2189`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4 && ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-drop-marks.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.70 user=39.02 sys=0.07 maxrss=220984`。

当前判断：

- `drop_refinement_marks` 的基础语义已和 Angr 对齐。
- 仍未实现 Angr `_refine_cyclic_core()` 里真正创建 `cyclic_refinement_outgoing` mark 的条件 break 改写；下一步要继续对照 Angr 那段做。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：补齐 replace_nodes_both 吸收 successor 入口

本轮补 Angr `replace_nodes_both(old_node_1=successor)` 的基础入口。Angr 里如果第二个旧节点不是当前 region member，而是 region successor，会先 replace 当前 member，再用 `absorb_successor_into()` 把 successor 在 full view 里的出边挂到新 structured node 上，同时隐藏 member 到 successor 的边。本轮只补共享 overlay 语义入口，不改具体 if/switch schema。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:302`
  新增 `RegionOverlay::replaceNodes(..., AbsorbedSuccessor, SelfLoop)` overload。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1574`
  实现 overload：先调用原 `replaceNodes()`，确认新 structured member 已进入当前 view，再调用 `absorbSuccessorInto()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:51`
  给 `collapseNodesAndSyncOverlay()` 增加可选 `AbsorbedSuccessor` 参数，后续 reducer schema 可以直接接 Angr `old_node_1` 语义。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1177`
  collapse 后把 `AbsorbedSuccessor` 传给 overlay replace；如果 source 已经是同一个 structured node，则直接执行 `absorbSuccessorInto()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1134`
  新增 `testOverlayReplaceNodesAbsorbsSuccessor()`，覆盖 replace member 后吸收 successor：新 structured member 接管 successor 出边，原 structured member 到 successor 的 full-view 边被隐藏。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2234`
  接入新测试。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4 && ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-replace-absorb.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.51 user=38.86 sys=0.06 maxrss=220604`。

当前判断：

- overlay 已有 Angr `replace_nodes_both()` 的 successor absorb 入口。
- 还没把具体 Phoenix if/switch reducer schema 改到这个入口；下一步要逐个对照 Angr schema 接入，避免猜测。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：补齐 overlay raw view 的 marked-edge 入口

本轮补 Angr `RegionOverlay.raw_graph` / `raw_graph_with_successors` 对应的基础能力。Angr 默认 view 会隐藏 `mark_edge()` 标记的边，但 raw view 可以显式包含这些 marked edges，供 Phoenix cyclic refinement 做试探和调试。本轮在 NotDec 的 overlay 查询 API 上增加 `IncludeMarkedEdges` 参数，默认仍隐藏 marked edges，不改变现有 reducer 行为。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:156`
  给 `OverlayManager::visibleNodeSuccessors()`、`visibleSuccessors()`、`quotientEdges()` 增加可选 `IncludeMarkedEdges` 参数，默认 `false`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:698`
  `visibleNodeSuccessors()` 在 `IncludeMarkedEdges=true` 时不再过滤 edge marks。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:748`
  `visibleSuccessors()` 把 `IncludeMarkedEdges` 传给 node successor 查询。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:760`
  `quotientEdges()` 在 member view、with-successors view、successor-to-successor view、extra-full edges 上统一支持包含 marked edges。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1187`
  扩展 `testOverlayEdgeMarksFilterAndRemap()`：验证默认 view 隐藏 marked edge，raw member view / raw full view 能看到 marked edge，并且 replace remap 后 raw view 仍能看到 remapped marked edge。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4 && ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-raw-view.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.76 user=39.13 sys=0.02 maxrss=220640`。

当前判断：

- overlay 查询现在有 Angr raw view 的最小等价入口。
- 默认 view 行为不变，现有 Phoenix reducer 仍会隐藏 marked edges。
- 还没实现 Angr `to_acyclic()` / blacklisted_edges 的零拷贝视图；这属于后续 reducer graph 视图对齐。
- 实现效果：7/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：补齐 overlay acyclic view 入口

本轮补 Angr `RegionOverlayGraph.to_acyclic_by_order()` 的最小对应能力。NotDec 目前还没有完整的 `RegionOverlayGraph` 对象，所以先在 `OverlayManager` 查询层增加显式 acyclic view 入口：按 view node order 过滤 `order[src] >= order[dst]` 的边，但不修改共享图，也不影响默认 view。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:163`
  新增 `visibleNodeSuccessorsAcyclic()`、`visibleSuccessorsAcyclic()`、`quotientEdgesAcyclic()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:68`
  新增 `isBackEdgeByOrder()`，按 Angr 的 `node_order[u] >= node_order[v]` 判断 view-level back edge；order 缺失时保守保留边。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:915`
  实现 `visibleNodeSuccessorsAcyclic()`，从 acyclic quotient full view 派生外部 successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:929`
  实现 `visibleSuccessorsAcyclic()`，保持当前 block successor 兼容入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:942`
  实现 `quotientEdgesAcyclic()`，先取默认 quotient view，再仅在返回 view 里过滤 blacklisted back edges。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:719`
  新增 `testOverlayAcyclicViewsFilterBackEdgesByOrder()`，验证默认 view 仍保留回边，acyclic view 按 order 去掉回边，并覆盖 child region successor 查询。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2321`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-acyclic-overlay.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.42 user=38.73 sys=0.09 maxrss=221712`。

当前判断：

- overlay 层现在具备 Angr blacklisted-edge acyclic view 的最小等价入口。
- 默认 `visible*()` / `quotientEdges()` 行为不变，现有 reducer 和 renderer 不受影响。
- 下一步应把 Phoenix cyclic / last-resort 中依赖 `MutableRegionGraph::analyze()` 临时 DFS dropped edges 的地方，逐步切到 overlay acyclic view 语义；这一步会碰到 block rewrite / deoptimization IR 边界，不能靠 fallback renderer 继续补特例。
- 实现效果：7/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：补齐 overlay 显式 blacklisted-edge view

本轮补 Angr `RegionOverlayGraph.to_acyclic(blacklisted_edges)` 的更直接入口。上一轮已经有按 node order 派生 back edge 的 acyclic view；这一轮增加显式 blacklist view，让后续 Phoenix / SAILR 能传入一组 view-level 边并只在查询结果里过滤它们，不修改共享图。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:163`
  新增 `visibleNodeSuccessorsBlacklisted()`、`visibleSuccessorsBlacklisted()`、`quotientEdgesBlacklisted()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:78`
  新增 `isBlacklistedEdge()`，按 `OverlayNodeKey` 的 view-level source/target 匹配 blacklist。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:923`
  实现 `visibleNodeSuccessorsBlacklisted()`，从 blacklisted full quotient view 派生 successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:937`
  实现 `visibleSuccessorsBlacklisted()`，保留当前 block successor 兼容入口。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:950`
  实现 `quotientEdgesBlacklisted()`，先取默认 quotient view，再过滤 blacklist 边；不调用 `hideEdge()`，所以不新增 rollback 状态。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1002`
  `quotientEdgesAcyclic()` 改成先按 order 派生 blacklist，再复用 `quotientEdgesBlacklisted()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:756`
  扩展 `testOverlayAcyclicViewsFilterBackEdgesByOrder()`，验证显式 blacklist 能过滤指定 member 边，且默认 view 保持不变。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:790`
  同一测试覆盖 child region successor 的 blacklist 查询。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2342`
  既有测试入口继续覆盖新增断言。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-blacklist-overlay.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.77 user=39.16 sys=0.08 maxrss=222204`。

当前判断：

- overlay view 现在同时具备 Angr `to_acyclic(blacklisted_edges)` 和 `to_acyclic_by_order(node_order)` 的最小接口。
- 这仍是查询层能力，尚未把 Phoenix cyclic / last-resort 的 reducer 流程切过来。
- 下一步如果继续切 reducer，需要处理 Angr `_refine_cyclic_core()` 里的 block copy / terminator rewrite。这是跨 C 和 Solidity 共用 IR 的真实设计点，不能用 fallback renderer 特例替代。
- 实现效果：7/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：补齐 overlay quasi-topological node order

本轮补 Angr `_generate_node_order()` 的 overlay 查询层基础。Angr 用 `graph_with_successors` 的 quasi-topological order 生成 `node_order`，再传给 `to_acyclic_by_order()`。NotDec 之前只能手写 `OverlayNodeKey -> order`，这不利于后续把 Phoenix / SAILR 的 cyclic 和 last-resort 流程切到 overlay view。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionOverlay.h:163`
  新增 `OverlayManager::quasiTopologicalNodeOrder()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:86`
  新增 `containsNodeKey()`，服务 Tarjan SCC。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:91`
  新增 `collectScc()`，在 view-level graph 上收集 SCC。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:973`
  实现 `quasiTopologicalNodeOrder()`：从 full quotient view 构造节点和边，收缩 SCC，按 component DAG 的确定性拓扑顺序生成 `OverlayNodeKey -> order`。
- `external/NotDec-llvm2c/lib/Structuring/RegionOverlay.cpp:1016`
  SCC 内如果包含当前 region head block，先把 head 放到 component 内第一个位置，贴近 Angr 的 loop-head 候选处理。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:733`
  扩展 `testOverlayAcyclicViewsFilterBackEdgesByOrder()`，验证简单循环上 helper 生成 `0,1,2` 顺序，并直接用于 `quotientEdgesAcyclic()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:798`
  同一测试验证 child region successor 也能生成 deterministic order。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-node-order.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.39 user=38.88 sys=0.05 maxrss=219132`。

当前判断：

- overlay 层现在能自己从 full quotient view 派生 `node_order`，后续 Phoenix / SAILR 不再需要外部手写 order。
- 这仍未切换 reducer 主流程；当前只是补齐 Angr `_generate_node_order()` 所需的共享 view 基础。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：last-resort edge ordering 使用 overlay node order

本轮开始把上一轮的 overlay `node_order` 接入 Phoenix reducer。范围只限 last-resort virtual edge ordering：如果当前 structuring 走 overlay path，就把 `OverlayNodeKey -> order` 映射到 `MutableRegionGraphAnalysis::NodeOrder`，供 Phoenix / SAILR 的 `orderVirtualizableEdges()` 使用；没有 overlay 时仍保持原来的 reducer DFS order。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:64`
  `virtualizeOneEdge()` 增加可选 `RegionOverlay *Overlay` 参数，默认 `nullptr`，保持旧测试和非 overlay path 调用不变。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1150`
  新增 `applyOverlayNodeOrder()`：从 `OverlayManager::quasiTopologicalNodeOrder()` 取 overlay order，再按 reducer node 的 `SourceNodes` 映射到 `GraphNodeId`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1170`
  一个 reducer node 如果对应多个 overlay source，取最小 order；找不到 overlay order 的 node 保留原分析结果。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2720`
  `lastResortRefinement()` 调用 `virtualizeOneEdge()` 时传入当前 overlay。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2739`
  `virtualizeOneEdge()` 在 `Graph.analyze()` 后调用 `applyOverlayNodeOrder()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:59`
  新增 `OrderCaptureStructurer`，捕获 `orderVirtualizableEdges()` 收到的 `Analysis.NodeOrder`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:822`
  新增 `testPhoenixOverlayPathUsesOverlayNodeOrder()`：构造一个 reducer DFS order 与 overlay quasi order 不同的图，验证非 overlay path 仍用旧 order，overlay path 使用 overlay order。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2405`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-order-virtualize.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.48 user=38.94 sys=0.05 maxrss=219224`。

当前判断：

- last-resort edge ordering 已经开始使用 Angr 风格 overlay view 的 order。
- 这还没有替换 `collectVirtualizableEdges()` 的 candidate 来源，也没有实现 Angr `_refine_cyclic_core()` 的 block rewrite。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：last-resort candidate 来源切到 overlay acyclic view

本轮继续对齐 Angr `PhoenixStructurer._last_resort_refinement()`。Angr 的 last-resort 候选边来自 full overlay graph 经 `to_acyclic_by_order(node_order)` 后的边，再按 member graph 做支配关系分桶。旧 NotDec 路径是从 reducer 当前 graph 直接收集 active edge，并在 root 上 fallback 到 DFS acyclic 构造时丢掉的边；这会绕过 overlay blacklist / marked edge 语义，也会把 Angr 已经通过 acyclic overlay view 排除的回边重新塞回候选集。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1092`
  新增 `collectOverlayVirtualizableEdges()`：overlay path 下从 `OverlayManager::quotientEdgesAcyclic()` 收集候选边，并把 `OverlayNodeKey` 映射回当前 reducer graph node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1110`
  `collectOverlayVirtualizableEdges()` 使用 `OverlayManager::quasiTopologicalNodeOrder()` 生成 Angr 风格 acyclic view 所需的 node order。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1161`
  `filterByAngrLastResortPriority()` 增加 `IncludeAcyclicDroppedEdges` 参数；overlay path 不再使用旧 reducer DFS dropped-edge fallback。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1170`
  补齐 Angr last-resort 的第三类 `other_edges` bucket，顺序为 no-dominance、secondary、other。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2832`
  `virtualizeOneEdge()` 在 overlay path 下使用 `collectOverlayVirtualizableEdges()`，非 overlay path 保持旧入口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:63`
  `OrderCaptureStructurer` 额外捕获传给 `orderVirtualizableEdges()` 的候选边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:865`
  新增 `testPhoenixOverlayLastResortUsesOverlayAcyclicCandidates()`：构造 `0 -> 1, 1 -> 0/2`，验证 overlay path 的 last-resort 候选不会包含 `1 -> 0` 回边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2441`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-acyclic-candidates.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.35 user=38.65 sys=0.05 maxrss=217976`。

当前判断：

- last-resort candidate 来源已经切到 Angr 风格 overlay acyclic view，减少了旧 reducer graph fallback 对 overlay 语义的绕过。
- 这还不是完整 Angr `_last_resort_refinement()`；真正的 `_refine_cyclic_core()` 还需要 block copy / terminator rewrite / mark_edge 这类共享 IR 能力，不能用 C 或 Solidity renderer 特例替代。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：virtual edge 同步 source replacement 到 overlay

本轮继续对齐 Angr `_virtualize_edge()` 的共享图语义。Angr 在虚拟化边时不是只隐藏一条边，还会在需要时创建 rewritten source node，然后执行 `detach_edge(src, dst)` 和 `replace_nodes_both(src, new_src)`。NotDec 之前已经能生成共享 `StructuredTree` 版 rewritten source，但 overlay 只同步了断边，没有把 source block 替换成 rewritten structured node；后续 overlay view 仍可能把 source 看成旧 block。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:95`
  新增 `MutableRegionGraph::setSourceNodes()`，允许 reducer node 在被 overlay replacement 后指向新的 structured overlay node。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:709`
  实现 `setSourceNodes()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1544`
  新增 `overlayNodesForGraphNode()`，优先使用 reducer node 的 `SourceNodes`，没有时回退到 block key。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1555`
  新增 `detachOverlayVirtualEdge()`，按 `OverlayNodeKey` 断开 shared overlay edge，支持 source 已经是 structured node 的情况。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1568`
  新增 `installVirtualizedEdge()`，统一执行 build rewritten source、`Graph.virtualizeEdge()`、overlay detach、`Overlay.replaceNodes()` 和 reducer `SourceNodes` 更新。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1591`
  明确按 Angr 顺序先 detach graph edge，再 replace source node，让 replacement 只继承剩余 real outgoing edges。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2878`
  hint path 改用 `installVirtualizedEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2898`
  普通 last-resort path 改用 `installVirtualizedEdge()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:231`
  新增 `testOverlayVirtualizationReplacesSourceNode()`，验证 overlay path 虚拟化 `0 -> 2` 后，`block 0` 被 structured node 替换，shared real edge 只保留 rewritten source 到 kept successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1650`
  更新 `testPhoenixOverlayLastResortDetachesVirtualizedEdge()`，不再要求旧 block successor 保留，而是验证 structured replacement 上 removed edge 消失、kept edge 保留。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-replace-virtualized-source.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.54 user=38.92 sys=0.04 maxrss=218052`。

当前判断：

- last-resort virtual edge 现在更接近 Angr：共享 overlay graph 同时看到 detach 和 replacement，不再只靠 renderer 或 reducer graph 记一条虚拟边。
- 这仍未覆盖 cyclic refinement 里现有 `virtualizeNonFollowLoopExits()` 的 overlay replacement；那条路径还需要继续按 Angr `_refine_cyclic_core()` 切。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：cyclic non-follow exits 复用 overlay virtual edge replacement

本轮把 cyclic refinement 里多出口 natural loop 的 `virtualizeNonFollowLoopExits()` 接到上一轮的 shared virtual edge 安装路径。这样 last-resort 和 cyclic refinement 不再各自维护一套“生成 rewritten source + 删除边”的逻辑；overlay path 也能看到同样的 detach + source replacement 语义。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2281`
  `virtualizeNonFollowLoopExits()` 增加 `RegionOverlay *Overlay` 参数。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2309`
  `virtualizeNonFollowLoopExits()` 改用 `installVirtualizedEdge()`，复用 Angr 风格 detach + replacement 顺序。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2640`
  `reduceGraphNaturalLoopOnce()` 调用 non-follow exit virtualization 时传入当前 overlay。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2314`
  新增 `testRefineCyclicOverlayVirtualizesNonFollowExits()`：构造有两个 loop exit 的 root，验证 overlay path 下非 follow exit `6` 被虚拟化，最终 loop structured node 只保留到 follow `5` 的 real edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2611`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-nonfollow-exits.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.86 user=39.27 sys=0.03 maxrss=220400`。

当前判断：

- 多出口 natural loop 的 graph-level virtualization 已经和 last-resort 使用同一套 overlay replacement 语义。
- 这仍不是完整 Angr `_refine_cyclic_core()`：outgoing edge 的条件恢复、`mark_edge(cyclic_refinement_outgoing=True)` 和更细的 while/do-while source block 查找还没完整迁到 overlay。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：extra continue edges 同步 overlay

本轮继续收敛 cyclic refinement 的 virtual edge 安装路径。Angr 对额外回到 loop head 的 continue edge 也是先 detach graph edge，能找到可改写 block 时再 replace source。NotDec 之前只在 reducer graph 里 `virtualizeEdge()`，overlay 仍可能保留旧的 real edge。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1568`
  `installVirtualizedEdge()` 增加 `RewriteSource` 参数；用于 Angr 那种只 detach、不改写 source 的情况。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2352`
  `virtualizeExtraContinueEdges()` 增加 `RegionOverlay *Overlay` 参数。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2402`
  extra continue edge 改用 `installVirtualizedEdge()`，普通 source 会同步 replacement，含 structured switch 的 source 只 detach，不改写。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2640`
  `reduceGraphNaturalLoopOnce()` 调用 extra continue virtualization 时传入当前 overlay。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2079`
  新增 `testRefineCyclicOverlayVirtualizesExtraContinues()`：构造双 latch loop，验证 overlay path 下 extra continue `4 -> 1` 被虚拟化，最终 loop structured node 保留 follow `5`，不再暴露到 head `1` 的 real edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2673`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-extra-continues.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.13 user=39.39 sys=0.05 maxrss=220584`。

当前判断：

- extra continue edge 现在也复用 Angr 风格 detach + optional replacement 语义，overlay 不再保留被虚拟化的 head edge。
- structured switch source 仍保持只 detach、不改写；这对应 Angr 找不到安全可改写 block 时先移除 edge 的保守分支。
- 还没补完整 successor outgoing edge 的 `mark_edge(cyclic_refinement_outgoing=True)` 和条件 break rewrite。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：successor outgoing edge 标记并补 break source

本轮继续对齐 Angr `_refine_cyclic_core()`。Angr 处理循环体跳到 loop successor/follow 的边时，不会像非 follow exit 那样直接从循环外边界删除，而是标记为 `cyclic_refinement_outgoing`，并在能安全改写 source 时把这条跳转写成 `break` 或条件 `break`。NotDec 这里使用共享 `StructuredTree` 表达 `If + Break`，C 和 Solidity 后端都只消费同一套结构化结果。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1570`
  新增 `markOverlayRefinementEdge()`，在 overlay shared graph 上标记 `cyclic_refinement_outgoing`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1623`
  新增 `rewriteLoopSuccessorExits()`：扫描 loop body 到 chosen follow 的 outgoing edge，跳过 head/latch，先标记 overlay edge；source 不是 structured switch 时，复用 `buildVirtualizedSource()` 生成 `break` source，并用 `Overlay::replaceNodes()` 把 overlay source 替换为 structured node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2704`
  在 `reduceGraphNaturalLoopOnce()` 里先处理 successor outgoing edge，再处理 non-follow exit virtualization。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:78`
  新增测试辅助 `treeContainsKind()`，用于检查生成的共享结构化树里是否含 `Break`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1987`
  新增 `testRefineCyclicOverlayMarksSuccessorBreakEdge()`，验证循环体内 `2 -> follow` 被改写出 `Break`，同时最终 overlay loop node 仍保留到 follow block 的 real edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2743`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-successor-break.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.46 user=38.76 sys=0.08 maxrss=216672`。

当前判断：

- successor outgoing edge 已经开始走 Angr 的 overlay 标记 + source rewrite 语义，不再靠 renderer 猜测循环出口。
- 还没实现完整 Angr `_find_node_going_to_dst()`：如果 source 已经是更深的 structured node，目前只改写 direct reducer source，不做嵌套节点替换。
- 这一步也暂时跳过 head/latch 的 loop-condition edge，避免把循环条件本身误写成 break source。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：structured source 内的 successor goto 改写为 break

上一轮 successor outgoing edge 只会改写 direct reducer source：如果 source 已经有 `StructuredRoot`，但跳到 follow 的控制转移藏在 structured subtree 里，就还没有 Angr `_find_node_going_to_dst()` 那种“在已结构化节点里找目标跳转”的语义。本轮补一个保守版本：只在共享 `StructuredTree` 里已经明确存在 `Target == follow` 的 `Goto/Break/Continue` 时复制子树并替换成 `Break`；不靠 CFG 猜深层 block。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:55`
  增加 `rewriteStructuredSourceTargetTransfer()` 前置声明，供 cyclic refinement 的 successor outgoing path 使用。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1662`
  `rewriteLoopSuccessorExits()` 现在优先尝试改写已有 `StructuredRoot` 里的显式 target；找不到时才回到原来的 `buildVirtualizedSource()` direct tail path。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1727`
  新增 `isTargetedControlTransfer()`，只匹配已经明确带 `Target` 的 `Goto/Break/Continue`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1734`
  新增 `copyReplacingTargetTransfer()`，递归复制 structured subtree 并替换匹配 target；遇到 `Switch` 或嵌套 loop 时不进入，避免把外层 loop 的 `break` 写进错误层级。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1794`
  新增 `rewriteStructuredSourceTargetTransfer()`，对外返回新的 replacement root；没有明确 target 时返回 `InvalidNodeId`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:103`
  新增 `treeContainsGotoTarget()` 测试辅助。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2067`
  新增 `testRefineCyclicRewritesStructuredSuccessorGoto()`：给 block `2` 预置一个 structured source，里面含 `goto 4`；refine 后验证 replacement root 含 `Break`，且不再含 `goto 4`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2820`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-overlay-structured-target.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.52 user=38.87 sys=0.08 maxrss=218760`。

当前判断：

- 这一步比上一轮更接近 Angr `_find_node_going_to_dst()`：source 已经结构化时，不再只能按原始 tail block 重建，而是能改写共享结构化结果里的显式 target。
- 这还不是完整 `_find_node_going_to_dst()`：NotDec 仍没有 AIL block 级 first/last statement 语义，也没有从 structured source 里恢复条件表达式的完整路径；当前只处理已经明确编码在 `StructuredNode::Target` 里的控制转移。
- 实现效果：7/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：shared GotoManager 基础接口

继续对齐 Angr SAILR deoptimization 的外围依赖。Angr 的 `StructuringOptimizationPass` 在 `DURING_REGION_IDENTIFICATION` 阶段会反复跑 `RecursiveStructurer + RegionSimplifier + GotoManager`，用 gotos 数量和 goto edge 位置判断 graph rewrite 是否值得保留。NotDec 还没有这条 deoptimization pass 管线，本轮先补 shared structuring 层的最小 `GotoManager`，只从 `StructuredTree` 收集显式 `Goto`，不碰 C/Solidity renderer。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h:14`
  新增 `StructuredGoto`，记录 `Source`、`Target` 和对应 `StructuredTree` node。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h:30`
  新增 `GotoManager`，提供 `collect()`、`gotosInBlock()`、`isGotoEdge()`、`size()` 等最小查询接口。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:6`
  新增 `sourceBlockForNode()`，用结构化节点自身的明确 `Block` 作为后续 goto source；裸 `Goto` 继承当前 source。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:28`
  新增 `collectGotos()`，递归遍历 `StructuredTree`，收集 `StructuredNodeKind::Goto` 且 `Target` 有效的节点。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:9`
  将 `GotoManager.cpp` 接入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:424`
  新增 `testGotoManagerCollectsSequenceGotoSources()`，验证 sequence 里裸 `goto` 继承前一个 basic block source。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:451`
  新增 `testGotoManagerCollectsIfGotoSources()`，验证 `if` 分支内的 goto 使用 `if` 自己的 block source。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2835`
  将两个新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-goto-manager.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.51 user=38.82 sys=0.09 maxrss=221300`。

当前判断：

- 这一步提供了 SAILR deoptimization 所需的第一个 shared metric：结构化结果里的 gotos 可被算法层统计和查询，不需要 renderer 反查文本或 AST。
- 这不是完整 Angr `StructuringOptimizationPass`：还没有固定点分析、graph rewrite 回滚、相对质量评估，也没有 `RegionSimplifier` 那种后处理后的 goto 归类。
- 实现效果：6/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：shared StructuringEvaluator 只读试跑

继续补 Angr SAILR deoptimization 的外围依赖。Angr 的 `_graph_is_structurable()` 会先对当前图做一次 region identification、recursive structuring，再更新 goto manager。NotDec 这里先实现只读试跑，不做 graph rewrite、固定点循环和质量选择。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringEvaluator.h:9`
  新增 `StructuringEvaluation` 和 `StructuringEvaluator`，返回 structured tree、goto summary 和成功标记。
- `external/NotDec-llvm2c/lib/Structuring/StructuringEvaluator.cpp:8`
  `StructuringEvaluator::evaluate()` 依次调用 `RegionIdentifier::identifyOverlay()`、`RecursiveStructurer::structure()` 和 `GotoManager::collect()`。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:9`
  将 `StructuringEvaluator.cpp` 接入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:41`
  sequence source 推进时不再跳过 `Label`，让 evaluator 收集到的 goto source 和共享 structured tree 的显式 block 更一致。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:170`
  新增 `GotoEmittingRegionStructurer` 测试辅助，直接生成带 `Goto` 的 structured tree。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:499`
  新增 `testStructuringEvaluatorCollectsGotoSummary()`，验证 evaluator 能完成递归结构化并收集 `0 -> 1` 的 goto。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2877`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structuring-evaluator.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.63 user=38.82 sys=0.08 maxrss=221528`。

当前判断：

- 这一步把 Angr `StructuringOptimizationPass` 的只读试跑能力放进 shared structuring 层，后续 C 和 Solidity 都能复用同一份结构化结果和 goto summary。
- 它还不是完整 SAILR：没有 deoptimization pass、没有 graph rewrite checkpoint、没有 fixed point，也没有 `RegionSimplifier`。
- 实现效果：5/10。
- 复杂度：2/10。
- 维护成本：2/10。

# 2026-06-21 实现记录：shared ControlFlowStructureCounter

继续对齐 Angr `StructuringOptimizationPass._improves_relative_quality()`。SAILR deoptimization 不能只看图还能不能结构化，也要避免 rewrite 之后把 loop 或 goto label 分布变差。本轮先在 shared `StructuredTree` 上实现可精确统计的部分，不碰 renderer。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringQuality.h:11`
  新增 `ControlFlowStructureCounter`，统计 while / do-while / infinite loop、goto target 计数和 label 顺序；保留 `ForLoops` 字段，因为 NotDec 当前没有 for-loop 节点。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringQuality.h:30`
  新增 `improvesRelativeStructuringQuality()`，用于后续 deoptimization pass 判断 rewrite 是否让结构质量变差。
- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:8`
  实现 structured tree 递归遍历，收集 loop、goto target 和 label 顺序。
- `external/NotDec-llvm2c/lib/Structuring/StructuringQuality.cpp:82`
  实现 Angr 风格相对质量检查：同等 loop 总数时不允许丢 for-loop；goto 总数不变时，不允许 target label 变多，也不允许把 goto 压到更靠前的 label。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringEvaluator.h:17`
  `StructuringEvaluation` 增加 `Quality` 字段。
- `external/NotDec-llvm2c/lib/Structuring/StructuringEvaluator.cpp:15`
  evaluator 成功结构化后同时收集 `ControlFlowStructureCounter`。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:10`
  将 `StructuringQuality.cpp` 接入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:513`
  扩展 evaluator 测试，验证返回的 quality 里包含 goto target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:516`
  新增 `testControlFlowStructureCounterCollectsSharedQuality()`，验证 loop、goto target、label 顺序统计。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:551`
  新增 `testRelativeQualityRejectsBackwardGotoTrade()`，验证更靠前 label 的 goto trade 会被拒绝。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:564`
  新增 `testRelativeQualityRejectsMoreGotoTargets()`，验证 goto 总数不变但 target label 变多会被拒绝。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2880`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structuring-quality2.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.45 user=38.74 sys=0.08 maxrss=217284`。

当前判断：

- 这一步补齐了 SAILR deoptimization 的第二个 shared metric：结构质量可以在算法层比较，C 和 Solidity 后端不用各自判断。
- 还没有实现完整 Angr pass 管线：没有 fixed point，没有 rewrite rollback，也没有 `RegionSimplifier` 后的质量统计。
- 实现效果：6/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：shared StructuringOptimizationPass wrapper

继续对齐 Angr `StructuringOptimizationPass.analyze()`。本轮只补公共 pass wrapper：它负责初始 structuring 检查、goto guard、rewrite 后重新结构化、失败回滚和相对质量检查。具体 SAILR deoptimization 规则还没有接入。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:15`
  新增 `StructuringOptimizationOptions`，按 Angr 默认值保存 `RequireStructurableGraph`、`PreventNewGotos`、`StrictlyLessGotos`、`RecoverStructureFails`、`MustImproveRelativeQuality`、`RequireGotos` 和 `MaxOptIters`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:25`
  新增 `StructuringOptimizationResult`，返回是否成功、是否修改、输出 CFG 和最终 structuring evaluation。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:32`
  新增 `StructuringOptimizationPass` 基类，子类只实现 `runOnGraph()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:5`
  实现初始 evaluation 是否需要运行的判断。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:10`
  实现最终 evaluation 的 goto 数量和 relative quality 检查。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:33`
  实现 `analyze()` 主流程：初始检查、候选 CFG rewrite、每轮重新 `StructuringEvaluator::evaluate()`，结构化失败时按配置回滚，最后输出通过检查的 CFG。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:10`
  将 `StructuringOptimizationPass.cpp` 接入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:196`
  新增 `CFGEdgeGotoRegionStructurer`，测试用 CFG successor 生成显式 goto。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:226`
  新增 `RemoveFirstSuccessorPass`，模拟减少 goto 的 rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:245`
  新增 `AddFirstSuccessorPass`，模拟引入新 goto 的 rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:646`
  新增 `testStructuringOptimizationPassAcceptsImprovedGraph()`，验证减少 successor/goto 的 rewrite 被接受并返回新 CFG。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:663`
  新增 `testStructuringOptimizationPassRejectsNewGotos()`，验证 `PreventNewGotos` 会拒绝新增 goto 的 rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3044`
  将两个新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structuring-optpass.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.92 user=39.17 sys=0.05 maxrss=221704`。

当前判断：

- 这一步把 Angr pass wrapper 的边界放到了 shared structuring 层，后续 SAILR deoptimization 子类可以只做 CFG rewrite，不需要碰 C/Solidity renderer。
- 还没实现 Angr 具体 deoptimization pass，也没有 `RegionSimplifier`；当前 wrapper 使用 `StructuringEvaluator` 的 shared tree 结果做检查。
- 实现效果：6/10。
- 复杂度：4/10。
- 维护成本：4/10。

# 2026-06-21 实现记录：optimization pass 失败回滚后继续固定点

继续对齐 Angr `_fixed_point_analyze()`。Angr 在一次 rewrite 导致结构化失败时，如果允许恢复，会回滚到上一份 graph，然后继续后续迭代直到 `max_opt_iters`。NotDec wrapper 之前回滚后直接退出循环，本轮改成回滚后继续。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:70`
  `StructuringOptimizationPass::analyze()` 在 rewrite 后重新 evaluate；如果失败且 `RecoverStructureFails` 为 true，恢复上一份 `StructuredCFG` 和 evaluation 后继续下一轮，而不是直接 `break`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:226`
  新增 `FailOnBlockRegionStructurer`，测试用来模拟某次 rewrite 后 graph 不可结构化。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:278`
  新增 `RecoverThenRemoveSuccessorPass`，第一轮制造不可结构化临时图，第二轮执行有效 rewrite。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:730`
  新增 `testStructuringOptimizationPassRecoversAndContinuesFixedPoint()`，验证失败图被回滚、第二轮继续执行、临时 block 不会进入最终输出。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3118`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structuring-optpass-recover.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.47 user=38.81 sys=0.05 maxrss=221332`。

当前判断：

- 这一步让 shared pass wrapper 的固定点恢复语义更接近 Angr。
- 仍未进入需要复制 block 的具体 SAILR deoptimization，因此避开了 C 后端 virtual block id 的未决问题。
- 实现效果：6/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：structuring trial 支持 edges_to_remove

继续对齐 Angr `StructuringOptimizationPass._graph_is_structurable()`。Angr 会在每次试结构化前按 pass 配置临时删除一些边，再做 region identification 和 recursive structuring。本轮把这个行为放到 shared `StructuringEvaluator`，作为只读 trial view，不修改原始 `StructuredCFG`。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringEvaluator.h:13`
  新增 `StructuringEdge`，表达要从 structuring trial view 里临时删除的 CFG edge。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringEvaluator.h:30`
  `StructuringEvaluator::evaluate()` 增加带 `EdgesToRemove` 的重载。
- `external/NotDec-llvm2c/lib/Structuring/StructuringEvaluator.cpp:11`
  新增 `withoutEdges()`，复制 `StructuredCFG` 后删除指定 successor 和 switch case target；如果删完 successor 且原 terminator 不是 return/unreachable，则把 terminator 标成 unreachable。
- `external/NotDec-llvm2c/lib/Structuring/StructuringEvaluator.cpp:51`
  带删边参数的 evaluator 使用删边后的 view 执行 `RegionIdentifier::identifyOverlay()`、`RecursiveStructurer::structure()`、`GotoManager::collect()` 和 quality 统计。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:24`
  `StructuringOptimizationOptions` 增加 `EdgesToRemove`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:40`
  pass wrapper 的初始 evaluation 使用 `Options.EdgesToRemove`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:70`
  rewrite 后和回滚后的 evaluation 同样使用 `Options.EdgesToRemove`，保证每轮固定点检查同口径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:638`
  新增 `testStructuringEvaluatorRemovesEdgesForTrialOnly()`，验证 trial view 会删掉 `0 -> 1` 的 goto，但原 CFG successor 不变。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:746`
  新增 `testStructuringOptimizationPassUsesRemovedEdgesForInitialGotos()`，验证 pass 的初始 goto 判断使用删边后的 view。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3149`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structuring-edges-to-remove.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.52 user=38.97 sys=0.06 maxrss=222148`。

当前判断：

- 这一步继续补齐 Angr pass wrapper 的通用能力，不涉及 C/Solidity renderer。
- `EdgesToRemove` 现在是 trial view 语义，还不是具体 deoptimization pass 的 rewrite 规则。
- 实现效果：6/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：SAILR deoptimization pipeline 骨架

继续对齐 Angr `StructuringOptimizationPass` 的 pass-manager 形状。Angr 的 SAILR 不是单个 structurer 类完成的，而是 Phoenix/SAILR structuring 加一组 deoptimization pass；每个 pass 试图改 graph，成功后才把结果交给后续 pass，失败则跳过。本轮先补 shared pipeline 和 `_get_new_gotos()` 钩子，不实现需要复制 block 的具体 pass。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:47`
  新增 `StructuringOptimizationPass::getNewGotos()` 虚函数，对齐 Angr `_get_new_gotos()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:5`
  默认 `getNewGotos()` 返回当前 evaluation 的 goto 集。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:20`
  final evaluation 的 new-goto 检查改为使用 `getNewGotos()`，后续 pass 可以定制比较口径。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPipeline.h:11`
  新增 `StructuringOptimizationPipeline`，表达 SAILR deoptimization pass 的顺序执行入口。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPipeline.cpp:7`
  实现 `addPass()` 和 `run()`；pass 成功则更新当前 `StructuredCFG`，失败或无变化则跳过。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:11`
  将新 pipeline 源文件接入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:279`
  新增 `AddFirstSuccessorIgnoringNewGotosPass`，测试 `_get_new_gotos()` 风格钩子。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:759`
  新增 `testStructuringOptimizationPassCanOverrideNewGotos()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:811`
  新增 `testStructuringOptimizationPipelineKeepsAcceptedPasses()`，验证已接受 pass 的结果不会被后续被拒绝 pass 覆盖。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3203`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target structuring-analysis-test -j4 && /sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pipeline.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.76 user=39.12 sys=0.04 maxrss=221724`。

当前判断：

- 这一步只补 SAILR pass 组织和安全检查扩展点，仍不触碰 block 复制。
- 后续实现 `CrossJumpReverter`、`ReturnDuplicatorLow`、`DuplicationReverter` 时，仍需要先决定虚拟/复制 block 的 shared ID 和 C/Solidity 渲染策略。
- 实现效果：6/10。
- 复杂度：3/10。
- 维护成本：3/10。

# 2026-06-21 实现记录：copied / virtual block 的 shared 表示

本轮先解决 SAILR deoptimization 继续往下做必须碰到的 block 复制边界。当前没有实现具体 SAILR pass；只把 `StructuredCFG`、shared structurer 和 C adapter 的身份边界补齐，让后续 pass 能新建或复制 block，而不是把复制语义写进 C/Solidity renderer。

核心约定：

- `CFGBlock::Id` 是控制流身份。复制或新建 block 必须有自己的稳定 `BlockId`。
- `CFGBlock::BodyBlock` 是渲染 body 的来源。原始 block 指向自己，复制 block 指向原始 body。
- Phoenix / Goto structurer 生成 `BasicBlock` 时保留当前控制流 `BlockId`，但 statements 从 `BodyBlock` 读取。
- C adapter 的 label / goto 也按 shared `BlockId` 建 label，不再依赖原始 `CFGBlock *`。这样 copied block 不会和原始 block 共用 label。
- C adapter 的 fallback if / switch 读取 shared `StructuredCFG` successor，不再回读原始 C CFG successor。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:52`
  新增 `CFGBlock::BodyBlock`，记录 copied / virtual block 的 body 来源。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:67`
  新增 `StructuredCFG::duplicateBlock()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:73`
  新增 `bodyBlock()` / `getBodyBlock()` 查询。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:5`
  `addBlock()` 默认把原始 block 的 `BodyBlock` 设成自己的 `Id`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:17`
  实现 `duplicateBlock()`，新 block 用新的 `BlockId`，body 来源沿用原始 body，successor 由调用者传入。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:42`
  实现 `bodyBlock()` / `getBodyBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:132`
  `appendBlockBody()` 改为从 `getBodyBlock()` 取 statements，但 `StructuredNode::Block` 仍是当前控制流 block。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:18`
  新增 `makeBlockBody()`，两条 Goto structuring 路径都复用 shared body-source 规则。
- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuredGoto.h:35`
  C 后端 adapter 新增 `createStructuredBlockLabelStmt()`，用于按 shared `BlockId` 创建 detached label。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:31`
  C adapter 保存当前 `StructuredCFG` 和 `BlockId -> LabelStmt`，renderer 不再保存 `BlockId -> CFGBlock *`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:75`
  新增 `getOrCreateLabelStmt()` / `getOrCreateLabel()`，label 身份来自 shared `BlockId`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:230`
  `invertCond()` 改为新建反向比较表达式，不再原地改 payload。否则一个 condition payload 被 while / if 复用时，后渲染节点会改坏前面的条件。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:289`
  fallback `renderIf()` 改为读取 shared CFG successor。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:339`
  fallback `renderSwitch()` 改为读取 shared CFG successor 和 case target。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:427`
  实现 `createStructuredBlockLabelStmt()`，生成 `structured_block_<id>` label。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:362`
  新增 `hasSinglePayload()` 测试 helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:671`
  新增 `testStructuredCFGDuplicatesBlockBodySource()`，验证复制 block 的稳定新 ID、body 来源和 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:696`
  新增 `testGotoStructurerRendersVirtualBlockBodySource()`，验证只设置 `BodyBlock` 的 virtual block 会渲染原始 statements，但 node block 仍是 virtual ID。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3248`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-copied-block.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.34 user=39.80 sys=0.11 maxrss=220248`。

当前判断：

- copied / virtual block 的最小 shared 表示已经落到 `StructuredCFG`，C 和 Solidity 都能通过 shared tree 读取 body payload。C 后端只保留必要的 Clang label 承载差异。
- 这一步还没有实现 Angr 的 `ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`。
- 下一步可以开始做第一个具体 SAILR deoptimization pass；优先选只需要复制 return/goto target block、接口最窄的 pass。
- 实现效果：7/10。复制 block 的身份和 body 来源边界已经明确。
- 复杂度：4/10。新增字段和少量 renderer adapter 状态，但没有新增算法分支。
- 维护成本：4/10。后续 pass 必须遵守 `Id` / `BodyBlock` 分工，否则容易把 label 身份和 body 来源混在一起。

# 2026-06-21 实现记录：接入 shared CrossJumpReverter

本轮开始实现具体 SAILR deoptimization pass。先选 Angr `CrossJumpReverter`，因为它只需要复制一个 linear goto target block，边界比 `ReturnDuplicatorLow` 的 return-region / Phi / connected component 逻辑窄。

对照 Angr 行为：

- 遍历当前 structuring 结果里的 goto source。
- 每个 source 只处理一个 goto。
- goto target 必须是当前 CFG successor。
- goto target 只能有一个 outgoing edge，避免复制条件块。
- 给每个要更新的 predecessor 复制一份 target block，并把 predecessor 的 successor 从原 target 改到 copy。
- 如果所有 predecessor 都被更新，删除原 target。
- NotDec shared 层没有 call counter；当前用 backend-neutral 的 statement 数量作为保守上限，默认最多复制 16 条 payload statement，不看 C/Solidity payload 内容。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:73`
  新增 mutable `getBlock()`，给 shared deoptimization pass 修改 CFG。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  新增 `removeBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:44`
  实现 mutable `getBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:65`
  实现 `removeBlock()`；删除 body source 前会把引用它的 copy 改成自持 body，并清掉其他 block 指向被删 block 的 successor / switch case。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:12`
  新增 `CrossJumpReverter` pass 类。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:30`
  新增 `buildSAILRDeoptimizationPipeline()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:12`
  新增 shared CFG 辅助函数：successor 判断、predecessor 收集、successor 替换。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:53`
  `CrossJumpReverter::defaultOptions()` 对齐 Angr 形状：`StrictlyLessGotos = true`，`MaxOptIters = 3`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:60`
  实现 `CrossJumpReverter::runOnGraph()`：收集候选、复制 target、重定向 pred、必要时删除原 target。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:136`
  `buildSAILRDeoptimizationPipeline()` 当前接入 `CrossJumpReverter`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRStructurer.h:18`
  `SAILRStructurer` override `structure()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:113`
  SAILR structuring 前先跑 shared deoptimization pipeline；pipeline 无变化时保留原 CFG。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:16`
  接入 `SAILRDeoptimization.cpp`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:718`
  新增 `testStructuredCFGRemoveBlockMaterializesCopiedBody()`，验证删除原 body source 后 copied block 仍能渲染 body。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:735`
  新增 `testCrossJumpReverterDuplicatesLinearGotoTarget()`，验证两个 goto predecessor 被改到两份 copied target，原 target 被删除。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3311`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。这里覆盖了 `notdec-llvm2c --algo=structured-sailr` 的 smoke。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-crossjump-phoenix.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.56 user=38.80 sys=0.10 maxrss=219716`。

验证限制：

- 顶层 `notdec --algo` 当前不接受 `sailr` / `structured-sailr`，所以 fortune 性能 smoke 仍按当前可用的 `--algo=phoenix` 口径跑。SAILR pipeline 路径由 llvm2c 的 `structuring-smoke` 覆盖。

当前判断：

- `CrossJumpReverter` 的 shared CFG rewrite 已经接入 SAILR pipeline。
- 这一步没有处理 Angr 的 call counter；shared 层目前只有 payload ref，不知道 payload 是否是 call，所以用 statement 数量做 backend-neutral 复制上限。
- `ReturnDuplicatorLow` 已经有最小 shared return-block 复制子集，但还没有补 Angr 的 return-region / Phi / connected component 语义。
- `DuplicationReverter`、`LoweredSwitchSimplifier` 还没有实现。
- 实现效果：7/10。第一个具体 SAILR deoptimization pass 已落地。
- 复杂度：4/10。主要增加一个 pass 和 `StructuredCFG::removeBlock()`。
- 维护成本：4/10。后续 pass 需要继续复用 `duplicateBlock()` / `removeBlock()`，避免各自手写 CFG 复制和删除语义。

# 2026-06-21 实现记录：ReturnDuplicatorLow 最小 shared 子集

本轮继续对照 Angr `ReturnDuplicatorLow`。完整 Angr 版本会寻找包含 end node 的 return-region，支持单入口链、connected in-edge component、Phi / virtual variable 刷新和 return region 删除。当前 shared `StructuredCFG` 还没有 Phi / vvar payload 语义，所以本轮只实现明确安全的最小子集：复制被多个 goto predecessor 共享的单个 terminal return block。

实现范围：

- 只处理 `TerminatorKind::Return` 且无 successor 的单 block return。
- return block 必须有多个 predecessor。
- 只复制当前 structuring 结果中 `GotoManager` 明确标记为 goto edge 的 predecessor。
- 每个 predecessor 得到一份 copied return block。
- 如果所有 predecessor 都被复制重定向，删除原 return block。
- 不处理 return-region 链、不处理 Phi / vvar 刷新、不做 connected in-edge component 合并。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:12`
  新增 `ReturnDuplicatorLow` pass 类。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:53`
  新增 `ReturnDuplicatorLow::defaultOptions()`，默认 `MaxOptIters = 4`，沿用 pass wrapper 的 goto / quality guard。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:59`
  实现 `ReturnDuplicatorLow::runOnGraph()`：收集共享 return block、筛选 goto predecessor、复制 return block、重定向 predecessor、必要时删除原 return block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:71`
  新增 `findLinearReturnRegion()`，只沿单 successor 的直线 return tail 向前扩展；因为 shared `StructuredCFG` 还没有 Phi / vvar payload 语义，所以暂时不碰分支 return-region。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:114`
  新增 `copyRegionForPredecessor()`，按 shared block id 复制整条 return tail，再把复制链内部 successor 重定向到复制块。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:176`
  `ReturnDuplicatorLow::runOnGraph()` 现在按 region head 处理线性 return tail，而不是只处理单个 return block。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:129`
  删除原 return block 前会确认所有当前 predecessor 都已经成功重定向，避免部分复制失败时删掉仍被引用的原块。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:215`
  同样收紧 `CrossJumpReverter` 删除原 target block 的条件。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:226`
  `buildSAILRDeoptimizationPipeline()` 按 Angr full preset 顺序把 `ReturnDuplicatorLow` 放在 `CrossJumpReverter` 前面。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:778`
  新增 `testReturnDuplicatorLowDuplicatesGotoReturnTarget()`，验证 `[preds] -> head -> return` 这条线性 return tail 会被复制成两份，原 head / return block 被删除，复制块自持。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3356`
  将新测试接入 `structuring-analysis-test`。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-retdup-chain.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.73 user=40.09 sys=0.08 maxrss=220944`。

当前判断：

- `ReturnDuplicatorLow` 的 shared CFG 子集已经从单块 return 提到线性 return tail。
- 这仍然不是完整 Angr `ReturnDuplicatorLow`：还缺 return-region 的分支情况、connected in-edge component、Phi / vvar 刷新和更广的多 block region copy。
- 暂时没有触碰 C/Solidity renderer，也没有把 payload 解释写进算法层。
- 实现效果：7/10。已经覆盖 Angr low pass 里最常见的直线 return-tail 形状。
- 复杂度：4/10。新增了 region 发现和 region 复制 helper，但还在 shared CFG 层。
- 维护成本：4/10。后续补完整 ReturnDuplicator 时，分支 return-region 还是要先把 Phi / payload 语义说清。

# 2026-06-21 实现记录：DuplicationReverter exact-match shared 合并

本轮补 Angr `DuplicationReverter` 的 shared 子集。完整 Angr 版本会先找相似块，再构造 merge graph 做合并；当前 shared `StructuredCFG` 还没有足够的 payload 语义去做相似度和 merge graph，所以本轮只做安全的 exact-match 子集：合并两个形状完全相同、前驱集合不相交、且彼此没有直接边的 block。

实现范围：

- 只比较 block 的 terminator、condition payload、successor 列表、case 列表和 statement payload 列表。
- 不比较 `BodyBlock`，避免把 copied block 的 shared body 身份误当成语义差异。
- 只在两个块形状相同、前驱集合不相交、且没有直接互指时才合并。
- 合并时把被删 block 的所有 predecessor 重定向到保留 block。
- 不做相似性搜索，不做 merge graph，不碰 renderer。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:12`
  新增 `DuplicationReverter` pass 类。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:18`
  新增 shared block 形状比较 helper 和 predecessor 集合判断 helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:228`
  实现 `DuplicationReverter::runOnGraph()`，按 exact-match 条件合并重复块。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:311`
  `buildSAILRDeoptimizationPipeline()` 现在按 Angr preset 顺序把 `DuplicationReverter` 放到 `ReturnDuplicatorLow` 前面。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:65`
  新增测试用 subclass，直接暴露 `DuplicationReverter::runOnGraph()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:784`
  新增 `testDuplicationReverterMergesExactDuplicateBlocks()`，验证并行重复块会合并，且 predecessor 会改到保留块。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-dupmerge.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=31.41 user=38.78 sys=0.07 maxrss=217252`。

当前判断：

- `DuplicationReverter` 的 exact-match shared 子集已经接入 pipeline。
- 这不是完整 Angr `DuplicationReverter`：还缺 similarity search、merge graph 和更宽的语义合并。
- 实现效果：6/10。只覆盖最保守的重复块合并。
- 复杂度：3/10。主要是 shared CFG 的 shape 判定和 predecessor 重定向。
- 维护成本：3/10。后续如果要扩到相似块合并，需要先补更强的 shared payload 比较语义。

# 2026-06-21 实现记录：SwitchDefaultCaseDuplicator shared default reuse 子集

本轮继续补 switch 侧的 shared deoptimization，但没有硬做 Angr `LoweredSwitchSimplifier`，因为当前 shared CFG 还没有 if-chain / comparison payload 的表达，直接猜 lower-switch 语义会越过边界。先落一个可以明确表达的子集：`Switch` 默认分支被外部 predecessor 复用时，复制 default block 给外部 predecessor。

实现范围：

- 只处理 `TerminatorKind::Switch` 且存在默认 successor 的 block。
- 只在 default block 还有单 successor 时复制。
- 默认 block 被多个 predecessor 复用时，保留一个 switch predecessor 指向原 default block。
- 其他 predecessor 改成指向 copied default block。
- 复制块继承原 default block 的 body / statements / successor。
- 不改 case payload，不做 if-chain 回写，不碰 renderer。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:34`
  新增 `SwitchDefaultCaseDuplicator` pass 类。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:163`
  新增 `defaultSwitchSuccessor()` 和 `SwitchDefaultCaseDuplicator::defaultOptions()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:174`
  实现 `SwitchDefaultCaseDuplicator::runOnGraph()`：复制被复用的 default block，并把外部 predecessor 改到 copy。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:311`
  `buildSAILRDeoptimizationPipeline()` 里把该 pass 放到 `CrossJumpReverter` 之前。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:71`
  新增测试用 subclass，直接暴露 `SwitchDefaultCaseDuplicator::runOnGraph()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:847`
  新增 `testSwitchDefaultCaseDuplicatorCopiesReusedDefaultBlock()`，验证外部 predecessor 会走 copied default block，switch 自己仍保留原 default block。

验证：

- `cmake --build /sn640/NotDec2/build --target notdec-backend-structuring structuring-analysis-test notdec-llvm2c-exe -j4`
  通过。
- `/sn640/NotDec2/build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。
- `ctest --test-dir /sn640/NotDec2/build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
  通过，5 个测试。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-switchdup.c --tr-level=2 --algo=phoenix`
  通过，`elapsed=32.04 user=39.42 sys=0.10 maxrss=218804`。

当前判断：

- 这只是 switch default reuse 的 shared 子集，不是完整 `LoweredSwitchSimplifier`。
- 还缺 if-chain 识别、switch lowering 回写和 case cluster 逻辑。
- 实现效果：5/10。只覆盖 switch 侧最稳的块级复用。
- 复杂度：3/10。只在 shared CFG 上复制 default block 和改 predecessor。
- 维护成本：3/10。后面真做 `LoweredSwitchSimplifier` 时，这个子集可以继续复用 copied block 语义。

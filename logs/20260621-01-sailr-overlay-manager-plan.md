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

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

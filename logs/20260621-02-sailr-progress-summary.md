# SAILR structuring 当前进展总结

可以先读 `logs/20260621-01-sailr-overlay-manager-plan.md` 前 73 行了解之前的规划。73 行之后都是进度记录。

## 当前状态

当前 goal 还没完成。已经完成的是 Angr 风格 structuring 的公共基础层，以及 copied / virtual block 的最小 shared 表示；还没完成的是完整 SAILR deoptimization 算法。

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

## 还差什么

还差真正的 Angr SAILR deoptimization pass：

- `ReturnDuplicatorLow`
- `CrossJumpReverter`
- `DuplicationReverter`
- `LoweredSwitchSimplifier`
- 相关 switch / duplication 辅助逻辑

这些 pass 还没有完整实现。

之前的主要卡点是：Angr 这些 pass 基本都会复制或新建 block。当前已经有最小 shared 表示，但具体 pass 还没有实现。

例子：

- `CrossJumpReverter` 复制 goto target
- `ReturnDuplicatorLow` 复制 return block
- `DuplicationReverter` 新建 merge graph
- `LoweredSwitchSimplifier` 维护 block copies

当前 C renderer 的 label 身份已经改为 shared `BlockId`，body payload 通过 `BodyBlock` 走 shared CFG。后续 pass 仍要注意不要把新 block 的算法语义写到 C/Solidity renderer 里。

## 下一步建议

下一步应该开始实现第一个真正的 SAILR pass。优先选接口最窄、只需要复制 return/goto target block 的 pass，再接入 `StructuringOptimizationPipeline`。

一句话总结：公共架构、Angr 风格执行框架、copied / virtual block 的 shared 表示已经搭好；完整 SAILR 还没完成，下一步是实现具体 deoptimization pass。

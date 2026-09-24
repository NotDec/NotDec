# 20260923-05 source-cases 回归修复：自然循环吸收 follow 块 + overlay 边失联

## 0. 一句话

交接文档 `20260923-03` §4 的回归定位并修好：`0f3c38f` 把 `isSequenceContinuation` 放宽到
“任意单活后继”之后，自然循环 region 的结构根把 follow 块一起吸收（follow 在子 region 图里是
`ExternalPlaceholder` 节点），外层 region 按 `Blocks.front()` 装配时又渲染了一遍，于是 7 个
source case 在 `return` 之后多出一份重复语句。只堵这一处后 `structuring-source-cases` 从
HEAD 的 8 个失败降到 1 个，剩下的失败暴露出同一批形状上的两个真 bug，一并修好：
子 region 走 fallback 装配时进入边没有重指向最终 structured key（父图丢掉进 region 的边，
入口 guard 被当成 fallthrough 平铺）；natural loop region 的支配分析在“没有无前驱节点”时
退化成最小 id 当入口，外层循环识别不出来。最终 17/17 source case 全绿，
flat 预算 goto 1787→1192、real jump 1695→1111、dangling 58→5、unresolved 297→289，
condition TODO 4078→4494（恢复出来的分支要渲染条件，条件 payload 还没恢复就是 TODO，按仓库
约定重锚并在 §6 说明）。

## 1. 回归定位

复现（`20260923-03` §4 的命令，case 002）：

```
cd /sn640/NotDec
llvm-22.1.0.obj/bin/clang -S -emit-llvm -O2 -fno-discard-value-names \
  -Wno-implicit-function-declaration \
  external/NotDec-llvm2c/test/structuring/source-cases/cases/002_loop_break_continue.c -o /tmp/case.ll
NOTDEC_STRUCT_DEBUG=1 build-notdec-nothreads/external/NotDec-llvm2c/bin/notdec-llvm2c \
  /tmp/case.ll -o /tmp/case.c --algo=structured-sailr
```

002 的 CFG：`0 entry` 分支到 `1 while.body`/`3 while.end`；`1` 分支到 `3`/`2 if.end`；
`2` 分支回 `1`/`3`。`RegionIdentifier` 得到 Root region `{0,1,2,3}` 和 NaturalLoop
child region `{1,2}`（follow 块 3）。

gdb 在 `MutableRegionGraph::collapseNodes` 下断点，子 region 的两次 collapse：

```
COLLAPSE n=2 members={0, 1}   # reduceGraphNaturalLoopOnce: {1},{2} → 循环节点 root=13
COLLAPSE n=2 members={3, 2}   # reduceSequenceOnce: 循环节点 + {3} → root=16, blocks={1,2,3}
COLLAPSE n=2 members={1, 0}   # 外层 Root: entry {0} + 子 region {1,2}
```

第二次 collapse 就是回归。member 3 是子 region 图里为 follow 块 3 建的
`ExternalPlaceholder`（`MutableRegionGraph::build(Cfg, Overlay)` 对 `!targetsMember()` 的边走
`getOrCreateExternalPlaceholder`）。`reduceIfOnce` 一直有
`TrueNode->ExternalPlaceholder || FalseNode->ExternalPlaceholder` 的保护，
`reduceSequenceOnce` 没有；`0f3c38f` 之前 `isFallthroughTo` 要求
`Tail->Terminator == Fallthrough`（follow 块是 Return/Unreachable），所以进不来，放宽成
“任意单活后继”之后就进来了。子 region 的 final-overlay 只剩一个节点（`blocks={1,2,3}`），
外层 Root 的子 region 节点（root=16）已经含块 3，而外层还有独立的块 3 节点；
`collapseCrossesNaturalLoopBoundary` 又挡住外层把它们合并，最终装配按 `Blocks.front()` 排序后
两份都渲染 —— 就是 `sink(...); return ...; sink(...); return ...`。

（交接文档把这次放宽记在 `fabd348` 名下；实际“任意单活后继”的放宽是同一会话 2 小时后的
`0f3c38f`，bisect 的父提交 `5355022` 通过、`fabd348` 只在部分 case 上退化。这里按实际情况记录。）

## 2. 修复

1. `reduceSequenceOnce`（`lib/Structuring/PhoenixStructurer.cpp`）：`To->ExternalPlaceholder`
   直接跳过。placeholder 只建模“本 region 之外的出口”，块归父 region 渲染，折进本 region 的根
   必然重复渲染。
2. `OverlayManager::setStructuredRoot`（`lib/Structuring/RegionOverlay.cpp`）：子 region 的
   reducer 通过 `replaceNodes` 已经把进入边重指向 `structured(中间 root, region)`；走
   fallback 装配时 `finalizeRegionMembers` 把父视图的 member 换成
   `structured(最终 root, region)`，旧 key 匹配不上任何 member，父图在
   `MutableRegionGraph::build` 里把这条边当 external node 丢掉。现在 finalize 时把
   `SharedNodeSuccessors` 里所有 `structured(*, 本 region)` 的边重指向最终 root（去重）。
   这一步修好 005/013/017：入口块的两个后继（terminal 臂 + 循环子 region）都留在图里，
   `reduceIfOnce` pattern 5 折成 `if (cond) { return; }` 而不是把 terminal 臂当唯一后继平铺。
3. `MutableRegionGraph` 入口提示（`include/.../MutableRegionGraph.h`、
   `lib/Structuring/MutableRegionGraph.cpp`）：natural loop 的 head 一定被 latch 指向，
   region 图里可能没有任何“无前驱”节点；原来 `firstWithoutActivePred` 找不到就返回最小 id，
   支配分析从错误节点出发，`reduceGraphNaturalLoopOnce` 的 `Analysis.dominates(Head, Latch)`
   失败，外层循环退化成 InfiniteLoop + goto（007）。现在 `build()` 记录 region head 作为 hint，
   只在“没有无前驱节点”时启用；Root region（有真正入口）与合成图保持原行为
   （`testAcyclicDroppedEdges` 依赖这一点）。
4. `foldGuardedLoopSkipIntoSequence`（`PhoenixStructurer.cpp`）：原实现只认“guard 的 If 是某个
   嵌套 Sequence 的最后一个孩子”；loop body 走 fallback 装配时 guard 的 label/body/If 是平铺
   兄弟，于是 007 的 `if (!cmp222) goto 6; <内层循环>; 6:` 留下 goto。补一个直接兄弟形状的分支：
   把 If 之前的兄弟和 If 包进一个 Sequence，再把后面的循环塞进 If 的 Then。
5. 不重复渲染已经折好的 branch（`PhoenixStructurer.cpp`）：`appendFallbackNode` 只在 root 含
   “结构化控制 / control transfer”时早退，而 `nodeTreeContainsStructuredControl` 不含 `If`，
   于是 pattern 5 折出的 If 之后又把 tail 的 branch 渲染一遍（007 出现两份
   `if (outer <= 0) return 0;`）；`buildVirtualizedBranchSource` 在
   `rewriteStructuredSourceTargetTransfer` 找不到显式 transfer 时也会用 `appendSourceBody`
   把已有的根再渲染一遍。新增 `nodeTreeRendersBranchAt`，两处都用它判断“这条 branch 已经画过”。

## 3. 验证

- `structuring-source-cases`：HEAD（对照二进制留在 `/tmp/notdec-llvm2c.head`）8 个失败：
  `loop_break_continue`、`nested_loop_switch`、`guarded_loop_skip`、`loop_two_returns`、
  `tail_early_return`、`loop_switch_shared_latch`、`nested_loop_break_only`、
  `switch_continue_only_latch`（后者的失败是缺 `continue;`，交接文档的清单没列它；而
  `nested_loop_break_continue` 在 HEAD 是通过的，本轮修复过程中一度被 placeholder 修复带出
  goto，最终由 3/4/5 修回）。修复后 `17/17` 通过。
- `structuring-analysis-test`：通过。新增 3 个回归测试：
  `testNaturalLoopKeepsExternalFollow`（002 形状，占位节点不被吸收）、
  `testFinalizedChildKeepsPredecessorEdge`（005 形状，guard 不被平铺）、
  `testGuardedInnerLoopStaysNested`（007 形状，guard 跳过内层循环时嵌套而不是 goto）。
  用 `git stash push -- include lib` 把库改动摘掉后重跑，第一个测试即失败，确认测试不是空转。
- `notdec.evm.solidity_pattern_compile`（103 case + solc）：通过，见 §4。
- 预先存在的失败集合没有变化：`structuring-smoke` 与 `sailr-bench2-migration` 的失败清单与
  HEAD 逐字节相同（`diff` 为空），registry 三测试（`legacy-phoenix-removed` /
  `structured-phoenix-available` / `shared-structurer-registry`）通过。

## 4. 指标（flat 口径，103 case）

| 指标 | HEAD | 本轮 | 预算 |
| --- | --- | --- | --- |
| goto | 1787 | **1192** | max 1192 |
| real jump | 1695 | **1111** | max 1111 |
| dangling goto | 58 | **5** | max 5 |
| unresolved value | 297 | **289** | max 289 |
| condition TODO | 4078 | 4494 | max 4494（重锚，见 §6） |
| helper funcs | 1313 | 1313 | floor 1276 |
| helper call sites | 2442 | 2421 | floor 2408 |
| solc 失败 | 0 | 0 | max 0 |

逐 case 对比（`/tmp/metrics-head.tsv` vs `/tmp/metrics-fixed.tsv`，用套件自己的计数函数）：
condition TODO 的增量集中在 goto 下降的 case（例如 `0189` todo +69 / goto 85→25，
`0679` +37 / 33→3，`0450`、`0038` 各 +26 / 16→0），没有“TODO 暴涨但 goto 不动”的 case。
少数 case goto 略升（`0029` +22、`25596` +11、`24259` +11），都是过去被静默丢掉的 guard/入口
分支现在按真实控制流渲染出来；总数仍远低于预算。

## 5. 遗留

- condition TODO 的 4494 里，条件表达式本身的恢复属于 type-recovery 线（PHI 反 phi 化、bound
  检查等），结构器只能把分支画出来；要继续压这个指标应该在 type recovery 侧做。
- `0029` / `25596` / `24259` 的少量 goto 上升说明这些形状里的新 guard/新入口分支还没被结构器
  吃掉（现在是真实 goto）。可以按同样思路找“已经渲染了 branch 又 virtualize”的剩余组合。
- `20260923-03` §5 的其余两项（type_recovery 快照漂移、sailr-bench2 预先存在失败）未动。

## 6. 预算重锚

按仓库约定（`compile-budget.json` 的 comment）把 5 个 ceiling 下调到新实际值，并把
condition TODO 从 4078 上调到 4494，理由写进 comment 与本文档：每一条新恢复出来的
if/while 都需要一个条件表达式，而恢复出的 guard/latch 条件大多还没被类型恢复解析，
于是原来的“goto / 被丢掉的 guard”变成 `false /* TODO`。goto+real jump 合计 3482→2303，
dangling 58→5，这是同一批改动换来的，重锚后的预算表见上。

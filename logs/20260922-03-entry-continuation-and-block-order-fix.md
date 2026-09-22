# 20260922-03 入口 continuation + collapse 块序修复（P1 遗留根因）

## 0. 一句话

上一轮（`20260922-02`）用"入口块不参与吸收"绕过的 P1 根因已经定位并修好：
不是 `collapseNodesAndSyncOverlay`/`replaceNodes` 丢 StructuredRoot，而是
(1) `MutableRegionGraph::collapseNodes` 按 member id 拼 `Blocks`，合并节点的
`Blocks.front()` 不是它的入口块，最终装配按 front 排序就把入口节点排到了后面；
(2) `dropUnreachableAfterRenderedTransfer` 于是把它当成"无条件跳转之后的死代码"
整体删掉。放宽 `isSequenceContinuation` 到宽规则后重修块序，flat 口径
goto 3640 → **1787**、real jump 3175 → **1695**、cond TODO 4303 → **4078**、
103/103 仍可 solc 编译，5 套 EVM solidity 测试全绿。

## 1. 最小复现与判定

`cases/0032_*.ll` 的 `transferFrom`：入口块 0 折进 terminal 臂后吸收块 2，
再吸收终端的 revert 块 57，collapse 得到 `blocks={57,0,2}`。因为 `Blocks.front()==57`，
最终装配把该节点排在 `block=54` 的节点之后，而后者 root 以 `goto 12` 结尾，
`dropUnreachableAfterRenderedTransfer` 判定"其后是不可进入的死代码"并把
root=206（含入口块 0 的 `uint256 temp_N;` 声明）整体丢弃。

诊断（`NOTDEC_STRUCT_DEBUG=1`，本轮新增 `[struct-region]`/`[struct-regions]`/
`final-overlay`/`root=`/`tblock=` 打印）：

```
[struct-asm ] node=91 root=206   （装配后还在）
[struct-asm2] node=91 root=206 inRoot=1   （dropGotoIntoFollowingNode 之后还在）
[struct-asm3] node=96 root=231 inRoot=0   （cleanupStructuredGotos 之后没了）
[struct-asm3] node=91 root=206 inRoot=0
[struct-drop ] ... dropped child=231 / 206
```

## 2. 修复

1. `MutableRegionGraph::collapseNodes`（`lib/Structuring/MutableRegionGraph.cpp`）：
   先选出"成员集内没有任何成员指向它"的 member 作为入口，把它的 Blocks 放在最前；
   有歧义（0 个或多个，如自环/菱形）时保持原 id 序。
2. `PhoenixStructurer::isSequenceContinuation` 恢复宽规则
   （fallthrough OR `From->Succs == {To}`，无入口块保护）。
3. `firstRenderedBlock`（`PhoenixStructurer.cpp`）补 `While` 分支：
   While 先渲染条件块 `Node->Block` 再渲染 body，这样
   "`goto <header>`; while(...)" 的前置 goto 会被 `dropGotoIntoFollowingNode`
   当成落到 loop header 的 fallthrough 丢掉：dangling 329 → 58。

## 3. 指标（默认 flat 口径，103 case）

| 指标 | 修复前 fabd348 | 宽规则（未修块序） | 本轮 |
| --- | --- | --- | --- |
| goto | 3640 | 2413 | **1787** |
| real jump | 3175 | ~1695 | **1695** |
| dangling goto | 55 | 329 | **58** |
| cond TODO | 4303 | 3942 | **4078** |
| unresolved value | 289 | ? | **297** |
| solc 失败 | 0 | 17 | **0** |
| helper_call_sites | 2408 | 2442 | **2442** |

dangling/unresolved 的小幅上升是"不再丢入口节点"的直接后果：恢复出来的入口块语句
本身带 unresolved bound 检查；另外一些新恢复的 guard-revert 分支在渲染器里被折成
`require()`，其 `// block_N:` label 不再渲染，于是指向它的 goto comment 变成
dangling（`BodyBuilder.cpp` 的 `guardRevertRequire` 折叠）。两者都是渲染器侧的
既有行为，不是 structurer 丢块；预算已按仓库约定重新锚定。

## 4. 遗留

- 渲染器把 guard-revert 折成 `require()` 时会丢掉被折分支的 `// block_N:` 注释，
  指向该块的 goto comment 因此 dangling。若要进一步压 dangling，应在折叠时保留
  comment（会影响 `notdec.evm.solidity_source` 的 golden 文本，需要一并评估）。

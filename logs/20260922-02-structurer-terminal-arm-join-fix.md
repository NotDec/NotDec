# 20260922-02 Phoenix 终止臂 join：structurer 修复与会话结论

## 0. 一句话

扁平形状下 goto 洪峰的主因不是"内联复制"，而是 Phoenix `reduceIfOnce` 把
**终止臂**折进 header 之后，`reduceSequenceOnce` 不认这条"隐式 continuation"，
于是每轮 last-resort 虚化一条边 → goto 级联。修好这条 continuation 判定后，
全语料 flat goto 5437 → **3640**（-33%）、cond_todo 4647 → **4303**、
dangling goto 501 → **55**，103/103 仍能被 solc 编译，5 套 EVM solidity 测试 4/5 绿
（第 5 个最初因为本修复暴露的声明丢失而红，见 §4，已用入口块保护修好）。

## 1. 最小复现（9 块、无内联、无环可归约）

`outlined/0118_*.tr.ll` 的 `public_transferFrom_...`（IR 里 9 块）：

```
0 -> {1, 6}    // calldatasize > 99；6 = revert
1 -> {4, 2}    // sload == MAX；4 = 共享 return
2 -> {5, 3}    // sload < arg2；5 = revert
3 -> 4         // fallthrough 进同一个 4
4 = Return，前驱 {1, 3}   // ← 共享 join
```

修复前渲染 7 个 `// goto block_*`；把 join 块 4 为前驱 3 克隆一份（`clone_join.py`）
立刻变成 0 个 goto。说明失败形状是"then 后继同时是 join"，需要的是
**duplication（拆共享 join）而不是相似度合并**——angr 的 ISD/DuplicationReverter
方向相反，因此没有迁移价值（另见 `logs/20260922-01-...md` §7）。

## 2. 代码级根因

`reduceIfOnce`（`lib/Structuring/PhoenixStructurer.cpp`）的 pattern 4/5 是
"终止臂 + 线性 follow"：

- 折叠后只把 {Header, Then(终止)} 合并，follow 仍是独立节点；
- 合并节点的 `Blocks.back()` 是那个终止块（Return/Unreachable），而它只剩
  一条活边（follow）；
- `reduceSequenceOnce` 用 `isFallthroughTo` 判断能否把 follow 接上，后者要求
  `Blocks.back()` 的 terminator 是 Fallthrough → 永远不成立；
- 于是 analyzeAcyclic 全 false → `lastResortRefinement` 每轮虚化一条边
  （`buildVirtualizedBranchSource`）→ `if (c) { goto X }` + 尾部 goto；
- 父层 header 又因为 join 有多前驱而无法用 pattern 3/6，级联下去。

## 3. 修复

`isSequenceContinuation(Cfg, R, FromId, ToId, Graph)`（新）只用于
`reduceSequenceOnce`：

1. 先走原来的 `isFallthroughTo`；
2. 否则，当 From 只有一条活边且指向 To、且 From 的 tail 是 **Return/Unreachable**
   时也接受（这正是 pattern 4/5 折出来的"if (cond) { terminal }"节点）；
3. 且 From 不是本 region 的入口块（见 §4 的 overlay 说明）。

调用方已经保证 `To->Preds.size() == 1`，所以把 To 接到 From 之后就是那条隐式
continuation，渲染顺序也正确。

同时新增（均为 env 开关，默认零输出）：
- `NOTDEC_STRUCT_DEBUG=1`：`[struct-cfg]` region 图 dump、`[struct-if] pattern/then/else/follow`、
  `[struct-virt]` 边、`[struct-tree]` 最终树；
- `NOTDEC_STRUCTURER=structured-phoenix|structured-goto|sailr`：Solidity 路径选 structurer
  （`--algo` 只作用于 C CLI，之前无法 A/B）。

## 4. 已知遗留：入口块/虚拟化节点的内容会被 overlay 弄丢

把规则放宽到"任何单活边节点"（不只终止 tail）时收益更大（flat goto 5437 → 2413），
但会让 17 个 case 的 `uint256 temp_N;` 声明（挂在**入口块**语句表上，
`BodyBuilder.cpp:2757-2770`）从最终树里消失 → solc `Undeclared identifier`。
即使只保留终止 tail 版本，case 24590 仍复现：

```
[struct-if] pattern=5 header={0} then={1} follow={2}
[struct] reduceSequenceOnce     ← 折叠入口节点后，其内容没有进入最终树
```

最终树直接从 block_15 开始。也就是说 `collapseNodesAndSyncOverlay`
（PhoenixStructurer.cpp:1467-1510）/ `Overlay::replaceNodes` 在某条路径上会丢掉
被合并节点的 StructuredRoot。目前用"入口块不参与该吸收"绕过（`From->Blocks.front() == R.Head`
即拒绝），所有 103 个 case 恢复可编译；**根因未修**，是下一步 P1。

## 5. 指标（默认 flat 口径，103 case）

| 指标 | 修复前 | 修复后 | 预算 |
| --- | --- | --- | --- |
| goto | 5437 | **3640** | 3640 |
| real jump | 4604 | **3175** | 3175 |
| dangling goto | 501 | **55** | 55 |
| condition TODO | 4647 | **4303** | 4303 |
| unresolved value | 289 | 289 | 289 |
| compile failures | 0 | **0** | 0 |
| helper functions | 1304(旧锚) | 1276 | 1276 |

outlined 口径：goto 424 → 233，cond_todo 729 → 658。

重灾 case（flat goto 总数）：0118 194→179（transfer 91→84、transferFrom 88→83）、
0637 163→125（78→63 / 73→61）、0657 160→139、2002 152→128、0029 264→242；
outlined 0118 transferFrom 7 → **0**。
`compile-budget.json` 已按新值收紧，comment 记录了理由。

## 6. 建议的下一步

1. **P1（阻塞项）** 修 `collapseNodesAndSyncOverlay`/`replaceNodes` 丢内容的问题，
   然后把 §3 的第 2 条放宽到"任何单活边节点"（已验证能再降 ~1200 goto，flat 2413）。
   验收：103 case solc 编译 0 失败 + 预算 + 两套 pattern 口径。
2. **P2** `test/structuring/structuring-analysis-test` 目前**编译不过**（Payload 变体 /
   `renderStructuredBody` 返回 Block 的 API 漂移，属既有问题），本会话加的最小复现
   单测（`testTerminalArmJoinStaysStructured`）因此还跑不起来；修好该 target 才能让
   structurer 修复有单测门禁。
3. **P3** SSA/条件侧：cond_todo 仍有 4303，0029 那种"一个 entry 吞掉整份合约"
   （912 块/16 switch）属于内联 scope 问题，与 structurer 正交。
4. **P4** 不建议迁移 angr 的 DuplicationReverter/AILMergeGraph（见
   `logs/20260922-01-outlined-baseline-and-structurer-root-cause.md` §7）。

## 7. 本会话改动清单

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp`
  - `isSequenceContinuation` + `reduceSequenceOnce` 使用它（本次修复）
  - `NOTDEC_STRUCT_DEBUG` 的 cfg/pattern/virt dump
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp`
  - `NOTDEC_STRUCTURER` 选择开关（默认 `sailr`，行为不变）
  - `NOTDEC_STRUCT_DEBUG` 的最终树 dump
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  - 最小复现单测 `testTerminalArmJoinStaysStructured`（当前 target 编译不过，见 §6.2）
- `test/evm/solidity-patterns/compile-budget.json`（收紧）
- 证据目录 `logs/artifacts/20260922-structurer-duplication/`
  （`flat|outlined|frozen|allmode|fix1_*|fix2_*|fix3_*`、`ir_*.py`、`clone_join.py`、
  `clone_terminal_joins.py`、`dump_cfg.py`、`ab_algo.sh`）
- `logs/20260922-01-outlined-baseline-and-structurer-root-cause.md`（上一轮调查）

；`BodyBuilder.cpp` 的开关默认行为与之前一致。

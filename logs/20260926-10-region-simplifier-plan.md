# 20260926-10 规划：RegionSimplifier 层与 IfSimplifier 第一条规则（P1）

> `2026-09-27 复核修正`（本文档随复核一起提交）：
> 1. §2.1 的链顺序改成 angr `region_simplifiers/region_simplifier.py::_simplify()` 的**实际**顺序；
> 2. R1 **不是** angr `if_.py` 的移植（`if_.py` 删的是块尾冗余 ConditionalJump，`goto.py` 删的是
>    "目标 == 顺序后继"的 goto，两者都不做本规则）——R1 是自定规则，层对齐 angr、规则不对齐；
> 3. "移动 T" **不用** `replaceEdge`/`duplicateRegion`/`removeBlock`（那些是 `StructuredCFG` 的
>    CFG 操作），R1 是纯 `StructuredTree` 重挂，CFG 一行不改；
> 4. 取反用已有的 `StructuredNode::ConditionNegated`，不需要在结构侧造 `negateConditionExpr`。
> 复核实测与代码事实见 §5。

## 0. 为什么是这一层

- 实测（0870，36 条 goto）：`ReturnDuplicatorLow` 拒因全是**正常拒因**（134 无 return 区域 / 24 单前驱 /
  4 调用预算，后者与 angr `max_calls_in_regions = 2` 一致），没人拒我们那 36 条；`CrossJumpReverter`
  按 angr 原规则只要"目标出度 = 1"（`SAILRDeoptimization.cpp:4347` 的
  `successorsOf(Target).size() != 1`），而主导形态的目标块开头是 `if`（出度 2）→ 不在候选集。
  2026-09-27 用当前二进制复现：`NOTDEC_DEOPT_DEBUG=1` 输出恰好 134/24/4。
- 所以 goto 只能在**结构阶段**消掉；angr 对应物是 `region_simplifiers/`（`region_simplifier.py` 链 +
  `if_.py`/`cascading_ifs.py`/`goto.py`/`expr_folding.py`/`loop.py`/`switch_*`），
  我们的仓库**没有这一层**（`PhoenixStructurer.cpp` 里 grep 不到对应抽象，也没有
  `RegionSimplifier.{h,cpp}`）。

## 1. 第一目标形态（0870 实测样本）

```solidity
// block_3:
if (!(slot_14[...] == 0)) {
  // goto block_76          ← 分支体只有一个 Goto
}
private__0x21bc_0x21bc(...); // ← T：目标块之前的"共享尾"，被渲染成紧邻的下一条语句
// block_76:                  ← X = goto 的目标标签
```

CFG 事实：`block_3` 在 cond 为假时跳到 `block_76`（跳过 T），为真时走 fallthrough 执行 T 再落到 `block_76`。
等价形态是 `if (cond) { T } block_76:` —— 也就是"**条件跳过一段共享尾**"。

0870 里真实的树形状（`NOTDEC_STRUCT_DEBUG=1`，函数 `public_transferFrom_address_address_uint256__0x4d9`）：

```
Sequence
  Sequence
    Sequence
      Label block=0
      BasicBlock block=0
      If block=0
        Goto target=1          ← Then 是独苗 Goto
    Label block=80             ← T = [Label 80, BasicBlock 80]，注意它在**上一层** Sequence 里
    BasicBlock block=80
  Sequence                     ← 这个兄弟子树的第一个节点就是 Label 1 = X
    Label block=1
    ...
```

即 T 不是 If 的平铺兄弟，而是往上走一层后收集到的后继兄弟；收集算法见 §2.2。

## 2. 设计

### 2.1 骨架（对齐 angr 的层；顺序按 angr 实际实现）

```cpp
// include/notdec-backends/Structuring/RegionSimplifier.h
class RegionSimplifier {
public:
  virtual ~RegionSimplifier() = default;
  virtual const char *name() const = 0;
  // 返回 true 表示树被改动；调用方按 angr 的固定顺序跑链，直到不动点或 max_opt_iters。
  virtual bool simplify(StructuredCFG &Cfg, StructuredTree &Tree,
                        structuring::NodeId Root) = 0;
};

// 链顺序按 angr region_simplifier.py::_simplify() 的实际顺序（逐条移植时按此排队）：
//   remove-empty -> Goto -> If -> Labels -> (ExprFolding) -> Switch* ->
//   IfElse -> CascadingIfs -> Loop -> 转 cascading ifs
// 本轮只实现 If（R1），其余占位。
bool runRegionSimplifiers(StructuredCFG &Cfg, StructuredTree &Tree,
                          structuring::NodeId Root);
```

- 遍历方式：**后序（bottom-up）**走 `StructuredTree`，与 angr `SequenceWalker` 一致；
- 落点：`BodyBuilder::readBody` 里 `Tree = Structurer->structure(Cfg)`（`lib/Solidity/BodyBuilder.cpp:3955`）
  之后调用一次；C 后端的同点是 `lib/notdec-llvm2c/StructuredGoto.cpp:197`（本轮先只接 Solidity，
  C 侧留同一入口，避免动到 C 的 smoke 期望）；
- 调试：`NOTDEC_SIMPLIFY_DEBUG=1` 打印每条规则"命中 / 未命中原因"的计数（与 `NOTDEC_DEOPT_DEBUG` 同风格）。

### 2.2 IfSimplifier R1（本规划的第一条规则）

**前提**（全部用 CFG 可查的事实，不靠文本猜测）：
1. 节点是 `If`，某一分支（Then 或 Else）的子树**只含一个 `Goto X`**（另一个分支为空）；
2. 同序的下一条语句序列 T（从该 If 的位置往上走 Sequence 嵌套、逐层收集后继兄弟，直到某个兄弟
   子树的**首节点**是 `Label X`）是**直线代码**（无循环、无嵌套分支、无其他 Goto）；
3. T 的入口块在 CFG 上**只有这一个 fallthrough 前驱**（`predecessorsOf(T 入口) == {If.Block}`，否则移动会改语义）；
4. T 的最后一个块以终结语句（return/revert/unreachable）结束，或在 CFG 上唯一后继是 X；
5. 规模 guard：T 的语句数/调用数不超过阈值（angr 的成本 guard 思路；本轮用 `IfSimplifierOptions`
   的默认值，move 不需要复制成本，阈值只是风险闸）。

**改写**（纯树重挂，CFG 不动）：把 T **移动**进那个分支（不复制）+ 条件取反 + 删 Goto 与 T 的原副本
→ 输出从 `if (!cond) { goto X } T X:` 变成 `if (cond) { T } X:`。具体：
- Goto 在 `Then`（Else 空）→ `If.ConditionNegated = !If.ConditionNegated`；
- Goto 在 `Else`（Then 空）→ `ConditionNegated` 不变；
- 两种情况都：`If.Then = 新建 Sequence(Children = T 的 NodeId 列表)`、`If.Else = InvalidNodeId`、
  `If.Children.clear()`（渲染器在 `Children` 非空时忽略 `Then/Else`）；
- T 的节点从各自父 Sequence 的 `Children` 里摘掉；旧 Goto 节点变成孤儿留在 `nodes()` 里即可
  （`StructuredTree` 只有 `addNode`，没有删除接口，孤儿不被遍历无副作用）；
- `Label X` 原地保留。

**质量门与原子提交**：`StructuredTree` 是 POD 向量，直接**整棵复制**做候选；改写后
`ControlFlowStructureCounter::collect` 的 goto 总数必须严格下降（且
`improvesRelativeStructuringQuality` 通过），否则丢弃副本。deopt 那套
`StructuringOptimizationPass` 是 CFG 级、要重新 structuring，**不能**直接复用。

**R2（后备，本轮不做）**：T 若被多个前驱共享（≥2 fallthrough），则改为**复制** T 进分支
（成本 guard 同 angr 的 `max_call_duplications`）；那时才可能需要 `StructuredCFG::duplicateRegion`
一类的 CFG 操作与 payload 复制。

## 3. 测试与验证

1. 单测（`structuring_analysis_test.cpp`）：
   - 正例：用合成 CFG + 手搭树复刻 0870 形状（`0 -> {1, 2}`，`2` 为 revert/不可达的 T，`1` 是 goto 目标），
     直接调 `runRegionSimplifiers`，断言：可达 Goto 数为 0、T 进入 `If.Then`、`ConditionNegated` 翻转、
     `Label X` 仍在；
   - 负例：同上但给 T 的入口块加第二个 CFG 前驱，断言树逐字段不变（原子不改）。
   - 注意：单测里结构器是直接 `PhoenixStructurer().structure(Cfg)`，不会走 `BodyBuilder::readBody`，
     所以单测要**直接调 simplifier**（而不是指望 hook）。
2. 集成：`structuring-analysis`、`structuring-source-cases`、`structuring-smoke`（先与基线 diff，
   基线失败清单见 §5.4）、EVM 四套 + compile-budget。
3. 定点：0870/0340/0637 期望 36 条 goto 明显下降（这三例的共享尾形态最干净）；0029/0258 观察。

## 4. 顺序与风险

顺序（与 angr 实际顺序一致）：**GotoSimplifier → IfSimplifier(R1) → RedundantLabel → ExprFolding →
Switch* → IfElseFlattener → CascadingIfs → LoopSimplifier**。本轮只落 R1 与运行框架。

风险：
- 移动会改变块身份与 payload 归属 → 必须走"复制树 + 质量门 + 整笔提交"，不能就地改主树；
- 前提 2/3 判断错会把"非共享尾"也搬进分支 → 所以前提必须全部来自 CFG 事实，且单测要有负例；
- 与 `ReturnDuplicatorLow` 可能争抢同一形态（它也在复制 return 区域）→ 先量"谁先命中"，必要时按 angr preset
  的顺序（ReturnDuplicator 在 CrossJumpReverter 之前）保持我们的 pipeline 顺序不变；
- 规则是自定的，可能让输出偏离 angr 的对照语义 → 每次都要用 compile-budget 棘轮和 source-cases 兜底。

## 5. 实现前核实（2026-09-27 复核，全部在当前代码/二进制上验证）

### 5.1 落点与 API

- 落点：`lib/Solidity/BodyBuilder.cpp:3955`（`Tree = Structurer->structure(Cfg)` 之后、
  `[struct-tree]` dump 与 `renderStructuredBody` 之前）；C 侧同点 `lib/notdec-llvm2c/StructuredGoto.cpp:197`。
- `StructuredTree`（`include/notdec-backends/Structuring/StructuredCFG.h:419`）只有
  `addNode`/`setRoot`/`nodes()`/`getNode`，没有删除；`StructuredNode`（同文件 :392）是 POD，
  子节点用 `NodeId`（`Children`/`Then`/`Else`/`Body`/`Default`/`StructuredCases`）。
- `ConditionNegated`（同文件 :399）已存在且渲染侧生效（`BodyBuilder.cpp:517` `conditionExpr`）。
- 质量门可复用：`ControlFlowStructureCounter::collect`（`StructuringQuality.h`，goto 计数 +
  `normalizeGotoLabels`）、`improvesRelativeStructuringQuality`；goto 枚举可复用
  `GotoManager::collect`（带 source/target/node）。

### 5.2 0870 实测（当前二进制 `build-notdec-nothreads/bin/notdec`，09-27 12:08）

- `0870_...ll --tr-level=2` 渲染出 **36** 条 `// goto block_`（与规划一致）。
- `NOTDEC_DEOPT_DEBUG=1`：`ReturnDuplicatorLow` 134 no-return-region / 24 single-predecessor / 4 region-budget。
- 树 dump 确认 §1 的三层 Sequence 嵌套形状。

### 5.3 度量口径

goto 数由 `test/run_evm_solidity_pattern_compile_suite.py::count_goto_kinds` 在**渲染后的 Solidity**
上统计（`GOTO_RE = // goto block_(\d+)`，同函数内解析 label，分 total/fallthrough/dangling/real）；
Solidity 渲染器里 `Goto`/`Label` 都只是注释（`BodyBuilder.cpp:807/892`）。

### 5.4 基线

- `structuring-analysis` 单测全绿；最近一次全量 ctest（09-27 12:06）唯一红的是 `structuring-smoke`。
- `structuring-smoke` 基线失败 30 行，已存 `/tmp/smoke-baseline.log`（`sailr_angr_dephication_*`、
  `sailr_*diamond_return_*`、`phi_demote_*` 的 C 侧期望漂移 + 1 个缺外部 fixture 的 skip）。
- 注意：ctest 用的 standalone `notdec-llvm2c` 此前是 09-26 的旧二进制（拓扑上已落后于 S1' 提交），
  已重建；以后基线一律用重建后的二进制。
- 未推提交：顶层 53、子模块 27；`external/binarysub`（脏）与 `build-notdec-nothreads2` 属其他会话，不要动。

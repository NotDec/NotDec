# 20260926-10 规划：RegionSimplifier 层与 IfSimplifier 第一条规则（P1）

## 0. 为什么是这一层

- 实测（0870，36 条 goto）：`ReturnDuplicatorLow` 拒因全是**正常拒因**（134 无 return 区域 / 24 单前驱 /
  4 调用预算，后者与 angr `max_calls_in_regions = 2` 一致），没人拒我们那 36 条；`CrossJumpReverter`
  按 angr 原规则只要"目标出度 = 1"，而主导形态的目标块开头是 `if`（出度 2）→ 不在候选集。
- 所以 goto 只能在**结构阶段**消掉；angr 对应物是 `region_simplifiers/`（`region_simplifier.py` 链 +
  `if_.py`/`cascading_ifs.py`/`goto.py`/`expr_folding.py`/`loop.py`/`switch_*`），
  我们的仓库**没有这一层**（`PhoenixStructurer.cpp` 里 grep 不到对应抽象）。

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

## 2. 设计

### 2.1 骨架（对齐 angr `region_simplifier.py`）

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

// 链顺序按 angr：IfSimplifier -> CascadingIfs -> GotoSimplifier ->
// ExprFolding -> Loop -> Switch*（本规划先只实现前两个所需的框架）。
bool runRegionSimplifiers(StructuredCFG &Cfg, StructuredTree &Tree,
                          structuring::NodeId Root);
```

- 遍历方式：**后序（bottom-up）**走 `StructuredTree`，与 angr `SequenceWalker` 一致；
- 落点：`BodyBuilder::readBody` 里 `Tree = Structurer->structure(Cfg)` 之后调用一次
  （C 后端同点对齐），这样 C/Solidity 共用、且不动 `Phoenix` 的内部判定；
- 调试：`NOTDEC_SIMPLIFY_DEBUG=1` 打印每条规则"命中 / 未命中原因"的计数（与 `NOTDEC_DEOPT_DEBUG` 同风格）。

### 2.2 IfSimplifier R1（本规划的第一条规则）

**前提**（全部用 CFG 可查的事实，不靠文本猜测）：
1. 节点是 `If`，某一分支（Then 或 Else）的子树**只含一个 `Goto X`**；
2. 同序的下一条语句序列 T（从该 If 的兄弟位置开始，直到 `Label X` 之前）是**直线代码**
   （无循环、无嵌套分支、无其他 Goto）；
3. T 的入口块在 CFG 上**只有这一个 fallthrough 前驱**（否则移动会改语义）；
4. T 以终结语句（return/revert/unreachable）或"落到 X"结束；
5. 规模 guard：T 的语句数/调用数不超过阈值（angr 的成本 guard 思路，默认沿用现有 options 字段）。

**改写**：把 T **移动**进那个分支（不复制），条件取反（`negateConditionExpr` 在渲染侧已有等价物），
删掉 `Goto`，删掉原位置的 T 副本，保留 `Label X`。
→ 输出从 `if (!cond) { goto X } T X:` 变成 `if (cond) { T } X:`。

**必须有质量门**：改写后 `goto` 数必须严格下降（沿用 `StrictlyLessGotos`/`StructuringQuality`），
且失败时原子回滚（复用 `StructuredCFG` 的候选图提交约定，与 `DuplicationReverter`/`CrossJumpReverter` 一致）。

**R2（后备，本轮不做）**：T 若被多个前驱共享（≥2 fallthrough），则改为**复制** T 进分支（成本 guard 同 angr
的 `max_call_duplications`），对应 angr `ReturnDuplicator` 的那一半职责。

## 3. 测试与验证

1. 单测（`structuring_analysis_test.cpp`）：合成 CFG 复刻 0870 形状
   （`0 -> {1, 3}`，`1: T(单语句) -> 3`，`3` 为标签/goto 目标），断言 R1 后树里不再有该 Goto、T 进入分支、
   `goto` 数下降；再加一个"T 有第二个前驱"的负例，断言不动。
2. 集成：`structuring-analysis`、`structuring-source-cases`、`structuring-smoke`（先与基线 diff 失败清单）、
   EVM 四套 + compile-budget。
3. 定点：0870/0340/0637 期望 36 条 goto 明显下降（这三例的共享尾形态最干净）；0029/0258 观察。

## 4. 顺序与风险

顺序：**IfSimplifier R1 → CascadingIfs → GotoSimplifier（纯清理）→ ExprFolding → Loop → Switch\***。

风险：
- 移动/复制会改变块身份与 payload 归属 → 必须走候选图 + 原子提交，不能就地改主图；
- R1 的前提 2/3 判断错会把"非共享尾"也搬进分支 → 所以前提必须全部来自 CFG 事实，且单测要有负例；
- 与 `ReturnDuplicatorLow` 可能争抢同一形态（它也在复制 return 区域）→ 先量"谁先命中"，必要时按 angr preset
  的顺序（ReturnDuplicator 在 CrossJumpReverter 之前）保持我们的 pipeline 顺序不变。

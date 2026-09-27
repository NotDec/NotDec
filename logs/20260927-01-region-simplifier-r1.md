# 20260927-01 RegionSimplifier 层落地：R1（条件跳过共享尾）

## 0. 一句话

`20260926-10` 规划的 S4 第一刀落地：新增结构后处理层 `RegionSimplifier`（angr `region_simplifiers/` 的对应物），
第一条规则 R1 把 `if (!cond) { goto X } T X:` 改写成 `if (cond) { T } X:`（T 移动、条件取反、goto 删除）。
全语料 103 case：goto 1218 → **1041**、real jump 1136 → **959**，其余计数器（condition TODO 2089、unresolved 97、
switch 67、recover-body 95、event-signature 515、helper 1943/5038、compile failures 0）**原地不动**，预算已按约定下调。
定点 0870/0340/0637：36→32 / 36→32 / 36→30。新增 3 条单测全绿。

## 1. 实现

| 文件 | 内容 |
| --- | --- |
| `include/notdec-backends/Structuring/RegionSimplifier.h` | `RegionSimplifier` 接口 + `runRegionSimplifiers` |
| `lib/Structuring/RegionSimplifier.cpp` | 链 + 不动点 + 候选树/质量门；R1 本体 |
| `lib/Solidity/BodyBuilder.cpp:3955` | `Structurer->structure(Cfg)` 之后调用（**C 路径本轮未接**） |
| `test/structuring/structuring_analysis_test.cpp` | 3 条单测：Then 里的 goto、Else 里的 goto、共享尾负例 |

- 事务：`StructuredTree` 是 POD 向量，直接整棵复制当候选；改写后要求 `ControlFlowStructureCounter`
  的 goto 总数严格下降且 `improvesRelativeStructuringQuality` 通过，否则整笔丢弃。deopt 那套
  `StructuringOptimizationPass`（CFG 级、要重新 structuring）**不能**复用。
- R1 前提（全部 CFG 事实）：If 的一个分支子树只含一个 `Goto X`、另一分支为空；从 If 往上走 Sequence 嵌套、
  逐层收集后继兄弟，直到出现以 `Label X` 开头的子树，得到的 T 必须是直线代码；T 的入口块唯一前驱是这个 If
  （`predecessorsOf(Entry) == {If.Block}`，且 If 的 CFG 后继恰好是 `{Entry, X}`）；T 末块终结、或唯一后继是 X；
  语句 ≤128、调用 ≤16。
- 取反：Goto 在 `Then` → 翻 `ConditionNegated`；在 `Else` → 不翻；两种情况都 `Then = Sequence(T)`、
  清 `Else`/`Children`。旧 Goto 变孤儿（`StructuredTree` 没有删除接口，孤儿不被遍历即无副作用）。
- 诊断：`NOTDEC_SIMPLIFY_DEBUG=1` 打印命中/未命中计数（**按节点去重**，否则不动点重扫会重复计数）；
  `NOTDEC_SIMPLIFY=0` 关掉整条链，用于 A/B 测基线。

## 2. 效果

### 2.1 定点（整文件渲染输出，`// goto block_` 计数）

| case | R1 前 | R1 后 |
| --- | --- | --- |
| 0870 | 36 | 32 |
| 0340 | 36 | 32 |
| 0637 | 36 | 30 |

R1 不只是删 goto：尾部是 revert 时渲染侧会顺势折成 `require`。0870 里
`if (msg.data.length > 99) { // goto block_1 } // block_80: revert(); // block_1:` 变成
`require(msg.data.length > 99);`；另一处 `if (!(x == 0)) { goto } … revert()` 变成 `if (x == 0) { revert(); }`。

### 2.2 全语料（`notdec.evm.solidity_pattern_compile`，103 case）

| 指标 | 旧预算 | 现在 | 
| --- | --- | --- |
| goto_occurrences | 1218 | **1041** |
| real_jump_occurrences | 1136 | **959** |
| dangling / fallthrough | 6 / — | 6 / 76 |
| condition_todo | 2089 | 2089 |
| unresolved | 97 | 97 |
| switch_todo / recover_body / event_signature | 67 / 95 / 515 | 67 / 95 / 515 |
| helper functions / call sites | 1943 / 5038 | 1943 / 5038 |
| compile failures | 0 | 0 |

`compile-budget.json` 已把 goto/real jump 下调到 1041/959，并在 comment 里记了这次下调。

### 2.3 测试

- `structuring-analysis`（含 3 条新单测）：绿。
- `structuring-source-cases`：绿。
- `structuring-smoke`：仍是原来那 30 行 C 侧期望漂移，与基线**逐行一致**（C 路径没接，符合预期）。
- EVM：`solidity_pattern_compile` / `solidity_rewrite` / `solidity_source` 绿；
  `solidity_patterns` 挂在 `24259` 的 `abi_return` 标记 floor（实际 8/9 < 期望 10）。**与 R1 无关**：
  该模式（`-o out.ll --tr-level=2`）根本不跑结构器（`[struct-fn]`/`[simplify]`/`[deopt-reject]` 全是 0 行），
  且 `NOTDEC_SIMPLIFY=0/1` 各跑 3 次输出 md5 完全相同（`3bf0d33b`）。这是 09-26/09-27 EVM escape/标记变更之后的
  旧 floor（同一 case 上一轮的 /tmp 产物是 abi_return=11/revert=69，现在是 9/89），需要单独重锚或修行为。

## 3. 还没吃到的形态（下一步）

0870 的去重诊断：候选 If-goto 分支 **32** 个 = missing-tail **12**（goto 目标的 label 不在祖先兄弟链上，
典型是"跳过大段代码"的 guard，不是共享尾）+ nonlinear-tail **16**（尾部含分支/循环/第二个 goto）+
命中 **4**；另有 4 条 goto 根本不在 If 分支里（顺序 Goto 等），本轮规则碰不到。
即 R1 只吃"短直线尾"。按 angr 顺序的下一步：GotoSimplifier（目标 == 顺序后继）、ExprFolding（同时压
condition TODO）、CascadingIfs、Loop；R2（多前驱时改成复制 T）留给共享尾形态。

## 4. 环境与复现

```sh
cd /sn640/NotDec
ninja -C build-notdec-nothreads notdec notdec-llvm2c-exe structuring-analysis-test
# 定点（GOTO_RE = // goto block_(d+)）
build-notdec-nothreads/bin/notdec test/evm/solidity-patterns/cases/0870_*.ll -o /tmp/0870.sol --tr-level=2
NOTDEC_SIMPLIFY=0 ... # 同一二进制测基线
NOTDEC_SIMPLIFY_DEBUG=1 ... # 命中/未命中计数
# 全语料棘轮（约 24 min，主要耗在 0029 的 solc --via-ir）
ctest --test-dir build-notdec-nothreads -R notdec.evm.solidity_pattern_compile
```

C 后端的同一落点是 `lib/notdec-llvm2c/StructuredGoto.cpp:197`；接之前要先处理 smoke 的期望漂移。

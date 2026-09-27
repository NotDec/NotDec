# 20260926-09 规划：按 angr 对齐 goto 消除（判等语义优先，P0）

## 0. 背景与结论（为什么是这个方向）

- 现象：EVM corpus `goto 1218 / real jump 1136`，集中在 0258(78) / 24601(72) / 25928(72) / 0657(42)，
  以及一批**恰好 36 条**的 case（0870/0340/0637…，它们的 condition TODO 只有 6~8）。
- angr 对这件事的做法（`/sn640/angr/angr/analyses/decompiler/optimization_passes/cross_jump_reverter.py`）：
  **找到 goto → 目标是单后继（线性）块 → 复制到各前驱**，启发式 `max_opt_iters=3`、
  `max_call_duplications=1`、`strictly_less_gotos=True`，阶段 `DURING_REGION_IDENTIFICATION`。
- **我们已经移植并在跑**：`CrossJumpReverter`（`lib/Structuring/SAILRDeoptimization.cpp:4285/4292`，
  默认选项 `MaxOptIters=3`/`StrictlyLessGotos=true`/`MaxDuplicatedCalls=1`，含连通前驱分组、
  switch case/default 边身份、原子提交；单测 `structuring_analysis_test.cpp:4153-4760`），
  默认结构器为 `sailr`（`DefaultStructurerName`），`SAILRStructurer.cpp:113` 构建整条 deopt pipeline。
  ⇒ **goto 消不掉的原因不是缺复制机制**，不要再加平行机制。
- 仓库自己的 angr 对照清单（`logs/20260920-03-sailr-angr-pass-parity-inventory.md`）给出的最大缺口是
  **判等语义**：`DuplicationReverter` 用的是结构/指针级精确判等（`samePayload`/`samePayloads`，
  `SAILRDeoptimization.cpp:19-30`，判定点 `:1267`），而 angr 是"**归一化后的结构相似度 + merge graph**"。
  该清单的原话：*重复的 guard 副本"语义相同、SSA/折叠形式不同"，是 goto/TODO 的最大来源*。
- 与实测吻合：0870 的共享目标 `block_32` 分别被 `if (slot_11 == 0)` 与 `if (!(slot_31[...] == 0))` 跳过去
  （两个 entry 的近似副本）；0029 的 68 条 goto **全部恰好 ×2**，即同一段 body 两份副本。

## 1. 参考版本修正（先说清，避免照旧名字找文件）

清单里写的 `structured_node.py: ExpressionNormalizer` / `structured_node_comparison` 属于**旧版 angr**，
`/sn640/angr` 这份 checkout 中已不存在。当前对应物：

| 用途 | 本 checkout 的位置 |
| --- | --- |
| 相似度（判等的定义） | `optimization_passes/duplication_reverter/similarity.py`：`ail_graph_similarity(block0, block1, graph)`、`longest_ail_graph_subseq`、`ail_similarity_to_orig_blocks`、`find_block_by_similarity` |
| 合并图 | `optimization_passes/duplication_reverter/ail_merge_graph.py`：`AILMergeGraph.create_conditionless_graph` / `create_mapping_to_merge_graph` / `AILBlockSplit` |
| 块级相似度 | `decompiler/block_similarity.py` |
| SSA/vvar 重命名 | `decompiler/dephication/` |
| 常量折叠/规范化 | `decompiler/peephole_optimizations/` |

本规划以**这份 checkout** 为准；实现前先读上述文件，把规则逐条抄成清单再动手。

## 2. 步骤

### S1（前置，本轮起点）判等归一化
目标：让"语义相同、写法不同"的两份 payload 判等成功。规则候选（**待与 angr 逐条核对后定稿**）：
1. vvar/SSA 重命名：跨副本把 vvar id 归一（angr `dephication/`）；
2. 常量折叠与常量规范化（angr `peephole_optimizations/`）；
3. 交换律操作数排序（add/mul/and/or/xor/eq/ne）；
4. 比较极性归一（`a < b` ↔ `b > a`，`!` 下沉）。
落点：`samePayload`/`samePayloads` 之前插入一层归一化（或在归一化后的视图上比较），
保证 `DuplicationReverter`（`:1267`）与其它 `samePayload` 调用点行为一致、可回滚。
验收：`structuring_analysis_test`（含新增归一化单测）+ `run_structuring_smoke` 不回归；
EVM 侧 real jump / goto 下降，condition TODO 不上升。

### S2 结构相似度
移植 `ail_graph_similarity` + `longest_ail_graph_subseq`（带图上下文，而不只是块内语句），
替换 `DuplicationReverter` 的精确判等；保留"严格减少 goto"的质量门（`StructuringQuality`）。
验收：同上 + 至少一个定点 case（0870/0340/0637 之一）real jump 下降可复现。

### S3 merge graph
移植 `AILMergeGraph`（`create_conditionless_graph` / `create_mapping_to_merge_graph` / split 语义），
把"等价子图合并"做成一次提交（沿用现有的候选图+原子提交约定）。

### S4（P1）region_simplifiers
逐项移植 `region_simplifiers/`，优先 `if_.py`、`cascading_ifs.py`、`expr_folding.py`
（同时直接压 condition TODO），再 `goto.py`/`ifelse.py`/`loop.py`/`switch_*.py`。

## 3. 门禁与素材

- 每次改动必过：`structuring_analysis_test`、`run_structuring_smoke.py`、
  `notdec.evm.solidity_patterns|source|rewrite|pattern_compile`（compile-budget 棘轮）。
- 指标口径：`goto/real jump` 必须严格下降（angr 的 `strictly_less_gotos` 精神），
  condition TODO / unresolved / recover_body / switch 不得上升。
- 定点素材：0870 / 0340 / 0637（36 jump、TODO≈6，形态最干净）；0029（68 jump、全 ×2）；
  0258 / 24601 / 25928（72~78）；inventory 指定的 0009 / 1485。
- 复现命令：`NOTDEC_STRUCT_DEBUG=1 notdec <case> -o /tmp/x.sol --tr-level=2`，
  配合 `/tmp/goto_targets_926.py`、`/tmp/jump_kinds_926.py`（本轮新增的分类脚本）。

## 4. 风险

- 归一化改变判等 ⇒ 可能让 `DuplicationReverter` 复制/合并更多，需防代码膨胀：
  沿用 angr 的成本 guard（调用数、max_opt_iters）并补"严格减少 goto"检查。
- `samePayload` 被多处使用（`:1267`、`:2008`、`:44`、`:108`…），改动要按调用点评估，
  必要时只对 `DuplicationReverter` 启用归一化视图。
- 旧 inventory 的名字与本 checkout 不一致，实现前必须核对，避免按过时语义实现。

## 4.5 实现前核实（S1 开动的第一刀）：我们的判等在 LLVM origin 层，不在 payload 结构层

读代码后的决定性事实：`samePayload` 是

```cpp
bool samePayload(const StructuredCFG &Graph, PayloadRef Lhs, PayloadRef Rhs) {
  return Graph.payloadOrigin(Lhs.Id) == Graph.payloadOrigin(Rhs.Id);   // SAILRDeoptimization.cpp:19-21
}
```

即**比较 payload 的来源 id**。来源是**另一个 payload id**（`StructuredCFG.cpp:302-308` 的
`PayloadOrigins` 映射：copy 出来的 payload 用 `setPayloadOrigin(newId, srcId)` 指回它复制自的那个
原始 payload），所以这个判等的语义是"**这两个 payload 是不是同一个原始渲染点的副本**"，而不是内容比较。
换句话说：今天没有任何"内容"层面的判等可供归一化，缺的是**内容/结构键**。而 payload 的实际 AST
（`BodyBuilder` 的 `Payload = variant<Statement, Expression>`）由**渲染器**持有，structuring 层只存
`PayloadRef` + origin 表（`StructuredCFG.h:279`）。angr 那边相反：deopt pass 直接比较 **AIL**，
也就是结构器自己的 IR，所以 `ail_graph_similarity` 才可能成立。

结论（修正 S1/S2 的顺序）：

- **S1' 的第一选择（LLVM 侧结构键，推荐）**：payload 与 LLVM 值是 1:1 的（渲染器就是按那个值渲染的），
  所以可以直接在 LLVM 侧算一个**递归结构键**：值 → (kind/opcode, 归一化后的操作数键)，对
  add/mul/and/or/xor/eq 做交换律归一、常量操作数折叠，**忽略 SSA 名字**，按函数 memoize；
  渲染器（payload provider）本来就在那个点上持有 `llvm::Value`，把键随 payload 一起交给 CFG 即可，
  `samePayload` 在有键时比键。这条路**不动 AST、不动结构器算法**，约 150~250 行 + 键的传递与单测。
  LLVM 里可参考/复用的现成物：`llvm/IR/StructuralHash.h`（忽略名字的递归结构哈希，API 是函数/模块级）、
  `llvm/Transforms/Utils/FunctionComparator.h`（MergeFunctions 的"带 Value→Value 映射走 def-DAG"实现，
  正是 α 等价的经典写法，但不是稳定的公开 API）；`Instruction::isIdenticalTo/isSameOperationAs` 只比
  操作数指针，跨副本无用。注意这是**代理判等**（比的是渲染前的 IR 结构，不是最终文本），angr 比 AIL 同理。
- **S1' 的第二选择（AST 侧结构键，必要时）**：只有当代理解等不够时（例如不同 LLVM 值渲染出同一高层形态，
  或同一值被不同 matcher 渲染成不同形态）才做：给 structuring 层一个**语义判等钩子**——渲染器按 payload 计算"归一化后的
  结构键"（或直接提供 `samePayloadSemantics(Lhs, Rhs)`），deopt pass 通过它比较，而不是比 origin。
  两种形态：(a) 回调比较；(b) 归一化结构哈希（更接近 angr 的 similarity，且给 S2 的 LCS 复用）。
- **S1 归一化规则**照原计划（vvar/临时名 alpha 重命名、常量折叠、交换律、比较极性、去冗余
  mask/not），但**作用在结构键的构造里**，不是作用在 origin 上。
- **S2** 的 `ail_graph_similarity`/`longest_ail_graph_subseq` 移植随后进行，输入换成结构键 + 图上下文。

之所以必须先做 S1'：在 origin 判等下，无论加多少归一化，两份"语义相同、来源不同"的 payload
都不可能判等，改动不会产生任何效果（这也是为什么现有 `DuplicationReverter` 只能吃 exact-match 子集）。

## 5. 本轮状态

只落规划文件，未改代码（S1 的第一个动作是读 `dephication/` 与 `peephole_optimizations/` 并定稿规则清单）。

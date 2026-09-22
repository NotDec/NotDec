# SAILR/Phoenix pass 覆盖度盘点（angr vs 我们）

参考源码：`/sn640/angr/angr/analyses/decompiler/`（structuring/ + optimization_passes/ + region_simplifiers/）。
我们的实现：`external/NotDec-llvm2c/lib/Structuring/`（18 文件 14287 行，deopt 集中在 `SAILRDeoptimization.cpp`）。
默认 structurer：`StructurerRegistry.h:13 DefaultStructurerName = "sailr"`（= Phoenix + 下面这 7 个 pass）。

## 1. 我们已经注册的 deopt pass（`SAILRDeoptimization.cpp:4461 buildSAILRDeoptimizationPipeline`）

1. SwitchDefaultCaseDuplicator
2. SwitchReusedEntryRewriter
3. DuplicationReverter
4. LoweredSwitchSimplifier
5. ReturnDuplicatorLow
6. ReturnDeduplicator
7. CrossJumpReverter

## 2. angr 有、我们没有（或明显更窄）的

按对当前症状（内联后重复 guard → goto 5437 / TODO 4647）的价值排序：

| 优先级 | angr 模块 | 作用 | 我们的状态 |
| --- | --- | --- | --- |
| **P0** | `optimization_passes/duplication_reverter/`（**整包**：`ail_merge_graph.py`、`similarity.py`、`utils.py`、`duplication_reverter.py`） | 在 AIL 上建 **merge graph**，用 **structural similarity** 判等后合并重复子图 | 只有单个 `DuplicationReverter`，比较是结构/指针级，**没有 similarity / merge graph** |
| **P0** | `structured_node.py: ExpressionNormalizer` / `structured_node_comparison` | 判等前做表达式规范化（SSA 重命名、常量折叠、交换律、比较极性归一） | 无等价物（grep 不到） |
| **P1** | `optimization_passes/cross_jump_reverter.py` | 跨跳转归并（完整候选集） | 有 `CrossJumpReverter`，需逐条对照是否只做了子集 |
| **P1** | `region_simplifiers/`（`if_.py`、`cascading_ifs.py`、`expr_folding.py`、`goto.py`、`ifelse.py`、`loop.py`、`switch_*.py`） | Phoenix 阶段的 region 级化简（if/loop/switch 折叠、表达式折叠） | 只有 `StructuringOptimizationPass`/`RegionOverlay`，无逐项对应 |
| **P2** | `condition_constprop.py`、`flip_boolean_cmp.py`、`expr_op_swapper.py`、`peephole_simplifier.py` | 条件常量传播、比较极性翻转、操作数交换 | 无；今天 matcher 侧手工认的折叠形式正是这一类 |
| **P2** | `ite_region_converter.py`、`ite_expr_converter.py`、`code_motion.py`、`deadblock_remover.py` | ITE 区域/表达式转换、代码外提、死块清理 | 部分可能有；待逐项确认 |
| **P3** | `ret_deduplicator.py`/`return_duplicator_*.py`、`switch_default_case_duplicator.py` 等 | 返回/switch 去重 | 已有 ReturnDeduplicator/ReturnDuplicatorLow/SwitchDefaultCaseDuplicator，属已覆盖 |
| **P3** | 架构相关（`mips_gp_*`、`x86_gcc_getpc`、`*canary*`、`register_save_area*`、`ret_addr_save`、`static_vvar_rewriter`）、字符串内建（`inlined_str*/mem*/wcscpy/eager_std_string*`） | 平台/库特化 | EVM 无对应语义，**不需要** |

## 3. 结论与推进顺序

1. **P0：移植 `ExpressionNormalizer` + `similarity` + `ail_merge_graph` 的判定语义**到我们的 `DuplicationReverter`（判等前归一化：SSA 重命名、常量折叠、交换律、比较极性）。当前重复的 guard 副本正是"语义相同、SSA/折叠形式不同"，这是 goto/TODO 的最大来源。
2. **P1：逐条对照 `cross_jump_reverter.py` 与我们的 `CrossJumpReverter`**，补齐候选集与判等。
3. **P1：逐项对照 `region_simplifiers/`**，优先 `if_.py`/`cascading_ifs.py`/`expr_folding.py`（`condition_todo` 直接受益）。
4. **P2：`condition_constprop`/`flip_boolean_cmp`**（与 matcher 的折叠形式识别同源）。
5. 每补一块立刻用 `StructuringQuality`/`StructuringEvaluator` + `compile-budget.json`（goto 5437 / TODO 4647）回归，并保持 `notdec.evm.solidity_patterns` 与 `..._patterns_outlined` 双绿。
6. 最小复现素材：0009 / 1485 的 `--emit-tr-input-ir` 输出（同一 guard 复制 3~5 份、常量折叠不同）。

## 4. 附：angr 侧需要读的文件（按序）

```
/sn640/angr/angr/analyses/decompiler/optimization_passes/duplication_reverter/{duplication_reverter,similarity,ail_merge_graph,utils}.py
/sn640/angr/angr/analyses/decompiler/structured_node.py            (ExpressionNormalizer, structured_node_comparison)
/sn640/angr/angr/analyses/decompiler/optimization_passes/cross_jump_reverter.py
/sn640/angr/angr/analyses/decompiler/region_simplifiers/{if_,cascading_ifs,expr_folding,goto,ifelse,loop}.py
/sn640/angr/angr/analyses/decompiler/optimization_passes/{condition_constprop,flip_boolean_cmp,expr_op_swapper}.py
/sn640/angr/angr/analyses/decompiler/structuring/{phoenix,sailr,recursive_structurer}.py
```

## 5. P0 迁移源图（angr → 我们）

angr 的"相似度判等"不在 `duplication_reverter.py` 里，而在两个被它 import 的模块：

| angr 文件 | 关键函数 | 迁移目标 |
| --- | --- | --- |
| `analyses/decompiler/block_similarity.py` | `is_similar`、`longest_ail_subseq` | 判等核心：把两个 block/语句序列按"AIL 结构等价"比较（表达式归一化后的结构比较），移植到我们对 payload/CFG 的等价判断 |
| `optimization_passes/duplication_reverter/similarity.py` | `ail_graph_similarity`、`longest_ail_graph_subseq`、`bfs_list_blocks` | 图级相似度：BFS 展开两个候选子图、按索引配对、discontinuity 终止、两个 special case（共用后继/前驱） |
| `optimization_passes/duplication_reverter/ail_merge_graph.py` | `AILMergeGraph` | 合并子图的构造与再插入 |
| `optimization_passes/duplication_reverter/duplication_reverter.py` | `_search_for_deduplication_candidate`、`_construct_merged_candidate`、`_reinsert_merged_candidate`、`maximize_similarity_of_blocks`、`_construct_best_condition_block_for_merge`、`similar_conditional_when_single_corrected`、`_correct_all_broken_jumps`、`_find_future_irreducible_gotos`、`collect_conditions_between_nodes` | 我们的 `DuplicationReverter` 要按这些方法逐个对齐（目前只有 7-pass 流水线里的一个窄实现） |
| `structured_node.py` | `ExpressionNormalizer`、`structured_node_comparison` | 表达式/结构比较的归一化基础，被 `block_similarity` 依赖 |

**迁移顺序**：`block_similarity.is_similar` → `similarity.ail_graph_similarity/longest_ail_graph_subseq` → `AILMergeGraph` → `DuplicationReverter` 的方法族。

## 6. 动手前要和用户确认的两个"表示层"问题（非 angr 逻辑，属于移植桥接）

1. angr 的比较建立在 **AIL**（`ailment.Block`/`Expression`）上，我们的 structurer 建立在 **LLVM IR + StructuredCFG/PayloadRef** 上。需要确认：是"按 angr 的语义在我们的表示上重写等价的比较函数"（推荐，且不引入新逻辑），还是要先把 payload 降到 AIL 式中间表示（工作量大）。
2. angr 的 `DuplicationReverter` 是 `StructuringOptimizationPass`，跑在 region identification 之后的 AIL 图上；我们的同阶段是 `buildSAILRDeoptimizationPipeline` 的 7 个 pass。需要确认我们的 `DuplicationReverter` 输入图与 angr 的 `self._graph` 语义对齐（有 goto 的 region 图），否则相似度候选集会不一致。

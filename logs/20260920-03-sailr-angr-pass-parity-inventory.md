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

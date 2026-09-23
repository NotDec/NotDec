# 交接：EVM Solidity 链路当前状态与下一步（2026-09-20 收尾）

> **【2026-09-23 注】** 本文里的 `NOTDEC_EVM_INLINE` 三档开关已删除：calldata 链内联是唯一
> 形状（`20260923-04-remove-evm-inline-switch.md`）。文中提到 `=0` / `all` 的段落只作历史记录；
> 当前的默认与唯一口径就是文中说的 `calldata` 档。

## 0. 链路与目标

EVM bytecode → Gigahorse → evm2llvm IR → NotDec（pre-TR → 类型恢复 → post-TR）→ Solidity 后端。
目标：ABI 参数恢复（`slot_N = argN`），以及让转出的 Solidity 可读。

## 1. 本会话已落地（全部已提交，未 push）

| commit | 内容 |
| --- | --- |
| `bda1d705` | 新增 `EvmHelperInliningPass`（pre-TR 内联）、0651 崩溃修复、日志 §11 |
| `1e6b142e` | 内联形状的 7 处 matcher 扩展（sub guard-block、mul 裸 eq、panic-0x12 候选、storage byte length、const-sub limit、revert trailing base、abi_return record 证据） |
| `7cfa0f19` | `NOTDEC_EVM_INLINE=calldata` scope 模式（reader + 调用者闭包，其余 noinline） |
| `c093a1b7` | pattern oracle 双形状标定（`--inline` 下界 + 每 case `inline` 覆盖 + 新 ctest） |
| `f58b8957` | **默认切换为 calldata 内联**、1485 matcher 扩展、空 scope 修复、编译预算按扁平口径重标定 |
| `50c7f62b` | calldata min-size guard 规范形式改写；source 套件解 pin |
| 后端 `5355022` | require 极性修复（`if(cond) body else revert` → `require(cond)`）；后端 `8873d70` 为更早的 4 类 emit 修复 |
| `c96dbaa8` / `ffe65618` | SAILR/Phoenix 的 angr pass 覆盖度盘点 + P0 迁移源图：`logs/20260920-03-sailr-angr-pass-parity-inventory.md` |

开关：`NOTDEC_EVM_INLINE`：不设/calldata = 默认扁平；`=0` = outlined；`=1`/all = 全模块代价模型。

指标：ABI `record==annotated` 扁平 **2264/2316（97.8%）** vs outlined 1858/2316。
`ctest -R notdec.evm.solidity` **5/5 通过**（patterns=默认扁平+下界/覆盖、patterns_outlined=pin outlined 精确、pattern_compile=扁平预算、rewrite、source）。

## 2. 当前最大问题：扁平形状下 structurer 放弃区域

| 指标 | outlined | 扁平默认 |
| --- | --- | --- |
| condition_todo | 729 | **4647** |
| goto | 424 | **5437** |
| dangling goto | 69 | **501** |
| unresolved value | 264 | 289 |

分布（扁平，3588 个渲染函数）：无 goto 3307 个（92%）；1–5 个 66 个；6–10 个 75 个；**>10 个 140 个（3.9%）贡献 4660 = 86%**。`// TODO: recover remaining body` 仅 105 处。
重灾区逐 case 对照（同一二进制）：

| case | outlined goto/blocks | 扁平 goto/blocks |
| --- | --- | --- |
| 0118 | 7 / 63 | 194 / 269 |
| 0637 | 0 / 137 | 163 / 311 |
| 2002 | 9 / 167 | 152 / 378 |
| 0657 | 12 / 155 | 160 / 344 |
| 0029 | 2 / 18 | 264 / 1330 |

结论：**这批失败是内联复制出来的重复区域导致的**（outlined 下同样代码结构化良好），不是 Phoenix/SAILR 主体坏。

## 3. structurer 与 angr 的现状

- 我们的实现：`external/NotDec-llvm2c/lib/Structuring/`（18 文件 14287 行）；默认 `StructurerRegistry.h:13 DefaultStructurerName = "sailr"`；Solidity 路径 `BodyBuilder.cpp:2917 createStructurer("sailr")`。
- 已注册 deopt pass 仅 7 个（`SAILRDeoptimization.cpp:4461`）：SwitchDefaultCaseDuplicator / SwitchReusedEntryRewriter / DuplicationReverter / LoweredSwitchSimplifier / ReturnDuplicatorLow / ReturnDeduplicator / CrossJumpReverter。
- angr（`/sn640/angr/angr/analyses/decompiler/`）有 ~45 个 optimization pass + `region_simplifiers/`；**P0 缺口**：`block_similarity.is_similar`、`duplication_reverter/{similarity,ail_merge_graph,duplication_reverter}.py`、`structured_node.ExpressionNormalizer/structured_node_comparison`；P1：`cross_jump_reverter.py` 完整候选集、`region_simplifiers/*`；P2：`condition_constprop`/`flip_boolean_cmp`/`expr_op_swapper`。
- 架构判断早已做过（`logs/20260619-01-structuring-framework-angr-ghidra-plan.md:729`）：我们**没有 AIL statement，只有 payload id + 结构节点，不能直接照搬**。`StructuredCFG` 的 CFG 层与 angr region graph 基本对齐，但语句/表达式是后端私有（`PayloadRef` 不透明 id，Solidity 侧是 `BodyBuilder` 的 AST），所以 angr 里"改写/合成语句"的 pass（`AILMergeGraph`、`maximize_similarity_of_blocks`）无法直接移植。
- **待用户拍板**：方案 A（图级逻辑照 angr 迁移 + 后端提供 payload 等价回调；回调是 angr 没有的桥接件，需讨论） vs 方案 B（先补语言中立的语句/表达式 IR，即当年没落地的 "AIL manager"，之后可整包照搬）。

## 4. 下一步（按顺序）

1. **重跑 structurer A/B（修正 CLI 字面量）**：`--algo=structured-sailr|structured-phoenix|structured-goto`（`sailr`/`phoenix` 会报 "Cannot find option named"）。确认 `--algo` 在 Solidity 路径是否生效、sailr 是否真的不同于 goto（若完全相同 = SAILR 层在扁平输入上未生效，是 bug）。
2. **0118 单点对照 angr**：dump region 划分 + SAILR deopt 候选/合并日志，定位"候选没生成"还是"生成但判等失败"；输入用 0118 的 `--emit-tr-input-ir` 扁平输出（outlined 只有 7 个 goto，复现面最小）。
3. 按用户决定（A 或 B）迁移 P0：`is_similar` → `similarity` → `AILMergeGraph` → `DuplicationReverter` 方法族；每步用 `StructuringQuality`/`StructuringEvaluator` + `compile-budget.json`（goto 5437 / TODO 4647）+ 两个 pattern 口径回归。
4. 另一条独立杠杆（不涉 angr）：把内联 scope 再收窄成"只展开 calldata 偏移相对形参不恒定的 reader"（`EvmCalldataAccessPass` 已在算的 polymorphic 集合），从源头减少重复区域；all 档实测 goto/TODO 2709/3269 低于 calldata 档 5437/4647，说明 scope 还有优化空间。

## 5. 约定与注意事项

- 默认口径四/五套 suite 必须保持全绿；预算指标只降不升，升必须在 `logs/` 记录理由。
- 提交惯例：`external/NotDec-llvm2c` 子模块先行，再顶层（顶层需带上子模块指针）。
- 不要动其他会话的脏文件：`src/TypeRecovery/mlsub/MLsubGenerator.{h,cpp}`、`external/binarysub`、`external/NotDec-bin2llvm`、未跟踪的 `llvm-22.1.0.nothreads.obj`。
- 构建/测试：`ninja -C build-notdec-nothreads notdec`；`ctest --test-dir build-notdec-nothreads -R 'notdec.evm.solidity'`；pattern 内联口径 `NOTDEC_EVM_INLINE=calldata python3 test/run_evm_solidity_patterns_suite.py ... --inline`；ABI 聚合 `NOTDEC_EVM_INLINE=calldata python3 logs/artifacts/20260920-worklist-24259/agg_abi_params.py`。
- 本会话产出的小工具在 `/tmp`：`/tmp/goto_dist.py`、`/tmp/goto_share.py`、`/tmp/kinds.py`、`/tmp/revertkinds.py`、`/tmp/show_case_diffs.py`（如需长期保留应移入 `logs/artifacts/`）。

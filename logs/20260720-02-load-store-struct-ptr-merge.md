# 20260720-02 load/store struct pointer merge 实现记录

## 原始 prompt

由于 C 语言不区分 load 和 store 类型，所以如果一个指针类型里的 load 和 store 类型都是其他结构体指针类型，那么可以将 load 和 store 类型合并。

基于这个思路去想一个合并策略并测试效果

## 背景和目标

binarysub 里同一个指针的 `ptr_load<T>` 和 `ptr_store<U>` 目前只会产生 `U <: T`。C 语言字段类型没有 load/store 两套视图，所以当 `T` 和 `U` 本身都已经表现为结构体指针时，可以尝试把这两个目标变量合并，减少类型碎片。

本次把策略做成开关并默认开启。目标是在 fortune 上看它是否降低 DebugInfo oracle 的 fragmentation，并确认是否引入 wrong merge；如果没有 wrong merge，先保留为默认策略。

## 实现

- `include/notdec/DecompilerContext.h:21`：新增 `Options::mergeStructPtrLoadStore`，默认开启。
- `src/NotDec.cpp:94`、`src/NotDec.cpp:299`：新增 CLI `--merge-struct-ptr-load-store` 并写入 `Options`；当前 `cl::init(true)`，可用 `--merge-struct-ptr-load-store=false` 关闭。
- `include/notdec/Passes/PassManager.h:78`、`include/notdec/Passes/PassManager.h:135`、`src/Passes/PassManager.cpp:342`：把策略开关传到类型恢复上下文；只有实际运行类型恢复时才会生效。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:134`、`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:139`：在 `ConstraintsGenerator` 里保存策略开关，并新增 `LoadStoreStructPtrMergeCandidate`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:221`：在每个 SCC 的约束求解和 pointer-analysis 派生约束后运行策略，并打印合并数量。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:540`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3494`：`MLsubRecovery` 保存策略开关并传给 SCC generator。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:109`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:124`：新增结构体字段证据判定。当前只把常量偏移且偏移值 `>= 4` 的 record field 当作结构体指针证据，避免把普通 `char *`/byte buffer 拉进来。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2400`：抽出 `configureConstraintContext()`，复用 merge eval 的 `onVariableMerged` hook。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2413`：实现 `hasStructPointerEvidence()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2439`：实现 `collectStructPtrLoadStoreMergeCandidates()`，扫描 V2N 可达变量的 upper bounds，找同一 pointer 变量上的同 size `ptr_load`/`ptr_store`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2504`：实现 `applyStructPtrLoadStoreMergePolicy()`。候选必须满足 load/store 目标都是同 level、同 pointer size 的变量，且都有结构体指针证据；合并复用 `binarysub::merge_variable_into()`。

后续调整：

- `include/notdec/Passes/PassManager.h:125`：默认开启后，不再因为 `tr-level < 2` 单独拒绝该策略；没有类型恢复时该开关不生效。
- `src/Passes/PassManager.cpp:402`：`TargetArch::Other` 只继续拒绝 HType dump / merge eval，不再因为默认策略开启而 abort。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:60`：新增 `FragmentedValueRecord`。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:552`：新增 `writeFragmentedTypes()`，输出 `fragmented_types.jsonl`。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:621`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:673`：收集每个 strict DebugInfo struct pointer 类型在各 SimpleType root 下的值，并在 finish 时落盘。

## fortune 测试

baseline:

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-merge-baseline-1784560001.ll \
  --tr-level=2 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-baseline-1784560001
```

新策略:

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-merge-loadstore-1784560002.ll \
  --tr-level=2 \
  --merge-struct-ptr-load-store \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-loadstore-1784560002
```

结果对比：

| 指标 | baseline | 新策略 |
| --- | ---: | ---: |
| bad_unions | 0 | 0 |
| polluted_components | 0 | 0 |
| fragmented_types | 2 | 2 |
| fragmented_nodes | 18 | 18 |
| merged_nodes | 13 | 28 |
| representative_nodes | 2338 | 2321 |
| peak_rss_mb | 1298 | 3564 |
| wall_ms | 167888 | 528168 |

输出路径：

- `/tmp/notdec-merge-eval-fortune-baseline-1784560001/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-loadstore-1784560002/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-baseline-1784560001/bad_unions.jsonl`
- `/tmp/notdec-merge-eval-fortune-loadstore-1784560002/bad_unions.jsonl`
- `/tmp/notdec-fortune-merge-baseline-1784560001.ll`
- `/tmp/notdec-fortune-merge-loadstore-1784560002.ll`

默认开启后的复跑：

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-20260721-default-loadstore.ll \
  --tr-level=2 \
  -g --work-dir=/tmp/notdec-fortune-work-20260721-default-loadstore \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260721-default-loadstore
```

输出路径：

- `/tmp/notdec-merge-eval-fortune-20260721-default-loadstore/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-20260721-default-loadstore/fragmented_types.jsonl`
- `/tmp/notdec-merge-eval-fortune-20260721-default-loadstore/DebugInfoValueTypes.txt`
- `/tmp/notdec-fortune-work-20260721-default-loadstore/ValueTypes.txt`
- `/tmp/notdec-fortune-work-20260721-default-loadstore/ValueHTypes.txt`
- `/tmp/notdec-fortune-work-20260721-default-loadstore/VarOrigins.txt`

本次结果：

| 指标 | 默认开启 |
| --- | ---: |
| bad_unions | 0 |
| polluted_components | 0 |
| fragmented_types | 2 |
| fragmented_nodes | 18 |
| merged_nodes | 28 |
| representative_nodes | 2321 |
| peak_rss_mb | 3509 |
| wall_ms | 527113 |

`fragmented_types.jsonl` 里两个碎片类型分别是：

- `struct:<anon>@../util/strfile.h:44*`：`zero_tbl::arg0`、`sum_tbl::arg0`、`sum_tbl::arg1`。
- `struct:fd*`：`@File_list`、`@File_tail`、`@Fortfile`、`new_fp::<ret>`、`add_file::arg5`、`add_dir::arg0`、`get_tbl::arg0`、`sum_noprobs::arg0`、`pick_child::arg0`、`pick_child::<ret>`、`open_dat::arg0`、`get_pos::arg0`、`open_fp::arg0`、`maxlen_in_list::arg0`、`matches_in_list::arg0`、`display::arg0`、`print_list::arg0`。

源码对照：

- `fortune.c:181` 定义 `FILEDESC`，内部包含 `STRFILE tbl`、`child/parent/next/prev`。
- `fortune.c:435` 的 `new_fp()` 分配并初始化 `FILEDESC`。
- `fortune.c:560` 的 `add_file()` 调 `new_fp()`，写字段，接到 `head/tail/parent` 链上。
- `fortune.c:741` 的 `add_dir()` 把 `fp->child` 和本地 `tailp` 传给 `add_file()`，形成子链。
- `fortune.c:1192` / `fortune.c:1203` 的 `zero_tbl()` / `sum_tbl()` 只操作 `STRFILE` 的数值字段；它们和 `FILEDESC.tbl` 的关系在调用点表现为 `&fp->tbl`。
- `fortune.c:1216`、`fortune.c:1313`、`fortune.c:1336`、`fortune.c:1374`、`fortune.c:1388`、`fortune.c:1483`、`fortune.c:1497`、`fortune.c:1524`、`fortune.c:1612` 是不同 `FILEDESC*` 参数的字段访问函数。

为什么仍碎片：

- 这些 DebugInfo 标注值大多是函数参数、返回值和全局变量；当前 eval 看的是 SimpleType root 是否被 DSU 合并，而不是最终 HType 形状是否相似。
- `VarOrigins.txt` 显示 `add_file::arg5` 和 `add_dir::arg0` 已经进入同一个最终 UType，但 SimpleType root 仍分别是 `v140@L0` / `v179@L0`，所以 eval 仍算碎片。
- `ValueHTypes.txt` 显示很多函数参数已经恢复成结构体形状，但它们只是形状相似或通过调用点单向约束关联，没有足够的双向等式让 SimpleType root 合并。
- `new_fp::<ret>`、`pick_child::<ret>`、`add_file::arg5` 这类边界值上界仍偏 `top`，字段证据不在这些 root 自身一层上，因此 load/store 策略没有触发。

后续策略可以优先考虑：

- 在 eval 里同时统计 SimpleType root 和最终 UType origin，避免把已经在 HType 层归到一起的值继续算成同等严重碎片。
- 对函数调用边界做更保守的等式传播：当实参/形参两侧都是同 pointer size、同结构体字段形状，且没有冲突字段时再尝试合并。
- 对返回值做 def-use 传播：`return fp` 或 `ret ptr %call.i` 这类直接返回，可把返回 root 和返回值 operand root 作为候选。
- 对 `&fp->tbl` 这种常量偏移形成的子对象，记录 base struct 和 field offset，作为 `STRFILE*` 参数之间间接同源证据；不要直接按 DebugInfo 类型合并。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- fortune baseline 和新策略都能跑完，`bad_unions.jsonl` 都为空。
- 默认开启后 fortune 复跑完成，`bad_unions.jsonl` 为空，并生成 `fragmented_types.jsonl`。

## 评估

- 实现效果：4/10。策略确实多合并了一些变量，且当前 DebugInfo oracle 下没有 wrong merge；但 fortune 的 oracle fragmentation 没改善。
- 复杂度：5/10。代码集中在 `MLsubGenerator`，没有改 binarysub；但新增了一套结构体指针证据判定，后续需要继续校准。
- 维护成本：6/10。当前默认开启，风险主要是耗时和 RSS；后续需要继续压低 post-solve merge 的成本。

## 结论

这个策略先默认开启保留。fortune 没有 wrong merge，但 SimpleType root 级别的 DebugInfo fragmentation 仍是 18；下一步应该把函数边界和返回值的等式候选单独做成更保守的策略，并把 eval 指标拆成 SimpleType root 和最终 HType/UType 两层。

原始 prompt：

> 继续之前的分析从debug info里面选一个类型，比如那个fd吧。好像说最需要补的是返回值的合并是吧，直接 return operand 和 function return root；调用实参/形参在两边都是结构体指针字段切片、且字段不冲突时合并。这个补上去再看看效果

# return / call arg merge 策略

## 背景

DebugInfo eval 里 `FILEDESC*` 仍有很多 root。当前已有 load/store struct pointer merge，但函数边界上还缺两类直接合并：

- `ret` 操作数和函数返回 root。
- call actual arg 和 formal arg；但只在两边都是一层结构体字段切片，且字段区间不冲突时合并。

## 实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:131` 新增 late merge 候选结构，保存 return operand / function return root。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:156` 新增 `StructFieldSlice`，只记录一层字段 offset / size，不递归扫描字段类型。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:168` 新增 call actual/formal 候选结构。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:269` 在 `ConstraintsGenerator::run()` 的 late phase 里运行 return merge 和 call arg/formal merge，再运行原来的 load/store merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2741` 新增 `tryMergeVariablesForPolicy()`，统一检查 level/size、调用 `binarysub::merge_variable_into()`、同步 V2N 和 merge eval。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2801` 新增 `applyReturnValueMergePolicy()`，方向是 return operand -> function return root。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2831` 新增一层字段切片收集，只看 variable bound 上的直接 record field。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2874` 新增字段兼容判断：字段区间不重叠，或者完全相同的 offset/size，才允许 actual/formal merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2931` 新增 `applyCallArgStructPtrMergePolicy()`，方向是 actual arg -> formal arg。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4155` 对跨 SCC summary instantiate 后的 actual/formal 也记录候选，并在 instantiate 循环后补跑一次 call arg/formal merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6681` 对同 SCC 直接 call 记录 actual/formal 候选。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6711` 在 `visitReturnInst()` 里记录 return merge 候选。

## 验证

构建：

```bash
cmake --build ./build --target notdec -j4
```

结果：通过。只剩本文件原有的未用变量 warning，没有新增 unused helper warning。

fortune：

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-20260721-return-call-merge-125648.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-fortune-work-20260721-return-call-merge-125648 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648
```

结果：

- 新增策略合计命中：return value merge 15 对；call arg/formal struct pointer merge 2 对。
- 原 load/store struct pointer merge 仍为 15 对。
- `Constraint generation done! SCC count:11`。
- `bad_unions = 0`，没有新增错误合并。
- `merged_nodes` 从上一轮 28 增加到 45。
- `fragmented_nodes = 18`、`fragmented_types = 2` 仍未下降；`FILEDESC*` 仍有 17 个 root，说明 fd 这组主要还缺更强的数据流/全局链路合并，不是这两条局部策略能直接解决。

输出文件：

- `/tmp/notdec-fortune-20260721-return-call-merge-125648.log`
- `/tmp/notdec-fortune-work-20260721-return-call-merge-125648/ValueTypes.txt`
- `/tmp/notdec-fortune-work-20260721-return-call-merge-125648/ValueHTypes.txt`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/fragmented_types.jsonl`
- `/tmp/notdec-merge-eval-fortune-20260721-return-call-merge-125648/bad_unions.jsonl`

## 评分

- 实现效果：6/10。策略命中且安全，但 fortune 的 fd fragmentation 没降。
- 复杂度：6/10。新增了统一 merge helper 和一层字段兼容判断，逻辑集中在 MLsubGenerator。
- 维护成本：6/10。后续如果要扩大 call arg/formal 合并，可以继续放宽字段证据条件；当前规则比较保守。

## 后续判断

fd 剩余碎片大多是函数参数和全局变量 root，例如 `File_list`、`File_tail`、`Fortfile`、`new_fp::<ret>`、`pick_child::<ret>` 以及多个 `FILEDESC*` 参数。下一步更可能需要围绕全局读写、链表字段流、以及 pointer-to-pointer 参数（`FILEDESC **head/tail`）补合并，而不是继续放宽本次的 actual/formal 条件。

# 原始 prompt

那按照这个思路试试：（对于发现load或store类型是结构体指针的ptr节点，即存储结构体指针的指针）优先尝试做多个PtrLoad之间的合并，以及合并多个PtrStore之间合并，然后再做现有的PtrLoad 和PtrStore的合并，理想情况下都合并为同一个节点。（保持同size的要求）

# 实现记录

## 背景

原来的结构体指针 slot 合并只处理同一个 ptr 节点上的 `PtrLoad` / `PtrStore` 配对。这样多个 load 结果之间、多个 store value 之间即使来自同一个存储结构体指针的 slot，也不会优先归并。

## 修改

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:149`：把候选结构改成通用的 `StructPtrSlotMergeCandidate`，不再只表达 load/store 配对。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:242`：新增同类 access 候选收集和 slot merge 执行函数声明。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:330`：把运行日志从 `load/store struct pointer merge` 改成更准确的 `struct pointer slot merge`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3393`：新增 `collectStructPtrSameAccessKindMergeCandidates()`，在同一个 ptr 节点内收集同 size 的多个 `PtrLoad` 或多个 `PtrStore` 目标。筛选仍要求变量节点、同 level、同 size、指针大小，并且两边都有一层结构体字段证据。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3480`：保留原来的 `PtrLoad` / `PtrStore` 候选收集，改用新的通用候选结构。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3545`：新增 `applyStructPtrSlotMergeCandidates()`，通过已有 `tryMergeVariablesForPolicy()` 执行合并，复用 V2N 更新、merge eval 和 field follow-up 逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3598`：把执行顺序改成 `load-load`、`store-store`、`load-store`。

## 验证

- `ninja -C build src/CMakeFiles/notdec-core.dir/TypeRecovery/mlsub/MLsubGenerator.cpp.o -v`：通过。仍有该文件原有的未使用变量 warning。
- `ninja -C build bin/notdec -v`：通过。
- fortune 普通运行：
  - 输出目录：`/tmp/notdec-fortune-slot-merge-20260724-a`
  - `struct pointer slot merge policy merged 16 pair(s)`
  - `bad_unions=0`
  - `fragmented_nodes=18`
  - `fragmented_types=2`
- fortune trace 运行：
  - 输出目录：`/tmp/notdec-fortune-slot-merge-trace-20260724-a`
  - `load-load-struct-ptr`: 12 次
  - `store-store-struct-ptr`: 1 次
  - `load-store-struct-ptr`: 3 次

## 结论

该策略按预期改变了合并顺序，并让同 slot 的多个 load/store 目标先合并。fortune 上没有出现错误合并，但 fd* fragmentation 还没有下降，说明剩余未合并节点主要不靠同一个 ptr slot 的同类 access 连接。

## 评分

- 实现效果：3/5。命中了新策略并保持 `bad_unions=0`，但 fortune 的 fd* 分片指标未改善。
- 复杂度：3/5。新增一组候选收集和通用执行函数，仍复用现有 merge/eval/follow-up 逻辑。
- 维护成本：2/5。规则仍受原开关控制，筛选条件和旧 load/store 规则一致。

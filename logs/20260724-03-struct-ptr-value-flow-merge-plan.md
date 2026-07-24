# 原始 prompt

我记得最早有一个policy，是找到结构体指针节点，然后在函数内的值之间的范围内，根据数据流关系直接去合并节点，难道这个策略在改动中丢失了吗？

尝试补一下吧，然后看一下性能是否有提升。但是不要搞什么收集同一函数内的 identity-flow 候选，要不尝试利用一下SimpleType 的变量节点内部的专门的指针字段，用来指向对应的什么代表节点？当前的ExtValuePtr它的底层大小是多少？能不能搞个编译时测试让它小于两倍的指针大小？然后把SimpleType 的变量节点内部的那个字段拓展一下，装下ExtValuePtr？

# 结构体指针 value-flow merge 计划

## 背景

当前 PHI/select 只生成单向 subtype 关系，并把 copy 关系交给 pointer analysis。它不会把 SimpleType root 合并，所以 fortune 里 `get_fort::%fp.5` 这类 PHI 可以和 `@Fortfile` slot 合到一起，但不会继续和 PHI incoming 的 `fd*` root 合并。

`binarysub::VariableState` 已经有 `void *externalHandle` 字段，merge 时也会把 handle 从被合并节点搬到代表节点。`ExtValuePtr` 当前是 32 字节，机器指针是 8 字节，不能按“小于两倍指针大小”内联进变量节点；更合适的是把稳定保存的 `ExtValuePtr*` 挂到 `externalHandle`。

## 目标

补一条窄规则：late phase 里直接看 PHI/select 变量节点的一层 lower bounds。如果 lower bound 和 PHI/select 目标都是 pointer-sized struct pointer，且一层字段 slice 不冲突，就合并 source -> destination。这样不额外维护一张函数内候选表，也不把 GEP/ptradd 当成同一个对象。

## 技术路线

创建 value 对应的 SimpleType 变量时，把 `externalHandle` 设成 `PNDiffValueHandles` 中稳定保存的 `ExtValuePtr*`。新增 helper 从 SimpleType root 取回代表 value，并只把 `llvm::PHINode` / `llvm::SelectInst` 作为 value-flow 目标。late phase 扫描当前 V2N 里能看到的变量 root，对 PHI/select root 的直接 lowerBounds 做保守筛选后复用 `tryMergeVariablesForPolicy()`。

## 风险

主要风险是把非 identity 的数据流也合并。第一版只看 PHI/select，且要求两边都有结构体字段证据；不看 GEP、ptradd、load/store 的普通数据流，不递归扫描字段，不合普通指针。

## 判断标准

- 构建通过。
- fortune 完整 source IR 能跑完。
- `bad_unions` 仍为 0。
- 观察 `struct pointer value-flow merge policy` 命中数、`fragmented_nodes` 和耗时是否改善。

# 实现记录

## 改动

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:128` 增加 `PointerDerivedValues`，记录 ptradd/GEP-like 结果，避免把字段地址当成对象本身合并。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:204` 声明 value-flow policy 的 helper；`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:353` 把 policy 接到 late merge 阶段；`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:512` 在 `setAsPtrAdd()` 里记录 ptradd 结果。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:64` 增加 `ExtValuePtr` 大小编译期检查：当前 `ExtValuePtr` 是 32 字节，机器指针是 8 字节，所以不内联进变量节点，而是把稳定的 `ExtValuePtr*` 挂到 `externalHandle`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2903` 到 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3021` 实现从 SimpleType root 取回代表 value、识别 PHI/select target、跳过 ptradd result 和 ptradd recurrence 的逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3021` 实现 `applyStructPtrValueFlowMergePolicy()`：只扫描 PHI/select root 的直接 lowerBounds，要求同 level、同 pointer size、双方都有 struct pointer 证据，且字段 slice 不冲突。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4119` 和 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4149` 在 `createNode()` / `addRemapType()` 中挂 `externalHandle`。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- 回归：`ctest --test-dir build -R 'notdec.type_recovery.(sysy|realworld).tr_level_2' --output-on-failure` 中 realworld 通过；sysy 有 9 个 golden 差异，但这些样例日志里 `struct pointer value-flow merge policy merged 0 pair(s)`，不是本策略触发。
- fortune：`/tmp/notdec-fortune-valueflow-20260724-d` 跑通，主 SCC 中 `struct pointer value-flow merge policy merged 20 pair(s)`。

## 效果

- 对比 `/tmp/notdec-fortune-level0-scc-20260724-b/eval/merge-eval-summary.json`：`fragmented_nodes` 从 17 降到 7，`bad_unions` 保持 0。
- fortune 本次 summary：`/tmp/notdec-fortune-valueflow-20260724-d/eval/merge-eval-summary.json`，`merged_nodes=146`，`representative_nodes=2278`，`wall_ms=21565`，`peak_rss_mb=1167`。
- `/usr/bin/time -v` 实测 fortune wall time 22.79s，最大 RSS 1,195,272 KB。相对 level0-scc 基线约 26.4s / 1.88GB，有明显下降。

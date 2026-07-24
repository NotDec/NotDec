# 原始 prompt

我记得最早有一个policy，是找到结构体指针节点，然后在函数内的值之间的范围内，根据数据流关系直接去合并节点，难道这个策略在改动中丢失了吗？

尝试补一下吧，然后看一下性能是否有提升。但是不要搞什么收集同一函数内的 identity-flow 候选，要不尝试利用一下SimpleType 的变量节点内部的专门的指针字段，用来指向对应的什么代表节点？当前的ExtValuePtr它的底层大小是多少？能不能搞个编译时测试让它小于两倍的指针大小？然后把SimpleType 的变量节点内部的那个字段拓展一下，装下ExtValuePtr？

# 修正 prompt

不要针对Phi当前搞的这些规则都太复杂了，没必要检测搞得这么严格。删掉所有的这些复杂逻辑节点，严格按照我这里说的：就搞一个从ExtValPtr中获取对应的value所属函数的一个helper函数，直接，在增加subtype边的时候，如果两个节点之中存在着结构体指针节点（不递归，仅遍历两边bounds其他策略也有这个逻辑，应该封装了函数吧？），两边所属函数相同，那么就直接合并

# 结构体指针 subtype 边合并记录

## 背景

原来补的 PHI/select late policy 太绕，还额外维护了 ptradd/GEP 派生值和 recurrence 检测。现在按更直接的口径处理：只要 subtype 边两端已经能看出是同一函数里的结构体指针节点，就直接合并 SimpleType root。

`binarysub::VariableState` 有 `void *externalHandle` 字段。`ExtValuePtr` 当前是 32 字节，机器指针是 8 字节，所以不把它内联进变量节点；仍把 `PNDiffValueHandles` 中稳定保存的 `ExtValuePtr*` 挂到 `externalHandle`。

## 当前实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`：删除 `PointerDerivedValues` 和 PHI/select value-flow late policy 声明；新增 `getExtValueFunction()`、`getVariableOwningFunction()`、`maybeMergeSameFunctionStructPtrSubtype()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`：在 `addSubtype()` 调用 `binarysub::constrain()` 后直接调用 `maybeMergeSameFunctionStructPtrSubtype(lhs, rhs)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：删除 PHI/select target、ptradd result、ptradd recurrence 的专门逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：新增 `getFunctionFromLLVMValue()` / `getExtValueFunction()`，从 `llvm::Value*`、`ReturnValue`、`UConstant`、`StackObject`、`HeapObject` 找所属函数。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：`maybeMergeSameFunctionStructPtrSubtype()` 只检查两端都是同 level、同 pointer size 的变量节点，双方都有 `hasStructPointerEvidence()`，且所属函数相同；满足后用 `tryMergeVariablesForPolicy("same-function-struct-ptr-subtype", LHS, RHS, detail)` 合并。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- 回归：`ctest --test-dir build -R 'notdec.type_recovery.(sysy|realworld).tr_level_2' --output-on-failure` 中 realworld 通过；sysy 仍是已有 9 个 golden 差异。
- fortune：`/tmp/notdec-fortune-same-func-subtype-20260724-a` 跑通。

## 效果

- fortune summary：`/tmp/notdec-fortune-same-func-subtype-20260724-a/eval/merge-eval-summary.json`。
- 对比 `/tmp/notdec-fortune-level0-scc-20260724-b/eval/merge-eval-summary.json`：`fragmented_nodes` 从 17 降到 7，`bad_unions` 保持 0。
- 本次 fortune：`merged_nodes=150`，`representative_nodes=2274`，`wall_ms=19596`，`peak_rss_mb=1150`。
- `/usr/bin/time -v` 实测 fortune wall time 20.81s，最大 RSS 1,178,608 KB。

# 原始 prompt

我记得最早有一个policy，是找到结构体指针节点，然后在函数内的值之间的范围内，根据数据流关系直接去合并节点，难道这个策略在改动中丢失了吗？

尝试补一下吧，然后看一下性能是否有提升。但是不要搞什么收集同一函数内的 identity-flow 候选，要不尝试利用一下SimpleType 的变量节点内部的专门的指针字段，用来指向对应的什么代表节点？当前的ExtValuePtr它的底层大小是多少？能不能搞个编译时测试让它小于两倍的指针大小？然后把SimpleType 的变量节点内部的那个字段拓展一下，装下ExtValuePtr？

# 修正 prompt

不要针对Phi当前搞的这些规则都太复杂了，没必要检测搞得这么严格。删掉所有的这些复杂逻辑节点，严格按照我这里说的：就搞一个从ExtValPtr中获取对应的value所属函数的一个helper函数，直接，在增加subtype边的时候，如果两个节点之中存在着结构体指针节点（不递归，仅遍历两边bounds其他策略也有这个逻辑，应该封装了函数吧？），两边所属函数相同，那么就直接合并

# 二次修正 prompt

对，按照这个改一下吧，但是shouldMergeSameLevelVarVar的名字先不改

# 结构体指针 subtype 边合并记录

## 背景

原来补的 PHI/select late policy 太绕，还额外维护了 ptradd/GEP 派生值和 recurrence 检测。随后一版把规则直接放到 `addSubtype()` 尾部，也不够像 policy。当前改为接入 binarysub 现有 `ConstraintContext::shouldMergeSameLevelVarVar`，由 solver 在处理 var-var subtype 边时决定是否用 policy merge 替代普通 bound。

`binarysub::VariableState` 有 `void *externalHandle` 字段。`ExtValuePtr` 当前是 32 字节，机器指针是 8 字节，所以不把它内联进变量节点；仍把 `PNDiffValueHandles` 中稳定保存的 `ExtValuePtr*` 挂到 `externalHandle`。

## 当前实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`：删除 `PointerDerivedValues` 和 PHI/select value-flow late policy 声明；新增 `getExtValueFunction()`、`getVariableOwningFunction()`、`shouldMergeSameFunctionStructPtrSubtype()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`：`addSubtype()` 不再直接 merge，只负责把 `ConstraintContext` 传给 `binarysub::constrain()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：删除 PHI/select target、ptradd result、ptradd recurrence 的专门逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：新增 `getFunctionFromLLVMValue()` / `getExtValueFunction()`，从 `llvm::Value*`、`ReturnValue`、`UConstant`、`StackObject`、`HeapObject` 找所属函数。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：`configureConstraintContext()` 设置 `Context.shouldMergeSameLevelVarVar`，回调到 `shouldMergeSameFunctionStructPtrSubtype()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：`shouldMergeSameFunctionStructPtrSubtype()` 只检查两端都是同 level、同 pointer size 的变量节点，双方都有 `hasStructPointerEvidence()`，且所属函数相同；满足后返回 `true`，由 binarysub enqueue `PolicyReplaceBound` merge。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- 回归：`ctest --test-dir build -R 'notdec.type_recovery.(sysy|realworld).tr_level_2' --output-on-failure` 中 realworld 通过；sysy 仍是已有 9 个 golden 差异。
- fortune：`/tmp/notdec-fortune-policy-hook-20260725-a` 跑通。

## 效果

- fortune summary：`/tmp/notdec-fortune-policy-hook-20260725-a/eval/merge-eval-summary.json`。
- 对比 `/tmp/notdec-fortune-level0-scc-20260724-b/eval/merge-eval-summary.json`：`fragmented_nodes` 从 17 降到 8，`bad_unions` 保持 0。
- 本次 fortune：`merged_nodes=145`，`representative_nodes=2279`，`wall_ms=19948`，`peak_rss_mb=1194`。
- `/usr/bin/time -v` 实测 fortune wall time 21.24s，最大 RSS 1,222,732 KB。

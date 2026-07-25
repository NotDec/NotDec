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

# 2026-07-25 追加：一边有 struct evidence 即可触发

## 本次 prompt

现在 same-function subtype merge 要求两边都有 struct evidence，这个应该是实现有问题，改成只需要一边有struct evidence试试。另外，有没有新增了struct evidence的hook回调，此时也需要对新增了struct evidence的节点做分析，可能也会暴露新的merge机会

## 实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:206`：新增 `onVariableNonVarBoundAdded()` 声明，用来接 solver 侧新增非变量 bound 的回调。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:250`：新增 `hasPointerLikeEvidence()` 声明，用来区分普通 pointer-size 整数和真正有指针使用证据的节点。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2768`：`configureConstraintContext()` 继续设置 `shouldMergeSameLevelVarVar`，并新增 `Context.onVariableNonVarBoundAdded`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2974`：`shouldMergeSameFunctionStructPtrSubtype()` 从“两边都有 struct evidence”改为“至少一边有 struct evidence，且两边都有浅层 pointer-like evidence”。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2991`：新增 `onVariableNonVarBoundAdded()`，当某个变量刚拿到非变量 bound 并呈现 pointer-like evidence 时，只扫描它直接的 lower/upper 变量 bound，复用 same-function subtype policy 入队 merge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3400`：新增 `hasPointerLikeEvidence()`，只看直接 lower/upper bound 是否含 `TMemObject` 或 `TFunction`，不递归。
- `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes`：更新数组化后的 HType 期望。
- `test/type-recovery/llvm-ir/expected/tr-level-2/21_PointerAnalysisBranchingFieldCycle.htypes`：更新同函数结构指针数据流合并后的递归类型期望。

## 判断

- `09_OffsetLoop` 的变化来自 `%p1 -> %p2 = %p1 + 4` 循环写，被合并后正规化成 `top:32[]*`，比两个 offset-4 record 更贴近数组访问。
- `21_PointerAnalysisBranchingFieldCycle` 的变化是多个同函数递归指针入口合到 `rec_34*`，符合这条策略的目标。
- 纯“一边有 struct evidence”在 `fortune.o3.wasm.ll` 上把普通 `i32` 数据流卷进结构体，RSS 到约 55GB 后中止，所以最终加了 pointer-like 门槛。
- fortune 的 `struct:fd*` 剩余碎片从 8 降到 7，主 global 传递链仍在同一个大 root；`bad_unions=0`。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure` 通过，15.78s。
- fortune：`/tmp/notdec-fortune-one-sided-pointergate-20260725-a` 跑通，`elapsed=27.21`，`maxrss=1200100 KB`。
- fortune eval：`fragmented_nodes=7`，`fragmented_types=2`，`bad_unions=0`，`merged_nodes=160`，`representative_nodes=2264`，`wall_ms=25826`，`peak_rss_mb=1171`。

## 成本

- 实现效果：7/10。能补上“证据晚到”导致错过的部分 merge，同时避免 wasm32 普通整数被过度合并。
- 复杂度：4/10。多了一个浅层 pointer-like 门槛，但仍只扫描直接 bounds，不引入新状态。
- 维护成本：3/10。规则仍集中在 `MLsubGenerator` 的 merge policy 附近，后续如果要收窄条件也好改。

# 2026-07-25 追加：typed pointer 外部值作为 pointer-like evidence

## 背景

前一版 pointer-like 门槛避免了 wasm32 上普通 `i32` 被卷入结构体，但也挡住了 `pick_child::%retval.0` 这种本身是 LLVM `ptr` 的 PHI。它的 incoming `%fp.*` 已经是 `rec_3135*`，但 return root 仍是 `top:64`，导致 `pick_child::<ret>` 继续碎片化。

## 实现

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2951`：新增 `hasTypedPointerExternalValue()`，只认 LLVM `ptr`/function typed 的 `llvm::Value*`、函数返回、stack/heap object，不把 pointer-sized integer 当指针。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3421`：`hasPointerLikeEvidence()` 先检查变量挂载的外部值类型，再回退到原来的直接 bound 扫描。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure` 通过，16.12s。
- fortune：`/tmp/notdec-fortune-typed-pointer-evidence-20260725-a` 跑通，`elapsed=27.38`，`maxrss=1198520 KB`。
- fortune eval：`fragmented_nodes=6`，`fragmented_types=2`，`bad_unions=0`，`merged_nodes=167`，`representative_nodes=2257`，`wall_ms=25977`，`peak_rss_mb=1170`。
- `pick_child::%retval.0` 和 `pick_child::<ret>` 都恢复为 `rec_3135*`，进入 `struct:fd*` 的大 root。

## 成本

- 实现效果：8/10。补回 `pick_child` 返回值链路，同时保留 wasm32 性能保护。
- 复杂度：3/10。只是在已有 pointer-like 判断前增加一层 typed pointer 快速判断。
- 维护成本：3/10。逻辑仍集中在 merge policy 的 evidence 判断里。

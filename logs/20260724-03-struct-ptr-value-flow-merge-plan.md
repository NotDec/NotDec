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

# 2026-07-25 计划：定位 pure one-sided evidence 爆炸根因

## 原始 prompt

前一版one-sided evidence内存爆炸可能是有什么额外的bug，接下来按照这个方向探索，看看根本原因到底是什么，而不是采取这种临时的缓解措施。如果真的纯 one-sided evidence有问题也得找出真正的具体问题

## 背景

当前用 pointer-like 门槛挡住了 `fortune.o3.wasm.ll` 的爆内存，但这只是保守保护。纯 one-sided evidence 在 realworld fortune wasm 上 RSS 涨到约 55GB，说明可能有某条 merge policy 把普通值卷进结构体，或者新增 evidence hook 反复暴露过宽 merge 机会。需要定位具体是哪类值、哪条 subtype 边、哪个 hook 触发造成的，而不是只保留门槛。

## 路线

先做临时实验改动，不提交：恢复 pure one-sided 条件，并给 same-function subtype policy 和 `onVariableNonVarBoundAdded()` 增加轻量计数/采样 trace。重点记录参与 merge 的两个变量是否有 struct evidence、是否是 LLVM typed pointer、所属函数、外部值 stable name、直接 bound 摘要和触发来源。然后限时跑 `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`，在内存失控前截取样本，找出最早把普通 `i32` 或非指针值并入结构体的 merge 链。确认后再决定修正点：可能是 evidence 识别过宽、hook 扫描时机不对、SCC level0 过宽、返回/phi 传播误判，或某类 LLVM `ptr`/integer wrapper 没有区分。

## 判断标准

- 能指出至少一个具体错误合并链：函数、值名、两端节点、触发的 subtype/hook 和为什么它不该合并。
- 能解释 55GB 爆内存是类型递归被错误放宽造成，还是 merge 数量/compact type 递归展开造成。
- 如果 pure one-sided 本身确实太宽，也要给出具体反例，而不是只说“太宽”。
- 实验改动只用于定位，最终代码必须干净，不能留下粗暴 tracing 或临时开关。

## 实验结果：函数内 pure one-sided，跨函数禁用

临时实验只恢复函数内 same-function subtype 的 pure one-sided，同时禁用 `applyCallArgStructPtrMergePolicy()` 和 call-site return merge。`fortune.o3.wasm.ll` 在 90 秒限时内跑完：`elapsed=32.30`，`maxrss=2724916 KB`，`Constraint generation done! SCC count:10`。

对比 pure one-sided 且保留跨函数 merge 的实验：90 秒超时，`maxrss=9553428 KB`，并且在 `realloc` 后 `call arg/formal struct pointer merge policy merged 68 pair(s)`；而禁用跨函数后该计数为 0，程序跑完。这个说明爆炸不是函数内 pure one-sided 单独造成的，至少主要放大点在跨函数 actual/formal 或 call-return merge。函数内 pure one-sided 会让更多节点获得结构体证据，然后跨函数策略继续把这些证据沿调用边传播，导致类型图变宽和求解成本上升。

## 追加实验：定位跨函数放大点

临时改动 `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 中 `shouldMergeSameFunctionStructPtrSubtype()`、`onVariableNonVarBoundAdded()`、`applyCallArgStructPtrMergePolicy()`、`applyReturnValueMergePolicy()`，只用于实验，结束后已恢复。

同一 wasm 输入的稳定基线是 `/tmp/notdec-wasm-pointerlike-baseline-20260725-b`：`elapsed=18.23`，`maxrss=1630316 KB`，`Constraint generation done! SCC count:10`。稳定代码在 `realloc` 后只做了 `return value merge=4`、`call arg/formal merge=7`、`field follow-up merge=10`。

只打开 pure one-sided 函数内合并、禁用跨函数 call-arg/call-return 的 trace 结果是 `/tmp/notdec-one-sided-no-cross-trace-20260725-a`：`elapsed=31.35`，`maxrss=2725816 KB`。前 80 个 same-function 候选都集中在 `main`，第一个就是 `main::%bb.loop_entry404.i0`，即 `load i32, ptr inttoptr (i32 2268 to ptr)`，被当成 struct evidence；随后它和同函数内一批 `i32` load、PHI、`malloc(i32 84)` 返回值合并。这说明 no-cross 的 2.7GB 不是死循环，而是函数内 pure one-sided 会把 wasm32 的普通 `i32` 地址值大范围并到结构体链里，导致 `ValueTypes.txt` 变大：稳定基线约 583KB，no-cross trace 约 1.4MB，旧 no-cross 输出约 3.0MB。

只禁 call-return、保留 call-arg 的实验 `/tmp/notdec-one-sided-callarg-only-20260725-a` 90 秒超时，`maxrss=9655108 KB`；这说明 call-arg actual/formal 合并单独就足够触发爆炸。

带 call-arg trace 的实验 `/tmp/notdec-one-sided-callarg-trace-20260725-a/run.log` 在 60 秒超时，`maxrss=6345572 KB`。第一个明显错误入口是：

`main::%bb.brif_next244.i0 -> strchr arg0`，actual 是 `add_file::%bb.loop_entry122.i3`（`%86 = load i32, ptr %85`），formal 是 `strchr::arg0`（`i32 %0`），两边 `actualTypedPtr=0 formalTypedPtr=0`，但都因为已有 offset slice 被判成 struct evidence。后续 `strlen`、`strcmp`、`strncmp`、`stat`、`open`、`opendir`、`perror`、`qsort`、`regexec` 等 libc-like 形参被串起来，常量字符串地址如 `i32 1089`、栈地址如 `%stack_addr -1024` 也被卷入。

当前结论：pure one-sided 本身太宽，根本问题不是 `hasPointerLikeEvidence()` 有 bug，而是 wasm32 里很多真实指针都表现为 `i32`，仅靠“有一边出现 struct field slice”无法区分结构体指针、字符串/buffer 指针、全局地址常量和普通地址整数。跨函数 call-arg policy 是主要放大器；函数内 one-sided 是前置污染源。

原始 prompt：

> 我觉得当前缺失的是这样的逻辑：当两个结构体指针节点被合并时，相同偏移的成员对应的节点，如果也是结构体指针而非普通类型指针，这两个节点也要考虑被合并。
>
> 先写成一个规划文件，然后尝试实现出来吧。

# 结构体字段 follow-up merge 计划

## 背景

当前已有 return / call arg / load-store 等结构体指针合并策略，但这些策略只合并 owner 节点。对于递归结构体，owner 合并后，同一 offset 的结构体指针字段如果没有继续合并，后续 TypeSimplifier 仍会看到多个递归入口，导致递归类型展开较深，fortune 的 `ValueTypes.txt` 中 FD 相关中间类型仍然很长。

## 目标

在 SimpleType 层补一条窄规则：当两个结构体指针变量成功合并后，检查它们同一 offset 的字段；如果两边字段访问到的目标也都是结构体指针变量，并且一层字段证据不冲突，就把字段目标加入后续合并。这样递归结构能沿字段同步收敛，减少后续 compact/coalesce 阶段的重复展开。

## 技术路线

实现上复用现有 late merge policy 框架，不改 binarysub 的 TypeSimplifier。每次结构体指针变量成功合并后，扫描合并后的 owner：如果同一 offset 出现多个结构体指针字段目标，就把这些目标两两加入候选；字段目标必须来自真实 direct load/store，且目标本身已有结构体字段证据。候选进入一个 worklist，再用现有 `tryMergeVariablesForPolicy()` 执行，直到没有新增字段 follow-up merge。

## 风险

主要风险是误把普通指针字段或不同字段合并。控制方式是只看相同 offset，只接受 pointer-sized variable，只接受已有结构体字段证据，并继续使用一层字段不冲突检查。第一版不跨 offset、不深递归、不基于字段集合相似度猜测。

## 判断标准

- 构建通过，现有 type recovery llvm-ir 回归通过。
- fortune 完整 source IR 能跑完，`bad_unions` 仍为 0。
- `merged_nodes` 增加，FD fragmentation 或 `coalesce_ms` 有改善；如果没有改善，也不能引入错误合并。

## 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:171` 增加 `StructFieldFollowupMergeCandidate`，保存 owner、offset 和两个字段目标。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:223` 增加字段目标收集、分组和 follow-up merge 入口声明。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2816` 在 `tryMergeVariablesForPolicy()` 成功后扫描合并后的 owner，把同 offset 多个结构体指针字段目标加入候选。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2964` 增加 `collectDirectStructPointerAccessTargets()`，只从字段地址上的 direct load/store 收集字段内容目标，不用 record 字段本身的宽度推断内容类型。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3012` 增加 `collectOneLevelStructPtrFieldTargetGroups()`，按 offset 收集合并后 owner 里看到的结构体指针字段目标。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3075` 增加 `collectStructPtrFieldFollowupMergeCandidatesForMergedOwner()`，对同 offset 多目标做两两候选，继续要求 pointer-sized、同 level/size、一层字段不冲突。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3124` 增加 `applyStructPtrFieldFollowupMergePolicy()`，复用 `tryMergeVariablesForPolicy("struct-field-followup", ...)` 执行 worklist。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3432` 在 load/store struct pointer merge 成功后也扫描合并后的 owner。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4563` 在 post-summary 阶段追加一次 field follow-up merge。

验证：

- `cmake --build ./build --target notdec -j4`：通过；仍有原来的 unused variable warning。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- fortune source IR 完整运行：`/tmp/notdec-fortune-field-followup-20260722-175129`。
  - 输出：`/tmp/notdec-fortune-field-followup-20260722-175129/out.ll`
  - 工作目录：`/tmp/notdec-fortune-field-followup-20260722-175129/work`
  - 评估结果：`/tmp/notdec-fortune-field-followup-20260722-175129/eval/merge-eval-summary.json`
  - 结果：`struct pointer field follow-up merge policy merged 16 pair(s)`，`bad_unions=0`，`merged_nodes=73`，`representative_nodes=2390`，`wall_ms=63531`。
  - trace 主 SCC：`coalesce_ms=3770`，`total_ms=36858`；相比修改前主要改善了 coalesce 阶段，剩下主要时间在 canonicalize。

评分：

- 实现效果：8/10。fortune 上实际新增 16 对合并，错误合并仍为 0，coalesce 时间明显下降；FD debug fragmentation 还没继续减少。
- 复杂度：6/10。新增逻辑只在 MLsubGenerator 内，但 helper 数量变多，后续最好继续压小字段证据查询代码。
- 维护成本：6/10。规则很窄，默认安全；如果后面还有类似字段证据策略，可以再抽公共工具，现在先不提前抽象。

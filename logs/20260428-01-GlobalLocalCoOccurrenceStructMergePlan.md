# Global/Local Co-Occurrence And Struct Merge Plan

> 2026-04-28 更新：第一、二阶段已完成；第三阶段及以后仍是计划。

## 背景

`external/binarysub` 当前 HEAD 是 `cde0e05 Revert "Isolate co-occurrence analysis per root"`。
revert 后，`TypeSimplifier::bulkSimplify` 又恢复成全局 co-occur：

1. 所有 root 先 `canonicalizeType`。
2. 所有 root 一起 `analyzeOccurrences`。
3. 每个 root 用同一份 co-occur 结果 `simplifyType`。
4. 最后 `coalesceCompactType` 成 `UType`。

这对当前要做的结构体合并是有用的：全局 co-occur 会保留更多顶层类型变量。
这些变量可以作为结构体片段之间的合并标签。

但最终展示类型不应该保留太多没必要的类型变量。因此需要：

1. 先做全局 co-occur，保留更多变量。
2. 在变量较多的 `CompactType` 上做结构体合并。
3. 再做局部 co-occur，删掉全局阶段为了结构体合并而留下的变量。

## 目标

目标不是恢复 `41075f1` 的 per-root 行为，而是同时保留两种分析：

```text
canonicalize all roots
  -> global co-occur simplify
  -> struct merge
  -> local co-occur simplify
  -> coalesce to UType
```

需要额外标记一类变量：

```text
全局 co-occur 后仍保留，
但局部 co-occur 会删除或合并掉的变量。
```

这类变量暂时叫 `structMergeOnlyVars`。它们不是最终展示给用户的核心变量，
但在结构体合并阶段很有用。

## 当前相关位置

主要改动集中在：

- `external/binarysub/include/binarysub/binarysub.h`
  - `TypeSimplifier`
  - `OccurrenceData`
  - `OccurrenceMap`
  - `CompactType`
- `external/binarysub/src/binarysub.cpp`
  - `TypeSimplifier::canonicalizeType`
  - `TypeSimplifier::analyzeOccurrences`
  - `TypeSimplifier::simplifyType`
  - `TypeSimplifier::coalesceCompactType`
  - `TypeSimplifier::bulkSimplify`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
  - `ConstraintsGenerator::genTypes`
  - 调试输出：`appendDebugValueTypes` / `appendDebugVarOrigins`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`
  - 后续如果要让结构体合并影响 `HType`，这里会消费合并结果。

## 第一阶段（已完成，2026-04-28）：把 co-occur 状态显式化

先恢复 `41075f1` 里好的部分：把 co-occur 分析状态从 `TypeSimplifier` 成员里拆出来。

新增：

```cpp
struct OccurrenceAnalysisState {
  OccurrenceMap coOccurrences;
  std::map<SimpleType, CompactTypePtr> processedRecVars;
};
```

接口改成：

```cpp
void analyzeOccurrences(const CompactTypePtr &Ty,
                        OccurrenceAnalysisState &Analysis);
```

`simplifyType` 不再直接读成员 `coOccurrences`，而是显式接受分析结果：

```cpp
CompactTypePtr simplifyType(const CompactTypePtr &Ty,
                            const OccurrenceAnalysisState &Analysis,
                            bool PrintDebug);
```

这一阶段不改变行为。`bulkSimplify` 仍然构造一份全局 `OccurrenceAnalysisState`，
然后所有 root 共用它。

### 第一阶段实现记录

已实现位置：

- `external/binarysub/include/binarysub/binarysub.h:327-389`
  - `OccurrenceAnalysisState`
  - `TypeSimplifier::analyzeOccurrences`
  - `TypeSimplifier::simplifyType`
  - `TypeSimplifier::isClear`
  - `TypeSimplifier::clear`
- `external/binarysub/src/binarysub.cpp:1132-1235`
  - `TypeSimplifier::analyzeOccurrences`
- `external/binarysub/src/binarysub.cpp:1791-1863`
  - `TypeSimplifier::bulkSimplify`
  - `TypeSimplifier::simplify`

本次实际落地后，行为保持为：

1. 所有 root 先 `canonicalizeType`
2. 所有 root 共用一份 `OccurrenceAnalysisState`
3. 再逐个 `simplifyType`

也就是说，这里只是把状态显式传递出来，没有提前切到 per-root 分析。

## 第二阶段（已完成，2026-04-28）：把 simplify 拆成 plan 和 apply

现在 `simplifyType` 内部有一个局部变量：

```cpp
std::map<SimpleType, std::optional<SimpleType>> varSubst;
```

它正好描述变量会被如何处理：

- `varSubst[V] = std::nullopt`：变量被删除。
- `varSubst[V] = W`：变量被合并到 `W`。
- 没有出现在 `varSubst`：变量保留。

把这部分抽成：

```cpp
struct SimplificationPlan {
  std::map<SimpleType, std::optional<SimpleType>> varSubst;
};
```

并拆出两个函数：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &RecVars,
    const OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &InputRecVars,
    const SimplificationPlan &Plan,
    std::map<SimpleType, CompactTypePtr> &OutputRecVars);
```

这样可以在局部阶段真正删除变量之前，先知道哪些变量会被删。

### 第二阶段实现记录

已实现位置：

- `external/binarysub/include/binarysub/binarysub.h:334-366`
  - `SimplificationPlan`
  - `TypeSimplifier::computeSimplificationPlan`
  - `TypeSimplifier::applySimplificationPlan`
  - `TypeSimplifier::simplifyType`
- `external/binarysub/src/binarysub.cpp:1237-1484`
  - `TypeSimplifier::computeSimplificationPlan`
- `external/binarysub/src/binarysub.cpp:1486-1589`
  - `TypeSimplifier::applySimplificationPlan`
- `external/binarysub/src/binarysub.cpp:1591-1595`
  - `TypeSimplifier::simplifyType`
- `external/binarysub/src/binarysub-test.cpp:476-536`
  - `test_simplification_plan_apply`
- `external/binarysub/include/binarysub/binarysub-test.h:3-18`
  - `test_simplification_plan_apply` 声明
- `external/binarysub/src/binarysub-test-main.cpp:35-41`
  - 测试入口接线

### 第二阶段实现时和原计划不同的地方

原计划里第二阶段接口写的是：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &RecVars,
    const OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &InputRecVars,
    const SimplificationPlan &Plan,
    std::map<SimpleType, CompactTypePtr> &OutputRecVars);
```

这次实现时没有把 `recVars/newRecVars` 也一起显式化，而是先只把
`varSubst` 这层 plan 显式化。当前实际接口是：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const SimplificationPlan &Plan);
```

这样做的原因很简单：第二阶段先保证“变量删/并计划”可以单独拿出来，同时不改变
现有行为。代价是：

- `computeSimplificationPlan()` 现在仍然会继续改：
  - `TypeSimplifier::recVars`
  - `TypeSimplifier::variableOrigins`
  - `OccurrenceAnalysisState::coOccurrences`
- `applySimplificationPlan()` 仍然读写：
  - `TypeSimplifier::newRecVars`

所以第二阶段现在只是把“变量如何处理”显式化了，递归变量状态还没有从
`TypeSimplifier` 里拆干净。这个缺口留到第三阶段处理。

### 第二阶段额外补的一点

`applySimplificationPlan()` 里顺手补了 substitution chain 解析。也就是
`A -> B -> C` 这种替换链，应用时会一直跟到最终变量，而不是只展开一层。

### 第二阶段验证

执行过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.global-local-phase12.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-global-local-phase12
```

结果：

- `binarysub` 自测通过
- `notdec-decompile` 编译通过
- `fortune.o3.wasm.ll` 在 `original` 模式下成功跑通

## 第三阶段：处理递归变量状态

当前 `TypeSimplifier` 用成员变量保存：

- `recVars`
- `newRecVars`

双阶段 simplify 后，不能继续让这两个状态隐式混在一起。

规划上用显式 bundle 描述当前 compact program：

```cpp
struct CompactProgram {
  std::map<PolarVar, CompactTypePtr> roots;
  std::map<SimpleType, CompactTypePtr> recVars;
};
```

流程变成：

```text
Input SimpleType roots
  -> CompactProgram compactProgram
  -> CompactProgram globalProgram
  -> StructMergeInfo
  -> CompactProgram mergedProgram
  -> CompactProgram localProgram
  -> UType results
```

`coalesceCompactType` 后续也最好接受显式 `recVars`：

```cpp
UTypePtr coalesceCompactType(const CompactTypePtr &Ty,
                             const std::map<SimpleType, CompactTypePtr> &RecVars,
                             bool Pol,
                             bool PrintDebug);
```

这样每个阶段读哪份递归绑定会更清楚。

## 第四阶段：全局 co-occur 后标记 structMergeOnlyVars

全局阶段：

```text
globalAnalysis = analyze all compact roots
globalPlan[root] = computeSimplificationPlan(root, globalAnalysis)
globalProgram[root] = apply globalPlan[root]
```

局部阶段先不应用，只计算 plan：

```text
localAnalysis[root] = analyze globalProgram[root] only
localPlan[root] = computeSimplificationPlan(globalProgram[root], localAnalysis[root])
```

然后对每个 root 计算：

```text
globalVars[root] = collect vars from globalProgram[root]
structMergeOnlyVars[root] =
  { V in globalVars[root] | localPlan[root] would delete or merge V }
```

这批变量用于调试和结构体合并：

```text
这些变量在全局阶段存在，
所以可以作为结构体片段的合并标签；
但它们不应该强迫最终 UType 继续显示该变量。
```

## 第五阶段：结构体合并建模

结构体合并先放在 `CompactType` 层做，不先放到 `UType` 层。

候选节点：

```cpp
struct StructCandidateNode {
  uint32_t id;
  PolarVar root;
  bool pol;
  std::string path;
  CompactTypePtr body;
  SimpleTypeSet labels;
  SimpleTypeSet structMergeOnlyLabels;
};
```

抽取规则：

1. 遍历 `globalProgram.roots`。
2. 遇到带 `record` 的 `CompactType` 时，生成一个候选节点。
3. `labels` 只取当前 `CompactType.vars`，不深入字段内部找变量。
4. 只有当前位置最终会形成交类型时，`vars` 才作为结构体合并标签。
   在当前 `coalesceCompactType` 规则下，`pol=false` 会合成交类型，`pol=true` 会合成并类型。
5. `structMergeOnlyLabels = labels ∩ structMergeOnlyVars[root]`。

这和前面讨论的模型对应：

```text
节点 = 结构体候选片段
标记 = 顶层交类型里的纯类型变量
冲突函数 = 两个结构体片段能否无损合并
```

第一版不要从字段内部变量建边，避免把只是共享泛型参数的结构体误合并。

## 第六阶段：结构体分组算法

采用缩紧后的整数规划问题，但实现用偏贪心近似。

约束：

```text
每个结构体组：
1. 要么只有一个节点；
2. 要么组内所有节点共享至少一个 label；
3. 组内任意两个节点不冲突。
```

算法：

```text
remaining = all candidate nodes

while remaining not empty:
  for each label:
    bucket = remaining nodes containing label
    group = greedy maximal conflict-free subset in bucket
    score = group size

  pick best group
  output group
  remove group nodes from remaining

afterwards:
  try to merge groups if they share a label and do not conflict
  try to insert singleton groups into existing compatible groups
```

冲突函数第一版只做无损判断：

```text
两个 record 如果同 offset 字段类型不可兼容，则冲突。
不同 offset 不冲突。
相同 offset 可递归兼容，则不冲突。
```

第一版不引入 `unknown` / union 弱化。弱化会改变目标函数，先不混进去。

## 第七阶段：结构体合并结果如何落地

分两步做，避免一次改太多。

### 7.1 只输出调试信息

先不改变最终类型，只输出：

- 每个候选结构体节点：
  - id
  - root
  - path
  - labels
  - structMergeOnlyLabels
  - record 摘要
- 每个分组：
  - group id
  - 共享 label
  - 包含哪些节点
  - 合并后的字段摘要
- 被局部 co-occur 删除或合并的 label。

建议新日志文件：

```text
workdir/type-struct-merge.md
```

### 7.2 再影响 HType 结构体声明

确认分组合理后，再让 `TypeBuilder` 使用分组结果。

可选方案：

1. 在 `binarysub` 输出 sidecar：

   ```cpp
   struct StructMergeInfo { ... };
   struct BulkSimplifyResult {
     std::map<PolarVar, UTypePtr> types;
     StructMergeInfo structMerge;
   };
   ```

2. 保留旧 API：

   ```cpp
   std::map<PolarVar, UTypePtr> bulkSimplify(...);
   ```

   旧 API 内部调用新 API，只返回 `.types`。

3. `MLsubGenerator::genTypes` 后续改用详细 API，把 `StructMergeInfo` 传给 `TypeBuilder`。

这样能避免一次性破坏现有调用点。

## 第八阶段：测试计划

需要分层测试。

### binarysub 单元测试

1. 全局阶段行为不变：
   - 引入 `OccurrenceAnalysisState` 后，现有 `binarysub` 测试输出不变。

2. 全局保留、局部删除变量：
   - 构造两个 root。
   - 全局 co-occur 下某变量保留。
   - 单 root 局部 co-occur 下该变量会被删除。
   - 检查 `structMergeOnlyVars` 包含这个变量。

3. 结构体候选抽取：
   - `α ∧ {0: int}` 和 `α ∧ {4: int}` 抽成两个节点，label 都是 `α`。
   - 字段内部出现的 `α` 不作为 label。

4. 贪心分组：
   - 无冲突节点进入同组。
   - 冲突节点被拆开。
   - 共享链但没有共同 label 的节点不能进同组。

### NotDec 集成测试

先只开 debug 输出，不影响最终类型。

重点看当前用例：

```text
test/type-recovery/realworld/cases/fortune.o3.wasm.ll
```

建议命令：

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

如果只想看目标用例，用现有 workdir/frozen IR 工作流跑 `fortune.o3.wasm.ll`。

## 风险

1. `recVars` / `newRecVars` 当前是成员状态，双阶段后容易串状态。
   需要优先显式化递归变量表。

2. `pol=true/false` 和外层日志里的 upper/lower 命名容易混。
   实现时以 `coalesceCompactType` 当前规则为准：
   - `pol=true` 合成 union。
   - `pol=false` 合成 intersection。

3. 结构体合并先不要改最终 HType。
   先输出分组调试，确认 `fortune` 上分组合理后再落地。

4. `structMergeOnlyVars` 不能只从最终 `UType` 反推。
   局部 simplify 后这些变量可能已经消失，必须在 `CompactType` 阶段记录。

## 建议实现顺序

1. 引入 `OccurrenceAnalysisState`，保持当前全局行为不变。
2. 抽出 `SimplificationPlan`，保持当前 simplify 输出不变。
3. 显式化 `recVars` 输入/输出，避免双阶段状态串扰。
4. 在 `bulkSimplify` 内跑全局阶段和局部 plan，先只 debug 输出 `structMergeOnlyVars`。
5. 在 `CompactType` 上抽取结构体候选节点。
6. 实现贪心分组，先只 debug 输出。
7. 确认分组合理后，再让分组影响 `TypeBuilder` 的结构体声明复用。

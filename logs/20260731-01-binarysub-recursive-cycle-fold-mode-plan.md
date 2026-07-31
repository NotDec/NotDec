# 用户原始 prompt

> 基于external/binarysub/doc/simplesub/RecursiveCycleApproximation.md，规划一下如何实现到binarysub里面做成一个独立的模式，写一个规划文件就可以

# 背景

`external/binarysub/doc/simplesub/RecursiveCycleApproximation.md` 设计了一个可选的递归
cycle 提前合并模式：当共同展开的状态中只有一侧变量已经重复时，保留这一侧的递归
骨架，把另一侧的递归 SCC 汇总到骨架中。cycle 2 和 cycle 3 的组合因此不必等到
LCM(2,3) 的完整状态再次出现。

当前 C++ 实现已经不是文档中的 Scala 版本，主要入口是：

- `include/binarysub/binarysub.h:280` 的 `CompactType`。它是有限的树/DAG；递归不能
  通过 `CompactType` 指针直接自引用。
- `include/binarysub/binarysub.h:541` 的 `CompactTypeScheme`，用 `recVars` 保存递归
  变量的 bounds。
- `include/binarysub/binarysub.h:616-725` 的 `CanonicalRecursiveEntry` 和
  `TypeSimplifier` 状态。当前已有 `Pending/Stable`、`ownerIndex`、并发 map 和递归
  bounds 更新逻辑。
- `src/binarysub.cpp:1516-1760` 的 `TypeSimplifier::canonicalizeType()`。
  `go0()` 做变量 closure 和外层转换，`go1()` 按
  `(CompactTypePtr, polarity)` 检测递归、合并 bounds、递归处理 record/function/pointer。
- `src/binarysub.cpp:2550-2800` 的 `bulkSimplifyDetailed()`。它先并行或串行
  canonicalize 所有 roots，再按 root 做 occurrence analysis、simplify 和 coalesce。
- `include/binarysub/binarysub.h:608-614` 的 `BulkSimplifyOptions` 当前只控制线程和
  canonicalize 并行开关，没有 canonicalize 算法模式。

当前 `binarysub` 工作树还有与本任务无关的本地状态：子模块中修改过的
`include/binarysub/binarysub-core.h`、`src/binarysub-core.cpp`，以及未跟踪的
`build-gcc14-poly-scc-20260729/`。实现时必须保留这些改动，不把它们作为本计划的基线
或回滚目标。

# 目标

在 `binarysub` 内增加一个明确的、可调用的 cycle 提前合并模式，满足以下目标：

1. 现有 Exact canonicalization 的默认行为、并行行为和输出保持不变。
2. 新模式不直接返回 `Top`，而是保留一个已重复 cycle 的结构，并把 companion cycle
   的 bounds 做极性正确的合并。
3. 对结构体同字段、同极性的 cycle 2/3，输出递归层数受骨架 cycle 限制，不再产生
   LCM 数量的递归结果节点。
4. 递归通过 `recVars` 和 fresh `SimpleType` 变量表示，不构造会破坏
   `CompactType::check()`、hash 或比较的直接自引用。
5. 用固定的关系证明和回归测试确认：顶层正位置满足
   `Exact <: Folded`；负位置使用相反方向。
6. 超出第一阶段支持范围的函数参数反极性、复杂分支和无法建立 SCC 的情况，不默默
   使用错误的折叠结果。

# 不做什么

- 不修改 NotDec 顶层调用方，不把新模式设为默认模式。
- 不修改现有 Exact 模式来“顺便”支持提前合并。
- 第一阶段不处理所有一般递归图的最优摘要，不承诺任意函数/指针 cycle 都能达到
  `O(m+n)`。
- 不把显式 `Union` 递归类型直接替换进现有 `CompactType` pipeline；该路线保持为
  后续备选，因为当前 occurrence analysis、simplify 和 coalesce 都依赖统一的
  `CompactTypeScheme`。
- 不在折叠模式中复用全局 Exact 的 `recursive`、`recursiveEntries` 和 `recVars`，
  避免不同 root 或并行任务互相污染。

# 技术路线

## 1. 增加独立模式和调用边界

在 `include/binarysub/binarysub.h` 增加公开枚举，例如：

```cpp
enum class CanonicalizeMode {
  Exact,
  Folded,
};
```

在 `BulkSimplifyOptions` 增加 `canonicalizeMode = CanonicalizeMode::Exact`。
默认值必须是 `Exact`，现有调用不需要改动。

`bulkSimplifyDetailed()` 按 mode 选择：

```text
Exact  -> 现有 canonicalizeType() 和现有共享递归状态
Folded -> 新的本地 folded canonicalization 路径
```

第一阶段让 `Folded` 的 canonicalize 阶段强制串行。后面的 local simplify/coalesce 是否
可以继续并行，等每个 root 的 folded `CompactTypeScheme` 能独立收集后再决定。trace 中
要同时记录 mode 和实际的 canonicalize parallel 状态，不能只看线程数推断算法路径。

建议不要先改现有 `canonicalizeType()` 的参数和 Exact 分支。新增一个内部的
`canonicalizeFoldedRoot()`，返回 root 和该 root 独有的 `recVars`，以便两条路径的
状态边界清楚。若测试需要直接访问 compact 结果，再增加一个返回
`CompactTypeScheme` 的测试/内部入口，而不是让测试读取 `TypeSimplifier` 的共享成员。

## 2. 把 canonicalize 状态按 root 隔离

在 `src/binarysub.cpp` 中增加 folded 模式专用的运行状态。它至少包含：

```cpp
struct FoldedCanonicalizeState {
  // 当前 root 的递归 path，必须有顺序，不能只有 visited set。
  struct PathFrame {
    PolarCompactTypeKey key;
    SimpleVarSet vars;
    // 进入该 frame 的结构边，用于确认是否是同一条递归路径。
    ...
  };
  std::vector<PathFrame> path;

  // 当前 root 的递归变量和 bounds；不写入 Exact 的共享成员。
  PolarCompactTypeMap<SimpleType> recursive;
  SimpleVarMap<CompactTypePtr> recVars;

  // 可选折叠规则、companion summary 和固定点工作队列。
  ...
};
```

真实字段应配注释，说明为什么 path 必须保留顺序，以及为什么 folded 状态不能复用
现有全局递归表。可以复用 `CompactTypeArena`、`go0` 的缓存逻辑和
`mergeCompactTypes()`，但缓存的生命周期必须跟随本次 folded root，不能让含有 mode
相关假设的结果进入 Exact 的全局缓存。

`CompactTypeScheme` 仍然是最终交付给 occurrence analysis 的边界：

```text
folded root + folded recVars -> bulkSimplifyDetailed 后续阶段
```

在 bulk 流程中，先按 root 收集这些本地 `recVars`，再建立 canonical origin 表和
local simplify 输入。不能在多个 root 的局部变量都完成前直接写共享 `recVars`。

## 3. 保留完整状态检测，增加有序 path 检测

folded `go1()` 仍然先检查完整 `(CompactType, polarity)`：

```text
完整状态重复 -> 使用精确递归变量，和 Exact 模式相同
```

只有完整状态没有重复时，才检查当前 `ty->vars` 中是否有同极性变量已经出现在 path：

```text
当前 vars 包含 L0，path 中曾出现 L0
但当前 CompactType 不等于之前的完整状态
-> 生成 FoldRequest，不直接回指旧节点
```

`FoldRequest` 至少需要保存：

- 首次出现的 path index。
- 重复的 `(SimpleType variable, polarity)`。
- cycle 的路径长度。
- 触发重复的结构边，至少区分 record field、function argument/result、pointer load/store。
- 当前状态和首次状态中的 companion 变量候选。

如果当前 path 中有多个候选变量，使用确定规则：先选择最短的重复区间；长度相同按
变量 id 排序。不得依赖 `std::set` 或 unordered container 的迭代顺序。

## 4. 只为可验证的递归 SCC 建立折叠规则

从首次状态和当前状态中的变量，建立带极性的依赖图：

```text
(variable, polarity)
```

边来自变量在当前 polarity 下的 bounds。record field 和函数返回保持 polarity，
函数参数、direct store 翻转 polarity。每条边保留结构位置信息，避免把不在同一递归
路径上的变量误当成一个 cycle。

第一阶段只接受以下规则：

- 骨架是同一个 record 字段上的单后继 cycle。
- cycle 内 polarity 不翻转。
- companion 变量属于可以完整列出的递归 SCC。
- SCC 内部没有无法保留的外部结构分支。

不满足条件时，Folded 模式应回退到本 root 的 Exact canonicalization，或者返回明确的
不支持结果；不能返回只展开了一半的 compact type。第一版采用回退 Exact，避免因为
模式选择导致错误的窄化。

这一步要特别处理 `CompactType` 的变量成分。现有 `go1()` 不是只合并非变量 bounds，
而是执行：

```cpp
res = mergeCompactTypes(pol, ty, bound);
```

所以 companion summary 必须保留成员变量对结果的贡献；不能只缓存 primitive、record
和 function 部分，否则原来未约束的变量可能被错误删除。

## 5. 创建 companion summary，不创建 CompactType 自引用

对被忘掉的 companion SCC 创建一个共享摘要。摘要用 fresh `SimpleType` 变量加
`recVars` 表示，逻辑上类似：

```text
R* = merge_p(R0, R1, ...)
recVars[R*] = merge_p(各成员的一层 bounds，SCC 内回边替换为 R*)
```

实现时需要一个 folded 专用的 summary 表，记录：

```cpp
struct FoldedCompanionSummary {
  std::uint32_t id;
  bool polarity;
  std::set<SimpleType> members;
  SimpleType recVar;
  CompactTypePtr oneLayerBody;
};
```

`oneLayerBody` 必须是有限 `CompactType`。SCC 内部回边只能保存为 `recVar` 变量，不能
把 `CompactTypePtr` 指回自身；这是因为当前 `CompactType::check()`、
`structuralHash()` 和比较逻辑都假定输入是有限树/DAG，现有测试也明确把直接
`CompactType` self-cycle 视为不支持。

summary 的变量成分要保留到最终 coalesce 可见的位置。若直接把所有成员复制到每个
骨架节点会重新产生 `O(mn)` 的集合和 hash 成本，应优先让 bucket 持有 summary id，
在展开和最终构造 body 时共享成员集合。若第一版无法安全共享而只能复制，应先接受
“限制输出节点数但不保证线性构造时间”，不要把这个实现误报成 `O(m+n)`。

## 6. 用固定 bucket 合并骨架

设骨架 cycle 长度为 `m`，创建 `m` 个 bucket 和 `m` 个 fresh recursion variables：

```text
A0, A1, ..., A(m-1)
```

路径中相对首次出现位置为 `d` 的骨架状态进入 `A(d mod m)`。bucket 保存：

- 本 bucket 的 polarity。
- 尚未被 companion summary 替代的 source compact state。
- 关联的 summary ids。
- 一层 body。
- 输出使用的 fresh recursive variable。

第一次发现 `L0` 重复时，不把当前 `L0 | R2` 直接回到原来的完整状态，而是：

```text
A0 接收首次的 (L0,R0) 和当前的 (L0,R2)
A1 接收 (L1,R1)
```

随后用 worklist 重新展开变化的 bucket。record `next` 的后继发送到下一个 bucket；
其他字段继续走普通 folded `go1()`。一个 bucket 的 source 没有变化时不重新处理，
全部 bucket 稳定后，把 body 写入本地 `recVars`。

这个固定点必须使用 `mergeCompactTypes(pol, lhs, rhs)`：

```text
正位置：componentwise join/union 方向
负位置：componentwise meet/intersection 方向
函数参数和 pointer store：翻转 polarity
```

不能在 folded 模式中统一使用 union，也不能以“当前一侧访问过”为理由丢掉当前
状态的字段。

## 7. 让 folded 结果接入现有后处理

`bulkSimplifyDetailed()` 之后的三个阶段继续复用现有逻辑：

1. `analyzeOccurrences()` 读取 folded root 和 folded `recVars`。
2. `computeSimplificationPlan()` / `applySimplificationPlan()` 继续处理变量合并。
3. `coalesceCompactType()` 通过 `recVars` 生成最终 `UType`。

需要专门检查以下边界：

- folded summary 的 fresh variable 有合法 size、origin 和 id，不与现有 root 或其他
  summary 冲突。
- folded `recVars` 被 `analyzeOccurrences()` 访问时不会把 summary 的有限 body
  再次展开成原来的 companion product。
- `newRecVars`、origin map 和 local simplify 的 root 隔离仍然成立。
- folded 模式的最终递归变量如果被 `simplifyType()` 合并或删除，不能留下悬空的
  `recVars` 条目。

如果 summary 需要在 coalesce 阶段才展开，应在 `coalesceCompactType()` 前把它转换成
普通 `CompactTypeScheme`，不要把 summary 专用指针泄漏到 `CompactType` 的 hash、比较
或打印路径。

## 8. 记录可观测指标

新增 folded 专用 trace，至少包括：

```text
canonicalize_mode=exact|folded
fold_requests
folded_roots
folded_fallback_exact
folded_companion_summaries
folded_buckets
folded_fixpoint_updates
folded_max_path_depth
folded_compact_nodes
```

保留现有 `canonicalizeProgressStateCount()`，但要明确它统计的是 Exact 状态还是
folded bucket 状态，不能让两者混在一个计数里误导性能判断。

trace 不作为正确性依据；它只用于证明 cycle 2/3 和较大互质 cycle 没有生成 LCM 数量
的 folded 输出节点，并帮助分析 fallback 原因。

# 实现阶段

## 阶段 0：基线和接口

1. 记录当前 Exact 模式在现有 `binarysub` 自测上的输出、测试数量和关键 trace。
2. 在 `binarysub.h` 增加 `CanonicalizeMode` 和 `BulkSimplifyOptions` 字段，默认 Exact。
3. 增加 mode trace 和一个不改变行为的选项测试。
4. 确认 Exact 路径继续使用现有 `CanonicalRecursiveEntry`、ownerIndex 和共享缓存。

判断标准：不启用新模式时，现有 binarysub 自测、Exact 输出和并行/串行比较不变。

## 阶段 1：record 同极性 cycle 2/3

1. 为 folded root 增加本地 path、递归表、recVars 和 fold request 状态。
2. 实现单个 record field 的 partial repeat 检测。
3. 实现 companion SCC summary 和有限 body 替换。
4. 实现两个 bucket 的固定点合并。
5. 接入 `CompactTypeScheme`、occurrence analysis 和 coalesce。

判断标准：结构体 cycle 2/3 的 Exact 结果仍有完整状态，Folded 结果最多保留骨架
cycle；`I | S` 不会错误变窄成 `I`；原始 `Variable` bounds 没有被修改。

## 阶段 2：一般 cycle 长度和性能

1. 将两个 bucket 推广为路径长度 `m` 的 bucket 数组。
2. 增加 3/5、5/7 等互质 cycle，比较 Exact 的 LCM 状态和 Folded 的骨架状态。
3. 让 companion summary 缓存一层 body 和成员成分，避免每个 bucket 重新扫描 companion SCC。
4. 验证 folded canonicalize 的 wall time、CPU time、peak memory 和 state/bucket trace。

判断标准：在简单单后继 cycle 上，Folded 输出节点数与骨架长度线性相关；未启用共享
summary 时如仍为 `O(mn)`，必须在 trace 和文档中明确，不把它称成线性实现。

## 阶段 3：负位置和有限支持的 pointer/function 分支

1. 增加负位置 record 测试，确认使用 intersection 方向，满足
   `Folded <: Exact` 的极性关系。
2. 增加 pointer load/store 的同极性和反极性测试。
3. 再处理函数返回值；函数参数先保持 Exact fallback，除非能证明 polarity 和 summary
   关系正确。
4. 对不支持的分支保留明确 fallback 统计。

判断标准：不支持的路径不会产生部分 folded body；所有已支持路径都有对应 polarity
测试和安全方向测试。

## 阶段 4：并行和默认策略评估

第一阶段不把 Folded canonicalize 放进现有共享 TBB 路径。等 local state、summary
生命周期和输出稳定后，才评估按 root 并行：

- 每个 root 的 folded state 必须完全独立。
- fresh variable、origin 和 `recVars` 合并必须无数据竞争。
- Exact 和 Folded 的 trace、输出 map 和 local simplify 不得互相覆盖。

即使并行实现完成，Folded 也继续保持显式 opt-in；不通过真实输入对比前不改默认模式。

# 风险

- **安全方向错误**：如果把正位置和负位置混用，或只合并当前 `R2` 而不传播到稳定，
  可能产生比精确类型更窄的结果。必须先保留完整状态检测，并用小型结构模拟证明。
- **递归表示错误**：直接把 `CompactTypePtr` 指回自身会触发当前 check/hash/compare 的
  有限 DAG 假设。所有新递归边必须经过 fresh variable 和 `recVars`。
- **变量成分丢失**：summary 只缓存非变量 bounds 会删除未约束变量。需要保留 member
  成分，或在无法共享时先回退 Exact。
- **全局状态污染**：复用 Exact 的 `recursive`、`recVars` 或 ownerIndex 可能让不同 root
  的折叠规则互相影响。第一阶段使用 local state，bulk 最后再合并 scheme。
- **并行不确定性**：bucket 固定点更新不是当前共享递归表的同一种并发协议。先串行，
  后续并行必须有单独的确定性和 race 检查。
- **复杂度虚假改善**：输出只有 `m` 个 bucket 不等于构造一定是 `O(m+n)`。必须区分
  状态数量、集合复制、hash 和 bounds 扫描成本。
- **后处理失配**：`analyzeOccurrences()`、`simplifyType()` 和 `coalesceCompactType()`
  可能依赖现有 recVars 的 ownership/origin 关系。折叠输出必须经过完整后处理测试，
  不能只看 canonicalize 打印结果。

# 验证和判断标准

## 单元测试

在 `src/binarysub-test.cpp` 增加独立测试函数，并从
`src/binarysub-test-main.cpp` 注册：

1. `test_canonicalize_cycle_fold_struct_2_3`
   - 构造本文的 `L0/L1` 和 `R0/R1/R2`。
   - Exact 和 Folded 都运行。
   - Exact 保留完整六状态；Folded 输出只保留骨架 bucket。
2. `test_canonicalize_cycle_fold_preserves_widening`
   - 检查 folded 每一层的 primitive/field/next 都覆盖 Exact 对应 bucket。
   - 明确检查 `I | S` 没有变成 `I`，并检查 `next` 仍然递归。
3. `test_canonicalize_cycle_fold_negative`
   - 在负位置检查 merge 方向和最终 `Folded <: Exact` 关系。
4. `test_canonicalize_cycle_fold_fallback`
   - 函数参数翻转或非单后继分支走 Exact fallback，并记录 fallback trace。
5. `test_canonicalize_cycle_fold_isolated`
   - 同一 `TypeSimplifier` 先后处理单独 `L0` 和 `L0 | R0`，确认单独 root 没有被
     另一个 root 的 folded summary 污染。
6. `test_canonicalize_exact_unchanged`
   - 现有 Exact 测试继续运行，并比较 normalized UType、origin 和 recVars 可达结果。

如果当前测试辅助函数不方便构造递归变量，应增加最小的 SimpleType constraint builder，
不要在测试中直接制造 `CompactType` 自引用。

## 构建和运行

在 `external/binarysub` 的现有构建目录中运行：

```bash
cmake --build <binarysub-build> --target binarysub -j4
<binarysub-build>/binarysub
```

同时验证：

- oneTBB 开启和 `-DBINARYSUB_ENABLE_TBB_PARALLEL=OFF` 的构建。
- Exact 默认模式的并行/串行结果一致。
- Folded 模式强制串行时 trace 明确，不误报为 canonicalize 并行。
- canonicalize timeout 触发时不会输出未完成的 folded scheme；应回退 Exact 或明确失败。

## 性能和真实输入

先使用合成 cycle 测量：

- Exact/Folded 的最大 path 深度。
- Exact/Folded 的 compact node、recursive var、bucket 和 summary 数量。
- wall time、CPU time、peak memory。

再选择一个 binarysub 现有真实输入做 smoke test。真实输入只用于确认 folded mode 不会
破坏变量 origin、local simplify 和最终 UType；不以真实输入没有触发 LCM 为成功标准。

# 复杂度和维护成本评估

- **实现效果：8/10。** 对单后继同极性递归 cycle 能把结果从 LCM 状态压到骨架长度，
  并保留比 `Top` 截断更多的 record/primitive 结构；复杂递归图只保证安全 fallback。
- **理解成本：7/10。** 需要同时理解 path partial repeat、companion summary、
  `recVars` 递归表示和 bulk root 隔离，明显高于只改现有递归检测。
- **后期维护成本：7/10。** Exact 和 Folded 两条路径会长期并存；通过显式 mode、默认
  Exact、本地状态和明确 fallback 限制影响面。若 summary 表示最终需要修改
  `CompactType`，维护成本会上升，应优先使用现有 `recVars` 和有限 body。

更简单但更粗的方案是 partial repeat 后保留前缀、在后续直接使用 `Top`；它容易实现，
但用户已经确认精度不够，因此不作为本计划目标。更精确且仍是 `O(m+n)` 的方案是保留
显式 union 的两棵递归类型，但它不适合当前要求统一 compact shape 的后续共现分析，
作为 summary 实现无法稳定接入时的后续方向。

# 完成标准

本计划完成实现后，必须同时满足：

```text
Exact 默认路径没有行为回归；
Folded 是显式可选模式，不修改原始 Variable bounds；
结构体 cycle 2/3 不再等待 LCM 状态才打结；
已支持正位置满足 Exact <: Folded，负位置满足 Folded <: Exact；
CompactType 仍是有限树/DAG，递归只通过 recVars 表示；
unsupported case 有明确 Exact fallback；
单测、oneTBB ON/OFF 构建和现有 binarysub 自测通过；
性能数据区分输出节点数和真实构造时间。
```

实现完成后，把具体修改文件、函数、验证命令、结果和偏离本计划的决定追加到本文件，
不要另建一个重复的实现日志。

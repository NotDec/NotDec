# 用户原始 prompt

> 那写一个新的规划文件吧，说明一下具体怎么做

# 状态（2026-08-02：实现已撤销）

本计划实现出的第一版只支持同名 record 字段上的单后继递归，在 memcached 限时探针处理的
253 个 root 中命中为 0，不能用于当前真实输入目标。因此已撤销 Folded 模式、主链路环境变量接线和
对应测试。此前独立完成的 `CompactTypeBuilder::mergeAll()` 可变聚合优化不属于本计划，继续保留。
本计划到此停止，下文只保留当时的设计，不再作为待实施方案。

撤销后，`external/binarysub/include/binarysub/binarysub.h:612` 的 `BulkSimplifyOptions` 不再包含模式选择，
`external/binarysub/src/binarysub.cpp:1702,2778` 的 `canonicalizeType()` 恢复为唯一 canonicalize 路径；
`src/TypeRecovery/mlsub/MLsubGenerator.cpp:6141` 删除 Folded 环境变量接线，`AGENTS.md:287-300` 同步删除说明。

验证通过：`./build/binarysub`、`./build/bin/TypeBuilderTest`（9/9）、
`./build/bin/MLsubGeneratorTest`（18/18）以及
`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`。

撤销效果 9/10：真实输入不会再进入无效分支；理解成本 1/10，维护成本 1/10。后续更合适的方案是单独
规划支持完整 `CompactType` 结构和极性的状态图算法，而不是继续给本实现增加特例。

# 方案调整

本计划改为遍历实际组合状态，不做 companion SCC summary，固定 bucket 只负责聚合。

Folded 模式仍保留 Exact/Folded 两种模式、partial repeat 和固定 bucket，但 bucket 更新不再调用
`mergeCompactTypes()` 生成新的不可变中间节点：

- Folded 模式使用每个 root 独有的可变临时状态。
- 临时状态不进入 structural hash、缓存或共享递归表。
- 状态遍历和聚合完成后才生成不可变 `CompactType`。
- Exact 仍是默认模式，不改现有实现。

后续如果按本计划实现，直接把实现记录、验证结果和计划调整写回本文件，不再新建一份重复日志。

# 背景

当前 `CompactType` 不可变是有意保留的约束。它会缓存 structural hash，并作为
`PolarCompactTypeSet/Map` 的结构键；`go0Cache` 和并行 canonicalize 也会共享这些有限
树/DAG。若在发布后修改节点，不仅节点自己的 hash 会失效，所有引用它的祖先 hash 也会失效，
还会破坏 unordered 容器的查找规则。因此不能为了 Folded 模式把 `CompactType` 整体改回可变。

另一方面，memcached profile 已确认当前内存高峰主要来自反复合并不可变 `CompactType`：

- `CompactTypeArena::merge()` 每次复制 `SimpleVarSet`、record 等容器并生成新节点。
- `TypeRef` 是非 owning handle，arena 为保证 handle 有效，会保留全部中间节点直到
  `TypeSimplifier::clear()`。
- 16 GiB 后段采样中，`SimpleVarSet` 节点约占 11.4 GiB，`CompactType` 本体约占
  3.3 GiB。
- 已实现的顶层 `CompactTypeBuilder::mergeAll()` 证明局部可变聚合有效：
  `CompactType` 构造减少 43.5%，早段 Heaptrack live heap 减少 12.5%。但它没有处理
  nested record/function/pointer merge，也没有减少 canonicalize 的组合状态数量。

Folded 模式和局部可变聚合解决的是两个不同问题：

```text
Folded bucket       遍历 LCM 个组合状态，但把它们合并到骨架数量的结果 bucket
可变 accumulator   减少一个 bucket 反复合并时产生的不可变中间节点
```

本计划把两者放在同一条 Folded 专用路径内。不能预先声称它能解决 memcached 内存问题；
需要先统计真实输入中的 partial repeat 和 Folded 命中率。

# 目标

1. 增加显式 opt-in 的 Folded canonicalize 模式，默认仍为 Exact。
2. 第一阶段支持同一 record 字段上的单后继、同极性递归 cycle；其他情况完整回退 Exact。
3. cycle 2/3、3/5、5/7 等组合遍历所有可达的 LCM 个组合状态，但只发布骨架数量的 bucket，
   不生成 LCM 数量的递归结果节点。
4. bucket 在 root 内使用可变 accumulator，状态遍历期间不生成 arena-owned merge 前缀。
5. 状态遍历完成后每个 bucket 只 freeze 一次，递归仍通过 fresh variable 和
   `recVars` 表示，不构造 `CompactTypePtr` 自环。
6. 正位置满足 `Exact <: Folded`，负位置满足 `Folded <: Exact`。
7. 用合成 cycle 和真实输入分别测量状态数、arena 节点数、时间和峰值内存，不把输出变短
   直接当作内存改善。

# 不做什么

- 不把 `CompactType` 字段改回可变。
- 不取消 `cachedHash`，不把 recursion key 改成对象 identity。
- 不修改现有 Exact `canonicalizeType()` 的算法和默认并行行为。
- 不在第一阶段处理 function argument、pointer store 等会翻转极性的递归路径。
- 不在第一阶段把 Folded 的可变 accumulator 推广成 Exact 的通用 nested builder。
- 不承诺任意递归图都能折叠；验证失败时按 root 回退 Exact。
- 不修改原始 `Variable` 的 lower/upper bounds。

# 总体边界

Folded 路径分为三个阶段：

```text
只读发现
  读取不可变 CompactType 和 SimpleType bounds
  发现 partial repeat，验证骨架和组合状态可枚举性

局部聚合
  使用 per-root 可变 accumulator，把每个实际组合状态追加到对应 bucket
  不进入 arena hash key、go0Cache、recursive 或 recVars

发布结果
  状态遍历完成后 freeze 成不可变 CompactType
  按 root 顺序提交 recVars 和 root
```

只有第三阶段的结果能交给现有 occurrence analysis、simplify 和 coalesce。失败的尝试必须在
freeze 和 fresh variable 分配前结束，避免一次 fallback 改变后续变量编号或污染共享状态。

# 具体设计

## 1. 模式和返回边界

在 `external/binarysub/include/binarysub/binarysub.h` 增加：

```cpp
enum class CanonicalizeMode {
  Exact,
  Folded,
};

struct BulkSimplifyOptions {
  bool enableParallel = true;
  std::optional<std::size_t> threadCount;
  CanonicalizeMode canonicalizeMode = CanonicalizeMode::Exact;
};
```

新增 `canonicalizeFoldedRoot()`，返回一个 root 局部结果，而不是直接写现有共享表：

```cpp
struct FoldedCanonicalizeResult {
  CompactTypePtr root;
  SimpleVarMap<CompactTypePtr> recVars;
  FoldedMetrics metrics;
  bool folded;
};
```

Folded canonicalize 第一阶段强制串行。每个 root 先完成发现、状态遍历和 freeze，再按 root 顺序
提交结果。不支持的 root 调用现有 Exact `canonicalizeType()`；已成功 Folded 的 root 使用自己的
fresh variables 和 `recVars`。

## 2. 有序路径和 FoldRequest

现有 Exact 路径只需要 `inProcess` 集合；Folded 还需要知道变量第一次出现的位置和进入该层的
结构边：

```cpp
enum class FoldEdgeKind {
  Root,
  RecordField,
};

struct FoldPathFrame {
  PolarCompactTypeKey fullKey;
  SimpleVarSet vars;
  FoldEdgeKind edgeKind;
  std::string fieldName;
  std::size_t depth;
};

struct FoldRequest {
  std::size_t entryIndex;
  SimpleType repeatedVar;
  bool polarity;
  std::string recursiveField;
  std::size_t cycleLength;
};
```

检测顺序固定为：

1. 完整 `(CompactType, polarity)` 已在路径中：继续使用 Exact 打结，不触发 Folded。
2. 完整状态不同，但当前 vars 中有同极性变量在路径中出现：生成 `FoldRequest`。
3. 多个变量同时重复：选择 cycle 长度最短的；长度相同按 variable id 排序。

发现阶段只记录引用和索引，不创建 bucket、不分配 fresh recursion variable，也不写全局 trace 状态。

## 3. 验证可支持的 cycle

第一阶段只有同时满足以下条件才进入 Folded：

- 从 `entryIndex` 到当前帧的每一步都经过同名 record field。
- 路径极性不变化。
- 骨架变量沿该字段形成一个可完整枚举的单后继 SCC。
- 组合状态可以沿记录字段继续遍历，且不会出现未识别的分支。
- 参与一层合并的 compact body 只包含 vars、prims、size 和 record；出现 function、ptrLoad、
  ptrStore 时回退 Exact。
- 骨架 SCC 不会通过 variable-only bounds 合并成其他 closure。

验证时从原始 `SimpleType` bounds 建立带 `(variable, polarity)` 的局部依赖图。边同时记录 record
字段名，不能只看“下一个变量是谁”。这样可以避免某个变量在其他字段出现时被误认为当前 cycle。

如果验证失败，记录明确的 fallback reason，然后直接运行 Exact。不能保留半成品 bucket，也不能用
`Top` 代替未支持部分。

## 4. Folded 专用可变 accumulator

不要让 `FoldBucket::source/body` 在每次更新时保存一个新的 `CompactTypePtr`。新增只在 Folded root
内部使用的 `MutableCompactAccumulator`：

```cpp
class MutableCompactAccumulator {
public:
  // 返回本次 append 是否让抽象值发生变化，用于记录 bucket 更新。
  bool append(const CompactTypePtr &input, bool polarity);

  // 只允许在状态遍历完成后调用。递归目标已经替换成 recVar，
  // 因此 freeze 生成的仍是有限 CompactType 树/DAG。
  CompactTypePtr freeze(CompactTypeArena &arena) &&;

private:
  SimpleVarSet vars;
  SimplePrimSet prims;
  std::optional<std::uint32_t> size;
  std::optional<std::map<std::string, MutableCompactSlot>> record;
};
```

`MutableCompactSlot` 在只有一个输入时可以借用不可变 child handle；第二个相同字段到来时才提升为
子 accumulator，并在该 accumulator 内继续合并。这样不需要提前复制整棵 record，也不会为每一次
字段冲突生成 arena 节点。

第一阶段 accumulator 只实现已验证的 vars/prims/size/record 规则：

- vars、prims：把新成员插入当前集合，是否插入成功就是 change signal。
- size：沿用当前 `mergeCompactTypes()` 的 max 规则。
- 正位置 record：保留共同字段；字段消失视为变化。
- 负位置 record：保留全部字段；新增字段视为变化。
- 同名字段：保持当前极性，递归进入 child accumulator。
- cycle field：不递归构造 child，而是放入下一个 bucket 的 fresh recVar。

function 和 pointer 不在 accumulator 里做一半支持。发现这些结构时应在进入固定点前回退 Exact。

可变 accumulator 不实现 `operator<`、`operator==` 或 structural hash，也不能放进现有 cache/map key。
change detection 由插入、删除和 child change 逐层返回，不通过反复 freeze 后比较 `CompactType`。

## 5. 遍历实际组合状态并固定 bucket

发现 partial repeat 后，继续按 record field
遍历实际可达的组合状态，直到完整 `(CompactType, polarity)` 状态重复。
因此 2/3、3/5、5/7 等场景仍分别访问 6、15、35 个组合状态。

骨架 cycle 长度为 `m` 时创建 `m` 个 bucket：

```cpp
struct FoldBucket {
  SimpleType recVar;
  bool polarity;
  MutableCompactAccumulator body;
  std::size_t successorBucket;
  std::uint64_t version;
};
```

相对骨架起点深度为 `d` 的实际组合状态进入 `bucket[d % m]`。状态的一层 body 直接追加到
该 bucket 的 accumulator；cycle field 的后继记录为下一个 bucket，其他字段进入当前 bucket
或其子 accumulator。这样保留了实际组合状态的相位关系，同时只发布骨架数量的递归 bucket。

遍历和更新规则：

1. 组合状态第一次出现时展开一层，并按 `d % m` 追加到对应 bucket。
2. 完整状态重复时使用现有 Exact 打结规则结束这次状态遍历。
3. `append()` 返回变化时增加 bucket version；同一个状态不重复追加。
4. 所有实际状态处理完成后，bucket 聚合完成。

第一阶段输入是有限 SCC，状态遍历最多处理完整状态表中的每个状态一次。实现仍要设置独立的
状态数量和 bucket 更新上限；超过上限视为实现错误或 unsupported case，丢弃本 root 的 Folded
尝试并运行 Exact，不能发布未稳定结果。

## 7. freeze 和发布

freeze 必须严格晚于状态遍历：

1. 按 bucket index 顺序递归 freeze 非 cycle child accumulator。
2. 每个 bucket 生成一个有限 body，cycle field 中只放预先分配的下一个 bucket recVar。
3. 形成 root 局部 `recVars` 后，检查所有 fresh recVar 都有 bound，且所有 bound 可达。
4. 计算 variable origins，按 root 顺序提交到 `TypeSimplifier`。
5. 销毁本 root 的 path、状态表、accumulator 和遍历队列。

发布以后不再修改任何 `CompactType` 或它的 child。这样 `cachedHash`、unordered recursion key、
`go0Cache` 和后续并行 local simplify 的假设都保持不变。

为了验证没有重新制造 merge 前缀，Folded metrics 至少记录：

```text
fold_requests
folded_roots
folded_fallback_exact
folded_buckets
folded_combination_states
folded_bucket_updates
folded_accumulator_promotions
folded_freeze_nodes
folded_arena_nodes
folded_max_path_depth
```

`folded_freeze_nodes` 应接近最终可达 Folded body 节点数；如果它随状态遍历次数一起增长，说明实现
仍在循环中物化不可变中间结果。

## 8. 接入 bulk simplify

`bulkSimplifyDetailed()` 按 mode 分流：

```text
Exact:
  保持当前并行 canonicalize 和共享递归状态

Folded:
  canonicalize 阶段串行
  每个 root 先尝试 Folded，失败则 Exact fallback
  成功结果按 root 顺序提交
  后续 occurrence analysis、simplify、coalesce 继续复用现有实现
```

Folded 成功结果最终仍是普通 `CompactTypePtr + recVars`，不能把 accumulator 或 bucket id
泄漏到后续阶段。现有 `analyzeOccurrences()`、`applySimplificationPlan()` 和
`coalesceCompactType()` 不应增加 Folded 专用分支。

# 实现阶段

## 阶段 0：接口和基线

- 增加 mode、trace 和 metrics，但 Exact 行为不变。
- 为 cycle 2/3 记录当前 Exact 的状态数、arena node 数和最终递归层数。
- 在 memcached frozen stage-B 输入上只加 partial repeat 统计，不启用折叠，确认真实命中率。

只有真实命中率足够时，才能把 Folded 当作 memcached 内存候选；命中率低时仍继续完成合成场景功能，
但不扩大性能结论。

## 阶段 1：可变 accumulator

- 只实现 vars/prims/size/record。
- 用当前 `mergeCompactTypes()` 作为 oracle，比较正负位置的有限 record 输入。
- 验证 append 期间 `CompactTypeArena::make()` 计数不增长，freeze 后才增长。
- 不接 Folded 检测，先把这个局部组件单独测清楚。

## 阶段 2：cycle 2/3

- 实现有序 path、partial repeat 和支持条件验证。
- 遍历 cycle 2 和 cycle 3 的六个组合状态，并将它们追加到两个 bucket。
- 接入 freeze、local recVars 和现有后处理。
- 验证 Exact 六状态不变，Folded 最多两个骨架 bucket。

## 阶段 3：一般单后继 cycle

- 推广到任意 `m/n`，加入 3/5、5/7。
- 检查每个可达组合状态只遍历一次，再按骨架位置追加到 bucket。
- 增加负位置 record；只有关系测试通过后才算支持负位置。

## 阶段 4：真实输入和内存判断

- 使用同一 frozen 输入、同一线程数、同一 allocator 和同一停止条件比较 Exact/Folded。
- 同时记录 partial repeat、fallback、bucket、freeze node、arena node 和 RSS。
- 若 Folded 命中少，不把 RSS 无变化视为 accumulator 失败；用合成 cycle 单独验证构造复杂度。
- 若 Folded 命中多但 arena node 仍随状态遍历次数增长，先修正 freeze 边界，不继续扩展支持范围。

# 测试

至少增加以下 binarysub 测试：

1. `mutable_accumulator_matches_record_merge`
   - 正负位置、变量、primitive、字段交集/并集和嵌套同名字段与现有 merge 结构一致。
2. `mutable_accumulator_freezes_once`
   - 多次 append 不增加 arena node，freeze 后只增加最终可达节点。
3. `cycle_fold_record_2_3`
   - Exact 六状态；Folded 两个 bucket；`I | S` 不会变成 `I`。
4. `cycle_fold_record_3_5` 和 `cycle_fold_record_5_7`
   - 访问 15/35 个组合状态，但 bucket 数等于选中的骨架长度。
5. `cycle_fold_negative_record`
   - 使用负位置合并方向，满足 `Folded <: Exact`。
6. `cycle_fold_fallback_without_side_effects`
   - function、pointer、分支 SCC 和极性翻转回退 Exact；变量编号和输出与直接 Exact 一致。
7. `cycle_fold_root_isolation`
   - 同一原始变量在单独 root 和 Folded root 中使用时互不修改。
8. `cycle_fold_hash_boundary`
   - 只有 freeze 后的节点进入 recursion key；重复 structural hash 和查找稳定。
9. `canonicalize_exact_unchanged`
   - 默认选项、显式 Exact、并行 Exact 和串行 Exact 的现有结果保持一致。

构建验证覆盖 oneTBB ON/OFF、binarysub 自测、`TypeBuilderTest`、`MLsubGeneratorTest` 和现有 LLVM IR
类型恢复 suite。真实输入产出的 IR 继续使用 LLVM 22 verifier 检查。

# 风险和处理

## mutable 状态泄漏到共享结构

这是最高风险。accumulator 不能提供 `CompactTypePtr`，不能进入 hash key，也不能被 go0 cache 返回。
API 上只保留 `append()` 和右值限定的 `freeze() &&`，让 freeze 后的 accumulator 不能继续更新。

## merge 语义复制错误

accumulator 会复制一部分 `CompactTypeArena::merge()` 规则。第一阶段只支持 record，并用旧 merge
作为结构 oracle。function/pointer 在支持前统一 fallback，避免维护两套半完整规则。

## 错误识别递归路径

仅按 vars 共现会把无关变量放进骨架。依赖图必须保留字段名和极性，并要求组合状态可完整遍历；
不能证明时回退 Exact。

## 状态遍历不终止或变化检测不准

change signal 必须来自实际集合/字段变化，不能依赖对象地址。增加版本号和更新上限；超过上限不发布结果。

## fallback 影响确定性

验证完成前不分配 fresh variable、不写 origin、不 freeze。这样失败尝试不会改变 Exact 的变量编号、hash
或输出顺序。

## 内存仍然很高

Folded 只处理触发 partial repeat 的 root，且第一阶段不优化 Exact nested merge。若 memcached 主要是普通
record merge 而不是 cycle 组合，Folded 收益会有限。此时应另开计划把 recursive mutable builder 推广到
Exact，不能继续扩大 Folded 语义范围来掩盖问题。

# 判断标准

实现正确需要同时满足：

```text
默认 Exact 行为和输出不变；
Folded 只在完整验证后的 record cycle 上启用；
正位置 Exact <: Folded，负位置 Folded <: Exact；
状态遍历期间不生成 arena-owned merge 前缀；
每个 bucket 聚合完成后只 freeze 一次；
CompactType 发布后保持不可变，递归只通过 recVars；
unsupported root 完整回退 Exact 且没有可观察副作用；
性能报告同时给出触发率、状态数、arena 节点数和峰值内存。
```

# 方案评估

- **预期效果：7/10。** 在递归 cycle 上减少最终结果节点和 merge 前缀，但仍遍历 LCM 组合状态；真实项目收益取决于
  partial repeat 命中率。
- **理解成本：6/10。** 新增局部 accumulator、cycle 验证和 freeze 边界，但不改变现有
  `CompactType` 和后处理表示。
- **维护成本：6/10。** 第一阶段只复制 record merge 规则，边界较窄；以后扩展 function/pointer 时
  必须同步旧 merge 语义和极性测试。

直接把全局 `CompactType` 改回可变看起来代码更少，但会破坏 cached structural hash、共享 DAG、cache
和并行递归检测，不采用。另一种方案是恢复引用计数 handle，让不可达中间节点自动释放；它会增加大量
handle 复制和原子引用计数成本，也不能减少重复 set 拷贝。当前更合适的方案是局部可变、稳定后 freeze。

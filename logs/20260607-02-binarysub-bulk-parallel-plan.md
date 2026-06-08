# 原始 prompt

```text
基于oneTBB的concurrent_hash_map的思路，以及之前的可以从哪里入手并行的思路，写一个logs文件，规划可以从哪里入手改成支持并行。要不同时支持用一个宏关闭并行支持？
```

# 背景

`fortune.o3.wasm.ll` 当前主要耗时在 `external/binarysub` 的
`TypeSimplifier::bulkSimplifyDetailed()`。

已有 trace 结果：

```text
roots=4437
canonicalize_ms=21051
struct_merge_ms=5855
analyze_ms=2292
origin_ms=505
simplify_ms=1093
coalesce_ms=448
total_ms=31975
```

现在最大瓶颈是 `canonicalizeType()`，第二是 `build_struct_merge_info()`。
`coalesceCompactType()` 已经被 structural hash / unordered recursion key 明显降下来了。

如果要做多 CPU，不能直接把当前 loop 并行起来。`TypeSimplifier` 里有共享可变状态：

```cpp
recursive
recVars
closureCache
newRecVars
variableOrigins
nextSyntheticUTypeVarId
```

这些状态有些只是 cache，有些会影响递归变量创建和 origin 合并。并行化要先区分清楚。

# 目标

目标是给 `bulkSimplifyDetailed()` 增加可选并行能力：

- 默认仍能单线程构建和运行。
- 可用编译宏完全关闭 oneTBB 依赖。
- 是否允许并行由 `bulkSimplifyDetailed()` / `bulkSimplify()` 的参数控制，不完全依赖环境变量。
- 打开并行后，类型语义应保持等价。
- 文本输出里的变量编号、递归变量名字、map 顺序允许有变化；如果变化影响 oracle，再单独更新 oracle 或做 canonical renaming。
- fortune Debug 用例要能看到明确收益，至少不能明显退化。

# 依赖和开关

建议使用 oneTBB：

```cpp
#include <oneapi/tbb/concurrent_hash_map.h>
#include <oneapi/tbb/parallel_for.h>
```

构建层面增加选项：

```cmake
option(BINARYSUB_ENABLE_TBB_PARALLEL
       "Enable oneTBB based parallel bulk simplifier" OFF)
```

打开时：

```cmake
find_package(TBB REQUIRED)
target_link_libraries(binarysub_lib PUBLIC TBB::tbb)
target_compile_definitions(binarysub_lib PUBLIC BINARYSUB_ENABLE_TBB_PARALLEL=1)
```

关闭时：

```cpp
#ifndef BINARYSUB_ENABLE_TBB_PARALLEL
#define BINARYSUB_ENABLE_TBB_PARALLEL 0
#endif
```

运行层面再加环境变量控制线程数：

```text
NOTDEC_BINARYSUB_THREADS=8
```

接口层面增加 options：

```cpp
struct BulkSimplifyOptions {
  bool enableParallel = false;
  std::optional<std::size_t> threadCount;
};

BulkSimplifyResult bulkSimplifyDetailed(const std::set<PolarVar> &types,
                                        bool printDebug,
                                        BulkSimplifyOptions options = {});
std::map<PolarVar, UTypePtr> bulkSimplify(const std::set<PolarVar> &types,
                                          bool printDebug,
                                          BulkSimplifyOptions options = {});
```

规则：

- 宏关闭：代码不 include oneTBB，不链接 TBB，永远走单线程。
- `options.enableParallel=false`：强制单线程，不看 `NOTDEC_BINARYSUB_THREADS`。
- `options.enableParallel=true`：允许并行。
- `printDebug=true`：强制单线程，避免 debug 输出乱序。
- 线程数优先级：
  1. `NOTDEC_BINARYSUB_THREADS`
  2. `options.threadCount`
  3. `std::thread::hardware_concurrency()`
- 最终线程数 <= 1：走单线程。

# 路线

## 1. 先搭并行入口，不改算法

先只做入口和 trace：

- 增加 `BulkSimplifyOptions`。
- 实现线程数解析 helper。
- 在 trace 里输出实际线程数。
- 单线程路径保持原样。
- 并行路径先暂时调用单线程实现，确认构建和开关没问题。

判断标准：

- `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 能正常构建。
- `BINARYSUB_ENABLE_TBB_PARALLEL=ON` 能正常构建。
- `options.enableParallel=false` 时输出和当前一致。
- `options.enableParallel=true` 且未设置环境变量时，默认线程数来自当前机器核心数。

## 2. 先清理 `coalesceCompactType()` 的静态 counter

`coalesceCompactType()` 当前有：

```cpp
static int recVarCounter = 0;
```

并行时这会有数据竞争。即使用 `std::atomic<int>` 可以避免数据竞争，也会让递归变量编号受线程调度影响。
这里更合适的是把它放进单次 coalesce 的局部状态：

```cpp
struct CoalesceContext {
  int recVarCounter = 0;
  PolarCompactTypeMap<std::string> recursive;
};
```

或者直接在 `coalesceCompactType()` 里使用局部 `int recVarCounter = 0`。

判断：

- 递归变量名在单个 `UType` 内有作用域，每个 root 各自从 `μ0` 开始应当可以接受。
- 如果实际打印逻辑把多个 root 的递归变量名当成全局名，再单独做 canonical renaming。
- 不使用 futex。这里不需要底层等待/唤醒原语。

## 3. 先并行 bulk 后半段

这是最安全的第一步。

`compactMap` 和 canonical `recVars` 已经生成后，每个 root 当前都会单独做：

```cpp
analyzeOccurrences(root)
buildLocalVariableOrigins(root)
simplifyType(root)
coalesceCompactType(root)
```

这部分本来就为每个 root 创建 `localSimplifier`，适合并行。

实现思路：

- 把 `compactMap` 转成有序 `vector`。
- `parallel_for` 按 index 处理 root。
- 每个 worker 只写自己的 result slot。
- 主线程最后按 index 顺序插回 `uMap`。

需要先处理的问题：

- `coalesceCompactType()` 的 `static recVarCounter` 已按上一步改成局部状态。
- trace 时间要 worker 本地累计，最后主线程汇总。

预期收益：

- fortune 里这部分只占十几个百分点，收益有限。
- 但这一步能验证 oneTBB 接入、线程数控制、结果合并方式。

## 4. 再看 `build_struct_merge_info()` 并行

`struct_merge_ms=5855`，占主 bulk 约 `18.3%`，值得看。

先补统计：

- record candidate 数量。
- bucket 数量。
- 每个 bucket 的 candidate size。
- conflict check 次数。
- 最大 bucket。

如果 bucket 分布适合，再并行：

- 候选收集可以按 root 并行。
- bucket 内 conflict/group 构造可以按 bucket 并行。
- 最终 `StructMergeInfo` 要排序，避免输出无意义抖动。

风险：

- 如果热点集中在一个超大 bucket，并行收益有限。
- 如果并行后 candidate id 分配顺序变化，sidecar 输出可能变化。

## 5. 最后并行 `canonicalizeType()`

这是收益最大的部分，也是风险最大的部分。

目标不是每个 root 完全独立跑一套 `TypeSimplifier`。那样简单，但会丢掉 bulk 内共享状态，可能让递归变量和 compact 结构变多。

更合适的是共享并发状态：

```cpp
closureCache
recursive
recVars
variableOrigins
```

oneTBB 的 `concurrent_hash_map` 适合这些共享表里的“查找或插入”：

```cpp
oneapi::tbb::concurrent_hash_map<Key, Value, HashCompare>
```

### 5.1 先并发化 `closureCache`

`closureCache` 是最适合先做的：

```text
key = TypeNode* + polarity
value = SimpleTypeSet
```

closure 算完后 value 不再变化。可以先查 cache，miss 时计算，再插入。

要注意：

- 多个线程可能同时 miss 并计算同一个 closure。可以接受重复计算，最后 `insert` 去重。
- 如果想避免重复计算，需要 pending 状态，但第一版不需要。

### 5.2 再处理 `recursive`

`recursive` 比较难：

```text
key = CompactTypePtr + polarity
value = fresh recursive SimpleType
```

它类似 NFA->DFA 里的全局 state-set 去重。多个线程发现同一个 compact shape 时，应该拿到同一个
fresh var。

可以用 `concurrent_hash_map` 的 accessor 做原子 get-or-create。

问题是 `recVars` 的 bound 可能后面才确定：

```text
recursive 先创建 fresh var
go1 递归完成后才写 recVars[fresh_var] = adapted
```

所以第一版可以把 `recursive` 并发化，但 `recVars` 仍用 mutex 保护普通 map。

### 5.3 `recVars` 和 `variableOrigins`

这两类有合并/更新，不适合第一版直接换成 concurrent map。

建议第一版：

- `recVars` 用一个 mutex 保护。
- `variableOrigins` 用一个 mutex 保护 origin union。
- 如果锁竞争严重，再细化成 per-key 锁或换 concurrent map。

### 5.4 fresh id 和文本输出

并行后 fresh recursive var 的创建顺序可能变化。语义上可以接受，但 `.htypes` 文本可能变。

第一版先接受文本变化：

- 以 `binarysub` 自测、fortune 能跑完、HType 人工 diff 为准。
- 如果 oracle 变化太大，再做 canonical renaming。

# 不建议的路线

- 不建议先自己写完整 concurrent hash map。
- 不建议一开始把 `canonicalizeType()` 拆成分批合并算法，改动太大。
- 不建议一开始做“每个 root 完全独立 TypeSimplifier”，可能让结果明显变粗。
- 不建议直接引 OpenMP。oneTBB 的 container 和 task API 更贴近这里的需求。

# 风险

- oneTBB 依赖会影响 binarysub 的独立构建，需要宏和 CMake option 能完全关闭。
- 并行 `canonicalizeType()` 容易引入数据竞争。
- `CompactType` structural hash/equality 如果在锁内太重，concurrent map 可能收益不明显。
- 递归变量和 synthetic id 顺序变化会导致 oracle diff。
- 如果热点是单个超大 root，并行 root 层级收益有限，仍然需要 SCC cutoff / canonicalize budget。

# 判断标准

功能：

- `./build/binarysub` 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过，或者 diff
  能解释为 HType 文本变化。
- fortune 能跑完，生成 `.ll` 和 `.htypes`。
- `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 时不需要 oneTBB，也能构建。
- `printDebug=true` 时仍走单线程。

性能：

- `options.enableParallel=false` 不明显退化。
- `NOTDEC_BINARYSUB_THREADS=1` 不明显退化。
- `NOTDEC_BINARYSUB_THREADS=8` 下 fortune Debug 有明确 wall time 下降。
- trace 中 `canonicalize_ms` / `struct_merge_ms` / total wall time 能解释变化。

可维护性：

- 关闭宏时没有 oneTBB include 和 link。
- 并行相关状态集中在少量 helper / wrapper 里，不把 lock 逻辑散到所有递归分支。
- 保留单线程路径，方便定位并行 bug。

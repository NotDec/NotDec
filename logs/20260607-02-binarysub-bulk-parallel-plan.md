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

## 4. 最后并行 `canonicalizeType()`

这是收益最大的部分，也是风险最大的部分。

目标不是每个 root 完全独立跑一套 `TypeSimplifier`。那样简单，但会丢掉 bulk 内共享状态，可能让递归变量和 compact 结构变多。

更合适的是共享并发状态：

```cpp
closureCache
go0Cache
recursive
recVars
variableOrigins
```

oneTBB 的 `concurrent_hash_map` 适合这些共享表里的“查找或插入”：

```cpp
oneapi::tbb::concurrent_hash_map<Key, Value, HashCompare>
```

不同状态要分开处理，不能都当成普通 map。

### 4.1 纯 cache：`closureCache` / `go0Cache`

`closureCache` 和 `go0Cache` 是最适合先做的：

```text
closureCache:
  key = TypeNode* + polarity
  value = SimpleTypeSet

go0Cache:
  key = TypeNode* + polarity
  value = CompactTypePtr
```

它们的 value 算完后不再变化。可以先查 cache，miss 时计算，再插入。

要注意：

- 多个线程可能同时 miss 并计算同一个 key。第一版可以接受重复计算，最后 `insert` 去重。
- 如果想避免重复计算，需要 pending 状态，但第一版不需要。
- `go0Cache` 已经在单线程里实现，且 `variableOrigins` 缺省 self 后，`go0()` 基本没有 origin 副作用。

### 4.2 union metadata：`variableOrigins`

`variableOrigins` 不是递归类型状态机，而是 metadata：

```text
SimpleType var -> set<original variable ids>
```

当前语义是：

```text
如果 var 不在 variableOrigins 里，默认 origin = {var.id}
```

并行时它可以用 oneTBB `concurrent_hash_map` 的 accessor 做 entry-level update，类似 compute：

```cpp
OriginMap::accessor acc;
map.insert(acc, key);
acc->second.insert(srcOrigins.begin(), srcOrigins.end());
```

规则：

- 只在发生非默认 origin 时插入 map。
- value update 只做 set union。
- 不需要 pending/stable。
- 不要同时持有两个 accessor。先拷贝 src origins，再更新 dst origins，避免反向锁顺序死锁。
- 不要在 accessor 持有期间递归调用 `go1()` 或做 `merge_compact_types()`。

更保守的第一版也可以让 worker 收集 thread-local origin，最后主线程 union。

### 4.3 pending/finalize 状态：`recursive` / `recVars`

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

所以 `recursive` / `recVars` 不能简单换成 concurrent map。它们更像一个状态机：

```text
missing -> pending(freshVar) -> stable(freshVar + bound)
```

建议不要拆成 `pendingMap` / `stableMap` 两个容器。用一个 concurrent map，value 里带状态。
更稳的形态是 map 只负责按 key 找到 entry，entry 自己负责 pending/stable 和等待：

```cpp
struct RecursiveEntry {
  SimpleType freshVar;
  CompactTypePtr bound;
  std::set<std::uint32_t> origins;
  enum class State { Pending, Stable };
  std::atomic<State> state = State::Pending;
  std::mutex mutex;
  std::condition_variable cv;
};

using RecursiveMap =
    oneapi::tbb::concurrent_hash_map<Key, std::shared_ptr<RecursiveEntry>, HashCompare>;
```

理由：

- 一个 map 只存一份 key / hash bucket / entry wrapper，比两个 map 省内存。
- pending -> stable 是同一个 entry 的状态变化，不需要搬移 entry，也没有迁移窗口。
- 递归引用遇到 pending entry 时可以直接返回 `freshVar`，这正是递归打断需要的行为。
- 两个 map 只有在 stable entry 很多、pending 很少、且 stable 查询是长期热点时才可能更快；这里是单次 canonicalize 构造过程，不值得先复杂化。

可以用 `concurrent_hash_map` 保证同一个 key 只有一个 entry。但不要用 map accessor 长时间保护内部字段。
正确做法是拿到 `shared_ptr<RecursiveEntry>` 后马上释放 accessor；递归、等待、finalize 都只碰 entry 自己的
mutex/cv。这样不会把 TBB map 的锁带进递归调用里。

重要规则：

- 看到 pending entry 时，如果当前位置是递归引用，应该直接返回 `freshVar`，不能等待 bound。
- 只有真正需要 final bound 的地方才等待 finalized；等待时不能持有其他 entry lock。
- `inProcess` 仍然是单次 `go1()` 调用栈的局部状态，不共享。
- `recursive` / `recVars` 可以共享，但不能让所有线程都写同一个 `recVars[freshVar]`。
- `getOrCreateRecursiveVar()` 插入 `recursive` 成功的那次 canonicalize 调用，应把这个 key 记录到当前调用局部的
  `ownedRecursiveKeys`。
- `finalizeRecursiveVar()` 仍然返回同一个 `freshVar`；但只有 `pty` 在当前调用的 `ownedRecursiveKeys` 里时，才更新
  `entry.bound` 和 `recVars[freshVar]`。
- 如果当前调用只是复用了别人已经创建的 recursive entry，就只返回 `freshVar`，不写 bound。
- 这个规则让每个 recursive entry 只有一个 logical writer，避免并行时多个线程互相覆盖 `recVars`。
- 不要在持有 entry accessor 时递归调用 `go1()`，否则容易死锁。

判断：

- 这个方案不是按 OS thread 认 owner，而是按一次 `canonicalizeType()` 调用的局部 owned set 认 owner。
- 创建 entry 的调用栈之后一定会回到对应外层 `go1()`，所以有机会 finalize。
- 这比 `Pending -> Stable` first-writer-wins 更贴近原算法：递归引用仍共享 fresh var，但 `recVars` 的定义只由创建者写。
- 并行下谁先创建 entry 仍可能受调度影响，所以文本表示可能变化；当前判断是语义应等价。
- 这块是下一轮 canonicalize 并行的核心实现点。第一轮已先完成 bulk 后半段并行。

### 4.4 fresh id 和文本输出

并行后 fresh recursive var 的创建顺序可能变化。语义上可以接受，但 `.htypes` 文本可能变。

第一版先接受文本变化：

- 以 `binarysub` 自测、fortune 能跑完、HType 人工 diff 为准。
- 如果 oracle 变化太大，再做 canonical renaming。

# 不建议的路线

- 不建议先自己写完整 concurrent hash map。
- 不建议一开始把 `canonicalizeType()` 拆成分批合并算法，改动太大。
- 不建议一开始做“每个 root 完全独立 TypeSimplifier”，可能让结果明显变粗。
- 不建议现在并行 `build_struct_merge_info()`。这块贪心分组策略后面可能还会改，先保留现有阶段级 timing；
  等它成为新的主瓶颈时再单独分析。
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

# 实现记录：并行入口和局部 coalesce counter

已实现第一阶段骨架，暂未引入实际 parallel loop。

改动：

- `external/binarysub/CMakeLists.txt`：
  - 增加 `BINARYSUB_ENABLE_TBB_PARALLEL` option，默认 `OFF`。
  - option 打开时 `find_package(TBB REQUIRED)`，链接 `TBB::tbb`，定义
    `BINARYSUB_ENABLE_TBB_PARALLEL=1`。
  - option 关闭时定义 `BINARYSUB_ENABLE_TBB_PARALLEL=0`，不 include / link oneTBB。
- `external/binarysub/include/binarysub/binarysub.h`：
  - 增加 `BulkSimplifyOptions`。
  - `bulkSimplifyDetailed()` / `bulkSimplify()` 增加 options 参数，默认 `{}`，保持旧调用兼容。
- `external/binarysub/src/binarysub.cpp`：
  - 增加 `NOTDEC_BINARYSUB_THREADS` 解析。
  - 增加 `resolve_bulk_simplify_thread_count()`。
  - `printDebug=true`、宏关闭、`enableParallel=false` 都强制 `threads=1`。
  - trace 的 `[simplify:bulk-begin]` 增加 `threads=` 和 `tbb=`。
  - `coalesceCompactType()` 的 `static int recVarCounter` 改成单次 coalesce 的局部 counter。

当前行为：

- 默认构建仍是单线程。
- `BINARYSUB_ENABLE_TBB_PARALLEL=ON` 只验证 oneTBB 依赖和宏开关，不改变算法路径。
- 这一阶段还没有实现 bulk 后半段并行，也没有并发化 `closureCache/go0Cache`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake -S external/binarysub -B /tmp/binarysub-tbb-parallel-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=ON
cmake --build /tmp/binarysub-tbb-parallel-build --target binarysub -j4
/tmp/binarysub-tbb-parallel-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/02_ConstantAddr1.ll \
  -o /tmp/notdec-binarysub-options-smoke.ll \
  --tr-level=2 \
  --gen-work-dir \
  --work-dir=/tmp/notdec-binarysub-options-smoke
```

结果：

- 默认 `binarysub` 构建和自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=ON` 构建和自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- 默认 trace 确认宏关闭路径：

```text
[simplify:bulk-begin] roots=13 threads=1 tbb=0
```

下一步：

- 实现 bulk 后半段 per-root 并行，仍不触碰 `canonicalizeType()` 的 `recursive/recVars` 共享状态。

# 实现记录：bulk 后半段 per-root 并行

已实现第二阶段。只并行 `bulkSimplifyDetailed()` 里 canonicalize 和 struct merge 之后的 per-root
local simplify，不并行 `canonicalizeType()`。

改动：

- `external/binarysub/src/binarysub.cpp`：
  - 宏打开时 include `oneapi/tbb/global_control.h` 和 `oneapi/tbb/parallel_for.h`。
  - 把 `compactMap` 转成有序 `LocalRoots` vector。
  - 增加 `LocalSimplifyResult`，每个 root 的 `UTypePtr` 和阶段耗时写入独立 slot。
  - 抽出 `processLocalRoot(index)`，串行和并行共用同一套逻辑。
  - `BINARYSUB_ENABLE_TBB_PARALLEL=1` 且 `ThreadCount > 1` 时，用 `parallel_for` 跑 local roots。
  - 用 `oneapi::tbb::global_control::max_allowed_parallelism` 限制线程数。
  - 主线程最后按 `LocalRoots` 顺序汇总 `uMap` 和各阶段耗时。
- `external/binarysub/src/binarysub-test.cpp`：
  - 增加 `test_bulk_simplify_parallel_options()`，比较 serial options 和
    `BulkSimplifyOptions{enableParallel=true, threadCount=2}` 的 bulk 输出。
- `external/binarysub/include/binarysub/binarysub-test.h`、
  `external/binarysub/src/binarysub-test-main.cpp`：
  - 接入新测试。

实现判断：

- 并行任务只读 canonical `recVars`，不改 `canonicalizeType()` 的 `recursive/recVars`。
- 每个 root 使用自己的 `OccurrenceAnalysisState` 和 `TypeSimplifier localSimplifier`。
- 多线程不写共享 `std::map`；结果先写 vector slot，最后主线程合并。
- `printDebug=true` 仍由线程数解析强制单线程。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-tbb-parallel-build
cmake -S external/binarysub -B /tmp/binarysub-tbb-parallel-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=ON
cmake --build /tmp/binarysub-tbb-parallel-build --target binarysub -j4
/tmp/binarysub-tbb-parallel-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-bulk-local-parallel-default.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-bulk-local-parallel-default.htypes
```

结果：

- 默认 `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 的 `binarysub` 自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=ON` 的 `binarysub` 自测通过，新测试覆盖
  `enableParallel=true/threadCount=2` 的 parallel options 路径。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- 默认 fortune smoke 通过：`elapsed=28.39s`，`rss_kb=185824`，相比上一轮 `28.49s` 无明显退化。

注意：

- 当前顶层 NotDec 默认构建仍是 `BINARYSUB_ENABLE_TBB_PARALLEL=OFF`，所以 fortune smoke 验证的是
  新结构的单线程 fallback。
- 要在 NotDec 主链路实际启用 bulk 后半段并行，还需要顶层 CMake 打开
  `BINARYSUB_ENABLE_TBB_PARALLEL`，并由调用方传入 `BulkSimplifyOptions{enableParallel=true}`。

# 实现记录：默认启用 bulk 后半段并行

已按当前判断把 binarysub bulk 后半段并行改成默认启用。仍保留 CMake 关闭路径。

改动：

- `external/binarysub/CMakeLists.txt:8`：`BINARYSUB_ENABLE_TBB_PARALLEL` 默认从 `OFF` 改为 `ON`。
- `external/binarysub/include/binarysub/binarysub.h:480`：`BulkSimplifyOptions::enableParallel` 默认从 `false` 改为 `true`。
- `external/binarysub/src/binarysub-test.cpp:598`：parallel options 测试里的 serial baseline 显式传 `enableParallel=false`，避免默认值变化后测试失去对比意义。
- `CMakeLists.txt:202`：新增 `NOTDEC_ENABLE_BINARYSUB_PARALLEL`，默认 `ON`。
- `CMakeLists.txt:218`：顶层把 `NOTDEC_ENABLE_BINARYSUB_PARALLEL` 写入子模块 `BINARYSUB_ENABLE_TBB_PARALLEL` cache，避免旧 build cache 保持 `OFF`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3063`：`ConstraintsGenerator::genTypes()` 显式传 `BulkSimplifyOptions{enableParallel=true}` 调用 `bulkSimplifyDetailed()`。

验证：

```bash
cmake -S . -B ./build -G Ninja
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
rm -rf /tmp/binarysub-default-parallel-build
cmake -S external/binarysub -B /tmp/binarysub-default-parallel-build -G Ninja
cmake --build /tmp/binarysub-default-parallel-build --target binarysub -j4
/tmp/binarysub-default-parallel-build/binarysub
rm -rf /tmp/binarysub-parallel-off-build
cmake -S external/binarysub -B /tmp/binarysub-parallel-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-parallel-off-build --target binarysub -j4
/tmp/binarysub-parallel-off-build/binarysub
NOTDEC_BINARYSUB_TRACE=1 NOTDEC_BINARYSUB_THREADS=8 ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-parallel-trace.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-parallel-trace.htypes \
  --gen-work-dir \
  --work-dir=/tmp/notdec-fortune-parallel-trace
```

结果：

- 顶层 configure 通过，找到 TBB。
- 顶层 `binarysub` 自测通过。
- 顶层 `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- binarysub standalone 默认并行构建和自测通过。
- binarysub standalone 显式 `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 构建和自测通过。
- fortune 直接运行通过，默认并行耗时 `elapsed=24.50s`，上一轮默认单线程 smoke 是 `28.39s`。
- fortune 两次 `NOTDEC_BINARYSUB_THREADS=8` 输出稳定，`.ll` 和 `.htypes` 的 `diff -q` 均无差异。
- fortune trace 确认默认并行生效：

```text
[simplify:bulk-begin] roots=4437 threads=8 tbb=1
[simplify:bulk-timing] roots=4437 canonicalize_ms=12229 struct_merge_ms=5785 analyze_ms=2487 origin_ms=204 simplify_ms=891 coalesce_ms=490 total_ms=18820
```

额外检查：

```bash
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy|realworld).tr_level_2' --output-on-failure
```

结果中 `llvm_ir` 通过，`sysy` 和 `realworld` 失败。但失败原因不是并行输出不稳定：

- `sysy`：普通 clang IR 目标被识别为 `TargetArch::Other`，当前 PassManager 拒绝 `--dump-htypes`。
- `realworld/fortune`：suite 注入的 `NOTDEC_EXTRA_CONSTRAINTS` 里 `ir_anchor.sha256` 与当前 IR 不匹配。

这两个问题与本次 TBB 默认启用无直接关系，后续应单独修测试环境或 oracle。

评分：

- 实现效果：8/10。默认并行已经生效，fortune 有可见收益，文本输出稳定。
- 理解成本：3/10。新增的是 CMake 开关和显式 options，调用路径容易看懂。
- 维护成本：3/10。TBB 依赖默认打开，但仍可用 CMake option 关闭；主要风险是不同环境没有 TBB 时默认 configure 会失败。

# 继续检查：canonicalize 并行前置判断

第 1-3 步已经完成：

- 并行入口和 TBB 开关已实现。
- `coalesceCompactType()` 的局部 counter 已处理。
- bulk 后半段 per-root 并行已实现并默认启用。
- fortune 默认并行能看到收益，输出重复运行稳定。

剩余主要是第 4 步：并行 `canonicalizeType()`。

检查 `external/binarysub/src/binarysub.cpp` 里的 `TypeSimplifier::canonicalizeType()` 后，当前不能只把
`closureCache` / `go0Cache` 换成 concurrent map 就并行 root loop。原因是同一个函数里还会写这些共享状态：

- `recursive`：递归路径上按 `CompactTypePtr + polarity` 创建 fresh var。
- `recVars`：递归展开完成后回填 fresh var 的 bound。
- `variableOrigins`：创建递归 fresh var 时写 origin。
- `closureCache` / `go0Cache`：纯 cache，但只覆盖一部分读写。

所以第 4 步的真实决策是：

- 如果继续共享一个 `TypeSimplifier` 并并行 `canonicalizeType()`，必须先实现 4.3 里的
  `recursive/recVars` pending -> stable 方案，否则会有数据竞争，也可能让同一个递归 shape 被创建成多个 fresh var。
- 如果每个 root 用独立 `TypeSimplifier` 做 canonicalize，再合并结果，实现简单一些，但会放弃 bulk 内共享递归状态，可能让输出变粗、递归变量变多。这条路线之前计划里不建议先做。

当前判断：

- `closureCache/go0Cache` 可以继续作为后续并发化的一部分，但它们不能单独解锁 canonicalize 并行。
- 下一步如果要继续拿最大收益，应该先做 `recursive/recVars` 的 entry 状态结构；这会改动核心语义路径，风险明显高于前面的 per-root local simplify。
- 在做这一步前，最好先补一个小型递归 shape 复用测试，确保并行后同形递归类型仍合并到同一个 fresh var，避免只靠 fortune smoke 判断。

# 实现记录：补递归 shape 复用测试

本次没有继续改算法，只先补第 4 步前置测试，保护后续并行 `canonicalizeType()` 时最容易破坏的行为。

改动：

- `external/binarysub/src/binarysub-test.cpp:637`：新增
  `test_bulk_simplify_reuses_recursive_shape()`。
  - 构造两个变量 `lhs/rhs`。
  - 分别给它们加入同形递归 record bound：`{next: lhs}` 和 `{next: rhs}`。
  - 用默认并行 options 跑 `bulkSimplifyDetailed()`。
  - 对两个 root 的输出做 `normalizeVariableNames()` 后比较，要求文本一致，并确认输出里仍包含递归 record 形状。
- `external/binarysub/include/binarysub/binarysub-test.h:15`：声明新测试。
- `external/binarysub/src/binarysub-test-main.cpp:44`：接入新测试。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 自测通过。

判断：

- 这个测试不是完整的并发递归状态测试，因为当前 `canonicalizeType()` 还没并行。
- 它先固定当前 bulk 语义：同形递归 bound 不应因为后续并行化而打印成不同 final form。

# 实现记录：收拢 canonicalize 递归状态操作

本次是第 4 步的准备重构，不改变算法和并行范围。

问题：

`TypeSimplifier::canonicalizeType()` 里原来直接操作：

- `recursive.find()` / `recursive[pty] = freshVar`
- `variableOrigins[freshVar] = ...`
- `recVars[freshVar] = adapted`

这些正是后面改 pending -> stable entry 时要替换的点。如果继续散在 `go1` lambda 里，后续改并发状态会更容易漏。

改动：

- `external/binarysub/include/binarysub/binarysub.h:508`：在 `TypeSimplifier` 声明几个递归状态 helper。
- `external/binarysub/src/binarysub.cpp:1047`：新增
  `collectCanonicalOriginsFromVars()`，集中处理递归 fresh var 的 origin 收集。
- `external/binarysub/src/binarysub.cpp:1056`：新增
  `inferRecursiveCompactSize()`，保留原来的 size 推断逻辑。
- `external/binarysub/src/binarysub.cpp:1079`：新增
  `getOrCreateRecursiveVar()`，集中处理 `recursive` 查找/创建和 `variableOrigins` 写入。
- `external/binarysub/src/binarysub.cpp:1093`：新增
  `finalizeRecursiveVar()`，集中处理 `recursive` 查找和 `recVars` 回填。
- `external/binarysub/src/binarysub.cpp:1233`、`external/binarysub/src/binarysub.cpp:1297`：
  `canonicalizeType()` 改为调用这些 helper。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-rec-helper.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-rec-helper.htypes
```

结果：

- `binarysub` 自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=24.69s`，`rss_kb=195764`，和上一轮默认并行 `24.50s` 基本一致。

判断：

- 这一步不是性能优化，目的是缩小后续并发化 `recursive/recVars` 的改动面。
- 下一步如果继续第 4 步，应把这些 helper 的内部实现替换成 entry 状态结构，而不是在 `go1` 里继续直接改 map。

# 实现记录：把 recursive value 改成 entry

本次继续推进第 4.3 的准备工作，仍不并行 `canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:488`：新增
  `CanonicalRecursiveEntry`。
  - `freshVar`：递归打断时返回的 fresh var。
  - `bound`：finalize 后得到的递归 bound。
  - `finalized`：当前是否已经回填 bound。
- `external/binarysub/include/binarysub/binarysub.h:496`：
  `TypeSimplifier::recursive` 从 `PolarCompactTypeMap<SimpleType>` 改成
  `PolarCompactTypeMap<CanonicalRecursiveEntry>`。
- `external/binarysub/src/binarysub.cpp:1079`：
  `getOrCreateRecursiveVar()` 改为读写 entry，但对外仍返回 `SimpleType freshVar`。
- `external/binarysub/src/binarysub.cpp:1097`：
  `finalizeRecursiveVar()` 改为设置 `entry.bound` / `entry.finalized`，同时继续维护现有
  `recVars[freshVar] = bound`，避免影响后续 simplify/coalesce 流程。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-rec-entry.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-rec-entry.htypes
```

结果：

- `binarysub` 自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=24.65s`，`rss_kb=195328`，和前两轮默认并行基本一致。

判断：

- 这一步只改变 `recursive` 的 value 形状，不改变语义，也不引入锁。
- `recVars` 仍然保留，因为后续 `analyzeOccurrences()`、`simplifyType()`、`coalesceCompactType()` 还依赖它。
- 下一步可以考虑把 entry 改成可等待状态，例如 `shared_ptr<Entry> + mutex/cv`，再讨论是否替换成 oneTBB concurrent map。

# 实现记录：把 recursive entry 改成 shared_ptr 状态对象

本次继续推进第 4.3 的数据结构准备，仍不并行 `canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:488`：
  `CanonicalRecursiveEntry` 增加 `State { Pending, Stable }`，替换上一版 `bool finalized`。
- `external/binarysub/include/binarysub/binarysub.h:498`：
  `TypeSimplifier::recursive` 改成
  `PolarCompactTypeMap<std::shared_ptr<CanonicalRecursiveEntry>>`。
- `external/binarysub/src/binarysub.cpp:1079`：
  `getOrCreateRecursiveVar()` 改为返回 `entry->freshVar`，miss 时创建 `shared_ptr` entry，初始状态为
  `Pending`。
- `external/binarysub/src/binarysub.cpp:1098`：
  `finalizeRecursiveVar()` 改为通过 `shared_ptr` 更新 `entry.bound`，并把状态改成 `Stable`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-rec-entry-shared.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-rec-entry-shared.htypes
```

结果：

- `binarysub` 自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=24.87s`，`rss_kb=194500`，仍在默认并行同一量级。

判断：

- 这一步仍不引入锁，也不改变 `go1()` 的执行路径。
- `shared_ptr<Entry>` 是为后续 oneTBB concurrent map 做准备：map accessor 只负责找到 entry，后续递归和等待可以只操作 entry 本身。
- 下一步如果继续，需要决定是否现在就加 `mutex/cv`，还是先把 `recursive` 容器替换成 oneTBB concurrent map。

# 实现记录：给 recursive entry 加内部锁

本次继续推进第 4.3 的 entry 状态结构，仍不并行 `canonicalizeType()`，也不替换 `recursive` 容器。

改动：

- `external/binarysub/include/binarysub/binarysub.h:8`、`external/binarysub/include/binarysub/binarysub.h:15`：
  引入 `condition_variable` 和 `mutex`。
- `external/binarysub/include/binarysub/binarysub.h:490`：
  `CanonicalRecursiveEntry` 增加构造函数、`mutex` 和 `condition_variable`。
- `external/binarysub/src/binarysub.cpp:1089`：
  `getOrCreateRecursiveVar()` 改成直接构造 `shared_ptr<CanonicalRecursiveEntry>(freshVar)`。
- `external/binarysub/src/binarysub.cpp:1093`：
  `finalizeRecursiveVar()` 在 entry 自己的锁内更新 `bound/state`，解锁后 `notify_all()`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-rec-entry-lock.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-rec-entry-lock.htypes
```

结果：

- `binarysub` 自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=24.85s`，`rss_kb=195768`，没有明显退化。

判断：

- 这一步只是让 entry 具备后续等待/唤醒能力。
- 当前还没有等待路径，所以不会改变单线程 canonicalize 行为。
- 下一步如果继续，应优先把 `recursive` 容器替换成 oneTBB concurrent map，并保持规则：拿到 `shared_ptr<Entry>` 后尽快释放 map accessor。

# 实现记录：把 recursive 容器切到 oneTBB concurrent map

本次把第 4.3 的 `recursive` 容器切到 oneTBB concurrent map，但仍不并行 `canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:26`：在公共头里补
  `BINARYSUB_ENABLE_TBB_PARALLEL` 默认值，保证 standalone OFF 构建也能包含该头。
- `external/binarysub/include/binarysub/binarysub.h:30`：宏打开时 include
  `oneapi/tbb/concurrent_hash_map.h`。
- `external/binarysub/include/binarysub/binarysub.h:419`：新增
  `PolarCompactTypeHashCompare`，适配 oneTBB 的 `hash()/equal()` 接口。
- `external/binarysub/include/binarysub/binarysub.h:439`：新增
  `ConcurrentPolarCompactTypeMap<T>`：
  - 宏打开时使用 `oneapi::tbb::concurrent_hash_map`。
  - 宏关闭时退回 `PolarCompactTypeMap<T>`。
- `external/binarysub/include/binarysub/binarysub.h:535`：
  `TypeSimplifier::recursive` 改成 `ConcurrentPolarCompactTypeMap<std::shared_ptr<CanonicalRecursiveEntry>>`。
- `external/binarysub/src/binarysub.cpp:1079`：
  `getOrCreateRecursiveVar()` 在 TBB 路径用 accessor 做 get-or-create，拿到 `shared_ptr<Entry>` 后退出 accessor 作用域。
- `external/binarysub/src/binarysub.cpp:1108`：
  `finalizeRecursiveVar()` 在 TBB 路径用 const_accessor 查 entry，复制 `shared_ptr<Entry>` 后退出 accessor 作用域，再更新 entry 自身状态。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-rec-concurrent-off-build
cmake -S external/binarysub -B /tmp/binarysub-rec-concurrent-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-rec-concurrent-off-build --target binarysub -j4
/tmp/binarysub-rec-concurrent-off-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-rec-concurrent.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-rec-concurrent.htypes
```

结果：

- 默认 ON 的 `binarysub` 自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 构建和自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=25.05s`，`rss_kb=194680`，相比前几轮默认并行略慢但仍在同一量级。

判断：

- 这一步只是把 `recursive` 容器换成并发容器，还没有并行 `canonicalizeType()`。
- helper 已经按计划避免在持有 TBB accessor 时递归或等待。
- 目前 `variableOrigins` 仍是普通 `std::map`，所以还不能直接并行 canonicalize root loop。

# 实现记录：给 variableOrigins 加 mutex 保护

本次按“先简单用 mutex”的方向处理 `variableOrigins`，仍不并行 `canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:550`：
  给 `TypeSimplifier` 增加 `variableOriginsMutex`。
- `external/binarysub/include/binarysub/binarysub.h:555`：
  增加 `originsForVar()`、`appendOriginsForVar()`、`setOriginsForVar()`、
  `mergeOriginsForVars()`、`snapshotVariableOrigins()`。
- `external/binarysub/src/binarysub.cpp:1047`：
  实现上述 helper，用一把 mutex 保护 `variableOrigins` 的读、写和 snapshot。
- `external/binarysub/src/binarysub.cpp:1079`：
  `collectCanonicalOriginsFromVars()` 改为一次性锁住 `variableOrigins` 后收集 origin。
- `external/binarysub/src/binarysub.cpp:1119`、`external/binarysub/src/binarysub.cpp:1132`：
  `getOrCreateRecursiveVar()` 写 fresh var origin 时改用 `setOriginsForVar()`。
- `external/binarysub/src/binarysub.cpp:1491`：
  `computeSimplificationPlan()` 合并 origin 时改用 `mergeOriginsForVars()`。
- `external/binarysub/src/binarysub.cpp:1846`、`external/binarysub/src/binarysub.cpp:1916`、
  `external/binarysub/src/binarysub.cpp:1942`：
  `coalesceCompactType()` 读取 origin 时改用 helper。
- `external/binarysub/src/binarysub.cpp:1861`、`external/binarysub/src/binarysub.cpp:2573`：
  遍历/拷贝 `variableOrigins` 前先做 snapshot。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-origin-mutex-off-build
cmake -S external/binarysub -B /tmp/binarysub-origin-mutex-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-origin-mutex-off-build --target binarysub -j4
/tmp/binarysub-origin-mutex-off-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-origin-mutex.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-origin-mutex.htypes
```

结果：

- 默认 ON 的 `binarysub` 自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 构建和自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=24.59s`，`rss_kb=195788`，没有明显退化。

判断：

- 这一步先用简单 mutex 消除 `variableOrigins` 的直接数据竞争点。
- 当前还没有并行 `canonicalizeType()`，所以没有新增锁竞争。
- 后续如果并行 root loop，`getOrCreateRecursiveVar()` 仍会在持有 `recursive` accessor 时读取/写入
  `variableOrigins`。当前没有反向锁顺序；如果后面新增反向访问，需要再拆开 accessor 作用域。

# 实现记录：给 canonical cache 加 mutex 保护

本次继续按“先简单用 mutex”的方向处理 `closureCache` 和 `go0Cache`，仍不并行
`canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:541`：
  给 `closureCache` 增加 `closureCacheMutex`。
- `external/binarysub/include/binarysub/binarysub.h:547`：
  给 `go0Cache` 增加 `go0CacheMutex`。
- `external/binarysub/include/binarysub/binarysub.h:557`：
  增加 `lookupClosureCache()`、`storeClosureCache()`、`lookupGo0Cache()`、
  `storeGo0Cache()`。
- `external/binarysub/src/binarysub.cpp:1047`：
  实现上述 helper，用一把 mutex 保护各自 cache 的查找和插入。
- `external/binarysub/src/binarysub.cpp:1228`：
  `canonicalizeType()` 里的 `closeOver()` 改为通过 helper 访问 `closureCache`。
- `external/binarysub/src/binarysub.cpp:1271`：
  `canonicalizeType()` 里的 `go0()` 改为通过 helper 访问 `go0Cache`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-cache-mutex-off-build
cmake -S external/binarysub -B /tmp/binarysub-cache-mutex-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-cache-mutex-off-build --target binarysub -j4
/tmp/binarysub-cache-mutex-off-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-cache-mutex.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-cache-mutex.htypes
```

结果：

- 默认 ON 的 `binarysub` 自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 构建和自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=25.39s`，`rss_kb=195804`，和最近几轮默认并行结果基本同一档。

判断：

- 这一步只保护 cache 容器本身，不把 miss 后的计算放进锁里。
- 后续并行 root loop 时，cache miss 可能重复计算，但不会破坏结果。
- 现在 `recursive`、`variableOrigins`、`closureCache`、`go0Cache` 的直接共享写点都已有保护。
  下一个真正需要处理的是 `recVars`：`finalizeRecursiveVar()` 仍会写
  `recVars[freshVar] = bound`。

# 实现记录：给 recVars 回填加 mutex 保护

本次继续收口 `canonicalizeType()` 的共享写点，但仍不并行 `canonicalizeType()`。

改动：

- `external/binarysub/include/binarysub/binarysub.h:537`：
  给 `recVars` 增加 `recVarsMutex`。
- `external/binarysub/include/binarysub/binarysub.h:581`：
  增加 `setRecVarBound()`。
- `external/binarysub/src/binarysub.cpp:1202`：
  `finalizeRecursiveVar()` 回填 `recVars` 时改用 `setRecVarBound()`。
- `external/binarysub/src/binarysub.cpp:1206`：
  实现 `setRecVarBound()`，用一把 mutex 保护 `recVars[var] = bound`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-recvars-mutex-off-build
cmake -S external/binarysub -B /tmp/binarysub-recvars-mutex-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-recvars-mutex-off-build --target binarysub -j4
/tmp/binarysub-recvars-mutex-off-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-recvars-mutex.ll \
  --tr-level=2 \
  --dump-htypes=/tmp/notdec-fortune-recvars-mutex.htypes
```

结果：

- 默认 ON 的 `binarysub` 自测通过。
- standalone `BINARYSUB_ENABLE_TBB_PARALLEL=OFF` 构建和自测通过。
- `notdec` 构建通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune smoke 通过：`elapsed=25.47s`，`rss_kb=195760`，和最近几轮默认并行结果基本同一档。

判断：

- 这一步只保护 `finalizeRecursiveVar()` 对 `recVars` 的回填写入。
- 后面的 `computeSimplificationPlan()` / `applySimplificationPlan()` / `coalesceCompactType()` 仍按现有流程单线程读写
  `recVars` / `newRecVars`。
- 继续真正并行 canonicalize root loop 前，还有一个不能靠 mutex 自动解决的决策点：
  如果两个线程对同一个 recursive entry 都进入 `finalizeRecursiveVar()`，第二次 finalize 遇到已有 stable
  bound 时应该怎么处理。计划里列过三个方向：相同则忽略、不同则 merge、或者先 assert/trace。
  当前代码还是覆盖写，不能直接并行 root loop。

# 尝试记录：canonicalize root loop 并行触发 bound 冲突

本次按原计划继续尝试第 4 步，但没有保留代码改动。

尝试内容：

- 把 `finalizeRecursiveVar()` 改成幂等 finalize：
  - `Pending` 时写 `entry.bound`，变成 `Stable`，并写 `recVars`。
  - `Stable` 时如果新旧 bound 相同则忽略。
  - `Stable` 时如果新旧 bound 不同则 trace/assert。
- 把 `bulkSimplifyDetailed()` 的 canonicalize 阶段改成：
  - `types` 转为有序 vector。
  - worker 写各自的 `CompactTypePtr` result slot。
  - 主线程按输入顺序插回 `compactMap`。

结果：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
rm -rf /tmp/binarysub-canonical-parallel-off-build
cmake -S external/binarysub -B /tmp/binarysub-canonical-parallel-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-canonical-parallel-off-build --target binarysub -j4
/tmp/binarysub-canonical-parallel-off-build/binarysub
cmake --build ./build --target notdec -j4
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

这些都通过。

fortune 带 trace 的一次运行通过：

```text
elapsed=31.61 rss_kb=197028
[simplify:bulk-timing] roots=4437 canonicalize_ms=13079 struct_merge_ms=5804 analyze_ms=3208 origin_ms=254 simplify_ms=1179 coalesce_ms=642 total_ms=19400
```

但不带 trace 的 fortune 触发 assert：

```text
Assertion `false && "Recursive entry finalized with a different compact bound"' failed.
elapsed=1.57 rss_kb=157856
```

`NOTDEC_BINARYSUB_THREADS=1` 也触发同一个 assert：

```text
Assertion `false && "Recursive entry finalized with a different compact bound"' failed.
elapsed=1.67 rss_kb=156416
```

判断：

- “同一个 recursive entry 的第二次 finalize 只会得到相同 bound”这个假设不成立。
- 而且它不只是并发调度问题；在线程数为 1 的路径里也会出现不同 bound。
- 当前串行算法实际依赖 `finalizeRecursiveVar()` 后写覆盖前写的行为。
- 因此不能用简单的 `Pending -> Stable` 一次写入模型直接并行 canonicalize root loop。
- 这次代码已回退，没有提交失败实现。

后来用一个很小的 SimpleType 图复现了同 key 两次 finalize 得到不同结构 bound：

```text
v0.lowerBounds += { a: v1, b: v0 }
v1.lowerBounds += { a: v0, b: v0 }
root = { v0: v0, v1: v1 }
```

临时 assert 打印出的差异类似：

```text
key: {v0}, positive
old bound: { a: { a: t, b: t }, b: t }
new bound: { a: u, b: t }
```

这里差异来自 `inProcess` 不同：同一个 `{v0}, positive` 从不同递归路径进入时，内部字段碰到的折返点不同。
这说明 bound 的结构表示可能不同。

当前判断：

- 这不是并行才有的问题，而是共享 `recursive` / `recVars` 后就需要面对的问题。
- 这两个 bound 很可能是同一个递归类型的不同折叠表示；底层 SimpleType 图已经固定，`CompactType + polarity`
  一样时，真实类型语义应当一样。
- 代码目前没有递归类型等价判断，所以不能用结构相等来证明它们相同。
- 因此后续并行方案不应 assert “第二次 finalize 结构必须一样”，而应避免多 writer：谁创建 recursive entry，谁负责写
  `recVars`。

后续可选方向：

- 按 4.3 的 owner set 方案继续：`recursive` 共享，`recVars` 只由创建者回填。
- 保留单线程 canonicalize 结果作为基线，先看 binarysub 自测、fortune 和 htype diff。
- 如果文本输出不稳定但语义等价，再考虑 canonical renaming 或递归类型规范化。
- 暂时不做递归类型等价判断；这比 owner set 复杂很多。

# 实现记录：canonicalize 并行实验开关

本次继续推进第 4 步，但没有把 canonicalize 并行设为默认。

实现：

- `external/binarysub/include/binarysub/binarysub-core.h`
  - `VarSupply::next` 改成 `std::atomic<std::uint32_t>`，`fresh_id()` 用 relaxed `fetch_add()`。
  - 原因是 canonicalize 并行实验会从多个线程创建 fresh SimpleType 变量，旧的 `next++` 有数据竞争。
- `external/binarysub/include/binarysub/binarysub.h`
  - `CanonicalRecursiveEntry` 增加 `ownerIndex`，记录当前 recursive key 的确定性 owner 候选。
  - `canonicalizeType()` 增加默认参数 `ownerIndex = 0`。
  - `getOrCreateRecursiveVar()` / `finalizeRecursiveVar()` 增加 `ownerIndex` 和 `ownedRecursiveKeys` 参数。
- `external/binarysub/src/binarysub.cpp`
  - `getOrCreateRecursiveVar()` 在共享 `recursive` entry 上记录最小 `ownerIndex`。
  - `finalizeRecursiveVar()` 只有当前 root 是 entry owner 时才写 `entry.bound` 和 `recVars`。
  - `bulkSimplifyDetailed()` 的 canonicalize 阶段改成 vector result slot；只有设置
    `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1` 时才用 TBB 并行这个阶段。
  - 默认路径仍串行 canonicalize，只保留已稳定的 bulk 后半段并行。

为什么没有默认打开：

- 设置 `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1` 后，fortune 能跑通，性能明显更好。
- 但 repeated fortune `.htypes` 仍不稳定。即使把 `rec_数字` 归一化，仍能看到递归折叠点不同导致的结构差异。
- `.ll` 在简单归一化 `rec_数字` 后一致，说明当前更像是递归类型表示差异，不像明显语义错误。
- 但当前 oracle 依赖文本结果，默认路径不能引入这种不稳定。

验证：

```bash
cmake --build ./build --target binarysub notdec -j4
./build/binarysub
rm -rf /tmp/binarysub-canon-exp-off-build
cmake -S external/binarysub -B /tmp/binarysub-canon-exp-off-build -G Ninja \
  -DBINARYSUB_ENABLE_TBB_PARALLEL=OFF
cmake --build /tmp/binarysub-canon-exp-off-build --target binarysub -j4
/tmp/binarysub-canon-exp-off-build/binarysub
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果都通过。

fortune 默认路径重复运行稳定：

```text
elapsed=30.62 rss_kb=195244
elapsed=30.75 rss_kb=194396
default htypes repeat identical
default ll repeat identical
```

canonicalize 并行实验路径：

```bash
NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1 ./build/bin/notdec ...
```

代表性结果：

```text
elapsed=16.17 rss_kb=202160
elapsed=16.13 rss_kb=201808
```

但 `.htypes` 重复运行不稳定，所以只保留为实验开关。

评分：

- 实现效果：6/10。默认路径保持稳定；实验路径证明 canonicalize 并行有明显收益，但文本稳定性未解决。
- 理解成本：6/10。`recursive` entry 增加 owner 规则，读代码时需要理解 owner 只约束 `recVars` 回填。
- 维护成本：5/10。实验开关隔离了风险；后续如果要默认启用，需要先做递归类型 canonical renaming 或更强的规范化。

后续更好的方案：

- 不再继续靠调度中的 owner 选择修补文本稳定性。
- 如果要默认打开 canonicalize 并行，应先在 UType/HType 输出前做递归类型稳定化，把等价但折叠点不同的递归表示归一。
- 另一条路是 canonicalize 阶段并行预计算，最后按 root 顺序串行提交 recursive/recVars，但这会损失一部分收益。

# 实现记录：canonicalize 状态数 progress trace

目标：

- 类比 NFA 到 DFA 的构造过程，在 `canonicalizeType()` 里记录已经展开过多少个
  `(CompactType, polarity)` 状态。
- 只通过现有 `NOTDEC_BINARYSUB_TRACE` 输出，避免默认 stderr/stdout 变化影响测试 oracle。
- 每新增约 1000 个状态输出一次，canonicalize 阶段结束再输出一次最终状态数和耗时。

实现：

- `external/binarysub/include/binarysub/binarysub.h:551`
  - 在 `TypeSimplifier` 里增加 `canonicalizeProgressStates`、
    `nextCanonicalizeProgressReport` 和 `canonicalizeProgressMutex`。
  - 这个集合只在 trace 打开时使用，key 和递归检测使用的 `(CompactType, polarity)` 一致。
- `external/binarysub/include/binarysub/binarysub.h:574`
  - 增加 `traceCanonicalizeProgress()` 和 `canonicalizeProgressStateCount()`。
- `external/binarysub/include/binarysub/binarysub.h:624`
  - `isClear()` / `clear()` 覆盖新增 trace 状态，保证 `TypeSimplifier` 生命周期检查仍然完整。
- `external/binarysub/src/binarysub.cpp:1081`
  - 实现 `traceCanonicalizeProgress()`：trace 没开直接返回；trace 开启时用 mutex 保护 set。
  - 达到 1000、2000、3000 这类阈值时输出
    `[simplify:canonicalize-progress] states=N`。
- `external/binarysub/src/binarysub.cpp:1408`
  - 在 `canonicalizeType()` 的 `go1()` 里，对每个非空 `pty = (ty, pol)` 记录一次。
- `external/binarysub/src/binarysub.cpp:2708`
  - 在 bulk canonicalize 阶段结束后输出
    `[simplify:canonicalize-final] states=N ms=M`。

验证：

```bash
cmake --build ./build --target binarysub -j4
./build/binarysub
cmake --build ./build --target notdec -j4
rm -rf /tmp/notdec-canon-progress
NOTDEC_BINARYSUB_TRACE=1 /usr/bin/time -f 'elapsed=%e cpu=%P rss_kb=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-canon-progress.ll --tr-level=2 \
  --gen-work-dir --work-dir=/tmp/notdec-canon-progress \
  --dump-htypes=/tmp/notdec-canon-progress.htypes
rg "canonicalize-progress|canonicalize-final|bulk-timing|bulk-compact-stats" \
  /tmp/notdec-canon-progress/binarysub-trace.log
```

结果：

```text
./build/binarysub: passed
fortune elapsed=36.10 cpu=121% rss_kb=199072

[simplify:canonicalize-progress] states=1000
[simplify:canonicalize-progress] states=2000
[simplify:canonicalize-progress] states=3000
[simplify:canonicalize-progress] states=4000
[simplify:canonicalize-progress] states=5000
[simplify:canonicalize-final] states=5078 ms=12629
[simplify:bulk-timing] roots=4437 canonicalize_ms=12629 struct_merge_ms=5860 analyze_ms=3327 origin_ms=275 simplify_ms=1199 coalesce_ms=7817 total_ms=24257
[simplify:bulk-compact-stats] roots=4437 nodes=15657 vars=51164 prims=6376 records=2129 functions=116 ptrs=4484 max_depth=20 rec_vars=79
```

评分：

- 实现效果：8/10。可以直接看到 canonicalize 状态数增长，fortune 主 bulk 最终是 5078 个状态。
- 理解成本：2/10。只是 trace 统计，不改变算法结果。
- 维护成本：2/10。状态挂在 `TypeSimplifier` 生命周期里，trace 不开时只有一次分支判断。

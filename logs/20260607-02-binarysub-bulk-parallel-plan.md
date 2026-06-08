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
- finalize 可能被多个线程尝试，不能假设只回填一次。
- 如果已有 bound 和新 bound 相同，可以忽略；如果不同，不能覆盖，至少要 merge 或先 trace/assert。
- 不要在持有 entry accessor 时递归调用 `go1()`，否则容易死锁。

判断：

- 单线程当前通常是 `recursive` 创建一次、`recVars` 回填一次。
- 并行共享后不能依赖这个“一次回填”假设。
- 这块不作为第一轮实现。第一轮 canonicalize 并行优先只共享纯 cache 和 origin metadata。

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

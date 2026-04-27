# 2026-04-27 PointerAnalysis trace stats

## 目的

增强 `binarysub-trace.log`，让它能直接反映 PA solve 阶段的慢点，而不是只看到：

- `PA` 最终 points-to 结果
- `PA` 派生出来的类型约束

之前的问题是：

- `binarysub-trace.log` 里已有的 `[pa:*]` 主要在
  `flushPointerDerivedTypeConstraints()`。
- `fortune.o3.wasm.ll` 的真实热点却在
  `PointerAnalysis::solve()` -> `addPointsTo()` -> `std::set<MemoryLocKey>::insert()`
  这条链上。

所以当时看 workdir trace，不容易直接看出：

1. 到底增长了多少 loc。
2. 重复插入有多少。
3. 哪个 slot 的 points-to set 最大。
4. path 长度是在涨，还是 slot 内对象数在涨。

## 修改文件与行号

### 1. `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h`

- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:66`
  - 新增 `AddPointsToStats`，统计：
    - `Attempts`
    - `Inserted`
    - `Duplicates`
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:100`
  - `PointerAnalysis` 新增成员 `AddStats`。

### 2. `src/TypeRecovery/mlsub/PointerAnalysis.cpp`

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:19`
  - 新增 `PASolveReasonStats`，统计每轮按 reason 的 attempts/inserted。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:24`
  - 新增 `PASolveSnapshot`，汇总：
    - `slots`
    - `locs`
    - `plain_locs`
    - `star_locs`
    - `star_elems`
    - `max_set`
    - `max_slot`
    - `max_path_elems`
    - `max_path_loc`
    - `top_slots`
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:40`
  - 新增 `countPathStars()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:50`
  - 新增 `collectPASolveSnapshot()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:92`
  - 新增 `formatPASolveSnapshot()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:118`
  - 新增 `formatPAReasonStats()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:149`
  - `PointerAnalysis::addPointsTo()` 新增聚合计数：
    - 每次尝试插入先加 `Attempts`
    - 重复 key 记 `Duplicates`
    - 成功插入记 `Inserted`
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:213`
  - `PointerAnalysis::solve()` 开始时写：
    - `[pa:solve-begin]`
    - 边数量
    - 初始 snapshot
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:225`
  - 每轮 solve 统计 `copy` / `field` / `load_ptr` / `store_ptr` 的
    attempts/inserted。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:300`
  - 每轮结束写：
    - `[pa:solve-iter]`
    - 当前 snapshot
    - 按 reason 的 attempts/inserted/duplicates
    - total attempts/inserted/duplicates
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:317`
  - solve 结束写：
    - `[pa:solve-end]`
    - 最终 snapshot
    - total attempts/inserted/duplicates

## 涉及函数

- `notdec::mlsub::PointerAnalysis::addPointsTo`
- `notdec::mlsub::PointerAnalysis::solve`
- `notdec::mlsub::collectPASolveSnapshot`
- `notdec::mlsub::formatPASolveSnapshot`
- `notdec::mlsub::formatPAReasonStats`

## 验证

### 1. 编译

```bash
cmake --build build --target notdec-decompile -j4
```

### 2. 单测

```bash
./build/bin/PAPathTest
```

通过。

### 3. 小 case trace

```bash
env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_shadow.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-field-cycle-shadow-trace3
```

`binarysub-trace.log` 里能看到：

- `[pa:solve-begin]`
- `[pa:solve-iter]`
- `[pa:solve-end]`

而且可以直接看出：

- 第 5 轮收敛
- `total_duplicates=29`
- 最大 points-to set 是 `value(main::%p):3`

### 4. fortune trace

```bash
timeout 25s env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-shadow.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-fortune-shadow-trace3
```

现在 `binarysub-trace.log` 已经能直接反映问题。

关键片段：

- `iter=10`
  - `locs=1801`
  - `max_set=240`
  - `total_duplicates=4030`
- `iter=12`
  - `locs=4255`
  - `max_set=576`
  - `total_duplicates=9362`
- `iter=14`
  - `locs=9949`
  - `max_set=1344`
  - `total_duplicates=22050`
  - `max_path_elems=7`
  - `top_slots=value(main::%_local_3.12):1344,...`

这说明 fortune 当前慢点已经可以直接从 log 看出来：

1. 不是只有 path 变长。
2. 更主要的是一些热点 slot 的 points-to set 急剧膨胀。
3. 同时重复插入非常多，`MemoryLocKey` 比较成本很高。

## 结论

这次没有加新的独立日志文件，只增强了现有 `binarysub-trace.log`。

增强后，PA solve 阶段至少已经能直接回答：

- 迭代轮数
- 每轮新增主要来自 copy 还是 field
- slot / loc 总量
- star path 规模
- 最大 path 长度
- 最大 points-to set 的 slot
- 重复插入累计次数

这样后面再看 `fortune`，不用先上 gdb，也能直接从 workdir trace 看出当前瓶颈。

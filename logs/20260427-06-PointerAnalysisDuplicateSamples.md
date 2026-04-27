# 2026-04-27 PointerAnalysis duplicate samples

## 目的

上一版 `binarysub-trace.log` 已经能看到：

- `total_duplicates`
- `max_set`
- `max_slot`
- `locs`
- `max_path_elems`

但用户继续追问的是：

- duplicate 到底是哪些对象在重复？
- 是 `copy` 还是 `field` 在重复？
- 能不能直接从 log 看出“它怎么 duplicate 很多”？

所以这次继续增强 `[pa:solve-iter]`，给每轮补少量 duplicate 样本。

## 修改文件与行号

### 1. `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h`

- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:65`
  - 新增 `DuplicateSample`。
  - 字段：
    - `Reason`
    - `SlotKey`
    - `Loc`
    - `Count`
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:99`
  - `PointerAnalysis` 新增：
    - `IterDuplicateSamples`
    - `IterDuplicateSampleOverflow`
    - `CollectIterDuplicateSamples`
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:102`
  - 新增 `recordDuplicateSample()` 声明。

### 2. `src/TypeRecovery/mlsub/PointerAnalysis.cpp`

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:127`
  - 新增 `formatPADuplicateSamples()`。
  - 负责把每轮 duplicate 样本格式化成：

```text
dup_samples=reason@slot->loc:count,...
```

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:149`
  - `PointerAnalysis::addPointsTo()` 在 duplicate 分支上新增：
    - 如果当前开启 solve 迭代样本收集，
    - 调用 `recordDuplicateSample(SlotKey, *It, Reason)`。

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:167`
  - 新增 `PointerAnalysis::recordDuplicateSample()`。
  - 当前策略：
    - 每轮最多保留 4 条样本
    - 命中已有样本则 `Count++`
    - 超出样本上限则累计到 `IterDuplicateSampleOverflow`

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:228`
  - 每轮 solve 开始前清空：
    - `IterDuplicateSamples`
    - `IterDuplicateSampleOverflow`
  - 并根据 `binarysub_trace_enabled()` 决定是否收集。

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:309`
  - `[pa:solve-iter]` 末尾追加：

```text
dup_samples=...
```

## 涉及函数

- `notdec::mlsub::PointerAnalysis::addPointsTo`
- `notdec::mlsub::PointerAnalysis::recordDuplicateSample`
- `notdec::mlsub::formatPADuplicateSamples`

## 验证

### 1. 小 case

命令：

```bash
env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_shadow.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-field-cycle-shadow-trace4
```

现在每轮可看到：

- `copy@value(main::%base)->main::stack(main::%buf):1`
- `copy@value(main::%p)->main::stack(main::%buf):1`
- `field@value(main::%next)->main::stack(main::%buf).@1[32]:1`

这已经能直接看出：

1. duplicate 既有 `copy`，也有 `field`
2. 重复的是哪些 slot / loc
3. 小循环里主要是在反复传播已有 loc

### 2. fortune

命令：

```bash
timeout 25s env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-shadow.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-fortune-shadow-trace4
```

现在 trace 至少已经能看出 duplicate 的形态：

- `iter=1`
  - `copy@value(add_file::%_local_7.1)->add_file::heap(add_file::%bb.blk_exit24.i0):3`
  - `copy@value(add_file::%_local_7.1)->add_file::heap(add_file::%bb.blk_exit7.i4):9`

- 到 `main` 那个大 SCC 后：
  - 大量 duplicate 样本都落在
    `main::heap(main::%bb.blk_exit39.i1)` 及其派生 field path 上
  - 例如：
    - `copy@value(main::%bb.blk_exit284.i1)->main::heap(main::%bb.blk_exit39.i1):1`
    - `copy@value(main::%bb.blk_exit284.i1)->main::heap(main::%bb.blk_exit39.i1).@1[32]:1`
    - `copy@value(main::%bb.blk_exit284.i1)->main::heap(main::%bb.blk_exit39.i1).@1.@4[32]:1`
    - `copy@value(main::%bb.blk_exit284.i1)->main::heap(main::%bb.blk_exit39.i1).@1.@4.@1[32]:1`

同时能和聚合统计对上：

- `iter=10`
  - `copy duplicates=502`
  - `field duplicates=940`
  - `dup_samples ... other:1438`
- `iter=14`
  - `copy duplicates=2938`
  - `field duplicates=5260`
  - `dup_samples ... other:8194`

## 目前能从 log 直接看出的事

1. duplicate 不是抽象的“很多重复”，而是具体落在某些热点 slot 上。
2. 大量重复命中的对象，集中在：
   - `main::heap(main::%bb.blk_exit39.i1)`
   - 以及它的多层 field 派生 path
3. duplicate reason 既有 `copy`，也有 `field`，并且后期 `field duplicates`
   很高。
4. 也就是说：
   - 不是 path 只是单向增长
   - 而是同一批对象路径被很多不同 copy / field 传播反复尝试插入

## 局限

当前 duplicate sampler 是“每轮前 4 条样本 + overflow 计数”，主要目的是：

- 不让 trace 体积爆炸
- 先看出 duplicate 的对象形态

它的局限是：

- `fortune` 这种大例子里 duplicate 太多太散，
- 前 4 条样本不一定就是“最频繁”的 4 条，
- 所以很多样本 `Count` 还是 `1`，
- 真正的大头落在 `other:N` 里。

## 下一步建议

如果还想让 log 更直接，可以继续做一版更强的聚合：

1. `top duplicate slots`
   - 每轮按 slot 聚合 duplicate 次数
2. `top duplicate locs`
   - 每轮按 loc 聚合 duplicate 次数
3. `top duplicate reason+slot`
   - 直接看哪类传播对哪个 slot 最糟糕

这样会比当前“前 4 条样本”更适合 fortune 这种大规模 case。

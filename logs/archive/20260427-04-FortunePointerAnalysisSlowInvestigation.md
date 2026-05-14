# 2026-04-27 fortune pointer analysis slow investigation

## 目标

继续调查为什么：

- `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`

在：

- `NOTDEC_POINTER_ANALYSIS_MODE=original`

下耗时明显短，

而在：

- `NOTDEC_POINTER_ANALYSIS_MODE=shadow`
- `NOTDEC_POINTER_ANALYSIS_MODE=replace`

下耗时明显变长。

## 计时结果

统一命令：

```bash
env NOTDEC_POINTER_ANALYSIS_MODE=<mode> ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/out.ll --tr-level=2 --frozen-tr-input-ir -g --work-dir=/tmp/wd
```

实测：

- `original`
  - 约 `12.35s`
  - 正常结束
- `shadow`
  - `30s timeout`
- `replace`
  - `30s timeout`

说明问题确实只在 PA 模式打开后出现。

## 短时 trace 对比

用：

```bash
timeout 10s env NOTDEC_POINTER_ANALYSIS_MODE=<mode> NOTDEC_BINARYSUB_TRACE=1 ...
```

观察：

### `original`

10 秒时 trace 已经进入：

- `simplify:bulk-*`

也就是已经走过 `PG.solve()`，进入后面的简化阶段。

### `shadow` / `replace`

10 秒时 trace 尾部还停在大 SCC 的：

- `pndiff:solve-end ...`

当时我一开始看上去像是“还卡在 PNDiff”。但后面结合 gdb 采样看，这个判断不完整：

- trace 到 `pndiff:solve-end` 后，
- 程序其实已经继续往后跑，
- 只是 `PA.solve()` 本身没有像 `flushPointerDerivedTypeConstraints()` 那样持续写
  `[pa:*]` trace，
- 所以 trace 文件看起来像是“停在 PNDiff 尾部”。

## gdb 采样结果

对 `shadow` 模式前台运行 8 秒后 `Ctrl-C`，主线程栈落在：

- `notdec::mlsub::operator<(PAPathAtom, PAPathAtom)`
  - `src/TypeRecovery/mlsub/PAPath.cpp:146`
- `notdec::mlsub::operator<(PAPathStar, PAPathStar)`
  - `src/TypeRecovery/mlsub/PAPath.cpp:154`
- `notdec::mlsub::operator<(PAPath, PAPath)`
  - `src/TypeRecovery/mlsub/PAPath.cpp:182`
- `notdec::mlsub::MemoryLocKey::operator<`
  - `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:39`
- `std::set<MemoryLocKey>::insert`
- `notdec::mlsub::PointerAnalysis::addPointsTo`
  - `src/TypeRecovery/mlsub/PointerAnalysis.cpp:39`
- `notdec::mlsub::PointerAnalysis::solve`
  - `src/TypeRecovery/mlsub/PointerAnalysis.cpp:109`
- `notdec::mlsub::ConstraintsGenerator::run`
  - `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:167`

这说明：

1. 慢点不在 summary 读取。
2. 慢点也不在 `flushPointerDerivedTypeConstraints()`。
3. 慢点已经明确落在 `PointerAnalysis::solve()` 的 `addPointsTo()` 去重插入。
4. 具体热点是 `MemoryLocKey` / `PAPath` 比较链。

也就是说，这次 `fortune` 的慢，不是“PA 开了以后类型约束变多”的后半段问题，
而是 `PA.solve()` 自己在 points-to 集合去重上花了很多时间。

## `PA.solve()` 入口边规模

在 `PointerAnalysis::solve()` 入口打断点，前两个 SCC 都是空 PA。真正的大 SCC 进入
`PA.solve()` 时，边规模是：

- `CopyEdges = 469`
- `FieldEdges = 315`
- `LoadPtrEdges = 0`
- `StorePtrEdges = 0`

说明这次 fortune 的主要开销来自：

- copy 传播
- field 传播

不是 pointer load/store 的对象内容流。

## 对现象的解释

当前 `20_PointerAnalysisFieldCycle.ll` 的 star path 修复，只解决了“循环 ptradd 导致
path 无限增长”这个问题。

它对 fortune 的帮助有限，原因是这里的主瓶颈不是“单条 path 无限长”，而更像是：

1. 大 SCC 里有很多 copy edge 和 field edge。
2. `PA.solve()` 迭代传播时，会不断往 `std::set<MemoryLocKey>` 插入候选 loc。
3. 即使 path 已经不会无限增长，仍然会产生大量不同的 `MemoryLocKey`。
4. `std::set` 去重时要反复调用：
   - `MemoryLocKey::operator<`
   - `PAPath::operator<`
   - `PAPathElem::operator<`
   - `PAPathAtom::operator<`
   - `OffsetRange::operator<`
5. 所以时间主要烧在“比较很多已经不再发散、但仍然很多的 path key”上。

换句话说：

- 小循环 case 的问题是“不收敛”。
- fortune 当前看到的问题更像是“虽然收敛，但比较和传播代价还是太高”。

## 当前结论

### 已确认

- `original` 快，`shadow/replace` 慢，差异真实存在。
- 慢点在 `PointerAnalysis::solve()`。
- 热点在 `std::set<MemoryLocKey>::insert()` 的 key 比较。
- `PAPath` star path 修复已经解决小循环不收敛，但没有解决 fortune 上的大规模
  points-to 去重成本。

### 还没确认

- 当前 `PointsTo` 最终总共有多少 slot / loc。
- 哪几个 slot 的 points-to set 最大。
- `469 copy + 315 field` 在 solve 迭代后，实际膨胀成了多少 `MemoryLocKey`。

## 下一步建议

下一轮如果要继续优化，优先方向应该是：

1. 给 `PointerAnalysis::solve()` 加 stats
   - 总 slot 数
   - 总 loc 数
   - 每轮新增 loc 数
   - 最大 points-to set 的 slot 和大小
2. 给 `MemoryLocKey` / `PAPath` 增加更便宜的 fast-path key
   - 例如 cached hash / cached fingerprint
   - 先比短 fingerprint，再落到完整 `operator<`
3. 看 `CopyEdges` 传播是否需要 worklist delta 化
   - 当前是全边全量重扫
4. 看 field 传播是否需要 per-slot delta，而不是每轮把整套 loc 再跑一遍

目前这轮调查还没动代码，结论是“fortune 的慢点已经从不收敛，转成了大规模
`MemoryLocKey` 比较与 set 插入成本”。

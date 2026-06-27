# 原始 prompt

> 形成一个新的logs/文档，规划如何改进使得进一步接近SAILR的算法，按照对结果的影响程度排序，先处理影响最大的

# 背景

现有 `DuplicationReverter` 已经能做 exact duplicate merge、goto 相关 common tail、以及线性 region tail 合并，但之前的候选门槛偏死，主要只认原始 input block。这样一来，已经 materialize 的 copied region 里如果前缀不同、主体和尾部相同，还是容易被挡在外面。

Angr 的目标更宽：它不是只合并原始块，而是围绕 goto 找候选，拆出相似子图，再把共同部分重新接回去。NotDec 这一步不做完整 merge graph，只先把 copied region 里已经 materialized 的块放进保守候选里。

# 目标

先补 P0 里最值钱的一小块：让 `DuplicationReverter` 能把已有 copied prefix 参与到 goto 相关相似 tail 合并里，同时不放开 synthetic / dephication 块。

判断标准很直接：

1. 现有的 goto 相关 linear tail 正例还能过。
2. 新增的 copied-prefix 正例能过。
3. 没有 goto hint 时不动图。
4. fortune 同口径时间不明显变差。

# 路线

这次没有去做 Angr 的完整 merge graph，只做了一个很窄的门槛放宽：

1. 保持 `DuplicationReverter` 仍然先围绕 `Current.Gotos` 找候选。
2. 让 shared-tail 合并接受已经 materialized 的 `Copied` region block。
3. 继续排除 `Synthetic` 和带 dephication 上下文的 block。
4. 继续用现有 payload origin 和 control shape 比较，别把语义更弱的块混进去。

# 风险

最大的风险是把门槛放得太松，误把现有正例之外的块也拖进来。另一个风险是 copied prefix 参与后，原来的 linear tail / statement tail 合并顺序被打乱，导致已有回归失败。

# 判断标准

这一步算对，只看三件事：

1. 现有 `structuring-analysis-test` 通过。
2. 新的 copied-prefix 回归通过。
3. fortune 同口径 `tr-level=2` 时间没有明显退化。

# 2026-06-27 实现记录：P0 copied-prefix shared tail 放宽

这轮把 `DuplicationReverter` 里 shared tail 候选的身份门槛放宽了一档。现在 `canUseForSharedTailMerge()` 允许已经 `BodyMaterialized` 且 `BodyBlock == Id` 的 `Copied` region block 进入 shared-tail 候选，只要它没有 dephication 上下文；`canSplitCommonStatementTailBlock()` 和 `canShareLinearRegionTailBlock()` 也复用这条门槛。`commonStatementTailCandidate()` 仍然要求 control shape 一致，但不再要求两边都必须是原始 input block。

改动：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:600-744`：
  新增 `canUseForSharedTailMerge()`，并让 `canSplitCommonStatementTailBlock()`、`canShareLinearRegionTailBlock()` 和 `commonStatementTailCandidate()` 复用它。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:2050-2095`：
  `DuplicationReverter::runOnGraph()` 仍然优先跑 goto 相关 linear tail / common statement tail，再回到 exact duplicate merge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4151-4268`：
  新增 `testDuplicationReverterMergesGotoRelatedLinearRegionTailWithCopiedPrefix()`，覆盖一个 copied linear prefix + shared tail 的合并回归。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9850-9870`：
  把新测试接进 `main()`。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
cmake --build build --target notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-sailr-p0-copy-prefix.c --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- `run_sailr_bench2_migration.py` 通过。
- fortune 通过，`elapsed=202.99 user=230.14 sys=1.76 maxrss=1273136`。

当前判断：

- 这一步只是把 copied region 的来源身份再往前推进一点。
- 还没有做 Angr 那种完整的 split / merge graph，也没有补 statement move、condition 合成和更宽的相似子图语义。

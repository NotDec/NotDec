# 20260720-02 load/store struct pointer merge 实现记录

## 原始 prompt

由于 C 语言不区分 load 和 store 类型，所以如果一个指针类型里的 load 和 store 类型都是其他结构体指针类型，那么可以将 load 和 store 类型合并。

基于这个思路去想一个合并策略并测试效果

## 背景和目标

binarysub 里同一个指针的 `ptr_load<T>` 和 `ptr_store<U>` 目前只会产生 `U <: T`。C 语言字段类型没有 load/store 两套视图，所以当 `T` 和 `U` 本身都已经表现为结构体指针时，可以尝试把这两个目标变量合并，减少类型碎片。

本次把策略做成实验开关，默认关闭。目标是先在 fortune 上看它是否降低 DebugInfo oracle 的 fragmentation，并确认是否引入 wrong merge。

## 实现

- `include/notdec/DecompilerContext.h:21`：新增 `Options::mergeStructPtrLoadStore`。
- `src/NotDec.cpp:94`、`src/NotDec.cpp:299`：新增 CLI `--merge-struct-ptr-load-store` 并写入 `Options`。
- `include/notdec/Passes/PassManager.h:78`、`include/notdec/Passes/PassManager.h:135`、`src/Passes/PassManager.cpp:342`：把策略开关传到类型恢复上下文，并要求 `tr-level >= 2`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:134`、`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:139`：在 `ConstraintsGenerator` 里保存策略开关，并新增 `LoadStoreStructPtrMergeCandidate`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:221`：在每个 SCC 的约束求解和 pointer-analysis 派生约束后运行策略，并打印合并数量。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:540`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3494`：`MLsubRecovery` 保存策略开关并传给 SCC generator。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:109`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:124`：新增结构体字段证据判定。当前只把常量偏移且偏移值 `>= 4` 的 record field 当作结构体指针证据，避免把普通 `char *`/byte buffer 拉进来。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2400`：抽出 `configureConstraintContext()`，复用 merge eval 的 `onVariableMerged` hook。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2413`：实现 `hasStructPointerEvidence()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2439`：实现 `collectStructPtrLoadStoreMergeCandidates()`，扫描 V2N 可达变量的 upper bounds，找同一 pointer 变量上的同 size `ptr_load`/`ptr_store`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2504`：实现 `applyStructPtrLoadStoreMergePolicy()`。候选必须满足 load/store 目标都是同 level、同 pointer size 的变量，且都有结构体指针证据；合并复用 `binarysub::merge_variable_into()`。

## fortune 测试

baseline:

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-merge-baseline-1784560001.ll \
  --tr-level=2 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-baseline-1784560001
```

新策略:

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-merge-loadstore-1784560002.ll \
  --tr-level=2 \
  --merge-struct-ptr-load-store \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-loadstore-1784560002
```

结果对比：

| 指标 | baseline | 新策略 |
| --- | ---: | ---: |
| bad_unions | 0 | 0 |
| polluted_components | 0 | 0 |
| fragmented_types | 2 | 2 |
| fragmented_nodes | 18 | 18 |
| merged_nodes | 13 | 28 |
| representative_nodes | 2338 | 2321 |
| peak_rss_mb | 1298 | 3564 |
| wall_ms | 167888 | 528168 |

输出路径：

- `/tmp/notdec-merge-eval-fortune-baseline-1784560001/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-loadstore-1784560002/merge-eval-summary.json`
- `/tmp/notdec-merge-eval-fortune-baseline-1784560001/bad_unions.jsonl`
- `/tmp/notdec-merge-eval-fortune-loadstore-1784560002/bad_unions.jsonl`
- `/tmp/notdec-fortune-merge-baseline-1784560001.ll`
- `/tmp/notdec-fortune-merge-loadstore-1784560002.ll`

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- fortune baseline 和新策略都能跑完，`bad_unions.jsonl` 都为空。

## 评估

- 实现效果：4/10。策略确实多合并了一些变量，且当前 DebugInfo oracle 下没有 wrong merge；但 fortune 的 oracle fragmentation 没改善。
- 复杂度：5/10。代码集中在 `MLsubGenerator`，没有改 binarysub；但新增了一套结构体指针证据判定，后续需要继续校准。
- 维护成本：6/10。默认关闭，风险可控；但策略版 wall time 和 RSS 明显变差，不适合作为默认策略。

## 结论

这个策略作为实验开关可以保留，但当前 fortune 结果不支持默认开启。下一步应该先输出候选明细，确认新增的 15 个合并到底落在哪些 load/store pair 上，再决定是收紧结构体指针证据，还是把策略提前到约束生成过程中降低 post-solve merge 成本。

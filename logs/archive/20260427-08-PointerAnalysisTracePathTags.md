# 2026-04-27 pointer analysis trace path tags

## 目的

前一版 trace 已经能看到：

- `max_path_loc`
- `dup_samples`
- `top_slots`

但仍有一个关键缺口：

- path 里只打印 offset，不打印 field tag

结果就是：

- `fortune` 里两个不同 ptradd 语句产生的 `@4`
- 在 trace 里看起来完全一样
- 但在 `PAPathAtom` 里它们其实不是同一个 key

这会直接误导后续分析，所以这次只补打印，不改求解逻辑。

## 修改文件与行号

### 1. `include/notdec/TypeRecovery/mlsub/PAPath.h`

- `include/notdec/TypeRecovery/mlsub/PAPath.h:72-74`
  - 新增详细 formatter 声明：
    - `formatDetailedPAPathAtom`
    - `formatDetailedPAPathElem`
    - `formatDetailedPAPath`

### 2. `src/TypeRecovery/mlsub/PAPath.cpp`

- `src/TypeRecovery/mlsub/PAPath.cpp:36-45`
  - 新增 `formatDetailedPAPathBody()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:207-209`
  - 实现 `formatDetailedPAPathAtom()`。
  - 格式是：
    - `@offset{tag}`
  - 例如：
    - `@4{ir:main::%wide.left.next}`
- `src/TypeRecovery/mlsub/PAPath.cpp:219-225`
  - 实现 `formatDetailedPAPathElem()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:238-247`
  - 实现 `formatDetailedPAPath()`。

### 3. `src/TypeRecovery/mlsub/PointerAnalysis.cpp`

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:157-167`
  - `formatMemoryLocKey()` 改为调用 `formatDetailedPAPath()`。
  - 这样 `[pa:*]` 相关 trace 里所有 `loc=`、`max_path_loc=`、
    `dup_samples=` 都会带上 field tag。

### 4. `unittests/Retypd/PAPathTest.cpp`

- `unittests/Retypd/PAPathTest.cpp:19-23`
  - 测试 helper 的 tag 改成 `VirtualPtrAdd`，避免空 `ReturnValue`
    在详细 formatter 里解引用崩掉。
- `unittests/Retypd/PAPathTest.cpp:88-95`
  - 新增 `PAPathDetailedFormatShowsTags`。

## 涉及函数

- `notdec::mlsub::formatDetailedPAPathAtom`
- `notdec::mlsub::formatDetailedPAPathElem`
- `notdec::mlsub::formatDetailedPAPath`
- `notdec::mlsub::formatMemoryLocKey`

## 验证

### 1. 单测

```bash
cmake --build build --target PAPathTest notdec-decompile -j4
./build/bin/PAPathTest
```

通过。

### 2. 最小复现 case

```bash
timeout 7s env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/21_PointerAnalysisBranchingFieldCycle.ll \
  -o /tmp/21_branching_field_cycle_tracefix3.ll --tr-level=2 \
  --frozen-tr-input-ir -g --work-dir=/tmp/notdec-branching-field-cycle-tracefix3
```

现在 `iter=2` 已经能直接区分两个 `@4`：

- `field@value(main::%wide.left.next)->main::heap(main::%base).@4{ir:main::%wide.left.next}[32]:1`
- `field@value(main::%wide.right.next)->main::heap(main::%base).@4{ir:main::%wide.right.next}[32]:1`

### 3. fortune

```bash
timeout 35s env NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-tracefix.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-fortune-tracefix
```

现在 `main::%bb.blk_exit39.i1` 那条热点链已经能直接看出两个不同的
`@4`：

- `@4{ir:main::%bb.brif_next299.i13}`
- `@4{ir:main::%bb.brif_next292.i13}`

例如 `iter=5` 的 duplicate 样本已经不再混淆：

- `... .@4{ir:main::%bb.brif_next299.i13}[32]`
- `... .@4{ir:main::%bb.brif_next292.i13}[32]`

## 结论

这次没有试图解决 path 膨胀，只把 trace 补到“能看清到底是哪条 field 在分叉”。

补完后，`fortune` 当前的问题已经可以直接从 log 里读出来：

1. 热点根对象是 `main::heap(main::%bb.blk_exit39.i1)`。
2. 回边上至少有一个 `@1` 和两个不同 tag 的 `@4`。
3. 当前 path 组合爆炸，不只是 offset 重复。

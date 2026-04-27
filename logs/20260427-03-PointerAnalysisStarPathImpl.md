# 2026-04-27 PointerAnalysis star path implementation

## 本次修改

这次按 `logs/20260427-02-PointerAnalysisStarPathPlan.md` 的大方向实现了
PA star path，但在归一化细节上做了一个收敛更稳的简化：

- 没有实现“append 一个 atom 后，只要在前面看到同 atom 就立刻折成 star”。
- 实现的是“尾部出现完整重复段 `X X` 时，折成 `Star(X)`”，再配合
  `Star(X) X => Star(X)` 吸收。

这样做的原因是它和真实 append 顺序一致，不会把 `A B C A` 直接折成
`(A B C)*` 后，下一步却只能继续 append `B C ...` 这种相位错位问题带进来。

## 修改文件与行号

### 1. `include/notdec/TypeRecovery/mlsub/PAPath.h`

- `include/notdec/TypeRecovery/mlsub/PAPath.h:17`
  - 新增 `PAFieldTag`，给 PA field step 一个稳定 tag。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:31`
  - 新增 `PAPathAtom`。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:38`
  - 新增 `PAPathStar`，保留 `Body` 和 `Summary`。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:45`
  - 新增 `PAPathElem`。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:47`
  - 新增 `PAPath`。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:53`
  - 声明 `PAFieldTag` / `PAPathAtom` / `PAPathStar` / `PAPathElem` /
    `PAPath` 的 `operator==` 和 `operator<`。
- `include/notdec/TypeRecovery/mlsub/PAPath.h:68`
  - 声明 path 格式化、`summarizeStarBody()`、`appendAndNormalize()`、
    `normalizePAPath()`。

### 2. `src/TypeRecovery/mlsub/PAPath.cpp`

- `src/TypeRecovery/mlsub/PAPath.cpp:20`
  - 新增 `makeStar()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:36`
  - 新增 `absorbTrailingBodyIntoLastStar()`，实现 `Star(X) X => Star(X)`。
- `src/TypeRecovery/mlsub/PAPath.cpp:71`
  - 新增 `collapseRepeatedAtomSuffix()`，实现尾部 `X X => Star(X)`。
- `src/TypeRecovery/mlsub/PAPath.cpp:114`
  - 新增 `mergeAdjacentEqualStars()`，实现 `Star(X) Star(X) => Star(X)`。
- `src/TypeRecovery/mlsub/PAPath.cpp:131`
  - 实现 `PAFieldTag` 的比较。
- `src/TypeRecovery/mlsub/PAPath.cpp:141`
  - 实现 `PAPathAtom` 的比较。
- `src/TypeRecovery/mlsub/PAPath.cpp:149`
  - 实现 `PAPathStar` 的比较。
- `src/TypeRecovery/mlsub/PAPath.cpp:157`
  - 实现 `PAPathElem` 的比较。
- `src/TypeRecovery/mlsub/PAPath.cpp:177`
  - 实现 `PAPath` 的比较。
- `src/TypeRecovery/mlsub/PAPath.cpp:186`
  - 实现 `formatPAFieldTag()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:194`
  - 实现 `formatPAPathAtom()` / `formatPAPathElem()` / `formatPAPath()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:215`
  - 实现 `summarizeStarBody()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:223`
  - 实现 `appendAndNormalize()`。
- `src/TypeRecovery/mlsub/PAPath.cpp:228`
  - 实现 `normalizePAPath()`。

### 3. `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h`

- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:16`
  - 引入 `PAPath.h`。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:28`
  - `MemoryLocKey::Path` 从 `std::vector<OffsetRange>` 改成 `PAPath`。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:68`
  - `FieldEdge` 新增 `PAFieldTag Tag`。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:97`
  - `getFieldObject()` 签名增加 `PAFieldTag Tag`。

### 4. `src/TypeRecovery/mlsub/PointerAnalysis.cpp`

- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:10`
  - 新增 `makeIRPtrAddTag()`，第一版直接用 `Dst` 作为稳定 `IRPtrAdd` tag。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:16`
  - `formatMemoryLocKey()` 改成调用 `formatPAPath()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:56`
  - `PointerAnalysis::getFieldObject()` 改成 `appendAndNormalize()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:75`
  - `PointerAnalysis::addField()` 记录 `PAFieldTag`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:113`
  - `PointerAnalysis::solve()` 的 field 传播改为把 `Tag` 一起传给
    `getFieldObject()`。

### 5. 构建与测试文件

- `src/CMakeLists.txt:20`
  - 把 `TypeRecovery/mlsub/PAPath.cpp` 加进 `notdec` 静态库。
- `unittests/Retypd/PAPathTest.cpp:19`
  - 新增 `PAPath` 单测。
- `unittests/Retypd/CMakeLists.txt:23`
  - 新增 `PAPathTest` 目标并接入 `gtest_discover_tests()`。

## 涉及函数

- `notdec::mlsub::summarizeStarBody`
- `notdec::mlsub::appendAndNormalize`
- `notdec::mlsub::normalizePAPath`
- `notdec::mlsub::formatPAPath`
- `notdec::mlsub::PointerAnalysis::getFieldObject`
- `notdec::mlsub::PointerAnalysis::addField`
- `notdec::mlsub::PointerAnalysis::solve`

## 验证

已执行：

```bash
cmake --build build --target PAPathTest notdec -j4
./build/bin/PAPathTest
timeout 10s env NOTDEC_POINTER_ANALYSIS_MODE=replace ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_replace.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-field-cycle-replace-plain
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果：

- `PAPathTest` 5 个测试全部通过。
- `20_PointerAnalysisFieldCycle.ll` 在 replace 模式下本地实测 `0.11s` 结束，
  不再出现之前的循环扩展超时。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。

另外检查了 `fortune.o3.wasm.ll`：

```bash
timeout 60s env NOTDEC_POINTER_ANALYSIS_MODE=shadow ./build/bin/notdec ...
timeout 60s env NOTDEC_POINTER_ANALYSIS_MODE=replace ./build/bin/notdec ...
```

两者都在 60 秒内没有结束。抓 10 秒 trace 后看到运行仍停在大 SCC 的
`PNDiff` 求解阶段，还没有进入 `PA.solve()`，所以当前不能把 fortune 的耗时
直接归因到这次 star path 改动，也不能用它证明 replace 侧已经完全解决。

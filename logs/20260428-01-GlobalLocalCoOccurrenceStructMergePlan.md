# Global/Local Co-Occurrence And Struct Merge Plan

> 2026-04-28 更新：第一、二阶段已完成；后续结构体 sidecar 分阶段实现。
> 2026-04-29 更新：去掉开头的 global co-occur analyze / global simplify。
> 结构体合并现在直接看 canonicalize 后的 `CompactType`，最终类型仍走后面的
> per-root local simplify。

## 背景

这条计划开始时，`external/binarysub` HEAD 是
`cde0e05 Revert "Isolate co-occurrence analysis per root"`。revert 后，
`TypeSimplifier::bulkSimplify` 又恢复成全局 co-occur：

1. 所有 root 先 `canonicalizeType`。
2. 所有 root 一起 `analyzeOccurrences`。
3. 每个 root 用同一份 co-occur 结果 `simplifyType`。
4. 最后 `coalesceCompactType` 成 `UType`。

最初的判断是：全局 co-occur 会保留更多顶层类型变量，这些变量可以作为
结构体片段之间的合并标签。

后面看下来，这个判断不够准确。结构体合并真正需要的是“尽量完整的结构片段和
标签关系”，不一定需要先经过一轮全局 simplify。全局 simplify 反而可能提前删掉、
合并或改写一些结构信息，让后面的合并少看到候选。

所以当前思路调整为：

1. 所有 root 先 `canonicalizeType`，得到完整的 `CompactType`。
2. 结构体合并直接看这批 canonical `CompactType`。
3. 后面的 per-root local simplify 继续负责最终展示类型。

## 当前流程

现在的顺序是：

```text
canonicalize all roots
-> struct merge analysis
-> per-root local co-occur analyze
-> per-root local simplify
-> coalesce UType
```

也就是说，结构体合并看的是 canonicalize 后的原始 `CompactType`。
局部 simplify 放在后面，只负责最终展示类型。现在不再单独维护
`structMergeOnlyVars` plan，也不再用全局 simplify 给结构体合并预处理标签。

## 已实现

- `external/binarysub/include/binarysub/binarysub.h:329`
  - 新增 `OccurrenceAnalysisState`，把 co-occur 分析结果显式传递。
  - 新增 `SimplificationPlan`，当前只记录变量删除/合并计划。
  - 新增 `StructMergeInfo` / `BulkSimplifyResult`。
  - 新增 `TypeSimplifier::bulkSimplifyDetailed()`，旧 `bulkSimplify()` 保持兼容。
- `external/binarysub/src/binarysub.cpp:1793`
  - 新增结构体候选、冲突判断、贪心分组 helper。
- `external/binarysub/src/binarysub.cpp:2022`
  - 从 canonicalize 后的 `CompactType` 抽取结构体候选。
- `external/binarysub/src/binarysub.cpp:2087`
  - 构造 `StructMergeInfo`，包括 candidate、group、`mergedBody`。
- `external/binarysub/src/binarysub.cpp:2184`
  - `bulkSimplifyDetailed()` 接入 canonical struct merge + local simplify 流程。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1507`
  - 新增 `appendDebugStructMerge()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3071`
  - `ConstraintsGenerator::genTypes()` 改用 `bulkSimplifyDetailed()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3113`
  - 在 workdir 里写出 `type-struct-merge.md`。

`SimplificationPlan` 的测试在：

- `external/binarysub/src/binarysub-test.cpp:476`
- `external/binarysub/src/binarysub-test-main.cpp:39`

## 2026-04-28 后续实现记录合并

原 `20260428-02`、`20260428-03`、`20260428-04` 已合并到本日志，不再单独保留。

### 结构体合并 trace

第一版实现时，结构体合并先只做 trace，不影响最终 `UType` / `HType`。

涉及位置：

- `external/binarysub/src/binarysub.cpp:1791-2141`
  - 新增结构体候选、冲突判断、贪心分组 helper。
- `external/binarysub/src/binarysub.cpp:2143-2197`
  - `TypeSimplifier::bulkSimplify()` 接入当时的 global/local 两阶段流程。

当时流程是：

```text
canonicalize all roots
-> global analyzeOccurrences
-> global simplifyType
-> trace_struct_merge_analysis
-> per-root local analyzeOccurrences
-> per-root local simplifyType
-> per-root coalesceCompactType
```

涉及函数和数据结构：

- `CompactConflictResult`
- `StructCandidateNode`
- `primitive_merge_conflict()`
- `compact_merge_conflict_impl()`
- `collect_struct_candidates_impl()`
- `trace_struct_merge_analysis()`
- `TypeSimplifier::bulkSimplify()`

验证过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.global-local-structmerge.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-global-local-structmerge
```

结果：`binarysub` 自测通过，`notdec-decompile` 编译通过，fortune 用例跑通。

### 结构体合并 sidecar

第二步把 trace 结果变成 `binarysub` 返回值，并在 NotDec workdir 写出：

```text
type-struct-merge.md
```

涉及位置：

- `external/binarysub/include/binarysub/binarysub.h:340-363`
  - 新增 `StructMergeCandidateInfo`、`StructMergeGroupInfo`、`StructMergeInfo`、
    `BulkSimplifyResult`。
- `external/binarysub/include/binarysub/binarysub.h:394-396`
  - 新增 `TypeSimplifier::bulkSimplifyDetailed()`。
- `external/binarysub/src/binarysub.cpp:2087-2170`
  - `trace_struct_merge_analysis()` 改成 `build_struct_merge_info()`。
- `external/binarysub/src/binarysub.cpp:2175-2228`
  - 新增 `bulkSimplifyDetailed()`，旧 `bulkSimplify()` 改成 wrapper。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:53-56`
  - 新增 `kTypeStructMergeFile = "type-struct-merge.md"`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1507-1587`
  - 新增 `appendDebugStructMerge()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3068-3111`
  - `genTypes()` 改用详细 API，并把 sidecar 写入 workdir。

验证过：

```bash
cmake --build build --target binarysub -j4
cmake --build build --target notdec-decompile -j4
./build/binarysub
rm -rf /tmp/notdec-fortune-structmerge-sidecar
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.structmerge-sidecar.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-structmerge-sidecar
```

结果：fortune 用例跑通，`type-struct-merge.md` 输出
`Candidates: 226 / Groups: 91`。

### group merged-body

第三步给 group sidecar 增加 `merged-body`，方便直接看每组会合成什么结构。

涉及位置：

- `external/binarysub/include/binarysub/binarysub.h:349-353`
  - `StructMergeGroupInfo` 新增 `std::string mergedBody`。
- `external/binarysub/src/binarysub.cpp:2156-2170`
  - `build_struct_merge_info()` 用 `merge_compact_types()` 合并组内候选 body。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1558-1587`
  - `appendDebugStructMerge()` 输出每个 group 的 `merged-body`。

验证过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
rm -rf /tmp/notdec-fortune-structmerge-mergedbody
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.structmerge-mergedbody.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-structmerge-mergedbody
```

结果：`binarysub` 自测通过，fortune 用例跑通，workdir sidecar 中出现
`merged-body:` 行。

## 2026-04-29 实现记录：去掉 global 预简化

本次只改 `external/binarysub/src/binarysub.cpp:2196-2223` 的
`TypeSimplifier::bulkSimplifyDetailed()`：

- 删除开头的全局 `OccurrenceAnalysisState analysis`。
- 删除所有 root 共用 `analyzeOccurrences()` 的循环。
- 删除所有 root 共用同一份 analysis 调 `simplifyType()` 的循环。
- `build_struct_merge_info()` 改为直接消费 canonicalize 后的 `compactMap`。
- local simplify 不动，但 worker 的 `recVars` / `variableOrigins` 改为继承
  canonicalize 阶段产生的状态。

验证：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
rm -rf /tmp/notdec-fortune-no-global-presimplify
NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.no-global-presimplify.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-no-global-presimplify
```

结果：

- `binarysub` 编译通过。
- `./build/binarysub` 全部通过。
- `notdec-decompile` 编译通过。
- fortune 用例跑通，生成 `/tmp/fortune.no-global-presimplify.out.ll`。
- 新 sidecar 主 SCC 从之前的 `Candidates: 226 / Groups: 91` 变为
  `Candidates: 304 / Groups: 109`。
- 最终 `.ll` 和上一版 `/tmp/fortune.structmerge-mergedbody.out.ll` 相同。

## 2026-04-29 性能调查和优化

问题确认：

- 2026-04-27 日志里，同一 fortune 用例在 `original` 模式、frozen 口径约
  `12.35s`。
- 本次优化前，当前代码同一 frozen 口径实测 `real 104.94s`，非 frozen 口径之前
  记录为 `real 103.52s`。
- 临时分段计时显示，大 SCC 中：
  - `canonicalize`: `2.55s`
  - `struct-merge`: `1.39s`
  - `local-total`: `91.27s`
  - `roots=5037`
  - `simplify=69.74s`

结论：慢点不在结构体合并分组本身。真正的问题是 local simplify 阶段对每个 root
都复制并遍历整张 SCC 递归变量表和 origin 表。复杂度变成近似：

```text
roots * full_scc_rec_vars
```

而不是只处理当前 root 可达的递归变量。

本次只改 `external/binarysub/src/binarysub.cpp` 的
`TypeSimplifier::bulkSimplifyDetailed()`：

- `external/binarysub/src/binarysub.cpp:2207-2214`
  - 先从 canonical origin 表计算 synthetic UType id 起点，避免后面只复制 origin
    子集时改变 `VarOrigins.txt` 里的 synthetic id。
- `external/binarysub/src/binarysub.cpp:2215-2259`
  - 新增 `buildLocalVariableOrigins` lambda。
  - 每个 root 只复制它自身和可达 recursive bindings 中出现的 origin。
- `external/binarysub/src/binarysub.cpp:2265-2279`
  - `analyzeOccurrences()` 改为直接用当前 canonical simplifier 运行。
  - local simplifier 的 `recVars` 改用 `localAnalysis.processedRecVars`。
  - 不再给每个 root 复制完整 `canonicalRecVars`。

中间计时：

- 只改可达 `recVars` 后，frozen 口径从 `real 104.94s` 降到 `real 33.06s`。
  - `local-total`: `19.28s`
  - `simplify`: `0.76s`
  - 剩下主要是每 root 复制完整 `variableOrigins`，`setup=10.52s`。
- 再改 origin 子集后，带临时计时的 frozen 口径为 `real 15.27s`。
  - `canonicalize`: `2.59s`
  - `struct-merge`: `1.43s`
  - `local-total`: `1.86s`

最终验证：

```bash
cmake --build build --target binarysub notdec-decompile -j4
./build/binarysub

rm -rf /tmp/notdec-fortune-final-frozen /tmp/fortune.final-frozen.out.ll
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.final-frozen.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-final-frozen

rm -rf /tmp/notdec-fortune-final-runtime /tmp/fortune.final-runtime.out.ll
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.final-runtime.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-final-runtime
```

结果：

- `./build/binarysub` 全部通过。
- frozen 口径：`real 15.36s`。
- 非 frozen 口径：`real 15.49s`。
- 和优化前 `/tmp/notdec-fortune-phase-current` 对比，下面文件逐字一致：
  - `/tmp/fortune.*.out.ll`
  - `type-struct-merge.md`
  - `ValueTypes.txt`
  - `ValueHTypes.txt`
  - `VarOrigins.txt`
- 额外跑了：

  ```bash
  ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 \
    --output-on-failure
  ```

  结果失败，耗时 `11.74s`。失败点是 fortune oracle 里的 struct.fd 字段恢复
  缺口，例如 `@File_list.fd`、`get_tbl::arg0.read_tbl`、
  `maxlen_in_list::arg0.next`。本次性能优化前后在手工 fortune 口径下对比的 `.ll` /
  type sidecar / ValueTypes / ValueHTypes / VarOrigins 都一致；CTest 的 oracle 缺口
  需要按结构体恢复质量另看。

## 2026-04-29 实现记录：HType 侧结构体复用

目的：前面的 sidecar 分组已经合理，但 `ValueHTypes.txt` 里仍有大量最终
`HType` 层重复声明。原因是 local simplify 后很多记录已经变成普通
`URecordType`，不再只通过 union/intersection 外层传递分组信息。

本次实现：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:24-33`
  - `TypeBuilderContext` 增加 `StructMergeInfo` 和 root group map。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3074-3112`
  - `genTypes()` 从 sidecar candidate/group 建 root label 到 group 的映射，
    并传给 `TypeBuilder`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:272-315`
  - 新增 `collectStructMergeOriginIds()`，递归收集普通 `URecordType` 内仍保留的
    origin id。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:458-587`
  - `TypeBuilder` 初始化 group label，按 origin overlap 选 struct-merge group。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1091-1098`
  - `convertStruct()` 没有外层 active group 时，尝试用当前 record 的 origin id
    直接命中 sidecar group。
- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:54-60`
  - 新增 exact HType layout cache，只按 `ValidRange + 字段 offset/size/type`
    复用完全相同的 `RecordDecl`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:590-622`
  - 新增 exact layout key 构造、查找和登记。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:876-888`
  - `craftStruct()` 创建新 `RecordDecl` 前先查 exact layout cache，命中则复用。

同步更新了两个 HType snapshot：

- `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes:3-17`
  - 去掉重复的 array/end 结构声明。
- `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes:64-66`
  - `stack_end` 改指向已存在的 `struct_1*`。
- `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes:25-27`
  - 去掉重复的 `offset -24` 结构声明。
- `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes:90-106`
  - `test1/test2` 的 stack 类型统一复用 `struct_5*`。

验证：

```bash
cmake --build build --target notdec-decompile -j4
cmake --build build --target TypeBuilderTest binarysub -j4
./build/bin/TypeBuilderTest
./build/binarysub
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure

rm -rf /tmp/notdec-fortune-structmerge-hlayout-final \
       /tmp/fortune.structmerge-hlayout-final.out.ll
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.structmerge-hlayout-final.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-structmerge-hlayout-final
```

结果：

- `TypeBuilderTest` 4 个测试通过。
- `./build/binarysub` 全部通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune frozen 口径：`real 16.33s`。
- `/tmp/notdec-fortune-structmerge-hlayout-final/ValueHTypes.txt`：
  - `structs=147`
  - `unions=37`
  - `unique_offset=21`
  - `unique_exact=79`
- 输出 `.ll` 和本次改 HType 复用前的
  `/tmp/fortune.structmerge-typebuilder-root.out.ll` 一致。

注意：尝试过“递归锚点按需创建”，能把 fortune struct 数进一步降到 83，但会让
`09_OffsetLoop`、`20_PointerAnalysisFieldCycle` 这类小用例从结构体指针退成
`void*`。这条路已经回退，后续不要按这个方向继续。

## 2026-04-29 实现记录：数组元素单字段 wrapper 消除

问题：fortune 的 `ValueHTypes.txt` 里出现这类结构：

```c
struct struct_88 {
  i8[4] field_0; /* at offset: 0 */
};
```

trace 确认来源是 `TypeBuilder::convertStruct()` 的 stride 数组分支。原始访问形如
`@0+4i` / `@4+4i`，递归到数组元素时变成 `@0` + `{}` marker。marker 被过滤后，
旧逻辑因为 `PointeeSize=4` 创建一个 4 字节 shell，于是得到
`struct { i8[4] @0 }`，外层再生成 `struct_88*[]`。

本次实现：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:79-86`
  - `convertStruct()` 增加 `PreferElementType` 参数，默认不改变普通指针记录转换。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1088-1090`
  - `convertStruct()` 接收 `PreferElementType`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1245`
  - stride 递归转换数组元素时传 `PreferElementType=true`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1295-1303`
  - 如果数组元素子问题只剩空 marker，就直接返回 `i8[N]`。
  - 如果只有一个非空字段，且 offset 为 0、大小等于元素大小，就直接返回字段类型。
  - 普通非数组元素路径仍保留原来的 struct shell。

同步更新了两个 HType snapshot：

- `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes`
  - `struct_2*[]` 收敛为 `top:32[]`。
- `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes`
  - `struct_1*[]` / `struct_4*[]` 收敛为 `i8[4][]`。

验证：

```bash
cmake --build build --target notdec-decompile -j4
cmake --build build --target TypeBuilderTest binarysub -j4
./build/bin/TypeBuilderTest
./build/binarysub
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure

rm -rf /tmp/notdec-fortune-single-field-element \
       /tmp/fortune.single-field-element.out.ll
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.single-field-element.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-single-field-element
```

结果：

- `TypeBuilderTest` 4 个测试通过。
- `./build/binarysub` 全部通过。
- `notdec.type_recovery.llvm_ir.tr_level_2` 通过。
- fortune frozen 口径：`real 16.35s`，没有明显性能回退。
- `ValueHTypes.txt` 从 `structs=147` 降到 `structs=145`。
- `struct { i8[4] field_0 @0 }` 这类 wrapper 在目标输出中消失。
- 输出 `.ll` 和 `/tmp/fortune.structmerge-hlayout-final.out.ll` 一致。

## 候选抽取

当前只抽取这类节点：

- 当前极性是 `pol == false`，也就是后续会形成顶层交类型的位置。
- 当前 `CompactType` 有非空 record。
- 当前 `CompactType` 顶层有非空 `vars`。

节点的 label 只取当前节点顶层 `vars`。字段内部出现同一个变量不算 label，
避免把只是共享泛型参数的结构体误合并。

## 冲突判断

冲突判断现在只做“能否无损合并”的保守判断：

- 类型变量只当作 label/unknown，本身不制造冲突。
- 两边都有 size 且不同，则冲突。
- primitive 同 size 同名不冲突。
- primitive 属于同一 semantic family，且按当前极性能 join/meet，则不冲突。
- primitive 和 aggregate 混在一起，视为冲突。
- function 和 record/pointer 混在一起，视为冲突。
- record 只递归检查同名字段；不同字段暂时认为可合并。
- function 参数按反极性递归，返回值按当前极性递归。
- pointer load 按当前极性递归，store 按反极性递归，access size 不同则冲突。

一个明显的后续改进点是字段冲突：现在只按字段字符串 exact match，
后面可能需要考虑 offset range overlap。


## 目标

目标不是恢复 `41075f1` 那种“每个 root 从 canonicalize 开始完全单独跑”的入口。
当前流程仍然先对所有 root 做 canonicalize，并在同一批 canonical root 上产出
`StructMergeInfo`。

关键点是：**local simplify 本来就是后半段的 per-root simplify**。本次去掉的是
结构体合并前面的 global co-occur analyze / global simplify，不是去掉后面的
local simplify。

当前目标流程是：

```text
canonicalize all roots
  -> struct merge on canonical CompactType
  -> per-root local co-occur simplify
  -> coalesce to UType
```

这里的优先级是：

1. `StructMergeInfo` / `type-struct-merge.md` 是当前最重要的产物，后面的候选、
   分组、`mergedBody` 输出逻辑不要动。
2. global 预简化不是必须步骤，先去掉，让结构体合并看到更原始的结构。
3. 最终 `UType` 暂时不受结构体合并影响，所以这次改动预期主要影响 sidecar。
4. `structMergeOnlyVars` 这条计划暂时放弃；不再为了区分“全局保留、局部删除”的
   变量额外维护标签集合。

## 当前相关位置

主要改动集中在：

- `external/binarysub/include/binarysub/binarysub.h`
  - `TypeSimplifier`
  - `OccurrenceData`
  - `OccurrenceMap`
  - `CompactType`
- `external/binarysub/src/binarysub.cpp`
  - `TypeSimplifier::canonicalizeType`
  - `TypeSimplifier::analyzeOccurrences`
  - `TypeSimplifier::simplifyType`
  - `TypeSimplifier::coalesceCompactType`
  - `TypeSimplifier::bulkSimplify`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
  - `ConstraintsGenerator::genTypes`
  - 调试输出：`appendDebugValueTypes` / `appendDebugVarOrigins`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`
  - 后续如果要让结构体合并影响 `HType`，这里会消费合并结果。

## 第一阶段（已完成，2026-04-28）：把 co-occur 状态显式化

先恢复 `41075f1` 里好的部分：把 co-occur 分析状态从 `TypeSimplifier` 成员里拆出来。

新增：

```cpp
struct OccurrenceAnalysisState {
  OccurrenceMap coOccurrences;
  std::map<SimpleType, CompactTypePtr> processedRecVars;
};
```

接口改成：

```cpp
void analyzeOccurrences(const CompactTypePtr &Ty,
                        OccurrenceAnalysisState &Analysis);
```

`simplifyType` 不再直接读成员 `coOccurrences`，而是显式接受分析结果：

```cpp
CompactTypePtr simplifyType(const CompactTypePtr &Ty,
                            const OccurrenceAnalysisState &Analysis,
                            bool PrintDebug);
```

这一阶段不改变行为。`bulkSimplify` 仍然构造一份全局 `OccurrenceAnalysisState`，
然后所有 root 共用它。

### 第一阶段实现记录

已实现位置：

- `external/binarysub/include/binarysub/binarysub.h:327-389`
  - `OccurrenceAnalysisState`
  - `TypeSimplifier::analyzeOccurrences`
  - `TypeSimplifier::simplifyType`
  - `TypeSimplifier::isClear`
  - `TypeSimplifier::clear`
- `external/binarysub/src/binarysub.cpp:1132-1235`
  - `TypeSimplifier::analyzeOccurrences`
- `external/binarysub/src/binarysub.cpp:1791-1863`
  - `TypeSimplifier::bulkSimplify`
  - `TypeSimplifier::simplify`

本次实际落地后，行为保持为：

1. 所有 root 先 `canonicalizeType`
2. 所有 root 共用一份 `OccurrenceAnalysisState`
3. 再逐个 `simplifyType`

也就是说，这里只是把状态显式传递出来，没有提前切到 per-root 分析。

## 第二阶段（已完成，2026-04-28）：把 simplify 拆成 plan 和 apply

现在 `simplifyType` 内部有一个局部变量：

```cpp
std::map<SimpleType, std::optional<SimpleType>> varSubst;
```

它正好描述变量会被如何处理：

- `varSubst[V] = std::nullopt`：变量被删除。
- `varSubst[V] = W`：变量被合并到 `W`。
- 没有出现在 `varSubst`：变量保留。

把这部分抽成：

```cpp
struct SimplificationPlan {
  std::map<SimpleType, std::optional<SimpleType>> varSubst;
};
```

并拆出两个函数：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &RecVars,
    const OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &InputRecVars,
    const SimplificationPlan &Plan,
    std::map<SimpleType, CompactTypePtr> &OutputRecVars);
```

这样可以在局部阶段真正删除变量之前，先知道哪些变量会被删。

### 第二阶段实现记录

已实现位置：

- `external/binarysub/include/binarysub/binarysub.h:334-366`
  - `SimplificationPlan`
  - `TypeSimplifier::computeSimplificationPlan`
  - `TypeSimplifier::applySimplificationPlan`
  - `TypeSimplifier::simplifyType`
- `external/binarysub/src/binarysub.cpp:1237-1484`
  - `TypeSimplifier::computeSimplificationPlan`
- `external/binarysub/src/binarysub.cpp:1486-1589`
  - `TypeSimplifier::applySimplificationPlan`
- `external/binarysub/src/binarysub.cpp:1591-1595`
  - `TypeSimplifier::simplifyType`
- `external/binarysub/src/binarysub-test.cpp:476-536`
  - `test_simplification_plan_apply`
- `external/binarysub/include/binarysub/binarysub-test.h:3-18`
  - `test_simplification_plan_apply` 声明
- `external/binarysub/src/binarysub-test-main.cpp:35-41`
  - 测试入口接线

### 第二阶段实现时和原计划不同的地方

原计划里第二阶段接口写的是：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &RecVars,
    const OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const std::map<SimpleType, CompactTypePtr> &InputRecVars,
    const SimplificationPlan &Plan,
    std::map<SimpleType, CompactTypePtr> &OutputRecVars);
```

这次实现时没有把 `recVars/newRecVars` 也一起显式化，而是先只把
`varSubst` 这层 plan 显式化。当前实际接口是：

```cpp
SimplificationPlan computeSimplificationPlan(
    const CompactTypePtr &Ty,
    OccurrenceAnalysisState &Analysis,
    bool PrintDebug);

CompactTypePtr applySimplificationPlan(
    const CompactTypePtr &Ty,
    const SimplificationPlan &Plan);
```

这样做的原因很简单：第二阶段先保证“变量删/并计划”可以单独拿出来，同时不改变
现有行为。代价是：

- `computeSimplificationPlan()` 现在仍然会继续改：
  - `TypeSimplifier::recVars`
  - `TypeSimplifier::variableOrigins`
  - `OccurrenceAnalysisState::coOccurrences`
- `applySimplificationPlan()` 仍然读写：
  - `TypeSimplifier::newRecVars`

所以第二阶段现在只是把“变量如何处理”显式化了，递归变量状态还没有从
`TypeSimplifier` 里拆干净。这个缺口留到第三阶段处理。

### 第二阶段额外补的一点

`applySimplificationPlan()` 里顺手补了 substitution chain 解析。也就是
`A -> B -> C` 这种替换链，应用时会一直跟到最终变量，而不是只展开一层。

### 第二阶段验证

执行过：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
cmake --build build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.global-local-phase12.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-global-local-phase12
```

结果：

- `binarysub` 自测通过
- `notdec-decompile` 编译通过
- `fortune.o3.wasm.ll` 在 `original` 模式下成功跑通

## 第三阶段：处理递归变量状态

当前 `TypeSimplifier` 用成员变量保存：

- `recVars`
- `newRecVars`

双阶段 simplify 后，不能继续让这两个状态隐式混在一起。

规划上用显式 bundle 描述当前 compact program：

```cpp
struct CompactProgram {
  std::map<PolarVar, CompactTypePtr> roots;
  std::map<SimpleType, CompactTypePtr> recVars;
};
```

流程变成：

```text
Input SimpleType roots
  -> CompactProgram compactProgram
  -> StructMergeInfo
  -> CompactProgram localProgram
  -> UType results
```

`coalesceCompactType` 后续也最好接受显式 `recVars`：

```cpp
UTypePtr coalesceCompactType(const CompactTypePtr &Ty,
                             const std::map<SimpleType, CompactTypePtr> &RecVars,
                             bool Pol,
                             bool PrintDebug);
```

这样每个阶段读哪份递归绑定会更清楚。

## 第四阶段（已调整）：去掉 global 预简化

原计划是先跑 global co-occur simplify，再标记 `structMergeOnlyVars`。这个思路现在
不继续走。

原因：

1. 后半段 local simplify 已经是每个 root 单独分析，能负责最终类型显示。
2. 结构体合并当前最需要的是更多候选和更完整的 record 形状。
3. global simplify 可能提前删掉、合并、改写顶层变量和递归绑定，反而减少后面能
   合并的信息。

调整后的阶段：

```text
compactMap[root] = canonicalizeType(root)
structMerge = build_struct_merge_info(compactMap)
for root in compactMap:
  localAnalysis = analyze root only
  localCompact = simplifyType(root, localAnalysis)
  result[root] = coalesceCompactType(localCompact)
```

注意：这不是把整个 `bulkSimplifyDetailed()` 退回旧 per-root 入口。差别在于：

- 仍然先 canonicalize 所有 root。
- 仍然在这一批 root 上统一构造 `StructMergeInfo`。
- 只是结构体合并前不再跑全局 co-occur / simplify。

## 第五阶段：结构体合并建模

结构体合并先放在 `CompactType` 层做，不先放到 `UType` 层。

候选节点：

```cpp
struct StructCandidateNode {
  uint32_t id;
  PolarVar root;
  bool pol;
  std::string path;
  CompactTypePtr body;
  SimpleTypeSet labels;
};
```

抽取规则：

1. 遍历 canonical `compactMap`。
2. 遇到带 `record` 的 `CompactType` 时，生成一个候选节点。
3. `labels` 只取当前 `CompactType.vars`，不深入字段内部找变量。
4. 只有当前位置最终会形成交类型时，`vars` 才作为结构体合并标签。
   在当前 `coalesceCompactType` 规则下，`pol=false` 会合成交类型，`pol=true` 会合成并类型。
5. 不再计算 `structMergeOnlyLabels`。

这和前面讨论的模型对应：

```text
节点 = 结构体候选片段
标记 = 顶层交类型里的纯类型变量
冲突函数 = 两个结构体片段能否无损合并
```

第一版不要从字段内部变量建边，避免把只是共享泛型参数的结构体误合并。

## 第六阶段：结构体分组算法

采用缩紧后的整数规划问题，但实现用偏贪心近似。

约束：

```text
每个结构体组：
1. 要么只有一个节点；
2. 要么组内所有节点共享至少一个 label；
3. 组内任意两个节点不冲突。
```

算法：

```text
remaining = all candidate nodes

while remaining not empty:
  for each label:
    bucket = remaining nodes containing label
    group = greedy maximal conflict-free subset in bucket
    score = group size

  pick best group
  output group
  remove group nodes from remaining

afterwards:
  try to merge groups if they share a label and do not conflict
  try to insert singleton groups into existing compatible groups
```

冲突函数第一版只做无损判断：

```text
两个 record 如果同 offset 字段类型不可兼容，则冲突。
不同 offset 不冲突。
相同 offset 可递归兼容，则不冲突。
```

第一版不引入 `unknown` / union 弱化。弱化会改变目标函数，先不混进去。

## 第七阶段：结构体合并结果如何落地

分两步做，避免一次改太多。

### 7.1 只输出调试信息

先不改变最终类型，只输出：

- 每个候选结构体节点：
  - id
  - root
  - path
  - labels
  - record 摘要
- 每个分组：
  - group id
  - 共享 label
  - 包含哪些节点
  - 合并后的字段摘要

建议新日志文件：

```text
workdir/type-struct-merge.md
```

### 7.2 再影响 HType 结构体声明

确认分组合理后，再让 `TypeBuilder` 使用分组结果。

可选方案：

1. 在 `binarysub` 输出 sidecar：

   ```cpp
   struct StructMergeInfo { ... };
   struct BulkSimplifyResult {
     std::map<PolarVar, UTypePtr> types;
     StructMergeInfo structMerge;
   };
   ```

2. 保留旧 API：

   ```cpp
   std::map<PolarVar, UTypePtr> bulkSimplify(...);
   ```

   旧 API 内部调用新 API，只返回 `.types`。

3. `MLsubGenerator::genTypes` 后续改用详细 API，把 `StructMergeInfo` 传给 `TypeBuilder`。

这样能避免一次性破坏现有调用点。

## 第八阶段：测试计划

需要分层测试。

### binarysub 单元测试

1. co-occur 状态显式化：
   - 引入 `OccurrenceAnalysisState` 后，现有 `binarysub` 测试输出不变。

2. canonical struct merge 输入：
   - 构造两个 root。
   - canonicalize 后先抽取结构体候选。
   - 再分别做 per-root local simplify。
   - 检查结构体 sidecar 不依赖 global simplify 的中间结果。

3. 结构体候选抽取：
   - `α ∧ {0: int}` 和 `α ∧ {4: int}` 抽成两个节点，label 都是 `α`。
   - 字段内部出现的 `α` 不作为 label。

4. 贪心分组：
   - 无冲突节点进入同组。
   - 冲突节点被拆开。
   - 共享链但没有共同 label 的节点不能进同组。

### NotDec 集成测试

先只开 debug 输出，不影响最终类型。

重点看当前用例：

```text
test/type-recovery/realworld/cases/fortune.o3.wasm.ll
```

建议命令：

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

如果只想看目标用例，用现有 workdir/frozen IR 工作流跑 `fortune.o3.wasm.ll`。

## 风险

1. `recVars` / `newRecVars` 当前是成员状态，双阶段后容易串状态。
   需要优先显式化递归变量表。

2. `pol=true/false` 和外层日志里的 upper/lower 命名容易混。
   实现时以 `coalesceCompactType` 当前规则为准：
   - `pol=true` 合成 union。
   - `pol=false` 合成 intersection。

3. 结构体合并先不要改最终 HType。
   先输出分组调试，确认 `fortune` 上分组合理后再落地。

4. 结构体合并 sidecar 不能从最终 `UType` 反推。
   local simplify 后候选和 label 可能已经消失，必须在 canonical `CompactType`
   阶段记录。

## 建议实现顺序

1. 引入 `OccurrenceAnalysisState`，保持当前全局行为不变。
2. 抽出 `SimplificationPlan`，保持当前 simplify 输出不变。
3. 显式化 `recVars` 输入/输出，避免双阶段状态串扰。
4. 去掉结构体合并前的 global co-occur / global simplify。
5. 在 canonical `CompactType` 上抽取结构体候选节点。
6. 实现贪心分组和 `mergedBody`，先只输出 sidecar。
7. 确认分组合理后，再让分组影响 `TypeBuilder` 的结构体声明复用。

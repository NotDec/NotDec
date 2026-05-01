# 2026-05-01 字段 pointer-like 类型下沉到 `convertFieldType()` 计划

## 背景

`20260501-02-HTypeFullPointerSemanticsAttempt.md` 把 `RecordDecl::Field.Type`
统一改成了“字段地址类型”：

- 标量字段会被包成一层 `PointerType`
- 指针值字段会变成 `PointerType(DualPointerType(...))`
- padding 也一样走这套包装

这条路让“字段类型最后统一带一层指针语义”更整齐，但也带来了一个新问题：

- 结构体成员顶层不再直接出现 `DualPointerType`
- 真正“这个成员本身就是指针值”的语义，被最后那层统一包装吃掉了
- 后端如果只看字段顶层类型，很难区分：
  - 这是一个真的指针值字段
  - 还是先得到一个 pointer-like 字段类型，再被外面机械地套了一层普通 `PointerType`

现在的判断是：问题不在“字段要不要有这层指针语义”，而在于这层语义补得太晚、太粗。
所有字段本来就至少该有一层 pointer-like 语义，这层语义来自 `UType`，之前靠
`wrapFieldStorageTy()` 统一补，但它把顶层 `DualPointer` 压进了普通 `PointerType`
里面。

## 目标

把字段类型约定收紧成更贴近语义的一版：

1. `convertFieldType()` 直接返回最终字段类型，而不是先返回“字段里面的值类型”，最后再统一包一层
2. 这个最终字段类型始终是 pointer-like，至少带一层指针语义
3. 真正的指针值字段，顶层直接保留 `DualPointerType` 或普通 `PointerType`
4. llvm2c 后端不再默认“字段顶层一定是后来统一补上的那层普通 pointer”

希望最后达到两个效果：

- `ValueHTypes.txt` 里，成员类型能直接暴露 `DualPointerType`
- `.c` 降级时，能按字段顶层的 pointer-like 形状去解释语义，而不是先假定外面还有一层多余包装

## 当前假设

这次计划基于下面几条假设。如果后面验证发现不对，再回头改：

1. `FieldDecl.Type` 本质上就该是真实字段类型外面带一层指针语义的结果。
   这层语义沿用自 `UType`，而且顶层允许是 `DualPointerType`，也允许是普通
   `PointerType`。
2. 现在的问题不是“要不要有这层指针语义”，而是不能先算出字段真实类型，
   再在最后无差别套一层普通 `PointerType`。
3. 所以不是简单删掉 `wrapFieldStorageTy()`，而是把“最终字段类型长什么样”移到
   `convertFieldType()` 里直接决定。
4. 如果源码语义上真是“指针的指针”，那应该自然表现为更深一层的嵌套，而不是靠
   最后的统一包装伪造出来。

## 方案

### 1. 让 `convertFieldType()` 直接返回最终字段类型

现在更合适的边界不是“哪些字段要不要额外包一层”，而是每个 `UType` 分支直接返回
最终的 pointer-like 字段类型。

预期大致是：

- primitive / top / bottom / type variable fallback
  - 返回 `PointerType(primitive-like)`
- `UFunctionType`
  - 返回一层函数指针，不再在外面补第二层
- `UPointerType`
  - 直接返回 `DualPointerType`
- `URecordType`
  - 直接返回 `PointerType(convertStruct(...))`
- `UUnion` / `UInter`
  - 合并的也是最终字段类型，不再先合并“裸值类型”再外包

这样结构体字段顶层就能重新看到 `DualPointerType`，而不是统一变成
`PointerType(DualPointerType(...))`。

### 2. TypeBuilder 去掉最后统一包装

`TypeBuilder` 这边要从“最后统一包一层”改成“每个分支自己返回最终字段类型”。

大致会涉及这几类点：

- 普通 struct field
- union member
- recursive shell / anchor body
- padding 字段

其中 padding 仍然更像布局占位，预计继续保留普通 `PointerType(char[])` 这类形状。
但真实字段不能再无差别走同一个 `wrapFieldStorageTy()`。

### 3. llvm2c 的字段解包规则改成按类型分流

现在 `TypeManager` 默认字段顶层是一层后来统一补上的普通 `PointerType`。
这个假设如果不改，前端即便把 `DualPointerType` 暴露出来，后端还是会误解。

这里其实已经有一套现成的 `DualPointer` 选择逻辑，可以直接复用，不需要重新发明一版。

当前 `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp` 里已经有：

- `chooseDualPointerMemberType()`
- `lowerDualPointerView()`
- `stripStoredFieldAddressType()`

其中 `chooseDualPointerMemberType()` 的规则正好就是：

1. 如果 `load` / `store` 只有一边存在，就选存在的那一边。
2. 如果一边是 `top` 或 `bottom`，另一边不是，就选不是 `top/bottom` 的那一边。
3. 其余情况按 variance：
   - 协变选 `store`
   - 逆变选 `load`

所以这次后端部分不该再写成“新增一套复杂分流逻辑”，而应该写成：

- 保留现有 `DualPointer` 选择规则，直接复用 `chooseDualPointerMemberType()`
- 旧逻辑里默认“字段顶层先剥一层普通 `PointerType`”的地方，要改成适配新的
  `FieldDecl.Type` 约定
- 如果字段顶层直接就是 `DualPointerType`，就走现有选择函数
- 如果字段顶层是普通 `PointerType`，再按它里面的 pointee 继续处理

相关逻辑至少会影响：

- 字段声明 lowering
- decl usage / define 顺序
- by-value aggregate cycle 检测
- memory/global field 的字段类型打印

## 实现记录

### TypeBuilder

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:117-119`
  - 增加 `getFieldValueTy()`，用于在 TypeBuilder 内部把最终字段类型还原成字段值类型。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:652-677`
  - 新增 `getFieldValueTy()`。
  - 普通 `PointerType(T)` 剥成 `T`。
  - 顶层 `DualPointerType` 保留原样。
  - `SetUnionType` / `SetInterType` 递归剥每个 term。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:679-755`
  - `convertFieldType()` 改成直接返回最终字段类型。
  - `top` / `bottom` / primitive / type variable fallback 返回 `PointerType(...)`。
  - `UFunctionType` 返回一层函数指针，参数和返回值仍走普通 `convert()`。
  - `UPointerType` 顶层直接返回 `DualPointerType`。
  - `URecordType` 返回 `PointerType(convertStruct(...))`。
  - `UUnion` / `UInter` 合并最终字段类型。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:797-802`
  - recursive anchor field 改用 `convertFieldType(T.body, SizeBytes)`，不再对 body 统一外包。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1004-1087`
  - `craftStruct()` 现在认为 `Fields` 里的真实字段已经是最终字段类型。
  - 裁剪、char array 合并、数组扩展时先用 `getFieldValueTy()` 看字段值，再把裁剪/扩展结果包回字段存储形状。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1312-1369`
  - stride 数组字段生成 `PointerType(ArrayType(...))`。
  - `PreferElementType` 返回字段值类型，避免数组元素变成字段地址类型。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1490-1521`
  - union member 改成保存最终字段类型。
  - 多字段 panel 先构造成 struct 值，再作为 union member 字段包一层。

### llvm2c TypeManager

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:203-239`
  - `stripStoredFieldAddressType()` 支持新的顶层字段形状。
  - `PointerType(DualPointerType)` 和顶层 `DualPointerType` 都走同一套 `lowerDualPointerView()`。
  - `SetUnionType` / `SetInterType` 递归剥字段类型。

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:459-495`
  - by-value aggregate cycle 检测改为先用 `getStoredFieldValueType()` 取得字段值，再判断直接聚合。

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:863-893`
  - `calcUseRelation()` 统计字段 decl usage 时，先剥字段值类型，并递归处理 set term。

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:954-970`
  - struct 字段声明继续基于 `getStoredFieldValueType()` 输出 C 字段类型。

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:986-1012`
  - decl define 顺序遍历补上 set union/intersection 的 term 遍历。

### 测试和 oracle

- `test/tools/compare_htypes_with_wasm_dwarf.py:186-216`
  - realworld oracle 识别顶层 `ptr<load=struct_X,...>` 这种新 pointer-like 字段。

- `test/tools/compare_htypes_with_wasm_dwarf.py:353-361`
  - memory section 没有 `decl =>` 时，从 `type => struct_X*` 反推出 memory decl。

- 更新了这些 HType golden：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/06_SimpleRecursive2.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes`

- `test/type-recovery/sysy/manifest.json`
  - `26_while_test1`、`34_arr_expr_len`、`73_int_io` 从 `xfail` 改成 `pass`。
  - 新增对应 `expected/tr-level-2/*.htypes`。

## 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. llvm-ir suite

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

结果：通过。

3. sysy suite

```bash
ctest --test-dir build -R notdec.type_recovery.sysy.tr_level_2 --output-on-failure
```

结果：通过。

4. fortune 当前关注性能口径

```bash
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.selective-field-wrapper.final.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-selective-field-wrapper-final
```

结果：

- 跑通。
- `real 15.16s`
- 当前参考是 `real 16.33s`，没有性能退化。
- `/tmp/notdec-fortune-selective-field-wrapper-final/ValueHTypes.txt` 中字段顶层已经能直接看到
  `ptr<load=..., store=..., psize=...>`，不是统一的 `ptr<...>*`。

5. fortune `.c` 路径

```bash
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.selective-field-wrapper.final.out.c \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-selective-field-wrapper-final-c
```

结果：

- 跑通并生成 C。
- `real 19.05s`
- 仍有较多 llvm2c fallback warning，主要是 pointer arithmetic、global offset 和 set type lossy lowering。

6. realworld oracle

```bash
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
```

结果：未通过。

剩余失败不是崩溃，而是 fortune oracle 质量项：

- `free_desc::arg0`、`matches_in_list::arg0` 仍恢复成 `void**`，没有恢复到 `struct fd*`。
- `get_tbl::arg0` / `maxlen_in_list::arg0` 的 `read_tbl` 等字段在当前 layout 中被数组/padding 形状覆盖。
- `@File_list` 能识别到 record pointer，但选到的 recovered decl 仍缺少 `fd` 前半段字段。

## 当前判断

这次实现达成了计划里的核心语义目标：

- 字段类型不再最后统一外包一层普通 `PointerType`。
- 指针值字段顶层能直接暴露 `DualPointerType`。
- llvm2c 能按新字段顶层形状剥字段值类型。

但 fortune realworld oracle 暴露出一个后续问题：

- 新字段形状改变了 struct-merge / layout 选择的表现。
- 一些 fd 相关候选仍会退成 `void**` 或较窄 layout。
- 这不是本次“字段类型下沉”本身能完全解决的问题，后续需要单独看 struct merge 候选选择和数组/padding 覆盖字段的问题。

## 方案评分

- 实现效果：7/10。核心 HType 形状达成，`.ll` 和 `.c` 都能跑，基础 suite 通过；fortune oracle 还有质量缺口。
- 复杂度：6/10。新增了字段类型/字段值类型的内部切换，理解成本比统一 wrapper 高一些，但边界更贴近语义。
- 维护成本：6/10。后端和 oracle 都需要理解顶层 `DualPointerType`；后续还要收敛 struct merge 候选和 layout 覆盖问题。

更好的后续方案：

1. 给 TypeBuilder 增加明确的“字段最终类型”和“字段值类型”命名边界，减少 `wrap` / `strip` 成对出现的代码。
2. 单独处理 struct merge 候选选择，不要只按字段数量判断是否复用已有 decl。
3. 对数组/char padding 扩展加保护，避免覆盖后面有语义的字段 offset。

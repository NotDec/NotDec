# 2026-05-03 递归地址类型补 pointer wrapper

## 背景

`fortune` 的 `ValueHTypes.txt` 里：

```text
add_file::%_param_1.0 => lower=bottom:32 ; upper=struct_12
```

但对应 UType 是 `({@4: μ31} & Ptr<top,bottom>) as μ31`，PNDiff 也把它标成
pointer。这里不是 solver 少了指针，而是 `URecursiveType -> HType` 时，递归
binding 和递归回边被直接当成 by-value aggregate 打印了。

## 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:47-50`
  - 新增 `RecursiveTypeAddressLike`
  - 记录当前递归 binder 是否描述地址语义

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:274-305`
  - 新增 `hasAddressLikeSurface`
  - 如果递归 body 顶层或 set term 里包含 `URecordType`、`UPointerType`、
    `UFunctionType`，就认为这个递归类型是地址语义
  - zero-sized marker 不算地址语义

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:891-942`
  - 函数 `TypeBuilder::convertRecursive`
  - 地址语义递归现在返回 `PointerType(RecursiveBindingType)`
  - 转换 body 期间把地址语义挂到 `RecursiveTypeAddressLike`

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1830-1840`
  - 函数 `TypeBuilder::convertVariable`
  - 当前递归变量如果属于地址语义 binder，返回 `PointerType(RecursiveRefType)`

- `unittests/Retypd/TypeBuilderTest.cpp:101-123`
  - 更新 `TypeBuilderRecursiveRecordSetAnchorsPointeeRecord`
  - 断言递归 record/set 结果外层是 pointer，递归字段也保留 pointer

- 更新 HType golden：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/06_SimpleRecursive2.htypes:4-33`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes:4-27`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes:4-33`

## 验证

已运行：

```bash
cmake --build ./build --target TypeBuilderTest -j4
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec-decompile -j4
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-recursive-pointer-wrapper/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-recursive-pointer-wrapper
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' \
  --output-on-failure
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 \
  --output-on-failure
```

结果：

- `TypeBuilderTest` 通过
- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通，时间 `real 15.86s`、`user 15.49s`、`sys 0.37s`
- 对比参考 `real 16.33s`，没有看到性能回退
- 新输出中：
  - `add_file::%_param_1.0 => ... upper=struct_12*`
  - `add_file::%bb.loop_entry122.i2 => ... upper=struct_12*`
  - `struct_12 { struct_12* field_0; }`
- realworld oracle 仍失败，失败点仍是 `@File_list` 缺字段、`free_desc` /
  `matches_in_list` 为 `void**` 等质量项，不是本次崩溃或性能问题

## 简评

- 实现效果：8/10。选中的问题修掉了，相关基础 golden 已同步。
- 复杂度成本：7/10。多了一条“递归 body 是否地址语义”的判断，但只在
  `TypeBuilder` 内部使用。
- 维护成本：7/10。后续如果 binarysub 增加新的 address-like UType，需要把
  `hasAddressLikeSurface` 同步扩展。

更好的长期方案是让 `URecursiveType` 自己携带 value/address 语义，不靠
`TypeBuilder` 从 body 外形推断。

## 2026-05-04 注释补充

只补注释，未改行为：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:47-50`
  - 说明 `RecursiveTypeAddressLike` 的权衡：
    binder 仍锚到内部 struct/function 形状，但 value type 和递归回边
    可能需要 pointer wrapper。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:274-305`
  - 在 `hasAddressLikeSurface` 前补充设计说明：
    因为 `URecordType` 语义变成地址值，不能把 `mu r. {@4: r}` 直接绑定成裸
    `rec`，否则会丢掉 `struct_X*` 的外层。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:901-903,1834-1838`
  - 说明 `Binding` 是内部递归形状，`Result` / recursive ref 是值侧类型。

这次没有重跑测试，因为只改注释。

## 2026-05-05 工具函数整理

### 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:73-75`
  - 删除未使用的 `finalizeRecursiveType` 声明

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:297-317`
  - 删除只转发到 `isVagueFieldBoundType` 的 `isVagueUnionMemberType`
  - `simplifyUnionMembers` 直接调用 `isVagueFieldBoundType`

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:741-751`
  - 删除未使用的 `TypeBuilder::finalizeRecursiveType` 定义
  - 当前递归收束由 `convertRecursive` 直接完成

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:274-294`
  - `stripRecursiveValueSurface` / `stripObjectValueSurface` 改名为
    `stripRecursiveValuePointer` / `stripObjectValuePointer`
  - 注释里改用更直白的 value type / member type

### 验证

已运行：

```bash
cmake --build ./build --target TypeBuilderTest notdec-decompile -j4
./build/bin/TypeBuilderTest
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-typebuilder-utils-cleanup/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-typebuilder-utils-cleanup
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' \
  --output-on-failure
```

结果：

- `TypeBuilderTest` 通过
- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通，时间 `real 15.17s`、`user 14.82s`、`sys 0.34s`
- 对比参考 `real 16.33s`，没有看到性能回退

### 简评

- 实现效果：7/10。只清掉明显旧代码，没有改行为。
- 复杂度成本：9/10。净减少代码。
- 维护成本：8/10。少了一条已经不用的递归收束路径。

## 2026-05-05 简化：删除 RecursiveTypeAddressLike

### 背景

上一版的 `RecursiveTypeAddressLike` 是保守补丁：先扫描 recursive body 是否像地址
类型，再决定递归 value type 是否包一层 pointer。现在 `convertFieldType` 已经明确
做 value/member 两种视角转换，这个扫描和额外 map 不再必要，反而增加理解成本。

### 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:118-126`
  - 更新 `convertFieldType` 注释
  - 删除 `RecursiveTypeAddressLike` 成员

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:274-294`
  - 删除 `hasAddressLikeSurface`
  - 保留两个很小的剥壳 helper：
    `stripRecursiveValuePointer` 和 `stripObjectValuePointer`

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:782-804`
  - `getFieldAddressValueTy`
  - `DualPointer(load=null, store=null)` 作为最大未知 pointer field，落成 `top`

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:807-884`
  - `convertFieldType` 改为大部分走 `convert()` 后处理
  - `UUnion` / `UInter` 逐项递归调用 `convertFieldType`
  - `URecordType` 仍走 `convertPointer(Ty, FieldSizeBytes)`，避免丢字段大小裁剪
  - `UPointerType` 走普通 `convert()` 后用 load/store 选择成员类型
  - 当前递归变量作为字段时返回裸 `RecursiveRefType`
  - 非递归 type variable 不能 assert；fortune 里字符串尾部模型会出现这种字段

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:886-934,1822-1827`
  - `convertRecursive` 直接返回 `rec*`
  - binder 仍锚到内部递归对象
  - `convertVariable` 在 value context 里返回 `rec*`
  - 字段 context 由 `convertFieldType` 剥回 `rec`

- `unittests/Retypd/TypeBuilderTest.cpp:64-69`
  - `TypeBuilderTopFieldRecordLayoutTest` 不再直接把 primitive 放进 record field
  - 改成 `Ptr<uint,uint>`，字段选择后仍验证落成 `uint`

- 更新 HType golden：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/06_SimpleRecursive2.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes`

### 验证

已运行：

```bash
cmake --build ./build --target clean
cmake --build ./build --target TypeBuilderTest notdec-decompile -j4
./build/bin/TypeBuilderTest
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-field-convert-simple/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-field-convert-simple
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' \
  --output-on-failure
```

结果：

- `TypeBuilderTest` 通过
- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通，时间 `real 15.08s`、`user 14.65s`、`sys 0.42s`
- 对比参考 `real 16.33s`，没有看到性能回退
- 新输出中：
  - `add_file::%_param_1.0 => ... upper=struct_12*`
  - `addr(0x8f0) => ... upper=ptr<load=struct_23*, ...>`
  - `struct_23 { i8 field_0; struct_24 field_1; }`
  - `struct_24 { top:8 field_0; struct_24 field_1; }`

### 简评

- 实现效果：8/10。保留了值侧 `struct*` 和字段侧 embedded recursive 的行为。
- 复杂度成本：8/10。删掉了额外 map 和 body 预扫描，剩下的是显式的 value/member
  转换。
- 维护成本：8/10。以后新增对象状 UType 时只需要确认 `convert()` 的 value type
  和 `convertFieldType` 的剥壳规则，不需要再同步一套 address-like 检测。

## 2026-05-05 调整：字段成员视角保留 embedded recursive ref

### 背景

上一版把地址语义递归的 binding/ref 都按 value type 补成 `rec*`。
这修好了普通值类型，例如：

```text
add_file::%_param_1.0 => upper=struct_12*
```

但字段成员视角被一起影响了。`addr(0x8f0)` 对应的结构里，字段本来应该先按值内嵌：

```text
struct_23 { i8; struct_24 }
struct_24 { bottom:8; struct_24 }
```

上一版会输出成 `struct_24*`。这会把后续“按值自嵌转数组”的信息提前藏掉。

### 实现记录

修改文件：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:320-332`
  - 新增 `stripRecursiveValuePointer`
  - 只在类型是 `PointerType(RecursiveBindingType/RecursiveRefType)` 时剥回
    内部递归类型

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:871-879`
  - 函数 `TypeBuilder::convertFieldType`
  - 字段成员视角遇到当前递归变量时，直接返回 `RecursiveRefType`
  - 不再调用 value-facing 的 `convertVariable()`

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:927-931`
  - 字段成员视角遇到 `URecursiveType` 时，先走 `convertRecursive()`，再用
    `stripRecursiveValuePointer` 剥回 embedded member shape

- `unittests/Retypd/TypeBuilderTest.cpp:121-124`
  - 更新 `TypeBuilderRecursiveRecordSetAnchorsPointeeRecord`
  - 断言 record 字段里保留 `RecursiveRefType`，不是 `PointerType`

- 更新 HType golden：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes:4-13`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes:4-16`

### 验证

已运行：

```bash
cmake --build ./build --target TypeBuilderTest -j4
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec-decompile -j4
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-recursive-field-member/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-recursive-field-member
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' \
  --output-on-failure
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 \
  --output-on-failure
```

结果：

- `TypeBuilderTest` 通过
- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通，时间 `real 14.97s`、`user 14.58s`、`sys 0.38s`
- 对比参考 `real 16.33s`，没有看到性能回退
- 新输出中：
  - `addr(0x8f0) => ... upper=ptr<load=struct_23*, ...>`
  - `struct_23 { i8 field_0; struct_24 field_1; }`
  - `struct_24 { bottom:8 field_0; struct_24 field_1; }`
  - `add_file::%_param_1.0` 仍是 `struct_12*`
- realworld oracle 仍失败，失败点仍是 `@File_list` 缺字段、`free_desc` /
  `matches_in_list` 为 `void**` 等质量项

### 简评

- 实现效果：8/10。value type 和 field member shape 分开了。
- 复杂度成本：7/10。多了一个很小的剥 wrapper helper。
- 维护成本：7/10。后续最好把 value/member 两种转换边界显式化，减少靠 helper 修正。

## 2026-05-05 `convertFieldType` 注释补充

只补注释，未改行为：

- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:124-129`
  - 说明 `convertFieldType` 不是 `convert(Ty)` 后剥一层 pointer。
  - `URecordType` 作为 LLVM value 是地址语义，但作为字段成员要落到 pointee
    record；这个只是特例，不能推广成通用规则。

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:827-836`
  - 说明字段转换问的是“这个 offset 上的 C member 是什么”。
  - 具体区别：
    - `UPointerType` 要从 load/store 视图按 variance 选成员类型
    - 函数类型作为成员时仍是函数指针，不能剥成函数对象
    - set/intersection 要逐项在字段上下文里转换
    - 递归类型也要区分 value type 和 embedded member

这次没有重跑测试，因为只改注释。

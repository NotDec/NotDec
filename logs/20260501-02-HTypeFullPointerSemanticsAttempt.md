# 2026-05-01 HType 全量指针语义尝试

## 背景

前面确认过一个根问题：

- `binarysub` / `UType` 里的 `URecordType.fields[offset]` 更接近“成员地址节点”
- 但当前 `TypeBuilder` 会把它直接当成 `RecordDecl::Field.Type`
- 这样 HType 里“结构体 field”混了两层语义：
  - 一层是 layout 里的成员地址
  - 一层是最终字段值类型

这次先尝试把 HType 彻底往地址语义推：

- `RecordDecl` / `UnionDecl` 里的真实成员字段都存“字段地址类型”
- `RecordPtrType` 在 HType / snapshot / clang lowering 里改成“record object / embedded record”
- 真正的结构体指针由 `PointerType(RecordPtrType)` 表达

这一步先不追 llvm2c 全部变干净，只先做最小适配，保证 build 和基本路径能跑。

## 方案

1. `TypeBuilder`
   - `URecordType` 走 pointer 入口时，先构造 record，再外包一层 pointer
   - `RecordDecl::Field.Type` / `UnionDecl::Member.Type` 对真实成员统一包一层地址类型
  - `padding_*` 也按字段地址语义包一层 pointer

2. `HType` 打印与 lowering
   - `RecordPtrType` 不再打印成 `struct X*`
   - snapshot formatter 里也不再给 `RecordPtrType` 自动补 `*`
   - clang lowering 里，`RecordPtrType` 直接降成 `struct X`

3. `llvm2c` 最小兜底
   - 结构体/union/global field 声明时，先剥掉一层“字段地址 storage pointer”
   - decl 依赖遍历、usage 统计也按剥掉后的字段值类型看

## 实现

### `include/notdec/TypeRecovery/mlsub/TypeBuilder.h`

- `129-132`
  - 新增 `wrapFieldStorageTy()`，统一给真实字段包一层地址类型

### `src/TypeRecovery/mlsub/TypeBuilder.cpp`

- `643-647`，`finalizeRecursiveType()`
  - 递归 shell 单字段改成存地址类型
- `958-964`，`craftStruct()`
  - struct 真实字段改成存地址类型
- `1443-1449`，`convertStruct()`
  - union 成员改成存地址类型
- `1549-1566`，`convertPointer()`
  - `URecordType` 改成 `getPtrTy(convertStruct(...))`
- `1582-1584`
  - pointer 入口的结果断言同步放宽到 set type

### `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp`

- `200-201`
  - `HType::getAsString(TK_Record)` 去掉自动追加的 `*`
- `484-485`
  - snapshot formatter 的 `formatType(TK_Record)` 也去掉自动 `*`

### `external/NotDec-llvm2c/include/notdec-llvm2c/TypeManager.h`

- `43-47`
  - 新增 `getStoredFieldValueType()` / `convertStoredFieldType()`

### `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp`

- `151-163`
  - 新增 `stripStoredFieldAddressType()`，先剥 storage pointer
- `575-583`
  - `RecordPtrType` 改成 lowering 为 record object，而不是 `record*`
- `606-623`
  - `calcUseRelation()` 改成对字段先剥一层再看 decl usage
- `643-655`
  - `convertUnion()` 用 `convertStoredFieldType()`
- `647-660`
  - `convertStruct()` 用 `convertStoredFieldType()`
- `692-704`
  - `defineDecls()` 的字段依赖遍历改成先剥 storage pointer
- `1063-1065` 附近
  - `createMemoryDecls()` 的全局字段声明也改成先剥 storage pointer

## 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. fortune 当前关注口径 `.ll`

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.htype-pointer-semantics.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-htype-pointer-semantics
```

结果：

- 跑通
- `real 16.68s`
- 对比当前参考 `16.33s`，同档

3. HType 形状检查

`/tmp/notdec-fortune-htype-pointer-semantics/ValueHTypes.txt` 中已能看到：

- 标量字段：`top:32*`
- dual pointer 字段：`ptr<...>*`
- 原本单层 shell：比如 `struct_12 { struct_13** field_0; }`

这说明 HType 的真实字段已经切到“字段地址类型”。

4. fortune `.c` 路径

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.htype-pointer-semantics.out.c \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-htype-pointer-semantics-c
```

结果：

- 跑通，产出 `.c`
- `real 21.17s`
- 但有大量 llvm2c warning / fallback，主要是：
  - pointer arithmetic 里出现 `struct **` / `char **`
  - `failed to get field at offset ...`
  - `unsupported type for pointer arithmetic`

说明最小适配只够让路径不崩，还不够把“字段地址语义”完整落到成员访问和 pointer arithmetic 上。

5. 仓库调试目录

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.htype-pointer-semantics.repo.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/sn640/NotDec/work_dir
```

结果：通过，仓库里的 `work_dir/ValueHTypes.txt` 已刷新。

## 当前判断

这次尝试证明了一件事：

- 让 HType 全量转成指针语义是可落代码的
- build 和 `.ll` 调试导出都能先跑起来

但也同时暴露出下一步真正要补的是 llvm2c 的“字段值视角”：

- 仅仅在声明时剥掉 storage pointer 还不够
- 成员访问、ptradd、field offset 解析等地方，仍然在按旧值语义和新地址语义混用

## 评分

- HType 语义一致性：8/10
- 当前输出稳定性：5/10
- 维护成本：6/10

原因：

- HType / snapshot 这边更一致了
- 但 llvm2c 还没有把这套新模型完全吃进去
- 后续一定还要继续收 `TypeManager::handlePtrAdd()`、成员访问和字段值 lowering

## 下一步建议

优先补 llvm2c 这三块：

1. `TypeManager::handlePtrAdd()`
   - 现在对成员取址很容易多出一层 `&`
2. 字段值 lowering
   - 对 `PointerType(DualPointerType(...))` 这种 storage-address type，不能只剥一层就直接走通用 `convertType()`
3. `field offset -> field value` 的统一 helper
   - 避免声明、成员访问、global split 各自写一套剥层逻辑

## 追加实现：严格剥字段地址和 DualPointer 选择

这次把 `stripStoredFieldAddressType()` 从“能剥就剥，不能剥就原样返回”
改成严格语义：

- 普通 `PointerType` 必须有 pointee，返回 pointee
- `PointerType(DualPointerType)` 先把 DualPointer 降成一个普通指针视图
- 直接遇到 `DualPointerType` 时，按成员选择规则返回成员类型
- 其他类型直接 fatal error，避免静默把字段地址和值类型混用

DualPointer 成员选择规则：

1. load/store 只有一边存在时选存在的一边。
2. 一边是 top/bottom，另一边不是，选另一边。
3. 其余按 variance：协变选 store，逆变选 load。

padding 字段也改到 `TypeBuilder` 阶段统一包 storage pointer，所以 llvm2c 不再需要
padding 例外。普通字段和 padding 字段都必须能剥一层。

### 修改点

- `external/NotDec-llvm2c/include/notdec-llvm2c/TypeManager.h:44-51`
  - `getStoredFieldValueType()` 增加 variance 参数
  - 保留单一 `HType*` 入口，不再增加字段级 padding 例外
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:157-237`
  - 新增 fatal helper、DualPointer 成员选择和严格剥离逻辑
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:444-452`
  - `ClangTypeResult::getStoredFieldValueType()` 改为调用严格 helper
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:692-708`
  - `calcUseRelation()` 继续通过字段类型剥出字段值类型
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:729-764`
  - `convertUnion()` / `convertStruct()` 继续通过字段类型剥出字段值类型
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:776-814`
  - `defineDecls()` 避免对已经转成值类型的字段再次剥离
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1147-1160`
  - `createMemoryDecls()` 继续通过字段类型剥出字段值类型
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:912-919`
  - only-padding struct 字段也改成 `wrapFieldStorageTy(char[N])`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:1032-1078`
  - padding before / padding after / padding only 都改成 `wrapFieldStorageTy(char[N])`

### 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. fortune `.c` 路径

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.strict-strip-padding-wrapped.out.c \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-strict-strip-padding-wrapped
```

结果：

- 跑通
- `real 21.04s`
- 对比上一轮 `.c` 结果 `21.17s`，同档

中间曾暴露 padding 字段 `i8[1]` 被误传给严格剥离的问题。最终处理方式是：
在 `TypeBuilder` 的 padding 插入点补上 storage pointer，不在 llvm2c 里加 padding 例外。

### 本次判断

- 实现效果：7/10。真实字段失败路径更明确，DualPointer 不再走旧的 load-first fallback。
- 复杂度：6/10。新增了几个小 helper，但集中在 TypeManager 局部。
- 维护成本：7/10。padding 也进入统一字段地址语义后，llvm2c 侧少了特殊分支。

## 追加说明：旧值语义和新地址语义仍在混用的具体位置

这次最小适配只把“字段声明层”改到了新模型，但 `llvm2c` 里很多消费点仍然沿用旧模型：

- 旧模型：`Field.Type` 直接就是最终字段值类型
- 新模型：`Field.Type` 是字段地址类型，最终字段值类型要先剥一层 storage pointer

现在最大的症状就是：

- 字段声明处已经先剥一层了
- 但成员访问、ptradd、field offset 查找等地方，仍有不少路径直接消费旧/新混合后的类型

下面按位置说明。

### 1. 字段声明已经是“值视角”，但 ptradd 仍按“直接对字段类型做指针算术”处理

当前字段声明的最小适配发生在：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:647-652`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:656-660`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1065` 附近

这里都会先走 `convertStoredFieldType()`，也就是：

1. 先用 `getStoredFieldValueType()` 剥掉一层 storage pointer
2. 再把剥完后的类型落成 C 字段类型

所以 C 结构体字段声明现在已经回到了“字段值类型”。

但 `handlePtrAdd()` 还是老逻辑，入口就假设：

- `Base` 是“指向 struct value 的指针”
- `PointeeTy` 如果是 `RecordType`，就可以直接按 offset 找字段

对应代码：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1430-1470`

关键判断是：

```cpp
auto PointeeTy = Ctx.getCanonicalType(ValTy->getPointeeOrArrayElementType());
if (llvm::isa<clang::RecordType>(PointeeTy)) { ... }
```

问题在于，新模型下很多“字段地址”现在真的会表现成：

- `struct S **`
- `char **`
- `int **`

也就是“指向字段地址对象的指针”。

这时 `PointeeTy` 往往还是 `PointerType`，不是 `RecordType`，于是会直接走：

- `unsupported type for pointer arithmetic; falling back to byte-wise pointer arithmetic`

fortune `.c` 路径里大量这类告警就是这个原因。典型日志现象：

- `PointerType 'struct struct_3652 **'`
- `PointerType 'char **'`
- `PointerType 'void **'`

它们不是随机冒出来的，而是旧 `handlePtrAdd()` 还在拿新地址语义的中间值当旧值语义的 base。

### 2. `createMemberExpr()` 生成的是“字段值表达式”，但新模型里很多地方真正需要的是“字段地址表达式”

成员访问本身在：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/Utils.cpp:626-640`

`createMemberExpr()` 产出的是标准 `MemberExpr`，类型就是 `Field->getType()`。

而 `handlePtrAdd()` 在 struct 分支里做的是：

1. `DerefBase = deref(Base)`
2. `ME = createMemberExpr(DerefBase, FD)`
3. `Base = addrOf(ME)`

对应：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1465-1469`

这个套路在旧模型下是对的：

- 字段声明类型就是字段值类型
- `&base->field` 就是字段地址

但新模型下，字段声明已经被最小适配剥回“值类型”了，而 HType 原始字段语义是“字段地址类型”。
于是这里会出现一个混合状态：

- `FieldDecl::Type` 在 HType 里仍是地址语义
- Clang `FieldDecl::getType()` 已经是剥完后的值语义
- `createMemberExpr()` 返回的是值语义表达式
- `addrOf()` 再补回一层地址

这个组合对“最终 C 代码声明”是必要的，但对“中间语义推导”是不够显式的。很多路径默认“成员表达式的类型”和“HType 里的字段类型”还是一一对应，现在已经不成立了。

换句话说：

- HType 字段类型：地址语义
- Clang 字段声明类型：值语义
- `MemberExpr` 类型：值语义
- `&MemberExpr`：地址语义

现在代码里没有一个统一 helper 把这四层关系讲清楚，所以后面很容易继续拿错层。

### 3. `getFieldAt(offset)` 仍按 HType range 找 field，但命中的 field 对应的 Clang 类型已经不是原 HType 类型

field offset 命中逻辑在：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:661-672`

这里只看 `Field.R.Start/Size`，不看类型。按 offset 命中到 `ast::FieldDecl` 后，
`handlePtrAdd()` 会取它的 `ASTDecl`，再用这个 Clang field 去生成成员访问。

也就是说现在这条链路是：

1. offset 命中基于 HType 里的布局 range
2. 成员表达式类型基于剥层后的 Clang field type

这本身不是错，但要求布局和剥层后的声明仍然严格对齐。

fortune `.c` 路径里出现的：

- `failed to get field at offset -1696`
- `failed to get field at offset -1536`
- `failed to get field at offset 7`

说明现在至少有一部分 offset 解释还没跟上新模型。常见原因有两类：

1. `Base` 已经多了一层地址，导致 offset 是针对“字段地址对象”算的，不再是针对 record object 本体算的
2. 字段声明被剥层后，某些 embedded-record / pointer-field 的边界在后续 ptradd 里被当成了错误层级

这一点从日志里能看到一个典型结构：

- 某个 struct 定义里字段已经是值语义，比如 `char *`、`struct struct_3926`
- 但 ptradd 查 offset 时却还是在旧模型的路径上找“下一级 field”

于是最终只能 fallback 到 byte-wise pointer arithmetic。

### 4. 依赖遍历和 usage 统计已经剥层，但 value path 还没统一

我这次已经把下面几处改成了先剥层再看：

- `calcUseRelation()`：`TypeManager.cpp:606-623`
- `defineDecls()` 的 `VisitType()`：`TypeManager.cpp:692-704`

这样做只是为了：

- embedded record 还能先定义
- decl usage 不会因为 storage pointer 多一层就看漏

但这仍然只是“声明依赖层”的修补。

真正的值路径，比如：

- `getType(ExtValuePtr Val)`
- 某些 load/store 结果的 C type 选择
- ptradd 后再进入成员访问的表达式类型流转

仍然大量依赖旧语义下的 `convertType()` 结果，没有统一改成“先区分这是字段地址，还是字段值”。

所以现在会看到两种现象同时出现：

1. declaration path 勉强能跑
2. expression path 告警很多，而且经常变成 `**`

### 5. 这次最小适配实际上只覆盖了“声明层剥一层”，没有覆盖“表达式层剥一层”

可以把当前状态理解成：

- 已完成：type declaration projection
  - HType 字段地址类型 -> C 字段值声明类型
- 未完成：expression projection
  - HType 字段地址表达式 -> C 字段值表达式 / 字段地址表达式

而 `llvm2c` 当前最依赖表达式层正确性的地方就是：

- `handlePtrAdd()`
- `createMemberExpr()`
- 后续 load/store / member access 的 type query

所以这次 `.c` 路径虽然能跑通，但 warning 很多，本质不是“某几个 case 特殊”，而是整条表达式路径还停在旧模型。

### 一个更直接的总结

当前代码里，至少有三种“字段相关类型”在同时存在：

1. `ast::FieldDecl::Type`
   - 现在是地址语义
2. `clang::FieldDecl::getType()`
   - 现在是剥层后的值语义
3. `handlePtrAdd()` / `MemberExpr` 中间值
   - 还在部分按旧值语义假设，部分又接到了新地址语义的 base

真正的问题不是哪一个单点实现错了，而是：

- 这三种类型之间还没有统一、显式的转换 helper
- 目前只有“声明层”用了 `getStoredFieldValueType()`
- “表达式层”还没有对应的 field-address / field-value 投影工具

后续如果继续做这条路，建议先补一个统一的 helper 层，把下面两件事分开：

1. 从 HType field storage type 取“字段值类型”
2. 从某个 base expression + field decl 取“字段值表达式”或“字段地址表达式”

否则每个消费点各自剥一层、补一层，迟早还会继续打架。

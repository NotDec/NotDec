# 2026-04-24 Set-type vector storage plan

## 背景

当前 `UType` 和 `HType` 里的 set-theoretic union / intersection 都还是二叉节点：

- `UUnion { lhs, rhs }`
- `UInter { lhs, rhs }`
- `SetUnionType { Lhs, Rhs }`
- `SetInterType { Lhs, Rhs }`

打印层已经有 flatten 逻辑，避免输出大量括号。但底层仍然是二叉树，所以很多遍历、打印、lowering 还需要重复处理同类嵌套。

这次先不改变“顺序是否等价”的语义。也就是说：

- `a | b` 和 `b | a` 暂时仍然可以是不同结构
- `a & b` 和 `b & a` 暂时仍然可以是不同结构
- 不做排序
- 不做去重
- 不做更复杂的代数化简

只先把同类 set 节点的存储形式从二叉改成 vector，并在构造时 flatten 同类嵌套。

## 计划

### 1. UType 改为 vector 存储

目标文件：

- `external/binarysub/include/binarysub/binarysub.h`
- `external/binarysub/src/binarysub.cpp`
- `src/TypeRecovery/simplesub/simplesub.cpp`

计划修改：

1. 把 `UUnion` 改成：

```cpp
struct UUnion {
  std::vector<UTypePtr> types;
};
```

2. 把 `UInter` 改成：

```cpp
struct UInter {
  std::vector<UTypePtr> types;
};
```

3. 保留现有二元 factory：

```cpp
make_uunion(UTypePtr lhs, UTypePtr rhs)
make_uinter(UTypePtr lhs, UTypePtr rhs)
```

二元 factory 内部负责：

- 如果参数本身是同类 `UUnion` / `UInter`，展开到当前 vector
- 否则直接追加
- 不排序
- 不去重

4. 新增 vector factory：

```cpp
make_uunion(std::vector<UTypePtr> types)
make_uinter(std::vector<UTypePtr> types)
```

vector factory 同样只做同类 flatten。

5. 更新所有遍历逻辑：

- `get_size()`
- `printTypeImpl()`
- `normalizeVariableNames()`
- `collect_utype_variable_origins()` 相关 visitor
- `MLsubGenerator.cpp` 里的 UType 变量收集

这些地方从访问 `lhs/rhs` 改成遍历 `types`。

6. 更新生成逻辑：

- `coalesceType()`
- `coalesceCompactType()`

当前多项 bounds / components 是用二元 left-fold 逐步构造 union/inter。改成直接收集 vector 后调用 vector factory。

### 2. HType SetUnion / SetInter 改为 vector 存储

目标文件：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`

计划修改：

1. 把 `SetUnionType` 改成保存：

```cpp
std::vector<HType *> Types;
```

2. 把 `SetInterType` 改成保存：

```cpp
std::vector<HType *> Types;
```

3. 提供统一访问接口：

```cpp
llvm::ArrayRef<HType *> getTypes() const;
```

4. `HTypeContext` 中保留二元接口：

```cpp
getSetUnionType(bool IsConst, HType *Lhs, HType *Rhs)
getSetInterType(bool IsConst, HType *Lhs, HType *Rhs)
```

二元接口内部转成 vector 接口。

5. 新增 vector 接口：

```cpp
getSetUnionType(bool IsConst, std::vector<HType *> Types)
getSetInterType(bool IsConst, std::vector<HType *> Types)
```

vector 接口负责：

- flatten 同类 `SetUnionType` / `SetInterType`
- 不排序
- 不去重
- 如果最终只有一个元素，直接返回这个元素

6. `TypeBuilder::doUnion()` / `TypeBuilder::doInter()` 保持简单：

- 两边完全相同则返回原类型
- canonical type 相同则返回 canonical type
- 否则调用 HTypeContext 的二元 set factory

这里暂时删除或弱化“按指针顺序交换 lhs/rhs”的逻辑，因为本轮不把不同顺序合并成同一个节点。

### 3. 简化打印和 lowering

目标文件：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp`
- `external/binarysub/src/binarysub.cpp`
- `src/TypeRecovery/simplesub/simplesub.cpp`

计划修改：

1. UType 打印：

- `UUnion` / `UInter` 直接遍历 `types`
- 不再需要递归 `collectUnionTerms()` / `collectInterTerms()`

2. HType 打印：

- `renderSetType()` 直接读取 `getTypes()`
- snapshot formatter 仍然可以按当前策略排序字符串输出，保证 snapshot 稳定
- 普通 `getAsString()` 不排序，保留构造顺序

3. HType lowering：

- `TypeManager.cpp` 中 set-like lowering 直接遍历 `getTypes()`
- 不再需要 worklist 递归 flatten 同类节点

### 4. 测试

优先修改已有测试，不扩大范围：

- `external/binarysub/src/binarysub-test.cpp`
- `unittests/Retypd/TypeBuilderTest.cpp`

验证点：

1. 嵌套 union 打印仍然 flatten：

```text
i8 | i16 | i32 | f32
```

2. 嵌套 intersection 打印仍然 flatten：

```text
i8 & i16 & (i32 | f32)
```

3. HType snapshot 输出仍然稳定。

4. 构造顺序暂时仍然保留，不新增 `a | b == b | a` 的断言。

建议执行：

```bash
cmake --build ./build --target TypeBuilderTest binarysub -j4
./build/binarysub
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec-decompile -j4
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-set-vector/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-set-vector
```

## 暂不做

本轮明确不做：

1. 不排序 set terms
2. 不去重 set terms
3. 不把 `a | b` 和 `b | a` 视为同一个节点
4. 不引入完整 set algebra 化简
5. 不改变 C lowering 的选型策略，只把遍历从二叉改成 vector

## 预期收益

1. UType / HType 的结构更贴近实际的 union/intersection 链。
2. 打印和 lowering 不再需要到处写二叉 flatten。
3. 后续如果要做交换律等价，只需要在 vector factory 里加排序和去重，不必再先改数据结构。

## 涉及函数清单

计划会涉及但尚未修改的函数：

- `binarysub::make_uunion()`
- `binarysub::make_uinter()`
- `binarysub::get_size()`
- `binarysub::printTypeImpl()`
- `binarysub::normalizeVariableNames()`
- `binarysub::TypeSimplifier::coalesceType()`
- `binarysub::TypeSimplifier::coalesceCompactType()`
- `notdec::ast::HTypeContext::getSetUnionType()`
- `notdec::ast::HTypeContext::getSetInterType()`
- `notdec::ast::HType::getAsString()`
- `notdec::ast::HTypeSnapshotFormatter::collectType()`
- `notdec::ast::HTypeSnapshotFormatter::formatType()`
- `notdec::llvm2c::ClangTypeResult::convertType()`
- `mlsub::TypeBuilder::doUnion()`
- `mlsub::TypeBuilder::doInter()`


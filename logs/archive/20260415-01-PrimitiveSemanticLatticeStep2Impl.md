# Primitive Semantic Lattice Step 2 实现记录

日期：2026-04-15

前置提交：

- 子模块 `external/binarysub` Step 1 提交：`6569b53`
- 主仓库 Step 1 记录提交：`97c053a`

## 完成情况

已完成 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 中的 Step 2：

- 在 family descriptor 上预计算祖先闭包
- 提供父节点查询
- 提供祖先集合查询
- 提供 `same family` 判断
- 提供 `A <: B` 判断

当前状态：

- 仍未接入 `external/binarysub/src/binarysub-core.cpp` 的 primitive 求解逻辑
- `join` / `meet` 还未实现

## 具体修改

### 1. 在 family descriptor 上补齐 Step 2 查询接口

文件：

- `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:47-153`

涉及类/接口：

- `binarysub::PrimitiveSemanticFamilyDescriptor::parentNodesOf()`
- `binarysub::PrimitiveSemanticFamilyDescriptor::ancestorNodesOf()`
- `binarysub::PrimitiveSemanticFamilyDescriptor::isSubtypeOf()`
- `binarysub::PrimitiveSemanticRegistry::findFamilyByCanonicalName()`
- `binarysub::PrimitiveSemanticRegistry::areInSameFamily()`

说明：

- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:80-102` 为 family descriptor 新增了 Step 2 基础查询接口
- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:129-136` 为 registry 新增了按 canonical name 反查 family 与 `same family` 判断接口
- 同时在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:100-102` 新增 `ancestorIndicesByNode_`，作为后续 subtype / join / meet 的缓存基础

### 2. 预计算祖先闭包并实现 subtype/family 查询

文件：

- `external/binarysub/src/binarysub-primitive-semantics.cpp:605-792`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:829-872`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:932-949`

涉及函数：

- `build_family_descriptor(const ParsedFamily &parsed)`
- `PrimitiveSemanticFamilyDescriptor::parentNodesOf(const PrimitiveSemanticNodeDescriptor &)`
- `PrimitiveSemanticFamilyDescriptor::ancestorNodesOf(const PrimitiveSemanticNodeDescriptor &)`
- `PrimitiveSemanticFamilyDescriptor::isSubtypeOf(const PrimitiveSemanticNodeDescriptor &, const PrimitiveSemanticNodeDescriptor &)`
- `PrimitiveSemanticRegistry::findFamilyByCanonicalName(std::string_view) const`
- `PrimitiveSemanticRegistry::areInSameFamily(std::string_view, std::string_view) const`

说明：

- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:757-790` 的 `build_family_descriptor()` 末尾新增了祖先闭包预计算
- 预计算结果按“直接父节点优先，再向 root 扩展”的顺序保存在 `ancestorIndicesByNode_`
- `parentNodesOf()` 负责返回 immediate parents
- `ancestorNodesOf()` 负责返回 transitive ancestor closure
- `isSubtypeOf()` 采用“相等或在祖先闭包中”的规则判断 `A <: B`
- `findFamilyByCanonicalName()` / `areInSameFamily()` 提供跨 family 的 registry 级查询

### 3. 扩展测试覆盖 Step 2 场景

文件：

- `external/binarysub/src/binarysub-test.cpp:426-538`

涉及函数：

- `test_primitive_semantic_lattice()`

说明：

- 在 `external/binarysub/src/binarysub-test.cpp:475-490` 新增了 direct parents、ancestor closure、`isSubtypeOf()` 的断言
- 在 `external/binarysub/src/binarysub-test.cpp:507-538` 新增了第二个 family，用来验证：
  - `findFamilyByCanonicalName()`
  - `areInSameFamily()`
  - 跨 family 返回 false

## 验证

执行命令：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 构建成功
- `test_primitive_semantic_lattice()` 在 Step 1 校验基础上继续覆盖了 Step 2 查询能力
- 现有 `binarysub` 测试全部通过

## 后续建议

Step 3 可以直接复用当前 `ancestorIndicesByNode_` 做 family 内 LUB 计算：

- 先求两个节点的公共 upper bounds
- 再选“最低”的那个作为唯一 `join`

如果后续需要 `meet`，也可以在当前缓存基础上补“后代闭包”或按祖先关系反向推导。

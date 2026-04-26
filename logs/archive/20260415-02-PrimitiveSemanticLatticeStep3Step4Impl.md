# Primitive Semantic Lattice Step 3-4 实现记录

日期：2026-04-15

## 完成情况

已完成 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 中的 Step 3 和 Step 4：

- 在 family descriptor 上实现同 family 的 `join`
- 在 family descriptor 上实现同 family 的 `meet`
- 在 registry 上实现基于 canonical name 的 `join` / `meet` 入口
- 补充成功路径、跨 family 报错、无 GLB 冲突、LUB/GLB 歧义等测试

当前状态：

- 仍未接入 `external/binarysub/src/binarysub-core.cpp`
- 目前 `join` / `meet` 只在 primitive semantic lattice registry 层可用

## 具体修改

### 1. 扩展 family / registry 接口

文件：

- `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:94-159`

涉及函数：

- `binarysub::PrimitiveSemanticFamilyDescriptor::joinOf()`
- `binarysub::PrimitiveSemanticFamilyDescriptor::meetOf()`
- `binarysub::PrimitiveSemanticRegistry::joinByCanonicalName()`
- `binarysub::PrimitiveSemanticRegistry::meetByCanonicalName()`

说明：

- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:94-106` 新增 family 内 `join` / `meet` 接口
- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:152-159` 新增 registry 级 canonical-name 包装接口
- `joinOf()` 约定：
  - 只处理同 family 节点
  - 若存在多个最小上界，则报错为歧义 LUB
- `meetOf()` 约定：
  - 只处理同 family 节点
  - 若不存在 GLB，直接报类型冲突
  - 若存在多个最大下界，也直接报类型冲突

### 2. 实现 family 内 LUB / GLB 计算

文件：

- `external/binarysub/src/binarysub-primitive-semantics.cpp:605-620`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:893-1002`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:1079-1120`

涉及函数：

- `upper_bound_indices_of(const PrimitiveSemanticFamilyDescriptor &, std::size_t)`
- `canonical_pair_string(const PrimitiveSemanticNodeDescriptor &, const PrimitiveSemanticNodeDescriptor &)`
- `binarysub::PrimitiveSemanticFamilyDescriptor::joinOf(const PrimitiveSemanticNodeDescriptor &, const PrimitiveSemanticNodeDescriptor &) const`
- `binarysub::PrimitiveSemanticFamilyDescriptor::meetOf(const PrimitiveSemanticNodeDescriptor &, const PrimitiveSemanticNodeDescriptor &) const`
- `binarysub::PrimitiveSemanticRegistry::joinByCanonicalName(std::string_view, std::string_view) const`
- `binarysub::PrimitiveSemanticRegistry::meetByCanonicalName(std::string_view, std::string_view) const`

说明：

- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:605-615` 新增 `upper_bound_indices_of()`，复用 Step 2 的祖先闭包生成“自身 + 全部上界”
- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:893-949` 实现 `joinOf()`：
  - 先求两个节点的公共 upper bounds
  - 再选唯一的 minimal upper bound 作为 LUB
  - 若没有公共上界，报 family 非法/内部错误
  - 若 minimal upper bound 不唯一，报歧义 LUB
- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:951-1002` 实现 `meetOf()`：
  - 遍历同 family 所有节点，收集公共 lower bounds
  - 再选唯一的 maximal lower bound 作为 GLB
  - 若没有公共 lower bound，报类型冲突
  - 若 maximal lower bound 不唯一，报歧义 GLB
- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:1079-1120` 实现 registry 包装接口，并对 unknown node / cross-family 情况直接报错

### 3. 扩展测试覆盖 Step 3 / Step 4

文件：

- `external/binarysub/src/binarysub-test.cpp:424-653`

涉及函数：

- `expect_query_error(const expected<T, Error> &, const char *)`
- `test_primitive_semantic_lattice()`

说明：

- 在 `external/binarysub/src/binarysub-test.cpp:424-438` 新增 `expect_query_error()`，用于验证 `join` / `meet` 的错误消息
- 在 `external/binarysub/src/binarysub-test.cpp:508-521` 覆盖基础 family 上的成功/失败路径：
  - `join(HWND, HBITMAP) = HANDLE`
  - `join(HANDLE, SOCKET) = root`
  - `meet(HANDLE, HWND) = HWND`
  - `meet(HWND, HBITMAP)` 无 GLB，按设计报冲突
- 在 `external/binarysub/src/binarysub-test.cpp:571-591` 覆盖 registry 级 canonical-name 入口和跨 family 报错
- 在 `external/binarysub/src/binarysub-test.cpp:593-653` 新增两组歧义图：
  - `ambiguous_join`：验证歧义 LUB 报错
  - `ambiguous_meet`：验证歧义 GLB 报错

## 验证

执行命令：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 构建成功
- `test_primitive_semantic_lattice()` 已覆盖 Step 1-4 的 registry/family 层能力
- 现有 `binarysub` 测试全部通过

## 后续建议

Step 5 可以直接围绕当前 registry 包装接口推进：

- 把 `(base, bits, node_name)` 到 semantic node 的查找入口补齐
- 统一 `join` / `meet` 错误格式，方便后续接到 `binarysub-core.cpp`
- 若后续需要性能优化，可再补 family 内 descendant closure 或更直接的 LUB/GLB cache

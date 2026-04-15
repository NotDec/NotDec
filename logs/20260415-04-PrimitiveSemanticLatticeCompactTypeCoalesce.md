# Primitive Semantic Lattice 在 CompactType Coalesce 阶段的合并记录

日期：2026-04-15

## 背景

在完成 Step 5 / Step 6 后，primitive semantic lattice 已经能影响
`binarysub-core.cpp` 的 primitive subtype 约束路径。

但 `binarysub` 在最终把 `CompactType` 合成为 `UType` 时，`CompactType::prims`
里的多个 primitive 仍然只是直接展开成多个 `UPrimitiveType` 组件，再按 polarity
用 `union` / `intersection` 串起来。

这会导致：

- 即使多个 primitive 属于同一个 semantic family
- 并且已经有可用的 `join` / `meet`

最后展示出来的结果仍然可能是：

- `prim.uint32.win.HWND | prim.uint32.win.HBITMAP`
- 或 `prim.uint32.win.HANDLE & prim.uint32.win.HWND`

而不是更紧凑的 family-local semantic lattice 结果。

## 本次修改

### 1. 调整 `coalesceCompactType()` 接口，显式传入 polarity

文件：

- `external/binarysub/include/binarysub/binarysub.h:264-267`

涉及函数：

- `binarysub::TypeSimplifier::coalesceCompactType(const CompactTypePtr &, bool, bool)`

说明：

- 在 `external/binarysub/include/binarysub/binarysub.h:264-267` 为
  `coalesceCompactType()` 增加了显式 `pol` 参数
- 这样顶层 negative polarity 的 `CompactType` 在 coalesce 时也能正确走
  semantic lattice 的 `meet` 路径，而不是总是按 positive polarity 处理

### 2. 在 `CompactType::prims` 合成阶段增加同 family primitive 的格内合并

文件：

- `external/binarysub/src/binarysub.cpp:38-107`
- `external/binarysub/src/binarysub.cpp:1423-1520`
- `external/binarysub/src/binarysub.cpp:1602-1624`

涉及函数：

- `coalesce_primitive_components(const SimpleTypeSet &, bool)`
- `binarysub::TypeSimplifier::coalesceCompactType(const CompactTypePtr &, bool, bool)`
- `binarysub::TypeSimplifier::bulkSimplify(const std::set<PolarVar> &, bool)`
- `binarysub::TypeSimplifier::simplify(SimpleType, bool, bool)`

说明：

- 在 `external/binarysub/src/binarysub.cpp:38-107` 新增
  `coalesce_primitive_components()`：
  - 先按 semantic family 对 `CompactType::prims` 分组
  - 对同 family primitive：
    - positive polarity 尝试做 `join`
    - negative polarity 尝试做 `meet`
  - 若格内合并成功，则输出单个 `UPrimitiveType`
  - 若格内合并失败，则退回旧行为，保留多个 primitive 组件
- 在 `external/binarysub/src/binarysub.cpp:1515-1520`，
  `coalesceCompactType()` 的 “Add primitives” 阶段改为调用该 helper
- 在 `external/binarysub/src/binarysub.cpp:1602-1624`，`bulkSimplify()` 和
  `simplify()` 传递正确的 polarity 给 `coalesceCompactType()`

### 3. 为 primitive coalesce 合并补充测试

文件：

- `external/binarysub/src/binarysub-test.cpp:472-482`
- `external/binarysub/src/binarysub-test.cpp:672-687`

涉及函数：

- `expect_compact_type_string(const CompactTypePtr &, bool, const char *)`
- `test_primitive_semantic_lattice()`

说明：

- 在 `external/binarysub/src/binarysub-test.cpp:472-482` 新增
  `expect_compact_type_string()`，直接检查 `CompactType -> UType` 的结果
- 在 `external/binarysub/src/binarysub-test.cpp:672-687` 新增 3 组断言：
  - positive polarity：
    - `HWND` 与 `HBITMAP` 合成为 `HANDLE`
  - negative polarity：
    - `HANDLE` 与 `HWND` 合成为 `HWND`
  - fallback：
    - `HWND` 与 `HBITMAP` 在 negative polarity 下无 GLB，继续保留显式
      intersection

## 验证

执行命令：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 构建成功
- 新增 primitive coalesce 合并测试通过
- 现有 `binarysub` 测试全部通过

## 效果

现在 `CompactType::prims` 在最终展示成 `UType` 时，若多个 primitive 来自同一
semantic family，会优先尝试用 lattice 结果压缩：

- positive polarity 更偏向 `LUB`
- negative polarity 更偏向 `GLB`

只有在格内无法稳定合并时，才退回原本的显式 `union` / `intersection`
表示。

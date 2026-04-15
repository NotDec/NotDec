# Primitive Semantic Lattice Step 5-6 实现记录

日期：2026-04-15

## 完成情况

已完成 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 中的 Step 5 和 Step 6：

- 在 registry 层补齐后续接入求解器所需的统一入口
- 暴露 process-global primitive semantic lattice registry
- 将 primitive semantic lattice 接入 `external/binarysub/src/binarysub-core.cpp` 的 primitive 约束路径
- 补充 primitive subtype 约束测试

当前状态：

- primitive semantic lattice 已能影响 `binarysub` 的 primitive subtype 约束
- 当前接入点仍只覆盖 primitive 与 primitive 的约束路径
- 还没有扩展到更多 simplifier/pretty-printer 语义压缩策略

## 具体修改

### 1. 补齐 Step 5 的统一接口

文件：

- `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:134-190`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:1039-1175`

涉及函数：

- `binarysub::PrimitiveSemanticRegistry::findNode(std::string_view, std::uint32_t, std::string_view, std::string_view) const`
- `binarysub::PrimitiveSemanticRegistry::clear()`
- `binarysub::globalPrimitiveSemanticRegistry()`
- `binarysub::clearGlobalPrimitiveSemanticRegistry()`

说明：

- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:139-142` 新增了按 `(base, bits, namespace, nodeId)` 查 semantic node 的统一入口
- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:186-190` 暴露了 process-global registry，供 Step 6 的 solver 接入使用
- 在 `external/binarysub/src/binarysub-primitive-semantics.cpp:1133-1175` 实现了 `clear()` 和 global registry 包装

### 2. 将 semantic lattice 接入 primitive 约束路径

文件：

- `external/binarysub/src/binarysub-core.cpp:188-231`
- `external/binarysub/src/binarysub-core.cpp:451-466`

涉及函数：

- `constrain_semantic_primitive_subtype(const TPrimitive &, const TPrimitive &)`
- `constrain_impl(const SimpleType &, const SimpleType &, std::function<void(const SimpleType &, const SimpleType &)>)`

说明：

- 在 `external/binarysub/src/binarysub-core.cpp:188-231` 新增 `constrain_semantic_primitive_subtype()`：
  - 若 primitive 名字未注册到 semantic lattice，则退回旧行为，由名字相等判断
  - 若两个 primitive 属于同一 family，则使用 registry 的 `join` / `meet` 判断 subtype 关系
  - 若 primitive size 与注册 family bits 不一致，则直接报错，防止错误配置混入求解
- 在 `external/binarysub/src/binarysub-core.cpp:451-466` 的 primitive 分支里接入该 helper：
  - 仍然先走 `lp->name == rp->name` 的快速路径
  - 否则尝试 semantic subtype
  - 若 semantic subtype 成立，则约束成功
  - 否则维持 primitive 约束失败

### 3. 扩展测试覆盖 Step 5 / Step 6

文件：

- `external/binarysub/src/binarysub-test.cpp:440-658`

涉及函数：

- `expect_primitive_constrain_ok()`
- `expect_primitive_constrain_error()`
- `test_primitive_semantic_lattice()`

说明：

- 在 `external/binarysub/src/binarysub-test.cpp:440-469` 新增 primitive subtype 约束断言 helper
- 在 `external/binarysub/src/binarysub-test.cpp:626-658` 补充了 Step 5 / Step 6 测试：
  - `registry.findNode("uint", 32, "win", "HWND")`
  - 注册 global registry
  - `constrain(prim.uint32.win.HWND, prim.uint32.win.HANDLE)` 成功
  - `constrain(prim.uint32.win.HWND, prim.uint32.win.root)` 成功
  - `constrain(prim.uint32.win.HWND, prim.uint32.win.HBITMAP)` 失败
  - `constrain(prim.uint32.win.HWND, prim.char8.cchar.digit)` 失败
  - semantic node name 与 `size` 不匹配时报错
  - 旧 primitive 名字相等/不等等价路径仍保持原行为

## 验证

执行命令：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 构建成功
- `test_primitive_semantic_lattice()` 已覆盖 Step 1-6
- 现有 `binarysub` 测试全部通过

## 后续建议

后续如果要继续打磨，可优先考虑两件事：

1. 是否把 semantic primitive 的输入/加载入口挂到主程序 CLI 或配置层
2. 是否在 simplifier / pretty-printer 里进一步利用 `join` / `meet` 压缩 primitive 结果展示

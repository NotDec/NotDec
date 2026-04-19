# fortune.o3.wasm llvm2c demoteSSA UAF 修复

日期：2026-04-19

## 1. 现象

针对：

- `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`

执行：

```bash
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.o3.wasm.c \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-fortune-uaf-fix
```

此前会在 `llvm2c` 阶段触发 UAF，典型栈落在：

- `notdec::llvm2c::demoteSSAFixHT`
- `notdec::llvm2c::demoteSSA`
- `llvm::Value::getValueID`

并由 ASAN 报：

- `heap-use-after-free`

## 2. 根因

`demoteSSAFixHT()` 旧逻辑只处理了一类引用：

1. `HT.ValueTypes` 里 “PHI 自己作为 key” 的条目

也就是只做了：

- 读取 `HT.ValueTypes[&PN]`
- 把 PHI 的类型迁到 `NameMap`
- `HT.ValueTypes.erase(&PN)`

但还有两类悬空引用没有同步清理：

1. `HT.ContraVariantValues` 里直接保存的 `&PN`
2. `HT.ValueTypes` / `HT.ContraVariantValues` 里
   `UConstant{ User = &PN }`

一旦 `demoteSSA()` 删除了这些 PHI，后续再对这些残留 `ExtValuePtr` 调：

1. `toString()`
2. `canonicalizeExtValue()`
3. `isa<GlobalValue>()`
4. 以及其他需要检查 `llvm::Value` 动态类型的逻辑

就会访问已经释放的 PHI 对象，从而触发 UAF。

## 3. 修改

修改位置：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1469](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1469)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1498](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1498)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1557](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1557)

涉及函数：

- `notdec::llvm2c::demoteSSAFixHT`

### 3.1 新增 PHI 删除集合

新增：

1. `DemotedValues`
   - 收集所有即将被 `demoteSSA()` 删除的 PHI
2. `ContraVariantNameSet`
   - 记录这些 PHI 在 demote 前是否属于 contravariant value

### 3.2 统一清理所有引用到旧 PHI 的 `ExtValuePtr`

新增 `referencesDemotedValue()` 判定：

1. `llvm::Value* == PHI`
2. `UConstant.User == PHI`
3. `StackObject/HeapObject` 若引用到待删 value 也一起过滤

然后在 `demoteSSA()` 之前统一从：

1. `HT.ValueTypes`
2. `HT.ContraVariantValues`

中删除所有引用这些 PHI 的条目。

这样就不会在 PHI 被 `eraseFromParent()` 后继续持有悬空 `ExtValuePtr`。

### 3.3 把 PHI 的 contravariant 极性迁到新 alloca

demote 后在识别 `.reg2mem` alloca 时，除了恢复原来的类型：

- `HT.ValueTypes.insert({&AI, Ty})`

现在还会检查：

- 该 PHI 在 demote 前是否位于 `ContraVariantNameSet`

如果是，则同步执行：

- `HT.ContraVariantValues.insert(&AI)`

避免修 UAF 的同时把原本的极性信息丢掉。

## 4. 验证

### 4.1 构建

```bash
cmake --build ./build --target notdec-decompile -j2
```

### 4.2 fortune 回归

```bash
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.o3.wasm.c \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-fortune-uaf-fix
```

结果：

1. 不再出现 `heap-use-after-free`
2. 成功输出：
   - `/tmp/fortune.o3.wasm.c`
   - `/tmp/notdec-fortune-uaf-fix/ValueCTypes.txt`
3. 日志尾部出现：

```text
Decompile result: /tmp/fortune.o3.wasm.c
```

### 4.3 语法检查

```bash
clang-14 -fsyntax-only -Wno-everything /tmp/fortune.o3.wasm.c
```

结果：通过。

### 4.4 侧面检查

在 `ValueCTypes.txt` 里搜索：

- `phi`
- `PHI`
- `reload`
- `reg2mem`

当前可以看到的是 demote 后留下的：

1. `.pre-phi` alloca
2. `*.reload` load

而不是原先已经被删除的旧 PHI key，这和修复预期一致。

## 5. 当前状态

这次修掉的是：

- `llvm2c demoteSSA` 之后 `HTypeResult` 里残留旧 PHI / `UConstant(User=PHI)`
  导致的 UAF

当前 `fortune` 仍然还有的不是 UAF，而是既有 backend 退化问题，例如：

1. `Structure analysis stopped making progress, quitting. Func: main`
2. `failed to get field at offset ...`
3. `unsupported type for pointer arithmetic`

这些现在不会阻止 `.c` 落盘，也和本次 UAF 根因不同。

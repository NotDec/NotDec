# 2026-04-20 DualPointer access-size fix

## 背景

这次修一个 `DualPointerType` / `UPointerType` 的语义错位：

1. `binarysub::UPointerType::psize` 在 `UType` 侧表示 pointer access size
2. 但 `TypeBuilder` 转成 `HType` 时，之前传给 `DualPointerType` 的却是
   模块指针位宽 `Parent.PointerSize * 8`
3. 结果 `ImportantHTypes.txt` / `.htypes` 里很多
   `ptr<load=i8, store=i8, psize=32>` 实际是在表达“按 8-bit 访问的 32-bit
   地址”，语义已经和 `UType.psize` 脱钩

这次目标：

- `HType` 内部字段改名成 `AccessSize`
- `TypeBuilder` 构造 `DualPointerType` 时保留 `UPointerType::psize`
- 输出继续沿用 `psize=...` 文本标签，但语义与 `UType` 侧一致

## 修改

### 1. `DualPointerType` 内部字段改成 `AccessSize`

文件：

- [external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h](/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h#L406)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp#L174)

涉及函数：

- `DualPointerType::DualPointerType()`
- `DualPointerType::getAccessSize()`
- `HType::getAsString()`
- `HTypeSnapshotFormatter::formatType()`
- `HTypeContext::getDualPointerType()`

具体改动：

1. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:409-424`
   - `DualPointerType` 成员从 `PointerSize` 改成 `AccessSize`
   - getter 从 `getPointerSize()` 改成 `getAccessSize()`

2. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:690-705`
   - `HTypeContext::getDualPointerType()` 的 key / 参数名同步改成 `AccessSize`

3. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:174-183`
   - `HType::getAsString()` 里的 `psize=` 改为打印 `getAccessSize()`

4. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:440-447`
   - snapshot formatter 里的 `psize=` 也改为打印 `getAccessSize()`

### 2. `TypeBuilder` 保留 `UType.psize`

文件：

- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L412)
- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L1242)

涉及函数：

- `TypeBuilder::convertFieldType()`
- `TypeBuilder::convertPointer()`

具体改动：

1. `src/TypeRecovery/mlsub/TypeBuilder.cpp:418-420`
   - 叶子 `UPointerType` 转成 `DualPointerType` 时，之前传
     `Parent.PointerSize * 8`
   - 现在改成直接传 `V->psize`

2. `src/TypeRecovery/mlsub/TypeBuilder.cpp:1242-1244`
   - 顶层 `UPointerType` 转 `DualPointerType` 的路径也同样改成 `V->psize`

这样 `UType` 到 `HType` 的 `psize` 语义保持一致。

### 3. 现有 `TypeManager` 的宽度估计同步读 `AccessSize`

文件：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L295)

涉及函数：

- `estimateTypeWidthBitsImpl()`

具体改动：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:295-296`
  - `DualPointerType` 分支从 `getPointerSize()` 改成 `getAccessSize()`

### 4. 更新受影响 snapshot golden

文件：

- [test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes)

变化原因：

- `psize` 现在按真实 access size 导出
- 一些之前只剩 `top:32` 的 stack-address roots 也因此更稳定地 materialize
  成了 `struct_*`

## 验证

执行：

```bash
cmake --build ./build --target all -j4

./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-accesssize-check2/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-accesssize-check2

ctest --test-dir build \
  -R notdec.type_recovery.llvm_ir.tr_level_2 \
  --output-on-failure

ctest --test-dir build \
  -R notdec.type_recovery.realworld.tr_level_2 \
  --output-on-failure
```

结果：

1. `fortune` 的 `ImportantHTypes.txt` 里现在同时出现：
   - `ptr<load=i8, store=i8, psize=8>`
   - `ptr<load=..., store=..., psize=32>`
2. `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过
3. `ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure` 通过

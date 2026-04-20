# 2026-04-20 Set-type pretty-print flatten

## 背景

`HTypes` 和 `UTypes` 在打印 `union` / `intersection` 时，之前按二叉树递归直接输出：

- `((a | b) | c)`
- `(a & (b & c))`

这会把同类 set-type 链打印成层层括号，读 `ImportantHTypes.txt` /
`ValueHTypes.txt` 时可读性很差。

本次目标：

1. 同类 `union` / `intersection` 链先扁平化再打印
2. 只有在父上下文优先级更高时才加括号
3. `HTypes` 和 `UTypes` 两边保持一致

## 修改

### 1. `HTypes` 增加 precedence-aware set printer

文件：

- [external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h](/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h#L303)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp#L10)

涉及函数：

- `HType::getAsString()`
- `HType::getAsString(int Precedence)`
- `HTypeSnapshotFormatter::formatType(const HType *)`
- `HTypeSnapshotFormatter::formatType(const HType *, int Precedence)`
- `collectSetTerms()`
- `renderSetType()`

具体改动：

1. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:305-306`
   - 给 `HType` 增加私有递归入口 `getAsString(int Precedence)`

2. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:831-832`
   - 给 `HTypeSnapshotFormatter` 增加私有递归入口
     `formatType(const HType *, int Precedence)`

3. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:10-70`
   - 新增 precedence 常量
   - 新增 `collectSetTerms()` 扁平化 `SetUnionType` / `SetInterType`
   - 新增 `renderSetType()` 统一负责 join 和按需加括号

4. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:178-268`
   - `HType::getAsString()` 改成走 precedence-aware 递归
   - `TK_SetUnion` / `TK_SetInter` 不再固定输出二叉括号
   - `TK_Pointer` / `TK_Array` / `TK_Function` 子节点也开始按上下文决定是否补括号

5. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:450-548`
   - `HTypeSnapshotFormatter::formatType()` 同步改成 precedence-aware
   - snapshot 路径会对扁平化后的同级分量做排序，保持输出稳定

### 2. 两套 `UTypes` printer 同步扁平化

文件：

- [external/binarysub/src/binarysub.cpp](/sn640/NotDec/external/binarysub/src/binarysub.cpp#L18)
- [src/TypeRecovery/simplesub/simplesub.cpp](/sn640/NotDec/src/TypeRecovery/simplesub/simplesub.cpp#L8)

涉及函数：

- `printTypeImpl()`
- `collectUnionTerms()`
- `collectInterTerms()`
- `printSetTerms()`

具体改动：

1. `external/binarysub/src/binarysub.cpp:18-58`
   - 新增 `collectUnionTerms()` / `collectInterTerms()` / `printSetTerms()`

2. `external/binarysub/src/binarysub.cpp:421-450`
   - `UUnion` / `UInter` 打印改为先 flatten 再 join
   - `UFunctionType` 参数/返回值也改用统一 precedence 常量

3. `src/TypeRecovery/simplesub/simplesub.cpp:8-48`
   - 为仓库内 simplesub 版本补同样的 helper

4. `src/TypeRecovery/simplesub/simplesub.cpp:347-363`
   - `UUnion` / `UInter` 改为扁平输出

### 3. 补测试

文件：

- [external/binarysub/include/binarysub/binarysub-test.h](/sn640/NotDec/external/binarysub/include/binarysub/binarysub-test.h#L12)
- [external/binarysub/src/binarysub-test-main.cpp](/sn640/NotDec/external/binarysub/src/binarysub-test-main.cpp#L35)
- [external/binarysub/src/binarysub-test.cpp](/sn640/NotDec/external/binarysub/src/binarysub-test.cpp#L554)
- [unittests/Retypd/TypeBuilderTest.cpp](/sn640/NotDec/unittests/Retypd/TypeBuilderTest.cpp#L82)

涉及函数：

- `test_utype_pretty_printing()`
- `expect_printed_utype()`
- `Retypd.HTypeSetPrettyPrintingFlattensChains`

具体改动：

1. `external/binarysub/src/binarysub-test.cpp:565-578`
   - 新增 `test_utype_pretty_printing()`
   - 覆盖：
     - `i8 | i16 | i32 | f32`
     - `i8 & i16 & (i32 | f32)`

2. `unittests/Retypd/TypeBuilderTest.cpp:82-101`
   - 新增 `HType` / snapshot formatter 的链式 flatten 断言

## 验证

执行：

```bash
cmake --build ./build --target TypeBuilderTest binarysub -j4
./build/binarysub
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec-decompile -j4
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-pretty-print/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-pretty-print
```

结果：

1. `./build/binarysub` 通过
2. `./build/bin/TypeBuilderTest` 通过
3. `fortune` 的 `ImportantHTypes.txt` 已从二叉括号链变为扁平输出，例如：
   - `store='m52:32 | 'v16:32 | 'y50:32`
   - `store='p76:8 | 'z78:8 | i8 | i8`
   - `load='a8:32 & 'd8:32 & 'g8:32 & ...`

## 备注

当前仍会在“混合优先级”位置保留必要括号，例如：

- `i8 & i16 & (i32 | f32)`

这部分是刻意保留，用来避免把 `intersection` 和 `union` 混在一起后失去结构信息。

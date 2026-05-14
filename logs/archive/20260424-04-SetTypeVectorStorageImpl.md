# 2026-04-24 Set-type vector storage 实现记录

## 背景

按 `logs/20260424-03-SetTypeVectorStoragePlan.md` 落地：

- `UType` 的 `UUnion` / `UInter` 从二叉节点改成 vector 存储
- `HType` 的 `SetUnionType` / `SetInterType` 从二叉节点改成 vector 存储
- 本轮仍然：
  - 不排序
  - 不去重
  - 不把 `a | b` 和 `b | a` 视为同一结构

另外核对后确认，计划里写的 `src/TypeRecovery/simplesub/simplesub.cpp` 在当前仓库不存在，所以实际没有这个文件可改。

## 代码修改

1. `external/binarysub/include/binarysub/binarysub.h:44-52, 96-152, 190-218`
   - 涉及函数：
     - `binarysub::make_uunion(std::vector<UTypePtr>)`
     - `binarysub::make_uunion(UTypePtr, UTypePtr)`
     - `binarysub::make_uinter(std::vector<UTypePtr>)`
     - `binarysub::make_uinter(UTypePtr, UTypePtr)`
     - `binarysub::get_size(const UTypePtr &)`
   - 修改内容：
     - `UUnion` / `UInter` 改成 `std::vector<UTypePtr> types`
     - 新增同类 flatten helper `append_flattened_set_terms`
     - 二元 factory 改成转调 vector factory
     - vector factory 在构造时 flatten 同类嵌套
     - `get_size()` 从二叉取 size 改成遍历 `types`

2. `external/binarysub/src/binarysub.cpp:22-38, 40-79, 220-353, 398-463, 859-879, 1773-1781`
   - 涉及函数：
     - `visit_utype_variables_impl()`
     - `normalizeVariableNames()`
     - `printTypeImpl()`
     - `coalesceType()`
     - `coalesceCompactType()`
   - 修改内容：
     - 删除基于二叉树的 `collectUnionTerms()` / `collectInterTerms()`
     - visitor、rename、pretty-print 全部改成遍历 `types`
     - `coalesceType()` / `coalesceCompactType()` 不再二元 left-fold
     - 改成先收集 vector，再一次性调用 vector factory

3. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1700-1716`
   - 涉及函数：
     - `collectUTypeVariableDetailsImpl()`
   - 修改内容：
     - `UUnion` / `UInter` 的变量收集从 `lhs/rhs` 改成遍历 `types`

4. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:431-457, 553-556, 755-829`
   - 涉及函数：
     - `HTypeContext::getSetUnionType(bool, HType *, HType *)`
     - `HTypeContext::getSetUnionType(bool, std::vector<HType *>)`
     - `HTypeContext::getSetInterType(bool, HType *, HType *)`
     - `HTypeContext::getSetInterType(bool, std::vector<HType *>)`
   - 修改内容：
     - `SetUnionType` / `SetInterType` 改成保存 `std::vector<HType *> Types`
     - 新增统一访问接口 `getTypes()`
     - context 的 set cache key 从二元 tuple 改成 term vector
     - vector factory 负责 flatten 同类 set
     - 如果 flatten 后只剩一个元素，直接返回该元素

5. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:23-40, 42-75, 257-271, 412-425, 542-552`
   - 涉及函数：
     - `collectSetTerms()`
     - `renderSetType()`
     - `HType::getAsString(int) `
     - `HTypeSnapshotFormatter::collectType()`
     - `HTypeSnapshotFormatter::formatType(const HType *, int)`
   - 修改内容：
     - set 打印改成直接读取 `getTypes()`
     - 常规打印仍保留构造顺序
     - snapshot formatter 仍然按字符串排序，保持 golden 稳定
     - snapshot 收集 set 子项时改成遍历 `getTypes()`

6. `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:225-239, 324-345, 462-520`
   - 涉及函数：
     - `canLowerToClangTypeImpl()`
     - `estimateTypeWidthBitsImpl()`
     - `ClangTypeResult::convertType(HType *)` 里的 `lowerSetLikeType`
   - 修改内容：
     - lowering 可达性检查改成遍历 `getTypes()`
     - set 宽度估计改成对所有 term 取最大可用位宽
     - lowering 侧删除递归 worklist flatten
     - 直接使用已经 flatten 的 `getTypes()`

7. `src/TypeRecovery/mlsub/TypeBuilder.cpp:211-240, 409-478, 501-606, 1234-1311`
   - 涉及函数：
     - 新增 helper `convertVisibleSetTerms()`
     - 新增 helper `maxSetTermMetric()`
     - `TypeBuilder::convertFieldType()`
     - `TypeBuilder::convert()`
     - `TypeBuilder::accessedPointeeSizeInBits()`
     - `TypeBuilder::convertPointer()`
     - `TypeBuilder::doUnion()`
     - `TypeBuilder::doInter()`
   - 修改内容：
     - `TypeBuilder` 侧所有 `UUnion` / `UInter` 消费点改成遍历 `types`
     - 对 zero-sized record marker 仍然过滤，但改成在线性遍历里处理
     - `accessedPointeeSizeInBits()` 改成对所有 term 取最大值
     - 删除 `doUnion()` / `doInter()` 里按指针顺序交换 lhs/rhs 的逻辑

8. `external/binarysub/src/binarysub-test.cpp:620-632`
   - 涉及函数：
     - `test_utype_pretty_printing()`
   - 修改内容：
     - 除了打印断言，再补一层内部结构断言
     - 直接检查 `UUnion` / `UInter` 的 `types.size()`

9. `unittests/Retypd/TypeBuilderTest.cpp:90-107`
   - 涉及函数：
     - `Retypd.HTypeSetPrettyPrintingFlattensChains`
   - 修改内容：
     - 新增 `SetUnionType` / `SetInterType` 的 `getTypes().size()` 断言
     - 同时保留原有常规打印和 snapshot 打印断言

## 验证

1. 构建
   - 命令：
     - `cmake --build ./build --target notdec-decompile -j4`
     - `cmake --build ./build --target TypeBuilderTest -j1`
     - `cmake --build ./build --target binarysub -j1`
   - 结果：
     - 全部通过
   - 备注：
     - 期间 `binarysub` 有一次链接阶段 `ld` 总线错误，重试 `-j1` 后通过；没有暴露代码层面的编译错误

2. 单测
   - 命令：
     - `./build/bin/TypeBuilderTest`
     - `./build/binarysub`
   - 结果：
     - `TypeBuilderTest` 4 个测试全部通过
     - `binarysub` 全部测试通过

3. `fortune` 定点验证
   - 命令：
     - `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-set-vector/out.ll --tr-level=2 -g --work-dir=/tmp/notdec-set-vector`
   - 结果：
     - 运行完成
     - 成功输出 `/tmp/notdec-set-vector/out.ll`

## 结论

这次已经把 `UType` / `HType` 的 set-like 节点底层结构从二叉改成了 vector，
并把打印、visitor、type builder、lowering、测试一起收齐。

当前语义边界仍然和计划一致：

- 构造顺序保留
- 不排序
- 不去重
- 不做交换律等价合并

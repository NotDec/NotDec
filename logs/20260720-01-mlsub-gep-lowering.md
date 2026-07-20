# MLsub GEP lowering 实现记录

## 原始需求

动态 GEP 能得到 常量 + 下标 * 步长，能转换成OffsetRange的话也行。那还是可以支持起来的，可以试试

## 背景

x64 source IR 会保留 LLVM `getelementptr`。之前 `MLsubVisitor::visitGetElementPtrInst` 只处理全零 GEP，其余直接报警告并不生成字段约束，导致 fortune 这类 IR 的结构字段信息明显不足。

## 实现

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:251` 新增 `getSigned64APInt()`，统一把 LLVM `APInt` 保守转成 `int64_t`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:258` 新增 `getGEPOffsetRange()`：先用 `GEPOperator::accumulateConstantOffset()` 处理常量 GEP；失败后用 `GEPOperator::collectOffset()` 得到 `常量 + value * scale`，并转成 `OffsetRange.offset + OffsetRange.access`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:305` 新增 `lowerGEPOperatorAsPtrAdd()`，把 GEP base/result/offset 接到已有 `ConstraintsGenerator::setAsPtrAdd()`，继续复用 BinarySub record field 和 PA field edge。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:331` 新增 `lowerConstantExprGEPAddress()`，处理 load/store 地址里的 constant expr GEP，以及 bitcast-of-GEP remap。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5910`、`5949` 在 `visitLoadInst()` / `visitStoreInst()` 里先尝试 lowering constant expr GEP 地址；table GEP 仍走旧的跳过逻辑。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5998` 改造 `visitGetElementPtrInst()`，非 table GEP 直接 lowering；不支持的 vector、负 base offset、负动态步长仍保守跳过。
- `external/binarysub/include/binarysub/HType.h:48` 给 `TypedDecl` 加 virtual destructor。GEP lowering 会生成更多 record/union decl，默认 ASAN 会暴露 `unique_ptr<TypedDecl>` 删除派生类的类型不匹配问题。
- `unittests/Retypd/TypeBuilderTest.cpp:32`、`85`、`108`、`143`、`213` 把 `TypeBuilderContext` 构造参数从旧的 `DataLayout` 改为当前 API 需要的 pointer size bytes。
- 更新 `test/type-recovery/llvm-ir/expected/tr-level-2/` 下 4 个 HType golden：`06_SimpleRecursive2`、`09_OffsetLoop`、`20_PointerAnalysisFieldCycle`、`21_PointerAnalysisBranchingFieldCycle`。变化主要是 GEP 字段被恢复成更具体的 struct/array 形状。

## 验证

- `cmake --build ./build --target notdec binarysub -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过，21 个 case 全部通过。
- `./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll -o /tmp/notdec-fortune-x64-gep-20260720-3/out.ll --tr-level=2 --dump-htypes=/tmp/notdec-fortune-x64-gep-20260720-3/fortune.htypes.txt --gen-work-dir --work-dir=/tmp/notdec-fortune-x64-gep-20260720-3/work`：通过，`fortune.htypes.txt` 生成 14462 行，日志没有 GEP lowering warning。
- `./build/binarysub`：通过。
- `cmake --build ./build --target TypeBuilderTest -j4 && ./build/bin/TypeBuilderTest`：通过，6 个 case 全部通过。

## 评价

- 实现效果：8/10。x64 source IR 的普通 GEP、动态下标 GEP、constant expr GEP 都能进入类型约束；fortune 已能生成更丰富 HType。
- 复杂度：6/10。主要复杂度来自 constant expr/bitcast 地址 remap，以及避免负偏移破坏 record size 计算。
- 维护成本：5/10。依赖 LLVM 自带 offset API，后续主要风险是 `OffsetRange` 不记录动态下标变量身份，只记录步长。

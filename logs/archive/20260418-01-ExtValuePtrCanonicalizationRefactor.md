# 2026-04-18 ExtValuePtr canonicalization refactor

## Summary

本次修改把 `(llvm::Value *, llvm::User *, OpInd)` 到 `ExtValuePtr` 的包装逻辑收口到统一入口，减少 `ConstraintGenerator` / `MLsubGenerator` / llvm2c `TypeBuilder` 内部“先传三元组、函数开头再 `llvmValue2ExtVal(...)`”的重复模式。

## Modified files and functions

1. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h`
   - 第 88-105 行新增 `canonicalizeExtValue(...)` 声明，并让 `llvmValue2ExtVal(...)` 直接委托给该入口。
   - 相关函数：
     - `canonicalizeExtValue`
     - `llvmValue2ExtVal`

2. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp`
   - 第 246-271 行实现统一 canonicalization 逻辑。
   - `canonicalizeExtValue(...)` 现在同时处理：
     - 常量按 use-site 包装成 `UConstant`
     - `inttoptr(ConstantInt)` 折叠成 `ConstantAddr`
   - `getExtValuePtr(...)` 改为复用 `canonicalizeExtValue(...)`，避免与 `llvmValue2ExtVal(...)` 的逻辑分叉。
   - 相关函数：
     - `canonicalizeExtValue`
     - `getExtValuePtr`

3. `include/notdec/Passes/ConstraintGenerator.h`
   - 第 392-445 行把核心接口拆成“canonical `ExtValuePtr` 主入口 + 兼容三参包装入口”。
   - 相关函数：
     - `getNode`
     - `getNodeOrNull`
     - `createNode`
     - `getOrInsertNode`
     - `getTypeVar`
     - `convertTypeVar`

4. `src/Passes/ConstraintGenerator.cpp`
   - 第 3166-3276 行对应实现改为内部只消费 canonical `ExtValuePtr`。
   - `convertTypeVar(...)` 在 `UConstant` 分支里改为直接使用 `IC->User` / `IC->OpInd`，不再依赖额外形参。
   - 相关函数：
     - `ConstraintsGenerator::getNodeOrNull`
     - `ConstraintsGenerator::getNode`
     - `ConstraintsGenerator::createNode`
     - `ConstraintsGenerator::getOrInsertNode`
     - `ConstraintsGenerator::getTypeVar`
     - `ConstraintsGenerator::convertTypeVar`

5. `include/notdec/TypeRecovery/mlsub/PNDiff.h`
   - 第 211-252 行给 `PNIGraph` 增加 canonical `ExtValuePtr` 主入口，三参版本只负责预包装。
   - 相关函数：
     - `PNIGraph::createPNINode`
     - `PNIGraph::getOrInsertPNINode`

6. `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`
   - 第 118-245 行把 `MLsub` 约束生成器内部的节点创建、查询、`setPointer` / `setNonPointer` / `getSize` 等接口改成优先接收 canonical `ExtValuePtr`。
   - 保留三参重载作为兼容层，在入口先 `canonicalizeExtValue(...)`。
   - 相关函数：
     - `convertSimpleType`
     - `createNode`
     - `getNodeOrNull`
     - `getOrInsertNode`
     - `addRemapType`
     - `setPointer`
     - `setNonPointer`
     - `getSize`

7. `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
   - 第 2465-2486 行对应实现改为直接处理 canonical `ExtValuePtr`。
   - `UConstant` 分支直接使用 `IC->User` / `IC->OpInd`。
   - 相关函数：
     - `ConstraintsGenerator::convertSimpleType`

8. `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h`
   - 第 184-200 行把 `TypeBuilder::getHighType` / `getType` / `getTypeL` 改成“canonical 主入口 + 三参兼容入口”。
   - 相关函数：
     - `TypeBuilder::getHighType`
     - `TypeBuilder::getType`
     - `TypeBuilder::getTypeL`

9. `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp`
   - 第 2238-2260 行 `TypeBuilder::getType(...)` 改为直接消费 canonical `ExtValuePtr`。
   - 第 2365-2370 行 `ExprBuilder::visitConstant(...)` 改为通过 `getExtValuePtr(...)` 构造 use-site aware key。
   - 相关函数：
     - `TypeBuilder::getType`
     - `ExprBuilder::visitConstant`

## Validation

1. 编译：

```bash
cmake --build ./build --target notdec-decompile -j4
```

2. 针对当前关注用例做直接回归：

```bash
./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/notdec-extvalueptr-check/fortune.out.ll \
  --tr-level=2 \
  --dump-htypes /tmp/notdec-extvalueptr-check/fortune.out.htypes
```

结果：

- 编译成功
- `fortune.o3.wasm.4.ll` 运行成功
- 产出了 `.ll` 与 `.htypes`


# 2026-04-18 Remove ExtValuePtr compatibility overloads

## Summary

这次收尾把 `ConstraintGenerator`、`MLsubGenerator`、`PNDiff` 和 llvm2c
`TypeBuilder` 内部保留的
`(ExtValuePtr, User *, OpInd)` 兼容式重载全部去掉，调用点统一直接传 canonical
`ExtValuePtr`。现在仓库里保留三参数形态的地方只剩边界转换入口
`getExtValuePtr(...)` / `llvmValue2ExtVal(...)`，不再在内部 API 上继续兼容旧调用方式。

## Modified files and functions

1. `/sn640/NotDec/include/notdec/Passes/ConstraintGenerator.h:392-420`
   - 删除内部兼容重载，只保留 canonical `ExtValuePtr` 主接口。
   - 涉及函数：
     - `ConstraintsGenerator::getNode`
     - `ConstraintsGenerator::getNodeOrNull`
     - `ConstraintsGenerator::createNode`
     - `ConstraintsGenerator::getOrInsertNode`
     - `ConstraintsGenerator::getTypeVar`
     - `ConstraintsGenerator::addAddConstraint`
     - `ConstraintsGenerator::addSubConstraint`
     - `ConstraintsGenerator::addCmpConstraint`

2. `/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:118-225`
   - 删除 `MLsub` 约束生成器内部对旧三参数模式的兼容入口。
   - `addRemapType(...)` 保持为两个 `ExtValuePtr` 版本。
   - 涉及函数：
     - `ConstraintsGenerator::convertSimpleType`
     - `ConstraintsGenerator::createNode`
     - `ConstraintsGenerator::getNodeOrNull`
     - `ConstraintsGenerator::getOrInsertNode`
     - `ConstraintsGenerator::addRemapType`
     - `ConstraintsGenerator::setPointer`
     - `ConstraintsGenerator::setNonPointer`
     - `ConstraintsGenerator::getSize`
     - `ConstraintsGenerator::setAsPtrAdd`

3. `/sn640/NotDec/include/notdec/TypeRecovery/mlsub/PNDiff.h:211-247`
   - 删除 `PNIGraph` 内部兼容重载，`PNIMap` 只通过 canonical key 建点和查询。
   - 涉及函数：
     - `PNIGraph::createPNINode`
     - `PNIGraph::getPNIVarOrNull`
     - `PNIGraph::getPNIVar`
     - `PNIGraph::getOrInsertPNINode`

4. `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3402-3782`
   - `RetypdGeneratorVisitor` 各类 visitor 调用点统一直接传
     `getExtValuePtr(...)` 结果，不再依赖兼容重载。
   - 典型位置：
     - `visitReturnInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3402-3413`
     - `visitCallBase()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3491-3558`
     - `visitSelectInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3562-3571`
     - `handlePHINodes()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3595-3604`
     - `visitCastInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3608-3625`
     - `visitGetElementPtrInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3672-3680`
     - `visitStoreInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3720-3761`
     - `visitLoadInst()` `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:3764-3782`
   - 同文件里 `PcodeOpType::addOpConstraint()` 也同步改为 canonical 调用：
     `/sn640/NotDec/src/Passes/ConstraintGenerator.cpp:4011-4039`

5. `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2708-3149`
   - `MLsubVisitor` 和 `PcodeOpType` 内部调用点统一直接显式构造 canonical
     `ExtValuePtr`。
   - 典型位置：
     - `visitCastInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2708-2729`
     - `visitCallBase()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2769-2815`
     - `visitReturnInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2818-2826`
     - `handlePHINodes()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2835-2842`
     - `visitLoadInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2851-2871`
     - `visitStoreInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2874-2894`
     - `visitGetElementPtrInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2904-2913`
     - `visitSelectInst()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2970-2978`
     - `visitAnd()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:2995-3019`
     - `visitOr()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3022-3047`
     - `PcodeOpType::addOpConstraint()` `/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:3116-3149`

6. `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:184-195`
   - `TypeBuilder` 只保留 canonical `ExtValuePtr` 主接口：
     - `TypeBuilder::getHighType`
     - `TypeBuilder::getType`
     - `TypeBuilder::getTypeL`
   - `ExprBuilder::getType()` 和 `CFGBuilder::getType()` 保持为边界 helper，
     但会在入口立刻调用 `getExtValuePtr(...)`：
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:311-315`
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:829-831`

7. `/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2087-2097`
   - `ExprBuilder::createCompoundLiteralExpr()` 改为显式把三元组先转换成 canonical
     `ExtValuePtr` 再取类型。
   - `/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2359-2369`
     `ExprBuilder::visitConstant()` 在 high type 查询时使用 use-site aware
     `ExtValuePtr`。
   - `/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2531-2534`
     `ConstantExpr` cast 路径也改成直接传 canonical key。

## Cleanup result

1. 运行以下检查：

```bash
rg -n "ExtValuePtr\\s+\\w+\\s*,\\s*(const\\s+)?(llvm::)?User\\s*\\*\\s*\\w+\\s*,\\s*(unsigned|long)\\s+\\w+" \
  include src external/NotDec-llvm2c -g '!build'
```

2. 结果：
   - 未再发现内部 API 保留 `ExtValuePtr + User + OpInd` 兼容签名。
   - 剩余三参数形态只在边界转换入口和边界 helper 中出现：
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:94`
       `getExtValuePtr(...)`
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:103`
       `llvmValue2ExtVal(...)`
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:313-315`
     - `/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:830-831`

## Validation

1. 构建：

```bash
cmake --build ./build --target notdec-decompile -j4
```

2. 聚焦用例回归：

```bash
mkdir -p /tmp/notdec-extvalueptr-fullcleanup
./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/notdec-extvalueptr-fullcleanup/fortune.out.ll \
  --tr-level=2 \
  --dump-htypes /tmp/notdec-extvalueptr-fullcleanup/fortune.out.htypes
```

3. 结果：
   - `notdec-decompile` 构建成功。
   - `fortune.o3.wasm.4.ll` 运行成功。
   - 成功产出 `fortune.out.ll` 和 `fortune.out.htypes`。

# 2026-04-20 Retypd Unit Test Reorg

## 背景

本次调整目标：

- 删除 `unittests/Retypd/GraphTest.cpp` 中旧 `ConstraintGraph` / `RExp` 相关测试；
- 保留并收拢 `MLsubGenerator` 相关测试；
- 将 `TypeBuilder` 测试从旧的混合文件中拆出，避免 `GraphTest.cpp` 同时承担旧 retypd 与 mlsub 两条线的职责；
- 确认旧 `TypeRecovery` 主链路代码仍保留，不误删 `ConstraintGraph` / `ConstraintGenerator` 生产实现。

## 代码修改

### 1. 单测入口改名与拆分

- `unittests/Retypd/CMakeLists.txt:3-25`
  - 删除原 `GraphTest` target。
  - 新增 `MLsubGeneratorTest` target。
  - 新增 `TypeBuilderTest` target。
  - 更新 `gtest_discover_tests(...)`，分别注册两个 target。

### 2. MLsubGenerator 测试独立

- `unittests/Retypd/MLsubGeneratorTest.cpp:13-35`
  - 新增辅助函数 `makeMLsubGeneratorForFunctionArgs(...)`。
- `unittests/Retypd/MLsubGeneratorTest.cpp:39-61`
  - 保留 `TEST(MLsub, PNDiffUnifiesRecursiveVariablePairsByDefault)`。
  - 在测试末尾显式调用 `notdec::mlsub::ConstraintsGenerator::releaseBinarysubState()`，避免 LSan 因 `binarysub` 图状态未释放而报错。
- `unittests/Retypd/MLsubGeneratorTest.cpp:63-82`
  - 保留 `TEST(MLsub, PNDiffRecursiveVariablePairUnificationCanBeDisabled)`。
  - 同样补充 `releaseBinarysubState()`。

### 3. TypeBuilder 测试独立

- `unittests/Retypd/TypeBuilderTest.cpp:9-52`
  - 从旧文件迁移 `TEST(Retypd, TypeBuilderSemanticPrimitiveAliasTest)`。
- `unittests/Retypd/TypeBuilderTest.cpp:54-80`
  - 从旧文件迁移 `TEST(Retypd, TypeBuilderTopFieldRecordLayoutTest)`。

### 4. 删除旧混合测试文件

- 删除 `unittests/Retypd/GraphTest.cpp`
  - 被删除内容包括：
    - 旧 `ConstraintGraph` 饱和 / 化简测试；
    - `RExp` 相关测试；
    - 原先混在同一文件中的 `MLsub` 与 `TypeBuilder` 测试。

## 验证

执行：

```bash
cmake --build build --target MLsubGeneratorTest -j4
cmake --build build --target TypeBuilderTest -j4
ctest --test-dir build -R '^(MLsub\.|Retypd\.TypeBuilder)' --output-on-failure
```

结果：

- `MLsub.PNDiffUnifiesRecursiveVariablePairsByDefault` 通过
- `MLsub.PNDiffRecursiveVariablePairUnificationCanBeDisabled` 通过
- `Retypd.TypeBuilderSemanticPrimitiveAliasTest` 通过
- `Retypd.TypeBuilderTopFieldRecordLayoutTest` 通过

## 关于旧 ConstraintGraph / ConstraintGenerator

本次没有删除旧 `ConstraintGraph` / `Passes/ConstraintGenerator` 生产代码。

原因：

- 用户已明确旧 `TypeRecovery` 主链路仍需保留；
- 当前库内仍存在多处直接依赖，包括：
  - `src/Passes/ConstraintGenerator.cpp`
  - `src/Passes/GraphTypePass.cpp`
  - `src/TypeRecovery/ConstraintGraph.cpp`
  - `src/TypeRecovery/NFAMinimize.cpp`
  - `src/TypeRecovery/PointerNumberIdentification.cpp`
  - `src/TypeRecovery/SketchToCTypeBuilder.cpp`
  - `include/notdec/TypeRecovery/mlsub/PNDiff.h`

## 后续命名 / 拆分建议

1. `include/notdec/Passes/ConstraintGenerator.h`
   - 该头文件同时承载旧 `TypeRecovery`、旧 `ConstraintsGenerator`、`SCCData`、`AllGraphs` 等多类职责，建议后续拆成：
   - `TypeRecovery/retypd/LegacyTypeRecovery.h`
   - `TypeRecovery/retypd/LegacyConstraintGenerator.h`
   - `TypeRecovery/retypd/LegacySCCModel.h`

2. `src/Passes/ConstraintGenerator.cpp`
   - 体量过大，建议按阶段拆分：
   - `.../LegacyTypeRecoveryOverrides.cpp`
   - `.../LegacyTypeRecoverySCC.cpp`
   - `.../LegacyConstraintGeneratorVisitor.cpp`
   - `.../LegacyConstraintGeneratorPostProcess.cpp`

3. `include/notdec/TypeRecovery/mlsub/PNDiff.h`
   - 当前直接 include 旧 `Passes/ConstraintGenerator.h`，命名与层次都偏重耦合。
   - 建议抽取它真正需要的公共类型（如 `ExtValuePtr` / `getType` / `getSize` 一类接口）到更轻的公共头，再让 `PNDiff` 脱离旧 pass 头文件。

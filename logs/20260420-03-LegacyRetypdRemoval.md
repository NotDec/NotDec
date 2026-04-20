# 2026-04-20 Legacy Retypd Removal

## 背景

本次目标是继续收缩旧 `ConstraintGraph` / `ConstraintGenerator` 链路，确认当前主类型恢复路径只保留 `MLsubGenerator + PNDiff + TypeBuilder`。

结论：

- 旧 `ConstraintGraph` / `ConstraintGenerator` / `GraphTypePass` / `NFAMinimize` / `PointerNumberIdentification` 及其配套 retypd 解析/表达式/Schema 代码，已经不再被当前 `PassManager` 主 pipeline 使用；
- `PNDiff` 对旧链路唯一还存在的实际依赖是头文件层的“借道 include”，并非需要旧恢复逻辑本身；
- 因此本次直接将整批旧链路源码和头文件从当前仓库源码树与 `notdec` 静态库目标中移除。

## 代码修改

### 1. 切断 PNDiff 对旧 ConstraintGenerator 头的依赖

- `include/notdec/TypeRecovery/mlsub/PNDiff.h:14-24`
  - 删除对 `Passes/ConstraintGenerator.h` 和 `notdec-llvm2c/Interface.h` 的 include。
  - 改为显式 include：
    - `Utils/DSUMap.h`
    - `notdec-llvm2c/Interface/Range.h`
    - `notdec-llvm2c/Interface/ExtValuePtr.h`
    - `notdec/TypeRecovery/LowTy.h`
    - `llvm/Support/raw_ostream.h`
  - 涉及类型/函数：
    - `notdec::mlsub::PNINode`
    - `notdec::mlsub::ConsNode`
    - `notdec::mlsub::PNIGraph`

### 2. 去掉 MemOpMatcher 对旧头的无效 include

- `src/Passes/MemOpMatcher.cpp:2-4`
  - 删除 `#include "Passes/ConstraintGenerator.h"`。
  - 说明该 pass 当前并不依赖旧 `ConstraintsGenerator` 定义。

### 3. 调整 notdec 静态库编译单元

- `src/CMakeLists.txt:2-25`
  - 从 `add_library(notdec STATIC ...)` 中移除以下旧源码：
    - `Passes/ConstraintGenerator.cpp`
    - `Passes/GraphTypePass.cpp`
    - `TypeRecovery/Lattice.cpp`
    - `TypeRecovery/DotSummaryParser.cpp`
    - `TypeRecovery/Parser.cpp`
    - `TypeRecovery/ConstraintGraph.cpp`
    - `TypeRecovery/RExp.cpp`
    - `TypeRecovery/PointerNumberIdentification.cpp`
    - `TypeRecovery/NFAMinimize.cpp`
    - `TypeRecovery/SketchToCTypeBuilder.cpp`
    - `TypeRecovery/retypd/Schema.cpp`
  - 保留当前实际路径：
    - `TypeRecovery/LowTy.cpp`
    - `TypeRecovery/mlsub/PNDiff.cpp`
    - `TypeRecovery/mlsub/MLsubGenerator.cpp`
    - `TypeRecovery/mlsub/Schema.cpp`
    - `TypeRecovery/mlsub/TypeBuilder.cpp`

### 4. 修复 PassManager 对旧辅助接口的残留引用

- `include/notdec/Passes/PassManager.h:21-25`
  - 新增 `#include "Utils/Utils.h"`，显式引入 `getSuffix(...)` 声明。
- `src/Passes/PassManager.cpp:421-427`
  - 删除旧的 `getTRDebugDir()` 用法。
  - 改为直接使用 `notdec::getWorkDirOpt()`。
  - 保持 `00-lifted.ll` dump 继续落在当前 workdir 机制下。
  - 涉及函数：
    - `notdec::passes::PassEnv::run_passes`

## 删除的源码 / 头文件

### 源码

- 删除 `src/Passes/ConstraintGenerator.cpp`
- 删除 `src/Passes/GraphTypePass.cpp`
- 删除 `src/TypeRecovery/ConstraintGraph.cpp`
- 删除 `src/TypeRecovery/DotSummaryParser.cpp`
- 删除 `src/TypeRecovery/Lattice.cpp`
- 删除 `src/TypeRecovery/NFAMinimize.cpp`
- 删除 `src/TypeRecovery/Parser.cpp`
- 删除 `src/TypeRecovery/PointerNumberIdentification.cpp`
- 删除 `src/TypeRecovery/RExp.cpp`
- 删除 `src/TypeRecovery/SketchToCTypeBuilder.cpp`
- 删除 `src/TypeRecovery/retypd/Schema.cpp`

### 头文件

- 删除 `include/notdec/Passes/ConstraintGenerator.h`
- 删除 `include/notdec/TypeRecovery/ConstraintGraph.h`
- 删除 `include/notdec/TypeRecovery/DotSummaryParser.h`
- 删除 `include/notdec/TypeRecovery/Lattice.h`
- 删除 `include/notdec/TypeRecovery/NFAMinimize.h`
- 删除 `include/notdec/TypeRecovery/Parser.h`
- 删除 `include/notdec/TypeRecovery/PointerNumberIdentification.h`
- 删除 `include/notdec/TypeRecovery/RExp.h`
- 删除 `include/notdec/TypeRecovery/SketchToCTypeBuilder.h`
- 删除 `include/notdec/TypeRecovery/TRContext.h`
- 删除 `include/notdec/TypeRecovery/retypd/Schema.h`

## 依赖检查

额外检查当前 `include/`、`src/`、`unittests/` 后，以下旧头已无任何剩余 include：

- `Passes/ConstraintGenerator.h`
- `TypeRecovery/ConstraintGraph.h`
- `TypeRecovery/DotSummaryParser.h`
- `TypeRecovery/Lattice.h`
- `TypeRecovery/NFAMinimize.h`
- `TypeRecovery/Parser.h`
- `TypeRecovery/PointerNumberIdentification.h`
- `TypeRecovery/RExp.h`
- `TypeRecovery/SketchToCTypeBuilder.h`
- `TypeRecovery/TRContext.h`
- `TypeRecovery/retypd/Schema.h`

## 验证

执行：

```bash
cmake --build build --target notdec -j4
cmake --build build --target MLsubGeneratorTest -j4
cmake --build build --target TypeBuilderTest -j4
ctest --test-dir build -R '^(MLsub\.|Retypd\.TypeBuilder)' --output-on-failure
```

结果：

- `src/libnotdec.a` 成功重建
- `MLsubGeneratorTest` 成功链接
- `TypeBuilderTest` 成功链接
- `MLsub.PNDiffUnifiesRecursiveVariablePairsByDefault` 通过
- `MLsub.PNDiffRecursiveVariablePairUnificationCanBeDisabled` 通过
- `Retypd.TypeBuilderSemanticPrimitiveAliasTest` 通过
- `Retypd.TypeBuilderTopFieldRecordLayoutTest` 通过

## 备注

本次没有触碰以下已有本地改动：

- `DEBUG.md`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
- `logs/20260420-02-ValueHTypesWorkdirDump.md`

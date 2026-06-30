# HType / TypeBuilder 断 LLVM 依赖实现记录

本次用户要求：

> clang::Decl *这一块要不还是用void*吧，直接开始断LLVM依赖吧

## 背景

后续想把 HType 和 UType -> HType 转换逻辑迁到 `external/binarysub`。这一步先不移动代码，只把 HType 和 TypeBuilder 对 LLVM / clang 的直接依赖降下来，让之后迁移少带 LLVM 头文件。

## 改动

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:54`：`TypedDecl::ASTDecl` 从 `clang::Decl *` 改成 `void *`，`getASTDecl()` / `setASTDecl()` 同步改成 `void *`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:60`、`external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:335`：`TypedDecl::getAs<T>()` 和 `HType::getAs<T>()` 改用本地 `T::classof()`，不再依赖 `llvm::dyn_cast`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:66`、`external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:948`：声明打印接口从 `llvm::raw_ostream` 改成 `std::ostream`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:480`、`external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:494`：`SetUnionType::getTypes()` / `SetInterType::getTypes()` 从 `llvm::ArrayRef` 改成返回 `const std::vector<HType *> &`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:26`、`external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:311`、`external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:631`：HType 实现里把 `StringRef`、`raw_ostream`、`raw_string_ostream`、`dyn_cast/cast` 换成标准库和 `getAs<T>()`，并删除末尾旧 clang 注释草稿。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/StructManager.h:17`、`external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/StructManager.cpp:7`：头文件只前置声明 `llvm::Module`，LLVM `Module` include 下沉到 `.cpp`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ValueNamer.h:9`、`external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ValueNamer.cpp:34`：头文件只前置声明 `llvm::Value`，依赖 `llvm::Value` 的 `getNewName(Value&)` 实现移到 `.cpp`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/TypeManager.h:54`、`external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:43`：C backend 在边界处把 HType 的 `void *` cast 回 `clang::Decl *`，clang 依赖留在 TypeManager 内。
- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:19`：`TypeBuilderContext` 不再保存 `llvm::DataLayout`，只收 `PointerSizeBytes`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:146`、`src/TypeRecovery/mlsub/TypeBuilder.cpp:160`、`src/TypeRecovery/mlsub/TypeBuilder.cpp:1561`：TypeBuilder 的 trace / 文件输出改成标准库，去掉 `llvm::DebugFlag` / `llvm::sys::fs` / `raw_ostream`。调试开关保留 `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:196`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3109`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3307`：`genTypes()` 传 pointer size bytes，不再把 `DataLayout` 传进 TypeBuilder。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2528`、`external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h:40`、`src/TypeRecovery/mlsub/HTypeDebug.cpp:244`：非 EVM 不再创建空 storage root；EVM 仍通过 `PrintEmptyStorageSection` 保留旧的空 `[storage]` 输出。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `rg -n "#include\\s*<llvm|llvm::|clang::|DataLayout|ArrayRef|raw_ostream|StringRef|Support/Casting" external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp include/notdec/TypeRecovery/mlsub/TypeBuilder.h src/TypeRecovery/mlsub/TypeBuilder.cpp`：无命中。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：20/21 通过；`21_PointerAnalysisBranchingFieldCycle` 仍失败，差异是等价递归 record 被拆成 `rec_36` / `rec_60`，临时恢复 eager storage 后仍存在，不是 storage root 延迟造成。
- `ctest --test-dir build -R notdec.type_recovery.sysy.tr_level_2 --output-on-failure`：当前环境 9 个用例都在 NotDec 入口 abort，原因是非 frozen SysY 路径下 `--dump-htypes` 被拒绝，不是本次 HType 输出 diff。
- `ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure`：NotDec 正常退出，DWARF oracle 比较失败在 `@File_list` 全局结构字段缺失；这属于现有结构恢复覆盖问题。
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-htype-depllvm.c --tr-level=2`：通过，`elapsed=174.60 user=202.07 sys=1.68 maxrss=1296864`。

## 评价

- 实现效果：8/10。HType 和 TypeBuilder 的 active 代码已经不直接使用 LLVM / clang，后续搬迁边界更清楚。
- 复杂度：6/10。主要是机械替换，新增复杂度集中在 `void *` 边界 cast 和 storage 空段兼容开关。
- 维护成本：6/10。`ValueNamer` 和 `StructManager` 仍有 LLVM 前置声明，后续真迁移到 binarysub 时最好再拆出纯标准库接口。

更好的后续方案：把 `ValueNamer` 的 LLVM 命名能力拆成 adapter，把 `StructManager::create(llvm::Module&)` 移出 HType 相关接口。这样 binarysub 侧可以完全不暴露 LLVM 类型名。

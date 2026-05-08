# 2026-05-08 LLVM 22 主项目迁移记录

## 背景

`logs/20260508-01-LLVM22Wasm2llvmPort.md` 已经把顶层 CMake 和
`external/NotDec-wasm2llvm` 迁到可编译状态。

`logs/20260508-02-LLVM22LLVM2CFirstErrors.md` 当前只负责
`external/NotDec-llvm2c`。

这份日志单独记录主项目本体，也就是 `src/`、`include/notdec/`、顶层构建开关相关的
LLVM 22 兼容修改，不把 `llvm2c` 的修复混进来。

## 目标

1. 在不启用 `llvm2c` 的前提下，单独编译主项目本体。
2. 找出 LLVM 22 下主项目自己的编译错误。
3. 先修低风险、机械替换级别的问题。

## 当前计划

1. 单开构建目录，配置 `-DNOTDEC_ENABLE_LLVM2C=OFF`。
2. 编译主项目 target，收集第一批错误。
3. 只修主项目本体中最简单的问题。
4. 回填验证结果，并记录修改位置、涉及函数和当前判断。

## 已完成：第一轮主项目简单兼容修复

这轮只处理主项目本体里“机械替换”和“缺失头文件/构建边界”这类问题，不碰
`external/NotDec-llvm2c` 源码，也不继续深挖 `DSROA` 和 opaque pointer 的设计问题。

### 构建入口调整

1. `src/CMakeLists.txt:31-36`
   - 给 `notdec` target 显式加 `external/NotDec-llvm2c/include`。
   - 原因：主项目 `src/` 里有不少类型恢复和 pass 代码直接复用了 `llvm2c` 的头文件类型，
     但这次为了先隔离 `llvm2c` 库本身，单独关掉了 `NOTDEC_ENABLE_LLVM2C`。
   - 涉及 target：`notdec`。

### 已修的主项目源码问题

1. `include/notdec/Passes/PassManager.h:12-16,46-50`
   - `llvm/Passes/PassPlugin.h` 改成 `llvm/Plugins/PassPlugin.h`。
   - `llvm::None` 改成 `std::nullopt`。
   - `StandardInstrumentations` 改成 LLVM 22 的构造签名，并把
     `registerCallbacks()` 的第二个参数改成 `&MAM`。
   - 涉及函数：`PassEnv::PassEnv`。

2. `src/Passes/PassManager.cpp:81-82,327-329`
   - `haveNoCommonBitsSet()` 改成走 `llvm::SimplifyQuery` 新接口。
   - `Module::getGlobalList()` 改成 `Module::globals()`。
   - `StringRef::equals()` 改成 `==`。
   - 涉及函数：`UndoInstCombine::run`、`DecompileConfig::find_special_gv`。

3. `include/notdec/Utils/Utils.h:11-12,125-131`
   - 补 `<set>`，清掉以前靠传递包含混过去的 `std::set` 报错。
   - 涉及模板：`emplace_atom`，也影响 `getFuncSetName` 声明。

4. `include/notdec/Passes/StackPointerFinder.h:38`
   - `StackPointerMatcher::match` 补 `const`，适配 LLVM 22 的 pattern matcher 调用。
   - 涉及函数：`StackPointerMatcher::match`。

5. `src/Passes/StackPointerFinder.cpp:100`
   - `mod.getGlobalList()` 改成 `mod.globals()`。
   - 涉及函数：`StackPointerFinderAnalysis::run`。

6. `src/Passes/MemOpMatcher.cpp:7,40,82-85,101-102`
   - `llvm/ADT/Optional.h` / `Optional` / `None` 改成 `std::optional` /
     `std::nullopt`。
   - `Type::getInt8PtrTy()` 改成 `PointerType::get(Context, 0)`。
   - 涉及函数：`matchOffset`、`addOffset`。

7. `include/notdec/TypeRecovery/mlsub/MLsubGraph.h:5-7`
   `include/notdec/TypeRecovery/mlsub/Schema.h:5-8`
   `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:9,44-46,226-228,474-481`
   `src/TypeRecovery/mlsub/MLsubGraph.cpp:1-7`
   - 主项目里残留的 `Optional.h` 头依赖改成 `<optional>`。
   - `MLsubGenerator.h` 不再把 `llvm2c` 头放在 `NOTDEC_ENABLE_LLVM2C` 宏后面，
     因为这个头里的 `Result` / `ExtValuePtr` 本来就是主项目类型恢复接口的一部分。
   - `is_cast` 参数补成 `llvm::Value *`，避免名字查找跑偏。
   - `SCCsCatalog` 改成 `std::optional`。
   - 涉及类型/函数：`MLsubRecovery::Result`、`MLsubRecovery::getResult`、
     `ConstraintsGenerator::is_cast`。

8. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:538-545,1225-1230,3643-3644,3797`
   - `llvm::json::Object` 改成逐项赋值。
   - `target_triple` 比较改成字符串对字符串。
   - `startswith()` 改成 `starts_with()`。
   - 涉及函数：`writeMLsubInputAnchor`、
     `validateExtraConstraintsAnchor`、
     `ConstraintsGenerator::MLsubVisitor::visitCallBase`、
     `ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst`。

9. `src/TypeRecovery/LowTy.cpp:175-177,184,193-194`
   - `startswith()` 改成 `llvm::StringRef(...).starts_with()`。
   - `Type::getInt8PtrTy()` 改成 `PointerType::get(Ctx, 0)`。
   - 涉及函数：`isInt1`、`ToLLVMType`。

10. `src/Passes/AllocAnnotator.cpp:2-5`
    - 补 `Constants.h`、`DataLayout.h`、`Module.h`。
    - 涉及函数：`AllocAnnotator::run`。

11. `src/Passes/retdec-stack/retdec-ir-modifier.cpp:9-10,235`
    - 补 `Constants.h`。
    - `getNextNonDebugInstruction()` 改成 `getNextNode()`。
    - 涉及函数：`convertToType`。

## 验证

配置：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -DLT_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj \
  -DNOTDEC_ENABLE_LLVM2C=OFF \
  -S . -B ./build-main-no-llvm2c -G Ninja
```

主验证命令：

```bash
cmake --build ./build-main-no-llvm2c --target notdec -j4
ninja -C build-main-no-llvm2c -k 100 notdec \
  > /tmp/notdec-main-no-llvm2c-build.log 2>&1
```

结果：

- 现在已经能把主项目失败面从一开始的很多零散错误，收敛到 5 个失败对象文件：
  - `src/CMakeFiles/notdec.dir/Passes/retdec-stack/retdec-ir-modifier.cpp.o`
  - `src/CMakeFiles/notdec.dir/Passes/retdec-stack/retdec-stack.cpp.o`
  - `src/CMakeFiles/notdec.dir/Passes/retdec-stack/retdec-symbolic-tree.cpp.o`
  - `src/CMakeFiles/notdec.dir/Passes/DSROA.cpp.o`
  - `src/CMakeFiles/notdec.dir/TypeRecovery/mlsub/MLsubGenerator.cpp.o`
- 这说明主项目第一轮“简单接口迁移”和“缺头文件/构建边界”问题基本清完了。

## 当前剩余问题

### 1. `DSROA.cpp` 是一整个大块 LLVM pass 迁移

主要报错：

- `IRBuilderDefaultInserter::InsertHelper` 签名变化
- `TypeSize::getFixedSize()` -> `getFixed() / getFixedValue()`
- `gep_type_iterator`、`makeArrayRef` 等旧 helper 变化
- 一整批 opaque pointer 旧接口：
  `getNonOpaquePointerElementType()`、`isOpaquePointerTy()`、
  `IRBuilder::getInt8PtrTy()`
- 还有少量 `ConstantExpr::*`、`FindDbgAddrUses`、lifetime intrinsic 接口变化

判断：

- 这不是“顺手改几个名字”能收掉的，最好单独一轮处理。

### 2. retdec-stack 还剩两类错误

1. `retdec-ir-modifier.cpp`
   - `ConstantExpr::getIntegerCast` / `getFPCast` / `getExtractValue`
     这些旧静态工厂接口已经变了。

2. `retdec-stack.cpp`、`retdec-symbolic-tree.cpp`
   - 还在直接用 `getPointerElementType()`，属于 opaque pointer 迁移。

### 3. `MLsubGenerator.cpp` 剩下的是 opaque pointer 问题

当前已经只剩：

- `convertSimpleTypeVal()` 里对 `PointerType::getPointerElementType()` 的判断
- `getPointerElemSize()` 里直接从 pointer type 取 element type

这个要回到“显式从哪拿 pointee type”来改，已经不是纯机械替换。

## 性能

这轮还只是恢复主项目在 LLVM 22 下的编译面，没形成可运行的完整 `notdec`，
所以没有 fortune 运行时间，也没有新增性能数据。

## 当前判断

方案评分：7.5/10。

优点：

- 已经把主项目和 `llvm2c` 库构建问题拆开。
- 主项目简单迁移点基本清掉，剩余问题边界更清楚。

问题：

- 剩下的 5 个失败对象里，`DSROA.cpp` 占了大头。
- `MLsubGenerator` 和 retdec-stack 的核心剩余问题都落在 opaque pointer。

下一步建议：

1. 先单独啃 `DSROA.cpp`，因为它是最大的单点阻塞。
2. 再处理 `MLsubGenerator.cpp` 和 retdec-stack 里 `getPointerElementType()` 的
   显式类型来源。

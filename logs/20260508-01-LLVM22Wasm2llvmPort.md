# 2026-05-08 LLVM 22 CMake 与 wasm2llvm 迁移记录

## 背景

项目准备从 LLVM 14 迁到本地构建的 LLVM 22.1.0。当前已经有
`llvm-22.1.0.obj/`，并保留旧目录 `llvm-source-old/`、`llvm-build-old/`。

目标是先让 CMake 明确使用 LLVM 22.1.0，并把 `external/NotDec-wasm2llvm`
迁到可编译、可启动、可做基础转换验证的状态。

## 已完成

1. `scripts/build-debug-llvm.sh:11-18`
   - 默认版本改为 `22.1.0`。
   - 新增 `LLVM_VERSION`、`LLVM_TAG`、`LLVM_SOURCE_HOME`、`LLVM_BUILD_HOME`、
     `LLVM_INSTALL_HOME`、`LLVM_ENABLE_PROJECTS`、`LLVM_ENABLE_RUNTIMES`、
     `BUILD_JOBS` 环境变量。

2. `scripts/build-debug-llvm.sh:34-49`
   - 默认 clone `llvmorg-22.1.0`。
   - 如果 `llvm-source` 已存在但不是目标 tag，直接报错，避免误用旧源码。

3. `scripts/build-debug-llvm.sh:51-75`
   - 默认安装到 `llvm-22.1.0.obj`。
   - 使用 `cmake -S/-B`，不依赖当前目录。

4. `CMakeLists.txt:17-18`
   - 顶层默认 LLVM 目录改为 `llvm-22.1.0.obj`。
   - 新增 `NOTDEC_LLVM_MAJOR_VERSION=22`。

5. `CMakeLists.txt:29-74`
   - 固定从 `LT_LLVM_INSTALL_DIR` 下的 `LLVMConfig.cmake` 找 LLVM。
   - `find_package(LLVM)` 后用 `LLVM_VERSION_MAJOR` 检查主版本。
   - 这样避开 `find_package(LLVM 22)` 不接受 `22.1.0` 的问题。

6. `CMakeLists.txt:89-99`
   - 清理旧 cache 中的 `NOTDEC_LLVM_DYLIB` 和 `NOTDEC_CLANG_CPP_LIBRARY`。
   - 共享库候选名从 `LLVM-14` 改为 `LLVM-${NOTDEC_LLVM_MAJOR_VERSION}`。

7. `external/NotDec-wasm2llvm/cmake/FindLLVM.cmake:8-65`
   - 同步 LLVM 22 查找逻辑。

8. `external/NotDec-llvm2c/cmake/FindLLVM.cmake:8-65`
   - 同步 LLVM 22 查找逻辑。

## 验证

配置命令：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -DLT_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj \
  -S . -B ./build -G Ninja
```

结果：配置通过，顶层、wasm2llvm、llvm2c 都找到 LLVM 22.1.0。

构建命令：

```bash
cmake --build ./build --target all -j4
```

结果：构建失败，第一批错误集中在 `external/NotDec-wasm2llvm`：

- `parser-block.cpp:166`：`BasicBlock::getFirstNonPHIOrDbgOrLifetime()` 现在返回 iterator，不能和 `nullptr` 比较。
- `parser.cpp:142`：`Module::setTargetTriple()` 需要 `llvm::Triple`，不能直接传字符串。
- `parser.cpp:240`、`241`、`249`、`252`、`257`：`StringRef::equals` 已不可用，需要改成 `==` 或其他现有接口。
- `parser-instruction.cpp:168`、`176`、`1896`：opaque pointer 下不能用 `getPointerElementType()`。
- `parser-instruction.cpp:249`、`976`、`1894`：`makeArrayRef` 不再按旧写法可用。
- `parser-instruction.cpp:536` 起多处：`Intrinsic::getDeclaration` 改为 `Intrinsic::getOrInsertDeclaration`。

完整日志在 `/tmp/notdec-llvm22-build.log`。

## 性能影响

这次只改构建脚本和 CMake 引用，项目尚未完成 LLVM 22 编译，不能运行 fortune 性能对比。

## 当前判断

方案评分：7/10。

优点：改动集中，CMake 已能稳定选中本地 LLVM 22，不再误链接 LLVM 14。

风险：子模块里还有较多 LLVM 22 API 迁移工作，尤其是 opaque pointer 相关代码，不能只机械替换。

下一步：先修 `external/NotDec-wasm2llvm` 的编译错误。建议从简单 API 替换开始，
再单独处理 `getPointerElementType()` 这类需要补显式类型来源的问题。

## wasm2llvm 修复记录

本次只处理 `external/NotDec-wasm2llvm`。

修改：

1. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-block.cpp:163`
   - `getFirstNonPHIOrDbgOrLifetime` 直接返回 LLVM 22 的 iterator。
   - 涉及函数：`getFirstNonPHIOrDbgOrLifetime`。

2. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser.cpp:22`
   - 引入 `llvm/TargetParser/Triple.h`。

3. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser.cpp:143`
   - `setTargetTriple` 改为传 `llvm::Triple("wasm32-unknown-wasi")`。
   - 涉及函数：`Context::visitModule`。

4. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser.cpp:241`
   - `StringRef::equals` 改为 `==`。
   - 涉及函数：`Context::visitModule`。

5. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:163`
   - local load 从 `AllocaInst::getAllocatedType()` 取显式 load type。
   - 涉及函数：`BlockContext::visitLocalGet`。

6. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:172`
   - global load 从 `GlobalVariable::getValueType()` 取显式 load type。
   - 涉及函数：`BlockContext::visitGlobalGet`。

7. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:246`
   - `makeArrayRef(arr, 2)` 改为 `ArrayRef<Value *>(arr, 2)`。
   - 涉及函数：`BlockContext::convertStackAddr`。

8. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:534`
   - `Intrinsic::getDeclaration` 批量改为 `Intrinsic::getOrInsertDeclaration`。
   - 涉及函数：`BlockContext::visitUnaryInst`、`BlockContext::visitBinaryInst`。

9. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:974`
   - SIMD shuffle mask 改为显式 `ArrayRef<int>`。
   - 涉及函数：`BlockContext::createSIMDExtend`。

10. `external/NotDec-wasm2llvm/lib/notdec-wasm2llvm/parser-instruction.cpp:1891`
    - indirect call table GEP 改为显式 `ArrayRef<Value *>`。
    - table load 从 table 数组元素类型取显式 load type。
    - 涉及函数：`BlockContext::visitCallIndirectInst`。

验证：

```bash
cmake --build ./build --target notdec-wasm2llvm -j4
```

结果：通过。

继续执行全量构建：

```bash
cmake --build ./build --target all -j4
```

结果：失败点已经前进到 `external/NotDec-llvm2c`。这里先只记录它是下一个阻塞点，
不继续把 `llvm2c` 的迁移细节写进本日志。当前看到的第一批错误包括：

- `llvm/ADT/Optional.h` 已不存在。
- `StringRef::equals`、`startswith` 等接口变化。
- Clang AST API 变化，例如 `BinaryOperator::getFPFeatures`、`CompoundStmt::Create`。
- llvm2c 里也还有 opaque pointer 相关的 `getPointerElementType()`。

性能：仍未完成全项目编译，不能跑 fortune 对比。

当前评分：7/10。wasm2llvm 的编译入口已经清掉，但只验证了编译，没有跑 wasm lifting 行为回归。

## wasm2llvm 工具动态链接修复

背景：

`build/external/NotDec-wasm2llvm/bin/notdec-wasm2llvm` 虽然能编出来，但启动即报：

```text
CommandLine Error: Option 'bitcode-mdindex-threshold' registered more than once!
LLVM ERROR: inconsistency in registered CommandLine options
```

判断：

- 工具在 shared LLVM 模式下已经通过 `notdec-wasm2llvm` 间接带入 `libLLVM.so`。
- 但 `tools/notdec-wasm2llvm/CMakeLists.txt` 还额外直接链接了 `LLVMBitWriter`。
- 实际 link line 同时出现 `libLLVM.so` 和 `libLLVMBitWriter.a`，容易把 LLVM 的
  `cl::opt` 注册代码带两次。

修改：

1. `external/NotDec-wasm2llvm/tools/notdec-wasm2llvm/CMakeLists.txt:5-17`
   - 先只链接 `notdec-wasm2llvm`。
   - 若 `NOTDEC_HAVE_SHARED_LLVM` 为真，再补 `notdec_llvm_deps`。
   - 仅在非 shared LLVM 模式下才直接链接 `LLVMBitWriter`。

验证：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -DLT_LLVM_INSTALL_DIR=/sn640/NotDec/llvm-22.1.0.obj \
  -S . -B ./build -G Ninja
cmake --build ./build --target notdec-wasm2llvm-exe -j4
build/external/NotDec-wasm2llvm/bin/notdec-wasm2llvm --help
build/external/NotDec-wasm2llvm/bin/notdec-wasm2llvm build/wabt/bin/hello.wasm -o hello.ll
build/wabt/bin/wasm2wat build/wabt/bin/hello.wasm -o /tmp/notdec-wasm2llvm-hello.wat
build/external/NotDec-wasm2llvm/bin/notdec-wasm2llvm /tmp/notdec-wasm2llvm-hello.wat -o /tmp/notdec-wasm2llvm-hello-from-wat.ll
```

结果：

- 工具可以正常启动，`--help` 可用。
- `.wasm` 输入可转成 LLVM IR。
- `.wat` 输入也可转成 LLVM IR。
- 动态依赖中只保留 `libLLVM.so.22.1`，不再额外拉入 `libLLVMBitWriter.a` 对应的重复注册问题。

剩余状态：

- `external/NotDec-wasm2llvm` 里未提交改动只剩
  `tools/notdec-wasm2llvm/CMakeLists.txt`。
- 上一个子模块提交仍是 `a0827f0 Port wasm2llvm build to LLVM 22`。
- `llvm2c` 后续迁移改记到新日志，不再继续追加到本文件。

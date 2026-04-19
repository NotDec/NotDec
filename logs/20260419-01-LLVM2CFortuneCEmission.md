# llvm2c `fortune.o3.wasm.ll` 转 C 崩溃修复

日期：2026-04-19

## 1. 问题

当前用 `.vscode/launch.json` 里的 `dec` 配置，针对
`test/type-recovery/realworld/cases/fortune.o3.wasm.ll` 生成 `.c` 时，
程序会在 `llvm2c` 阶段断言退出：

```text
clang::QualType::getCommonPtr():
Assertion `!isNull() && "Cannot retrieve a NULL type pointer"' failed.
```

`gdb` 回溯定位到：

- `notdec::llvm2c::SAContext::createDecls`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1944`

## 2. 根因

触发崩溃的具体全局变量是：

- `@__stack_pointer = internal global i32 68912, !notdec.stackpointer !0`

在 `SAContext::createDecls()` 里，原逻辑会对普通全局变量直接执行：

```cpp
PTy = TB.getType(&GV)->getPointeeType();
```

但当前 `llvm2c`/高层类型链路里，像 `__stack_pointer` 这类全局有可能已经被
lower 成“对象类型”而不是“存储指针类型”。这时 `TB.getType(&GV)` 返回的是
`int`，继续取 `getPointeeType()` 就会得到空 `QualType`，随后
`VarDecl::Create()` 再访问这个空类型便触发断言。

## 3. 修改

### 3.1 修复全局变量声明类型选择

修改位置：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1931](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1931)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1956](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp#L1956)

涉及函数：

- `notdec::llvm2c::SAContext::createDecls`

改动内容：

1. 先用 IR 存储类型 `TB.visitType(*GV.getType())` 建立基线 `StorageTy`
2. 默认把声明类型取为 storage pointee
3. 如果存在恢复类型 `TB.getType(&GV)`：
   - 若它与当前声明类型兼容，则直接使用恢复类型
   - 若它的 pointee 与当前声明类型兼容，则使用恢复类型的 pointee
   - 否则保留恢复类型本身
4. 不再对 `PTy` 做第二次无条件 `getPointeeType()`

效果：

1. `__stack_pointer` 这类“恢复后就是对象类型”的全局不再被错误剥空
2. 指针型全局仍能在“对象类型”和“存储指针类型”之间做兼容选择

### 3.2 把可回退路径的 `Error` 诊断改成 `Warning`

修改位置：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1221](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L1221)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1233](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L1233)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1285](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L1285)
- [external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:1315](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp#L1315)

涉及函数：

- `notdec::llvm2c::ClangTypeResult::tryHandlePtrAdd`

改动内容：

1. `struct/union` 非常量偏移
2. `getFieldAt(offset)` 失败
3. 数组下标构造失败
4. 不支持的指针算术类型

这些分支原本打印 `Error:` 后返回 `nullptr`，随后由 `handlePtrAdd()` 自动回退到
`char *` 字节级指针算术，因此并不是“整次反编译失败”。

本次把这几类诊断统一改成：

- `Warning: ...; falling back to byte-wise pointer arithmetic.`

这样当前 case 虽然仍会暴露恢复精度不足，但不会再在成功出 C 的同时打印一串
误导性的“Error”。

## 4. 验证

### 4.1 构建

```bash
cmake --build ./build --target notdec-decompile -j2
```

### 4.2 复现命令

```bash
NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
NOTDEC_BINARYSUB_TRACE=1 \
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.o3.wasm.c \
  --tr-level=2 \
  -g \
  --work-dir=/sn640/NotDec/work_dir
```

### 4.3 结果

当前结果已从“断言崩溃”变为：

```text
Decompile result: /tmp/fortune.o3.wasm.c
```

并且生成出的 C 可以通过语法检查：

```bash
clang-14 -fsyntax-only -Wno-everything /tmp/fortune.o3.wasm.c
```

## 5. 剩余问题

当前 case 仍然存在但未阻断落盘的问题包括：

1. `Structure analysis stopped making progress, quitting. Func: main`
2. `Warning: Cannot find type for load inst: ...`
3. 多处 `failed to get field at offset -12 / -1`，当前会回退到字节级指针算术
4. 这些 warning 分支仍然会附带 `Decl->dump()` / `Type->dump()` 输出，日志较吵

后续如果继续清 `llvm2c`：

1. 优先看 `TypeManager.cpp` 里的 field-offset 路径为什么会拿到负 offset
2. 再看 `StructuralAnalysis.cpp` 的 load type 选择为何经常走 fallback
3. 最后再回头收 `Phoenix` 在 `main` 上的结构化停滞

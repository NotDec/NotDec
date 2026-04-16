# MLsub Default Builtin Signature JSON For Fortune

日期：2026-04-16

## 本次目标

把上一版直接写在 `MLsubGenerator.cpp` 里的 builtin libc / POSIX / regex 签名，改成仓库内默认加载的 JSON 资源文件。

目标语义：

1. 默认情况下自动加载这份 builtin signature JSON
2. 显式 `NOTDEC_SIGNATURE_OVERRIDE` 仍然保留，并覆盖同名 builtin 签名
3. builtin 资源文件校验采用宽松模式
   - 函数不存在：跳过
   - 参数个数不匹配：警告并跳过
   - vararg：警告并跳过
4. 显式 `NOTDEC_SIGNATURE_OVERRIDE` 继续保持严格校验

## 代码修改

### 1. 新增默认 builtin signature JSON 资源文件

文件：

- [resources/mlsub_builtin_signatures.json](/sn640/NotDec/resources/mlsub_builtin_signatures.json)

这份 JSON 直接采用上一批已经实现的 MLsub signature schema：

- 顶层 `version=1`
- 顶层 `functions`
- 支持：
  - `primitive`
  - `var`
  - `ref`
  - `ptr`
  - `record`
  - `null`
  - `constraints`

当前资源文件覆盖的函数族包括：

- 字符串 / 文本：
  - `strlen`
  - `strcmp`
  - `strncmp`
  - `strdup`
  - `strchr`
  - `strrchr`
  - `strncpy`
  - `atoi`
  - `atol`
  - `getenv`
  - `setlocale`
  - `getopt`
  - `printf`
  - `fprintf`
  - `snprintf`
  - `perror`
  - `__assert_fail`
- 内存：
  - `malloc`
  - `realloc`
  - `free`
- stdio：
  - `fopen`
  - `fdopen`
  - `fclose`
  - `fflush`
  - `fgets`
  - `fputs`
  - `fread`
  - `putc`
- 文件 / 目录：
  - `open`
  - `read`
  - `close`
  - `access`
  - `stat`
  - `opendir`
  - `readdir`
  - `closedir`
- regex / libc 其它：
  - `regcomp`
  - `regexec`
  - `regfree`
  - `qsort`
  - `toupper`
  - `tolower`

### 2. CMake 默认复制资源并把路径编进 notdec

文件：

- [src/CMakeLists.txt:38](/sn640/NotDec/src/CMakeLists.txt#L38)
  到
  [src/CMakeLists.txt:58](/sn640/NotDec/src/CMakeLists.txt#L58)

这里新增了：

1. `resources/mlsub_builtin_signatures.json`
   复制到
   `build/share/notdec/mlsub_builtin_signatures.json`
2. 通过 `target_compile_definitions()` 把默认路径编译为：
   - `NOTDEC_DEFAULT_MLSUB_SIGNATURE_OVERRIDE_PATH`

因此当前开发构建里，`build/bin/notdec` 无需额外环境变量，也会自动加载这份默认资源。

### 3. MLsubRecovery 支持“默认资源 + 显式 override”合并加载

文件：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:375](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L375)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:35](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L35)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:420](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L420)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:430](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L430)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:487](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L487)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:556](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L556)

涉及函数：

- `MLsubRecovery::run()`
- `MLsubRecovery::loadSignatureFile()`

主要变化：

1. `run()` 每次执行前先初始化一个空的合并文档：
   - `{"version": 1, "functions": {}}`
2. 若默认 JSON 资源路径存在：
   - 先加载 builtin 资源
   - 使用 `StrictValidation=false`
3. 若环境变量 `NOTDEC_SIGNATURE_OVERRIDE` 存在：
   - 再加载显式 override
   - 使用 `StrictValidation=true`
4. `loadSignatureFile()` 不再直接替换整份文档，而是把命中的函数条目 merge 到当前 `functions` map 中

因此同名函数的最终优先级为：

1. 显式 `NOTDEC_SIGNATURE_OVERRIDE`
2. 默认 builtin signature JSON

### 4. 删除上一版 C++ 代码内硬编码的 builtin signature 构造逻辑

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:120](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L120)

上一版的这些 helper / builder 已经移除，不再继续维护 C++ 内嵌 builtin schema：

- `primitiveExpr()`
- `varExpr()`
- `refExpr()`
- `ptrExpr()`
- `recordExpr()`
- `subtypeConstraintExpr()`
- `functionSpecExpr()`
- `buildBuiltinSignatureOverrideSpec()`

现在 builtin 只保留为数据文件。

### 5. bottom-up 应用点恢复成统一 override 应用逻辑

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:813](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L813)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:820](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L820)

涉及函数：

- `MLsubRecovery::bottomUpPhase()`

这里已经不再区分“显式 override”和“builtin override”的不同调用路径，而是统一走：

1. `getSignatureOverrideSpec()`
2. `applySignatureOverride()`

builtin 是否生效，完全取决于默认 JSON 是否已经在 `run()` 里 merge 进当前文档。

## 验证

### 1. JSON 语法检查

运行：

```bash
python3 -m json.tool resources/mlsub_builtin_signatures.json >/tmp/mlsub_builtin_signatures.pretty.json
```

结果：通过。

### 2. 重新配置与构建

运行：

```bash
cmake -S . -B ./build -G Ninja
cmake --build ./build --target notdec-decompile -j2
```

结果：通过。

### 3. `fortune.o3.wasm.1.ll` baseline

运行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/fortune-baseline.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune-baseline.htypes \
  --gen-work-dir \
  --work-dir=/tmp/fortune-baseline-work
```

baseline 中外部函数签名几乎都是纯 `32-bit` 变量，例如：

- `@strlen => bottom:32 (*)('q3:32)*`
- `@strdup => bottom:32 (*)('v5:32)*`
- `@fprintf => bottom:32 (*)('l3:32, 'm3:32, 'n3:32)*`
- `@fgets => bottom:32 (*)('q5:32, 'r5:32, 's5:32)*`
- `@regexec => bottom:32 (*)('p7:32, 'q7:32, 'r7:32, 's7:32, 't7:32)*`

并且全文件中：

- `ptr<load=i8, store=i8>` 出现次数：`0`
- `ptr<load=void*, store=void*, psize=32>` 出现次数：`0`

### 4. 默认 JSON 生效后

运行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/fortune-builtin.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune-builtin.htypes \
  --gen-work-dir \
  --work-dir=/tmp/fortune-builtin-work
```

注意：这里没有设置 `NOTDEC_SIGNATURE_OVERRIDE`。

`/tmp/fortune-builtin.htypes` 中对应签名已经明显收敛，例如：

- `@strlen => i32 (*)(('n4:32 & ptr<load=i8, store=i8, psize=32>))*`
- `@strncpy => 'f5:32 (*)(('f5:32 & ptr<load=i8, store=i8, psize=32>), ('g5:32 & ptr<load=i8, store=i8, psize=32>), ('h5:32 & i32))*`
- `@fprintf => i32 (*)(('i4:32 & ptr<load=void*, store=void*, psize=32>), ('j4:32 & ptr<load=i8, store=i8, psize=32>), 'k4:32)*`
- `@fgets => 'n6:32 (*)(('n6:32 & ptr<load=i8, store=i8, psize=32>), ('o6:32 & i32), ('p6:32 & ptr<load=void*, store=void*, psize=32>))*`
- `@regexec => i32 (*)(('m8:32 & ptr<load=void*, store=void*, psize=32>), ('n8:32 & ptr<load=i8, store=i8, psize=32>), ('o8:32 & i32), ('p8:32 & ptr<load=void*, store=void*, psize=32>), ('q8:32 & i32))*`

并且全文件中：

- `ptr<load=i8, store=i8>` 出现次数：`54`
- `ptr<load=void*, store=void*, psize=32>` 出现次数：`16`

## 当前结论

这次已经把 builtin signature 从“代码硬编码”切到了“默认加载的 JSON 资源”。

对 `fortune.o3.wasm.1.ll` 来说，效果和上一版内嵌实现保持一致，但现在的维护方式更合适：

1. builtin 签名可以直接在 JSON 里增删改
2. `MLsubGenerator.cpp` 不再承载一大段 hardcoded builtin schema 构造器
3. 显式 `NOTDEC_SIGNATURE_OVERRIDE` 仍然可继续覆盖默认 builtin
4. 默认 builtin 对别的模块采用宽松匹配，不会因为同名库函数参数个数不同而直接把整个运行打断

## 还没做的

1. `FILE*` / `DIR*` / `regex_t*` 当前仍统一走 `ptr<load=void*, store=void*>` 风格的 opaque handle
2. 默认 builtin JSON 目前还是手工维护，没有再做 schema validator 或注释生成工具
3. 如果后续 builtin 集合继续扩大，可以考虑再把资源路径和安装规则补齐到 install / package 层

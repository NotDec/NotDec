# MLsub Summary / Signature Split Implementation

日期：2026-04-16

## 本次完成内容

这次把 MLsub 里的 override 正式拆成了两条语义不同的路径：

1. `NOTDEC_SUMMARY_OVERRIDE`
   - lower-bound
   - 约束方向为 `OverrideTy <: F`
2. `NOTDEC_SIGNATURE_OVERRIDE`
   - upper-bound
   - 约束方向为 `F <: OverrideTy`
   - 第一版限制为只能作用在有函数体的函数上

## 代码修改

### 1. `MLsubRecovery` 状态拆分

文件：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340)
- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:378](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L378)
- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:392](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L392)

涉及字段 / 函数：

- `MLsubRecovery::SummaryFile`
- `MLsubRecovery::SignatureFile`
- `MLsubRecovery::SummaryOverrideDoc`
- `MLsubRecovery::SummaryOverrideFuncs`
- `MLsubRecovery::loadSummaryFile()`
- `MLsubRecovery::getSummaryOverrideSpec()`
- `MLsubRecovery::applySummaryOverride()`
- `MLsubRecovery::applyUpperBoundSignatureOverride()`

这里把原来只有一套 `SignatureOverride*` 状态的实现，改成 summary/signature 两套并行状态。

### 2. 默认 builtin 资源改为 summary 语义

文件：

- [src/CMakeLists.txt:38](/sn640/NotDec/src/CMakeLists.txt#L38)
- [src/CMakeLists.txt:55](/sn640/NotDec/src/CMakeLists.txt#L55)
- [resources/mlsub_builtin_summaries.json](/sn640/NotDec/resources/mlsub_builtin_summaries.json)

涉及构建项：

- `NOTDEC_MLSUB_BUILTIN_SUMMARIES_SOURCE`
- `NOTDEC_MLSUB_BUILTIN_SUMMARIES_BUILD`
- `NOTDEC_DEFAULT_MLSUB_SUMMARY_OVERRIDE_PATH`

这里把默认 builtin JSON 从 `mlsub_builtin_signatures.json` 改名成 `mlsub_builtin_summaries.json`，并把编译期宏也同步改成 summary 路径。

### 3. loader / 应用点按方向拆开

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:123](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L123)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:135](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L135)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:533](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L533)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:603](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L603)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:765](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L765)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:827](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L827)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:924](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L924)

涉及函数：

- `loadOverrideFileImpl()`
- `MLsubRecovery::run()`
- `MLsubRecovery::loadSummaryFile()`
- `MLsubRecovery::loadSignatureFile()`
- `MLsubRecovery::applySummaryOverride()`
- `MLsubRecovery::applyUpperBoundSignatureOverride()`
- `MLsubRecovery::bottomUpPhase()`

具体行为：

1. `run()` 启动时分别初始化 summary/signature 两份 override 文档
2. 默认 builtin JSON 先走 summary loader
3. 显式 `NOTDEC_SUMMARY_OVERRIDE` 再 merge 到 summary 文档
4. 显式 `NOTDEC_SIGNATURE_OVERRIDE` 独立 merge 到 signature 文档
5. `bottomUpPhase()` 里先应用 summary lower-bound，再应用 signature upper-bound
6. signature loader 在 strict 模式下会拒绝 declaration-only 函数

### 4. 调试说明同步

文件：

- [AGENTS.md:174](/sn640/NotDec/AGENTS.md#L174)
- [logs/20260416-02-MLsubBuiltinLibcSignatureFortune.md:1](/sn640/NotDec/logs/20260416-02-MLsubBuiltinLibcSignatureFortune.md#L1)

这里把仓库内当前有效的语义说明同步成：

- builtin 默认资源属于 summary
- `NOTDEC_SUMMARY_OVERRIDE` / `NOTDEC_SIGNATURE_OVERRIDE` 的方向明确区分

## 验证

### 1. 资源与构建

运行：

```bash
python3 -m json.tool resources/mlsub_builtin_summaries.json >/tmp/mlsub_builtin_summaries.pretty.json
cmake -S . -B ./build -G Ninja
cmake --build ./build --target notdec-decompile -j2
```

结果：通过。

### 2. builtin summary 自动生效

运行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/fortune-summary.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune-summary.htypes
```

结果：

1. stderr 中出现多条 `Applying MLsub summary override to ...`
2. `/tmp/fortune-summary.htypes` 中可见：
   - `@strlen => i32 (*)(('l4:32 & ptr<load=i8, store=i8, psize=32>))*`
   - `@printf => i32 (*)(('a4:32 & ptr<load=i8, store=i8, psize=32>), 'b4:32)*`
   - `@fprintf => i32 (*)(('g4:32 & ptr<load=void*, store=void*, psize=32>), ('h4:32 & ptr<load=i8, store=i8, psize=32>), 'i4:32)*`

说明默认 builtin 资源已经按 summary 方向继续生效。

### 3. signature override 仅允许有函数体的函数

运行：

```bash
env NOTDEC_SIGNATURE_OVERRIDE=/tmp/mlsub_signature_main_2arg.json \
  ./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/fortune-signature-should-fail.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune-signature-should-fail.htypes
```

结果：

- stderr 直接报错：
  - `Error: invalid MLsub override at functions.main: signature override requires a function definition`

说明 declaration-only 函数已经不会再误走 upper-bound signature 路径。

### 4. 有函数体的 signature override 入口已接通

运行：

```bash
env NOTDEC_SIGNATURE_OVERRIDE=/tmp/mlsub_signature_main_2arg.json \
  ./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01_Simple1-signature.ll \
  --tr-level=2 \
  --dump-htypes /tmp/01_Simple1-signature.htypes
```

结果：

- stderr 中出现：
  - `Applying MLsub signature override to main`

说明 upper-bound signature override 已经沿定义体函数路径接入到 bottom-up 应用点。

# MLsub Override `pndiff` 支持

日期：2026-04-16

## 本次完成内容

这次在 MLsub 的 summary / signature override JSON 中新增了独立的 `pndiff`
段，用来直接给函数边界值设置 PNDiff 状态。

当前支持：

- `target.kind = "ret"`
- `target.kind = "arg"` 且使用 `index`
- `state = "ptr"`
- `state = "number"`，兼容 `num`

## 代码修改

### 1. `MLsubGenerator.cpp` 新增 `pndiff` schema 校验与 target 解析

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:116](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L116)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:145](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L145)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:157](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L157)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:192](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L192)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:246](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L246)

涉及函数 / 结构：

- `OverridePNDiffState`
- `ResolvedPNDiffTarget`
- `parsePNDiffState()`
- `resolvePNDiffTarget()`
- `applyPNDiffOverrides()`
- `loadOverrideFileImpl()`

具体行为：

- loader 现在会校验 `functions.<name>.pndiff` 必须是数组
- 每个 entry 必须带 `target` 和 `state`
- 同一 target 重复设置相同状态允许，冲突状态直接报错
- `ret` 只允许用于非 `void` 函数
- 非 pointer / 非 pointer-sized-int 的 target 会直接报错，不再静默忽略

### 2. summary / signature 应用点都接入 `pndiff`

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:895](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L895)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:958](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L958)

涉及函数：

- `MLsubRecovery::applySummaryOverride()`
- `MLsubRecovery::applyUpperBoundSignatureOverride()`

具体行为：

- `constraints` 和函数类型约束照常注入
- 之后会对当前函数的 `ret/arg` 边界值调用 `applyPNDiffOverrides()`
- 因此 `NOTDEC_SUMMARY_OVERRIDE` 和 `NOTDEC_SIGNATURE_OVERRIDE` 现在共用同一套
  `pndiff` schema

## builtin 资源更新

文件：

- [resources/mlsub_builtin_summaries.json:4](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L4)
- [resources/mlsub_builtin_summaries.json:103](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L103)
- [resources/mlsub_builtin_summaries.json:196](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L196)
- [resources/mlsub_builtin_summaries.json:318](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L318)
- [resources/mlsub_builtin_summaries.json:417](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L417)

涉及函数：

- `__assert_fail`
- `access`
- `atoi`
- `atol`
- `close`
- `closedir`
- `fclose`
- `fdopen`
- `fflush`
- `fgets`
- `fopen`
- `fprintf`
- `fputs`
- `fread`
- `free`
- `getenv`
- `getopt`
- `malloc`
- `open`
- `opendir`
- `perror`
- `printf`
- `putc`
- `qsort`
- `read`
- `readdir`
- `realloc`
- `regcomp`
- `regexec`
- `regfree`
- `setlocale`
- `snprintf`
- `stat`
- `strchr`
- `strcmp`
- `strdup`
- `strlen`
- `strncmp`
- `strncpy`
- `strrchr`
- `tolower`
- `toupper`

具体行为：

- 为这些 builtin 增加了 `pndiff` 段
- 对文件描述符、长度、flags、返回码、字符参数等边界值标成 `number`
- 对缓冲区、字符串、目录流、`FILE *`、`regex_t *` 等边界值标成 `ptr`
- 对 `fgets` / `realloc` / `strchr` / `strncpy` / `strrchr` 这类带
  `var/ref` 关联的接口，也显式补上了 `ret/arg` 边界值的 PNDiff 标注

## 验证

### 1. JSON 与构建

运行：

```bash
python3 -m json.tool resources/mlsub_builtin_summaries.json >/tmp/mlsub_builtin_summaries.pretty.json
cmake --build ./build --target notdec-decompile -j2
```

结果：通过。

### 2. summary / signature 两条路径的 `pndiff` 都生效

输入：

- `test/type-recovery/llvm-ir/cases/01_Simple1.ll`

临时 override：

- `/tmp/mlsub_pndiff_summary.json`
- `/tmp/mlsub_pndiff_signature.json`

运行后检查：

- `/tmp/mlsub_pndiff_summary_work/03-pndiff-final.ll`
- `/tmp/mlsub_pndiff_signature_work/03-pndiff-final.ll`

观察到函数注释一致为：

```llvm
; pndiff: ret=num, arg0=num, arg1=ptr
```

说明：

- `NOTDEC_SUMMARY_OVERRIDE` 的 `pndiff` 已生效
- `NOTDEC_SIGNATURE_OVERRIDE` 的 `pndiff` 也已生效

备注：

- 运行末尾仍会出现现有环境里的 LeakSanitizer 提示
- 但 annotated IR 已成功产出，不影响本次 `pndiff` 功能验证

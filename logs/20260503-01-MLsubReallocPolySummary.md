# 2026-05-03 `realloc` builtin polymorphic 标记补齐

## 背景

`resources/mlsub_builtin_summaries.json` 里，`malloc`、`free`、`memcpy`
已经显式标了 `is_polymorphic: true`，但 `realloc` 虽然也是按 `var/ref`
建模的 `void *` 风格接口，却漏了这个标记。

这会让 `realloc` 不能按“不同调用点可取不同具体类型”的口径参与 polymorphic
builtin 处理。

## 目标

做最小修补：

1. 给 `realloc` 补上 `is_polymorphic: true`
2. 顺手检查当前 builtin summary 里有没有同类漏标

## 实现记录

修改文件：`resources/mlsub_builtin_summaries.json`

- 第 389 行附近，函数条目 `realloc`
  - 新增 `is_polymorphic: true`

核查结论：

- 当前和 `realloc` 同一类、已经按多态 builtin 处理的条目只有：
  - `malloc`
  - `free`
  - `memcpy`
- `fgets` / `strchr` / `strncpy` / `strrchr` 虽然也用了 `var/ref`，但语义是固定
  `char *`，不是这次要补的 `void *` 多态接口
- `read` / `fread` / `qsort` 语义上带 `void *`，但当前 summary 还是固定
  `uint8*` / opaque 指针建模，不属于这次“同类漏标”的最小修补范围

## 验证

已运行：

```bash
python3 -m json.tool resources/mlsub_builtin_summaries.json >/tmp/mlsub_builtin_summaries.pretty.json
cmake --build ./build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-realloc-poly/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-realloc-poly
```

结果：

- JSON 校验通过
- 运行时加载的是 `build/share/notdec/mlsub_builtin_summaries.json`
- 日志中已出现 `Applying MLsub summary override to realloc`
- `/tmp/notdec-fortune-realloc-poly/SCCs.txt` 结果包含：
  - `SCC1,realloc (level = 1)`
  - `SCC2,free (level = 1)`
  - `SCC3,malloc (level = 1)`
- fortune 当前口径运行时间：
  - 参考：`real 16.33s`（2026-04-29）
  - 本次：`real 15.01s`、`user 14.67s`、`sys 0.34s`
  - 结论：没有看到性能回退

## 简评

- 实现效果：8/10
- 复杂度成本：9/10
- 维护成本：9/10

这次改动很小，和现有 polymorphic builtin 规则一致。更进一步如果要处理
`read` / `fread` / `qsort`，应该先把它们的 summary 建模收紧，再决定是否标
`is_polymorphic`，不适合在这次顺手混改。

# 2026-04-20 ImportantHTypes and wasm DWARF text dump

## 背景

这次把两边给人工对比用的类型视图统一成相近布局：

1. `work_dir/` 新增 `ImportantHTypes.txt`
2. `test/tools/extract_wasm_dwarf_truth.py` 默认输出非 JSON 文本
3. suite 继续保留 JSON truth 给 `compare_htypes_with_wasm_dwarf.py` 用

目标是先在文本里直接对齐：

- memory / 全局变量起始偏移
- 函数参数与返回值
- 再去后面的 `decls` 查结构体布局

## 修改

### 1. 类型恢复 workdir 新增 `ImportantHTypes.txt`

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L53)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1421)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2726)

涉及函数：

- `writeDebugValueHTypes()`
- `primeSnapshotFormatter()`
- `unwrapFunctionType()`
- `formatJoinedHTypes()`
- `formatImportantOffset()`
- `writeDebugImportantHTypes()`
- `MLsubRecovery::genASTTypes()`

具体改动：

1. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:56`
   - 新增 `kImportantHTypesFile = "ImportantHTypes.txt"` 常量。

2. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1434-1575`
   - 新增 `writeDebugImportantHTypes()` 相关 helper。
   - `[memory]` section 里只打印 memory 总类型和 memory decl，不再单独重复 `addr(0x...)` roots。
   - `[functions]` section 只收集函数 value，按函数名字母序输出，并拆成 `ret` / `argN`。
   - `[decls]` section 继续复用 `HTypeSnapshotFormatter` 的稳定 `struct_*` 命名。

3. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2726-2728`
   - 在 `MLsubRecovery::genASTTypes()` 导出 `ValueHTypes.txt` 的同时，再导出 `ImportantHTypes.txt`。

### 2. DWARF truth 默认改成文本，同时保留 JSON

文件：

- [test/tools/extract_wasm_dwarf_truth.py](/sn640/NotDec/test/tools/extract_wasm_dwarf_truth.py#L133)
- [test/tools/extract_wasm_dwarf_truth.py](/sn640/NotDec/test/tools/extract_wasm_dwarf_truth.py#L161)
- [test/tools/extract_wasm_dwarf_truth.py](/sn640/NotDec/test/tools/extract_wasm_dwarf_truth.py#L334)
- [test/tools/extract_wasm_dwarf_truth.py](/sn640/NotDec/test/tools/extract_wasm_dwarf_truth.py#L499)
- [test/tools/extract_wasm_dwarf_truth.py](/sn640/NotDec/test/tools/extract_wasm_dwarf_truth.py#L562)

涉及函数：

- `parse_location()`
- `sanitize_identifier()`
- `TruthBuilder.resolve_type()`
- `TruthBuilder.extract_records()`
- `TruthBuilder.extract_globals()`
- `TruthBuilder.extract_functions()`
- `TruthBuilder.extract_bindings()`
- `TruthBuilder.type_size_bits()`
- `TruthBuilder.render_type_text()`
- `render_decl()`
- `write_text_output()`
- `main()`

具体改动：

1. `test/tools/extract_wasm_dwarf_truth.py:133-149`
   - `parse_location()` 现在只要表达式里带 `DW_OP_addr`，就顺手算出 `base_address`。
   - 这样 `DW_OP_addr ... DW_OP_deref_size ...` 这类全局变量也能按起始偏移排序。

2. `test/tools/extract_wasm_dwarf_truth.py:161-496`
   - `TruthBuilder` 新增稳定 decl 名、函数类型、全局列表、函数列表、文本类型渲染。
   - 支持把 `DW_TAG_subroutine_type` 渲染成文本函数类型。
   - 结构体字段与全局列表都做了稳定排序。

3. `test/tools/extract_wasm_dwarf_truth.py:499-559`
   - 新增 `render_decl()` / `write_text_output()`。
   - 文本输出拆成 `[memory]` / `[functions]` / `[decls]` 三段。
   - `[memory]` 里直接给出全局变量起始偏移，从低到高排序。
   - `[functions]` 按函数名字母序输出 `ret` / `argN`。
   - `[decls]` 里补 synthetic `dwarf_memory` 和所有 record 定义。

4. `test/tools/extract_wasm_dwarf_truth.py:562-587`
   - CLI 新增 `--json-output`。
   - `--output` 现在写文本 truth；如果传 `--json-output`，再额外写 JSON truth。

### 3. suite runner 改成同时产出 `.truth.txt` 和 `.truth.json`

文件：

- [test/run_type_recovery_suite.py](/sn640/NotDec/test/run_type_recovery_suite.py#L180)

涉及函数：

- `prepare_truth()`
- `main()`

具体改动：

1. `test/run_type_recovery_suite.py:190-206`
   - `prepare_truth()` 改成调用：
     - `--output <case>.truth.txt`
     - `--json-output <case>.truth.json`
   - 通过条件也改成同时检查两份文件都存在。

2. `test/run_type_recovery_suite.py:323-339`
   - suite cleanup 里加入 `<case>.truth.txt`。

### 4. 文档同步

文件：

- [DEBUG.md](/sn640/NotDec/DEBUG.md#L154)
- [test/type-recovery/realworld/README.md](/sn640/NotDec/test/type-recovery/realworld/README.md#L19)

具体改动：

1. `DEBUG.md:154-164`
   - 新增 `ImportantHTypes.txt` 说明，明确它是给人工对比用的精简视图。

2. `test/type-recovery/realworld/README.md:19-23`
   - 补充说明 suite 现在会同时生成 truth JSON 和文本 truth。

## 验证

执行：

```bash
python3 -m py_compile \
  test/tools/extract_wasm_dwarf_truth.py \
  test/run_type_recovery_suite.py \
  test/tools/compare_htypes_with_wasm_dwarf.py

cmake --build ./build --target all -j4

python3 test/tools/extract_wasm_dwarf_truth.py \
  --input test/lifting/wasm/cases/fortune.o3.wasm \
  --output /tmp/notdec-truth-check/fortune.truth.txt \
  --json-output /tmp/notdec-truth-check/fortune.truth.json

./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-important-check4/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-important-check4

python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest ./test/type-recovery/realworld/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-realworld-important-check2

ctest --test-dir build \
  -R notdec.type_recovery.realworld.tr_level_2 \
  --output-on-failure
```

结果：

1. `fortune.truth.txt` 已生成，包含：
   - `[memory]` 全局偏移升序
   - `[functions]` 函数字母序
   - `[decls]` 结构体定义
2. `ImportantHTypes.txt` 已生成，包含同样的 memory/functions/decls 三段；memory 绝对偏移直接通过 decl field 注释查看
3. `python3 test/run_type_recovery_suite.py ...` 通过
4. `ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure` 通过

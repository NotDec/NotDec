# Fortune 函数参数结构体指针回归补充

日期：2026-04-19

## 1. 背景

继续检查 `test/type-recovery/realworld/cases/fortune.o3.wasm.ll` 的类型恢复结果时，
发现若干 `struct fd *` 形参虽然字段具体类型仍有不少 type var，但函数签名层面已经
稳定恢复成 `struct_*` 指针，并且这些结构切片已经覆盖了若干可信的字段偏移。

这类结果适合先纳入“layout-only”回归，而不是继续强行做字段类型精确比较。

## 2. 本次修改

### 2.1 扩展 debug-truth comparator，支持函数参数结构切片检查

修改文件：

- `test/tools/compare_htypes_with_debug_truth.py`

涉及位置与函数：

- 第 146-168 行：新增 `parse_function_pointer_type()`，从 `.htypes` 的 `@func => ret (*)(...) *`
  条目里拆出参数表达式
- 第 190-205 行：新增 `find_record_pointer_targets()`，从参数表达式中提取
  `struct_*` 指针目标
- 第 342-415 行：`Comparator.compare_root()` 新增 `recovered = "function_param"`
  分支
- 第 417-469 行：新增 `Comparator.compare_function_param()`，按函数名和参数序号定位
  恢复后的形参类型
- 第 560-597 行：新增 `Comparator.compare_record_decl_layout()`，对恢复出的
  `struct_*` 切片做字段偏移存在性检查

当前这条新比较路径只支持：

- truth 侧是“指向 record 的指针”
- recovered 侧是函数参数里已经出现 `struct_*` 指针
- `compare_mode = "layout"`

也就是只检查“恢复成结构体指针且切片覆盖指定字段偏移”，不检查字段具体类型。

### 2.2 把 fortune 中稳定的 `fd*` 形参加入 realworld manifest

修改文件：

- `test/type-recovery/realworld/manifest.json`

涉及位置：

- 第 66-128 行：新增 5 个 `function_param` roots

纳入检查的函数参数：

- `free_desc::arg0`
- `get_tbl::arg0`
- `matches_in_list::arg0`
- `maxlen_in_list::arg0`
- `print_list::arg0`

这些 root 当前分别命中恢复后的：

- `struct_9`
- `struct_10`
- `struct_15`
- `struct_16`
- `struct_17`

都来自 `.htypes` 中对应的 `@free_desc`、`@get_tbl`、`@matches_in_list`、
`@maxlen_in_list`、`@print_list` 函数类型条目。

### 2.3 文档同步

修改文件：

- `test/type-recovery/realworld/README.md`
  - 第 22-30 行：说明 realworld suite 现在同时包含 memory-root 精确比较和
    function-parameter layout-only 比较
- `test/README.md`
  - 第 74-77 行：补充 `debug-struct-compare` 现在除了 `[memory]` root 之外，
    还支持 recovered function parameter layout slice

## 3. 验证

执行：

```bash
python3 test/run_type_recovery_suite.py \
  --binary /sn640/NotDec/build/bin/notdec \
  --manifest /sn640/NotDec/test/type-recovery/realworld/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-realworld-suite-after
```

结果：

- `fortune.o3.wasm` 通过
- compare report 中新增的 5 个 function-param roots 全部通过

对应 report 位于：

- `/tmp/notdec-realworld-suite-after/fortune.o3.wasm.compare.txt`
- `/tmp/notdec-realworld-suite-after/fortune.o3.wasm.compare.json`

## 4. 当前边界

本次没有把 `add_file::arg3/arg4` 这类 `fd **` 参数一起纳入，因为它们在 `.htypes`
里表现为 `ptr<load=... struct_* ...>` 这种更深一层的指针包装，当前 comparator
还没有做 pointer-depth 对齐比较。

如果后续这类双重指针也稳定了，可以沿本次 `function_param` 路径继续扩成：

- pointer-to-record
- pointer-to-pointer-to-record

两档检查。

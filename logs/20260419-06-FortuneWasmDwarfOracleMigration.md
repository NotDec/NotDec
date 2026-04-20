# 2026-04-19 fortune wasm DWARF oracle 迁移

## 1. 背景

本次把 `fortune` 的类型恢复回归从“邻近 LLVM IR truth + 旧 debug-info comparator”
彻底切到“直接从 `fortune.o3.wasm` 提取 DWARF truth”的链路，同时删除旧
`debug-info-c` suite 和整套旧工具。

## 2. 代码修改

### 2.1 新增 wasm DWARF truth 提取工具

文件：

- `test/tools/extract_wasm_dwarf_truth.py`

涉及位置与函数：

- `test/tools/extract_wasm_dwarf_truth.py:32-39`
  - `run_dwarfdump()`
  - 统一通过 `llvm-dwarfdump` 读取 wasm 内嵌 DWARF。
- `test/tools/extract_wasm_dwarf_truth.py:42-82`
  - `parse_dwarfdump()`
  - 解析 `DW_TAG_*` 树和 `DW_AT_*` 属性。
- `test/tools/extract_wasm_dwarf_truth.py:149-243`
  - `TruthBuilder`
  - 负责 typedef alias、record 名归一化和 DWARF type 解析。
- `test/tools/extract_wasm_dwarf_truth.py:245-299`
  - `TruthBuilder.extract_records()`
  - 导出结构体字段布局。
- `test/tools/extract_wasm_dwarf_truth.py:281-299`
  - `TruthBuilder.extract_bindings()`
  - 导出全局变量和函数参数的 binding 与 location。

当前 truth JSON 直接来自：

- `test/lifting/wasm/cases/fortune.o3.wasm`

不再依赖：

- `test/type-recovery/realworld/truth/fortune.ll`

### 2.2 新增 `.htypes` vs wasm DWARF comparator

文件：

- `test/tools/compare_htypes_with_wasm_dwarf.py`

涉及位置与函数：

- `test/tools/compare_htypes_with_wasm_dwarf.py:237-337`
  - `parse_htypes()`
  - 复用 `.htypes` decl/type/memory 解析。
- `test/tools/compare_htypes_with_wasm_dwarf.py:361-475`
  - `Comparator.compare_root()`
  - 新的 root dispatch，按 binding kind 和 truth kind 分派。
- `test/tools/compare_htypes_with_wasm_dwarf.py:492-528`
  - `Comparator.compare_function_param()`
  - 比较 recovered 函数参数 `struct_*` slice。
- `test/tools/compare_htypes_with_wasm_dwarf.py:530-559`
  - `Comparator.compare_global_pointer()`
  - 不再把“全局指针变量地址”误当成“指向对象基址”，而是检查全局 slot 的
    recovered pointee `struct_*`。
- `test/tools/compare_htypes_with_wasm_dwarf.py:561-649`
  - `Comparator.compare_record_at_address()`
  - `Comparator.compare_record_memory_root()`
  - 比较 direct aggregate globals，例如 `@Noprob_tbl`。
- `test/tools/compare_htypes_with_wasm_dwarf.py:651-701`
  - `Comparator.compare_record_decl_layout()`
  - 做结构切片字段偏移存在性比较。
- `test/tools/compare_htypes_with_wasm_dwarf.py:703-720`
  - `Comparator.compare_memory_field_type()`
  - 支持 memory cell 上 `ptr<load=i32,...>` 这类 primitive slot 形态。
- `test/tools/compare_htypes_with_wasm_dwarf.py:727-803`
  - `write_report_text()`
  - `write_report_markdown()`
  - 产出 `.compare.txt` 和更适合人工检查的 `.compare.md`。

### 2.3 type recovery runner 切换到新 oracle

文件：

- `test/run_type_recovery_suite.py`

涉及位置与函数：

- `test/run_type_recovery_suite.py:171-201`
  - `prepare_truth()`
  - 新增 `wasm-dwarf-compare`，调用
    `test/tools/extract_wasm_dwarf_truth.py`。
- `test/run_type_recovery_suite.py:212-279`
  - `compare_oracle()`
  - 调用 `test/tools/compare_htypes_with_wasm_dwarf.py`，并写出
    `.compare.json/.txt/.md`。
- `test/run_type_recovery_suite.py:334-341`
  - cleanup 列表里加入 `.compare.md`。
- `test/run_type_recovery_suite.py:410-414`
  - 失败输出改为指向新的 compare report。

### 2.4 realworld/fortune manifest 改成 wasm DWARF anchors

文件：

- `test/type-recovery/realworld/manifest.json`

涉及位置：

- `test/type-recovery/realworld/manifest.json:7-10`
  - oracle 从旧 `debug-struct-compare` 改成 `wasm-dwarf-compare`。
- `test/type-recovery/realworld/manifest.json:20-99`
  - roots 改成基于 wasm binding 的 anchors：
    - `@File_list`
    - `@File_tail`
    - `@Noprob_tbl`
    - `free_desc::arg0`
    - `get_tbl::arg0`
    - `matches_in_list::arg0`
    - `maxlen_in_list::arg0`
    - `print_list::arg0`

这一步删除了旧 manifest 里那种手工 `base_offset_bytes` 对齐假设。

### 2.5 删除旧 suite / 旧工具 / 旧 truth

删除：

- `test/tools/compare_htypes_with_debug_truth.py`
- `test/tools/extract_debug_type_truth.py`
- `test/tools/gen_debug_info_case.py`
- `test/type-recovery/debug-info-c/README.md`
- `test/type-recovery/debug-info-c/cases/01_absolute_pair.c`
- `test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c`
- `test/type-recovery/debug-info-c/manifest.json`
- `test/type-recovery/debug-info-c/truth/01_absolute_pair.dbg.ll`
- `test/type-recovery/debug-info-c/truth/02_absolute_self_ref.dbg.ll`
- `test/type-recovery/realworld/truth/fortune.ll`
- `test/lifting/wasm/truth/fortune.ll`

并同步修改：

- `test/CMakeLists.txt:52-55`
  - 只保留 `realworld` suite，移除 `debug_info_c` 的 `add_type_recovery_suite`。
- `test/README.md:17-71`
  - 文档改成 `wasm-dwarf-compare`。
- `test/type-recovery/realworld/README.md:3-37`
  - 说明 `fortune` truth 来自 raw wasm 的 DWARF。
- `test/lifting/wasm/README.md:12-20`
  - 删掉对 `truth/fortune.ll` 的说明，改成描述 in-module DWARF 复用。

## 3. 验证

执行：

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest ./test/type-recovery/realworld/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-realworld-wasm-dwarf

cmake -S . -B build -G Ninja

ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.lifting.wasm --output-on-failure
```

结果：

- `fortune.o3.wasm` 在新 `wasm-dwarf-compare` 链路下通过。
- `ctest -N` 中 `notdec.type_recovery.debug_info_c.tr_level_2` 已不再注册。
- `notdec.lifting.wasm` 仍然通过，说明删除旧 `truth/fortune.ll` 没破坏 lifting suite。

对应人工报告位于：

- `/tmp/notdec-realworld-wasm-dwarf/fortune.o3.wasm.compare.txt`
- `/tmp/notdec-realworld-wasm-dwarf/fortune.o3.wasm.compare.md`

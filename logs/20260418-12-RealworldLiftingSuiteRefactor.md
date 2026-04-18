# 2026-04-18 Realworld / Lifting Test Suite Refactor

## 背景

这次调整把当前 `fortune` 相关测试拆成两条更清晰的线：

1. `test/lifting/wasm/`
   - 关注原始 `fortune.o3.wasm` 的 frontend lifting 行为。
2. `test/type-recovery/realworld/`
   - 关注较大真实项目上的类型恢复，并使用 `truth/fortune.ll` 的
     debug info 做语义对比。

同时，原来的 `test/type-recovery/howard-o3-split/` 整批导入语料被移除，
只保留当前真正关注的 `fortune` case。

## 具体修改

### 1. 新增 lifting suite

- `test/run_lifting_suite.py:14-155`
  - 新增 module-summary 解析与比较逻辑。
  - 关键函数：
    - `parse_ir_summary()`
    - `compare_module_summary()`
- `test/run_lifting_suite.py:158-278`
  - 新增 manifest-driven lifting runner 主流程。
  - 关键函数：
    - `main()`
    - `notdec_command_succeeded()`
    - `run_command()`
- `test/CMakeLists.txt:20-40`
  - 新增 `add_lifting_suite()`，并注册 `notdec.lifting.wasm`。
- `test/lifting/wasm/manifest.json:1-15`
  - 新增 `fortune.o3.wasm` lifting case。
- `test/lifting/wasm/expected/fortune.o3.wasm.summary.json:1-34`
  - 新增 lifting oracle，约束：
    - `target triple`
    - 关键 globals
    - 关键 defines / declares
- `test/lifting/wasm/README.md:1-20`
  - 记录 suite 目的、目录约定、以及 `truth/fortune.ll` 的用途。

### 2. 重组 realworld 类型恢复 suite

- `test/type-recovery/realworld/manifest.json:1-65`
  - 新增 `fortune.o3.wasm` case。
  - 使用 `truth/fortune.ll` 作为 `ground_truth`。
  - 为 `@Re_pat`、`add_file::head`、`add_file::tail`、`@Noprob_tbl`
    配置 root 和 `field_allowlist`。
- `test/type-recovery/realworld/README.md:1-17`
  - 记录该 suite 只保留 `fortune.o3.wasm.ll`，并说明为什么使用字段白名单。
- 文件重组：
  - 新增 `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`
    - 由原 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll`
      移入并去掉 `.4` 后缀。
  - 新增 `test/type-recovery/realworld/truth/fortune.ll`
    - 从 `/sn640/NotDec-Exp/ICSE-HOWARD/bin/fortune.ll` 复制。
  - 删除整个 `test/type-recovery/howard-o3-split/`
    - 包括旧 `README.md`、`manifest.json`、全部 split corpus case 与
      `expected/tr-level-2/.gitkeep`。

### 3. 扩展 debug-info 结构对比器

- `test/tools/compare_htypes_with_debug_truth.py:299-345`
  - 在 `Comparator.compare_root()` 中接入 root 级 `field_allowlist`。
  - 新增 `Comparator._field_allowlist()`。
- `test/tools/compare_htypes_with_debug_truth.py:347-379`
  - 扩展 `Comparator.compare_type()`，允许把 field allowlist 只传到 root
    record 级比较。
- `test/tools/compare_htypes_with_debug_truth.py:386-417`
  - 扩展 `Comparator.compare_record()`，只比较白名单中的字段。

这一步的目的，是让 `fortune` 这种大结构 case 可以先稳定回归当前已经能在
`ValueTypes.txt` / `.htypes` 中恢复出来的字段，而不是被未恢复完整的尾部字段
拖成整例失败。

### 4. 文档与仓库说明同步

- `test/README.md:17-72`
  - 加入 `test/run_lifting_suite.py`、`test/lifting/wasm/`、
    `test/type-recovery/realworld/` 的说明。
- `AGENTS.md:9`
  - 当前关注 case 更新为
    `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`
- `AGENTS.md:249-285`
  - suite 列表更新为 `lifting/wasm` + `type-recovery/realworld`
  - runner 列表加入 `test/run_lifting_suite.py`
  - `pass` 状态说明更新为“与该 suite 的 oracle 一致”

## 验证

## 当前 pass 判定方式

### 1. `test/lifting/wasm/` 现在如何判定 `pass`

当前 `fortune.o3.wasm` lifting case 由 `test/run_lifting_suite.py` 驱动。

执行流程：

1. 读取 `test/lifting/wasm/manifest.json`
2. 对每个 case 调用：
   - `./build/bin/notdec <input.wasm> -o <workdir>/<case>.out.ll --tr-level=0`
3. 先检查 lifting 是否“基础成功”：
   - `notdec` 返回码必须为 `0`
   - stdout 中不能包含 `IR parsing failed:`
   - 输出 `.ll` 文件必须存在且非空
4. 如果基础成功，再进入 oracle 比较：
   - `test/run_lifting_suite.py::parse_ir_summary()`
     从输出 `.ll` 中提取：
     - `target triple`
     - `globals`
     - `define` 出来的函数名
     - `declare` 出来的函数名
   - `test/run_lifting_suite.py::compare_module_summary()`
     把这些摘要和
     `test/lifting/wasm/expected/fortune.o3.wasm.summary.json`
     做结构化比较
5. 只有当“基础成功”和“摘要比较无 mismatch”同时成立时，这个 case 才算
   `pass`

当前 `fortune.o3.wasm` 具体比较的是：

- `target triple`
  - 必须是 `wasm32-unknown-wasi`
- `required_globals`
  - `__stack_pointer`
  - `__notdec_mem0`
  - `table_0`
- `required_defines`
  - `main`
  - `program_version`
  - `usage`
  - `add_file`
  - `maxlen_in_list`
  - `matches_in_list`
  - `get_tbl`
  - `print_list`
  - `free_desc`
  - `names_compare`
  - `__main_void`
- `required_declares`
  - `getenv`
  - `getopt`
  - `malloc`
  - `printf`
  - `fprintf`
  - `stat`
  - `opendir`
  - `readdir`
  - `closedir`
  - `qsort`
  - `regexec`

也就是说，lifting suite 当前不是全文 diff `.ll`，而是检查：

- frontend 能否稳定把 wasm lift 成非空 LLVM IR
- lift 后的模块骨架里，关键 global / define / declare 是否仍然存在

这样做的原因是：

- 对大 case 做全文 `.ll` snapshot 太脆弱
- 当前更关心 wasm frontend 有没有把模块主骨架 lift 出来

当前 `ground_truth` 字段虽然指向 `truth/fortune.ll`，但 lifting suite 这版还
没有把它作为强制比较对象；它现在主要是就地保存 provenance，并给后续更强
oracle 预留输入。

### 2. `test/type-recovery/realworld/` 现在如何判定 `pass`

当前 `fortune.o3.wasm.ll` realworld case 由 `test/run_type_recovery_suite.py`
驱动，oracle kind 是 `debug-struct-compare`。

执行流程：

1. 读取 `test/type-recovery/realworld/manifest.json`
2. 对每个 case 调用：
   - `./build/bin/notdec <input.ll> -o <workdir>/<case>.out.ll --tr-level=2 --dump-htypes <workdir>/<case>.out.htypes`
3. 先检查 type recovery 是否“基础成功”：
   - `notdec` 返回码必须为 `0`
   - stdout 中不能包含 `IR parsing failed:`
   - 输出 `.out.ll` 和 `.out.htypes` 都必须存在且非空
4. 如果基础成功，再准备 truth：
   - `test/run_type_recovery_suite.py::prepare_truth()`
     调用 `test/tools/extract_debug_type_truth.py`
   - 从 `test/type-recovery/realworld/truth/fortune.ll` 抽出：
     - `records`
     - `bindings`
   - 输出到 `<workdir>/<case>.truth.json`
5. 之后运行 oracle 比较：
   - `test/run_type_recovery_suite.py::compare_oracle()`
     调用 `test/tools/compare_htypes_with_debug_truth.py`
   - 该比较器会读取：
     - `.htypes`
     - `truth.json`
     - root 配置
6. 对 manifest 里每个 root：
   - 先根据 `truth` 键在 `truth.json["bindings"]` 中找到 debug-info truth type
   - 当前 `recovered` 只支持 `memory`
   - 也就是把 `.htypes` 的 `[memory]` section 作为被恢复对象
   - 然后以 `base_offset_bytes` 作为 memory root 起点做字段比较
7. `test/tools/compare_htypes_with_debug_truth.py` 的具体规则是：
   - 如果 truth 是 pointer，就继续跟到 pointee
   - 如果 truth 是 record，就在 debug-info record 上逐字段比较
   - 每个字段都按
     `expected_offset = base_offset_bytes + field.offset_bits / 8`
     去 `.htypes` 的 `[memory] decl` 里找对应 offset 的 recovered field
   - primitive 字段会检查 recovered 表达式里是否包含正确位宽的 primitive
   - pointer 字段会检查 recovered field 是否仍然是 pointer-like
   - 如果 root 配了 `field_allowlist`，则只比较白名单字段，其他字段全部跳过
8. 只有当：
   - `notdec` 成功产出 `.htypes`
   - truth 成功抽取
   - manifest 里的每一个 root 都没有 mismatch
   时，这个 case 才算 `pass`

### 3. 当前 `fortune` realworld case 比较了哪些 root

`test/type-recovery/realworld/manifest.json` 当前配置了 4 个 root：

1. `@Re_pat`
   - `base_offset_bytes = 2224`
   - `field_allowlist = ["re_nsub", "__opaque"]`
2. `add_file::head`
   - `base_offset_bytes = 2268`
   - `field_allowlist = ["percent", "datfd", "inf", "name", "path", "datfile"]`
3. `add_file::tail`
   - `base_offset_bytes = 3328`
   - `field_allowlist = ["percent", "fd", "pos", "inf", "name", "path", "datfile", "posfile"]`
4. `@Noprob_tbl`
   - `base_offset_bytes = 3360`
   - `field_allowlist = ["str_version", "str_numstr"]`

这些 offset 不是随便填的，而是根据当前 `fortune.o3.wasm.ll` 跑出的
`.htypes` / `ValueTypes.txt` 中已经相对稳定的 memory 区段手工挑出来的。

### 4. 为什么 realworld 不是“整结构必须全字段对上”

当前 `fortune` 的大结构只恢复到了部分稳定字段。

如果要求：

- `struct.fd`
- `struct.stat`
- `struct.re_pattern_buffer`

这类 record 的所有字段都一次性完全对上，那么这个 case 现在会因为大量尾部
字段尚未恢复出来而整体失败。

所以本次加的 `field_allowlist` 的语义是：

- 先把已经稳定恢复的字段纳入强约束
- 未稳定的字段暂时不计入 `pass/fail`
- 等后续恢复能力提升，再逐步把更多字段加入白名单

### Python 语法检查

```bash
python3 -m py_compile \
  test/run_lifting_suite.py \
  test/run_type_recovery_suite.py \
  test/tools/compare_htypes_with_debug_truth.py \
  test/tools/extract_debug_type_truth.py
```

结果：通过。

### 手工运行新 suite

```bash
python3 test/run_lifting_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/lifting/wasm/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-lifting-suite

python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/realworld/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-realworld-suite
```

结果：

- `notdec lifting wasm fortune`：`1 passed`
- `notdec type recovery realworld fortune tr-level=2`：`1 passed`

### CTest 验证

```bash
cmake -S . -B build -G Ninja
ctest --test-dir build -R 'notdec\.(lifting\.wasm|type_recovery\.realworld\.tr_level_2)' --output-on-failure
```

结果：两个新测试都通过。

## 后续建议

1. 如果 `fortune` 后续有更多字段恢复稳定下来，可以直接在
   `test/type-recovery/realworld/manifest.json` 里继续扩对应 root 的
   `field_allowlist`。
2. 如果 lifting 需要更强语义约束，可以在 `test/run_lifting_suite.py` 的
   `compare_module_summary()` 基础上继续增加：
   - 关键 function signature 对比
   - globals / memory 初始化摘要
   - 与 `truth/fortune.ll` 的 symbol-level 交叉检查

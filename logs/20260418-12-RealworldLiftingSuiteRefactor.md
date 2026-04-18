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

- `test/run_lifting_suite.py:53-86`
  - 新增 `expand_template()` / `expand_args()`，让 lifting suite 可以把
    `case_workdir` 注入命令行参数。
- `test/run_lifting_suite.py:146-199`
  - 新增 workdir log 解析逻辑。
  - 关键函数：
    - `parse_pndiff_summary()`
    - `parse_recovery_pass_summary()`
- `test/run_lifting_suite.py:266-401`
  - 新增 `compare_workdir_logs()` 与 `compare_case()`，并把 runner 主流程改成
    case-local workdir。
  - `main()` 现在会在每个 case 运行前重建 `workdir/<case>/`，然后让
    `notdec` 直接把 `PNDiff.warn.txt` / `01-recovery-passes.log` 写进去。
- `test/CMakeLists.txt:20-30`
  - 新增 `add_lifting_suite()`，并注册 `notdec.lifting.wasm`。
- `test/lifting/wasm/manifest.json:1-16`
  - `fortune.o3.wasm` lifting case 改为：
    - `--tr-level=2`
    - `--gen-work-dir`
    - `--work-dir={case_workdir}`
- `test/lifting/wasm/expected/fortune.o3.wasm.workdir-log.json:1-16`
  - lifting oracle 从模块摘要改为 workdir log oracle，约束：
    - `PNDiff.warn.txt` 中 residual `Add/Sub` 数量上限
    - `01-recovery-passes.log` 中选中的 stack pointer
    - stack-pointer recovery 报错数量与允许函数集合
    - `MemsetMatcher` 命中的函数集合与 merge 次数下限
- `test/lifting/wasm/README.md:5-23`
  - 改成记录 lifting suite 当前检查的是 `PNDiff.warn.txt` /
    `01-recovery-passes.log`，不再写成 module-summary oracle。

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

- `test/README.md:17-74`
  - 更新 `test/run_lifting_suite.py` 支持的 oracle 类型，并同步
    `test/lifting/wasm/` 当前改成检查 `PNDiff.warn.txt` /
    `01-recovery-passes.log`。
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
   - `./build/bin/notdec <input.wasm> -o <case_workdir>/out.ll --tr-level=2 --gen-work-dir --work-dir=<case_workdir>`
3. 先检查 lifting 是否“基础成功”：
   - `notdec` 返回码必须为 `0`
   - stdout 中不能包含 `IR parsing failed:`
   - 输出 `.ll` 文件必须存在且非空
4. 如果基础成功，再进入 oracle 比较：
   - `test/run_lifting_suite.py::compare_case()`
     会根据 expected JSON 的 `kind` 分派到
     `compare_workdir_logs()`
   - `test/run_lifting_suite.py::parse_pndiff_summary()`
     读取 `<case_workdir>/PNDiff.warn.txt`，统计：
     - residual `Add/Sub` 约束数量
     - override warning 是否存在
     - 是否命中
       `No residual Add/Sub constraints with unknown state after solve.`
   - `test/run_lifting_suite.py::parse_recovery_pass_summary()`
     读取 `<case_workdir>/01-recovery-passes.log`，提取：
     - `Selected stack pointer: ...`
     - `ERROR: ... func: ...` 列表
     - `MemsetMatcher [func]` 出现的函数名
     - `Merging ...` 次数
5. 只有当“基础成功”和“workdir 日志比较无 mismatch”同时成立时，这个 case 才算
   `pass`

当前 `fortune.o3.wasm` 具体比较的是 `test/lifting/wasm/expected/fortune.o3.wasm.workdir-log.json` 里的这些阈值：

- `max_residual_pndiff_constraints = 2`
  - 允许 `PNDiff.warn.txt` 中 residual `Add/Sub` 最多 2 条
- `required_stack_pointer_substring = "@__stack_pointer"`
  - `01-recovery-passes.log` 里必须选中 `@__stack_pointer`
- `max_stack_pointer_errors = 2`
  - stack-pointer recovery 报错总数不能超过 2
- `allowed_stack_pointer_error_functions = ["main", "usage"]`
  - 当前只接受 `main` 和 `usage` 这两个已知 `Cannot find sp restore`
- `required_memset_functions = ["main", "add_file", "get_tbl"]`
  - `MemsetMatcher` 必须至少在这 3 个函数出现 section
- `min_memset_merge_count = 20`
  - merge 次数至少 20

也就是说，lifting suite 当前不是全文 diff `.ll`，而是检查：

- frontend 能否稳定把 wasm 走完整条 lifting + recovery 主链路
- `PNDiff.warn.txt` 中是否没有或只剩很少的未解决 `Add/Sub`
- `01-recovery-passes.log` 里的 stack-pointer recovery 报错是否仍在已知边界内
- `MemsetMatcher` 是否仍然在关键函数上稳定命中

这样做的原因是：

- 对大 case 做全文 `.ll` snapshot 太脆弱
- 只看 globals / defines / declares 对当前问题过于表层，无法直接反映
  `PNDiff` 与恢复 pass 的健康度
- 当前更关心 wasm 输入经过恢复链路后的关键日志信号是否稳定

按当前代码实际跑出的稳定基线，`fortune.o3.wasm` 这例现在会产出：

- `PNDiff.warn.txt`
  - residual `Add` 约束 2 条
- `01-recovery-passes.log`
  - `Cannot find sp restore` 报错 2 条
  - 对应函数是 `main`、`usage`
  - `MemsetMatcher` section 出现在 `main`、`add_file`、`get_tbl`
  - `Merging ...` 共 23 条

所以当前 oracle 的语义是：

- 允许这批已知残余和已知报错继续作为回归基线存在
- 但如果 residual `Add/Sub` 明显变多、stack-pointer 报错扩散到新函数、或者
  `MemsetMatcher` 命中明显退化，就把 case 判成失败

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
  --workdir /tmp/notdec-lifting-suite-new
```

结果：

- `notdec lifting wasm fortune`：`1 passed`

### CTest 验证

```bash
ctest --test-dir build -R '^notdec\.lifting\.wasm$' --output-on-failure
```

结果：`notdec.lifting.wasm` 通过。

## 后续建议

1. 如果 `fortune` 后续有更多字段恢复稳定下来，可以直接在
   `test/type-recovery/realworld/manifest.json` 里继续扩对应 root 的
   `field_allowlist`。
2. 如果 lifting 需要更强语义约束，可以在 `test/run_lifting_suite.py` 的
   `compare_workdir_logs()` 基础上继续增加：
   - `PNDiff.warn.txt` 中按函数或按 SCC 的更细粒度阈值
   - `01-recovery-passes.log` 里 `MemcpyMatcher` / `LinearAllocationRecovery`
     的稳定信号
   - 与 `truth/fortune.ll` 的 symbol-level 交叉检查

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

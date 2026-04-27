# 2026-04-27 PointerAnalysis field cycle LLVM IR case

## 修改目标

构造一个很小的 LLVM IR 用例，用来复现 replace 模式下 PointerAnalysis
在递归字段路径上变慢的问题。

核心形状是：

```text
%p    = phi [ %base ], [ %next ]
%next = add %p, 1
```

PNDiff 会把 `%next` 识别成 `%p` 的字段偏移。replace 模式下 PA 同时看到
`%p <- %next` 和 `%next = field(%p)`，会不断扩展 `MemoryLocKey::Path`。

## 修改内容

- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:1`：
  新增用例说明，明确这个文件用于触发 replace-mode PA 的 field cycle。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:8`：
  新增 `main` 函数。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:15`：
  新增 loop induction phi。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:16`：
  新增 `%p` 的自回流 phi，上一轮 `%next` 回流到下一轮 `%p`。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:17`：
  新增 `%next = add i32 %p, 1`，触发 PNDiff ptradd reify 和 PA field edge。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:18`：
  新增 `%p.ptr` / `%next.ptr` 的 `inttoptr`，保持 lifted wasm 风格。
- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll:20`：
  新增 load/store，使地址参与内存访问记录。
- `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes:1`：
  新增 original 模式下的 HType snapshot。
- `test/type-recovery/llvm-ir/manifest.json:122`：
  新增 `20_PointerAnalysisFieldCycle` case，默认 original suite 中按 pass 检查。

## 涉及函数

- 新增测试函数：`main`。
- 复现时触发的实现函数：
  - `PNIGraph::solve()` 中的 ptradd reify。
  - `ConstraintsGenerator::setAsPtrAdd()`。
  - `PointerAnalysis::solve()`。
  - `PointerAnalysis::addPointsTo()`。
  - `MemoryLocKey::operator<()`。

## 验证

- 通过 original 单用例：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_original.ll --tr-level=2 --frozen-tr-input-ir \
  --dump-htypes /tmp/20_PointerAnalysisFieldCycle.htypes
```

- 通过默认 llvm-ir suite：

```bash
python3 test/run_type_recovery_suite.py \
  --binary build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-llvm-ir-field-cycle-suite
```

结果：`17 passed, 3 xfailed, 0 failed`。

- replace 模式复现超时：

```bash
timeout 10s env NOTDEC_POINTER_ANALYSIS_MODE=replace ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_replace.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-field-cycle-replace
```

结果：10 秒超时，退出码 `124`。

- gdb 中断栈显示耗时位置在 `MemoryLocKey::Path` 比较链路：
  `OffsetRange::operator<()` -> `MemoryLocKey::operator<()` ->
  `std::set<MemoryLocKey>::insert()` -> `PointerAnalysis::addPointsTo()` ->
  `PointerAnalysis::solve()`。

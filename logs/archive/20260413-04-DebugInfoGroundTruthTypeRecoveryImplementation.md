# 2026-04-13 DebugInfo Ground Truth Type Recovery Implementation

## 本次落地内容

围绕 `logs/20260413-03-DebugInfoGroundTruthTypeRecoveryProposal.md`，先实现了 Phase 1 的最小闭环：

1. 扩展 `test/run_type_recovery_suite.py`
   - 保留现有 `htype-snapshot` 路径
   - 新增 `debug-struct-compare` oracle
   - 新增 `strip-debug` 的 `source_prep`
   - 新增 `prepare_truth` 与 `compare_oracle` 阶段

2. 新增工具脚本
   - `test/tools/gen_debug_info_case.py`
     - 统一把 C case 编译成 `wasm32-unknown-wasi` 的 `*.dbg.ll`
     - 支持 `--check`
   - `test/tools/extract_debug_type_truth.py`
     - 从 `*.dbg.ll` 中抽取结构体与变量类型真值
     - 当前优先覆盖 `DIBasicType` / `DIDerivedType(pointer)` /
       `DICompositeType(struct)` / `DILocalVariable` / `DIGlobalVariable`
   - `test/tools/compare_htypes_with_debug_truth.py`
     - 解析 `.htypes`
     - 对 recovered `[memory]` decl 做相对偏移语义比较
     - 当前支持 primitive / pointer / self-recursive pointer 的第一阶段比较

3. 新增 suite
   - `test/type-recovery/debug-info-c/`
   - 首批 pass case：
     - `01_absolute_pair`
     - `02_absolute_self_ref`
   - `truth/*.dbg.ll` 通过生成脚本 check in

4. 更新测试入口文档
   - `test/CMakeLists.txt`
   - `test/README.md`

## 设计取舍

第一版没有强行侵入 `notdec` 主链路，也没有新增 machine-readable HType dump。

对 recovered 侧的比较视图，当前采用了偏工程化的第一阶段策略：

- root 先只支持 manifest 显式声明的 `memory` root
- case 通过固定绝对地址把源码 struct 绑定到 recovered `[memory]` decl
- 比较时按 `base_offset_bytes + field_offset` 对齐 recovered 字段
- 对 memory slot 中的 `ptr<load=..., store=..., psize=...>`，优先把 `store`
  视作“该槽位存储的值类型”
- 若 `store` 类型能对齐到某个 `addr(0x...)` 的 HType，再把它解释为指针目标

这让 `struct Pair { int; int *; }` 和自引用 `struct Node` 两类最小 case
已经可以用 debug metadata 做 pass/fail。

## 验证

脚本级验证：

```bash
python3 -m py_compile \
  test/run_type_recovery_suite.py \
  test/tools/gen_debug_info_case.py \
  test/tools/extract_debug_type_truth.py \
  test/tools/compare_htypes_with_debug_truth.py
```

truth 自检：

```bash
python3 test/tools/gen_debug_info_case.py \
  --case test/type-recovery/debug-info-c/cases/01_absolute_pair.c \
  --output test/type-recovery/debug-info-c/truth/01_absolute_pair.dbg.ll \
  --check

python3 test/tools/gen_debug_info_case.py \
  --case test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c \
  --output test/type-recovery/debug-info-c/truth/02_absolute_self_ref.dbg.ll \
  --check
```

runner 验证：

```bash
ASAN_OPTIONS=detect_leaks=0 \
python3 test/run_type_recovery_suite.py \
  --binary build/bin/notdec \
  --manifest test/type-recovery/debug-info-c/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-debug-info-suite-asan-off
```

结果：

- `01_absolute_pair` pass
- `02_absolute_self_ref` pass

兼容性抽查：

```bash
ASAN_OPTIONS=detect_leaks=0 \
python3 test/run_type_recovery_suite.py \
  --binary build/bin/notdec \
  --manifest /tmp/notdec-single-snapshot-manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-single-snapshot-workdir
```

其中单 case snapshot 路径 `06_SimpleRecursive2` 通过，说明旧 oracle 路径仍可工作。

## 当前限制

1. truth extractor 目前只覆盖第一阶段需要的 DI 语义，尚未细化 array /
   union / richer typedef 展开
2. comparator 当前只实现了 `memory` root，不支持把 debug truth 直接对齐到
   `.htypes` 的一般 `[types]` stable key
3. 在当前 agent 运行环境下，直接运行 `notdec` 会被 LeakSanitizer 的
   ptrace 限制打断返回码；本次验证使用临时
   `ASAN_OPTIONS=detect_leaks=0`，但没有把这个重新写回仓库默认配置
4. 重新 `cmake -S . -B build` 后当前 `ctest --test-dir build` 仍显示
   `No tests were found`，说明这份现有 build tree 没有启用测试注册；
   suite 本身已接入 `test/CMakeLists.txt`，但该 build 目录下未形成可执行的
   CTest 清单

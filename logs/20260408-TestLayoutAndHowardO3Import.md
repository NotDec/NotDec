# 2026-04-08 Test Layout And HOWARD O3 Import

## 背景

这次工作包含两部分：

- 继续清理主仓库 `test/` 布局，使当前权威回归和历史实验目录分离
- 将 HOWARD 实验目录中的 O3 split LLVM IR 全量导入主仓库，作为后续类型推理回归语料

---

## 1. Test Layout Follow-up

测试布局进一步统一为：

- `test/type-recovery/llvm-ir/`
- `test/type-recovery/sysy/`
- `test/run_type_recovery_suite.py`
- `test/legacy/wasm/`

其中：

- `sysy` suite 通过 manifest 驱动，先把 `cases/*.c` 编译为 LLVM IR，再复用同一套 HType snapshot runner
- 旧的 `test/wasm/` 工作区迁入 `test/legacy/wasm/`，避免继续与当前权威回归布局混放

---

## 2. HOWARD O3 Corpus Import

新增：

- `test/type-recovery/howard-o3-split/`

该 suite 全量导入：

- `/sn640/NotDec-Exp/ICSE-HOWARD/splited/splitted_irs`

中所有 `*.o3*.ll` 输入，共 92 个 case，覆盖：

- `fortune`
- `grep`
- `gzip`
- `lighttpd`
- `wget`

当前策略：

- 先全量入库到 `cases/`
- 先接入 `CTest`
- `manifest.json` 中统一标记为 `skip`
- 后续再逐步从 `skip` 提升为 `xfail/pass`，并补 `expected/tr-level-2/*.htypes`

初步人工 baseline 探测表明，前若干个 O3 case 在 `--tr-level=2` 下大多会触发：

- `PNDiff assertion while looking up PNI variable`

因此本批提交不尝试一次性打通该语料，只保证：

- 语料完整入库
- suite 结构与主测试布局一致
- 现有 `ctest` 不被新语料阻塞

---

## 3. Howard Front Cases Follow-up

后续继续抽查最前面的三个 case：

- `fortune.o3.wasm.1`
- `fortune.o3.wasm.2`
- `fortune.o3.wasm.3`

统一使用 `.vscode/launch.json` 里的 `dec` 配置同一组环境变量，等价命令形态为：

```bash
env \
  NOTDEC_DEBUG_DIR=/tmp/notdec-howard-caseX \
  NOTDEC_TYPE_RECOVERY_DEBUG_DIR=/tmp/notdec-howard-caseX \
  NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec-Exp/ICSE-HOWARD/summarys.json \
  NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec-Exp/ICSE-HOWARD/signatures.json \
  NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  NOTDEC_DISABLE_INTERPROC=1 \
  NOTDEC_SAT_DISABLE=1 \
  ASAN_OPTIONS=detect_leaks=0 \
  ./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.N.ll \
    -o /tmp/notdec-howard-caseX/out.ll \
    --tr-level=2 \
    --dump-htypes /tmp/notdec-howard-caseX/out.htypes
```

### 3.1 初始稳定复现问题

这三个 case 最开始暴露的是一组很集中的 `mlsub` 入口问题，而不是
HOWARD 特有的复杂类型冲突：

1. `visitICmpInst -> addCmpConstraint()` 直接 `PG.unifyVar(LHS, RHS)`，
   对常量 operand 没先 canonicalize / 建 PNI 节点，触发
   `PNDiff::getPNIVar()` 断言
2. `convertSimpleTypeVal()` 对
   - `ConstantPointerNull`
   - `UndefValue`
   仍然保留 `TODO` 断言
3. `PcodeOpType::addOpConstraint()` 在给常量 operand 取 size 时丢了
   `User/OpInd`，触发 `llvmValue2ExtVal(... User is Null)` 断言
4. `visitCallBase()` 把 `llvm.memset.*` 这类 intrinsic 落进
   `unhandledCalls`，后续 `bottomUpPhase()` 试图把它们当普通 SCC callee
   接 summary，最终在 `Func2SCCIndex.at(...)` 处抛 `std::out_of_range`

### 3.2 本次最小修补

在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 中做了四个很小的对齐修补：

- `addCmpConstraint()` 改成先对两侧 operand 做 `llvmValue2ExtVal()`，
  再 `getOrInsertNode()`，最后用 canonicalized key 做 `PNI` unify
- `convertSimpleTypeVal()` 为 `ConstantPointerNull` / `UndefValue`
  返回普通 `binarysub::variable`
- `PcodeOpType::addOpConstraint()` 对 operand size 改用
  `cg.getSize(Op, I, Index)`，保留常量上下文
- `handleIntrinsicCall()` 暂时将 `llvm.memset` / `llvm.memcpy` /
  `llvm.memmove` 视为已处理，避免进入 `unhandledCalls`

这批改动刻意不碰 llvm2c，也不改 `HType` 输出策略；目标仅是先把
HOWARD 前沿 case 从“稳定崩溃”推进到“能够产出 HType snapshot”。

### 3.3 当前结果

修补后，下面三个 case 在 `--tr-level=2 --dump-htypes` 下都能成功产出：

- `/tmp/notdec-howard-case1/out.ll`
- `/tmp/notdec-howard-case1/out.htypes`
- `/tmp/notdec-howard-case2/out.ll`
- `/tmp/notdec-howard-case2/out.htypes`
- `/tmp/notdec-howard-case3/out.ll`
- `/tmp/notdec-howard-case3/out.htypes`

其中 `out.htypes` 均为非空，可作为后续 golden baseline 的候选起点。

### 3.4 剩余观察

虽然前三个 case 已经不再崩溃，但运行日志里仍稳定出现：

- `WARN: MLsubGenerator: unhandled instruction return/op: fsub`
- `WARN: MLsubGenerator: unhandled instruction return/op: fmul`
- `WARN: MLsubGenerator: unhandled instruction return/op: fdiv`

这说明当前前几个 HOWARD case 的“主阻塞”已经从
“常量 / compare / intrinsic 路径未接好导致直接 abort”
转移为：

- 浮点算术目前主要是放行但不充分建模
- 后续更可能影响类型质量，而不是进程稳定性

因此下一批更合适的方向是：

1. 决定浮点二元算术在 `mlsub` 中至少采用何种最小保守建模
2. 基于当前三个已跑通 case 补 `expected/tr-level-2/*.htypes`
3. 再把 manifest 中前几个 `fortune` case 从 `skip` 提升为
   `pass` 或 `xfail`

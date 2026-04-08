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

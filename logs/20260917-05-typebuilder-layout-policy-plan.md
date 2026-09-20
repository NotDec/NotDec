# TypeBuilder 布局策略开关：A(panel-narrow) / B(object-consensus)

日期：2026-09-17

## 用户原始 prompt

> 把这两种算法都引入 TypeBuilder 吧，然后增加开关允许切换。当前的评估的方式和那边 Bench2
> 文件夹里面 RecStruct 的评估链路的一致吗？

澄清后的选择：

- A = `panel_narrow`：重叠字节簇只保留一个代表（最窄），不再把重叠证据全部输出；
- B = `type-consensus`：按运行时分组（用户选定：**同一内存对象 / 同一 SCC 的访问**）聚合证据，
  每个 offset 取最窄宽度，并与当前布局取并集（对应离线表的 `hyb_type_majority_narrow_recovered`）。

## 背景事实（离线数据集，578 case，全部 GT 字段口径）

| 策略 | P | R | F1 |
| --- | ---: | ---: | ---: |
| recovered（当前 TypeBuilder 输出） | 0.558 | 0.128 | 0.208 |
| identity（per-root 证据原样） | 0.420 | 0.095 | 0.155 |
| panel_narrow（per-root 重叠取最窄） | 0.630 | 0.091 | 0.159 |
| type_union（跨 root 合并证据，指针宽度） | 0.312 | 0.348 | 0.329 |
| type_majority_narrow（跨 root + 每 offset 取最窄） | 0.343 | 0.383 | 0.362 |
| hyb_type_majority_narrow_recovered（B 的离线形态） | 0.351 | 0.405 | 0.376 |

收益分解：跨 root 覆盖 +0.174（identity→type_union），每 offset 取最窄 +0.033
（type_union→type_majority_narrow），与生产布局并集再 +0.014。A 单独替换是 precision 杠杆
（0.420→0.630），与生产布局并集后 F1 0.208→0.238。

注意：离线表的 B 用 DWARF strict key 分组（有调试信息），真实流水线没有这个键；
因此按用户选择改为"同一内存对象 / 同一 SCC 的访问"分组，能拿到多少收益需要实测。

## 设计

### 开关

- `notdec --layout-policy=baseline|panel|consensus|consensus+panel`（默认 baseline，行为不变）；
- 同步环境变量 `NOTDEC_LAYOUT_POLICY`，让 microsub2（直接构造 MLsubRecovery）也能切；
- 传递链：`DecompilerContext::Options.layoutPolicy` → `MLsubRecovery::setLayoutPolicy()` →
  `TypeBuilderContext` → `TypeBuilder`；microsub2 侧在 `runTypeRecovery` 里读取 env 或新增
  `--layout-policy` 参数。

### A：panel-narrow（`external/binarysub/src/TypeBuilder.cpp`）

- 入口：`craftStruct(Fields, ValidRange, Name, T, IsCovariant)`（约 1259 行）。`Fields` 是
  已排序的 `(SimpleRange, HType*)` 证据列表，当前对每个 entry 都产出一个 `FieldDecl`。
- 新增前置步骤：按 byte range 求连通簇；每个簇只保留最窄的一个代表（同宽取 offset 最小），
  簇之间的真实空隙仍按现有逻辑生成 `padding_`（`isPadding=true`），可选子开关
  `--layout-padding=fields|gaps` 控制 padding 是否作为字段输出。
- 影响面：只在 policy != baseline 时改变 `Fields`，不触碰 `convertStruct` 的字段收集、
  `rememberExactRecordLayout` 缓存键（缓存仍按原始 Fields 记录，需要把 policy 计入 key，否则
  不同 policy 的运行会互相污染——只影响同进程内，且 policy 全局一致，先用断言保证）。

### B：object-consensus

- 证据聚合点：`MLsubGenerator.cpp` 里 SCC 级转换（`TypeBuilder TB(TBCtx)` 约 7025 行）遍历
  该 SCC 的 value 并调用 TypeBuilder。按"同一内存对象"分组：用该 value 的 canonical UType
  记录对象（或 PNDiff/V2N 指向的同一 memory object）作为组键，把组内所有 value 的 record 字段
  并集作为该组布局的输入，转换一次并复用（现有 `PrevDecl`/`findExactRecordLayout` 已提供
  "同一 Decl 用更丰富字段重建"的机制，可扩展为并集 + 每 offset 取最窄）。
- 与当前布局取并集：B 的输出与 baseline 输出按 (offset,size) 去重合并（对应离线 `hyb_*`）。
- 风险：无调试信息时"同一对象"的判定可能有误合并；实现时只合并同一 SCC 内 canonical
  UType 相同/同一 memory object 的 value，不做跨 SCC 结构相似度合并。

## 验证

1. 离线：对齐 Eval2 人口（过滤 opaque 类型；GT 无预测的参数也计入分母）后重建数据集并重算
   全部策略，作为基线；再为 policy=A/B 增加"从 TypeBuilder 输出直接构造布局"的对照。
2. 端到端：`coreutils` 子集 + `memcached/tmux/vsftpd/wrk` 的真实 Eval2 A/B（同一 IR、同一 GT），
   同时检查输出 IR 通过 LLVM 22 verifier、`bad_unions`/`fragmented_*` 不恶化。
3. 回归：`MLsubGeneratorTest`、`binarysub-test`、`ctest -R notdec.type_recovery.llvm_ir.tr_level_2`。

## 回退

所有新行为都在 `layoutPolicy != baseline` 时才生效；默认路径不改变任何输出，可用开关即时回退。

## 实现记录与验证（2026-09-17，goal round 1-2）

### 代码

- `external/binarysub/include/binarysub/TypeBuilder.h`、`src/TypeBuilder.cpp`：
  - `LayoutPolicy{Baseline, PanelNarrow, ObjectConsensus, ObjectConsensusPanel}` +
    `parseLayoutPolicy` / `layoutPolicyName`；
  - A：`convertStruct` 的重叠簇分支只保留最窄代表（`keepsOverlapRepresentative()`）；
  - B：`craftStruct` 外包一层 consensus——按字段起始 offset 建倒排索引找候选组，
    兼容判据为"共享 offset 上 canonical 字段类型一致，且共享数 ≥2 或 ≥较小记录的一半"，
    并要求并集两两不重叠（否则跳过），加入组时用并集经锚点重建共享 decl；
    锚点注册进 `TypeCache`，使 craftStruct 的 PrevDecl 路径扩展同一个 decl。
- 解决 consensus 只在 SCC 内的问题：`TypeBuilderContext` 承载 consensus 注册表，
  `MLsubRecovery::solveAndLowerTypes()` 在 consensus 策略下创建一个跨 SCC 共享的 context
  传给 `genTypes(...)`（默认策略仍是每 SCC 一个 context，行为不变）。
- `src/NotDec.cpp`：`--layout-policy=baseline|panel|consensus|consensus+panel`，
  环境变量 `NOTDEC_LAYOUT_POLICY`；`PassManager.h/.cpp` 透传到 `MLsubRecovery`；
  microsub2 在 `runTypeRecovery` 读同一环境变量。

### 验证

- notdec 重建通过；microsub2 重新链接通过；两个入口都验证开关生效
  （wrk：297→186 decls / 926→700 fields；microsub2 arch：68→57 / 246→199）。
- source-ir 5 个 case 的离线 Eval2 口径（用 `--dump-htypes` 的**最终** HType，因为跨 SCC 扩展会
  让早期 SCC 的 work-dir 文件过时）：

| case | baseline | panel (A) | consensus (B) | consensus+panel |
| --- | ---: | ---: | ---: | ---: |
| ffplay | 0.162 | 0.197 | 0.163 | 0.199 |
| fortune | 0.020 | 0.019 | 0.020 | 0.019 |
| memcached | 0.126 | 0.148 | 0.126 | 0.149 |
| ngircd | 0.035 | 0.045 | 0.035 | 0.045 |
| wrk | 0.240 | 0.232 | 0.240 | 0.232 |
| **均值** | **0.116** | **0.128** | **0.117** | **0.129** |

### 结论与风险

- A 是有效杠杆（+0.012 均值；decl/field 数大幅下降），B 目前只有 +0.001。
- B 的两个迭代坑：以"字段集合哈希"为键时只能配对完全等价的记录（无收益）；
  放开成倒排索引后并集可能重叠，触发 `RecordDecl::addField` 的 overlap 断言，已加不重叠守卫。
- 离线表里 B 的 +0.17 来自**按 DWARF 类型分组**（有调试信息），运行时没有这个键；
  用结构兼容守卫代替后只能合并本来就同源的记录，因此收益很小。
- 回退：所有新行为都在 `layoutPolicy != baseline` 时生效，默认可即时回退。

## 真实 Eval2 A/B（2026-09-18，goal round 3）

工具：`Bench2/scripts/policy_eval_ab.py`——复用已有 IDA 库重跑 microsub2（`NOTDEC_LAYOUT_POLICY=<policy>`），
用与 Bench2 flow 相同的 `microsub2_to_recstruct` 桥接转换，再用 RecStruct `run-eval` 打分；结果写到
`results/microsub2<policy>-<case>/`，基线 `results/microsub2-<case>/` 不动。

coreutils 40 个 case（O0-Os 各 8 个，baseline 平均 F1 0.2527）：

| 策略 | 平均 ΔF1 | 中位 ΔF1 | 胜/负/平 |
| --- | ---: | ---: | ---: |
| panel (A) | **-0.0130** | +0.0003 | 20/17/3 |
| consensus+panel (A+B) | -0.0135 | 0.0000 | 19/18/3 |

- 39/40 个 case 上 A+B 与 A 完全相同 → B 在真实指标上仍然没有增量。
- 最大回退：O2/chown 0.300→0.151、O2/chgrp 0.298→0.151、Os/chown 0.257→0.178、O1/cksum 0.095→0.021；
  最大提升：O0/base32 0.218→0.275、O1/comm 0.468→0.500、O1/chcon 0.228→0.257。
- 结论：A 缩小布局（字段数降 60-85%）但**真实 Eval2 上是净负**（离线代理上的 +0.012 不可靠）；
  B 无论离线还是真实指标都只有 ~0.001 量级。两个策略都保留在开关后面，**默认仍用 baseline**。

## 2026-09-18：移除 B（ObjectConsensus）

真实 Eval2 上 `consensus+panel` 与 `panel` 在 40 个 coreutils case 里有 39 个完全相同，
B 的增量只有 ~0.001；离线代理同样只有 +0.001。按"没有收益就去掉"，删除：

- `LayoutPolicy` 只保留 `Baseline` / `PanelNarrow`，`parseLayoutPolicy` 不再接受 consensus；
- 删除 consensus 组、倒排索引、兼容判据、跨 SCC 共享 `TypeBuilderContext`、
  `craftStruct` 包装层（`craftStructImpl` 合并回 `craftStruct`）；
- `genTypes` 去掉 `SharedContext` 参数；microsub2 仍读 `NOTDEC_LAYOUT_POLICY`，
  现在只接受 baseline/panel。

A（panel）暂时保留在开关后（默认 baseline）；真实 Eval2 上它是 20 胜/17 负、均值 -0.0130，
是否一并删除待定（见下方讨论）。

## 2026-09-18 最终决定：A 也移除，回到 baseline

A（panel）在真实 Eval2（40 个 coreutils case）上是 20 胜 / 17 负 / 3 平，均值 -0.0130、
中位 +0.0003；离线代理上是 +0.012（代理偏乐观）。按"没有收益就移除"的原则，用户确认把 A 也删掉。

已回退的内容：

- `external/binarysub`：`TypeBuilder.h` / `TypeBuilder.cpp` 与 HEAD 完全一致（LayoutPolicy、
  parse/name 函数、context 字段、panel 分支全部删除；`craftStruct` 恢复原样）；
- `NotDec`：`--layout-policy` CLI、`NOTDEC_LAYOUT_POLICY` 读取、`Options.layoutPolicy`、
  PassEnv/`build_passes` 参数、`MLsubRecovery::setLayoutPolicy` 与 `genTypes` 参数全部删除
  （相关文件与 HEAD 一致）；
- `microsub2`：移除 `NOTDEC_LAYOUT_POLICY` 读取；**保留** `--work-dir`（数据集采集需要，属于另一项工作）。

验证：`notdec --help` 不再有 `--layout-policy`；wrk / microsub2 arch 均正常运行，输出通过 verifier。
结论记录：类型级共识（B）与 panel 收窄（A）在真实指标上都没有正收益，因此默认路径保持原算法。

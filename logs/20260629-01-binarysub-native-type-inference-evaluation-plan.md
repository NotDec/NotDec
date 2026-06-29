# 原始 prompt

```text
把详细的规划写成一个文档
```

# 讨论输入

本计划基于前一轮讨论的结论：

- 论文主 claim 是恢复反编译可用、可验证的结构化类型关系，不承诺完整还原源码类型名。
- 实验只拿 native ELF 做主结果。Bench2 已收集 Manta 论文相关项目的一批 Ubuntu 24.04 amd64 目标和匹配 debug info。
- 真值主要是可观测、确定性的类型关系。typedef 可以单独考虑，因为当前算法支持从库函数签名和用户定义 subtype lattice 传递一部分 typedef 信息。
- 实验方案先尽量铺全，能算的指标先规划进去。实现和论文写作时再删掉不稳定或解释价值不高的指标。
- 外部 baseline 放进规划，后续单独做。
- 第一批基于 Bench2 已收集的几个目标。

# 背景

`external/binarysub/paper` 目前已经有论文主线和形式化方向，但
`external/binarysub/paper/tex/sections/09-evaluation.tex` 仍是 TODO。现在需要先把实验
规划写清楚，再决定后续 evaluator、NotDec 类型推理接入和论文评估章怎么落地。

Bench2 数据集在 `/sn640/NotDec-Exp/Bench2`。它已经收集了多个真实项目的 native ELF
和 debug info，目标清单在：

- `/sn640/NotDec-Exp/Bench2/manifest/benchmark-targets.tsv`
- `/sn640/NotDec-Exp/Bench2/manifest/benchmark-needed.tsv`
- `/sn640/NotDec-Exp/Bench2/manifest/elf-buildids.tsv`

当前目标包括 vsftpd、libuv、memcached、lighttpd、tmux、OpenSSH、wolfssl、Redis、
libicu、Vim、Python、wrk、FFmpeg、fortune、PHP 等。多数目标有 build-id 对应的
`/usr/lib/debug/.build-id/...debug` 文件，Python debug 版本有 embedded debug。这个集合
足够支撑 native ELF 的第一版实验。

仓库里已有 wasm 侧 DWARF 对比管线：

- `test/tools/extract_wasm_dwarf_truth.py`
- `test/tools/compare_htypes_with_wasm_dwarf.py`
- `test/type-recovery/realworld/manifest.json`

这条管线证明了“用 DWARF 作为 oracle，对 `.htypes` 做语义比较”这个方向可行。但它现在
面向 wasm 和回归测试，不是 native ELF 的论文级批量评测。native 实验应复用它的思想，
不要直接继承它的范围限制。

# 目标

本实验规划的目标是回答一个核心问题：

> NotDec 的二进制类型推理能否在 stripped native ELF 上恢复反编译真正需要的类型关系，
> 并且这些关系能被 DWARF ground truth 稳定验证？

具体目标：

- 建立 native ELF 的 DWARF oracle 和 NotDec 输出之间的统一关系模型。
- 用 Bench2 真实项目量化类型恢复的准确率、覆盖率和性能。
- 分开评估结构体布局、字段类型、指针关系、函数签名、数组、嵌套结构、递归结构、函数指针和 typedef/subtype 传播。
- 用 ablation 说明寄存器消除、summary、PtrStore/PtrLoad、数组/结构体重建等模块分别带来的收益。
- 规划外部 baseline，对比 Ghidra 以及可运行的 Retypd、TRex、Manta 类工具。
- 形成论文 Evaluation 章节可以直接使用的 RQ、表格、图和 case study 设计。

# 非目标

下面这些不要放进主 claim：

- 不承诺恢复所有源码 typedef 名字。typedef 名字在主准确率里不作为必须匹配项，只做单独实验。
- 不把未访问字段的恢复率当成主指标。未访问字段从 stripped binary 行为里本来就不可知，可以做补充统计。
- 不把 local variable 的源码级一一对应作为主指标。优化后的 DWARF location list、寄存器复用和反编译变量拆合并会引入大量噪声。
- 不把 decompiled C 文本是否和源码相似作为主指标。本文评估类型关系，而不是整体反编译文本相似度。
- 不把外部 baseline 的不可运行 artifact 当成必须完成项。能跑就比，不能跑就只做定性说明或引用论文已报告结果。

# 参考实验设计

从 Zotero 里已经查到几类相关工作：

- Kline 和 Kulkarni 的 decompiler inference accuracy framework：用 DWARF 做 ground truth，
  比较函数、变量、类型等 source-level constructs。本文可以借它的“统一中间表示 +
  ground truth/decompiler 双向抽取 + 指标报告”思路。
- Finding the Dwarf：用大规模 WebAssembly DWARF 数据评估类型恢复，适合借鉴 top-level
  type exact match、参数/返回类型准确率和数据集统计方式。
- Retypd：是本文最重要的二进制类型推理相关工作。它报告 const annotation recall 等
  类型推理能力，适合放 related work 和 baseline 讨论。
- Manta：目标集合和 Bench2 有交集，报告 precision/recall，并把类型信息用于下游 bug
  detection。本文可以借鉴 precision/recall 口径，但主任务是反编译类型恢复。
- TRex：强调 behavior-capturing types，而不是完全恢复源码类型。这个观点和本文主 claim
  一致，可以支持“canonical shape 优先，typedef 名字单独评估”的口径。

# Research Questions

## RQ1: 总体准确率

问题：NotDec 在 stripped native ELF 上能恢复多少可验证类型关系？

主指标：

- `coverage`：DWARF truth 中有多少 root 能在 NotDec 输出里找到可比较对象。
- `precision`：NotDec 输出中可判断的类型事实有多少和 DWARF 一致。
- `recall`：DWARF 中可观测的类型事实有多少被 NotDec 恢复。
- `F1`：precision 和 recall 的调和平均。
- `exact-shape match`：整个函数签名、全局对象或结构体 slice 是否完整匹配。
- `field-offset match`：字段偏移是否匹配。
- `field-type match`：字段类型形状是否匹配。

注意：

- 主 recall 的分母只放可观测事实。所谓可观测，指 binary 中存在访问、传参、返回、全局引用或库函数约束能支持该事实。
- `DWARF-all-field recall` 可以作为补充，展示源程序完整字段恢复到什么程度，但不能作为主结论。

## RQ2: 复杂类型恢复能力

问题：系统对不同类型构造的恢复能力是否均衡？

分组：

- primitive scalar
- pointer
- struct field
- array and stride
- nested struct
- recursive struct
- function pointer
- callback signature
- global object
- function parameter
- function return

每组报告：

- truth fact count
- recovered fact count
- matched fact count
- precision
- recall
- representative failures

## RQ3: typedef 和用户 subtype lattice

问题：库函数签名和用户 subtype lattice 是否能让 typedef 信息在 stripped binary 中传播？

主表不要求 typedef 名字匹配。typedef 单独看：

- `alias coverage`：truth 中有 typedef/alias 的对象，有多少能在输出里保留 alias 信息。
- `alias precision`：输出 alias 和 truth alias 是否一致，或者是否落在同一个用户定义 subtype lattice 等价类里。
- `canonical shape unchanged`：打开 typedef/subtype 传播后，canonical shape 准确率是否不下降。
- `summary propagation count`：从库函数签名传播出来的 typedef/subtype 约束数量。

建议把 typedef 分为两类：

- C 语言层面普通 typedef，例如 `size_t`、`uint32_t`、项目自定义 handle。
- 用户 subtype lattice 定义的语义别名，例如 file descriptor、socket、length、offset、errno-like status。

第一类适合和 DWARF alias 比较。第二类更多体现工具输出价值，可能需要人工定义 truth 或 case study。

## RQ4: 模块贡献

问题：关键模块分别贡献了什么？

建议 ablation：

- full pipeline
- no register elimination
- no library summaries
- no user subtype lattice
- no PtrStore/PtrLoad propagation
- no array reconstruction
- no nested struct reconstruction
- no recursive type folding
- no typedef preservation

每个 ablation 至少报告：

- notdec success/fail/timeout count
- root coverage
- field-level precision/recall/F1
- signature accuracy
- type inference time
- peak memory
- constraint count

寄存器消除是前置关键阶段。它的 ablation 可能导致很多目标根本跑不通，这种情况下不要只写
accuracy 下降，要单独报告 pipeline availability。

## RQ5: 外部 baseline

问题：和现有工具相比，NotDec 的类型输出有什么差异？

优先级：

1. Ghidra：最现实。Bench2 已有 Ghidra 项目和 bin2llvm 相关流程，Ghidra 自身也能解析
   debug info 之外的 binary。需要确认导出类型的方式，避免把 debug info 泄露给 baseline。
2. Retypd：如果 artifact 能跑，比较类型关系和 const/pointer 相关输出。如果跑不通，做定性比较。
3. TRex：如果 artifact 能跑，重点比较 behavior-capturing type 和 canonical shape。否则引用论文结果。
4. Manta：Bench2 项目和 Manta 有关系，但 Manta 目标偏 bug detection。可做 precision/recall 口径参考，
   不强行要求同表全量比较。

baseline 的公平性要求：

- 输入必须是 stripped binary。
- baseline 不能读取目标程序 debug info。
- 允许使用系统库头文件和标准函数签名，但要固定版本。
- 输出要转换成同一套 type relation schema 后再比较。

## RQ6: 性能和可扩展性

问题：系统在真实项目上是否能稳定跑完？

报告内容：

- target size
- function count
- recovered function count
- lifted IR size
- generated constraint count
- type variable count
- memory object count
- solver time
- reconstruction time
- total NotDec time
- peak RSS
- timeout count
- crash count

如果某些大目标只跑 shared library，不跑完整 executable，要在 benchmark table 里明确写出选择原因。

# Benchmark 设计

## 第一批 pilot

第一批要覆盖多种类型现象，同时避免一开始被超大目标拖住。

建议 pilot：

| Project | Target | 理由 |
| --- | --- | --- |
| fortune | `/usr/games/fortune` | 小目标，已有经验，适合打通 native oracle。 |
| libuv | `libuv.so.1.0.0` | shared library，API、handle、callback 和 typedef 多。 |
| memcached | `/usr/bin/memcached` | C 服务端，结构体、全局状态和库调用多。 |
| lighttpd 或 vsftpd | `/usr/sbin/lighttpd` 或 `/usr/sbin/vsftpd` | 网络服务，适合结构体 case study。 |
| redis-benchmark 或 redis-cli | `/usr/bin/redis-benchmark` 或 `/usr/bin/redis-cli` | Redis 项目但比 server 小，适合稳定性检查。 |
| wolfssl | `libwolfssl.so.42.0.0` | typedef、结构体和库 API 价值高。 |

第一批判断标准：

- 每个目标都能抽出 DWARF truth inventory。
- 至少 3 个目标能跑完 NotDec 类型推理。
- 至少 2 个目标能产出 field-level report。
- 至少 1 个 shared library 目标能产出 function signature report。

## 第二批扩展

第二批用于补充规模和多样性：

- tmux：中等复杂度，结构体和终端状态多。
- OpenSSH client/server：安全项目，类型复杂，但运行和解析成本可能高。
- Vim：大型 C 项目，适合展示 scalability。
- PHP extensions：`.so` 目标小，适合模块级分析。
- Python shared library：大型 C API，typedef 多。

## 第三批压力测试

第三批用于性能和失败分析：

- FFmpeg libraries：目标大，类型多，适合 scalability，但不适合第一阶段调 evaluator。
- libicu libraries：C++ 成分和模板类型可能明显增加 DWARF 复杂度，需要单独说明处理边界。
- Python interpreter/debug build：规模大，可能有 embedded debug，适合作为后期 stress。

# Truth Schema

建议把 DWARF truth 和 NotDec 输出都转换为同一套 relation schema。不要直接比较 C 字符串。

## Root

Root 是可比较对象的入口：

- `function:<name or address>`
- `function_param:<function>:<index>`
- `function_return:<function>`
- `global:<name or address>`
- `record:<dwarf type id>`
- `library_summary:<symbol>`

local variable 暂时不进主指标，只进入补充统计。

## Type Shape

类型形状建议包含：

- `kind`: primitive、pointer、record、array、function、typedef、unknown
- `size_bits`
- `signedness`: signed、unsigned、unknown
- `pointee`
- `record_id`
- `fields`
- `elem`
- `stride_bits`
- `count`
- `return_type`
- `param_types`
- `alias`
- `canonical`

## Facts

核心 facts：

- `FunctionSignatureFact(function, param_shapes, return_shape)`
- `FunctionParamFact(function, index, shape)`
- `FunctionReturnFact(function, shape)`
- `GlobalTypeFact(address_or_name, shape)`
- `RecordFieldFact(record, offset_bits, size_bits, field_shape)`
- `PointerFact(source, pointee_shape)`
- `ArrayFact(source, elem_shape, stride_bits, count_optional)`
- `FunctionPointerFact(source, param_shapes, return_shape)`
- `TypedefFact(alias, canonical_shape)`
- `SubtypeFact(lhs_alias, rhs_alias)`
- `ObservedAccessFact(base, offset_bits, access_size_bits, access_kind)`

`ObservedAccessFact` 很重要。它用于确定哪些 DWARF 字段进入主 recall 分母。

# Canonicalization 规则

主准确率使用 canonical type shape：

- 忽略 typedef 名字，只保留 canonical shape。
- 结构体名字不要求一致，优先按 root binding、字段偏移和字段形状匹配。
- primitive 默认比较 bit width。signedness 如果能从 DWARF 和输出同时确定，可以单独计分。
- pointer 先比较 pointer-shaped，再比较 pointee canonical shape。
- array 比较 elem shape 和 stride。count 如果 binary 中不可观测，不作为必须匹配项。
- recursive type 比较 cycle shape，不要求递归变量名一致。
- function pointer 比较参数个数、参数 shape 和返回 shape。
- padding 不作为字段。未访问空洞可以作为 layout gap 记录，但不进入字段准确率。
- union 第一版只统计存在和大小，不做精细 field match。后续如果需要，单独设计 union 指标。

typedef/alias 单独算，不混进主 canonical shape 表。

# 可观测字段定义

一个 DWARF field 进入主 recall 分母，需要满足至少一个条件：

- binary 中存在对该字段 offset 的 load/store。
- 字段所在对象作为函数参数、返回值或全局对象被 NotDec 识别到。
- 库函数 summary 对该字段或包含它的对象产生约束。
- 指针运算、array stride 或嵌套对象重建能定位到该 offset。

如果字段只存在于 DWARF 中，但 stripped binary 没有访问，也没有任何约束能推到它，不进入主 recall。

补充指标可以报告：

- `all-DWARF-field recall`
- `unobserved-field count`
- `padding/gap bytes`

# 输出分组

每个 predicted fact 分为四类：

- `correct`：和 truth 匹配。
- `wrong`：找到对应 truth，但 shape 或 offset 不匹配。
- `missing`：truth 可观测，但输出没有。
- `unjudged`：输出事实找不到可靠 truth root，不能算 false positive。

论文主 precision 应只在 judged predictions 上计算，同时报告 `unjudged rate`。这样可以避免
DWARF 不完整或 root 对齐失败时错误惩罚工具输出。

# 表格和图

论文 Evaluation 至少准备这些表：

## Benchmark Table

列：

- project
- target path
- binary type
- stripped size
- function count
- debug source
- DWARF record count
- DWARF field count
- observed field count
- exported symbol count
- library dependency count

## Main Accuracy Table

列：

- target
- roots
- comparable roots
- coverage
- judged predictions
- unjudged predictions
- precision
- recall
- F1
- exact-shape match
- field-offset match
- field-type match

## Construct Breakdown Table

行：

- primitive
- pointer
- struct field
- array
- nested struct
- recursive struct
- function pointer
- callback
- global
- function parameter
- function return
- typedef alias

列：

- truth facts
- recovered facts
- matched facts
- precision
- recall
- notes

## Ablation Table

行：

- full
- no register elimination
- no summaries
- no subtype lattice
- no PtrStore/PtrLoad
- no array reconstruction
- no nested struct reconstruction
- no recursive folding

列：

- success targets
- timeout targets
- coverage
- precision
- recall
- F1
- solver time
- total time
- peak RSS

## Baseline Table

列：

- target
- NotDec
- Ghidra
- Retypd if available
- TRex if available
- Manta if available
- notes

如果某个 baseline 只能输出部分类型信息，不要强行全表比较。可以把它放进 partial baseline table。

## Performance Table

列：

- target
- binary size
- IR size
- functions recovered
- constraints
- type variables
- memory objects
- solver time
- reconstruction time
- total time
- peak RSS

## Case Study

建议写 2 到 3 个：

- 一个网络服务结构体恢复，比如 memcached/lighttpd/vsftpd 的核心 connection/request 结构。
- 一个 shared library typedef/callback 传播，比如 libuv handle/callback。
- 一个递归或嵌套结构恢复，比如 fortune 或 Redis 里的 list/tree-like 结构。

# 实现路线规划

本计划只规划实验，不立即实现。后续可以按下面阶段推进。

## 阶段 1: Truth inventory

目标：先知道 Bench2 里每个目标能提供多少可用真值。

工作：

- 按 `benchmark-targets.tsv` 读取 target、build-id、debug path。
- 校验 binary 和 debug file 是否匹配。
- 抽取 DWARF 中的 functions、globals、records、fields、typedefs、arrays、function pointers。
- 输出 per-target inventory CSV/JSON。
- 标记目标是否适合 pilot。

判断标准：

- 第一批 pilot 每个目标都能抽出 truth inventory。
- inventory 能区分 record count、field count、typedef count、array count、recursive record count。
- 能列出 top records，方便人工选择 case study。

## 阶段 2: Native relation schema

目标：定义 DWARF truth 和 NotDec output 的统一 relation schema。

工作：

- 先定 JSON schema，不急着写完整 evaluator。
- 把 DWARF truth 转成 relation facts。
- 把 `.htypes` 或后续 NotDec type output 转成 relation facts。
- 明确 canonicalization、typedef、array、recursive type 的比较规则。

判断标准：

- 一个 target 的 DWARF facts 可以稳定输出。
- 同一个 target 多次抽取结果稳定。
- schema 能表达至少 function param、global、record field、array、typedef。

## 阶段 3: Pilot evaluator

目标：在 1 到 2 个 native target 上跑通端到端比较。

工作：

- 先选 fortune 和 libuv 或 memcached。
- 从 stripped binary 跑 NotDec，输出类型结果。
- 从 debug file 抽 truth。
- 生成 compare report。
- 报告 coverage、field precision/recall、signature accuracy。

判断标准：

- report 能解释每个 missing/wrong fact。
- 能生成 markdown/CSV/JSON 三种报告。
- 能区分 judged 和 unjudged predictions。

## 阶段 4: Pilot benchmark

目标：跑 5 到 6 个第一批目标，确认指标和失败类型。

工作：

- 跑 pilot 表中的目标。
- 记录每个目标是否成功、耗时、内存、失败原因。
- 汇总主准确率表和 construct breakdown table。
- 根据失败原因调整 schema 或 root matching。

判断标准：

- 至少 3 个目标产生有效准确率结果。
- 至少 2 个目标能支持 case study。
- 指标没有明显自相矛盾，比如 precision 很高但 unjudged rate 极高。

## 阶段 5: Ablation

目标：证明每个关键模块的必要性。

工作：

- 固定 pilot 目标和 evaluator。
- 每次关闭一个模块。
- 对比 full pipeline 和 ablation。
- 对失败目标单独记录是 crash、timeout、empty output 还是 accuracy drop。

判断标准：

- 至少能说明 register elimination 和 summaries 对 coverage 的影响。
- 至少能说明 PtrStore/PtrLoad 对跨指针字段传播的影响。
- 至少能说明 array/nested/recursive reconstruction 对复杂类型的影响。

## 阶段 6: Baseline

目标：和外部工具做同口径比较。

工作：

- 先接 Ghidra，因为它最可控。
- 调研 Retypd、TRex、Manta artifact。
- 能跑的 baseline 转成同一 relation schema。
- 不能跑的 baseline 记录原因，不强行比较。

判断标准：

- Ghidra 至少能在 3 个 pilot target 上产出可比较类型 facts。
- baseline 输入不含 debug info。
- baseline table 能说明 NotDec 的优势和弱点，而不是只给总分。

## 阶段 7: Full benchmark and paper tables

目标：生成论文最终表格。

工作：

- 扩展到 Bench2 更大目标。
- 冻结 target 版本、工具版本和运行参数。
- 生成所有表格、图和 case study 材料。
- 把失败目标分类写进威胁和限制。

判断标准：

- 每个表能追溯到 raw JSON/CSV。
- 每个 benchmark 的 debug source、binary source 和 build-id 可复核。
- 论文中的主结论不依赖单个目标。

# 风险

## DWARF 不等于源码真相

DWARF 是最实用的 oracle，但它受编译器、优化、debug info 质量影响。特别是 local variable、
inlined function、optimized-out field 和 union 可能很难稳定比较。

处理：

- 主指标用可观测事实。
- local variable 只做补充。
- 记录 unsupported DWARF kind。

## NotDec output 和 DWARF root 对齐困难

函数名、地址、PIE relocation、global address、反编译内部名字都可能不一致。

处理：

- 先用 build-id、symbol table、DWARF low_pc/high_pc、global location 做 root matching。
- 对无法稳定匹配的 prediction 标 `unjudged`。
- report 中保留 root matching 证据。

## 未访问字段造成 recall 误导

源结构体可能有很多字段，但 binary 只访问其中几个。要求恢复所有字段不公平。

处理：

- 主 recall 只看 observed fields。
- all-DWARF-field recall 只作为补充。

## typedef 指标容易污染主结论

typedef 名字在编译后大多不可观测，而且不同头文件/库版本会产生 alias 差异。

处理：

- canonical shape 是主指标。
- typedef/subtype 单独报告。
- 用户 subtype lattice 的 truth 要明确来源。

## baseline 公平性

Ghidra、Retypd、TRex、Manta 的输入要求和输出粒度不同，可能不能直接同表比较。

处理：

- 统一转 relation schema。
- 不能判断的输出标 `unjudged`。
- 对只支持部分类型的 baseline 做 partial comparison。

## bin2llvm 和类型推理耦合

类型恢复失败可能来自 lifting、寄存器消除、CFG、summary、solver、重建任一阶段。

处理：

- performance report 分阶段记录。
- ablation 中区分 pipeline availability 和 accuracy。
- crash/timeout 单独统计，不混入 wrong type。

# 判断标准

第一版规划完成后的判断标准：

- 有一份清晰的 Evaluation Plan，能直接指导 evaluator 实现。
- 能解释为什么主实验选 native ELF、为什么 debug info 只做 oracle。
- 能解释为什么主指标比较 canonical type relation，而不是 C 字符串或 typedef 名。
- 能列出 Bench2 第一批 pilot target 和选择理由。
- 能列出论文需要的主要表格和图。
- 能列出 ablation、baseline 和 performance 的最小可行口径。

第一版实现完成后的判断标准：

- 至少 5 个 pilot target 有 truth inventory。
- 至少 3 个 pilot target 有 NotDec compare report。
- 至少 2 个 target 有可写进论文的 case study。
- 主 accuracy table、construct breakdown table、performance table 都能从 raw JSON/CSV 生成。
- 所有结果都能追溯到 Bench2 target、debug file build-id 和 NotDec commit。

# 当前建议

下一步先不要写论文 Evaluation 正文，也不要急着接所有 baseline。先做两个小闭环：

1. Bench2 truth inventory：确认每个 native target 的 DWARF 事实规模和复杂类型分布。
2. Native relation schema：把 DWARF 和 NotDec 输出都落到同一个 JSON facts 格式。

这两个闭环完成后，再开始写 evaluator 和 pilot benchmark。这样后续不管是加 typedef、
ablation，还是接 Ghidra baseline，都不会反复推翻指标口径。

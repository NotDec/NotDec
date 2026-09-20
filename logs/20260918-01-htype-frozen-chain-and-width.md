# HTypeBuilder 公平评估链路（冻结 UType）+ 字段宽度策略

日期：2026-09-18

## 用户目标

> 专心提升 HTypeBuilder 那一块的效果即可……后面多试试不同算法，看看有没有提升空间。

约束（硬性）：**算法不能使用调试信息**；调试信息只允许出现在打分阶段（GT/Eval2）。
之前离线数据集（Bench2/datasets/htype-layout）只能当“证据浏览器”，不能做算法决策
（实测与真实 Eval2 平均差 -0.0292，且输入是二次推导的、还用了 DWARF strict key）。

## 1. 公平链路：冻结 post-constraint 状态，只重放 UType -> HType

思路：约束求解（UType 的产生）与布局转换（UType -> HType，即 TypeBuilder）分开。
每个 case 只解一次约束并把结果存成 checkpoint；之后所有算法候选都从同一个 checkpoint
重放，输入 UType 逐字节相同，只有布局算法不同；打分仍然走完整真实链路
（microsub2_to_recstruct.py -> all.yaml -> RecStruct run-eval）。

链路（Bench2/scripts/htype_stage_ab.py）：

1. freeze（每 case 一次）：把 `<case>/runs/microsub2/*.stripped` 与 `.i64` 复制到
   `datasets/htype-layout/frozen/<case>/bin/`，运行
   `microsub2 --idb <bin> --type-recovery --emit-post-constraint-state <dir>/ckpt`；
2. replay（每个算法候选一次）：
   `microsub2 --idb <bin> --type-recovery --load-post-constraint-state <dir>/ckpt --htype-report ...`
   （环境变量切换算法）；
3. 打分：`convert(<gt>, lvars.json, htypes.txt)` -> `all.yaml` -> `run_eval(groundtruth_dir, results_dir)`。

freeze 一次约 2-20 s，replay 略快于 direct（省掉约束求解），因此可以全 coreutils（253 case）
反复跑多套算法。

### 验收：frozen replay 必须复现 direct 运行

11 个 coreutils case（O0/cat、O1/arch、O2/ls、O2/cp、Os/printf、O1/chmod、O2/chown、
O3/dd、Os/csplit、O1/comm、O2/od），`NOTDEC_BINARYSUB_THREADS=1`，`--variants direct,baseline
--repeat 2`：

- direct vs frozen replay 平均 ΔF1 = **+0.0000**（11 个里只有 O1/chmod 一个 repeat 差 0.0003）；
- 同臂重复跑（noise floor）：11 个 case max 0.0003，mean 0.0000。

结论：checkpoint 重放对最终 Eval2 指标是无损的（报告文本仍有 ~百行级别重命名/重编号差异，
属于同一 UType 图的编号波动，不影响 (offset,size) 字段集合）。

### 链路注意事项

- **checkpoint 绑定输入路径**：manifest 的 `ir_sha256` 是对 `renderModuleToString(M)` 求的，
  而模块里 `source_filename` 是 `--idb` 的**完整路径**。emit 与 replay 必须传同一个 `--idb`
  路径（复制到别处会 "frozen IR SHA-256 mismatch"）。
- **emit 与 replay 的 `--work-dir` 设置必须一致**：emit 不带 work-dir、replay 带 work-dir 时
  会报 `SCC identity mismatch at index 0`（emit/replay 都带或都不带都可以重放）。
  真实评估链路（run_dataset_flow.py / run_bench2_flow.py）**不带** `--work-dir`，
  本链路同样不带，保持一致。根因未定位，已记录为待查。
- `NOTDEC_BINARYSUB_THREADS` 影响报告文本但不影响 Eval2 指标（O0/wc 在 1/2/4/8 线程下
  F1 都是 0.5835）；A/B 统一用 1 线程降低噪声。
- `results/microsub2-*` 里的历史官方基线是 9/16 用**旧二进制**跑出来的，与当前二进制的
  direct 结果有偏差（181 个 case 平均 +0.0042，最大 +0.15）。A/B 必须用同一二进制现场跑，
  不能拿历史基线当对照。

## 2. 损失归因（真实 Eval2 的 eval2.md，25-31 个 case）

`Bench2/scripts/analyze_htype_losses.py` 解析 `eval2.md` 的每参数 GT/预测字段表，
把每个漏掉的 GT 字段和每个多出的预测字段分类：

- 命中 1471 / GT 6763 / 预测 4278；漏 5234，多 2692；
- 漏的字段里 **76.2% 是 `no-pred-near`**（该 offset 附近完全没有预测）：主要是“该参数压根没被
  恢复成结构体指针”（116 个参数，占漏检质量 15%）以及纯证据缺失；
- 漏的 18.0%、多的 43.5% 是 **`same-offset-diff-size`**（同 offset 宽度不对），
  其中最大的一类是预测成 `void*`(8B) 而 GT 是 1/2/4 字节字段：
  多出 (4,8,'void*') 415、(1,8,'void*') 132、(2,8,'void*') 61；
  漏掉 (4,8) 374、(1,8) 119、(2,8) 59；
- 20.2% 的预测字段落在**重复 offset** 上（union panel 展开的成员），这批字段大多是 extra，
  但 panel 收窄策略在真实 Eval2 上曾经是净负（见 logs/20260917-05），说明其中也有真命中。

## 3. 假设与实现：字段类型宽度必须与字段 range 一致

从 `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1` 的 trace（每次 convertStruct 的 raw field
range/access/utype）可以看到典型形态：

```
[4]  range=@106 accessed_bits=32 accessed_bytes=4 utype=Ptr<⊤, ⊥>       -> 现在输出 void* (8B)
[9]  range=@124 accessed_bits=8  accessed_bytes=1 utype=Ptr<uint:8, ⊥>  -> 现在输出 void* (8B)
[24] range=@68  accessed_bits=32 accessed_bytes=4 utype=Ptr<uint:32, ⊥> -> 现在输出 void* (8B)
```

即：**字段 range 来自“观测到的访问宽度”（指针 UType 取 psize），但成员类型来自 UType 转换**，
两者宽度不一致时（`Ptr<top,bottom>` → `void*`）声明出来的 HType 自带矛盾：字段 range 是 4B，
类型却是 8B 的指针。所有“按类型算大小”的消费者（Bench2 桥、Eval2 的 (offset,size)、
写进 IR 的 lvar 类型串）都会看到一个证据从未支持过的宽度。

实现（external/binarysub/src/TypeBuilder.cpp，仅实验、默认关闭）：

- `NOTDEC_TYPEBUILDER_FIELD_WIDTH=sync`：成员类型比 range **宽**时，改用宽度精确相等的
  无符号整数（1/2/4/8 字节）或 `i8[N]`；
- `=syncboth`：类型比 range **窄**时也按 range 加宽；
- 不设置（默认）：行为与 HEAD 完全一致；
- 位置：只在 `craftStruct()` 生成 `FieldDecl` 前统一处理，覆盖 convertStruct 与 union panel 两条路径；
- 回退：取消环境变量，或删除该代码块与唯一调用点。

## 3.5 实测：宽度一致性策略有大幅收益（2026-09-18 下午）

在冻结链路上跑真实 Eval2 A/B（`htype_stage_ab.py --tag-prefix ab`，每个 case 三臂
baseline / wsync(=只收窄) / wsyncboth(=收窄+放宽) 同一 checkpoint 重放）：

- **O0 全部 53 个 case：wsyncboth 平均 ΔF1 = +0.0852，53 胜 0 负**（最小 +0.0195，最大 +0.2138）；
- wsync（只收窄）几乎无效果（17 个 case 里只有 1 个差 0.0023）→ 收益来自"类型比字段 range 窄/宽度未知"的那一支；
- 34 个 case 的三指标对比：eval2 F1 0.3117→0.3967（P +0.104，R +0.073），
  **eval3 F1 0.0615→0.0419（R −0.019）**，eval4 基本不变。
- IR 仍通过 `opt -passes=verify`。

机制（trace + debug 打印）：字段的 byte range 来自**观测宽度**（`accessedPointeeSizeInBits`，
对交集取各 term 的最大值），而字段类型由 UType 转换得到；当 UType 是
`Ptr<..., psize=0>`（pointee 宽度未知，常见于"record ∩ 指针"里 primitive term 被过滤掉的分支）
或 `getVoidPtr()` 时，声明出来的类型宽度（8 字节的 void*，或 psize=0 的 DualPointer）与
range（1/2/4）不一致。下游（Bench2 桥、Eval2 的 (offset,size)、写进 IR 的 lvar 类型串）
都是**按类型算宽度**，于是看到错误宽度；把成员类型换成"宽度等于 range 的无符号整数"
（1/2/4/8 字节，其它宽度用 i8[N]）后，O0 上 F1 大涨。

eval3（嵌套指针层级）下降的原因：这些字段原本算作指针层级 1，换成整数后层级丢失。

### 变体对比与最终策略（47 个跨 O1-Os 的 case，真实 Eval2+Eval3）

| 变体 | 触发条件 | ΔEval2 F1 | ΔEval3 F1 |
| --- | --- | ---: | ---: |
| wsync (`sync`) | 类型比 range 宽 | ~0 | ~0 |
| wwiden / wwidend0 / wsyncboth | 类型比 range 窄或宽度未知 | +0.059 (31胜/5负) | **−0.048** (1胜/16负) |
| **wwiden0bare** | 宽度未知**且**类型不含 pointee/load/store 信息 | **+0.052 (31胜/5负)** | **+0.0002 (2胜/1负)** |

三个"宽"变体在 41 个 case 上数值完全一致 → 实际生效的只有"宽度未知（DualPointer 的
AccessSize < 8bit，即 psize=0）"这一支；收窄分支（类型比 range 宽）在这些 case 上没有影响。

`widen0bare`（最终推荐）额外跳过带 pointee/load/store 信息的成员类型：那些字段原本提供
指针层级（eval3 的嵌套路径），重写会丢结构；跳过后 eval3 的下降完全消失，eval2 收益保留
~87%（唯一明显少赚的是 O2/join +0.186 → +0.024）。

已知风险：个别 case 上 baseline 的"字节粒度 8 字节 hedge"是有效命中来源（O3/df：
144 个 (0,8),(1,8),… 字段命中 13 个 GT 字段），改成正确宽度后 eval2 掉 0.13
（`O3/df` 在 bare 下仍是 −0.1335），属于"基线的不一致布局偶然命中"。

### 全量结果（229 个 coreutils case，真实 Eval2，冻结输入链路）

`baseline vs wwiden0bare`（每 case 同一 checkpoint、同一二进制、同一线程数）：

| 指标 | baseline | wwiden0bare | Δ |
| --- | ---: | ---: | ---: |
| 平均 Eval2 F1 | 0.3013 | **0.3726** | **+0.0713** |
| 中位 ΔF1 | — | — | +0.0748 |
| 胜/负/平 | — | — | **202 / 13 / 12** |
| 平均 ΔEval3 F1 | — | — | −0.0000（6胜/4负） |

按优化级别：O0 +0.0872（105 例）、O1 +0.0750（54）、O2 +0.0425（32）、
O3 +0.0560（18）、Os +0.0342（18）。最大回退：O3/df −0.133、O3/stat −0.121、
O2/stat −0.106（都是 baseline 依赖"字节粒度 8 字节 hedge"的 case）。

测量噪声：同一 baseline 臂在两个批次里重跑 146 个 case，141 个完全相同，
平均差 −0.0005，最大 0.042（O0/join 0.535 vs 0.494）——单 case 的小差异有噪声，
但 202 胜 13 负、均值 +0.071 的系统性效应远超噪声。

### 2026-09-18 晚：改用"字段 extent"作为评估口径（用户建议，已实现并实测）

用户指出：既然 `void*`-类成员类型本身没有宽度信息，应该让**评估侧不要按类型名算宽度**，
而是用 TypeBuilder 实际恢复的字段字节范围（`FieldDecl.R.Size`）。

事实确认：HType 报告原先只有 `/* at offset: N */`，没有 extent，桥接脚本只能按类型名推宽度。
因此加了（默认关闭、环境变量开启）：

- `NOTDEC_HTYPE_REPORT_FIELD_BYTES=1`：报告里的字段注释变成
  `/* at offset: N, bytes: M */`（`HType.cpp` 的 `appendFieldComment`，record/union 都覆盖）；
- `microsub2_to_recstruct.py`：解析 `bytes:`，有则用它作为字段 size，否则回落到类型推导。

用同一批冻结 case、同一二进制重跑 `baseline`（未改算法）+ 新口径：

| 口径 | baseline 平均 Eval2 F1 | Δ（extent − type） |
| --- | ---: | ---: |
| 按类型名算宽度（旧） | 0.2991 | — |
| 按字段 extent（新） | **0.3864** | **+0.0873**（中位 +0.0863，197胜/12负，212 个 case） |

且在新口径下，之前那个 `widen0bare`(TypeBuilder 侧) 策略的增量为 **+0.0002（5胜/2负）**，
eval3 也是 ±0.0000 → 说明那 +0.07 的收益本质是"评估口径把 extent 丢了"，不是算法缺陷；
改口径后该策略对指标已无作用（是否保留仅取决于是否要求打印出的 decl 自洽）。

影响与注意：
- ConsGen / RecStruct 的 yaml 本来就带显式 size，不受影响，因此 microsub2 与它们的对比在新口径下更公平；
- 但历史 `results/microsub2-*`、RESULT.md 里的 microsub2 行需要用新口径重跑；
- 报告里仍会看到 `void* field; /* at offset: 0, bytes: 4 */` 这种"类型与 extent 不自洽"的声明，
  这只是可读性/下游 IR 类型串的问题，不再影响 Eval2 口径。

### 2026-09-18 定稿：报告默认带 extent，TypeBuilder 实验代码删除

用户决定：

1. **HType 报告默认输出 `/* at offset: N, bytes: M */`**（去掉 `NOTDEC_HTYPE_REPORT_FIELD_BYTES` 环境变量开关）：
   `HType.cpp` 的 `appendFieldComment` 对 record/union 字段都无条件追加 extent（`Field.R.Size > 0` 时）；
   解析方同步迁移：
   - `Bench2/scripts/microsub2_to_recstruct.py`：`bytes:` 优先作为字段 size，兼容"只有 offset"和"只有 bytes"两种旧/新写法；
   - `Bench2/scripts/collect_htype_layout_dataset.py`：`DECL_FIELD` 增加 `bytes` 捕获（离线证据浏览器用）。
   检查过没有别的解析方依赖旧格式：`binarysub-test.cpp` 只在注释里提到 "at offset"，NotDec `unittests` 无相关断言。
2. **删除 TypeBuilder 侧 255 行实验代码**（`external/binarysub/src/TypeBuilder.cpp` 已 `git checkout` 回 HEAD）：
   新口径下它只有 +0.0002 的噪声级增量，保留只会引入一个全局开关和额外维护面。
   `Field.WidthPolicy` 相关枚举/辅助函数全部随文件回退。

全量复核（228 个 coreutils case，默认格式、未改算法、冻结点重放）：

| 口径 | 平均 Eval2 F1 |
| --- | ---: |
| 旧：按类型名算宽度 | 0.3024 |
| **新（默认）：按字段 extent** | **0.3874** |

Δ = +0.0850（中位 +0.0847，208 胜 / 13 负 / 7 平），Eval3 平均 Δ = −0.0000。
按优化级别（新口径）：O0 0.3932（105 例）、O1 0.4220（54）、O2 0.3474（32）、
O3 0.3312（18）、Os 0.3778（19）。

单点：O0/arch 0.4124、O0/cat 0.4124、O3/df 0.0111、O2/stat 0.0324；IR 通过 `opt -passes=verify`。
注意 O3/df 在新口径下从 0.156 掉到 0.011 —— 说明新口径不是"普遍放水"：baseline 那批 `void*` 字段的
真实 extent 就是 1 字节（字节粒度 hedge），旧口径把它们当成 8 字节才"偶然"命中。

迁移检查：`binarysub-test.cpp` 只在注释里提到 "at offset"，NotDec `unittests` 无相关断言；
已存量的旧 `.htypes.txt`（含 `datasets/htype-layout/**`、`runs/microsub2/*.htypes.txt`）仍可解析
（`bytes` 缺省时回落到类型推导），所以历史结果不会被破坏，只是口径不同。
改动落在 NotDec 的 `external/binarysub`（microsub2 链路用的那份）；`binarysub-private/src` 是另一份
较旧的副本（7 月版本，microsub/cons_gen 用），未同步。

### 最终策略定义

`NOTDEC_TYPEBUILDER_FIELD_WIDTH=widen0bare`（默认不设置 = HEAD 行为）：

> 在 `craftStruct()` 生成 `FieldDecl` 前，若成员类型是"叶子形状"、
> **完全不携带 pointee / load / store 信息**（`void*`、`ptr<load=void,store=void,psize=0>`）、
> 且其可计算宽度为 0（宽度未知），同时字段 range > 0，则把成员类型换成宽度等于 range 的
> 无符号整数（1/2/4/8 字节，其它宽度为 `i8[N]`）。

理由：这类类型对"字段占多少字节"没有提供任何信息，字段 range 是唯一的宽度证据；
换掉之后 HType 声明与自身 range 自洽，下游（Bench2 桥、Eval2、lvar 类型串）也才看到真实宽度。

## 4. 待办

- [ ] 全 coreutils（253 case，178+ 已冻结）跑 `baseline / wsync / wsyncboth` 三臂 A/B，
      用真实 Eval2 统计平均 ΔF1、胜/负/平；
- [ ] 按结果决定是否把胜出的策略提升为正式开关/默认；
- [ ] 继续挖其它方向：no-pred-near 的“参数没恢复成结构体指针”、union panel 语义、
      重复 offset 字段的去重（曾失败，需要新证据）。

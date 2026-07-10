# 原始 Prompt

> 按照这个方式挺不错的，规划一下要怎么改进具体的代码。

# bin2llvm 未知外部函数签名两遍分析计划（已完成）

## 背景

当前 `NativeRegisterSummary` 遇到 declaration 或 indirect call 时直接使用完整 ABI
输入集合。对于没有 prototype 的外部函数，这会把 RDI、RSI、RDX 等寄存器全部标成
可能被读取，进而把调用它们的内部函数推成过宽的参数列表。

现有未知外部函数参数数量推断发生在 `FunctionBuilder` 已经构造 SummarySSA 之后：

```text
NativeRegisterSummary
-> buildInitialSignatureShapes
-> FunctionBuilder
-> collectSignatureCallArgs
-> refineUnknownExternalParamShapes
```

这个位置太晚。外部函数 shape 虽然被收窄，但前面得到的 `ReadEntry`、entry demand 和
内部函数 shape 不会重新计算。`FunctionBuilder` 还会修改 IR，因此不能先运行它收集参数，
再回头重跑 summary。

## 已确认的方向

改成固定两遍分析：

```text
基础 NativeRegisterSummary
  unknown external 输入参数按空集
  known external 使用 prototype

未知外部函数签名推断
  汇总所有 callsite 的明确参数设置
  合并用户 prototype

最终 NativeRegisterSummary
  使用推断后的 external prototype

SummarySSA 和签名重写
```

第一遍的目标是得到不受未知外部函数 ABI 最大输入集合污染的基础 `ReadEntry`。
第二遍复用现有 SCC、call transfer 和 top-down demand，自动处理 wrapper 参数转发及其
上层 caller，不另外实现一套 overlay worklist。

## 目标

1. unknown external 在第一遍 summary 中不读取任何 ABI 参数，但仍按 ABI 处理
   caller-saved clobber。
2. known external 和用户 JSON prototype 在两遍 summary 中都使用准确的参数信息。
3. 在不修改 IR 的情况下，第一遍 summary 同时收集 unknown external callsite 参数证据。
4. 将 callsite 证据汇总成临时 `NativeExternalPrototypeMap`，再跑第二遍 summary。
5. SummarySSA 只消费第二遍结果；后续阶段不再修改 external 参数数量。
6. 保留现有 warning 文件，报告推断结果、不一致 callsite 和完全无证据的 external。

## 技术路线

### 1. 让 RegisterSummary 接收 external prototype

修改：

- `include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h`
- `lib/passes/summary/NativeRegisterSummary.cpp`

在 `NativeRegisterSummaryOptions` 中增加：

- external prototype map。
- unknown external 输入处理策略。
- 是否收集 unknown external callsite 证据。

保留当前公开 API 的保守默认行为，避免单独调用 `runNativeRegisterSummary()` 的旧代码
静默改变语义。`runNativeRegisterSummarySSA()` 会显式选择新的 `NoInputs` 策略。

`Analyzer::transferCall()` 按以下顺序处理：

1. internal definition：继续使用 `Effects[callee]`。
2. known external：根据 prototype 只读取真实参数 slot。
3. unknown external：第一遍和未推断成功的第二遍都不读取 ABI 输入。
4. declaration 和 indirect call 的 caller-saved clobber 仍按 ABI 应用。

prototype 到 ABI integer/float slot 的映射必须与 `shapeForKnownExternal()` 共用同一套
规则，避免 summary 和最终 LLVM signature 对同一个 prototype 得到不同参数寄存器。

### 2. 第一遍 summary 不附加最终 metadata

在 `runNativeRegisterSummarySSA()` 中：

1. 先加载内置和用户 external prototype。
2. 完成 stack frame、canary 和 register pointer canonicalization。
3. 第一遍调用 `runNativeRegisterSummary()`：
   - `AttachMetadata=false`。
   - unknown external 输入策略为 `NoInputs`。
   - 开启 callsite 证据收集。

第一遍结果只用于 external 签名推断，不作为后续 SummarySSA 的最终 facts，避免留下
过期 metadata。

### 3. 在 RegisterSummary 内收集 callsite 证据

扩展 `NativeRegisterSummary` 返回值，增加 unknown external callsite 记录。每条记录至少
包含：

- caller 名字。
- callee 名字；indirect call 单独标记。
- 每个 ABI 参数 slot 的 index、register、offset 和 width。
- call 前该 slot 的来源分类。

来源只分为当前需要的几类：

```text
LocalDefinition   call 前由本函数明确写入
ForwardedEntry    仍然是函数入口值
Mixed             不同路径来源不一致
CallProduced      来自前一个 call 的返回或 clobber
Unknown           当前分析无法判断
```

不要把这些来源加入公开的 `ReadEntry/MayEntry/MayNonEntry` 抽象域。单独做一个轻量
callsite evidence 状态，避免改变现有 summary lattice 的含义。

证据收集应在 bottom-up effect 稳定后进行一次 CFG replay，使用最终 internal callee effect：

1. entry slot 初始为 `ForwardedEntry`。
2. 普通 register store 或覆盖完整 ABI slot 的 partial write 变为 `LocalDefinition`。
3. call 产生或 clobber 的 register 变为 `CallProduced`。
4. CFG join 合并成 `Mixed`。
5. 在 unknown external call 前保存当前 ABI input slot 状态。

第一版只把完整 slot 定义视为强证据。x86-64 低 32 位 GPR 写继续按现有规则视为完整
64 位定义；其他不完整 partial write 保持 `Mixed/Unknown`，不要提前扩大范围。

### 4. 汇总临时 external prototype

在 `NativeRegisterSummarySSA.cpp` 中新增一个只读聚合步骤，输入第一遍 callsite 证据和
已加载的 prototype map，输出：

- 合并后的临时 `NativeExternalPrototypeMap`。
- signature warnings。

规则：

1. 内置或用户 prototype 优先，不参与未知签名推断。
2. 每个 callsite 只统计从 arg0 开始连续的 `LocalDefinition` 前缀。
3. `ForwardedEntry`、`Mixed`、`CallProduced` 和 `Unknown` 不算强参数证据。
4. 同一 external 的最终推断参数数量取所有 callsite 强前缀的最大值。
5. callsite 前缀不一致时保留 `inconsistent_unknown_external_arity` warning。
6. 最大值大于零时生成临时 fixed-arity prototype，并保留
   `inferred_unknown_external_arity` warning。
7. 所有 callsite 都没有强证据时保持零参数假设，并新增
   `unresolved_unknown_external_signature` warning。

临时 prototype 只补参数数量，不猜参数类型。第一版继续按 ABI integer slot 生成未类型化
参数。返回值仍走现有 demanded return 逻辑，不在这个阶段提前猜测。

### 5. 第二遍 summary 生成最终 facts

用合并后的 prototype map 再调用一次 `runNativeRegisterSummary()`：

- `AttachMetadata=true`。
- known 和 inferred external 使用 prototype 参数。
- 未推断 external 仍按零参数处理。
- 不再收集 callsite 证据。

第二遍结果作为唯一的：

- `summaryFactsByFunction()` 输入。
- internal signature shape 输入。
- SummarySSA range planning 和 entry/exit demand 输入。

wrapper 场景不需要特殊逻辑。只要 unknown external 在其他 callsite 被推断出参数，第二遍
现有 `applyFunctionEffect()` 和 SCC fixpoint 就会把 `ReadEntry` 自动传播到 wrapper 和它的
caller。

### 6. 调整 SummarySSA 后置推断

当前 `refineUnknownExternalParamShapes()` 会在 `FunctionBuilder` 之后修改 external shape。
改造后它不能再改变参数数量，否则最终 shape 又会与第二遍 summary 不一致。

处理方式：

- 将参数数量推断主体移到两遍 summary 之间。
- `FunctionBuilder::collectSignatureCallArgs()` 继续负责为最终 call rewrite 找到实际参数值。
- 原 `refineUnknownExternalParamShapes()` 删除，或改成只做一致性审计：
  - 检查最终 binding 是否覆盖推断 shape。
  - 不修改 `SignatureShape::Params`。
  - 不一致时追加 warning。

`markSignatureCallArgStores()` 和 `rewriteSignatureShapes()` 只处理最终确定的 shape。

## 预计修改文件

- `include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h`
  - summary options。
  - unknown external callsite 证据结构。
- `lib/passes/summary/NativeRegisterSummary.cpp`
  - prototype-aware call effect。
  - unknown external 零输入策略。
  - callsite evidence CFG replay。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp`
  - 两遍 summary 调度。
  - external prototype 聚合。
  - 移除后置 shape 修改。
- `tests/native_register_summary_test.cpp`
  - 零输入和 prototype-aware call effect 单元测试。
- `tests/native_register_summary_ssa_test.cpp`
  - 两遍传播、callsite 聚合和 warning 回归测试。
- `external/NotDec-bin2llvm/ARCHITECTURE.md`
  - 实现完成后更新 SummarySSA 实际顺序。
- `external/NotDec-bin2llvm/docs/analysis/abstract-interpretation-register-summary.md`
  - 说明基础 summary 的 unknown external 零输入假设及第二遍修正。

## 测试与判断标准

### 单元测试

至少覆盖：

1. 单独调用 unknown external 不再让 caller 的所有 ABI input 都变成 `ReadEntry`。
2. known external prototype 只读取 prototype 对应的 integer/float 参数 slot。
3. 一个 unknown external 的多个 callsite 分别设置 1 个和 3 个参数，最终推断为 3，并输出
   不一致 warning。
4. 前一个 call 的 clobber 不计入下一次 unknown external 参数数量。
5. partial write 只有覆盖完整 ABI slot 时才算强证据。
6. external 在一个 callsite 被明确推断后，纯 wrapper 在第二遍 summary 中得到对应参数。
7. 用户 JSON prototype 优先于自动推断。
8. 后置 SummarySSA 不再改变 external 参数数量。
9. 最终 module 通过 LLVM verifier，不残留新增错误 helper。

### Fortune

使用当前同口径 fortune 命令分别记录：

- 最终函数参数数量变化。
- unknown external warning 数量。
- 剩余 register access 和 summary helper 数量。

## 风险

1. **零参数是假设，不是语义证明**
   - 完全没有强证据的透明 wrapper 仍可能被推成零参数。
   - 必须输出 unresolved warning，并允许用户 prototype 覆盖。
2. **callsite 来源分类过粗**
   - PHI、复杂 partial write 和 call return 可能被归为 Mixed/CallProduced。
   - 第一版宁可少推参数，不把弱证据当强证据。
3. **prototype slot 映射重复**
   - summary 和 signature rewrite 必须共用同一映射规则，否则会出现 summary 认为读 GPR，
     rewrite 却生成 XMM 参数的问题。
4. **后置逻辑再次收窄 shape**
   - 必须禁止 `FunctionBuilder` 后再修改参数数量。
5. **未推断 external 的寄存器 store 清理**
   - 当前路线明确选择零参数假设，相关 store 可能在后续被删除。
   - warning 文件必须记录这一假设；保守保留所有潜在 store 不在本次计划范围内。

## 不做什么

- 不实现通用 external signature overlay worklist。
- 不无限迭代 summary 和 signature inference。
- 不在第一版推理透明转发参数。
- 不从 unknown external 推断复杂 C 类型或结构体参数。
- 不改变 known prototype JSON 格式。
- 不重构完整 SummarySSA range SSA。

## 完成标准

1. pipeline 顺序固定为“基础 summary、external 签名推断、最终 summary、SummarySSA rewrite”。
2. unknown external 不再通过 ABI 最大输入集合污染第一遍 `ReadEntry`。
3. 已明确设置参数的 unknown external 能在第二遍正确影响 wrapper 和上层 caller。
4. external 参数数量在 SummarySSA rewrite 前已经确定，后面不再发生 shape 收窄。
5. 现有 unknown external arity、clobber、return 和 range 测试保持通过。
6. fortune IR 通过 LLVM 22 verifier，函数签名、warning 和 register residue 有记录。

## 实现记录

完成日期：2026-07-10。

### 代码改动

- `include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h:31-75`
  - 新增 `NativeRegisterUnknownExternalInputPolicy`、callsite 来源和 slot evidence 数据结构。
  - `NativeRegisterSummaryOptions` 可接收 external call shape，并控制是否收集 unknown external 证据。
- `lib/passes/summary/NativeRegisterSummary.cpp:1070-1074, 1259-1344, 1754-1849`
  - `Analyzer::run()` 在第一遍 summary 后收集 unknown external callsite。
  - `collectUnknownExternalCallsiteEvidence()` 复用 bottom-up 最后保存的稳定 block flow，不重复实现一套 summary 求解。
  - `applyUnknownExternalCallEffect()` 分开处理输入读取和 ABI clobber；`NoInputs` 不读取参数，但仍保留 caller-saved clobber。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:1398-1548, 5848-5906`
  - `buildExternalCallShapes()` 统一把 prototype 映射到 ABI register slot。
  - `inferUnknownExternalPrototypes()` 汇总所有 callsite 的连续 `LocalDefinition` 前缀，取最大 arity 并生成 warning。
  - `runNativeRegisterSummarySSA()` 固定执行“基础 summary、external 推断、最终 summary、SummarySSA rewrite”。
  - 删除旧的 SummarySSA 后置 external 参数数量收窄。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:2204-2290`
  - 修复多返回值引入后的 bit-demand 位宽断言；aggregate `extractvalue` 结果按标量结果宽度保守使用 full mask。
- `tests/native_register_summary_test.cpp:400-523`
  - 覆盖 unknown external 零输入、known prototype 精确读取和 callsite 来源分类。
- `tests/native_register_summary_ssa_test.cpp:3047-3438`
  - 覆盖多 callsite 最大 arity、JSON prototype 优先、call clobber、PHI clobber 和 binary clobber。
- `tests/native_register_summary_ssa_test.cpp:2653-2733`
  - 覆盖 aggregate return extract 和 register store，防止 bit-demand 位宽断言回归。
- `ARCHITECTURE.md:241-388`
  - 更新 native SummarySSA 的实际 pass 顺序和清理步骤。
- `docs/analysis/abstract-interpretation-register-summary.md:256-393`
  - 补充 external 输入/clobber 分离、两遍分析、证据分类和 callsite 聚合规则。

计划中的“稳定后重新 replay CFG”实现时改为直接保存并复用 bottom-up 最后一轮的稳定
block flow。这样证据仍来自稳定状态，也没有复制或重新实现 SummarySSA/CFG 求解逻辑。

### 验证

通过：

```bash
./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/fortune.native.ll \
  -o /tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/fortune.native.bc

llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/fortune.native.bc \
  -disable-output
```

fortune 产物：

- lifting IR：
  `/tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/fortune.native.ll`
- bitcode：
  `/tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/fortune.native.bc`
- warning：
  `/tmp/notdec-bin2llvm-fortune-two-pass-final-20260710/register-ssa-warnings.tsv`

结果：

- `recode_new_outer` 推断为 1 个参数。
- `recode_new_request` 没有强证据，保持 0 个参数并输出 unresolved warning。
- `recode_string` 的两个 callsite 分别得到 1 和 2，最终取 2，并输出 inconsistent warning。
- IR 中没有活跃的 `summary_return`、`summary_clobber`、匿名数字函数声明和负常量
  `inttoptr`；`llvm.ssub.with.overflow.i32` 有实际调用，不是无用声明。
- `undef` 只用于构造 aggregate return 的首个 `insertvalue`，随后两个字段都被覆盖。
- 仍残留 `@RDI` 和一个 `notdec.partial_read.i64.i32`。`main` 最终参数是 `RSI/R9`，
  但对照 `0x2820` 的 prologue，真实入口把 `EDI` 保存到 `EBP`、把 `RSI` 保存到
  `R12`。因此 fortune 还没有完整消除入口寄存器，`main` 参数恢复仍有既有问题；
  这不是本次 unknown external 两遍分析新增的回归。

### 评价

- 实现效果：8/10。unknown external 不再按 ABI 最大参数集合污染第一遍 summary，且
  callsite 推断结果能够进入第二遍 SummarySSA。
- 复杂度：7/10。新增的数据流来源状态较多，但复用了现有 bottom-up flow 和最终
  SummarySSA，没有复制求解器。
- 后期维护成本：7/10。prototype 到 ABI slot 的映射集中在 SummarySSA 调度层；后续主要
  风险是 callsite 来源分类和函数入口参数恢复需要继续补测试。

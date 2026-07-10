# 原始 Prompt

> 之前重构外部函数类型推测的时候，不是分了两阶段的register summary分析吗？第一阶段是保守分析，假设外部函数调用的参数为0，找出一定存在的参数。然后第二阶段前分析了外部函数的参数数量，再第三阶段重做 register summary分析。  这里也一样吧，在第一阶段，对于vararg的函数采取保守假设吧，都假设为没有任何额外参数，然后第二阶段前也类似分析一下，分callsite看有几个vararg参数，最后第二阶段重做register summary分析就准确了。详细规划一下，先不急着实现

# bin2llvm vararg callsite 两遍 RegisterSummary 计划

## 背景

当前 `runNativeRegisterSummarySSA()` 已经有两遍 `NativeRegisterSummary`：

```text
基础 RegisterSummary
-> 推断 unknown external 参数数量
-> 最终 RegisterSummary
-> SummarySSA 和函数签名重写
```

unknown external 在基础 summary 中按零输入处理，因此不会先把全部 ABI 参数寄存器污染成
`ReadEntry`。基础 summary 稳定后，再根据各 callsite 前明确写入的参数寄存器推断 prototype，
最后重跑 summary。

known vararg 还没有采用同样的处理。`buildExternalCallShapes()` 当前会把 vararg declaration
展开到全部整数 ABI 输入寄存器；`FunctionBuilder::callReadsRegister()` 和
`callArgStoreBindings()` 也会做相同展开。结果是即使某个 callsite 只传固定参数，RCX、R8、
R9 仍会被当作实参读取。

fortune 当前的直接表现是：

```llvm
define i64 @main(i64 %RSI.arg, i32 %RDI.arg, i64 %R9.arg)
```

`%R9.arg` 在函数体中没有使用。它来自 `__fprintf_chk` 等 vararg call 的全 ABI 输入假设，
不是源码中的真实参数。

## 核心判断

这里应复用现有两遍分析，不新增第三套 summary 算法：

```text
基础 RegisterSummary
  known fixed external：使用完整固定 prototype
  known vararg external：只读取固定参数，不读取任何额外参数
  unknown external：不读取参数
  所有 external call：仍正常应用 ABI clobber

callsite 参数推断
  unknown external：按 callee 汇总多个 callsite
  known vararg：每个 callsite 单独推断额外参数

最终 RegisterSummary
  使用 fixed prototype、unknown 推断结果和 vararg callsite 结果

SummarySSA / signature rewrite
  使用与最终 RegisterSummary 相同的 callsite 参数集合
```

用户所说的“第一阶段、第二阶段前推断、第三阶段重做”在实现上仍是两次
`NativeRegisterSummary`，中间插入一次外部调用参数推断。

## 目标

1. known vararg 在基础 summary 中只读取固定参数。
2. 基础 summary 稳定后，按 callsite 推断整数 vararg 参数数量。
3. 同一 vararg callee 的不同 callsite 可以保留不同参数数量，不能合并成最大值。
4. 最终 RegisterSummary、top-down demand、SummarySSA 和 call rewrite 使用同一份 callsite
   结果。
5. external declaration 仍保持“固定参数 + `...`”，只改变各 callsite 的实际参数列表。
6. 保持 unknown external 现有的按 callee 最大 arity 推断，不产生回归。

## 为什么 known vararg 必须按 callsite 保存

unknown external 没有 prototype，最终需要生成一个临时固定签名。同一 callee 只能选一个
参数数量，因此当前取多个 callsite 的最大连续参数前缀是合理的保守选择。

known vararg 不同。它本来就允许不同 callsite 使用不同数量的额外参数，例如：

```text
fprintf(stream, "ok")
fprintf(stream, "%s %d %p", text, count, pointer)
```

如果按 callee 取最大值，第一个调用仍会被迫读取后一个调用才有的 RCX、R8、R9。这里应：

- declaration 继续保存固定参数和 `VarArg=true`。
- 每个 callsite 单独保存推断出的额外参数 slot。
- 最终 summary 和重写都先查 callsite 结果，再回退到 callee 的固定 shape。

不同 callsite 的 vararg 数量不同是正常语义，不应输出“不一致签名” warning。

## 数据表示

### 1. callee 级 fixed shape

保留 `NativeExternalCallShapeMap`，但改变 known vararg 的构造规则：

- non-vararg：保存 prototype 的全部固定输入。
- vararg：也只保存 prototype 的固定输入。
- 不再在 `buildExternalCallShapes()` 中追加全部整数 ABI 输入。

这个 map 表示“无论哪个 callsite 都确定存在的输入”。

### 2. callsite 级 final shape

新增 callsite map，键使用原始 `llvm::CallBase *`，值保存最终要读取的输入 range：

```text
NativeExternalCallsiteShapeMap
  callsite -> fixed inputs + inferred vararg inputs
```

call 指针只在本次 pipeline 内使用。两遍 summary 和 SummarySSA 之间不会修改 call
指令，因此不需要给 IR 增加稳定 ID。真正重写 call 之后，这个 map 不再参与分析。

每个 callsite 结果还应保留：

- 固定参数数量。
- 推断出的额外整数参数数量。
- 额外参数对应的 ABI register range。
- 是否受到 `MaxArgs` 截断。

### 3. callsite evidence

现有 `NativeRegisterUnknownExternalCallsite` 只服务 unknown external。计划将它泛化成
external callsite evidence，并增加分类：

```text
UnknownExternal
KnownVarArg
```

每条 evidence 保存：

- 原始 call 指针。
- caller、callee 名称。
- 候选 ABI slot 及其 range。
- 每个 slot 在 call 前的来源：
  `LocalDefinition`、`ForwardedEntry`、`Mixed`、`CallProduced` 或 `Unknown`。
- known vararg 的固定参数数量、固定输入 slot 和 `MaxArgs`。

继续复用基础 summary 收敛后的 `StableFlows` 做一次 CFG replay。不要复制
`NativeRegisterSummary` 的求解逻辑，也不要在 `FunctionBuilder` 修改 IR 后再反推。

## 详细流程

### 1. 基础 RegisterSummary

基础 summary 的 call transfer 规则改为：

1. internal definition：继续使用 callee `FunctionEffect`。
2. known non-vararg：读取 prototype 对应的全部固定输入。
3. known vararg：只读取 prototype 对应的固定输入。
4. unknown external 和 indirect call：继续按 `NoInputs`。
5. 以上 external call 都正常应用 caller-saved clobber 和返回寄存器写入。

基础 summary 仍设置：

```text
AttachMetadata = false
RunTopDownDemand = false
CollectExternalCallsiteEvidence = true
```

这样得到的 `ReadEntry` 不受未知参数和 vararg tail 污染，同时 call 前的值来源已经包含
稳定的 internal callee effect。

### 2. evidence 收集

在稳定 CFG state 上同时收集两类调用：

- unknown external：候选 slot 仍是完整整数 ABI 输入序列。
- known vararg：候选 slot 只包含固定参数之后仍可用于整数 vararg 的 GPR slot。

known vararg 的候选起点不能直接使用 `shape.Params.size()`：

- 固定 `double` / `float` 参数使用 XMM，不消耗 GPR。
- 例如固定参数为 `(i64, double)` 时，固定输入是 `RDI + XMM0`，第一个整数 vararg
  应从 `RSI` 开始，而不是按两个固定参数跳到 `RDX`。

应从固定 `NativeSignatureSlot` 中找出已经占用的整数 ABI slot，再按
`abi.IntegerInputsInOrder` 生成剩余的连续 GPR 候选序列。

如果固定 prototype 无法完整映射到当前 ABI，则不推断该 callsite 的 vararg tail，并输出
warning。不能在固定参数位置不确定时继续猜额外参数。

### 3. known vararg callsite 推断

对每个 callsite 单独处理：

1. 固定参数始终保留。
2. 从第一个剩余 GPR slot 开始检查 evidence。
3. 只有连续的 `LocalDefinition` 算作明确的额外参数。
4. 遇到 `ForwardedEntry`、`Mixed`、`CallProduced` 或 `Unknown` 立即停止。
5. 不允许跳过中间 slot 后继续统计后面的寄存器。
6. `MaxArgs != 0` 时，额外参数数量最多为：

   ```text
   MaxArgs - FixedArgs
   ```

   `MaxArgs` 是源码层总参数数量，不是 GPR 数量。
7. 最终 callsite shape 为：

   ```text
   fixed input slots + 本 callsite 的连续 LocalDefinition tail
   ```

这里不在多个 callsite 之间取最大值。

### 4. unknown external 推断保持现状

unknown external 继续：

- 每个 callsite 统计从 arg0 开始的连续 `LocalDefinition` 前缀。
- 同一 callee 汇总所有 callsite。
- 最终临时 fixed arity 取最大值。
- callsite 不一致时输出 warning。

这部分仍生成临时 `NativeExternalPrototypeMap`。known vararg callsite 结果不写回 prototype
map，因为它不是 callee 级签名。

### 5. 最终 RegisterSummary

`NativeRegisterSummaryOptions` 同时接收：

- callee 级 `ExternalCallShapes`。
- callsite 级 `ExternalCallsiteShapes`。

增加统一查询函数，顺序固定为：

```text
callsite final shape
-> callee fixed shape
-> unknown external policy
```

`transferCall()` 和 top-down call demand 必须共用这个查询函数。不能只修 bottom-up
`ReadEntry`，否则 `ExitDemand` 仍可能把未推断的 vararg 寄存器重新标活。

最终 summary 设置：

```text
AttachMetadata = options.AttachMetadata
RunTopDownDemand = true
CollectExternalCallsiteEvidence = false
```

其结果仍是后续 `summaryFactsByFunction()` 和 internal signature shape 的唯一输入。

## SummarySSA 和签名重写接入

仅修 `NativeRegisterSummary` 不够。当前以下位置也会把 vararg 展开到全部 ABI 输入：

- `FunctionBuilder::callReadsRegister()`
- `FunctionBuilder::callArgStoreBindings()`
- 最终 call 参数构造

三处必须消费同一份 `ExternalCallsiteShapes`。

### 1. call liveness

`callReadsRegister()` 对 external call 使用统一 shape 查询：

- fixed 参数按 declaration shape。
- vararg tail 按本 callsite 的推断结果。
- 不再看到 `VarArg=true` 就遍历全部 ABI 输入。

### 2. 参数值绑定

`callArgStoreBindings()` 不再用：

```text
shape.Params.size() .. Abi.InputsInOrder.size()
```

构造 vararg 参数，而是显式遍历：

```text
fixed SignatureShape::Params
callsite inferred vararg slots
```

额外参数的 LLVM 参数 index 从 `FixedArgs` 开始连续编号。固定浮点参数仍计入
`FixedArgs`，但不会错误消耗 GPR slot。

### 3. call rewrite

external declaration 的 `SignatureShape` 仍只包含固定参数，并保持：

```text
VarArg = true
```

每个 call 的实际参数列表使用：

```text
固定参数 binding + 本 callsite 的 vararg binding
```

不能把推断出的 tail 追加到 declaration 的固定参数列表，否则同一 callee 的不同 callsite
又会被统一成一个错误 arity。

`SignatureRewriteState` 应保存 callsite shape，直到所有 call 参数 binding 和 call rewrite
完成。重写后的新 call 不需要继续保留旧 call 指针映射。

## 浮点 vararg 边界

第一版只推断整数 GPR vararg tail。

SysV x86-64 的整数和浮点参数使用两套独立寄存器序列。如果一个 vararg call 同时有额外
GPR 和 XMM 参数，仅从“哪些寄存器被写入”无法可靠恢复它们在源码参数列表中的交错顺序。

因此第一版不应直接把 XMM evidence 混进整数 tail：

- 固定 typed float 参数继续按 prototype 精确处理。
- 额外 XMM 定义只用于检测“不支持的混合 vararg”。
- 检测到额外 XMM evidence 时，该 callsite 只保留固定参数，并输出
  `unsupported_float_vararg_evidence` warning。
- 不根据 store 指令出现顺序猜源码参数顺序。

后续如要支持 `printf("%f", value)`，需要结合格式字符串、`AL` 中的 SSE 参数数量或更完整
的调用约定分析，另行规划。

## Warning 规则

成功的不同 callsite arity 不算冲突。warning 只报告真正不确定或被截断的情况：

- `incomplete_fixed_vararg_mapping`
  - 固定 prototype 无法完整映射到 ABI。
- `non_contiguous_vararg_evidence`
  - 中间 slot 不是 `LocalDefinition`，后面又出现明确本地定义。
- `vararg_evidence_truncated_by_max_args`
  - evidence 超过用户或内置 prototype 的 `MaxArgs`。
- `unsupported_float_vararg_evidence`
  - 存在额外 XMM 参数，但当前无法恢复参数交错顺序。

普通 fixed-only vararg call 不需要 warning；它本身是合法调用。

## 预计修改文件

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h`
  - 泛化 external callsite evidence。
  - 增加 callsite shape map 和 summary options。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp`
  - known vararg 基础零 tail。
  - 统一 callsite/callee shape 查询。
  - evidence 收集。
  - bottom-up 和 top-down 使用同一 call shape。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp`
  - known vararg callsite 推断。
  - 两遍 summary 调度。
  - `SignatureRewriteState` 保存 callsite shape。
  - call liveness、binding 和 rewrite 使用明确 tail。
- `external/NotDec-bin2llvm/tests/native_register_summary_test.cpp`
  - 基础 summary 和最终 callsite shape 测试。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp`
  - 多 callsite、bounded vararg、typed fixed 参数和 fortune 场景测试。
- `external/NotDec-bin2llvm/ARCHITECTURE.md`
  - 实现完成后补充 known vararg 的 callsite 推断顺序。
- `external/NotDec-bin2llvm/docs/analysis/abstract-interpretation-register-summary.md`
  - 实现完成后说明基础 summary 的 fixed-only vararg 假设。

## 测试计划

### RegisterSummary 单元测试

1. known vararg 的基础 shape 只读取 fixed slots。
2. fixed-only call 不再让 RCX、R8、R9 变成 `ReadEntry`。
3. final callsite shape 只读取推断出的连续 tail。
4. top-down demand 与 bottom-up 使用相同 callsite shape。
5. clobber 仍正常写 caller-saved 和返回寄存器。
6. unknown external 原有零输入和最大 arity 推断保持通过。

现有 `testKnownVarArgExternalKeepsFullInputDemand()` 的预期需要反转或拆分。全 ABI input
demand 正是本次要删除的行为。

### SummarySSA 单元测试

1. 同一 vararg callee 有两个 callsite：
   - 一个只有固定参数。
   - 一个有三个额外整数参数。
   - 最终两个 call 分别保留各自实际 arity。
2. `__fprintf_chk` 只设置 RDI、RSI、RDX：
   - 不读取 RCX、R8、R9。
3. `__sprintf_chk` 明确设置到 R9：
   - 保留该 callsite 的全部实际参数。
4. `open`、`prctl` 等 bounded vararg：
   - 最终总参数数不超过 `MaxArgs`。
5. 固定参数为 `(i64, double, ...)`：
   - 固定输入为 RDI 和 XMM0。
   - 第一个整数 vararg 从 RSI 读取。
   - LLVM 参数 index 位于两个固定参数之后。
6. tail 中出现 clobber、PHI 或二元表达式依赖 clobber：
   - 在该 slot 停止，不把后续寄存器当参数。
7. declaration 仍是 fixed params + vararg function type。
8. 最终 module 通过 LLVM 22 verifier。

### Fortune

重跑 fortune 后检查：

1. `main` 不再出现未使用的 `%R9.arg`。
2. `RDI`、`RSI` 等真实参数不被误删。
3. `__fprintf_chk` 等 call 的实际参数数量符合 callsite。
4. 不新增 raw register load/store、`summary_return` 或 `summary_clobber` 残留。
5. warning 文件只报告真正不确定的 vararg 场景。

`RSI -> R12` 等与 zero-demand 或 callee-saved 相关的问题不属于本计划，验证时单独记录，
不要混入 vararg 修复。

## 风险

1. **callsite shape 与 callee shape 混用**
   - 如果某处仍只按 callee 查询，较大 callsite 会继续污染较小 callsite。
   - 判断标准是 bottom-up、top-down、SummarySSA 和 rewrite 全部走统一查询。
2. **固定浮点参数导致 GPR 起点错误**
   - 不能用固定参数总数直接索引 GPR。
   - 必须按实际 fixed slot 占用情况计算整数 tail。
3. **`MaxArgs` 含义用错**
   - 它限制源码参数总数，不是 ABI 寄存器数量。
4. **混合整数和浮点 vararg 顺序不可知**
   - 第一版宁可 warning 并少推，不根据寄存器写入顺序猜参数顺序。
5. **evidence 指针生命周期**
   - callsite map 只能跨越两遍只读分析，不能在 IR 重写后继续使用旧指针。
6. **只修 summary，未修 rewrite**
   - 会出现函数参数消失，但 call 仍读取全部 ABI store 的不一致状态。

## 不做什么

- 不新增第三套 RegisterSummary 或复制现有求解器。
- 不把 known vararg 的不同 callsite 合并成最大参数数量。
- 不修改 external prototype JSON 格式。
- 不把推断出的 vararg tail 固化成 declaration 的 fixed params。
- 不在本次实现通用浮点 vararg 参数顺序恢复。
- 不修改与 vararg 无关的 stack frame、callee-saved 或 zero-demand 逻辑。

## 判断标准

1. 基础 summary 对 known vararg 只读取固定参数。
2. known vararg 额外参数在两遍 summary 之间按 callsite 推断。
3. 最终 RegisterSummary 和 SummarySSA 使用同一 callsite shape。
4. 同一 callee 的不同 callsite 可以生成不同实际参数数量。
5. bounded vararg 严格遵守 `MaxArgs`。
6. fortune 的无用 `%R9.arg` 消失，真实参数和调用语义不退化。
7. 原有 unknown external 两遍分析、单元测试和 LLVM verifier 保持通过。

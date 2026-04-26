# 类型推理可解释性初步规划

## 背景

当前 `tr-level=2/3` 的类型恢复链路，已经能在 `NOTDEC_TYPE_RECOVERY_DEBUG_DIR/ValueTypes.txt` 中输出最终的：

- `IR value / ExtValuePtr -> binarysub UType`

但这个输出只有“终态”，中间缺少如下关键信息：

- 某个 `llvm::Value` 最初对应到哪个 `SimpleType`
- 某条 subtype / record / ptr-load / ptr-store 约束是谁产生的
- `TypeSimplifier` 从 `SimpleType` 到 `CompactType`，再到 `UType` 的各步合并和简化依据
- 结构体成员路径、`load`/`store` 访问、指针加法偏移，最后为何表现成某个 field path

如果这些映射关系能保留下来，用户在看到最终类型结果后，就可以反查：

- “为什么这个值被推成指针/函数/结构体”
- “为什么这个结构体会有 offset=4 的字段”
- “为什么这个字段是 load-side/store-side”
- “这个成员路径对应的是哪条 IR 指令或哪组约束”

## 当前实现中的关键挂点

### 1. NotDec 侧的最早稳定映射

`src/TypeRecovery/mlsub/MLsubGenerator.cpp` / `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

- `ConstraintsGenerator::V2N`
  - 当前保存 `ExtValuePtr -> SimpleType`
  - 这是整个链路里最早、最稳定的“IR 值 -> binarysub 节点”映射
- `ConstraintsGenerator::ValueTypes`
  - 当前保存 `ExtValuePtr -> HType*`
  - 这是最终给后端和调试展示使用的类型结果

### 2. 约束生成阶段

`MLsubVisitor` 已经在以下位置显式生成关键约束：

- `visitLoadInst`
  - `PtrVal <: make_ptr_load(RetVal, BitSize)`
- `visitStoreInst`
  - `PtrVal <: make_ptr_store(StoreVal, BitSize)`
- `visitCallBase`
  - 构造函数类型 `make_function(Args, Ret)`
- `convertSimpleType(ConstantAddr)`
  - 常量地址映射到 `MemoryType` 上的 record field

这意味着：

- `load` / `store` 来源天然可追踪
- field 来源也可以部分追踪
- 但目前 subtype 约束本身没有 provenance 记录

### 3. 指针加法 / 访问路径阶段

当前和结构体访问路径最相关的代码在：

- `src/TypeRecovery/mlsub/PNDiff.cpp`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

现状：

- `matchOffsetRange()` 已经能把加法/乘法/shl 链提取为 `OffsetRange`
- `PNIGraph::eraseConstraint()` 里本来预留了把 `ptr + offset` 转成 pointer-access 的入口
- 但 `ConstraintsGenerator::setAsPtrAdd(...)` 仍是空实现
- `visitGetElementPtrInst()` 目前基本只是告警，不是真正的 explainable path 入口

结论：

- 如果要把“某次指针加法”精确映射成“结构体成员路径”，需要先补齐 ptr-add / GEP 到 field-path 的正式建模
- 这件事既是可解释性的重点，也是当前链路的明显缺口

### 4. binarysub 内部简化阶段

`external/binarysub/src/binarysub.cpp`

当前 `TypeSimplifier` 的主要阶段是：

1. `canonicalizeType(SimpleType, pol)`
   - 把 `SimpleType` 展开成 `CompactType`
2. `analyzeOccurrences(CompactType)`
   - 统计变量/primitive 共现关系
3. `simplifyType(CompactType)`
   - 做变量简化和结构合并
4. `coalesceCompactType(CompactType)`
   - 转换为最终 `UType`

这四步本身就天然构成了一条 explainability pipeline，但当前没有对外暴露 provenance。

### 5. 最终结构体呈现阶段

`src/TypeRecovery/mlsub/TypeBuilder.cpp`

- `URecordType.fields` 中的字符串会被解析成 `OffsetRange`
- `UPointerType.load/store` 会被转成 dual-pointer 语义
- `craftStruct()` 最终产出 `RecordDecl::FieldDecl`

因此：

- “UType 里的 field string / ptr load-store”
- “HType / RecordDecl 里的最终字段”

这之间也可以补一层映射，帮助用户从最终结构体字段反查到 UType 节点和原始 IR。

## 目标

建议把“可解释性”拆成两个层次：

### A. Debug/诊断层

目标是让开发者和高级用户能回答：

- 某个最终类型结果来自哪些中间节点
- 某个字段/成员路径来自哪些 IR 指令
- 某个 union/intersection/recursive 结果是在哪一步形成的

这一层优先输出到 `NOTDEC_TYPE_RECOVERY_DEBUG_DIR`

### B. 用户展示层

目标是让最终 UI / CLI / 日志能回答：

- “为什么这里出现了这个成员”
- “为什么这里被判定为 load/store 指针”
- “这个字段最可能对应哪些 IR 访问”

这一层可以基于 A 层的 sidecar trace 二次整理，不必一开始就侵入代码生成逻辑

## 建议设计方向

### 一、先引入 sidecar provenance，而不是一开始改动所有核心类型

第一阶段建议不要急着把 provenance 塞进：

- `SimpleType`
- `CompactType`
- `UType`

原因：

- 这些结构已经参与求解、哈希比较、递归检测和合并
- 直接把 provenance 内嵌进去，容易破坏等价性、缓存键和性能

更稳妥的路线是：

- 保持核心类型结构尽量不变
- 额外维护一套 `TraceContext / ProvenanceStore`
- 用稳定 ID 把外部 provenance 和内部类型节点关联起来

也就是：

- 核心对象负责“求解”
- sidecar 对象负责“解释”

### 二、把 provenance 的实体拆成四类

建议至少维护下面四类实体：

### 1. 源实体 `SourceAnchor`

表示“解释链”的起点，候选字段：

- `kind`
  - `llvm-value`
  - `instruction`
  - `operand`
  - `return-value`
  - `constant-addr`
- `function`
- `bb`
- `inst opcode`
- `inst text`
- `operand index`
- `ext-value string`

这里不要求一开始就做稳定跨版本 ID，先保证一次运行内可定位即可。

### 2. 约束实体 `ConstraintEvent`

表示“谁产生了什么约束”，建议记录：

- `id`
- `kind`
  - `subtype`
  - `record-field`
  - `ptr-load`
  - `ptr-store`
  - `function`
  - `ptr-add`
  - `merge/remap`
- `lhs simple type`
- `rhs simple type`
- `source anchors`
- `note`

其中：

- `visitLoadInst` / `visitStoreInst` / `visitCallBase` / `convertSimpleType(ConstantAddr)` 都应走统一记录接口
- 不要只调用 `binarysub::constrain(lhs, rhs, cache)`，而是在 NotDec 侧包一层 trace 记录

### 3. 简化实体 `SimplifyEvent`

表示 `TypeSimplifier` 各阶段的中间结果与原因，建议至少覆盖：

- `canonicalize`
  - `PolarVar -> CompactTypeRoot`
- `occurrence`
  - 某个变量在何处共现
- `simplify`
  - 某个 `CompactType` 被折叠/裁剪/归一化的原因
- `coalesce`
  - `CompactType -> UType`

这层不用一开始追求“每次 merge 都有完美证明”，但至少要能回答：

- 最终某个 `UType` 对应哪些 `PolarVar`
- 这些 `PolarVar` 最早来自哪些 `SimpleType`

### 4. 呈现实体 `PresentationBinding`

把最终对用户可见的对象重新绑定回前面的 trace：

- `ExtValuePtr -> HType`
- `UType field path -> RecordDecl::FieldDecl`
- `load/store side -> DualPointerType`
- `struct member offset/path -> source anchors`

### 三、优先维护三条核心反查链

### 链 1: `IR Value -> SimpleType -> PolarVar -> UType`

这是最基础的“一个值为什么是这个类型”链。

实现建议：

- 在 `ConstraintsGenerator::createNode/addRemapType` 中记录 `ExtValuePtr -> SimpleTypeId`
- 在 `genTypes()` 中记录：
  - `SimpleTypeId + polarity -> PolarVar`
  - `PolarVar -> UTypeId`

这样最终 `ValueTypes.txt` 就能升级成：

- 终态类型
- 参与求解的极性
- 对应的 `SimpleType` / `PolarVar`
- 中间 simplification 根节点

### 链 2: `IR load/store -> ptr-load/ptr-store -> field path`

这是解释结构体成员和指针语义的核心链。

实现建议：

- `visitLoadInst`
  - 记录 `load inst -> ptr-load constraint`
- `visitStoreInst`
  - 记录 `store inst -> ptr-store constraint`
- 在 `TypeBuilder::convertPointer/convertFieldType` 中，把：
  - `UPointerType.load`
  - `UPointerType.store`
  - `URecordType.fields`
  转成 `PresentationBinding`

这样用户看到某个字段时，可以回答：

- 这个字段是从读出来的，还是写进去的
- 它对应的是直接 load/store，还是某个 record field

### 链 3: `ptr-add / gep -> OffsetRange -> record field path`

这是解释“为什么这里会出现结构体成员”的关键链。

实现建议：

1. 先把 `setAsPtrAdd()` 真正实现起来
2. 对 `matchOffsetRange()` 的结果建立 trace
3. 给 `OffsetRange::str()` 的结果增加 sidecar 绑定
4. 在 `TypeBuilder` 把 field string 解析回 `OffsetRange` 时，重新绑定到源 IR

当前阶段需要明确一点：

- 如果 `ptr-add / gep` 仍然只是 TODO，那么“结构体成员来源解释”只能做到部分可用
- 所以这条链应视为 explainability 的核心增量，不只是日志增强

### 四、建议的数据结构

下面是一种偏保守、适合先落地的做法。

### 1. NotDec 侧新增 `TypeRecoveryTrace`

建议放在 `include/notdec/TypeRecovery/mlsub/` 和 `src/TypeRecovery/mlsub/`

候选内容：

- `DenseMap<ExtValuePtr, TraceSourceId>`
- `DenseMap<SimpleType, SimpleTypeTraceId>`
- `DenseMap<binarysub::PolarVar, PolarVarTraceId>`
- `DenseMap<const binarysub::CompactType*, CompactTypeTraceId>`
- `DenseMap<const binarysub::UType*, UTypeTraceId>`
- `std::vector<ConstraintEvent>`
- `std::vector<SimplifyEvent>`
- `std::vector<PresentationBinding>`

注意：

- `CompactType` / `UType` 现阶段可能主要靠指针身份绑定
- 这意味着 trace 生命周期必须限定在一次求解和一次输出流程中

### 2. binarysub 侧可选增加 observer 接口

如果 sidecar 仅靠外部包装不够，建议在 `TypeSimplifier` 上增加一个可选 observer：

- `TypeSimplifierObserver* Obs = nullptr`

暴露的回调可以很少，先控制在：

- `onCanonicalized(PolarVar, CompactTypePtr)`
- `onOccurrence(PolarVar, OccurrenceData)`
- `onSimplified(before, after, reason)`
- `onCoalesced(CompactTypePtr, UTypePtr)`

这样可以避免直接把 provenance 写死在 `binarysub` 核心类型里。

### 五、日志与输出格式建议

建议不要把所有信息继续塞进单个 `ValueTypes.txt`，而是拆成多个文件：

- `ValueTypes.txt`
  - 保留人类可读摘要
- `trace-values.json`
  - `ExtValuePtr -> SimpleType / PolarVar / UType`
- `trace-constraints.jsonl`
  - 每条约束一行，便于 grep / 追加输出
- `trace-simplify.json`
  - `canonicalize / occurrence / simplify / coalesce`
- `trace-fields.json`
  - `OffsetRange / field path / load-store / source instructions`

如果需要快速落地，可以先只做两层：

1. 文本摘要
2. JSON 明细

### 六、推荐实施顺序

### 阶段 1: 基础 provenance

目标：

- 打通 `ExtValuePtr -> SimpleType -> UType`
- 给已有 `load/store/call/constant-addr` 约束打上 source

落点：

- `ConstraintsGenerator::createNode`
- `ConstraintsGenerator::addSubtype`
- `ConstraintsGenerator::addRemapType`
- `ConstraintsGenerator::genTypes`

预期收益：

- 可以先回答“这个值为什么是这个最终类型”

### 阶段 2: TypeSimplifier explainability

目标：

- 暴露 `PolarVar -> CompactType -> UType`
- 给 occurrence/simplify 增加原因记录

落点：

- `external/binarysub/include/binarysub/binarysub.h`
- `external/binarysub/src/binarysub.cpp`

预期收益：

- 可以回答“这个 union/intersection/recursive 结果是怎么形成的”

### 阶段 3: 结构体路径 explainability

目标：

- 打通 `ptr-add / gep -> OffsetRange -> field path`
- 让最终成员字段可反查到 IR

落点：

- `src/TypeRecovery/mlsub/PNDiff.cpp`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`

预期收益：

- 可以回答“为什么这里会有 offset=4 的字段”

### 阶段 4: 面向用户的展示整理

目标：

- 在 debug 输出之外，增加更直接的 explain 接口

候选形式：

- CLI `--type-trace`
- debug dir 下按值/字段生成 explain 文件
- 后续网页或 IDE 展示

### 七、当前最值得优先处理的点

从“投入/收益”比来看，建议先做下面两件事：

### 1. 补全约束 provenance

因为这个改动最局部、最稳，且马上就能提升：

- load/store 的来源追踪
- call 参数/返回值来源追踪
- 常量地址对应 memory field 的来源追踪

### 2. 设计并补齐 ptr-add/GEP 到 field path 的正式映射

因为用户最关心的“为什么结构体里有这个成员”，本质上就依赖这条链。

如果这部分不补：

- explainability 只能解释“最终字段长什么样”
- 但很难解释“字段为何存在”

### 八、主要风险与未决问题

### 1. 指针身份是否足够稳定

- `SimpleType` / `CompactType` / `UType` 目前大量依赖指针身份
- 如果后续做跨阶段缓存或序列化，可能需要显式 ID

### 2. 简化步骤的“原因”可能不是单一来源

一个最终 `UType` 可能来自多条 bounds 和多次 merge：

- 是否要保留“完整原因集”
- 还是只保留“代表性原因”

这需要在可读性和体积之间权衡。

### 3. field string 目前既是语义载体，也是显示载体

`URecordType.fields` 里的字符串同时承担：

- record key
- offset/path 编码

长期看最好把“显示名”和“语义路径”拆开，否则 provenance 只能依赖字符串反解析。

### 4. 子模块边界

`TypeSimplifier` 位于 `external/binarysub`

这意味着：

- 如果 explainability 需要深入到 canonicalize/simplify/coalesce
- 很可能必须在子模块里增加 observer 或 trace hook

主仓库侧单独包裹，未必能拿到足够细的原因信息。

### 九、建议的首个最小版本

如果只做一个最小但有用的 MVP，建议范围如下：

1. 在 NotDec 侧记录每条约束的来源指令
2. 输出 `ExtValuePtr -> SimpleType -> PolarVar -> UType`
3. 单独输出 `load/store` 的来源映射
4. 对 `URecordType.fields` 保留 `field path -> source constraints` 的 sidecar

这个版本还不能完美解释所有 simplify 决策，但已经能覆盖：

- 某个值为什么是指针
- 某个字段为什么存在
- 某个 load/store 为什么落到这个成员路径

### 十、后续实现建议

可以按下面顺序推进：

1. 先在主仓库加 `TypeRecoveryTrace`，不改 `binarysub` 数据结构
2. 给约束生成入口统一加 trace API
3. 先把 debug dir 输出做出来，再决定是否需要 CLI/UI 展示
4. 等第一轮输出足够有用之后，再决定是否向 `binarysub` 子模块引入 observer

## 结论

这件事的核心不是“多打印一点日志”，而是把当前隐式存在的映射链显式化：

- `IR/ExtValuePtr`
- `SimpleType / subtype constraints`
- `CompactType / occurrence / simplify`
- `UType / HType / RecordDecl field`

其中最关键的两条主线是：

- `Value -> 类型`
- `指针访问 -> 结构体路径`

建议先以 sidecar provenance 的方式落地，优先做：

- 约束来源记录
- `Value -> SimpleType -> UType` 映射
- ptr-add/GEP 到 field path 的正式追踪入口

这样能在不大幅扰动当前推理核心的前提下，逐步把整个类型推理过程变得可解释。

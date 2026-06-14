# EVM 高层类型统一识别 Pass 规划

## 原始 prompt

MappingMatcher是什么，是storage吗，那一块单独处理，这里暂时不用管。把当前的规划写到一个logs/文件里面。特别开头要强调一下之前的考量：calldata、memory已经完全统一处理了，IR形状都是LLVM的load store了，所以统一处理是完全没问题的。而且也不要搞什么单独识别，单独rewrite在不同的pass，就按照类型处理，而不是按什么memory，calldata单独分

## 背景

当前 EVM 路线已经把 calldata 和 memory 访问基本统一到 LLVM IR 形状上：`evm_calldataload`、`evm_calldatacopy`、EVM memory access 等低层 helper 被改写成普通 `load`、`store`、`llvm.memcpy`，并用 pointer base、offset、bounds marker、checked alias 等信息保留来源和约束。

因此后续高层类型识别不应该再按 calldata、memory 分别写一套 pass，也不应该让不同 pass 各自识别、各自 rewrite 同一种类型。现在更合适的方向是：先把低层访问都规范成统一的 LLVM memory 访问，再按高层类型形状统一识别，例如 dynamic bytes、dynamic array、ABI tuple、struct、return/revert payload、event payload、external call payload。

也就是说，分类维度应该是“这是什么类型/布局”，不是“它来自 calldata 还是 memory”。来源仍然可以作为对象属性参与判断，但不应该变成 pass 边界。

Storage/mapping 这一块暂时不放进本规划。storage slot、mapping hash、storage array、storage bytes/string 的规则和 calldata/memory 的 LLVM load/store 对象不同，继续按 storage 语义路线单独处理。

## 目标

新增一个统一的高层类型识别 pass，暂名：

```text
EvmHighLevelTypePass
```

它的职责是消费已经统一后的 LLVM IR、HType、metadata 和 marker，把函数中的对象按高层类型归类。它不负责重新把 calldata 或 memory helper rewrite 成 LLVM IR，也不负责 storage/mapping。

这个 pass 应该识别和记录：

- ABI tuple/head-tail 布局。
- dynamic bytes/string。
- dynamic array。
- static array。
- struct/tuple-like object。
- return payload。
- revert/custom error payload。
- event data payload。
- external call input/output payload。

这些类型可以出现在 calldata、memory、returndata 或临时 ABI buffer 中。pass 内部 matcher 按类型拆，不按来源拆。

## 基本设计

先建立一个统一的对象视图，而不是直接在 matcher 里反复扫 IR：

```text
Object
  base value
  source/provenance
  observed loads/stores/memcpy
  bounds/min-size facts
  consumers/producers
  current HType
```

`source/provenance` 只表达对象来自哪里，例如 calldata、memory、returndata、return buffer、revert buffer、event buffer、external call input buffer。它用于选择 layout 细节，但不是 pass 拆分依据。

高层 matcher 只面向这个对象视图：

- `AbiTupleMatcher`：识别 head/tail、offset field、tail object。
- `DynamicBytesMatcher`：识别 length + payload、copy range、padding。
- `DynamicArrayMatcher`：识别 length、element access、element size。
- `StructMatcher`：识别固定 offset field 集合。
- `PayloadMatcher`：识别 return/revert/event/external call buffer 的 ABI payload。

这里不引入 `CalldataBytesMatcher`、`MemoryBytesMatcher` 这种按来源命名的 matcher。同一个 dynamic bytes 规则应该能处理 calldata tail、memory bytes、return data payload 等，只在 layout rule 上根据 provenance 做少量分支。

## 和现有 Pass 的关系

现有 pass 继续负责低层规范化和局部证据：

- `MemoryBufferRewritePass`：把 EVM memory 访问整理成 LLVM memory 访问。
- `EvmCalldataAccessPass`：把 calldata 访问整理成 LLVM load/memcpy，并提供 min-size checked alias。
- `CheckedBoundsPass`：识别 panic bounds 和 arithmetic guard，提供 bounds 证据。
- ABI return/revert/event/external call 相关 pass：短期仍可保留现有 marker，但后续应逐步改成消费统一高层类型结果。

`EvmHighLevelTypePass` 不应该再去匹配旧的 `evm_calldataload` / `evm_mload` 形状。它只看规范化后的 LLVM IR 和已有证据。这样可以避免每个语义 pass 都重新猜 calldata/memory 布局。

建议位置：

```text
MemoryBufferRewritePass
EvmCalldataAccessPass
CheckedBoundsPass
TypeRecovery
EvmHighLevelTypePass
后续语义 consumer / cleanup
```

是否放在 TypeRecovery 前后需要看第一版需要哪些信息。保守做法是先放在 TypeRecovery 后，优先消费 HType；如果发现某些高层类型证据必须反哺类型推理，再拆出一个前置 evidence pass，但不要按 calldata/memory 拆。

## 不做什么

- 不处理 storage/mapping。mapping 属于 storage 语义，后续继续走 storage 单独规划。
- 不再按 calldata、memory 分别做 dynamic bytes / array 识别 pass。
- 不在第一版里直接大规模 rewrite C backend 可见 IR。先产出统一 metadata/result，等消费侧稳定后再替换低层 marker。
- 不把 address、bool、uintN/intN 的值域规范化塞进这个 pass。它们可以作为 HType 或 value normalization 的输入/输出，但不是这个 pass 的主任务。
- 不删除已有 guard。cleanup 仍放到后续统一处理。

## 风险

- 如果对象视图太早绑定 HType，可能会把类型推理还没稳定的结果固化。第一版应该允许 unknown，并把 matcher 结果当作候选。
- calldata 和 memory 虽然 IR 形状统一，但 ABI layout 起点不同。需要用 provenance 记录“这是 ABI head 指针还是直接指向 length/data”，但不要把这个差异扩大成两个 pass。
- 现有 marker 比较分散。第一版应先读已有 marker，不急着一次性迁移全部 producer。
- 如果 pass 直接 rewrite 太多 IR，容易和现有 ABI return/revert/event pass 冲突。第一版先产出可 dump、可检查的分类结果。

## 判断标准

- 同一种类型只实现一套 matcher。例如 dynamic bytes 不因为 calldata/memory 写两套 matcher。
- 在已有 calldata/memory 统一 IR 上，能把 ABI head/tail、dynamic bytes、dynamic array 和 payload object 标出来。
- 后续 ABI return、revert、event、external call pass 能逐步改成消费这个统一结果，而不是继续各自扫描 calldata/memory 形状。
- storage/mapping 不被这个 pass 混进来，避免把完全不同的 storage layout 规则揉到一起。
- patterns suite 和 type recovery suite 不能出现明显性能下降；第一版至少对当前关注用例做同口径耗时对比。

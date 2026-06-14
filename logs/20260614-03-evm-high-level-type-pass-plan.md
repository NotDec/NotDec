# EVM 高层类型统一识别 Pass 规划

## 原始 prompt

MappingMatcher是什么，是storage吗，那一块单独处理，这里暂时不用管。把当前的规划写到一个logs/文件里面。特别开头要强调一下之前的考量：calldata、memory已经完全统一处理了，IR形状都是LLVM的load store了，所以统一处理是完全没问题的。而且也不要搞什么单独识别，单独rewrite在不同的pass，就按照类型处理，而不是按什么memory，calldata单独分

## 背景

当前 EVM 路线已经把 calldata 和 memory 访问基本统一到 LLVM IR 形状上：`evm_calldataload`、`evm_calldatacopy`、EVM memory access 等低层 helper 被改写成普通 `load`、`store`、`llvm.memcpy`，并把这些访问交给类型推理恢复 HType。

因此后续高层类型分析不应该再按 calldata、memory 分别写一套 pass，也不应该让不同 pass 各自扫描访问模式、各自 rewrite 同一种类型。现在更合适的方向是：先让低层访问规范化和类型推理完成，再完全基于 HType 结果做高层归类，例如 dynamic bytes、dynamic array、ABI tuple、struct、return/revert payload、event payload、external call payload。

也就是说，分类维度应该是“这是什么类型/布局”，不是“它来自 calldata 还是 memory”。来源仍然可以作为对象属性参与判断，但不应该变成 pass 边界。

Storage/mapping 这一块暂时不放进本规划。storage slot、mapping hash、storage array、storage bytes/string 的规则和 calldata/memory 的 LLVM load/store 对象不同，继续按 storage 语义路线单独处理。

## 目标

新增一个统一的高层类型归类 pass，暂名：

```text
EvmHighLevelTypePass
```

它的职责是消费类型恢复产出的 HType，以及已有的对象来源信息，把函数中的对象按高层类型归类。它不负责重新把 calldata 或 memory helper rewrite 成 LLVM IR，不负责扫描 `load/store/memcpy` 访问模式，也不负责 storage/mapping。

这个 pass 应该从 HType 中读取并记录：

- ABI tuple/head-tail 布局。
- dynamic bytes/string。
- dynamic array。
- static array。
- struct/tuple-like object。
- return payload。
- revert/custom error payload。
- event data payload。
- external call input/output payload。

这些类型可以出现在 calldata、memory、returndata 或临时 ABI buffer 中。pass 内部按 HType 类型分类，不按来源拆，也不为每种来源写单独识别逻辑。

## 基本设计

先建立一个统一的 HType 视图，而不是直接在 pass 里扫访问模式：

```text
Object
  base value
  source/provenance
  recovered HType
  known consumers/producers
  optional bounds facts already attached by earlier passes
```

`source/provenance` 只表达对象来自哪里，例如 calldata、memory、returndata、return buffer、revert buffer、event buffer、external call input buffer。它只作为解释 HType 的上下文，不作为访问模式匹配入口，也不是 pass 拆分依据。

高层分类只面向 HType 视图：

- `AbiTupleClassifier`：把 HType 中的 tuple/struct-like 对象归到 ABI tuple。
- `DynamicBytesClassifier`：把 HType 中的 bytes/string-like 对象归到 dynamic bytes/string。
- `DynamicArrayClassifier`：把 HType 中的 array-like 对象归到 dynamic array。
- `StructClassifier`：把 HType 中的固定字段对象归到 struct/tuple-like object。
- `PayloadClassifier`：结合对象来源，把 HType 对象标为 return/revert/event/external call payload。

这里不引入 `CalldataBytesMatcher`、`MemoryBytesMatcher` 这种按来源命名的 matcher，也不新增扫描 `load/store/memcpy` 的访问模式 matcher。同一个 HType 分类逻辑应该能覆盖 calldata tail、memory bytes、return data payload 等；provenance 只用于说明这个类型在当前语义场景中的角色。

## 和现有 Pass 的关系

现有 pass 继续负责低层规范化和局部证据：

- `MemoryBufferRewritePass`：把 EVM memory 访问整理成 LLVM memory 访问。
- `EvmCalldataAccessPass`：把 calldata 访问整理成 LLVM load/memcpy，并提供 min-size checked alias。
- `CheckedBoundsPass`：识别 panic bounds 和 arithmetic guard，提供 bounds 证据。
- ABI return/revert/event/external call 相关 pass：短期仍可保留现有 marker，但后续应逐步改成消费统一高层类型结果。

`EvmHighLevelTypePass` 不应该再去匹配旧的 `evm_calldataload` / `evm_mload` 形状，也不应该重新匹配规范化后的 `load/store/memcpy` 访问形状。它只看 HType 和已有对象来源。这样可以避免每个语义 pass 都重新猜 calldata/memory 布局。

建议位置：

```text
TypeRecovery
EvmHighLevelTypePass
后续语义 consumer / cleanup
```

这个 pass 固定放在 TypeRecovery 后。它是 HType consumer，不是类型推理前的 evidence pass。如果发现 HType 信息不足，应回到低层证据生产或类型推理本身补输入，而不是在这个 pass 里补访问模式匹配。

## 不做什么

- 不处理 storage/mapping。mapping 属于 storage 语义，后续继续走 storage 单独规划。
- 不按 calldata、memory 分别做 dynamic bytes / array 识别 pass。
- 不扫描 `load/store/memcpy` 访问模式来推导类型；类型结论必须来自 HType。
- 不在第一版里直接大规模 rewrite C backend 可见 IR。先产出统一 metadata/result，等消费侧稳定后再替换低层 marker 或 consumer。
- 不把 address、bool、uintN/intN 的值域规范化塞进这个 pass。它们可以作为 HType 或 value normalization 的输入/输出，但不是这个 pass 的主任务。
- 不删除已有 guard。cleanup 仍放到后续统一处理。

## 风险

- 如果 HType 结果还不够完整，这个 pass 只能产出 unknown 或部分分类。不要在这里用访问模式补猜类型，否则会回到多套 matcher 的路线。
- calldata 和 memory 虽然 IR 形状统一，但 ABI layout 起点不同。需要用 provenance 解释 HType 对象角色，但不要把这个差异扩大成两个 pass。
- 现有 marker 比较分散。第一版应先读已有对象来源和 HType，不急着一次性迁移全部 consumer。
- 如果 pass 直接 rewrite 太多 IR，容易和现有 ABI return/revert/event pass 冲突。第一版先产出可 dump、可检查的分类结果。

## 判断标准

- 同一种类型只实现一套 HType 分类逻辑。例如 dynamic bytes 不因为 calldata/memory 写两套 matcher。
- 基于 HType 能把 ABI tuple、dynamic bytes、dynamic array 和 payload object 标出来。
- 后续 ABI return、revert、event、external call pass 能逐步改成消费这个统一结果，而不是继续各自扫描 calldata/memory 形状。
- storage/mapping 不被这个 pass 混进来，避免把完全不同的 storage layout 规则揉到一起。
- patterns suite 和 type recovery suite 不能出现明显性能下降；第一版至少对当前关注用例做同口径耗时对比。

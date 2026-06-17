# EVM 高层类型统一识别 Pass 规划

## 原始 prompt

MappingMatcher是什么，是storage吗，那一块单独处理，这里暂时不用管。把当前的规划写到一个logs/文件里面。特别开头要强调一下之前的考量：calldata、memory已经完全统一处理了，IR形状都是LLVM的load store了，所以统一处理是完全没问题的。而且也不要搞什么单独识别，单独rewrite在不同的pass，就按照类型处理，而不是按什么memory，calldata单独分

## 背景

当前 EVM 路线已经把 calldata 和 memory 访问基本统一到 LLVM IR 形状上：`evm_calldataload`、`evm_calldatacopy`、EVM memory access 等低层 helper 被改写成普通 `load`、`store`、`llvm.memcpy`，并把这些访问交给类型推理恢复 HType。

因此后续高层类型分析不应该再按 calldata、memory 分别写一套 pass，也不应该让不同 pass 各自扫描访问模式、各自 rewrite 同一种类型。现在更合适的方向是：先让低层访问规范化和类型推理完成，再完全基于 HType 结果做高层归类，例如 dynamic bytes、dynamic array、ABI tuple、struct、return/revert payload、event payload、external call payload。

也就是说，分类维度应该是“这是什么类型/布局”，不是“它来自 calldata 还是 memory”。来源仍然可以作为对象属性参与判断，但不应该变成 pass 边界。

Storage/mapping 不放在这份规划里。它们已经有单独的 storage 高层 rewrite 路线，并且现在也能从 HType 里拿到类型结果；这里不再重复规划 storage slot、mapping hash、storage array、storage bytes/string 这些内容。

这两条路线共用同一份 HType 输出和对象来源信息，但消费方式不同：

- 本规划处理 calldata、memory、returndata、return/revert/event/external call payload 这些“非 storage”的高层类型归类。
- storage 规划处理 `sload` / `sstore` 及其 path rewrite。
- 两边都不回头重新扫旧 helper 形状，而是优先消费类型恢复结果。

## 目标

新增一个统一的高层类型重写 pass，暂名：

```text
EvmHighLevelTypePass
```

它的职责是消费类型恢复产出的现有 HType，以及已有的对象来源信息，匹配到能认出的高层对象后直接重写成更高层的 helper / intrinsic / 标记。它不负责重新把 calldata 或 memory helper rewrite 成 LLVM IR，不负责扫描 `load/store/memcpy` 访问模式，也不负责 storage/mapping。

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

这些类型可以出现在 calldata、memory、returndata 或临时 ABI buffer 中。pass 内部不按 calldata、memory 拆成两套，也不先分一层分类再 rewrite。

## 基本设计

直接基于现有 HType 和对象来源信息做重写，不再单独造一层统一视图。

这里不引入新的分类器，也不先把对象归一遍再处理。能直接确认的就直接重写，不能确认的就保留原样。

storage 相关的东西不放进这套 rewrite。storage 的 path、mapping、array、bytes/string 现在走另一条路线，后续如果要统一出口，也应该是在结果层面合并，而不是在这个 pass 里混进 storage 语义。

## 和现有 Pass 的关系

现有 pass 继续负责低层规范化和局部证据：

- `MemoryBufferRewritePass`：把 EVM memory 访问整理成 LLVM memory 访问。
- `EvmCalldataAccessPass`：把 calldata 访问整理成 LLVM load/memcpy，并提供 min-size checked alias。
- `CheckedBoundsPass`：识别 panic bounds 和 arithmetic guard，提供 bounds 证据。
- ABI return/revert/event/external call 相关 pass：短期仍可保留现有 marker，但后续应逐步改成消费统一高层类型结果。

`EvmHighLevelTypePass` 不应该再去匹配旧的 `evm_calldataload` / `evm_mload` 形状，也不应该重新匹配规范化后的 `load/store/memcpy` 访问形状。它只看现有 HType 和已有对象来源，匹配到了就直接 rewrite，不先走一层单独的标记或分类。这样可以避免每个语义 pass 都重新猜 calldata/memory 布局。

storage 侧也已经有自己的高层 rewrite pass。两边的关系可以这样定：

- `EvmHighLevelTypePass` 先把非 storage 对象按高层类型归类。
- `EvmStorageHighLevelRewritePass` 继续处理 storage path rewrite。
- 两边都依赖同一套 HType 结果，但不共享 matcher。
- 如果后面需要统一对外输出，可以再加一层结果汇总，不在这里提前合并。

建议位置：

```text
TypeRecovery
EvmHighLevelTypePass
后续语义 consumer / cleanup
```

这个 pass 固定放在 TypeRecovery 后。它是 HType consumer，不是类型推理前的 evidence pass。如果发现 HType 信息不足，应回到低层证据生产或类型推理本身补输入，而不是在这个 pass 里补访问模式匹配。

## 大致做法

这一版不追求把所有高层语义一次性拉平，只按对象类型分别处理：

- ABI tuple / head-tail：保留现有 buffer 证据，能确认布局时就把对应访问改写成 tuple / field 访问。
- dynamic bytes/string：直接处理长度、短数据、长数据这些稳定形状，不再分 calldata/memory 两套写法。
- dynamic/static array：按 HType 里的数组形状和索引关系重写元素访问、长度访问。
- struct/tuple-like 对象：按字段边界和偏移重写成字段访问，不再把它拆成通用指针算术。
- return / revert / event / external call payload：直接把 payload 相关的写入、拷贝或封装点改成高层 helper，让后续 consumer 看到的是语义对象，不是底层 buffer 操作。

这些重写都放在同一个 pass 里做。每一类都只保留一套判断和 rewrite 路径，不单独拆 matcher pass。

## 不做什么

- 不按 calldata、memory 分别做 dynamic bytes / array 识别 pass。
- 不扫描 `load/store/memcpy` 访问模式来推导类型；类型结论必须来自 HType。
- 不在第一版里直接大规模 rewrite C backend 可见 IR。先产出统一 metadata/result，等消费侧稳定后再替换低层 marker 或 consumer。
- 不把 address、bool、uintN/intN 的值域规范化塞进这个 pass。它们可以作为 HType 或 value normalization 的输入/输出，但不是这个 pass 的主任务。
- 不删除已有 guard。cleanup 仍放到后续统一处理。

## 风险

- 如果 HType 结果还不够完整，这个 pass 只能保留原样或做一部分重写。不要在这里用访问模式补猜类型，否则会回到多套 matcher 的路线。
- calldata 和 memory 虽然 IR 形状统一，但 ABI layout 起点不同。需要用 provenance 解释 HType 对象角色，但不要把这个差异扩大成两个 pass。
- 现有 marker 比较分散。第一版应先读已有对象来源和 HType，不急着一次性迁移全部 consumer。
- 如果 pass 直接 rewrite 太多 IR，容易和现有 ABI return/revert/event pass 冲突。第一版先产出可 dump、可检查的分类结果。

## 判断标准

- 同一种类型只实现一套重写逻辑。例如 dynamic bytes 不因为 calldata/memory 写两套 matcher。
- 基于 HType 能把 ABI tuple、dynamic bytes、dynamic array 和 payload object 标出来。
- 后续 ABI return、revert、event、external call pass 能逐步改成消费这个统一结果，而不是继续各自扫描 calldata/memory 形状。
- storage/mapping 不被这个 pass 混进来，避免把完全不同的 storage layout 规则揉到一起。
- patterns suite 和 type recovery suite 不能出现明显性能下降；第一版至少对当前关注用例做同口径耗时对比。

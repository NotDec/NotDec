# AbiReturnPass / SolidityRevertPass 对接 HType 计划

## 原始 prompt

把当前聊的关于改进AbiReturnPass 和SolidityRevertPass 的规划和各种注意事项都写到一个新的logs/下的文档里。特别注意目前准备删掉MemoryConsumer 和MemoryWrite 这种标注相关的东西，相关匹配逻辑挪到具体的Pass里面

## 背景

`AbiReturnPass` 和 `SolidityRevertPass` 都在处理 EVM memory 里的 ABI payload：

- `AbiReturnPass` 识别 `evm_return(mem, base, size)`。
- `SolidityRevertPass` 识别 `evm_revert(mem, base, size)`。

这里的 `base`、`size` 是底层 EVM return/revert 指令的参数，分别表示 payload 在 memory 里的起始
offset 和字节长度。

目前这些 pass 里仍有不少按访问形状找 payload 的逻辑，例如围绕
`notdec_solidity_memory_write`、`notdec_solidity_memory_consumer` 等 marker 回头扫写入。下一步不应该继续
扩大这套标注层。内存里的内容应该让类型推理负责，后置 pass 只读 HType 结果。

## 目标

1. `AbiReturnPass` 和 `SolidityRevertPass` 保留各自的出口识别逻辑。
2. payload 内容不再靠访问模式匹配恢复，而是通过 `HTypeResult` 查 `base` 的结构体指针类型和字段范围。
3. 删除或停止依赖 `MemoryConsumer`、`MemoryWrite` 这类语义/写入标注。
4. 如果 `base` 没被推成结构体指针，不做 mstore 扫描兜底，记录成类型恢复缺口。

## 总体路线

### 1. Pass 职责

`AbiReturnPass` 负责：

- 找 `evm_return(mem, base, size)`。
- 从 HType 里读取 return buffer 字段。
- 生成 return tuple / dynamic return 候选。

`SolidityRevertPass` 负责：

- 找 `evm_revert(mem, base, size)`。
- 从 HType 里读取 revert buffer 字段。
- 做 empty / Panic / Error(string) / custom error candidate / returndata bubble 分类。
- 保留 checked/bounds guard 和 Panic code 的关系。

两者暂不合并。revert 可以看作异常返回，但分类规则和 guard 关系比普通 return 多，单独保留 pass 更清楚。
可共用一个很薄的 buffer 字段读取 helper。

### 2. 类型结果入口

后置 pass 不解析 `.htypes` 文本，直接访问 C++ 数据结构：

- 通过 `MLsubRecovery::getResult(Mod, MAM)` 拿 `HTypeResult`。
- 对 `base` 调 `HTypeResult::getDefaultValueType(base)`。
- 如果结果是结构体指针，解引用到 record。
- 按 field range 读取 `[0, size)` 内的字段类型。

理论上，如果后续基于 `base` 做了 memory access，类型推理应该把 `base` 推成指针类型，并给出结构体字段。
这才是 return/revert payload 的主要信息来源。

### 3. 不再走访问模式匹配

`AbiReturnPass` / `SolidityRevertPass` 后续不应该再靠这些方式恢复 payload：

- 在 basic block 里向前扫 `notdec_solidity_memory_write`。
- 根据连续 `mstore` / `mstore8` / copy 的形状猜 ABI 字段。
- 用 `MemoryConsumer` 提前告诉 pass 这个 buffer 是 return/revert/event。
- 用 `MemoryWrite` 作为语义 pass 的主要输入。

如果类型恢复还需要低层 memory access facts，可以在类型恢复内部保留必要表示；但这些不应该作为
Solidity 语义 pass 的标注接口继续扩散。

### 4. 删除标注层

准备删除或停止使用：

- `MemoryConsumerKind`
- `MemoryConsumer`
- `MemoryBufferFacts::Consumers`
- 面向语义 pass 的 `MemoryWrite` 输入
- `notdec_solidity_memory_consumer` 这类 consumer marker
- return/revert pass 里围绕 `notdec_solidity_memory_write`、`notdec_solidity_memory_copy_write` 的 payload 推断逻辑

保留的边界是：

- pass 自己匹配 `evm_return` / `evm_revert` 出口。
- 类型推理负责把 memory access 汇总成 `base` 的结构体指针和字段类型。
- 后置 pass 只读 HType，不重建 memory layout。

## base 仍是整数时怎么办

如果 `evm_return` / `evm_revert` 的 `base` 查出来仍是普通整数：

1. 不回退到旧的访问模式匹配。
2. 记录函数名、call 指令、base、size、当前 HType。
3. 把样例列为类型恢复缺口，反馈给类型恢复阶段修。

这类情况通常说明 memory access 没有把 `base` 和对应 memory object 连起来，或者 HType lowering 没把对象字段暴露到
`base` 的结构体指针类型上。

## 风险

- 类型恢复现在可能还没有稳定地把所有 return/revert buffer base 推成结构体指针。
- 动态 bytes/string/array 的 tail range 可能还需要更好的 field range 表达。
- 删除 `MemoryWrite` 依赖前，要确认现有测试里哪些 oracle 只是 marker 数量，不是真正语义。
- `SolidityRevertPass` 的 Panic/Error 分类现在有一部分可能还依赖旧写入匹配，需要拆开迁移。

## 判断标准

- `AbiReturnPass` / `SolidityRevertPass` 不再通过 memory write marker 推 payload。
- return/revert buffer 的字段类型来自 `HTypeResult`。
- base 不是结构体指针的样例能被明确记录，而不是静默降级。
- 现有 EVM solidity patterns 测试仍通过，必要时更新只针对旧 marker 的 oracle。
- selected-apehex-80 中成功生成 HType 的样本，抽查 return/revert 结果不明显倒退。

# EVM 高层语义恢复后续 Pass 总览

## 原始 prompt

参考logs/archive/20260602-05-evm-memory-object-type-recovery-pass-ordering-plan.md，还有logs/20260603-03-EvmDedaubTypeRecoveryCase.md以及logs/20260603-04-EvmDedaubA0f3MemoryCase.md，看看还有哪些高层语义没有恢复，规划一下后续还要加哪些PASS负责恢复新的语义，写成一个新的logs/规划文件

## 背景

`20260602-05` 已经把 EVM 路线调整为：先把 memory object、calldata、storage 等低层访问接入类型恢复，再让后续语义 pass 消费 HType。`20260603-03` 和 `20260603-04` 的 Dedaub 对照说明，剩下的问题主要不是单个 memory matcher，而是 calldata、storage、dynamic bytes、message hash、signature、external call、event 这些高层语义还没有稳定接到类型恢复结果上。

后续原则：

- 类型恢复之后的 pass，能基于 HType 就不要基于模式匹配。
- matcher 只补 HType 缺少的边界、来源或 consumer 证据。
- ABI buffer、dynamic bytes / memory bytes 这类通用格式做 helper，不单独做 pass。
- 每个具体语义 pass 以恢复高层 helper、替换对应低层 IR 为目标；证据不够时保留低层 IR。

## 后续模块

### `EvmCalldataAccessPass`

Calldata 应该先作为每个 public entry 独立的输入 buffer 接入类型恢复。这个 pass 放在类型恢复之前，统一整理 `evm_calldataload`、`evm_calldatacopy` 等访问，让 MLsub/HType 能恢复 ABI head/tail、动态数组、bytes/string 和参数值类型。

详细规划见：

- `logs/20260612-02-evm-calldata-access-pass-plan.md`

### `EvmStorageSemanticsPass`

Storage 语义 pass 放在类型恢复之后，优先消费 HType 和 storage 访问事实，恢复 mapping hash 链、storage bytes/string short/long 编码、packed field 和 direct slot 访问。它不负责直接决定 mapping key 是否是 address，这类类型结论交给类型推理。

详细规划见：

- `logs/20260612-03-evm-storage-semantics-pass-plan.md`

### 通用 helper

ABI buffer 解析不单独做成 `EvmAbiEncodingPass`。ABI head/tail、dynamic tail、padding、tuple field 会出现在 return、revert、event、external call input、keccak message buffer 里，应该整理成共享 helper。

dynamic bytes / memory bytes 也不单独做成 pass。它作为 helper 被 calldata HType consumer、event、external call、signature/message 等场景按需调用。helper 优先消费 HType；HType 不足时再用 copy、padding、consumer 形状补证据。

### 其他语义 pass

后续还需要继续细化：

- `EvmSignaturePass`：识别 signature bytes、`r/s/v` 解析、`ecrecover` precompile。
- `EvmExternalCallPass`：恢复 call target/value/input/output、returndata 失败路径。
- `EvmEventSemanticsPass`：扩展动态 event data，减少临时 memory 暴露。
- `EvmSemanticCleanupPass`：最后清理已被高层 helper 覆盖的低层 helper 和 compiler guard。

## 建议顺序

```text
SelectorEntryOutliningPass
PayabilityGuardPass
SolidityRevertPass / CheckedBoundsPass 的 guard 识别部分
MemoryBufferRewritePass
EvmCalldataAccessPass
TypeRecovery
EvmStorageSemanticsPass
EvmSignaturePass
EvmExternalCallPass
AbiReturnPass
EventLogPass / EvmEventSemanticsPass
EvmSemanticCleanupPass
```

`CheckedBoundsPass` 仍然按 panic / guard 形状识别，不需要判断 pointer 是 ABI array、storage bytes 还是 allocation buffer。ABI return、revert、event、external call、signature 等 pass 消费 HType 和共享 helper，不再各自重新扫描整套 memory / calldata 格式。

## 判断标准

- `20260603-03` 样例里，`Execute(address[])` 的 calldata buffer HType 能恢复出动态数组结构。
- `20260603-03` 样例里，`_balanceOf/_allowance` 的 mapping hash 链和 key 数据流能作为类型推理输入。
- `20260603-03` 样例里，`_name/_symbol` 有 storage string 候选。
- `20260603-04` 样例里，`callTx(bytes,address,uint256,bytes)` 的 calldata buffer HType 能恢复出两个 bytes field。
- `20260603-04` 样例里，签名消息、`keccak256` 和 `ecrecover` 能连起来。
- `SolidityPatterns.cpp` 不继续增加大块 matcher，新逻辑拆到具体 cpp 文件或共享 helper。

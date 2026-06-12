# EvmStorageSemanticsPass 规划

## 原始 prompt

还是单独把EvmCalldataAccessPass的具体规划，EvmStorageSemanticsPass的具体规划单独拆到单独的logs/文件吧，logs/20260612-01-evm-high-level-semantics-pass-plan.md里面还是仅保留最早的那种简单的介绍。处理完这个之后commit一下

## 背景

Storage 访问有很多 Solidity 特殊格式：direct slot、mapping hash、nested mapping hash 链、storage bytes/string short/long 编码、packed field。当前已有一些 storage matcher helper，但还没有完整语义 pass 把这些访问整理成 HType 后续能消费的高层 storage helper。

这个 pass 放在类型恢复之后。它优先消费 HType，只有 HType 缺少 slot、key、bit 区间、short/long 分支等证据时，才用局部 matcher 补证据。它不负责直接决定 mapping key 是 address 还是 uint256，这类类型结论交给类型推理。

## 目标

把确认过的 storage 访问恢复成高层 helper，例如：

```text
value = evm.storage.load(slot)
value = evm.storage.mapping.load(base_slot, key)
value = evm.storage.mapping2.load(base_slot, key0, key1)
value = evm.storage.bytes.load(base_slot)
field = evm.storage.packed.load(slot, bit_offset, bit_width)
```

store 侧对应恢复成：

```text
evm.storage.store(slot, value)
evm.storage.mapping.store(base_slot, key, value)
evm.storage.bytes.store(base_slot, value)
evm.storage.packed.store(slot, bit_offset, bit_width, value)
```

helper 已经覆盖原始语义后，后续 cleanup 才能删除 `mstore(0,key)`、`mstore(32,slot)`、`sha3(0,64)` 等 scratch 操作。证据不够时保留低层 IR，但要把 slot/key/value 数据流作为类型推理输入。

## 输入证据

优先使用：

- HType / 类型恢复结果。
- `evm_sload(slot)` / `evm_sstore(slot, value)`。
- calldata buffer HType / calldata field 来源事实。
- value normalization 证据，例如 address mask、bool range、small uint mask、signextend。
- storage bytes/string 的 short / long 编码形状。

已有 matcher helper 作为补充：

- `matchStorageScratchKeccak()`：识别 `mstore(0, key); mstore(32, baseSlot); sha3(0, 64)`。
- `matchStorageMappingAccess()`：识别 mapping slot 后接 `sload/sstore`。
- `matchStorageArrayDataKeccak()`：识别 `sha3(slot)` 指向的动态数组 / long bytes 数据区。
- `matchPackedStorageAccess()`：识别 packed slot 上的 mask / shift / merge。

只凭 `sha3(0,64)` 不能定死类型。`sha3(key, slot)` 只能说明这是 storage 地址计算。key/value 类型要由类型推理综合 calldata HType、value normalization、load/store value 和使用位置来判断。

## Direct Slot

直接 slot 形状：

```text
value = sload(slot)
sstore(slot, value)
```

如果 slot 是常量，记录 direct storage field。普通 256-bit value 不在 storage pass 里定型。只有遇到特殊传递格式才进一步处理：

- value 被当作 bytes/string length 编码使用，进入 storage bytes/string。
- value 通过 mask/shift 取子字段，进入 packed field。
- 固定 hash slot 保留成 direct slot summary，不强行恢复业务变量名。

## Mapping / Nested Mapping

mapping 常见形状：

```text
mstore(0, key)
mstore(32, baseSlot)
slot1 = sha3(0, 64)
value = sload(slot1)
```

nested mapping：

```text
mstore(0, key2)
mstore(32, slot1)
slot2 = sha3(0, 64)
value = sload(slot2)
```

pass 负责恢复 key 链和 base slot：

```text
base_slot = 1
keys = [key0_value, key1_value]
value = sload(slot2)
```

如果 key 来自 calldata field，只记录这个数据流事实。key 是 address 还是 uint256，不在 storage pass 里拍板。类型推理后续把 calldata HType、value normalization 和 storage hash 事实合并。

base slot 暂时不是常量时不要直接放弃。它可能来自上一级 mapping hash，或者经过 helper 形参传递。能展开多少层就记录多少层。

## Storage Bytes / String

Solidity storage `bytes` / `string` 的 slot 同时编码长度和短数据：

- short：低 bit 为 0，长度从 slot 低字节右移 / mask 得到，数据内联在同一个 slot。
- long：低 bit 为 1，长度通常是 `slot >> 1`，数据从 `keccak(slot)` 后连续 storage slot 读取。

pass 需要把 short/long 两个分支合并成同一个 storage bytes/string 访问。`_name` / `_symbol` getter 这类场景里，不应该把两个分支分别当成 uint256 数组访问。

如果复制循环还缺少足够证据，至少要保留：

- base slot 是 storage bytes/string candidate。
- 当前路径是 short 还是 long。
- long 分支的数据 base 是 `keccak(baseSlot)`。

`bytes` 和 `string` 编码一样。没有函数名、返回类型或后续字符串语义时，先保留 bytes/string candidate。

## Packed Field

packed storage 常见形状：

- `sload(slot)`。
- `shr/shl/and` 取某段 bit。
- 写回时先清掉原字段，再 `or` 合入新值。

pass 记录：

- slot。
- bit offset。
- bit width。
- load/store。
- value 来源。

这个 bit 区间给类型推理使用。storage pass 不需要把 160-bit 字段直接命名成 address，也不需要把 1-bit 字段直接命名成 bool。

## 和 HType Store Evidence 的关系

如果类型恢复已经给出了某个 store 对应 field 的 evidence，就优先使用该 evidence。这样可以从 field offset / field type 反向找到相关 store，再确认它是不是 storage value、mapping key 或 packed field 写入。

struct 内嵌 mapping / array、多维动态数组、跨函数合并证据的全局 storage layout，不应该在 storage pass 里硬编码成大量专用分支。storage pass 负责把每段 storage 地址计算恢复成可组合事实：base slot、key 链、array index、packed bit 区间、bytes/string 数据区。类型推理和 HType 再合并这些事实，得到更完整的 storage layout。

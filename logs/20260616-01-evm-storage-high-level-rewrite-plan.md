# EVM Storage 高层语义恢复规划

## 原始 prompt

接下来应该是盘点一下，基于HType能怎么继续恢复各种高层的语义，比如针对各种具体的高层类型做rewrite pass

是的，写个新的plan，一边推进一边发现之前问题可以顺便修

不要单独分什么标记和rewrite，就在一个pass内直接完成识别和rewrite。其次，规范一下evm.storage.mapping_array.load 这种原语到底有哪些，具体的语义是什么，不要为所有的嵌套的组合去构建对应intrinsic，因为组合的数量是无尽的，规范一下有哪些这样的intrinsic函数吧

规划了这个pass放到pass pipeline哪个位置了吗

## 背景

当前 storage 类型推理已经能在 `[storage]` 里输出一棵较清楚的 HType 树。典型形状包括：

```text
slot_0
  bytes
    length
    short_data
    long_elem
    long_index

slot_4
  map
    key
    value
      field_slot_1
        dynamic_array
          elem
          index
          length

slot_0
  packed_16_160
```

这说明 storage 的布局信息已经不只停留在 `evm_sload` / `evm_sstore` 上，而是能被归集成合约级 storage root。下一步可以基于这棵树恢复更高层的 Solidity 语义。

这里的重点不是马上生成 Solidity 源码，而是先把 IR 里的低层 storage 访问直接改写成稳定的高层 storage intrinsic：

```text
storage.slot_0
storage.slot_4.map[key].value.field_slot_1.dynamic_array[index]
storage.slot_0.bytes
storage.slot_0.packed_16_160
```

后续 C 后端、Solidity-like 输出、审计报告或论文展示都可以消费这些事实。

## 目标

新增一条 storage 高层语义恢复路线，暂名：

```text
EvmStorageHighLevelRewritePass
```

第一版目标：

- 消费 HType `[storage]` 结果，而不是重新从零猜 storage layout。
- 把 `evm_sload` / `evm_sstore` 对应到稳定 storage path。
- 把 mapping / array / bytes/string / packed 的地址计算链折叠成少量可组合的高层 storage intrinsic。
- 在一个 pass 内完成识别和 rewrite。不要单独拆“标记 pass”和“rewrite pass”。
- 推进过程中如果发现 HType storage 树、matcher、UType lowering 有旧问题，可以顺手修，但要保持改动范围直接服务于这条路线。

暂不在第一版解决：

- 给每个字段恢复源码变量名。
- 直接输出完整 Solidity。
- 强行决定 `bytes` 还是 `string`。
- 强行把 160-bit packed field 定成 `address`，除非有额外语义证据。

## 输入证据

主要输入：

- HType `[storage]` tree。
- `evm_sload(slot)` / `evm_sstore(slot, value)`。
- 已有 storage matcher 识别出的 path：
  - direct slot
  - mapping
  - nested mapping
  - static array
  - dynamic array
  - packed field
  - storage bytes/string short/long
- value HType / semantic lattice，例如 `address`、`bool`、`u256`、bytes/string candidate。
- ABI / public function / known helper summary 提供的外部类型证据。

辅助输入：

- calldata 参数 HType。mapping key 经常来自 ABI 参数。
- return/revert/event/external call 的 high-level type 结果。storage value 经常流向这些 payload。
- checked-bounds / mask / signextend 等 value normalization 证据。

## 总体路线

只做一个 pass。这个 pass 在函数内扫描 storage 地址计算和 `evm_sload` / `evm_sstore`，结合 HType storage tree 确认 path，然后立刻 rewrite 成 storage intrinsic。识别成功就替换低层 IR；识别不完整就保留原 IR，不另外产出一套“只标记不改写”的中间 pass。

rewrite 结果必须用少量可组合 intrinsic 表达。不要为每一种嵌套组合造一个新函数，例如不要造：

```text
evm.storage.mapping_array.load(...)
evm.storage.mapping_struct_dynamic_array.store(...)
evm.storage.mapping2.store(...)
```

这些组合数量没有上限。正确做法是先构造 storage 位置，再对这个位置 load/store：

```text
root = evm.storage.slot(4)
user = evm.storage.map.value(root, key)
field = evm.storage.field(user, 1)
elem = evm.storage.dynamic_array.elem(field, index)
value = evm.storage.load(elem)
```

同一个 intrinsic 序列既能表示 `mapping(address => uint256[])`，也能表示 `mapping(address => User).history[i]`，区别只在中间组合了哪些位置构造器。

## Pipeline 位置

第一版实际放在 EVM 的类型恢复之后、现有 Solidity 语义 rewrite 之后：

```text
MLsubRecoveryMain / MLsubRecoveryOpt
HTypeDumpPass                // 如果启用 --dump-htypes，先 dump 原始 HType
AbiReturnPass
SolidityRevertPass
CheckedBoundsPass
EventLogPass
EvmStorageHighLevelRewritePass
```

原因：

- 这个 pass 依赖 `[storage]` HType，所以必须在 MLsub 类型恢复之后。
- 它的职责是消费类型结果并改写 IR，第一版不把改写后的 intrinsic 再送回类型推理。
- 一开始考虑过放在 `AbiReturnPass` / `EventLogPass` 前面，让后续 pass 看到更清楚的 storage 访问。但实际测试里，太早改写会让 checked-bounds 等旧 matcher 看不到原始 `evm_sload/sstore`，例如 storage array / bytes 的 guard marker 会丢。
- 当前先把它放到 `AbiReturnPass`、`SolidityRevertPass`、`CheckedBoundsPass`、`EventLogPass` 后面，作为 storage cleanup / high-level rewrite。后续如果某个 pass 真的需要消费 storage intrinsic，再单独迁移那个 pass 的 matcher。
- `HTypeDumpPass` 放在它前面，dump 的仍然是 rewrite 前的类型推理结果，便于调试 storage HType 本身。

需要注意：

- `TR.getResult()` 是基于 rewrite 前 IR 生成的。storage pass 新建的 intrinsic 第一版不要求有 HType。
- 后续 pass 如果继续查 `TR`，只能可靠查询旧 IR 里已有 value 的类型；不要假设新 intrinsic 自动有类型结果。
- 如果后面确实需要给新 intrinsic 补类型，要么做轻量注解，要么再设计一次 post-rewrite 类型同步，不在第一版里做。

## Storage Intrinsic 规范

intrinsic 分两类：位置构造 intrinsic 和访问 intrinsic。

位置构造 intrinsic 不读写 EVM storage，只表达“这是哪个 storage 位置”。它们可以看成 path builder：

```text
ref = evm.storage.slot(slot_number)
ref = evm.storage.field(base_ref, slot_offset)
ref = evm.storage.map.value(base_ref, key)
ref = evm.storage.static_array.elem(base_ref, index)
ref = evm.storage.dynamic_array.elem(base_ref, index)
```

语义：

- `evm.storage.slot(n)`：常量 direct slot `n`。
- `evm.storage.field(base, k)`：struct 字段或 mapping value 内部字段，表示 `base + k`。
- `evm.storage.map.value(base, key)`：Solidity mapping 地址，表示 `keccak256(key, base_slot)`。nested mapping 通过重复调用这个 intrinsic 表示。
- `evm.storage.static_array.elem(base, index)`：static array 元素，表示 `base + index`。
- `evm.storage.dynamic_array.elem(base, index)`：dynamic array 元素，表示 `keccak256(base) + index`。

访问 intrinsic 负责真正的 storage 读写或特殊编码：

```text
value = evm.storage.load(ref)
evm.storage.store(ref, value)

value = evm.storage.packed.load(ref, bit_offset, bit_width)
evm.storage.packed.store(ref, bit_offset, bit_width, value)

len = evm.storage.dynamic_array.length.load(base_ref)
evm.storage.dynamic_array.length.store(base_ref, len)

len_word = evm.storage.bytes.length.load(base_ref)
short_word = evm.storage.bytes.short_data.load(base_ref)
long_word = evm.storage.bytes.long_elem.load(base_ref, index)
evm.storage.bytes.length.store(base_ref, len_word)
evm.storage.bytes.short_data.store(base_ref, short_word)
evm.storage.bytes.long_elem.store(base_ref, index, long_word)
```

语义：

- `load/store(ref)`：读写完整 256-bit storage word。
- `packed.load/store(ref, off, width)`：读写一个 slot 内的 bit range。`packed.store` 的语义包括保留同 slot 其他 bit。
- `dynamic_array.length.load/store(base)`：读写动态数组主 slot 的 length。
- `bytes.length.*`：读写 Solidity storage `bytes/string` 主 slot 里的长度编码 word。这里保留 `length * 2` / `length * 2 + 1` 的 storage 编码事实，不急着抽象成普通长度值。
- `bytes.short_data.*`：读写 short bytes/string 主 slot 里的 inline data 部分。
- `bytes.long_elem.*`：读写 long bytes/string 在 `keccak(slot) + index` 上的 256-bit 数据 word。

暂时不引入整体 `evm.storage.bytes.load/store`。storage bytes/string 不是一个能直接放进 LLVM `i256` 的 primitive value；当前 IR 里能稳定对应的是 length word、short inline data word、long data word。后续如果识别到 Solidity 把 storage bytes/string 批量复制到 memory，再单独把整段循环或拷贝模式提升成更高层的赋值/复制语义。

暂时不单独引入 `map.key` intrinsic。key 的类型证据来自 `evm.storage.map.value(base, key)` 的 `key` 参数，以及 HType 里的 `slot:N.map.key` 字段。

## 1. Direct Slot

Solidity：

```solidity
contract C {
    uint256 totalSupply; // slot 0

    function set(uint256 x) public {
        totalSupply = x;
    }

    function get() public view returns (uint256) {
        return totalSupply;
    }
}
```

当前 HType 期望：

```text
storage
  slot_0
```

低层 IR：

```text
sstore(0, x)
y = sload(0)
```

rewrite：

```text
ref = evm.storage.slot(0)
evm.storage.store(ref, x)
y = evm.storage.load(ref)
```

能直接恢复：

- 这是 direct storage slot。
- load/store 访问的是同一个合约状态字段。

还需要额外证据：

- `slot_0` 是 `uint256`、`address`、`bool` 还是 enum。
- 源码变量名。

## 2. Packed Field

Solidity：

```solidity
contract C {
    bool enabled;   // slot 0, packed
    address owner;  // slot 0, packed
}
```

典型低层 IR：

```text
word = sload(0)
owner = and(shr(16, word), (1 << 160) - 1)
```

当前 HType 期望：

```text
storage
  slot_0
    packed_16_160
```

rewrite：

```text
ref = evm.storage.slot(0)
owner = evm.storage.packed.load(ref, bit_offset=16, bit_width=160)
```

能直接恢复：

- packed field 所在 slot。
- bit offset。
- bit width。
- 读写方向。

可以作为候选但不能硬判：

- width 160 强烈暗示 `address`。
- width 1 强烈暗示 `bool`。
- 小宽度可能是 `uintN`、`enum`、`bool`，需要结合 mask、比较、ABI、函数签名。

推进时要顺手检查：

- packed write 的 clear-mask / shift / or 是否都能映射回同一个 `packed_off_width`。
- packed read 和 write 是否合并到同一个 storage field。

## 3. Static Array

Solidity：

```solidity
contract C {
    uint256[10] values; // slot 0

    function set(uint256 i, uint256 v) public {
        values[i] = v;
    }

    function get(uint256 i) public view returns (uint256) {
        return values[i];
    }
}
```

低层 IR：

```text
slot = 0 + i
sstore(slot, v)
y = sload(slot)
```

当前 HType 期望：

```text
storage
  slot_0
    static_array
      index
      elem
```

rewrite：

```text
base = evm.storage.slot(0)
elem = evm.storage.static_array.elem(base, i)
evm.storage.store(elem, v)
y = evm.storage.load(elem)
```

能直接恢复：

- base slot。
- index value。
- elem load/store。

还需要额外证据：

- array length。`uint256[10]` 里的 `10` 不一定能从访问中知道。
- elem 的具体 Solidity 类型。当前如果只有读写关系，HType 可能显示 `ptr<load=void, store=void, psize=256>`，这说明 elem 有读写对象形状，但内部值类型还没具体化。

推进时要顺手检查：

- `elem` 的 HType 如果只有 `void`，要确认是值没有更多语义，还是 UType -> HType lowering 丢了信息。
- static array 和 direct slot `slot + const` 的 struct field offset 不要混淆。

## 4. Dynamic Array

Solidity：

```solidity
contract C {
    uint256[] history; // slot 2

    function push(uint256 v) public {
        history.push(v);
    }

    function get(uint256 i) public view returns (uint256) {
        return history[i];
    }
}
```

Solidity storage 形状：

```text
length = sload(2)
base = keccak256(2)
elem_slot = base + i
y = sload(elem_slot)
```

当前 HType 期望：

```text
storage
  slot_2
    dynamic_array
      length
      index
      elem
```

rewrite：

```text
base = evm.storage.slot(2)
len = evm.storage.dynamic_array.length.load(base)
elem = evm.storage.dynamic_array.elem(base, i)
y = evm.storage.load(elem)
```

能直接恢复：

- dynamic array base slot。
- length slot。
- data base 是 `keccak(baseSlot)`。
- elem index。

还需要额外证据：

- elem 类型。
- push/pop 的完整语义。第一版可以只恢复 load/store 和 length。

## 5. Mapping

Solidity：

```solidity
contract C {
    mapping(address => uint256) balance; // slot 4

    function set(address a, uint256 v) public {
        balance[a] = v;
    }

    function get(address a) public view returns (uint256) {
        return balance[a];
    }
}
```

低层 IR：

```text
mstore(0, a)
mstore(32, 4)
slot = keccak256(0, 64)
sstore(slot, v)
y = sload(slot)
```

当前 HType 期望：

```text
storage
  slot_4
    map
      key
      value
```

rewrite：

```text
base = evm.storage.slot(4)
value_ref = evm.storage.map.value(base, a)
evm.storage.store(value_ref, v)
y = evm.storage.load(value_ref)
```

能直接恢复：

- mapping base slot。
- key dataflow。
- value load/store。

还需要额外证据：

- key 是 `address` 还是 `uint256`。
- value 具体类型。

key 类型候选来源：

- ABI 参数类型。
- address mask / `evm_call` / `evm_balance` 等 address 语义。
- public getter signature。

## 6. Nested Mapping

Solidity：

```solidity
contract C {
    mapping(address => mapping(address => uint256)) allowance; // slot 1

    function set(address owner, address spender, uint256 v) public {
        allowance[owner][spender] = v;
    }
}
```

低层 IR：

```text
slot1 = keccak256(owner, 1)
slot2 = keccak256(spender, slot1)
sstore(slot2, v)
```

当前 HType 期望：

```text
storage
  slot_1
    map
      key
      value
        map
          key
          value
```

rewrite：

```text
base = evm.storage.slot(1)
outer_value = evm.storage.map.value(base, owner)
inner_value = evm.storage.map.value(outer_value, spender)
evm.storage.store(inner_value, v)
```

能直接恢复：

- nested mapping 层数。
- 每层 key 的数据流。
- 最终 value 的 load/store。

风险：

- 两层 mapping 的字段名都叫 `map.key/value`，展示时要靠 path 层级区分，不能只看字段名。
- 如果 base slot 不是常量，而是前一层 hash，matcher 必须保留链条，不要提前丢失。

## 7. Struct Field + Array / Mapping

Solidity：

```solidity
contract C {
    struct User {
        uint256 id;
        uint256[] history;
    }

    mapping(address => User) users; // slot 4

    function set(address a, uint256 i, uint256 v) public {
        users[a].history[i] = v;
    }
}
```

Solidity storage 形状：

```text
user_slot = keccak256(a, 4)
history_slot = user_slot + 1
base = keccak256(history_slot)
elem_slot = base + i
sstore(elem_slot, v)
```

当前 HType 期望：

```text
storage
  slot_4
    map
      key
      value
        field_slot_1
          dynamic_array
            length
            index
            elem
```

rewrite：

```text
root = evm.storage.slot(4)
user = evm.storage.map.value(root, a)
history = evm.storage.field(user, 1)
elem = evm.storage.dynamic_array.elem(history, i)
evm.storage.store(elem, v)
```

更好的展示层可以显示成：

```text
storage.slot_4.map[a].value.field_slot_1.dynamic_array[i] = v
```

这里不要急着生成 `users[a].history[i]`，因为 `users` 和 `history` 是源码变量名，需要 debug/source/ABI 之外的证据。

## 8. Bytes / String

Solidity：

```solidity
contract C {
    bytes data;   // slot 0
    string name;  // slot 1
}
```

Solidity storage 编码：

- short：长度和数据都在主 slot。
- long：主 slot 存 `length * 2 + 1`，数据从 `keccak(slot)` 开始。

当前 HType 期望：

```text
storage
  slot_0
    bytes
      length
      short_data
      long_elem
      long_index
```

rewrite：

```text
base = evm.storage.slot(0)
len_word = evm.storage.bytes.length.load(base)
short_word = evm.storage.bytes.short_data.load(base)
long_word = evm.storage.bytes.long_elem.load(base, i)
```

能直接恢复：

- 这是 bytes/string 编码形状。
- short/long 两条路径属于同一个 storage object。
- long data base 是 `keccak(slot)`。
- 每次 rewrite 对应的是一个 256-bit storage word，而不是完整 bytes/string value。

还需要额外证据：

- `bytes` 还是 `string`。
- 某段循环或 helper 是否是在把完整 bytes/string 拷贝到 memory。

提升成 `string` 的候选证据：

- ABI 返回类型或 public getter 类型。
- 函数名 / selector metadata。
- 字符串库函数或 UTF-8 语义使用。
- 显式 summary/signature override。

第一版 rewrite 只做 `evm.storage.bytes.length/short_data/long_elem.*`。展示给用户时可以先把这三类访问归在同一个 `bytes` object 下，同时在类型结果里保留 `string candidate` 信息。

## Pass 内部流程

这个 pass 内部可以分步骤实现，但它对外还是一个 pass：识别成功后直接 rewrite，识别失败则保留原 IR。不要把“标记”和“rewrite”拆成两个 pass。

### 1. 读取 Storage HType

输入 `[storage]` HType tree，输出内部 storage layout model：

```text
StorageNode {
  path: slot:4.map.value.field@slot+1.dynamic_array.elem
  kind: elem / key / value / length / packed / bytes ...
  htype: ...
}
```

这里可以内部用结构体，但对外仍保留字符串 path，避免把 binarysub 里的 field name 体系打乱。

判断标准：

- 能从当前 14 个 EVM storage HType regression 里扫描出所有 path。
- 不依赖 `field_N`。
- 不影响普通 memory HType。

### 2. 匹配并改写 Storage Access

扫描 IR 里的 `evm_sload` / `evm_sstore` 和相关地址计算。每识别出一个完整 storage access，就立即改写成 storage intrinsic：

```text
base = evm.storage.slot(...)
ref = evm.storage.map.value(base, key)
value = evm.storage.load(ref)
```

优先用 HType 已有 path；HType 缺口才调用局部 matcher 补证据。匹配不完整时保留原始 IR，不生成半成品 intrinsic。

判断标准：

- direct slot、packed、static array、nested mapping、mapping struct dynamic array 都能 rewrite。
- rewrite 后的 intrinsic 序列能组合表达嵌套 path，不出现 `mapping_array` / `mapping2` 这类组合爆炸 helper。
- 识别失败时保留低层 IR，不误 rewrite。

### 3. Debug Dump

虽然不单独做标记 pass，但这个 pass 仍然要 dump rewrite 决策，方便测试和调试，例如：

```text
[storage-rewrite]
static_array::%y
  path: slot:0.static_array.elem
  rewrite: slot(0) -> static_array.elem(index=arg0) -> load

nested_mapping::%slot2.store
  path: slot:1.map.value.map.value
  rewrite: slot(1) -> map.value(owner) -> map.value(spender) -> store(arg2)
```

判断标准：

- suite golden 可以锁住 rewrite 决策。
- 人能直接从 dump 看出原始 IR 对应哪个 storage path 和 intrinsic 序列。

### 4. 按类型逐步打开 Rewrite

每次只推进一种形状：

1. direct slot。
2. packed read/write。
3. static array。
4. mapping。
5. nested mapping。
6. dynamic array。
7. bytes/string。
8. struct field + array/mapping 组合。

判断标准：

- intrinsic 覆盖原始语义后，才能 cleanup 删除对应 scratch `mstore/sha3` 链。
- 没覆盖完整时保留原 IR，不生成半成品 intrinsic。

## 需要顺手修的问题

推进中如果遇到下面问题，可以直接修：

- HType storage path 没有归到同一棵树。
- field name 又退回 `field_N`，导致 rewrite path 不稳定。
- 某类 storage matcher 能识别 load 不能识别 store，或者相反。
- packed read/write 的 bit offset/width 不一致。
- bytes short/long 分支没有归到同一个 `.bytes` 节点。
- nested mapping hash 链丢了上一层 key。

但不要顺手做这些：

- 大范围重构 pass pipeline。
- 把所有 storage 类型一次性美化成 Solidity 类型。
- 只在 formatter 里隐藏真实类型问题。
- 为一个 case 引入专门 path object，除非字符串规则已经明显不够。

## 风险

- HType 是推理结果，不是源码 layout 原表。它能表达已观察到的访问，不一定覆盖未访问字段。
- `ptr<load=void, store=void, psize=256>` 不一定是错误。它可能只是说明有读写关系，但值没有更具体类型。
- mapping key/value 的具体 Solidity 类型常常要依赖 ABI 或外部语义。只看 storage path 不能硬判。
- bytes/string 编码相同，必须保留 `bytes/string candidate`，不能过早定成 string。
- rewrite 太早删除 scratch memory 会影响 keccak 证据。cleanup 必须等 storage intrinsic 覆盖完整语义。
- storage pass 和 calldata/memory 的统一高层类型 pass边界要清楚。storage 的地址规则特殊，仍单独处理；但 value 类型可以复用统一 HType / semantic lattice。

## 判断标准

- 新 pass 能从当前 storage HType regression 中直接 rewrite 出稳定 storage intrinsic。
- 每一种高层类型都有最小 Solidity 源码例子、IR 形状、HType path、预期 rewrite。
- direct slot、packed、static array、mapping、nested mapping、dynamic array、struct field array、bytes/string 都有测试覆盖。
- rewrite 前后语义不丢。证据不足时保留原 IR，不生成半成品 intrinsic。
- 如果推进中修了旧问题，日志要写清楚修的是 HType tree、matcher、还是 lowering。
- 性能不能明显下降。涉及 HType scan / rewrite 后，至少跑 `notdec.type_recovery.evm.tr_level_2` 和 fortune 当前关注用例同口径时间。

## 当前实现记录

本轮先落地最小可用版本，覆盖 direct slot、mapping / nested mapping、static array、简单 dynamic array data slot 和完整 word load/store。packed、bytes/string、dynamic array length、debug dump 还没做。

实际修改：

- [include/notdec/Passes/evm/SolidityPatterns.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatterns.h:61)：声明 `EvmStorageHighLevelRewritePass`。这个 pass 持有 `MLsubRecovery &TR`，只消费已经生成的 storage HType。
- [src/CMakeLists.txt](/sn640/NotDec/src/CMakeLists.txt:17)：把 `EvmStorageHighLevelRewritePass.cpp` 加入 `notdec-core`。
- [src/Passes/PassManager.cpp](/sn640/NotDec/src/Passes/PassManager.cpp:325)：把 pass 放到 `AbiReturnPass`、`SolidityRevertPass`、`CheckedBoundsPass`、`EventLogPass` 之后。最初尝试放在 `AbiReturnPass` 前，会让旧 storage bounds matcher 看不到原始 `evm_sload/sstore`，例如 `checked_bounds_storage_array_01` 丢 marker，所以改成最后做 storage cleanup。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:21)：新增 helper 函数声明生成逻辑，包括 `evm.storage.slot`、`evm.storage.field`、`evm.storage.map.value`、`evm.storage.static_array.elem`、`evm.storage.dynamic_array.elem`、`evm.storage.load/store`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:80)：`buildStorageRefFromAdd` 处理 `base + constant`、`constant + index`、`keccak(base) + index`。其中能递归恢复 base 的常量加法优先当作 field，不能恢复 base 时再按 static array 处理。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:149)：`buildStorageRef` 递归处理常量 slot、`add`、mapping keccak、dynamic array data keccak。匹配不完整时返回空，保留原 IR。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:204)：`rewriteStorageAccess` 把 `evm_sload/sstore` 替换成 storage ref + `load/store`。新 call 会复制原 call 的全部 metadata，避免丢掉前面 Solidity pass 加的 `notdec.solidity.*` 信息。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:238)：`run` 只读取 `TR.ResultVal` 缓存，不再调用 `TR.getResult()`。原因是 pass 现在位于若干 rewrite pass 之后，不能在 IR 已经变动后触发一次新的 HType 生成。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:239)：给 rewrite suite 增加 `evm.storage.*` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:98)：增加 static array 和 nested mapping 两个 frozen storage rewrite 用例。

已经验证：

```text
cmake --build ./build --target all -j4
ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
git diff --check
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-storage-rewrite.ll --tr-level=2 --frozen-tr-input-ir --dump-htypes=/tmp/notdec-fortune-storage-rewrite.htypes
```

结果：

- build 通过。
- `notdec.type_recovery.evm.tr_level_2` 通过，1.68s。
- `notdec.evm.solidity_rewrite` 通过，81.71s，新 storage rewrite case 已覆盖 `static_array.elem` 和 nested `map.value`。
- `git diff --check` 通过。
- fortune smoke 通过，当前本地 ASan 构建下 `elapsed=73.68 rss_kb=1266744`。这次 pass 只接在 EVM triple 分支，fortune 主要用于确认非 EVM 主链路没有被误伤。

未完成和风险：

- `notdec.evm.solidity_patterns` 全量仍有失败，但栈显示其中一类断言发生在 `MLsubRecoveryMain` 的 `addEVMRuntimeSemanticConstraints`，早于 storage rewrite pass 运行。这个问题不是挪 pass 位置能解决的，后面要单独查 type recovery 对大 EVM 样例里某些 constant operand 的处理。
- 当前 rewrite 还没有使用 HType tree 校验每个具体 path，只用已缓存 storage HType 作为总开关，再复用现有 storage matcher 从 IR 形状恢复路径。
- 还没做 packed、bytes/string、dynamic array length 和 debug dump。

评分：

- 实现效果：6/10。已经能产出组合式 storage intrinsic，但覆盖面还窄。
- 理解成本：6/10。代码集中在一个 pass，路径规则直接；但 `add` 的 field/static array 判定需要继续用测试约束。
- 维护成本：6/10。复用了现有 matcher，后续扩展 packed/bytes 会继续增加局部规则，最好在 debug dump 补上后再扩大覆盖。

## 当前实现记录：常量证据和 dynamic array length

本轮继续修了两块：

1. storage matcher 里的常量证据不再跳过，而是带 `ExtValuePtr`。
2. storage rewrite 新增 dynamic array length load/store intrinsic，先覆盖 length load。

实际修改：

- [include/notdec/Passes/evm/SolidityPatternUtils.h](/sn640/NotDec/include/notdec/Passes/evm/SolidityPatternUtils.h:83)：`StorageScratchKeccakMatch`、`StorageArrayDataKeccakMatch`、`StorageMappingAccessMatch`、`StorageArrayDataAccessMatch` 的 key/base slot 证据改成 `ExtValuePtr`。原因是 `i256 0` 这类常量必须带 `User/OpInd`，否则会在 `ExtValuePtr` canonicalize 时断言，也会把不同语义位置的同一个常量混在一起。
- [src/Passes/evm/solidity-patterns/StorageAccessMatchers.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/StorageAccessMatchers.cpp:50)：`matchStorageScratchKeccak` 和 `matchStorageArrayDataKeccak` 从 `mstore` 捕获 key/base slot 时，用 `getExtValuePtr(Store->StoredValue, Store->Inst, 0)` 保留常量 operand 位置。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:4143)：storage key/index/packed/bytes 证据改用 `ExtValuePtr`，`addStorageValueEvidenceConstraint` 直接对证据建类型节点，不再跳过常量。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:4454)：dynamic/static array index 从 `add` / `evm_add` 拆出时，也带对应 operand 位置。
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp:4745)：packed write 从 `or` / `shl` 拆值时保留中间 operand 位置，避免常量写入值再次变成裸 `ConstantInt`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:53)：新增 `evm.storage.dynamic_array.length.load/store` 声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:104)：新增 dynamic array length 判定。规则是同一函数里存在以该 slot 为 base 的 dynamic-array data access；如果 `sload(base)` 结果像 bytes/string 的 low-bit length decode，则不当作 array length。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:221)：`rewriteStorageAccess` 对已确认的 length load 改写成 `evm.storage.dynamic_array.length.load(ref)`，普通元素仍走 `evm.storage.load(ref)`。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:239)：增加 `storage_field`、`storage_dynamic_array.elem`、`storage_dynamic_array.length.*` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:134)：增加 `storage_mapping_struct_array_high_level_rewrite`，覆盖 `slot(4) -> map.value(key) -> field(1) -> dynamic_array.length.load` 和 `dynamic_array.elem`。

已经验证：

```text
cmake --build ./build --target all -j4
./build/bin/notdec test/evm/solidity-patterns/cases/0457_19495180_71d7525532_df21a257bd5c.ll -o /tmp/notdec-0457.ll --tr-level=2
./build/bin/notdec test/evm/solidity-patterns/cases/0258_19493864_78edd9f88e_b098925cbd2d.ll -o /tmp/notdec-0258.ll --tr-level=2
ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
git diff --check
```

结果：

- build 通过。
- `0457...` 通过，覆盖 mapping key 常量。
- `0258...` 通过，覆盖 packed / dynamic-array index 里的常量证据。
- `notdec.type_recovery.evm.tr_level_2` 通过，1.70s。
- `notdec.evm.solidity_rewrite` 通过，81.95s。
- `notdec.evm.solidity_patterns` 通过，459.95s。
- `git diff --check` 通过。

性能口径：

- 这轮没有再跑 fortune。fortune 是 wasm/二进制主链路的非 EVM sanity，不能代表 EVM storage rewrite 的性能。
- 后续如果要评估这条 EVM storage 路线，应该固定一个或几个 EVM 大样例，按同一 `--tr-level` 和同一 runner 统计耗时。

当前剩余：

- packed read/write 还没有在 rewrite pass 里改成 `evm.storage.packed.load/store`。
- bytes/string 还没有改成 `evm.storage.bytes.length/short_data/long_elem.*`。
- debug dump 还没做。

## 当前实现记录：packed read

本轮再推进了一块：

- packed read 已经能改写成 `evm.storage.packed.load(ref, bit_offset, bit_width)`。
- 这条 rewrite 避开了 bytes/string 的 short/long length decode，不再抢 `storage_bytes_encoding` 的 marker。

实际修改：

- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:53)：新增 `evm.storage.packed.load` 原语声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:142)：新增 bytes/string length decode 的本地排除判断，避免 packed rewrite 吃掉 `sload` 上的 `storage_bytes_encoding` 形状。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:245)：新增 packed read 识别和 rewrite。对 `sload -> shr/lshr -> and lowmask` 形状，直接替换成 `evm.storage.packed.load(ref, offset, width)`；如果中间指令只剩自己无用途，就顺手删掉。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:256)：增加 `storage_packed_load_calls` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:154)：增加 `storage_packed_read_high_level_rewrite`，覆盖 packed read。

已经验证：

```text
cmake --build ./build --target all -j4
./build/bin/notdec test/type-recovery/evm/cases/12_evm_storage_packed_read.ll -o /tmp/notdec-packed-read.ll --tr-level=2 --frozen-tr-input-ir
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

结果：

- packed read 单例通过。
- `notdec.evm.solidity_rewrite` 通过，81.91s。
- `notdec.evm.solidity_patterns` 通过，459.71s。

当前决策点：

- bytes/string 不做整体 `bytes.load/store`。下一步按 `length / short_data / long_elem` 三类 storage word 访问推进；如果后续发现批量复制到 memory 的模式，再单独识别成完整 bytes/string 的赋值或拷贝。

## 当前实现记录：bytes/string 读侧拆分

本轮按上面的决策推进 bytes/string rewrite：

- 不引入整体 `evm.storage.bytes.load/store`。
- `sload(base)` 如果被用于 Solidity storage bytes/string 长度解码，改写成 `evm.storage.bytes.length.load(base_ref)`。这个 intrinsic 返回主 slot 原始 word，不是已经右移后的普通长度。
- short 分支里的 `word & -256` 改写成 `evm.storage.bytes.short_data.load(base_ref)`。
- long 分支里的 `sload(keccak(base) + index)` 改写成 `evm.storage.bytes.long_elem.load(base_ref, index)`。
- 写侧暂时不做 bytes/string 专门 intrinsic，避免把普通 `sstore(slot, value)` 误认成完整 bytes/string 赋值。

实际修改：

- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:75)：新增 `evm.storage.bytes.length.load`、`evm.storage.bytes.short_data.load`、`evm.storage.bytes.long_elem.load` 三个原语声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:169)：新增 bytes/string length decode 的正向识别，只认 `lowbit/short_len/full_len/select` 这类稳定形状。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:625)：新增 short data rewrite，把 short inline data 提取表达式替换成 `bytes.short_data.load`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:671)：新增 long elem rewrite，从 `keccak(base)+index` 拆出 `index`，替换成 `bytes.long_elem.load`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:702)：`rewriteStorageAccess` 先处理 bytes long elem，再处理 packed；对 bytes base slot 先替换 short data，再把主 `sload` 改成 `bytes.length.load`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:760)：`run` 先扫描函数，收集确实出现 bytes/string length decode 的 base slot，后续按这个集合改写 short/long 访问。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:259)：rewrite suite 增加 bytes 三类 intrinsic 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:171)：增加 `storage_bytes_short_high_level_rewrite` 和 `storage_bytes_long_high_level_rewrite` 两个 frozen 用例。

已经验证：

```text
cmake --build ./build --target all -j4
./build/bin/notdec test/type-recovery/evm/cases/09_evm_storage_bytes_short.ll -o /tmp/notdec-bytes-short.ll --tr-level=2 --frozen-tr-input-ir
./build/bin/notdec test/type-recovery/evm/cases/10_evm_storage_bytes_long.ll -o /tmp/notdec-bytes-long.ll --tr-level=2 --frozen-tr-input-ir
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
git diff --check
```

结果：

- build 通过。
- bytes short 单例输出 `evm.storage.bytes.length.load` 和 `evm.storage.bytes.short_data.load`，没有普通 `evm.storage.load`。
- bytes long 单例输出 `evm.storage.bytes.length.load` 和 `evm.storage.bytes.long_elem.load`，没有普通 `evm.storage.load`。
- `notdec.evm.solidity_rewrite` 通过，82.24s。
- `notdec.type_recovery.evm.tr_level_2` 通过，1.77s。
- `notdec.evm.solidity_patterns` 通过，457.73s。
- `git diff --check` 通过。

性能口径：

- 这次仍没有用 fortune 作为主性能判断。原因是这条改动只影响 EVM storage rewrite，fortune 是非 EVM wasm/二进制 sanity。
- 本轮用 EVM rewrite/pattern suite 覆盖性能和误识别风险。后续要做稳定性能对比，应固定一个 EVM 大样例或小批量样例，单独记录同口径耗时。

评分：

- 实现效果：7/10。bytes/string 已经不再被当成整体 primitive，读侧三种 storage word 访问能分开表达。
- 理解成本：6/10。规则集中在一个 pass，但 length decode 和 long elem 都有局部 matcher，后面最好加 debug dump。
- 维护成本：6/10。暂时没有动公共 matcher；如果写侧 bytes/string 也要做，需要先固定更多 Solidity codegen 形状。

## 当前实现记录：packed store、debug dump、bytes 写侧判断、EVM 性能样例

本轮继续推进四件事：

1. packed write 改写成 `evm.storage.packed.store(ref, off, width, value)`。
2. 给 storage rewrite pass 增加 `LLVM_DEBUG` 输出，方便看命中和跳过原因。
3. 分析 bytes/string 写侧，先不实现宽泛 rewrite。
4. 固定一个 EVM 大样例作为 storage rewrite 性能 smoke。

实际修改：

- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:75)：新增 `evm.storage.packed.store(ref, off, width, value)` 原语声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:138)：新增 `debugRewrite` / `debugSkip`，通过 `-debug-only=evm-storage-high-level-rewrite` 打印命中和跳过信息。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:326)：新增 packed write 需要的 clear-mask bit range 解析。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:437)：新增 `and clear_mask`、`or`、`shl` matcher，同时支持 LLVM op 和 `evm_and/evm_or/evm_shl`。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:581)：新增 `eraseDeadTree`，packed store 成功后清理无用途的旧 `sload/and/shl/or` 链。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:648)：新增 `matchPackedStoreValue`，识别 `old=sload(slot)`、`old & clear_mask`、`value << off`、`or` 这种稳定 packed write 形状。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:679)：新增 `isPackedWritePreservedWordLoad`，避免旧 word 的 `sload` 先被 packed read 吃掉，导致后续 `sstore` 看不到 packed write。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:716)：新增 `rewritePackedStore`，生成 `packed.store` 并保留原 `sstore` metadata。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:1007)：`evm_sstore` 分支先尝试 packed store，再回落到普通 `storage.store` 或 dynamic array length store。
- [test/type-recovery/evm/cases/15_evm_storage_packed_write.ll](/sn640/NotDec/test/type-recovery/evm/cases/15_evm_storage_packed_write.ll:1)：新增 packed write 最小 frozen IR，用 `offset=160,width=96` 覆盖写侧。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:259)：rewrite suite 增加 `storage_packed_store_calls` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:171)：新增 `storage_packed_write_high_level_rewrite`。

debug 输出示例：

```text
./build/bin/notdec test/type-recovery/evm/cases/15_evm_storage_packed_write.ll \
  -o /tmp/notdec-packed-write.ll --tr-level=2 --frozen-tr-input-ir \
  -debug-only=evm-storage-high-level-rewrite

evm storage rewrite: skip packed-store preserved word load in packed_write:   %old = call i256 @evm_sload(i256 0)
evm storage rewrite: packed.store in packed_write:   call void @evm_sstore(i256 0, i256 %merged) ->   call void @evm.storage.packed.store(i256 %0, i256 160, i256 96, i256 %v)
```

bytes/string 写侧判断：

- 现在不做整体 `evm.storage.bytes.store`，也不急着实现所有 `bytes.length/short_data/long_elem.store`。
- Solidity 源码里 storage bytes/string 写侧不是单一形状。`ArrayUtils.cpp` 里 push/pop/resize 会处理 short 到 long、long 到 short、清理末尾 slot、长度编码等多种路径。
- 可以先做的窄规则是：
  - `sstore(ref, encoded_len_word)`，并且同一控制流里已有 bytes length decode 或 storage-byte-array bounds 证据时，改成 `bytes.length.store(ref, encoded_len_word)`。
  - `sstore(keccak(ref)+index, word)`，并且 base 已确认是 bytes/string 时，改成 `bytes.long_elem.store(ref, index, word)`。
  - short inline data store 只在能证明写的是主 slot inline data 部分时再做；它经常和 encoded length 合并在同一个 word 里，不能只看 `sstore(ref, value)`。
- 暂时不做的宽规则：
  - 把 resize/push/pop 的整段 helper 直接提升成完整 bytes assignment。
  - 把任意 `sstore(ref, value)` 当成 `bytes.length.store` 或 `bytes.short_data.store`。
  - 把 copy-to-storage helper 里的多个 `sstore` 折成整体 bytes store。

已经验证：

```text
cmake --build ./build --target all -j4
./build/bin/notdec test/type-recovery/evm/cases/15_evm_storage_packed_write.ll -o /tmp/notdec-packed-write.ll --tr-level=2 --frozen-tr-input-ir
./build/bin/notdec test/type-recovery/evm/cases/15_evm_storage_packed_write.ll -o /tmp/notdec-packed-write.ll --tr-level=2 --frozen-tr-input-ir -debug-only=evm-storage-high-level-rewrite
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-storage-rewrite.ll --tr-level=2
git diff --check
```

结果：

- build 通过。
- packed write 单例输出 `evm.storage.packed.store(i256 %0, i256 160, i256 96, i256 %v)`，没有普通 `evm.storage.store`。
- debug 单例能输出 preserved-word skip 和 packed.store 命中。
- `notdec.evm.solidity_rewrite` 通过，83.29s。
- `notdec.type_recovery.evm.tr_level_2` 通过，1.75s。
- `notdec.evm.solidity_patterns` 通过，460.85s。
- 固定性能样例 `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll`：1.2M、20368 行；本地本轮 `elapsed=17.68 rss_kb=958320`。
- `git diff --check` 通过。

评分：

- 实现效果：8/10。packed 读写闭环完成，debug 输出能直接解释为什么 preserved word load 没被改写。
- 理解成本：6/10。packed write matcher 比 read 多一些局部 helper，但规则和 MLsub 现有 packed write 形状一致。
- 维护成本：6/10。bytes 写侧暂时没有硬做，减少误识别风险；后续如果要继续，应先补更具体的 Solidity 写侧用例。

## 当前实现记录：bytes/string 写侧窄规则

本轮把上面规划里的 bytes/string 写侧窄规则落地了一部分：

- 支持 `evm.storage.bytes.length.store(base_ref, len_word)`。
- 支持 `evm.storage.bytes.long_elem.store(base_ref, index, word)`。
- 仍然不做整体 `evm.storage.bytes.store`。
- 仍然不做 `evm.storage.bytes.short_data.store`。原因是 short inline data 和 encoded length 通常合并写进同一个主 slot，单看 `sstore(base, value)` 不能稳定拆出“只写 short data”。

这次改写只在 base slot 已经通过 bytes/string length decode 确认时触发。也就是说，普通 `sstore(slot, value)` 不会因为 slot 常量相同就被当成 bytes length store；`sstore(keccak(slot)+index, value)` 也必须能回到已确认的 bytes/string base，才会变成 long elem store。

实际修改：

- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:88)：新增 `evm.storage.bytes.length.store(base_ref, len_word)` 原语声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:108)：新增 `evm.storage.bytes.long_elem.store(base_ref, index, word)` 原语声明。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:975)：新增 `rewriteBytesLongElemStore`，识别 `sstore(keccak(base)+index, word)`，并删除不再使用的旧 slot 计算链。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:1008)：新增 `rewriteBytesLengthStore`，识别已确认 bytes/string base 上的主 slot 写入。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:1074)：`evm_sstore` 分支先尝试 bytes long elem store 和 bytes length store，再尝试 packed store，最后回落到普通 storage store / dynamic array length store。
- [src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp](/sn640/NotDec/src/Passes/evm/solidity-patterns/EvmStorageHighLevelRewritePass.cpp:1121)：继续复用 `BytesBaseSlots`，只把同一函数内已有 bytes/string length decode 的 base 作为写侧候选。
- [test/run_evm_solidity_rewrite_suite.py](/sn640/NotDec/test/run_evm_solidity_rewrite_suite.py:262)：rewrite suite 增加 `storage_bytes_length_store_calls` 和 `storage_bytes_long_elem_store_calls` 计数。
- [test/evm/solidity-rewrite/manifest.json](/sn640/NotDec/test/evm/solidity-rewrite/manifest.json:226)：新增 `storage_bytes_write_high_level_rewrite`。
- [test/type-recovery/evm/cases/16_evm_storage_bytes_write.ll](/sn640/NotDec/test/type-recovery/evm/cases/16_evm_storage_bytes_write.ll:1)：新增 bytes 写侧 frozen IR，覆盖主 slot length word 写入和 long data slot 写入。

已经验证：

```text
cmake --build ./build --target all -j4
./build/bin/notdec test/type-recovery/evm/cases/16_evm_storage_bytes_write.ll -o /tmp/notdec-bytes-write.ll --tr-level=2 --frozen-tr-input-ir
ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-storage-rewrite.ll --tr-level=2
git diff --check
```

结果：

- build 通过。
- bytes 写侧单例输出 `evm.storage.bytes.length.load`、`evm.storage.bytes.length.store`、`evm.storage.bytes.long_elem.store`，没有普通 `evm.storage.store`。
- `notdec.type_recovery.evm.tr_level_2` 通过，1.74s。
- `notdec.evm.solidity_rewrite` 通过，82.61s。
- `notdec.evm.solidity_patterns` 通过，461.84s。
- 固定 EVM 性能样例 `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll` 本轮 `elapsed=18.99 rss_kb=955376`。
- `git diff --check` 通过。

仍未覆盖：

- `bytes.short_data.store`。
- short/long 互转、push/pop/resize 的整体 helper 语义。
- storage bytes/string 到 memory 的批量复制或整体赋值。
- `bytes` 到 `string` 的最终提升。当前继续按 storage 编码访问表达，等 ABI、库函数或其它语义证据再决定是否展示成 `string`。

评分：

- 实现效果：8/10。bytes/string 读侧和两个常见写侧 storage word 都能拆开表达，避免了整体 `bytes.store` 这种不稳定语义。
- 理解成本：6/10。新增规则复用已有 bytes base 收集和 dynamic array data matcher，没有再引入新 path 表示。
- 维护成本：6/10。规则仍然偏局部，后续补 short_data 或 helper rewrite 时需要对照 Solidity codegen，再加更具体用例。

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

第一版放在 EVM 的类型恢复之后、其他 Solidity 语义 rewrite 之前：

```text
MLsubRecoveryMain / MLsubRecoveryOpt
HTypeDumpPass                // 如果启用 --dump-htypes，先 dump 原始 HType
EvmStorageHighLevelRewritePass
AbiReturnPass
SolidityRevertPass
CheckedBoundsPass
EventLogPass
```

原因：

- 这个 pass 依赖 `[storage]` HType，所以必须在 MLsub 类型恢复之后。
- 它的职责是消费类型结果并改写 IR，第一版不把改写后的 intrinsic 再送回类型推理。
- 放在 `AbiReturnPass` / `EventLogPass` 前面，可以让后续 Solidity 语义 pass 看到更清楚的 storage 访问，例如 `return balances[a]` 先变成 `evm.storage.load(evm.storage.map.value(evm.storage.slot(0), a))`。
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

bytes_value = evm.storage.bytes.load(base_ref)
evm.storage.bytes.store(base_ref, bytes_value)
```

语义：

- `load/store(ref)`：读写完整 256-bit storage word。
- `packed.load/store(ref, off, width)`：读写一个 slot 内的 bit range。`packed.store` 的语义包括保留同 slot 其他 bit。
- `dynamic_array.length.load/store(base)`：读写动态数组主 slot 的 length。
- `bytes.load/store(base)`：读写 Solidity storage `bytes/string` 编码，内部覆盖 short/long 两种情况。

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
b = evm.storage.bytes.load(base)
evm.storage.bytes.store(base, b)
```

能直接恢复：

- 这是 bytes/string 编码形状。
- short/long 两条路径属于同一个 storage object。
- long data base 是 `keccak(slot)`。

还需要额外证据：

- `bytes` 还是 `string`。

提升成 `string` 的候选证据：

- ABI 返回类型或 public getter 类型。
- 函数名 / selector metadata。
- 字符串库函数或 UTF-8 语义使用。
- 显式 summary/signature override。

第一版 rewrite 成 `evm.storage.bytes.*`。展示给用户时可以先叫 `bytes`，同时在类型结果里保留 `string candidate` 信息。

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

# EVM Storage 接入类型推理计划

## 原始 prompt

不，先不要急着考虑EvmStorageSemanticsPass以及plan里的内容，那边可能要大改。把接入方式这一块扩写并每部分都举例子（solidity源码）应该怎么接入，单独写成一个logs/文件。我感觉这一块还是有不少细节，特别是对那种特殊的什么数组类型等，可能要弄成那种指针加法里面特殊的field name。

## 背景

这里先不讨论 `EvmStorageSemanticsPass`。这份日志只讨论一件事：storage 访问怎么作为类型推理输入。

现有 binarysub / SimpleSub 的做法是：

- 程序里的数据流变成子类型约束。
- 类型变量记录上下界。
- 普通内存通过“地址对象 + field/load/store”接入。

普通内存的例子是：

```text
store value, ptr  ->  value <: memory_location
load ptr          ->  memory_location <: load_result
gep ptr, offset   ->  ptr has field offset
```

storage 也有 load/store，但地址不是普通 byte offset。Solidity 把 storage layout 编码成：

- 常量 slot。
- `keccak(key, slot)` 的 mapping。
- `keccak(slot) + index` 的动态数组。
- 一个 slot 里的 packed bit field。
- bytes/string 的 short/long 两套编码。

所以接入类型推理时，关键不是把 `evm_sload(key)` 直接当普通 load，而是先把 `key` 解释成 storage path，再给 path 建类型变量。

## 总原则

不要把 storage 塞进普通线性内存。

建议单独建一个 storage root type，形式上仍然复用 `TMemObject` / record field，但 field name 使用 storage path：

```text
storage.slot:0
storage.slot:1.map
storage.slot:2.array
storage.slot:3.packed@160:8
storage.slot:4.bytes
```

这样做的好处是：

- 仍然能用 SimpleSub 的 record field 约束。
- 和普通 pointer + offset 的模式一致，容易接进现有代码。
- field name 里保留 Solidity storage 地址语义，不把 mapping / array / packed 混成普通 `uint256` 地址。

大致类型形状：

```text
StorageRoot <: {
  "slot:0": CellTy,
  "slot:1.map": {
    "key": KeyTy,
    "value": ValueTy
  },
  "slot:2.array": {
    "index": IndexTy,
    "length": LengthTy,
    "elem": ElemTy
  }
}
```

这里的 record 不是源码里的 struct，只是类型推理内部的约束载体。后面 HType 或其它结果再决定怎么显示。

## 1. Direct Slot

Solidity 例子：

```solidity
contract C {
    uint256 public totalSupply; // slot 0

    function set(uint256 x) public {
        totalSupply = x;
    }

    function get() public view returns (uint256) {
        return totalSupply;
    }
}
```

Solidity storage 形状：

```text
sstore(0, x)
y = sload(0)
```

类型接入：

```text
field name: storage.slot:0

x <: Cell(slot:0)
Cell(slot:0) <: y
```

落到 binarysub 可以是：

```text
StorageRoot <: { "slot:0": Cell0 }
x <: Cell0
Cell0 <: y
```

如果 `x` 还被 `evm_balance`、`evm_call` 的 address 参数使用，或者被 address mask 清理，就会继续产生：

```text
x <: address_like_type
```

最后 `Cell0` 自然会吸收这些约束。storage 这层不需要提前判断它是 `uint256` 还是 `address`。

## 2. Mapping

Solidity 例子：

```solidity
contract C {
    mapping(address => uint256) public balanceOf; // slot 0

    function set(address a, uint256 v) public {
        balanceOf[a] = v;
    }

    function get(address a) public view returns (uint256) {
        return balanceOf[a];
    }
}
```

Solidity storage 形状：

```text
mstore(0, a)
mstore(32, 0)
slot = keccak256(0, 64)
sstore(slot, v)

mstore(0, a)
mstore(32, 0)
slot = keccak256(0, 64)
y = sload(slot)
```

类型接入：

```text
field name: storage.slot:0.map

a <: Map0.key
v <: Map0.value
Map0.value <: y
```

落到 record：

```text
StorageRoot <: {
  "slot:0.map": {
    "key": Key0,
    "value": Val0
  }
}

a <: Key0
v <: Val0
Val0 <: y
```

这个模型表达的是“slot 0 是一个 mapping，它的 key 类型是 Key0，value 类型是 Val0”。它不表达某一个具体 key 对应的具体对象身份。对类型恢复来说，这通常刚好够用。

如果 key 从 calldata 来：

```solidity
function get(address a) public view returns (uint256) {
    return balanceOf[a];
}
```

calldata 侧会给 `a` 产生 ABI 参数约束，storage 侧再给：

```text
a <: Key0
```

两边合并后，key 才更可能显示成 address。storage matcher 本身不拍板。

## 3. Nested Mapping

Solidity 例子：

```solidity
contract C {
    mapping(address => mapping(address => uint256)) public allowance; // slot 1

    function approve(address owner, address spender, uint256 v) public {
        allowance[owner][spender] = v;
    }

    function get(address owner, address spender) public view returns (uint256) {
        return allowance[owner][spender];
    }
}
```

Storage 地址形状：

```text
slot1 = keccak256(owner, 1)
slot2 = keccak256(spender, slot1)
sstore(slot2, v)
```

类型接入有两种可选表达。

第一种是扁平 field name：

```text
field name: storage.slot:1.map.map

owner   <: Allowance.key0
spender <: Allowance.key1
v       <: Allowance.value
Allowance.value <: y
```

第二种是嵌套 record：

```text
StorageRoot <: {
  "slot:1.map": {
    "key": OwnerTy,
    "value": {
      "map.key": SpenderTy,
      "map.value": ValueTy
    }
  }
}
```

更建议第二种。原因是它和源码结构更接近，后面如果 value 不是 `uint256`，而是 struct 或 array，也能自然继续往下挂。

## 4. Static Array

Solidity 例子：

```solidity
contract C {
    uint256[3] public xs; // slot 0, xs[0] at slot 0

    function set(uint256 i, uint256 v) public {
        xs[i] = v;
    }

    function get(uint256 i) public view returns (uint256) {
        return xs[i];
    }
}
```

Storage 形状：

```text
slot = 0 + i
sstore(slot, v)
y = sload(slot)
```

如果 `i` 是常量：

```text
xs[2] -> slot 2
field name: storage.slot:2
```

这可以退化成 direct slot。

如果 `i` 是动态值，不能给每个 index 建无限 field。建议用数组元素模型：

```text
field name: storage.slot:0.static_array

i <: Array.index
v <: Array.elem
Array.elem <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:0.static_array": {
    "index": IndexTy,
    "elem": ElemTy
  }
}
```

如果能从编译器 guard 看到 `i < 3`，可以额外给数组记长度事实：

```text
Array.length_const = 3
```

这个长度先不要硬塞进 SimpleType。更适合放 side table，后面生成 HType 时再用。

## 5. Dynamic Array

Solidity 例子：

```solidity
contract C {
    uint256[] public xs; // slot 0 stores length

    function push(uint256 v) public {
        xs.push(v);
    }

    function get(uint256 i) public view returns (uint256) {
        return xs[i];
    }
}
```

Storage 形状：

```text
len = sload(0)
base = keccak256(0)
elem_slot = base + i
y = sload(elem_slot)
```

类型接入：

```text
field name: storage.slot:0.dynamic_array

LengthCell(slot:0) <: len
i <: Array.index
v <: Array.elem
Array.elem <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:0.dynamic_array": {
    "length": LengthTy,
    "index": IndexTy,
    "elem": ElemTy
  }
}
```

这里 `slot:0` 同时是 length slot，又是 array object 的 base slot。不要把它当普通 direct uint256 字段后就结束，否则会丢掉 `keccak(slot)` 后面的元素类型。

如果代码里只有 `xs.length`：

```solidity
function len() public view returns (uint256) {
    return xs.length;
}
```

只生成：

```text
Array.length <: ret
```

暂时没有 `elem` 也没关系。

## 6. Mapping 到 Dynamic Array

Solidity 例子：

```solidity
contract C {
    mapping(address => uint256[]) public history; // slot 2

    function get(address a, uint256 i) public view returns (uint256) {
        return history[a][i];
    }
}
```

Storage 形状：

```text
arr_slot = keccak256(a, 2)
len = sload(arr_slot)
data_base = keccak256(arr_slot)
elem_slot = data_base + i
y = sload(elem_slot)
```

类型接入：

```text
field name: storage.slot:2.map.value.dynamic_array

a <: Map.key
i <: Array.index
Array.elem <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:2.map": {
    "key": AddressKeyTy,
    "value": {
      "dynamic_array.length": LengthTy,
      "dynamic_array.index": IndexTy,
      "dynamic_array.elem": ElemTy
    }
  }
}
```

这个例子说明 field name 不能只用最终 `keccak` 值。必须保留 path：

```text
slot 2 -> map key a -> array elem index i
```

否则类型推理知道有一个动态数组元素，却不知道它属于 mapping 的 value。

## 7. Array 到 Mapping

Solidity 例子：

```solidity
contract C {
    struct Bucket {
        mapping(address => uint256) values;
    }

    Bucket[] public buckets; // slot 0

    function get(uint256 i, address a) public view returns (uint256) {
        return buckets[i].values[a];
    }
}
```

Storage 形状大致是：

```text
array_base = keccak256(0)
bucket_slot = array_base + i * bucket_size
mapping_base_slot = bucket_slot + field_offset
value_slot = keccak256(a, mapping_base_slot)
y = sload(value_slot)
```

类型接入：

```text
field name: storage.slot:0.dynamic_array.elem.field:values.map

i <: Array.index
a <: Map.key
Map.value <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:0.dynamic_array": {
    "index": IndexTy,
    "elem": {
      "field:values.map": {
        "key": KeyTy,
        "value": ValueTy
      }
    }
  }
}
```

这里的 `field:values` 名字实际恢复前可能没有源码名，可以先用 slot offset：

```text
field@slot+0.map
field@slot+1.map
```

后面如果能合并到 struct HType，再显示成更像源码的字段名。

## 8. Struct in Storage

Solidity 例子：

```solidity
contract C {
    struct User {
        address owner;
        uint256 balance;
    }

    User public user; // slot 0: owner, slot 1: balance

    function set(address a, uint256 b) public {
        user.owner = a;
        user.balance = b;
    }
}
```

Storage 形状：

```text
sstore(0, a)
sstore(1, b)
```

如果没有源码名，类型推理只能看到：

```text
storage.slot:0
storage.slot:1
```

但如果多个访问来自同一个 base object，例如 array elem 或 mapping value 的 struct：

```solidity
mapping(address => User) public users; // slot 5

function getBalance(address a) public view returns (uint256) {
    return users[a].balance;
}
```

Storage 形状：

```text
base = keccak256(a, 5)
slot = base + 1
y = sload(slot)
```

类型接入：

```text
field name: storage.slot:5.map.value.field@slot+1

a <: Map.key
Field1 <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:5.map": {
    "key": KeyTy,
    "value": {
      "field@slot+1": Field1Ty
    }
  }
}
```

这就是用户提到的“像指针加法里的特殊 field name”。普通内存用 byte offset，storage struct 更适合用 slot offset：

```text
field@slot+0
field@slot+1
field@slot+2
```

packed 字段再在这个 field 后面加 bit 区间。

## 9. Packed Field

Solidity 例子：

```solidity
contract C {
    struct Flags {
        bool enabled;      // 1 bit / 1 byte level evidence
        uint8 kind;
        address owner;
    }

    Flags public flags; // often packed into slot 0

    function setOwner(address a) public {
        flags.owner = a;
    }
}
```

Storage 形状：

```text
old = sload(0)
cleared = old & mask
new = cleared | (a << bit_offset)
sstore(0, new)
```

读取时：

```text
word = sload(0)
owner = (word >> bit_offset) & ((1 << 160) - 1)
```

类型接入时不要只给整个 slot 一个 `Cell0`。要给 packed field 单独建 cell：

```text
field name: storage.slot:0.packed@offset:width

a <: PackedField(0, offset, 160)
PackedField(0, offset, 160) <: owner
```

record 形状：

```text
StorageRoot <: {
  "slot:0": WholeSlotTy,
  "slot:0.packed@0:8": Field0Ty,
  "slot:0.packed@8:8": Field1Ty,
  "slot:0.packed@16:160": Field2Ty
}
```

`WholeSlotTy` 可以保留，但 value 类型主要从 packed field 走。否则 `bool`、`uint8`、`address` 会全混到一个 256-bit word 里。

如果 packed field 属于 mapping value struct：

```solidity
mapping(address => Flags) public flagsOf; // slot 3
```

field name 可以是：

```text
storage.slot:3.map.value.field@slot+0.packed@16:160
```

这比单独记 `slot = keccak(a,3)` 更有用，因为它保留了“mapping value 的 packed 字段”。

## 10. Bytes / String

Solidity 例子：

```solidity
contract C {
    string public name; // slot 0

    function set(string memory s) public {
        name = s;
    }

    function get() public view returns (string memory) {
        return name;
    }
}
```

Solidity storage 编码：

- short string：长度和数据都在 slot 0。
- long string：slot 0 存长度编码，数据从 `keccak256(0)` 后面连续 slot 读取。

类型接入建议把它当一个 bytes/string object，而不是两个无关路径。

```text
field name: storage.slot:0.bytes

Bytes.length <: len
Bytes.short_data <: ret_bytes
Bytes.long_elem <: ret_bytes.elem
```

record 形状：

```text
StorageRoot <: {
  "slot:0.bytes": {
    "length": LengthTy,
    "short_data": ShortDataTy,
    "long_index": IndexTy,
    "long_elem": ElemTy
  }
}
```

如果没有证据区分 bytes 和 string，就先叫 bytes。后面如果函数返回 ABI string、或者调用语义要求 string，再提升成 string。

重点是 short / long 两个分支必须合并到同一个 `slot:0.bytes`。如果拆成：

```text
storage.slot:0
storage.slot:keccak(0).array
```

类型结果会看起来像一个 uint256 字段加一个数组，源码语义就丢了。

## 11. Bytes Array / String Array

Solidity 例子：

```solidity
contract C {
    string[] public names; // slot 0

    function get(uint256 i) public view returns (string memory) {
        return names[i];
    }
}
```

Storage 形状：

```text
len = sload(0)
array_base = keccak256(0)
elem_slot = array_base + i

// elem itself uses bytes/string short/long encoding
elem_word = sload(elem_slot)
if long:
  elem_data_base = keccak256(elem_slot)
```

类型接入：

```text
field name: storage.slot:0.dynamic_array.elem.bytes

i <: Array.index
Bytes.length <: ret.length
Bytes.long_elem <: ret.elem
```

record 形状：

```text
StorageRoot <: {
  "slot:0.dynamic_array": {
    "index": IndexTy,
    "elem": {
      "bytes.length": LengthTy,
      "bytes.short_data": ShortDataTy,
      "bytes.long_index": LongIndexTy,
      "bytes.long_elem": ByteElemTy
    }
  }
}
```

这个例子说明 bytes/string 不是 direct slot 的特例。它可以出现在 array elem、mapping value、struct field 里。

## 12. Mapping 到 Struct 到 Dynamic Array

Solidity 例子：

```solidity
contract C {
    struct User {
        uint256 score;
        uint256[] history;
    }

    mapping(address => User) public users; // slot 4

    function getHistory(address a, uint256 i) public view returns (uint256) {
        return users[a].history[i];
    }
}
```

Storage 形状：

```text
user_base = keccak256(a, 4)
history_slot = user_base + 1
history_data_base = keccak256(history_slot)
elem_slot = history_data_base + i
y = sload(elem_slot)
```

类型接入：

```text
field name:
storage.slot:4.map.value.field@slot+1.dynamic_array

a <: Map.key
i <: Array.index
Array.elem <: y
```

record 形状：

```text
StorageRoot <: {
  "slot:4.map": {
    "key": KeyTy,
    "value": {
      "field@slot+1.dynamic_array": {
        "length": LengthTy,
        "index": IndexTy,
        "elem": ElemTy
      }
    }
  }
}
```

这类组合是接入方式必须支持的重点。不要给每种组合写死一个独立类型规则。更好的做法是统一维护 path segment：

```text
slot(4)
  -> map(key=a)
  -> value
  -> field(slot+1)
  -> dynamic_array(index=i)
  -> elem
```

最后把 path segment 序列编码成 field name 或嵌套 record。

## 13. 建议的 Path Segment

内部先用结构化 path，不要一开始就拼字符串。建议类似：

```text
StoragePath =
  RootSlot(N)
  Segment::Mapping(KeyValue)
  Segment::ArrayIndex(IndexValue, StaticOrDynamic)
  Segment::StructFieldSlotOffset(N)
  Segment::PackedBits(BitOffset, BitWidth)
  Segment::BytesString
```

生成约束时再转成 field name。

例子：

```text
RootSlot(4)
Mapping(a)
StructFieldSlotOffset(1)
ArrayIndex(i, Dynamic)
```

转成：

```text
storage.slot:4.map.value.field@slot+1.dynamic_array.elem
```

对应 side constraints：

```text
a <: storage.slot:4.map.key
i <: storage.slot:4.map.value.field@slot+1.dynamic_array.index
```

这样既保留了字段名，又能把 key/index 单独接进类型推理。

## 14. 约束生成位置

短期更适合放在 MLsub 约束生成附近，而不是后置语义 rewrite。

原因：

- `evm_sload/sstore` 的 value 类型必须进入求解。
- mapping key 和 array index 的类型要和 calldata、address mask、算术比较一起合并。
- packed field 的 value 类型要避免被整个 256-bit slot 吞掉。

建议在 `ConstraintsGenerator::MLsubVisitor::visitCallBase()` 处理 EVM helper 时加 storage evidence：

```text
evm_sload(slot):
  if slot can be normalized to StoragePath:
    Cell(path) <: result
  else:
    slot <: storage_key

evm_sstore(slot, value):
  if slot can be normalized to StoragePath:
    value <: Cell(path)
  else:
    slot <: storage_key
```

如果 path 里有 key/index：

```text
key <: KeyType(path prefix)
index <: IndexType(path prefix)
```

如果只识别到 `sha3(key, base)`，但 base 还不是常量，也不要丢。可以建一个相对 path：

```text
storage.unknown_base.map
```

等后面发现 base 来自上一层 mapping 或函数参数，再合并。

## 15. Field Name 规则

field name 要稳定、可读、可排序，方便 debug 和 HType。

建议格式：

```text
slot:N
slot:N.map
slot:N.map.value.map
slot:N.dynamic_array
slot:N.static_array
field@slot+K
packed@B:W
bytes
```

组合例子：

```text
slot:0
slot:1.map.key
slot:1.map.value
slot:1.map.value.field@slot+2
slot:2.dynamic_array.elem
slot:3.map.value.field@slot+0.packed@16:160
slot:4.dynamic_array.elem.bytes
```

如果落到 `TMemObject`，不要把每个子字段都摊平到 root。优先嵌套：

```text
StorageRoot {
  "slot:3.map": {
    "key": KeyTy,
    "value": {
      "field@slot+0": {
        "packed@16:160": OwnerTy
      }
    }
  }
}
```

只有 debug 输出可以再打印完整 path。

## 16. 不应该做的事

不要只用 `storage_key` primitive 表达全部 storage 语义。它只能说明一个值可作为 storage key，不能恢复 mapping key/value。

不要把 `keccak256(0,64)` 一律当 mapping。它还可能是别的 hash 用途。需要结合 `sload/sstore` 和 scratch memory 写入形状。

不要把 mapping 的具体 key 值作为 field name：

```text
slot:0.map[key=a]
```

这会导致同一个 mapping 的不同 key 变成不同字段，类型无法合并。key 应该生成约束：

```text
a <: slot:0.map.key
```

不要把 dynamic array 的每个 index 当字段：

```text
slot:0.array[i]
```

动态 index 应该约束到 `array.index`，元素类型约束到 `array.elem`。

不要把 packed slot 的整个 256-bit value 当字段类型来源。packed 字段要按 bit 区间单独建 cell。

## 17. 判断标准

实现后可以用这些源码形状做小样例：

- direct slot：`uint256 totalSupply`。
- mapping：`mapping(address => uint256) balanceOf`。
- nested mapping：`mapping(address => mapping(address => uint256)) allowance`。
- static array：`uint256[3] xs`。
- dynamic array：`uint256[] xs`。
- mapping to array：`mapping(address => uint256[]) history`。
- array to mapping：`Bucket[] buckets; mapping inside Bucket`。
- mapping to struct：`mapping(address => User) users`。
- packed field：`bool + uint8 + address` packed into one slot。
- string/bytes：`string name`。
- string array：`string[] names`。

每个样例至少检查：

- storage path 是否稳定打印。
- key/index/value 是否各自进了类型约束。
- load/store 是否通过同一个 cell 连起来。
- packed 字段没有和整个 slot 混掉。
- bytes/string short/long 是否合到同一个 bytes object。

性能上，path normalization 必须有缓存。一个 `sload/sstore` 最多沿 def-use 向前看有限深度，不要全函数反复扫 scratch memory。


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

所以接入类型推理时，关键不是把 `evm_sload(key)` 直接当普通 load，而是先把 `key` 解释成稳定的 storage 字符串字段，再给这个字段建普通类型变量。

## 总原则

不要把 storage 塞进普通线性内存。

建议单独建一个全局 storage root type，形式上仍然复用 `TMemObject` / record field。storage 路径不要做成 binarysub 里的新类型，只编码成普通字符串 field name：

```text
slot:0
slot:1.map
slot:2.array
slot:3.packed@160:8
slot:4.bytes
```

这样做的好处是：

- 仍然能用 SimpleSub 的 record field 约束。
- 和普通 pointer + offset 的模式一致，容易接进现有代码。
- field name 里保留 Solidity storage 地址语义，不把 mapping / array / packed 混成普通 `uint256` 地址。

这里要再单独强调一条：`StorageRoot` 应该当成全局对象处理。

也就是说，不管某个 storage 访问是从 level = 0 的普通函数里出现，还是因为一个 level > 0 的多态函数实例化出来的，`StorageRoot` 以及它下面所有由字符串 field name 创建出来的类型变量，都应该落在 `level = 0`。

这条规则的目的很直接：

- storage 不是某个局部 let 绑定的临时值。
- storage 代表的是合约整体的全局状态。
- 多态函数只是“碰到”它，不是“拥有”它。

所以这里不应该让 `freshenAbove` 把 storage field 复制成更高 level 的新变量，也不应该让 `StorageRoot` 随调用点抬高 level。更稳妥的做法是，storage 相关字段变量在建模时就固定为全局层，再通过普通约束把它和当前调用点的 key / value / index 连起来。

大致类型形状：

```text
StorageRoot <: {
  "slot:0": Slot0Obj,
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
field name: slot:0

StorageRoot <: { "slot:0": Slot0Obj }
Slot0Obj <: make_ptr_store(x, 256)
Slot0Obj <: make_ptr_load(y, 256)
```

这里的 `Slot0Obj` 不是一个新造的值类型，而是直接复用
`external/binarysub/include/binarysub/binarysub-core.h` 里的
`TMemObject` 表达方式。`make_ptr_store(x, 256)` 和
`make_ptr_load(y, 256)` 内部都是带 `directStore` / `directLoad` 的
`TMemObject`。

这和当前 LLVM `load/store` visitor 的写法一致：

```text
load  ptr -> addSubtype(PtrVal, make_ptr_load(RetVal, BitSize))
store ptr -> addSubtype(PtrVal, make_ptr_store(StoreVal, BitSize))
```

同一个 `Slot0Obj` 同时有 directStore 和 directLoad 时，binarysub 现有规则会推出：

```text
x <: y
```

如果 `x` 还被 `evm_balance`、`evm_call` 的 address 参数使用，或者被 address mask 清理，就会继续产生：

```text
x <: address_like_type
```

最后这些约束都会汇到 `Slot0Obj` 的 direct store/load value 上。storage 这层不需要提前判断它是 `uint256` 还是 `address`。

后面例子里如果写 `Map.value`、`Array.elem`、`PackedField`，它们只是在说明 layout 里的哪个字符串字段。真正落到 binarysub 时，只要这个字段会被 `sload/sstore` 访问，就从 `StorageRoot` 上取这个 field name 对应的普通类型变量，再用 `directLoad/directStore` 连接具体 value。

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
field name: slot:0.map

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
field name: slot:1.map.map

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
field name: slot:2
```

这可以退化成 direct slot。

如果 `i` 是动态值，不能给每个 index 建无限 field。建议用数组元素模型：

```text
field name: slot:0.static_array

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
field name: slot:0.dynamic_array

LengthObj(slot:0) <: make_ptr_load(len, 256)
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
field name: slot:2.map.value.dynamic_array

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

这个例子说明 field name 不能只用最终 `keccak` 值。必须保留完整路径信息：

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
field name: slot:0.dynamic_array.elem.field:values.map

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
slot:0
slot:1
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
field name: slot:5.map.value.field@slot+1

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

类型接入时不要只给整个 slot 一个 field。要给 packed field 单独建 field：

```text
field name: slot:0.packed@offset:width

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
slot:3.map.value.field@slot+0.packed@16:160
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
field name: slot:0.bytes

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
slot:0
slot:keccak(0).array
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
field name: slot:0.dynamic_array.elem.bytes

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
slot:4.map.value.field@slot+1.dynamic_array

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

这类组合是接入方式必须支持的重点。不要给每种组合写死一个独立类型规则。更好的做法是在 matcher 内部统一维护 path segment：

```text
slot(4)
  -> map(key=a)
  -> value
  -> field(slot+1)
  -> dynamic_array(index=i)
  -> elem
```

最后把 path segment 序列编码成字符串 field name。

## 13. 建议的 Path Segment

内部可以先用结构化 path，最后再拼成字符串。注意，这个 `StoragePath` 只是 matcher / normalization 里的临时结构，不进入 binarysub 类型系统。binarysub 最终只看到 `StorageRoot` 的字符串字段和普通 `SimpleType` 变量。

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
slot:4.map.value.field@slot+1.dynamic_array.elem
```

对应 side constraints：

```text
a <: slot:4.map.key
i <: slot:4.map.value.field@slot+1.dynamic_array.index
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
  if slot can be normalized to storage field name:
    FieldTy = getOrCreateStorageField(fieldName, level=0)
    FieldTy <: make_ptr_load(result, 256)
  else:
    slot <: storage_key

evm_sstore(slot, value):
  if slot can be normalized to storage field name:
    FieldTy = getOrCreateStorageField(fieldName, level=0)
    FieldTy <: make_ptr_store(value, 256)
  else:
    slot <: storage_key
```

这里的 `getOrCreateStorageField(fieldName, level=0)` 不需要引入新抽象。它可以只是：

1. 在 `StorageRoot` 上查找这个字符串 field name。
2. 没有就创建一个 level 0 的普通 `SimpleType` 变量。
3. 对 `StorageRoot` 加一条 record field 约束，让这个字段进入最终类型结果。
4. 返回这个普通变量，后面继续用 `make_ptr_load` / `make_ptr_store`。

如果字段路径里有 key/index：

```text
key <: KeyType(fieldName prefix)
index <: IndexType(fieldName prefix)
```

这里的 level 规则必须固定：

```text
CurrentLevel = CG.lvl          // 当前函数 / 多态实例自己的 level
StorageLevel = 0              // storage root 和所有 storage field 变量永远用 0

key/index/value/result 使用它们原本的当前类型节点。
StorageRoot、StorageField(fieldName)、KeyType(fieldName)、IndexType(fieldName) 都用 StorageLevel 创建。
```

例子：

```solidity
function read(address a) internal view returns (uint256) {
    return balanceOf[a];
}
```

如果 `read` 作为多态 summary 在 caller 里实例化，`a` 和返回值可以有当前实例的 level。但 `balanceOf` 对应的 `"slot:0.map.key"` / `"slot:0.map.value"` 字段变量仍然必须是 level 0。否则同一个 storage mapping 会被不同调用点复制成多个类型对象，最后无法汇总成合约级 layout。

如果只识别到 `sha3(key, base)`，但 base 还不是常量，也不要丢。可以先建一个相对字段名：

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

只有 debug 输出可以再打印完整路径。

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

不要把 packed slot 的整个 256-bit value 当字段类型来源。packed 字段要按 bit 区间单独建 field object。

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

- storage 字符串字段是否稳定打印。
- key/index/value 是否各自进了类型约束。
- load/store 是否通过同一个 storage 字符串字段变量的 directLoad/directStore 连起来。
- packed 字段没有和整个 slot 混掉。
- bytes/string short/long 是否合到同一个 bytes object。

性能上，storage field normalization 必须有缓存。一个 `sload/sstore` 最多沿 def-use 向前看有限深度，不要全函数反复扫 scratch memory。

## 实现记录：Direct Slot 最小接入

本次先实现 direct slot，不处理 mapping / array / packed / bytes 的 normalization。

修改点：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:105`：给 `ConstraintsGenerator` 增加共享的 `StorageType` 和 `StorageFields`。它们表示合约全局 storage root 和字符串字段表。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:133`：扩展 `ConstraintsGenerator` 构造函数，允许从 `MLsubRecovery` 传入共享 storage root。默认参数保留旧单测构造方式。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:315`：声明 `getOrCreateStorageField()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:454`：给 `MLsubRecovery` 增加全局 `StorageType` 和 `StorageFields`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2224`：实现 `ConstraintsGenerator::getOrCreateStorageField()`。它按字符串字段创建 level 0 普通 `SimpleType` 变量，并把字段挂到 `StorageType` 的 record 约束上。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2519`：在 `MLsubRecovery::run()` 初始化 `StorageType`，level 固定为 0。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3022`：`bottomUpPhase()` 把同一个 `StorageType` / `StorageFields` 传给每个 SCC 的 `ConstraintsGenerator`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3283`：`topDownPhase()` 后释放 storage root 并清空字段表。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4107`：增加 direct slot 字段名生成，当前只接受能落到 `uint64_t` 的常量 slot，格式是 `slot:N`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4162`：`evm_sload(const_slot)` 给对应字段加 `make_ptr_load(result, 256)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4173`：`evm_sstore(const_slot, value)` 给对应字段加 `make_ptr_store(value, 256)`。
- `unittests/Retypd/MLsubGeneratorTest.cpp:86`：新增 `EVMStorageDirectSlotConnectsStoreToLoad`，验证同一个 storage 字符串字段上的 store/load 会推出 `stored <: loaded`。

验证：

```bash
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec /tmp/notdec-storage-direct.ll \
  -o /tmp/notdec-storage-direct-out.ll --tr-level=2 -g \
  --work-dir=/tmp/notdec-storage-direct-work2
rg -n '\[storage:field\]|slot:0|store\[256\]|load\[256\]' \
  /tmp/notdec-storage-direct-work2/binarysub-trace.log
```

smoke IR 内容是：

```llvm
target triple = "evm"

declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define i256 @direct_slot(i256 %x) {
entry:
  call void @evm_sstore(i256 0, i256 %x)
  %y = call i256 @evm_sload(i256 0)
  ret i256 %y
}
```

trace 里能看到 `slot:0` 字段、`store[256]` 和 `load[256]` 都挂在同一个 storage 字段变量上。`ValueTypes.txt` 里 `direct_slot` 的 lower type 变成 `'j -> 'j`，说明 store 输入和 load 返回已经连通。

当前边界：

- 只处理常量 direct slot。
- `keccak(key, slot)` mapping 已在下一步补上。
- `keccak(slot) + index` array、packed bit field、bytes/string short/long 还没实现。
- 还没有把 storage root 作为单独 HType 结果导出；本次只让 value 约束参与求解。

## 实现记录：Mapping 最小接入

本次继续实现一层 mapping。只处理 Solidity 常见 scratch memory 形状：

```text
mstore(0, key)
mstore(32, constant_base_slot)
slot = keccak256(0, 64)
sload/sstore(slot, value)
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2`：引入 `Passes/evm/SolidityPatternUtils.h`，复用已有 Solidity storage matcher。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:317`：`getOrCreateStorageField()` 增加可选 `BitSize`，让 mapping key 字段能直接建成 256-bit 类型变量。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2225`：按传入 `BitSize` 创建 storage 字段变量。未传时仍用 pointer size，保持 direct slot value 字段的 `TMemObject` 用法。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4117`：新增 mapping 字段识别 helper。它调用 `matchStorageMappingAccess()`，并且目前只接受常量 base slot。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4199`：`evm_sload(mapping_slot)` 生成 `"slot:N.map.key"` 和 `"slot:N.map.value"` 两个字符串字段；key 走 `key <: slot:N.map.key`，value 走 `slot:N.map.value <: make_ptr_load(result, 256)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4220`：`evm_sstore(mapping_slot, value)` 使用同样字段；key 走 `key <: slot:N.map.key`，value 走 `slot:N.map.value <: make_ptr_store(value, 256)`。

验证：

```bash
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest

NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec /tmp/notdec-storage-direct.ll \
  -o /tmp/notdec-storage-direct-out.ll --tr-level=2 -g \
  --work-dir=/tmp/notdec-storage-direct-work

NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec /tmp/notdec-storage-mapping.ll \
  -o /tmp/notdec-storage-mapping-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g --work-dir=/tmp/notdec-storage-mapping-work
```

mapping smoke IR：

```llvm
target triple = "evm"

declare i256 @evm_sha3(i256, i256, i256)
declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)

define i256 @mapping_slot(i256 %key, i256 %v) {
entry:
  store i256 %key, ptr inttoptr (i256 0 to ptr)
  store i256 0, ptr inttoptr (i256 32 to ptr)
  %slot.store = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  call void @evm_sstore(i256 %slot.store, i256 %v)
  store i256 %key, ptr inttoptr (i256 0 to ptr)
  store i256 0, ptr inttoptr (i256 32 to ptr)
  %slot.load = call i256 @evm_sha3(i256 0, i256 0, i256 64)
  %y = call i256 @evm_sload(i256 %slot.load)
  ret i256 %y
}
```

结果：

- direct slot trace 里仍能看到 `slot:0` 同时挂 `store[256]` 和 `load[256]`，`direct_slot` lower type 是 `'j -> 'j`。
- mapping trace 里能看到 `slot:0.map.key` 和 `slot:0.map.value`。`slot:0.map.value` 同时挂 `store[256]` 和 `load[256]`，`mapping_slot` lower type 是 `⊤ -> 'p -> 'p`，说明 value 参数和返回值已经连通。
- mapping smoke 需要 `--frozen-tr-input-ir`。不 frozen 时，优化器可能把 `ptr inttoptr(0)` scratch memory 写入折成不可用形状。

当前边界：

- 一层 mapping 已支持；nested mapping 在下一步补上。
- scratch memory 写入仍按普通内存 visitor 生成约束，所以 smoke trace 里会看到和 `ptr null` / `inttoptr(32)` 相关的既有 size-mismatch 记录。这不是 storage 约束本身的问题，但后面如果要把 mapping smoke 变成稳定单测，最好直接测 matcher 或给 EVM scratch memory 单独建模。

## 实现记录：Nested Mapping 最小接入

本次把 mapping base slot 从“必须是常量”扩成“常量或上一层 mapping hash”。没有引入专门 path object，内部只递归生成字符串前缀。

Solidity 形状：

```solidity
mapping(address => mapping(address => uint256)) public allowance; // slot 1
```

IR 形状：

```text
slot1 = keccak256(owner, 1)
slot2 = keccak256(spender, slot1)
sstore(slot2, v)
y = sload(slot2)
```

生成字段：

```text
slot:1.map.key
slot:1.map.value.map.key
slot:1.map.value.map.value
```

约束含义：

```text
owner   <: slot:1.map.key
spender <: slot:1.map.value.map.key
slot:1.map.value.map.value <: make_ptr_load(y, 256)
slot:1.map.value.map.value <: make_ptr_store(v, 256)
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4117`：新增 `MappingStorageKeyConstraint`，用来保存某个 key 应该约束到哪个字符串字段。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4122`：新增 `StorageFieldPrefixMatch`，只在 matcher 内部临时保存字符串前缀和 key 约束，不进入 binarysub。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4132`：新增 `getStorageFieldPrefixFromBaseSlot()`。常量 base 生成 `slot:N`；如果 base 是 `evm_sha3`，递归匹配上一层 `keccak(key, base)`，然后把前缀扩成 `.map.value`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4172`：`getMappingStorageFieldMatch()` 改为使用递归前缀，并把当前层 key 追加到 `Prefix + ".key"`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4248`：`evm_sload` 对所有 key 约束逐条生成 `key <: fieldName`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4271`：`evm_sstore` 使用同样逻辑。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-nested-mapping.ll \
  -o /tmp/notdec-storage-nested-mapping-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-nested-mapping-work
```

结果：

- trace 里出现 `slot:1.map.key`、`slot:1.map.value.map.key`、`slot:1.map.value.map.value`。
- `slot:1.map.value.map.value` 同时挂 `store[256]` 和 `load[256]`。
- `ValueTypes.txt` 里 `nested_mapping` lower type 是 `⊤ -> ⊤ -> 'q -> 'q`，说明第三个参数 `v` 和返回值已经通过 nested mapping value 连通。

当前边界：

- mapping 链已支持；`mapping => array` 在下一步补上。
- 还不处理 `mapping => struct` 的 `+ field_offset`。
- 递归深度当前是 4，避免异常 IR 造成无限回溯。这个值只是保守上限，后面遇到更深真实样例再调。
- scratch memory 的普通内存约束问题仍然存在，和上一节一样。

## 实现记录：Dynamic Array 最小接入

本次接入动态数组元素访问，以及 mapping value 是动态数组的组合。仍然只用字符串字段：

```text
slot:0.dynamic_array.index
slot:0.dynamic_array.elem
slot:2.map.key
slot:2.map.value.dynamic_array.index
slot:2.map.value.dynamic_array.elem
```

Solidity 形状：

```solidity
uint256[] public xs;                    // slot 0
mapping(address => uint256[]) history;  // slot 2
```

IR 形状：

```text
data_base = keccak256(slot)
elem_slot = data_base + i
sstore(elem_slot, v)
y = sload(elem_slot)

arr_slot = keccak256(a, 2)
data_base = keccak256(arr_slot)
elem_slot = data_base + i
y = sload(elem_slot)
```

约束含义：

```text
i <: slot:0.dynamic_array.index
slot:0.dynamic_array.elem <: make_ptr_load(y, 256)
slot:0.dynamic_array.elem <: make_ptr_store(v, 256)

a <: slot:2.map.key
i <: slot:2.map.value.dynamic_array.index
slot:2.map.value.dynamic_array.elem <: make_ptr_load(y, 256)
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4132`：新增 `ArrayStorageFieldMatch`，保存数组字符串前缀、可选 index 和继承自外层 mapping 的 key 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4202`：新增 `getDynamicArrayIndex()`，从 `data_base + i` 或 `evm_add(data_base, i)` 里提取 index；提不出来时只建 elem，不建 index。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4234`：新增 `getArrayStorageFieldMatch()`，复用 `matchStorageArrayDataAccess()`，再用 `getStorageFieldPrefixFromBaseSlot()` 生成 `slot:N.dynamic_array` 或 `slot:N.map.value.dynamic_array`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4316`：`evm_sload(array_slot)` 生成外层 mapping key、array index 和 elem load 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4358`：`evm_sstore(array_slot, value)` 生成同样字段，并把 elem 接到 `make_ptr_store(value, 256)`。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4

NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-dynamic-array.ll \
  -o /tmp/notdec-storage-dynamic-array-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-dynamic-array-work

NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-mapping-array.ll \
  -o /tmp/notdec-storage-mapping-array-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-mapping-array-work
```

结果：

- dynamic array trace 里出现 `slot:0.dynamic_array.index` 和 `slot:0.dynamic_array.elem`，elem 同时挂 `store[256]` 和 `load[256]`。
- mapping array trace 里出现 `slot:2.map.key`、`slot:2.map.value.dynamic_array.index`、`slot:2.map.value.dynamic_array.elem`，elem 同时挂 `store[256]` 和 `load[256]`。
- `dynamic_array` lower type 是 `⊤ -> 'p -> 'p`，说明 value 参数和返回值连通。

当前边界：

- length slot 还没有接到 `dynamic_array.length`。现在如果有 `sload(slot)`，仍然会先按 direct slot 处理。
- 只识别简单的 `data_base + i` / `evm_add(data_base, i)`。数组元素是 struct、或者 index 带乘法/字段偏移时还没处理。
- static array 在下一步补上；常量 index 仍会退化成 direct slot。

## 实现记录：Static Array 最小接入

本次接入静态数组的动态 index 形状：

```text
slot = constant_base + i
sstore(slot, v)
y = sload(slot)
```

生成字段：

```text
slot:0.static_array.index
slot:0.static_array.elem
```

约束含义：

```text
i <: slot:0.static_array.index
slot:0.static_array.elem <: make_ptr_load(y, 256)
slot:0.static_array.elem <: make_ptr_store(v, 256)
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4138`：新增 `StaticArrayStorageFieldMatch`，保存静态数组前缀和 index。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4260`：新增 `getStaticArrayStorageFieldMatch()`，识别 `constant + i` 和 `evm_add(constant, i)`。如果两边都是常量或都不是常量，就不匹配。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4373`：`evm_sload(static_array_slot)` 生成 index 和 elem load 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4427`：`evm_sstore(static_array_slot, value)` 生成 index 和 elem store 约束。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-static-array.ll \
  -o /tmp/notdec-storage-static-array-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-static-array-work
```

结果：

- trace 里出现 `slot:0.static_array.index` 和 `slot:0.static_array.elem`。
- `slot:0.static_array.elem` 同时挂 `store[256]` 和 `load[256]`。
- `static_array` lower type 是 `⊤ -> 'k -> 'k`，说明 value 参数和返回值已经连通。

当前边界：

- 静态数组长度还没有记录。后面如果能从 bounds guard 看到 `i < N`，长度适合放 side table，再由 HType 使用。
- 常量 index 仍按 direct slot，例如 `xs[2]` 会显示成 `slot:2`，不会反推出它属于 `slot:0.static_array`。
- `base + i * elem_size + field_offset` 这种数组元素 struct 还没有处理。

## 实现记录：Packed Field 读取侧最小接入

本次只接入 packed 字段的读取侧，处理直接 slot 上的常见形状：

```text
word = sload(0)
field = (word >> bit_offset) & ((1 << bit_width) - 1)
```

生成字段：

```text
slot:0.packed@16:160
```

约束含义：

```text
slot:0.packed@16:160 <: extracted_value
```

这里字段变量仍然建成 256-bit。`160` 只写进字段名，因为 EVM IR 里的提取结果仍是 `i256`，如果把字段变量建成 160-bit，会和现有 SimpleSub 约束大小不一致。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4143`：新增 `PackedStorageFieldReadMatch`，保存 packed 字段名和提取后的 value。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4299`：新增 `getLowMaskBitWidth()`，识别低位连续 1 的 mask，并从 mask 推出 bit width。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4319`：新增 `getBitwiseAndOtherOperand()`，识别 LLVM `and` 或 `evm_and`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4347`：新增 `getShiftedValue()`，识别 `evm_shr(offset, word)` 和 LLVM `lshr`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4373`：新增 `getPackedReadFromExtractedValue()`，把 `and(shr(sload), mask)` 规约成 `packed@offset:width`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4395`：新增 `getPackedStorageFieldReadMatch()`，从 `evm_sload` 的直接 user 或下一层 user 里找 packed read。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4510`：`evm_sload` 在 direct slot fallback 前优先尝试 packed read，命中后不再给 whole slot 加 `make_ptr_load`。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-packed-read.ll \
  -o /tmp/notdec-storage-packed-read-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-packed-read-work2
```

smoke IR：

```llvm
target triple = "evm"

declare i256 @evm_sload(i256)
declare i256 @evm_shr(i256, i256)

define i256 @packed_read() {
entry:
  %word = call i256 @evm_sload(i256 0)
  %shifted = call i256 @evm_shr(i256 16, i256 %word)
  %owner = and i256 %shifted, 1461501637330902918203684832716283019655932542975
  ret i256 %owner
}
```

结果：

- trace 里出现 `slot:0.packed@16:160`。
- `slot:0.packed@16:160` 约束到 `%owner`，随后通过返回值约束连到函数返回。
- 这个 smoke 没有再生成 whole `slot:0` 的 `load[256]`，避免把整个 packed word 当字段 value。

当前边界：

- clear-mask / shift / or 写入在下一步补上。
- 只处理 direct slot，不处理 `mapping value struct` 里的 `field@slot+0.packed@...`。
- 只处理低位连续 1 的 mask 和一层 `shr`。Solidity 其它优化形状需要继续补 matcher。

## 实现记录：Packed Field 写入侧最小接入

本次接入 packed 字段写入侧的常见形状：

```text
old = sload(0)
cleared = old & clearMask
new_bits = value << bit_offset
new = cleared | new_bits
sstore(0, new)
```

生成字段和约束：

```text
value <: slot:0.packed@16:160
```

这个匹配必须同时满足：

- `old` 来自同一个 slot 的 `sload`。
- `clearMask` 的 0 bit 区间是连续的一段。
- `value << offset` 的 offset 和 clearMask 的 0 bit 起点一致。

如果不满足这些条件，就不按 packed 字段处理，避免把普通位运算误当成 storage layout。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4148`：新增 `PackedStorageFieldWriteMatch`，保存 packed 字段名和写入 value。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4324`：新增 `PackedBitRange`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4329`：新增 `getClearMaskBitRange()`，从 clear mask 中提取连续 0 bit 区间。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4392`：新增 `getBitwiseAndOtherOperandForClearMask()`，识别 `old & clearMask`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4421`：新增 `getBitwiseOrOperands()`，识别 LLVM `or` 或 `evm_or`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4468`：新增 `getShiftedWriteValue()`，识别 `value << offset` 或 `evm_shl(offset, value)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4541`：新增 `getSameSlotSLoad()`，确认 clear part 保留的是同一个 storage slot 的旧值。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4555`：新增 `getPackedStorageFieldWriteMatch()`，把 `old & clearMask | value << offset` 规约成 `packed@offset:width`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4598`：新增 `isPackedWritePreservedWordLoad()`，避免写入前的 `old = sload(slot)` 再生成 whole-slot `load[256]`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4730`：`evm_sload` 对 packed 写入里的 preserved word 直接跳过。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4787`：`evm_sstore` 在 direct slot fallback 前优先尝试 packed write，命中后生成 `value <: packedField`。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-packed-write.ll \
  -o /tmp/notdec-storage-packed-write-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-packed-write-work3
```

smoke IR：

```llvm
target triple = "evm"

declare i256 @evm_sload(i256)
declare void @evm_sstore(i256, i256)
declare i256 @evm_shl(i256, i256)
declare i256 @evm_shr(i256, i256)

define i256 @packed_write_read(i256 %owner) {
entry:
  %old = call i256 @evm_sload(i256 0)
  %cleared = and i256 %old, -95780971304118053647396689196894323976171195136409601
  %shifted.store = call i256 @evm_shl(i256 16, i256 %owner)
  %new = or i256 %cleared, %shifted.store
  call void @evm_sstore(i256 0, i256 %new)
  %word = call i256 @evm_sload(i256 0)
  %shifted.load = call i256 @evm_shr(i256 16, i256 %word)
  %loaded = and i256 %shifted.load, 1461501637330902918203684832716283019655932542975
  ret i256 %loaded
}
```

结果：

- trace 里出现 `slot:0.packed@16:160`。
- `%owner` 约束到 `slot:0.packed@16:160`，读取侧 `%loaded` 也从同一个字段出来。
- `packed_write_read` lower type 是 `'r -> 'r`，说明 packed 写入值和读取返回已经连通。
- 写入前的 preserved word 没有再生成 whole `slot:0` 的 direct load；没有把整 slot word 混入 packed 字段 value。

当前边界：

- 只处理 direct slot packed write。
- 不处理 mapping value struct 里的 packed write，例如 `slot:3.map.value.field@slot+0.packed@16:160`。
- 不处理更复杂的 mask 生成方式，例如 mask 不是常量，或者 clear / shift / or 被 select、phi、helper call 拆开。

## 实现记录：Bytes/String Length 最小接入

本次只接入 bytes/string 的长度读取，不接入数据内容。

Solidity 的 `bytes` / `string` 在 storage 里会把 length 和 short data 编码到同一个 slot。常见长度恢复形状是：

```text
word = sload(slot)
is_short = (word & 1) == 0
full_len = word / 2
short_len = full_len & 127
len = select is_short, short_len, full_len
```

本次把这个 `len` 接到独立字段：

```text
slot:0.bytes.length <: len
```

这样不会把整个 `word = sload(slot)` 当成 `slot:0` 的普通 direct load，也不会把 short data 和 length 混在一个字段里。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4153`：新增 `BytesStorageLengthMatch`，保存 bytes/string length 字段名和最终 length 值。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4447`：新增 `binaryOpHasSameValueOperand()`，复用到 bytes length matcher，判断 `and` 的其中一边是不是原 slot word。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4453`：新增 `isBytesLengthFullLen()`，识别 `evm_div(word, 2)`、`evm_shr(1, word)` 和 LLVM `lshr word, 1`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4481`：新增 `isBytesLengthShortLen()`，识别 `full_len & 127`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4491`：新增 `matchStorageBytesLengthValue()`，识别 `select((word & 1) == 0, short_len, full_len)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4620`：新增 `getBytesStorageLengthMatch()`，从 `evm_sload` 的 users 里向后找 length decode。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4841`：`evm_sload` 在 packed read 和 direct slot fallback 前优先尝试 bytes length，命中后生成 `slot:N.bytes.length`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5220`：`visitSelectInst()` 改用 `getOrInsertNode()`，允许 storage matcher 先给 `select` 结果加约束，visitor 之后再补普通 select 约束。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-length.ll \
  -o /tmp/notdec-storage-bytes-length-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-bytes-length-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

smoke IR：

```llvm
target triple = "evm"

declare i256 @evm_sload(i256)
declare i256 @evm_shr(i256, i256)

define i256 @bytes_length() {
entry:
  %word = call i256 @evm_sload(i256 0)
  %low = and i256 %word, 1
  %is.short = icmp eq i256 %low, 0
  %full = call i256 @evm_shr(i256 1, i256 %word)
  %short = and i256 %full, 127
  %len = select i1 %is.short, i256 %short, i256 %full
  ret i256 %len
}
```

结果：

- trace 里出现 `slot:0.bytes.length`。
- 没有生成 whole `slot:0` 的 direct load。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 只处理 direct slot 的 bytes/string length。
- 还不处理 short data、long data element。
- 还不处理 mapping / array / struct 里的 bytes/string。
- 还不能区分 bytes 和 string，字段名暂时统一用 `.bytes.length`。

## 实现记录：Dynamic Array Length 有证据接入

本次把动态数组的 length slot 接进 storage 字段，但只在同一个函数里已经看到同一个 base slot 的数组元素访问时生效。

例如：

```text
len = sload(0)
base = keccak256(0)
slot = base + i
sstore(slot, v)
```

生成：

```text
slot:0.dynamic_array.length <: make_ptr_load(len, 256)
i <: slot:0.dynamic_array.index
v <: slot:0.dynamic_array.elem
```

mapping 到动态数组也使用同一套规则：

```text
arr = keccak256(key, 2)
len = sload(arr)
base = keccak256(arr)
slot = base + i
sstore(slot, v)
```

生成：

```text
key <: slot:2.map.key
slot:2.map.value.dynamic_array.length <: make_ptr_load(len, 256)
i <: slot:2.map.value.dynamic_array.index
v <: slot:2.map.value.dynamic_array.elem
```

这里故意不处理只有 `sload(slot)` 的 `xs.length`。单看 IR 时它和普通 `uint256 public x` 完全一样，除非有 layout metadata 或别处能看到 `keccak(slot)` 元素访问，否则不应该猜。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4138`：新增 `ArrayLengthStorageFieldMatch`，保存动态数组前缀和 mapping key 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4280`：新增 `functionHasArrayDataAccessForBaseSlot()`，扫描同一函数里的 `evm_sload` / `evm_sstore`，确认是否存在同 base slot 的 `keccak(base)+index` 元素访问。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4318`：新增 `getArrayLengthStorageFieldMatch()`，把 length slot 规约到 `slot:N.dynamic_array.length` 或 `slot:N.map.value.dynamic_array.length`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4861`：`evm_sload` 在 mapping/direct fallback 前优先尝试动态数组 length。这样 `mapping => dynamic array` 的 length slot 不会被误当成 mapping value 的 whole load。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-dynamic-array-length.ll \
  -o /tmp/notdec-storage-dynamic-array-length-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-dynamic-array-length-work
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-mapping-array-length.ll \
  -o /tmp/notdec-storage-mapping-array-length-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-mapping-array-length-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- direct array smoke 里出现 `slot:0.dynamic_array.length`、`slot:0.dynamic_array.index`、`slot:0.dynamic_array.elem`。
- mapping array smoke 里出现 `slot:2.map.key`、`slot:2.map.value.dynamic_array.length`、`slot:2.map.value.dynamic_array.index`、`slot:2.map.value.dynamic_array.elem`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 只用同函数内元素访问作为 length 的证据。跨函数 “一个函数只读 length，另一个函数读 elem” 还不能合并。
- 只有 `xs.length` 的函数仍会按 direct slot 处理。
- 还不处理动态数组元素是 struct 时的 `base + i * elem_slots + field_offset`。

## 实现记录：Mapping Value Struct Slot Offset 最小接入

本次接入 mapping value 里的 struct 字段偏移，处理这种形状：

```text
base = keccak256(key, 5)
slot = base + 1
y = sload(slot)
```

生成：

```text
key <: slot:5.map.key
slot:5.map.value.field@slot+1 <: make_ptr_load(y, 256)
```

写入同理：

```text
v <: slot:5.map.value.field@slot+1
```

这里没有尝试恢复源码字段名，只保留 slot offset。`field@slot+1` 后面可以继续挂 packed 字段，例如以后处理 `field@slot+0.packed@16:160`。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4127`：`MappingStorageFieldMatch` 新增 `ValueFieldName`，让 mapping value 可以指向 `.value` 或 `.value.field@slot+N`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4201`：新增 `getConstantOffsetFromBase()`，识别 `base + 常量` 和 `evm_add(base, 常量)`，递归深度限制为 2。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4246`：`getMappingStorageFieldMatch()` 根据 `StorageSlot` 相对 `keccak` hash 的常量偏移生成 value 字段名。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4934`：`evm_sload` 使用 `Mapping->ValueFieldName` 生成 mapping value load 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4996`：`evm_sstore` 使用 `Mapping->ValueFieldName` 生成 mapping value store 约束。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-mapping-struct-field.ll \
  -o /tmp/notdec-storage-mapping-struct-field-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-mapping-struct-field-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- trace 里出现 `slot:5.map.key`。
- trace 里出现 `slot:5.map.value.field@slot+1`。
- 读写同一个 `field@slot+1` 后，返回值和写入值连通。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- offset 0 仍然显示成 `.value`，因为只看 `keccak(key, slot)` 无法区分 `mapping(address => uint256)` 和 `mapping(address => Struct).field0`。
- 只处理 mapping value 的常量 slot offset。
- 还不处理动态数组元素 struct 的 `base + i * elem_slots + field_offset`。
- 还不处理 mapping value struct 里的 packed read/write。

## 实现记录：Mapping Value Packed Field 最小接入

本次接入 mapping value slot 里的 packed 字段，处理这种形状：

```text
slot = keccak256(key, 3)
word = sload(slot)
owner = (word >> 16) & ((1 << 160) - 1)
```

以及写入：

```text
old = sload(slot)
cleared = old & clearMask
new = cleared | (owner << 16)
sstore(slot, new)
```

生成：

```text
key <: slot:3.map.key
owner <: slot:3.map.value.field@slot+0.packed@16:160
slot:3.map.value.field@slot+0.packed@16:160 <: loaded_owner
```

这里使用 `.field@slot+0.packed@...`，是为了保留“mapping value 的第 0 个 storage slot 里的 packed 子字段”。它不表示已经能区分源码里的 `mapping(address => uint8)` 和 `mapping(address => Struct).field0`；只是避免把 packed 字段和整个 mapping value 混在一起。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4127`：新增 `StorageValueFieldPrefixMatch`，保存字段名前缀和 mapping key 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4154`：`PackedStorageFieldReadMatch` 新增 key 约束，packed read 命中 mapping slot 时也能约束 key。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4160`：`PackedStorageFieldWriteMatch` 新增 key 约束，packed write 同理。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4250`：新增 `getStorageValueFieldPrefixFromSlotExpression()`，从 storage slot 表达式本身识别 direct slot、`keccak(key, slot)`、`keccak(key, slot)+常量`。这是为了处理 packed write 里 `sstore` 前面已有 `old = sload(slot)`，普通 mapping matcher 向前扫描会被这个 `sload` 截断的问题。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4345`：新增 `getStorageValueFieldPrefixMatch()`，统一给 packed matcher 提供字段名前缀。direct slot 保持 `slot:0`；mapping base slot 在 packed 场景下使用 `slot:N.map.value.field@slot+0`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4781`：packed read 使用通用字段前缀，字段名可生成 `slot:3.map.value.field@slot+0.packed@16:160`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4881`：packed write 使用通用字段前缀，并把 key 约束带回 visitor。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5028`：`evm_sload` 先跳过 packed write preserved word，避免 `old = sload(slot)` 被 mapping whole load 捕获。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5033`：mapping load 分支里先尝试 packed read，命中后不生成 mapping whole value load。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5107`：mapping store 分支里先尝试 packed write，命中后不生成 mapping whole value store。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-mapping-packed-field.ll \
  -o /tmp/notdec-storage-mapping-packed-field-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-mapping-packed-field-work2
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-packed-write.ll \
  -o /tmp/notdec-storage-packed-write-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-packed-write-regress
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- mapping packed smoke 里出现 `slot:3.map.key`。
- mapping packed smoke 里出现 `slot:3.map.value.field@slot+0.packed@16:160`。
- mapping packed smoke 没有生成 `slot:3.map.value` 的 whole load/store。
- direct packed regression 仍然生成 `slot:0.packed@16:160`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- mapping base slot 的 packed 字段统一显示成 `.field@slot+0.packed@...`，不是源码级字段名。
- 还不处理 mapping value 的非 0 slot packed 字段里更复杂的表达式，例如 offset 不是常量。
- 还不处理动态数组元素 struct 里的 packed 字段。

## 实现记录：Mapping Value Struct Dynamic Array 组合接入

本次接入 plan 里的 `mapping -> struct field -> dynamic_array` 组合：

```text
user_base = keccak256(key, 4)
history_slot = user_base + 1
history_data_base = keccak256(history_slot)
elem_slot = history_data_base + i
y = sload(elem_slot)
```

生成：

```text
key <: slot:4.map.key
i <: slot:4.map.value.field@slot+1.dynamic_array.index
slot:4.map.value.field@slot+1.dynamic_array.elem <: make_ptr_load(y, 256)
```

如果同函数里也读取了 `history_slot`：

```text
len = sload(history_slot)
```

还会生成：

```text
slot:4.map.value.field@slot+1.dynamic_array.length <: make_ptr_load(len, 256)
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4414`：`getArrayStorageFieldMatch()` 在 `BaseSlot` 上优先尝试 `getStorageValueFieldPrefixFromSlotExpression()`。这样 `keccak(key, 4)+1` 可以作为动态数组 base slot，继续拼成 `.dynamic_array`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4479`：`getArrayLengthStorageFieldMatch()` 同样复用 storage value field 前缀，让 `sload(history_slot)` 归到 `.dynamic_array.length`。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-mapping-struct-array.ll \
  -o /tmp/notdec-storage-mapping-struct-array-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-mapping-struct-array-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- trace 里出现 `slot:4.map.key`。
- trace 里出现 `slot:4.map.value.field@slot+1.dynamic_array.length`。
- trace 里出现 `slot:4.map.value.field@slot+1.dynamic_array.index`。
- trace 里出现 `slot:4.map.value.field@slot+1.dynamic_array.elem`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 只处理 base slot 是 `keccak(key, slot)+常量` 的动态数组字段。
- 还不处理动态数组元素是多 slot struct 时的 `base + i * elem_slots + field_offset`。
- 还不处理这类动态数组元素里的 bytes/string 或 packed 字段。

## 实现记录：Array Element Bytes/String Length 最小接入

本次只把 bytes/string 的 length matcher 推广到数组元素，不接入 short data / long data。

处理这种形状：

```text
array_base = keccak256(0)
elem_slot = array_base + i
word = sload(elem_slot)
len = decode_bytes_length(word)
```

生成：

```text
i <: slot:0.dynamic_array.index
slot:0.dynamic_array.elem.bytes.length <: len
```

这里的重点是：`sload(elem_slot)` 不能先被普通 `dynamic_array.elem` whole load 吃掉。只有识别到 Solidity bytes/string length decode 时，才转成 `.elem.bytes.length`。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4166`：`BytesStorageLengthMatch` 新增 key/index 约束列表。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4856`：`getBytesStorageLengthMatch()` 不再只接受 direct slot；先尝试普通 storage value prefix，再尝试 dynamic array elem 和 static array elem。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4862`：dynamic array elem 命中后生成 `ArrayPrefix + ".elem.bytes.length"`，并把 `ArrayPrefix + ".index"` 约束带回 visitor。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5083`：`evm_sload` 把 bytes length 分支放到 mapping/array whole load 前面，避免 bytes/string 元素被当成普通 256-bit 元素。

验证：

```bash
cmake --build ./build --target notdec MLsubGeneratorTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-array-bytes-length.ll \
  -o /tmp/notdec-storage-array-bytes-length-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-array-bytes-length-work
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-length.ll \
  -o /tmp/notdec-storage-bytes-length-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-bytes-length-regress
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- array bytes length smoke 里出现 `slot:0.dynamic_array.index`。
- array bytes length smoke 里出现 `slot:0.dynamic_array.elem.bytes.length`。
- direct bytes length regression 仍然出现 `slot:0.bytes.length`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 只处理 length，不处理 `bytes.short_data`、`bytes.long_elem`。
- 还不能区分 bytes 和 string，字段名仍统一用 `.bytes`。
- array elem 之外的 mapping/struct bytes length 可复用同一前缀逻辑，但还没有单独 smoke。

## 实现记录：EVM bytes/string 语义格

本次只接入类型格，不改 storage path。storage 字段名仍然统一用 `.bytes`：

```text
slot:0.bytes.length
slot:0.bytes.short_data
slot:0.bytes.long_elem
```

`string` 作为更具体的语义类型，不通过 path 名字表达，而是通过 binarysub primitive semantic lattice 表达：

```text
string <: bytes <: root
```

这样同一个 storage 变量可以先保守恢复成 bytes；后续如果 ABI、summary、库函数或其它证据说明它是 string，再把对应值标成 `prim.uint256.evm.string`。这也避免把短编码和长编码拆成两个最终类型，因为 Solidity 同一个 `bytes/string` 变量运行时可以这次短、下次长。

Solidity 例子：

```solidity
contract C {
    string public name;

    function setName(string memory x) public {
        name = x;
    }

    function raw() public view returns (bytes memory) {
        return bytes(name);
    }
}
```

这里 storage 侧仍然是 `slot:0.bytes.*`。`setName` 的 ABI 或 signature 能提供 `string` 证据；`raw()` 的返回值只需要满足 bytes。由于 `string <: bytes`，同一个值可以在需要 bytes 的地方使用。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:273`：在内建 EVM i256 primitive semantic lattice 里新增 `bytes`、`string`，并加入 `string -> bytes -> root`。
- `unittests/Retypd/TypeBuilderTest.cpp:54`：新增 `SemanticPrimitiveStringIsBytesSubtype`，验证 `string <: bytes` 成立、反向不成立，并确认 TypeBuilder 能把 `prim.uint256.evm.string` 转成带 semantic 注释的 typedef。

验证：

```bash
cmake --build ./build --target notdec TypeBuilderTest -j4
./build/bin/TypeBuilderTest
NOTDEC_SUMMARY_OVERRIDE=/tmp/notdec-evm-string-semantic-summary.json \
  ./build/bin/notdec /tmp/notdec-evm-string-semantic.ll \
  -o /tmp/notdec-evm-string-semantic-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-evm-string-semantic-work
```

结果：

- `TypeBuilderTest` 通过。
- smoke 中 `NOTDEC_SUMMARY_OVERRIDE` 能接受 `prim.uint256.evm.string` 和 `prim.uint256.evm.bytes`。
- `/tmp/notdec-evm-string-semantic-work/ValueTypes.txt` 中出现 `prim.uint256.evm.string:256 -> prim.uint256.evm.bytes:256`。
- `/tmp/notdec-evm-string-semantic-work/ValueHTypes.txt` 中出现 `prim.uint256.evm.string` 和 `prim.uint256.evm.bytes` 对应 typedef。

当前边界：

- 这里只接入格本身，还没有新增自动打 `string` 标记的位置。
- 后续优先从 ABI/signature override、public 函数签名、已知 runtime/helper 语义里补 `string` 证据。
- 不把所有 `.bytes` storage 默认提升成 `string`，否则会把 Solidity `bytes` 误判成 `string`。

## 实现记录：Bytes/String Long Data 最小接入

本次只接 long bytes/string 的数据块读取和写入，不处理 short data。

Solidity 同一个 `bytes/string` storage 变量有两种运行时编码：

```solidity
contract C {
    string public name; // slot 0

    function getWord(uint256 i) public view returns (bytes32) {
        return bytes(name)[i];
    }
}
```

long 版本的底层形状是：

```text
word = sload(0)
len = decode_bytes_length(word)
data_base = keccak256(0)
chunk = sload(data_base + i)
```

这里 `keccak256(0) + i` 和动态数组元素形状很像。为了不把普通动态数组误判成 bytes，本次只在同一个 base slot 已经出现 bytes length decode 时，才把 `keccak256(slot) + i` 归到：

```text
slot:0.bytes.long_index
slot:0.bytes.long_elem
```

否则仍然走普通动态数组：

```text
slot:0.dynamic_array.index
slot:0.dynamic_array.elem
```

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4176`：新增 `BytesStorageLongElemMatch`，保存 `.bytes` 前缀、可选 long index 和外层 mapping key 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4930`：新增 `functionHasBytesLengthDecodeForBaseSlot()`，在同一函数内检查 base slot 的 `sload(slot)` 是否被 bytes length decode 使用。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4975`：新增 `getBytesStorageLongElemMatch()`，复用 array data matcher 识别 `keccak(slot)+i`，但额外要求 base slot 有 bytes length decode。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5160`：`evm_sload` 先识别 bytes length，再识别 array length，避免 long bytes 场景里 `slot:0.bytes.length` 被误写成 `slot:0.dynamic_array.length`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5184`：`evm_sload(keccak(slot)+i)` 命中 long bytes 时生成 `.bytes.long_index` 和 `.bytes.long_elem`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5302`：`evm_sstore(keccak(slot)+i, value)` 命中 long bytes 时使用同一组字段。

验证：

```bash
cmake --build ./build --target notdec -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-long-elem.ll \
  -o /tmp/notdec-storage-bytes-long-elem-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-bytes-long-elem-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- trace 里出现 `slot:0.bytes.length`。
- trace 里出现 `slot:0.bytes.long_index`。
- trace 里出现 `slot:0.bytes.long_elem`。
- trace 里没有 `slot:0.dynamic_array.length` 或 `slot:0.dynamic_array.elem`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 还不处理 short bytes/string 的 inline data，也就是 `slot:0.bytes.short_data`。
- long bytes 的识别依赖同函数内能看到 length decode。只看到 `keccak(slot)+i` 时仍按动态数组处理。
- 还没有把 storage root 作为单独 HType 结果导出；本次仍只通过 trace 和 value 约束验证 storage 字段。

## 实现记录：Bytes/String Short Data 最小接入

本次补上 short bytes/string 的 inline data 提取。只处理最直接的形状：

```text
word = sload(slot)
len = decode_bytes_length(word)
short_data = word & ~0xff
```

生成：

```text
slot:0.bytes.length <: len
slot:0.bytes.short_data <: short_data
```

这里仍然要求同一个 slot 已经能识别 bytes length decode。也就是说，单独看到 `word & ~0xff` 不会被当成 short bytes，避免普通 packed/bit mask 误判。

Solidity 例子：

```solidity
contract C {
    bytes public data; // slot 0

    function firstWord() public view returns (bytes32) {
        return bytes32(data);
    }
}
```

短数据时，`data` 的内容在 slot 0 的高 31 字节，低 1 字节放 `length * 2`。所以 `word & ~0xff` 可以作为 inline data 证据。

修改点：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4176`：新增 `BytesStorageShortDataMatch`，保存 `.bytes.short_data` 字段名和提取后的 data value。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4930`：新增 `isBytesShortDataMask()`，只接受低 8 bit 为 0、其余 bit 为 1 的常量 mask。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4950`：新增 `matchStorageBytesShortDataValue()`，识别 `and(word, ~0xff)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4970`：新增 `getBytesStorageShortDataMatch()`，复用 bytes length 的 storage 前缀和 key/index 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5277`：`evm_sload` 的 bytes length 分支里同步生成 `.bytes.short_data` 约束。

验证：

```bash
cmake --build ./build --target notdec -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-short-data.ll \
  -o /tmp/notdec-storage-bytes-short-data-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-bytes-short-data-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- trace 里出现 `slot:0.bytes.length`。
- trace 里出现 `slot:0.bytes.short_data`。
- trace 里没有 `slot:0.dynamic_array.*`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 只处理 `and(word, ~0xff)`，还不处理 shift、byte-copy loop 或 helper 拆出来的 short data。
- short data 目前只是 storage evidence，不负责把 bytes/string 提升成最终 `string`。
- storage root 仍未单独导出为 HType 结果。

## 实现记录：导出 `[storage]` HType 结果

本次把全局 storage root 从 trace 里推进到 HType 输出里。`ValueHTypes.txt` 和 `ImportantHTypes.txt` 现在都有独立的 `[storage]` section。

当前没有把 storage path 直接当 C 字段名。原因是普通 HType record 仍然按 offset range 解析，适合 memory / struct，不适合 `slot:0.bytes.long_elem` 这种 storage path。最小做法是：

```text
[storage]
decl => struct_0
type => struct_0

struct struct_0 {
  top:256 field_0; /* storage path: slot:0.bytes.length */
  u256 field_1;    /* storage path: slot:0.bytes.short_data */
};
```

也就是说，字段顺序稳定，原始 storage path 放在字段注释里。这样不会碰普通 memory 的 `OffsetRange::fromStr()` 路径。

修改点：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:63`：`HTypeResult` 增加 `StorageType` / `StorageDecl`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:100`：`HTypeResult::print()` 增加 `[storage]` section。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:118`：`ConstraintsGenerator` 增加 `StorageHType` / `StorageDecl`，因为 binarysub graph 在 `topDownPhase()` 后会释放，storage 必须在 `genTypes()` 阶段先转成 HType。
- `include/notdec/TypeRecovery/mlsub/TypeBuilder.h:71`：新增 `convertStorageRecord()`。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:900`：实现 storage 专用转换。它不解析 storage 字段名为 `OffsetRange`，而是按 `StorageFields` 的稳定顺序创建 record 字段，并把原始 path 写入 comment。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3121`：`genTypes()` 在第 0 个 SCC 中同时求解 `MemoryType` 和 `StorageType`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3204`：把求解后的 storage UType 交给 `TypeBuilder::convertStorageRecord()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3283`：`genASTTypes()` 把第 0 个 SCC 保存的 storage HType 放入最终 `HTypeResult`。
- `src/TypeRecovery/mlsub/HTypeNormalize.cpp:144`：normalize 时把 `StorageType` 当作全局根处理，并避免把 `StorageDecl` 当成普通透明单字段 record 消掉。
- `src/TypeRecovery/mlsub/HTypeDebug.cpp:244`：`ImportantHTypes.txt` 增加 `[storage]` section。

验证：

```bash
cmake --build ./build --target notdec TypeBuilderTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-short-data.ll \
  -o /tmp/notdec-storage-with-storage-section-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-with-storage-section-work
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-long-elem.ll \
  -o /tmp/notdec-storage-long-storage-section-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-long-storage-section-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- short-data smoke 的 `ValueHTypes.txt` 出现 `[storage]`。
- short-data smoke 的 decl 注释里出现 `storage path: slot:0.bytes.length` 和 `storage path: slot:0.bytes.short_data`。
- long-data smoke 的 decl 注释里出现 `storage path: slot:0.bytes.length`、`storage path: slot:0.bytes.long_elem`、`storage path: slot:0.bytes.long_index`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- `[storage]` 里的字段名仍是 `field_N`，真实 path 在注释里。
- 字段类型是当前 raw HType 结果，可能显示 `ptr<load=..., store=...>`，暂不做 Solidity 展示层美化。
- 顶层当前还有一个 unrelated `external/NotDec-bin2llvm` submodule 指针差异，本次提交不会包含它。

## 实现记录：Storage HType 分层树

本次把 storage 的 HType lowering 从 flat record 改成了前缀树。

还是同样的 binarysub 输入：

```text
slot:4.map.key
slot:4.map.value.field@slot+1.dynamic_array.elem
```

但在 HType 输出里，会先按公共前缀归并：

```text
slot:4
  map
    key
    value
      field@slot+1
        dynamic_array
          elem
```

这样 `[storage]` 里可以看到更深的结构层次，不再只是两个扁平字段。现在的转换规则是：

1. binarysub 约束仍然保持 flat 字符串 field。
2. `TypeBuilder::convertStorageRecord()` 先按 `.` 分段建树。
3. 每个内部节点都变成一个新的 `RecordDecl`。
4. 如果节点既有 leaf 值又有子节点，就额外保留一个该节点自己的字段，避免丢 whole-slot 语义。

这次没有改普通 memory 的 `OffsetRange::fromStr()` 行为，也没有动 binarysub 里的 field 名解析规则。

修改点：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:900`：`convertStorageRecord()` 从 flat record 改成 prefix tree lowering。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:901`：新增 storage path 分段和树节点结构。
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:935`：内部节点递归生成新的 `RecordDecl`。

验证：

```bash
cmake --build ./build --target notdec TypeBuilderTest -j4
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-short-data.ll \
  -o /tmp/notdec-storage-tree-short-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-tree-short-work
NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec \
  /tmp/notdec-storage-bytes-long-elem.ll \
  -o /tmp/notdec-storage-tree-long-out.ll --tr-level=2 \
  --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-storage-tree-long-work
cmake --build ./build --target all -j4
./build/bin/MLsubGeneratorTest
./build/bin/TypeBuilderTest
```

结果：

- short-data smoke 的 `[storage]` 里出现嵌套 `struct_0 -> struct_1`。
- long-data smoke 的 `[storage]` 里同样出现嵌套层次，并保留 `slot:0.bytes.long_elem`、`slot:0.bytes.long_index`。
- `cmake --build ./build --target all -j4`、`MLsubGeneratorTest`、`TypeBuilderTest` 均通过。

当前边界：

- 字段名还没改成可读的路径名，仍是 `field_N`。
- 这个树化只发生在 storage HType lowering，不影响 binarysub 或 memory struct 的 flat range 解析。
- 还没有把这种层次直接映射成 Solidity 风格的高层字段名，只是先把结构分出来。

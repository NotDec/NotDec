# Storage bytes/string 短长编码

## 简介

`bytes` 和 `string` 如果放在 storage 里，会使用专门编码：

```solidity
contract C {
    string public name;
    bytes public data;
}
```

短数据可以直接放在一个 storage slot 里。长数据则把长度放在 base slot，内容放到
`keccak256(base_slot)` 开始的一组 slot。

这套编码比普通动态数组更特殊，因为它要区分短字符串和长字符串。

## 编译过程与低层模式

storage bytes/string 的 base slot 里会带一个标志位。

短数据，也就是长度小于 32 字节时，数据和长度信息都放在 base slot：

```text
slot:
    high bytes: data
    low byte:   length * 2
```

低 bit 为 0 表示短编码。

长数据时，base slot 存长度编码，内容从 hash 地址开始：

```text
slot:
    length * 2 + 1
```

低 bit 为 1 表示长编码。

内容地址：

```text
mstore(0x00, base_slot)
data_base = KECCAK256(0x00, 32)
```

然后第 0 个数据 word 存在 `data_base`，第 1 个存在 `data_base + 1`，依次类推。

读取时，编译器要先看 base slot 的低 bit：

```text
word = SLOAD(base_slot)
if (word & 1) == 0:
    decode_short_bytes(word)
else:
    len = (word - 1) / 2
    data_base = keccak256(base_slot)
    load_long_bytes(data_base, len)
```

如果编码不合法，Solidity 可能触发 `Panic(0x22)`。

## 例子详解

Solidity 源码：

```solidity
contract C {
    bytes public data;

    function setShort() external {
        data = hex"112233";
    }
}
```

编译器要保证的语义是：长度为 3 的 bytes 存进 storage，并能被 ABI getter 正确读出。

短编码大概是：

```text
encoded = 0x112233...0006
SSTORE(data_slot, encoded)
```

低字节 `0x06` 是 `length * 2`，也就是 `3 * 2`。低 bit 是 0，表示短编码。

读取时：

```text
word = SLOAD(data_slot)
if word & 1 == 0:
    len = (word & 0xff) / 2
    bytes = high_bytes(word, len)
```

长 bytes 例子：

```solidity
contract C {
    bytes public data;

    function setLong(bytes calldata x) external {
        data = x;
    }
}
```

如果 `x.length >= 32`，简化写入形状：

```text
SSTORE(data_slot, x.length * 2 + 1)

mstore(0x00, data_slot)
base = KECCAK256(0x00, 32)

SSTORE(base + 0, first_32_bytes)
SSTORE(base + 1, second_32_bytes)
...
```

需要记住的是：storage bytes/string 不是简单的“slot 存长度，hash 区存数据”。
短数据会直接塞进 base slot，只有长数据才使用 hash 后的数据区。

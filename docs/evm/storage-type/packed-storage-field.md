# Packed storage field

## 简介

Solidity 会把多个小状态变量打包到同一个 storage slot 里：

```solidity
contract C {
    uint128 public a;
    uint64 public b;
    bool public c;
}
```

这些变量加起来不超过 32 字节时，编译器可以让它们共享一个 256-bit slot。

这样可以减少 storage 读写次数，节省 gas。

## 编译过程与低层模式

假设三个字段都在 slot 0：

```text
bits 0..127:   a
bits 128..191: b
bit 192:       c
```

读取字段时，编译器会 `SLOAD` 整个 slot，然后 shift/mask：

```text
word = SLOAD(0)
a = word & ((1 << 128) - 1)
b = (word >> 128) & ((1 << 64) - 1)
c = (word >> 192) & 1
```

写字段更复杂，因为不能覆盖同一个 slot 里的其他字段。

写 `b` 的形状：

```text
old = SLOAD(0)
cleared = old & ~(mask64 << 128)
new_part = (b & mask64) << 128
new_word = cleared | new_part
SSTORE(0, new_word)
```

这里的核心是“先清掉旧字段，再把新字段放回对应 bit 区间”。

## 例子详解

Solidity 源码：

```solidity
contract C {
    uint128 public a;
    uint64 public b;

    function setB(uint64 x) external {
        b = x;
    }
}
```

编译器要保证的语义是：

- 只更新 `b`。
- 不能破坏同一个 slot 里的 `a`。
- `x` 只能占 64 bit。

简化 EVM 伪指令：

```text
x = CALLDATALOAD(4) & 0xffffffffffffffff

old = SLOAD(0)

mask = 0xffffffffffffffff << 128
cleared = old & ~mask
new_part = x << 128
new_word = cleared | new_part

SSTORE(0, new_word)
```

读取 `b`：

```text
word = SLOAD(0)
b = (word >> 128) & 0xffffffffffffffff
return abi_encode_uint64(b)
```

如果字段在 mapping 元素里，slot 先由 mapping addressing 算出来，再做同样的 shift/mask：

```text
slot = keccak256(user, base_slot)
word = SLOAD(slot)
field = (word >> offset) & mask
```

需要记住的是：packed storage 的底层代码看起来像一堆位运算，但源码里可能只是普通字段读写。
关键线索是 `SLOAD/SSTORE` 周围的 shift、mask、clear、or。

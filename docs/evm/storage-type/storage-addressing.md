# Storage addressing

## 简介

Solidity 状态变量放在 EVM storage 里：

```solidity
contract C {
    uint256 public a;
    mapping(address => uint256) public balance;
    uint256[] public xs;
}
```

EVM storage 是一个 key-value 表：

```text
SLOAD(slot)
SSTORE(slot, value)
```

每个 `slot` 和 `value` 都是 256 bit。

简单状态变量可以直接用固定 slot。mapping 和动态数组不行，它们需要用 hash 算地址。

## 编译过程与低层模式

简单变量按声明顺序分配 slot：

```solidity
uint256 a; // slot 0
uint256 b; // slot 1
```

读取 `a`：

```text
value = SLOAD(0)
```

mapping 的元素地址用 `keccak256(key, base_slot)`：

```solidity
mapping(address => uint256) balance; // base slot = 0
balance[user]
```

简化 EVM 伪指令：

```text
mstore(0x00, user)
mstore(0x20, 0)
slot = KECCAK256(0x00, 64)
value = SLOAD(slot)
```

动态数组的长度存在 base slot，元素数据从 `keccak256(base_slot)` 开始：

```solidity
uint256[] xs; // base slot = 1
```

读取长度：

```text
len = SLOAD(1)
```

读取 `xs[i]`：

```text
mstore(0x00, 1)
base = KECCAK256(0x00, 32)
value = SLOAD(base + i)
```

嵌套 mapping 或 mapping 到数组，会形成多次 hash 链。

## 例子详解

Solidity 源码：

```solidity
contract C {
    mapping(address => uint256) public balance;

    function get(address user) external view returns (uint256) {
        return balance[user];
    }
}
```

编译器要保证的语义是：不同 `user` 的余额落到不同 storage slot，而且不和其他状态变量冲突。

简化 EVM 伪指令：

```text
user = decode_address_from_calldata()

mstore(0x00, user)
mstore(0x20, 0)          ; balance 的 base slot
slot = KECCAK256(0x00, 64)

value = SLOAD(slot)
return abi_encode_uint256(value)
```

动态数组例子：

```solidity
contract C {
    uint256[] public xs;

    function at(uint256 i) external view returns (uint256) {
        return xs[i];
    }
}
```

简化 EVM 伪指令：

```text
i = decode_uint256_from_calldata()

len = SLOAD(0)
if i >= len:
    panic(0x32)

mstore(0x00, 0)
base = KECCAK256(0x00, 32)
value = SLOAD(base + i)
return abi_encode_uint256(value)
```

需要记住的是：mapping 和动态数组的 storage 地址不是固定常量，而是由 base slot、key、
index 和 `KECCAK256` 组合出来的。

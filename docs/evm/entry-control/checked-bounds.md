# Checked arithmetic 和 bounds check

## 简介

Solidity 0.8 以后，普通整数算术默认带溢出检查：

```solidity
contract C {
    function add(uint256 a, uint256 b) external pure returns (uint256) {
        return a + b;
    }
}
```

如果 `a + b` 溢出，函数不会静默回绕，而是触发 `Panic`。

数组访问也会生成边界检查：

```solidity
function get(uint256[] memory xs, uint256 i) internal pure returns (uint256) {
    return xs[i];
}
```

如果 `i >= xs.length`，也会触发 `Panic`。

## 编译过程与低层模式

EVM 的 `ADD`、`SUB`、`MUL` 默认都是 256-bit 模运算。也就是说，底层 opcode 本身不会报溢出。

Solidity 要实现 checked arithmetic，就要在运算前后补判断。

`uint256` 加法常见形状：

```text
sum = a + b
if sum < a:
    panic(0x11)
return sum
```

`0x11` 是 arithmetic overflow/underflow panic code。

`uint256` 减法常见形状：

```text
if b > a:
    panic(0x11)
diff = a - b
```

除零检查：

```text
if b == 0:
    panic(0x12)
q = a / b
```

数组越界常见形状：

```text
if index >= length:
    panic(0x32)
value = array[index]
```

动态内存分配也会触发资源检查：

```text
if length >= 2^64:
    panic(0x41)
payload_size = round_up(length * 32)
```

enum 转换、小整数转换、bytes/string storage 编码错误也可能生成不同 panic code。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function add(uint256 a, uint256 b) external pure returns (uint256) {
        return a + b;
    }
}
```

编译器要保证的语义是：

- 正常情况下返回数学意义上的 `a + b`。
- 如果结果超过 `uint256` 最大值，就 revert，并带 `Panic(0x11)`。

简化 EVM 伪指令：

```text
sum = ADD(a, b)
if LT(sum, a):
    ptr = mload(0x40)
    mstore(ptr, 0x4e487b71 << 224)
    mstore(ptr + 4, 0x11)
    revert(ptr, 36)

return abi_encode_uint256(sum)
```

如果源码使用 `unchecked`：

```solidity
function addUnchecked(uint256 a, uint256 b) external pure returns (uint256) {
    unchecked {
        return a + b;
    }
}
```

底层通常就只剩：

```text
sum = ADD(a, b)
return abi_encode_uint256(sum)
```

再看数组访问：

```solidity
function at(uint256[] memory xs, uint256 i) internal pure returns (uint256) {
    return xs[i];
}
```

memory 动态数组的第一个 word 通常是长度，后面才是元素：

```text
length = mload(xs)
if i >= length:
    panic(0x32)

value = mload(xs + 32 + i * 32)
```

需要记住的是：这些检查不是用户手写的业务 `if`，而是编译器为了保持 Solidity 语义插入的保护。
它们通常通过 `Panic(code)` 表达失败原因。

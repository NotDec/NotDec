# ABI 返回值编码

## 简介

Solidity 函数返回值也要按 ABI 编码：

```solidity
contract C {
    function f() external pure returns (uint256, bool) {
        return (7, true);
    }
}
```

EVM 没有“返回两个值”的指令。它只有：

```text
RETURN(memory_offset, memory_size)
```

所以编译器要把返回值写进 memory，然后返回这段 memory。

## 编译过程与低层模式

静态返回值每个占 32 字节：

```text
ptr = mload(0x40)
mstore(ptr + 0, value0)
mstore(ptr + 32, value1)
return(ptr, 64)
```

`bool` 也按 32 字节 ABI word 返回，值通常是 `0` 或 `1`。

动态返回值要使用 head/tail：

```solidity
function f() external pure returns (string memory)
```

返回 `"hello"` 的 ABI 形状：

```text
word 0: offset to tail = 32
word 1: length = 5
word 2: bytes("hello") padded to 32 bytes
```

多返回值里只要有动态类型，也会有 head/tail：

```text
return (uint256 x, string s)

word 0: x
word 1: offset to s tail
tail:
    length
    bytes
```

## 例子详解

Solidity 源码：

```solidity
contract C {
    function f() external pure returns (uint256, bool) {
        return (7, true);
    }
}
```

编译器要保证的语义是返回两个 ABI word。

简化 EVM 伪指令：

```text
ptr = mload(0x40)
mstore(ptr + 0, 7)
mstore(ptr + 32, 1)
RETURN(ptr, 64)
```

如果返回动态 string：

```solidity
function hello() external pure returns (string memory) {
    return "hello";
}
```

简化 EVM 伪指令：

```text
ptr = mload(0x40)
mstore(ptr + 0, 32)
mstore(ptr + 32, 5)
mstore(ptr + 64, "hello")
RETURN(ptr, 96)
```

这里 `RETURN(ptr, 96)` 返回的是 ABI 编码后的结果，不是 Solidity 内部 string 对象指针。

需要记住的是：返回值恢复的关键通常不是 `RETURN` 自己，而是它前面那组 `mstore`。
这些 `mstore` 决定返回了几个值、哪些是动态类型、每个值大概是什么宽度。

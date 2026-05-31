# ABI revert encoding

## 简介

Solidity 的错误也会按 ABI 编码：

```solidity
contract C {
    error TooSmall(uint256 got);

    function f(uint256 x) external pure {
        require(x > 10, "too small");
        if (x == 11) {
            revert TooSmall(x);
        }
    }
}
```

底层只有 `REVERT(ptr, size)`。错误类型和参数都放在 memory 的 revert data 里。

## 编译过程与低层模式

ABI revert data 通常从 4 字节 selector 开始。

`Error(string)`：

```text
selector = 0x08c379a0
```

编码形状：

```text
0x00..0x03: selector
0x04..0x23: offset to string tail
0x24..0x43: string length
0x44..    : string bytes
```

`Panic(uint256)`：

```text
selector = 0x4e487b71
```

编码形状：

```text
0x00..0x03: selector
0x04..0x23: panic code
```

custom error：

```solidity
error TooSmall(uint256 got);
```

形状和函数调用参数类似：

```text
0x00..0x03: selector("TooSmall(uint256)")
0x04..0x23: got
```

因为 selector 只有 4 字节，后面的 ABI 参数从 `ptr + 4` 开始。这一点和普通 return buffer
每个 word 从 32 字节边界开始不完全一样。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function f(uint256 x) external pure {
        require(x > 10, "too small");
    }
}
```

编译器要保证的语义是：`x <= 10` 时，revert data 表示 `Error("too small")`。

简化 EVM 伪指令：

```text
if x <= 10:
    ptr = mload(0x40)
    mstore(ptr, 0x08c379a0 << 224)
    mstore(ptr + 4, 32)
    mstore(ptr + 36, 9)
    mstore(ptr + 68, "too small")
    REVERT(ptr, 100)
```

custom error 例子：

```solidity
error TooSmall(uint256 got);

function g(uint256 x) external pure {
    if (x <= 10) {
        revert TooSmall(x);
    }
}
```

简化 EVM 伪指令：

```text
if x <= 10:
    ptr = mload(0x40)
    mstore(ptr, selector("TooSmall(uint256)") << 224)
    mstore(ptr + 4, x)
    REVERT(ptr, 36)
```

custom error 通常比 revert string 省 gas，因为它不需要把完整字符串写进字节码和 memory。

需要记住的是：revert encoding 和 ABI return 很像，都是写 memory buffer；区别是它最后交给
`REVERT`，而且前 4 字节通常是错误 selector。

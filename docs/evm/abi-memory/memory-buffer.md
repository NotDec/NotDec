# Memory buffer 和动态对象

## 简介

Solidity 里经常会构造 bytes、string、数组、返回值、错误数据、外部调用参数：

```solidity
contract C {
    function f(uint256 x) external pure returns (bytes memory) {
        return abi.encode(x, "ok");
    }
}
```

这些数据在 EVM 里通常先写到 memory，然后再交给 `RETURN`、`REVERT`、`CALL` 或 `LOG`。

EVM memory 是临时字节数组。一次调用结束后，memory 不会持久保存。

## 编译过程与低层模式

Solidity 约定 memory 地址 `0x40` 存着 free memory pointer，也就是下一块可用 memory 的起点。

常见分配流程：

```text
ptr = mload(0x40)
new_ptr = ptr + size
mstore(0x40, new_ptr)
```

之后编译器会从 `ptr` 开始写 buffer：

```text
mstore(ptr + 0, first_word)
mstore(ptr + 32, second_word)
mstore(ptr + 64, third_word)
```

ABI 动态对象一般包含长度和数据。例如 `bytes memory b = hex"112233"` 在 memory 中可以理解为：

```text
b + 0:  length = 3
b + 32: data   = 0x112233...
```

动态数组也类似：

```text
arr + 0:  length
arr + 32: element0
arr + 64: element1
```

这里的“buffer”不是 Solidity 源码里的一个固定概念，而是反编译时为了理解一组连续
memory 写入而使用的说法。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function makeBytes() external pure returns (bytes memory) {
        return hex"11223344";
    }
}
```

编译器要保证的语义是：

- 构造一个长度为 4 的动态 bytes。
- 返回值要按 ABI 编码。

简化 memory 形状可以分两层看。

第一层是 bytes 对象本身：

```text
bytes_ptr:
    mstore(bytes_ptr + 0, 4)
    mstore(bytes_ptr + 32, 0x11223344...)
```

第二层是 ABI return buffer。动态返回值不是直接把对象丢出去，而是编码成 head/tail：

```text
ret_ptr:
    mstore(ret_ptr + 0, 32)        ; 第一个返回值的 tail offset
    mstore(ret_ptr + 32, 4)        ; bytes length
    mstore(ret_ptr + 64, 0x11223344...)
    return(ret_ptr, 96)
```

简化 EVM 伪指令：

```text
ret_ptr = mload(0x40)
mstore(0x40, ret_ptr + 96)

mstore(ret_ptr, 32)
mstore(ret_ptr + 32, 4)
mstore(ret_ptr + 64, 0x11223344...)
RETURN(ret_ptr, 96)
```

需要记住的是：很多 Solidity 语义最后都会变成“从 free memory pointer 分配一块区域，
连续 mstore，然后把这段 memory 交给某个 EVM 指令”。

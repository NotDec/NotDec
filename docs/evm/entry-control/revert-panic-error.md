# revert、Panic、Error 和 custom error

## 简介

Solidity 里有多种失败方式：

```solidity
contract C {
    function f(uint256 x) external pure {
        require(x > 0, "x is zero");
        assert(x != 13);
        if (x == 7) {
            revert MyError(x);
        }
    }

    error MyError(uint256 x);
}
```

从源码看，它们分别像是 `require`、`assert`、`revert custom error`。
到 EVM 层，最终都会执行 `REVERT(offset, size)`。

区别在于 revert data 怎么编码。

## 编译过程与低层模式

EVM 的 `REVERT` 接收两个参数：

```text
REVERT(memory_offset, memory_size)
```

它会把 memory 中这一段字节作为错误返回数据。

常见几种形状：

```text
revert(0, 0)
```

这是空 revert，没有错误数据。nonpayable guard、fallback reject、一些 ABI bounds check
都可能生成它。

```text
Panic(uint256)
```

Solidity 内置 panic 使用 selector `0x4e487b71`。常见于 `assert`、算术溢出、
数组越界等编译器检查。

简化 memory 写入：

```text
mstore(ptr, 0x4e487b71 << 224)
mstore(ptr + 4, panic_code)
revert(ptr, 36)
```

`Error(string)` 使用 selector `0x08c379a0`，常见于 `require(cond, "message")`。
它后面跟 ABI string 编码。

```text
mstore(ptr, 0x08c379a0 << 224)
mstore(ptr + 4, 32)          ; string offset
mstore(ptr + 36, length)
mstore(ptr + 68, bytes...)
revert(ptr, total_size)
```

custom error 和函数调用很像，也是 4 字节 selector 加 ABI 参数：

```text
mstore(ptr, selector("MyError(uint256)") << 224)
mstore(ptr + 4, x)
revert(ptr, 36)
```

还有一种常见形状是 returndata bubble。低级 call 失败后，Solidity 可能把被调用方的
returndata 原样 revert 出去：

```text
size = RETURNDATASIZE
RETURNDATACOPY(0, 0, size)
REVERT(0, size)
```

这表示“外部调用失败，把对方给出的错误原样传上来”。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function div(uint256 a, uint256 b) external pure returns (uint256) {
        require(b != 0, "zero");
        return a / b;
    }
}
```

编译器要保证的语义是：

- `b != 0` 时继续执行除法。
- `b == 0` 时返回 ABI 编码的 `Error("zero")`。

简化 EVM 逻辑：

```text
if b == 0:
    ptr = mload(0x40)
    mstore(ptr, 0x08c379a0 << 224)
    mstore(ptr + 4, 32)
    mstore(ptr + 36, 4)
    mstore(ptr + 68, "zero")
    revert(ptr, 100)

result = a / b
return abi_encode_uint256(result)
```

再看 `assert`：

```solidity
function f(uint256 x) external pure {
    assert(x != 0);
}
```

失败时更像：

```text
if x == 0:
    ptr = mload(0x40)
    mstore(ptr, 0x4e487b71 << 224)
    mstore(ptr + 4, 0x01)
    revert(ptr, 36)
```

`0x01` 是 panic code，表示 generic assertion failure。

需要记住的是：EVM 只有 `REVERT`，Solidity 的失败类型主要靠 revert data 区分。
只看到 `REVERT` 不够，还要看它前面往 memory 写了什么。

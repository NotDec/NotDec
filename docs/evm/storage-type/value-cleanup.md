# Value cleanup 和类型线索

## 简介

EVM 的栈值和 storage word 都是 256 bit。Solidity 源码里却有很多窄类型：

```solidity
contract C {
    address public owner;
    bool public enabled;
    uint8 public count;
    int16 public delta;
}
```

编译器必须保证这些值在使用时符合源码类型的宽度和符号规则。

这些清理动作本身不是业务逻辑，但在底层会表现成普通位运算。

## 编译过程与低层模式

无符号窄整数和 address 常见用 mask：

```text
uint8  mask: value & 0xff
uint16 mask: value & 0xffff
address mask: value & 0xffffffffffffffffffffffffffffffffffffffff
```

`address` 本质是 160 bit，所以底层和 `uint160` 很像。

有符号整数常用 `SIGNEXTEND`。例如 `int16` 需要保留 16 bit 并扩展符号：

```text
clean = SIGNEXTEND(1, raw)
```

这里 `1` 表示最高有效字节编号。`int16` 占 2 字节，所以编号是 1。

bool 常见归一化是把任意非零值变成 1：

```text
bool_value = iszero(iszero(raw))
```

也就是：

```text
raw == 0 -> 0
raw != 0 -> 1
```

这些操作可能出现在 calldata 参数解码后、storage 读取后、算术或比较前、ABI 返回前。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function f(address a, uint8 x, bool b) external pure returns (address, uint8, bool) {
        return (a, x, b);
    }
}
```

编译器要保证的语义是：

- `a` 只保留低 160 bit。
- `x` 只保留低 8 bit。
- `b` 必须是 0 或 1。

简化 EVM 伪指令：

```text
raw_a = CALLDATALOAD(4)
a = raw_a & 0xffffffffffffffffffffffffffffffffffffffff

raw_x = CALLDATALOAD(36)
x = raw_x & 0xff

raw_b = CALLDATALOAD(68)
b = ISZERO(ISZERO(raw_b))

ptr = mload(0x40)
mstore(ptr + 0, a)
mstore(ptr + 32, x)
mstore(ptr + 64, b)
RETURN(ptr, 96)
```

另一个有符号整数例子：

```solidity
function g(int16 x) external pure returns (int16) {
    return x;
}
```

简化 EVM 伪指令：

```text
raw_x = CALLDATALOAD(4)
x = SIGNEXTEND(1, raw_x)
return abi_encode_int16(x)
```

需要记住的是：mask、`SIGNEXTEND`、双重 `ISZERO` 常常只是类型清理。不能一看到
`AND` 就认为源码里一定有按位与。

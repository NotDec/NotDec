# ABI 参数解码

## 简介

外部调用 Solidity 函数时，参数放在 calldata 里：

```solidity
contract C {
    function transfer(address to, uint256 amount) external returns (bool) {
        return true;
    }
}
```

调用 `transfer(address,uint256)` 的 calldata 大概是：

```text
0x00..0x03: selector
0x04..0x23: address to
0x24..0x43: uint256 amount
```

Solidity 函数体里能直接用 `to` 和 `amount`，是因为编译器在入口处生成了解码代码。

## 编译过程与低层模式

静态参数比较简单。selector 后面每个参数占 32 字节：

```text
arg0 = calldataload(4)
arg1 = calldataload(36)
arg2 = calldataload(68)
```

但编译器还要先检查 calldata 长度够不够：

```text
if calldatasize < 4 + 32 * arg_count:
    revert(0, 0)
```

窄类型还要清理。例如 `address` 只有 160 bit，ABI word 是 256 bit：

```text
raw = calldataload(4)
to = raw & ((1 << 160) - 1)
```

动态参数更复杂。ABI head 里放 offset，tail 里放 length 和 data：

```solidity
function f(string calldata s) external
```

calldata 大概是：

```text
0x00..0x03: selector
0x04..0x23: offset to string tail, usually 32
tail:
    length
    bytes data
```

编译器要检查 offset、length、tail 是否都在 calldata 范围内，必要时还会用 `CALLDATACOPY`
把动态数据复制到 memory。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function set(address owner, uint256 amount) external {
    }
}
```

编译器要保证的语义是：

- calldata 至少有 selector + 两个参数，也就是 4 + 64 字节。
- 第一个参数按 address 使用，只保留低 160 bit。
- 第二个参数按 uint256 使用。

简化 EVM 伪指令：

```text
if CALLDATASIZE < 68:
    REVERT(0, 0)

raw_owner = CALLDATALOAD(4)
owner = raw_owner & 0xffffffffffffffffffffffffffffffffffffffff

amount = CALLDATALOAD(36)
```

再看动态参数：

```solidity
function echo(bytes calldata data) external pure returns (bytes calldata) {
    return data;
}
```

简化解码逻辑：

```text
if CALLDATASIZE < 36:
    REVERT(0, 0)

offset = CALLDATALOAD(4)
tail = 4 + offset

if tail + 32 > CALLDATASIZE:
    REVERT(0, 0)

len = CALLDATALOAD(tail)
data_start = tail + 32

if data_start + len > CALLDATASIZE:
    REVERT(0, 0)
```

需要记住的是：ABI decode 不只是 `CALLDATALOAD`。它还包含长度检查、offset 检查、
动态 tail 检查，以及 address/bool/小整数的清理。

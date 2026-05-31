# payable / nonpayable guard

## 简介

Solidity 函数默认不能收 ETH。只有显式写了 `payable` 的入口才能接受 `msg.value > 0` 的调用。

```solidity
contract C {
    function f() external {
    }

    function g() external payable {
    }
}
```

`f` 是 nonpayable。调用 `f` 时如果带了 ETH，应该失败。

`g` 是 payable。调用 `g` 时可以带 ETH。

EVM 没有 payable 这个函数属性。编译器要在 nonpayable 入口插入检查。

## 编译过程与低层模式

编译器会在 nonpayable 函数入口附近读 `CALLVALUE`。`CALLVALUE` 就是这次调用带来的 wei 数量。

典型逻辑是：

```text
if CALLVALUE != 0:
    revert(0, 0)

continue_function_body
```

也可以写成等价形状：

```text
value = CALLVALUE
ok = value == 0
if ok:
    goto body
else:
    revert(0, 0)
```

payable 函数通常没有这段 guard。因为 payable 入口允许 `CALLVALUE` 非 0。

这段代码经常出现在 selector 命中之后、ABI 参数解码之前。原因很直接：如果这个函数本来就不能收 ETH，
编译器没必要继续解码参数。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function set(uint256 x) external {
        value = x;
    }

    uint256 public value;
}
```

用户没有写 `payable`，所以 `set` 是 nonpayable。

编译器要保证的语义是：

- `msg.value == 0` 时，继续执行 `value = x`。
- `msg.value != 0` 时，立刻 revert。

简化 EVM 伪指令：

```text
set_entry:
    if CALLVALUE != 0:
        REVERT(0, 0)

    if CALLDATASIZE < 36:
        REVERT(0, 0)

    x = CALLDATALOAD(4)
    SSTORE(value_slot, x)
    STOP
```

这里可以看到两个不同来源的 `REVERT(0, 0)`：

- 第一个来自 nonpayable guard。
- 第二个来自 ABI 参数长度检查。

它们底层都可能是空 revert，但含义不同。前者表示“这个入口不能收 ETH”，后者表示“calldata
不够长，参数解码失败”。

如果源码改成：

```solidity
function set(uint256 x) external payable {
    value = x;
}
```

那么入口通常会少掉 `CALLVALUE != 0` 这段检查。

需要记住的是：nonpayable guard 是编译器插入的入口保护，不是用户手写的 `require(msg.value == 0)`。
两者底层形状很像，但位置和上下文不同。

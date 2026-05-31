# Selector 分发

## 简介

Solidity 合约里可以有多个 `public` 或 `external` 函数：

```solidity
contract C {
    function balanceOf(address owner) external view returns (uint256) {
        return 1;
    }

    function transfer(address to, uint256 amount) external returns (bool) {
        return true;
    }
}
```

EVM 调用一个合约时，并不会直接指定“调用哪个函数”。调用者只传一段 calldata。
Solidity ABI 约定 calldata 前 4 字节是函数 selector，也就是函数签名 hash 的前 4 字节。

例如：

```text
balanceOf(address)        -> 0x70a08231
transfer(address,uint256) -> 0xa9059cbb
```

所以编译器要在合约入口生成一段分发代码：读 calldata 前 4 字节，根据 selector 跳到对应函数。

## 编译过程与低层模式

编译器要处理几件事：

- calldata 至少要有 4 字节，才可能包含 selector。
- 从 calldata 第一个 32 字节里取出最高 4 字节。
- 把这个 selector 和每个 public/external 函数的 selector 常量比较。
- 命中后跳到对应函数入口。
- 没命中时进入 fallback，或者直接 revert。

简化 EVM 伪指令大概是：

```text
if CALLDATASIZE < 4:
    goto fallback_or_receive

word0 = CALLDATALOAD(0)
selector = word0 >> 224

if selector == 0x70a08231:
    goto balanceOf_entry

if selector == 0xa9059cbb:
    goto transfer_entry

goto fallback_or_revert
```

这里 `CALLDATALOAD(0)` 一次读 32 字节。selector 在 calldata 的最前面，也就是这个 word
的最高 4 字节，所以要右移 224 bit。

真实字节码不一定按源码顺序比较。optimizer 可能把比较链改成二分查找，也可能调整基本块顺序。
但核心动作还是：读 selector、和常量比较、跳转。

## 例子详解

Solidity 源码：

```solidity
contract C {
    function f() external pure returns (uint256) {
        return 1;
    }

    function g(uint256 x) external pure returns (uint256) {
        return x + 1;
    }
}
```

编译器要保证的语义是：

- calldata 前 4 字节等于 `f()` 的 selector 时执行 `f`。
- calldata 前 4 字节等于 `g(uint256)` 的 selector 时执行 `g`。
- 调 `g` 时，selector 后面还要有一个 ABI 编码的 `uint256` 参数。
- selector 不认识时，如果没有 fallback，就失败。

简化后的入口逻辑：

```text
entry:
    if CALLDATASIZE < 4:
        goto no_selector

    selector = CALLDATALOAD(0) >> 224

    if selector == selector("f()"):
        goto f_entry

    if selector == selector("g(uint256)"):
        goto g_entry

    goto no_match

f_entry:
    return abi_encode_uint256(1)

g_entry:
    x = abi_decode_uint256(calldata[4:36])
    return abi_encode_uint256(x + 1)

no_selector:
no_match:
    revert(0, 0)
```

如果合约有 `receive()`，且 calldata 为空，`CALLDATASIZE < 4` 的路径可能进入 `receive`。
如果有 `fallback()`，不认识的 selector 也可能进入 fallback。

需要记住的是：selector 分发不是用户手写业务逻辑，而是 Solidity 为 ABI 调用生成的入口控制流。

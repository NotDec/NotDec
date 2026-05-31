# 入口与控制语义

这一类模式回答一个最基本的问题：一笔 EVM 调用进入合约后，代码怎么决定执行哪个
Solidity 函数，什么情况下直接失败。

Solidity 源码里有 `public`、`external`、`fallback`、`receive`、`payable`、
`require`、`assert` 这些概念。EVM 本身没有这些概念。编译器只能用 calldata、
callvalue、条件跳转和 `REVERT` 拼出这些语义。

## 本目录包含的模式

- [Selector 分发](selector-dispatch.md)：根据 calldata 前 4 字节选择 public/external 函数。
- [payable / nonpayable guard](payability-guard.md)：非 payable 入口拒绝携带 ETH 的调用。
- [revert、Panic、Error 和 custom error](revert-panic-error.md)：不同失败路径的底层编码。
- [checked arithmetic 和 bounds check](checked-bounds.md)：Solidity 0.8+ 的算术检查和边界检查。

## 建议阅读顺序

先看 selector 分发。它解释了为什么一个合约通常会有一个大入口函数。

再看 nonpayable guard。它是 public 函数入口附近最常见的编译器插入检查。

然后看 revert 系列。后面的算术检查、数组边界检查、ABI 解码失败，最后都会落到
某种 `REVERT`。

最后看 checked/bounds，因为它依赖前面对 `Panic` 和 `REVERT` 的理解。

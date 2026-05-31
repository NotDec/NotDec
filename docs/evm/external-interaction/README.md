# 外部交互语义

这一类模式说明合约怎么和外部世界交互。

Solidity 里的外部调用、接口调用、`transfer`、`send`、低级 `call`、event，编译后都会变成
EVM helper 指令或 opcode。它们通常还会和 memory buffer、ABI 编码、returndata 处理、
revert bubble 混在一起。

## 本目录包含的模式

- [External call 和 returndata](external-call-returndata.md)：`CALL`、`STATICCALL`、
  `DELEGATECALL`、成功检查、返回数据处理。
- [Event log](event-log.md)：`LOG0` 到 `LOG4`、topic、data buffer。

## 建议阅读顺序

先看 external call。它涉及 call data、return data、失败冒泡，是 EVM 交互里最常见也最复杂的模式。

再看 event log。event 不改变合约状态，但会把 topic 和 data 写入交易日志，也大量复用 ABI 编码规则。

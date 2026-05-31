# Storage 与类型线索

这一类模式说明 Solidity 状态变量在 EVM storage 里怎么存。

EVM storage 是一个从 256-bit key 到 256-bit value 的持久化表。Solidity 的状态变量、
mapping、动态数组、bytes/string、打包字段，最后都要落到这个表里。

EVM 也没有 `uint8`、`address`、`bool` 这类窄类型。编译器会用 mask、shift、
`SIGNEXTEND` 等操作，把 256-bit 值清理成源码类型需要的宽度。

## 本目录包含的模式

- [Value cleanup 和类型线索](value-cleanup.md)：mask、signextend、bool 归一。
- [Storage addressing](storage-addressing.md)：mapping、动态数组和嵌套 storage 地址。
- [Packed storage field](packed-storage-field.md)：多个小字段打包到同一个 storage slot。
- [Storage bytes/string 短长编码](storage-bytes-string.md)：bytes/string 在 storage 里的两种编码。

## 建议阅读顺序

先看 value cleanup。它是理解小整数、address、bool 的基础。

再看 storage addressing。它解释 mapping 和动态数组为什么需要 hash。

然后看 packed storage field。它把 value cleanup 和 storage slot 结合起来。

最后看 bytes/string。它同时涉及 storage 地址、长度编码、短长分支，复杂度最高。

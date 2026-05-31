# ABI 与内存语义

这一类模式说明 Solidity 怎么把参数、返回值和错误放进 EVM 的 calldata 与 memory。

EVM 的外部输入是 `calldata`，输出靠 `RETURN`，错误靠 `REVERT`。这些都只是字节区间。
Solidity ABI 在这些字节区间上规定了类型编码方式：每 32 字节一个 word，动态类型用
head/tail，bytes/string/array 还要保存长度。

## 本目录包含的模式

- [Memory buffer 和动态对象](memory-buffer.md)：free memory pointer、分配、连续写 buffer。
- [ABI 参数解码](abi-decode.md)：从 calldata 读 public/external 函数参数。
- [ABI 返回值编码](abi-return.md)：把返回值写到 memory，再 `RETURN`。
- [ABI revert encoding](abi-revert-encoding.md)：把 Panic、Error、custom error 编码到 revert data。

## 建议阅读顺序

先看 memory buffer。后面 ABI return、revert、event、external call 都会复用这套写法。

然后看 ABI decode。它解释参数怎么从 calldata 进入函数体。

再看 ABI return 和 ABI revert encoding。它们和 decode 方向相反：把值写回 memory，
再交给 `RETURN` 或 `REVERT`。

# EVM Solidity 底层模式

这组文档介绍 Solidity 源码编译到 EVM 后常见的底层形状。

这里先不讲反编译器识别和实现。重点是先看懂：

- Solidity 用户写了什么。
- 编译器为了保持语义会补什么代码。
- 这些代码在 EVM 指令层大概长什么样。
- 有些地方如果用 LLVM IR 更直观，也会用简化 IR 辅助说明。

## 阅读顺序

建议按下面顺序读：

1. [入口与控制语义](entry-control/README.md)
2. [ABI 与内存语义](abi-memory/README.md)
3. [Storage 与类型线索](storage-type/README.md)
4. [外部交互语义](external-interaction/README.md)

如果要继续增加 EVM Solidity 语义恢复功能，先看
[EVM Solidity 功能开发规范](feature-development-guideline.md)。

入口控制流决定一笔调用先进哪里，ABI 和 memory 决定参数、返回值、错误怎么编码，
storage 决定状态变量怎么存，外部交互则覆盖 call 和 event。

## 简化约定

文档里的 EVM 伪指令不是完整字节码。它只保留和当前模式有关的核心操作，例如：

```text
CALLDATALOAD 0
SHR 224
EQ selector
JUMPI public_foo
```

这类片段只说明形状，不保证栈上的每一步都完整展开。

文档里的 Solidity 例子也会尽量短。真实编译结果会受编译器版本、optimizer、via-ir
选项影响，但核心语义通常相同。

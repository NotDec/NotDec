# EvmCalldataAccessPass 规划

## 原始 prompt

还是单独把EvmCalldataAccessPass的具体规划，EvmStorageSemanticsPass的具体规划单独拆到单独的logs/文件吧，logs/20260612-01-evm-high-level-semantics-pass-plan.md里面还是仅保留最早的那种简单的介绍。处理完这个之后commit一下

## 背景

当前 public entry 里的 calldata 访问主要表现为 `evm_calldataload`、`evm_calldatacopy`、calldata size guard 和若干 private helper 调用。这样类型推理看到的是一批分散 helper call，而不是一个有字段、有范围访问的输入对象。

Calldata 和普通 memory 的区别是：每个 public function 有自己的 ABI 格式。因此应该在每个 public entry 内引入一个独立 calldata buffer，把这个函数内所有 calldata 访问归到同一个对象上，再交给类型推理恢复结构。

这个 pass 不负责识别最终 ABI 参数类型，也不负责判断 address、bool、uintN、bytes/string、array。它只负责把 calldata 访问整理成类型推理能消费的形式。

## 目标

在类型恢复前执行：

```text
MemoryBufferRewritePass
EvmCalldataAccessPass
TypeRecovery
```

每个 public entry 入口生成一个 calldata buffer 获取点，例如：

```llvm
%cd = call ptr @notdec_evm_get_calldata(ptr %calldata)
```

然后把该 public entry 内的 `evm_calldataload`、`evm_calldatacopy`、相关 bounds guard 和 private helper offset 关系都归到 `%cd` 上。

## 重写形状

静态 word 访问从：

```llvm
%w0 = call i256 @evm_calldataload(ptr %calldata, i256 4)
%w1 = call i256 @evm_calldataload(ptr %calldata, i256 36)
```

整理成同一个 calldata buffer 的 offset 访问：

```text
%w0 = load_word %cd[4]
%w1 = load_word %cd[36]
```

这里的 `load_word %cd[4]` 是目标形状，不是最终 LLVM 语法。实现时可以用 intrinsic，也可以用和现有 memory object / stack pointer rewrite 一致的 pointer/address 表达式。关键是类型推理能看出 `%cd` 是同一个对象，offset 4、36、68 等位置被读取。

range copy 从：

```llvm
call void @evm_calldatacopy(ptr %mem, ptr %calldata,
                            i256 %dst, i256 %src, i256 %len)
```

整理成：

```text
copy memory[%dst..%dst+%len] <- %cd[%src..%src+%len]
```

这样 dynamic bytes/string、array tail 等结构不需要在 pass 里猜。类型推理可以根据 `%cd` 上的 head load、tail load、range copy、bounds guard 恢复结构。

## Public Entry 边界

一个 public entry 内应该只有一个 `notdec_evm_get_calldata`。所有直接 calldata 访问都归到这个 buffer。

如果 public entry 调用 private helper，而 helper 的形参表示 calldata offset，需要把 helper 内的访问映射回调用点的 `%cd`。已有 `getUniqueCallsiteArgUInt64Constant()` 可以处理“所有 callsite 都传同一个常量”的 helper。对于被多个 public entry 用不同 offset 复用的 helper，不能把 helper 本身固定成某个 ABI 格式；应在调用点上下文归属到各自 public entry 的 calldata buffer，必要时再考虑 clone 或 summary。

## 和类型推理 / HType 的关系

这个 pass 给类型推理提供：

- calldata buffer 对象。
- buffer 上的 word load。
- buffer range copy 到 memory object。
- bounds guard 和访问范围的关系。
- private helper 形参 offset 回推后的访问关系。

类型推理负责恢复：

- 哪些 offset 是 ABI head field。
- 哪些 head 指向 dynamic tail。
- dynamic tail 是 bytes/string/array/tuple。
- 元素类型和嵌套结构。
- address、bool、uintN/intN 等值类型。

后续语义 pass 只读取 calldata buffer 的 HType。例如 external call 或 signature pass 需要知道某个 bytes 参数时，应优先查 HType 是否说明 `%cd` 的某个 field 是 bytes，再调用 dynamic bytes helper 处理 memory 侧 copy。不能回到各 pass 自己重新扫 `evm_calldataload/evm_calldatacopy` 的路线。

## 实现注意

- 不在这个 pass 里生成 `evm.abi.arg.address`、`evm.abi.arg.bytes` 之类最终参数 helper。
- 不在这个 pass 里处理 address 高 96 bit 清零、bool range、uintN mask、signextend 等 value normalization；这些是类型推理输入或单独 value normalization 逻辑。
- 不删除 bounds guard。guard 是否隐藏由后续 cleanup 或 guard pass 统一处理。
- PHI 合并出来的 calldata offset 不要跳过。能表达成 `%cd` 上的动态 offset 访问就保留给类型推理。
- fallback 里手写协议也可以先作为 calldata buffer 访问表达，不要强行当 Solidity ABI 参数。

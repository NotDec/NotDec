# TypeBuilder convertStruct Trace

## 背景

为了更方便调试 `TypeBuilder::convertStruct()` 处理数组/递归 record 子问题时的输入形状，本次增加了一个窄范围的入口摘要打印。

目标是让调试时可以直接看到：

- 当前 `convertStruct()` 的 `PointeeSize`
- 一共有多少个 `RawFields`
- 每个 field 的 `OffsetRange`
- 其中 `access` 里的各个 stride / count
- 对应 field 推导出的 pointee 大小
- 对应 `UType` 的文本摘要

这样在追像：

- `@0+1i`
- `@0+4i`
- `PointeeSize=1` 但 field 却是 4-byte

这类问题时，不需要先下断点才能看到输入形状。

## 修改位置

本次只改了：

- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)

涉及的主要位置：

- 在文件顶部增加 `DEBUG_TYPE`
- 在匿名 namespace 中增加环境变量开关与判定函数
- 在 `TypeBuilder::convertStruct()` 开头增加摘要打印

关键代码位置：

- [src/TypeRecovery/mlsub/TypeBuilder.cpp:20](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L20)
- [src/TypeRecovery/mlsub/TypeBuilder.cpp:63](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L63)
- [src/TypeRecovery/mlsub/TypeBuilder.cpp:615](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L615)

## 开关方式

新增环境变量：

- `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1`

设计上支持两种触发方式：

1. 环境变量
   - `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1`
2. LLVM debug type
   - `DEBUG_TYPE = "mlsub_typebuilder"`

但考虑到当前 `notdec` 主程序里 `initDebugOptions()` 仍是注释状态，实际更稳定的用法是环境变量方式。

## 输出格式

每次进入 `TypeBuilder::convertStruct()` 时，会打印一段：

```text
[TypeBuilder::convertStruct] begin pointee_size=<none> raw_fields=2
  [0] range=@0+1i offset=0 access=[{size=1, count=0}] accessed_bits=32 accessed_bytes=4 utype='o37
  [1] range=@1 offset=1 access=[] accessed_bits=0 accessed_bytes=0 utype={}
```

字段含义：

- `pointee_size`
  - 当前这层递归被要求满足的元素边界
  - 没有则显示 `<none>`
- `raw_fields`
  - 当前 `RawFields` 总数
- `range`
  - `OffsetRange::str()` 的结果
  - 例如 `@0+1i`、`@4`、`@-12`
- `offset`
  - `OffsetRange.offset`
- `access`
  - `OffsetRange.access`
  - 每一项打印成 `{size=..., count=...}`
- `accessed_bits`
  - `TypeBuilder::accessedPointeeSizeInBits()` 的结果
- `accessed_bytes`
  - 由 `accessed_bits` 换算得到
- `utype`
  - `binarysub::printType()` 的文本摘要

## 为什么这样设计

### 1. 入口打印比崩溃点打印更有用

`convertStruct()` 的问题很多都不是在函数一开始就炸，而是经过：

- stride 提取
- 子问题递归
- field layout 合成
- `craftStruct()` / `convertFieldType()` / `convertPointer()`

之后才暴露。

如果只在崩溃点看局部变量，经常已经看不到“最初传进来的 access pattern”了。

所以这次选择在 `convertStruct()` 一进来就打印输入摘要。

### 2. 只打印当前真正影响决策的数据

本次没有直接把整个 `UType` 树做完整 dump，而是优先打印：

- `OffsetRange`
- access 的 stride / count
- pointee size
- `UType` 简要文本

这是因为当前调试目标主要是回答：

- 为什么这一层被识别成数组
- stride 是多少
- 子问题边界是多少
- payload size 为什么和边界冲突

这些都可以从当前输出直接看出来。

### 3. 默认不污染正常输出

该打印默认关闭，只在显式打开：

- `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1`

时出现。

所以不会影响普通反编译、测试或 golden 结果。

## 使用示例

调试当前 `fortune.o3.wasm.1.ll` 时，可以这样跑：

```bash
env \
  NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
  NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec-Exp/ICSE-HOWARD/summarys.json \
  NOTDEC_SIGNATURE_OVERRIDE=/sn640/NotDec-Exp/ICSE-HOWARD/signatures.json \
  NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  NOTDEC_DISABLE_INTERPROC=1 \
  NOTDEC_SAT_DISABLE=1 \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/out.c \
  --tr-level=2 \
  -g \
  --work-dir=/sn640/NotDec/work_dir
```

可以直接观察到之前关键问题对应的输入形状，例如：

- `range=@0+1i offset=0 access=[{size=1, count=0}]`
- 下一层递归变成 `pointee_size=1`
- 但内部 field 可能仍显示 `accessed_bits=32 accessed_bytes=4`

这正是第五层问题里：

- “byte-stride 元素边界只有 1 byte”
- “内部 payload 却还是 4 byte”

的直接证据。

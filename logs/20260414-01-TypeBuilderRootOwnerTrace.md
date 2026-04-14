# TypeBuilder Root Owner Trace

## 背景

为了回答“到底是哪个 LLVM value / 指令的类型链路触发了
`PointeeSize.value() < Size`”这个问题，本次在 `TypeBuilder` 调试打印里补了一层
root owner 追踪。

之前 `NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1` 只能看到：

- 当前 `convertStruct()` 的 `RawFields`
- access pattern
- `PointeeSize`
- `UType`

但看不到“这条转换链是从哪个 top-level `ExtValuePtr` 开始的”。

## 修改位置

涉及三个文件：

- [include/notdec/TypeRecovery/mlsub/TypeBuilder.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/TypeBuilder.h)
- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)

### 1. `TypeBuilder` 持有当前 root label

在：

- [TypeBuilder.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/TypeBuilder.h)

里新增：

- `CurrentRootDebugLabel`
- `setDebugRootLabel(std::optional<std::string>)`

目的是让 `TypeBuilder` 在一次顶层 `TB.convert(...)` 期间，知道自己当前是在替哪个
`ExtValuePtr` 落 HType。

### 2. `genTypes()` 在进入 `TB.convert()` 前设置 root

在：

- [MLsubGenerator.cpp:480](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L480)

附近，把：

- `toString(Ent.first, true)`

作为 root label 塞给 `TypeBuilder`，再调用：

- `TB.convert(It->second)`

这样如果当前 top-level value 是某条 LLVM 指令、参数、返回值、常量包装等，trace
里就能直接打印出来。

对 memory 总类型，也会显式设置：

- `root=<memory>`

### 3. `convertStruct()` 打印 root 和 mismatch

在：

- [TypeBuilder.cpp:619](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L619)

附近，入口 trace 现在会打印：

- `root=<...>`

在：

- [TypeBuilder.cpp:952](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L952)

附近，当命中：

- `PointeeSize < Size`

时，会额外打印一条专门的 mismatch 诊断，内容包括：

- `root`
- `pointee_size`
- `synthesized_size`
- `first_field_start`
- `first_field_size`
- `field_count`

## 现在怎么定位

直接运行：

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

关注这类输出：

```text
[TypeBuilder::convertStruct] begin root=<...> ...
[TypeBuilder::convertStruct] pointee/layout mismatch root=<...> ...
```

如果 `root=` 后面是某个具体的 LLVM value 文本，那么它就是触发这条类型转换链的
top-level owner。

## 对 `fortune.o3.wasm.1.ll` 当前 case 的结论

这次实际跑出来的关键输出是：

```text
[TypeBuilder::convertStruct] pointee/layout mismatch root=<memory> pointee_size=1 synthesized_size=4 ...
```

这说明：

- 当前触发 `PointeeSize < Size` 的，不是某条单独 SSA 指令的 top-level HType
- 而是 memory 总类型（`<memory>`）在递归展开某个 byte-stride record 子问题时触发的

也就是说，这个 case 的问题来源更像：

- “某段内存对象/内存摘要的类型形状”

而不是：

- “某一个 `%x = load ...` 指令自己的顶层类型”

## 如果 root 不是 `<memory>`

对于普通 value，trace 会直接把：

- `toString(ExtValuePtr, true)`

打印到 `root=` 里。此时你通常可以直接看到：

- 某条 LLVM instruction
- 某个 function argument
- 某个 return value
- 或某个特殊包装常量

这时基本不需要再额外下断点，就能知道是哪个 top-level LLVM value 把问题带进来的。

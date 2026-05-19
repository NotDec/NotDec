# 2026-05-19 bin2llvm module-limit20 参数缺少 varnode 问题调研

## 用户原始 prompt

```text
写一个新的日志记录一下本次发现的问题，并分析如何解决
```

## 结论

这次重跑 `memcached / lighttpd / openssh` 的 module-limit20 以后，暴露了一个新的、和前面 CFG / PHI 不同的问题：

`ExportHeritageModule.java` 导出的某些参数条目只有 `storage`，没有 `varnode`，而 `HeritageToLLVM.cpp::mapParameters()` 目前把这种情况直接当成 fatal。

这不是导出失败，也不是 LLVM verifier 失败，而是 native lowering 在“绑定函数参数到 LLVM argument”这一步过于严格。

## 本次重跑结果

重跑目录：

```text
/tmp/notdec-bench2-rerun-20260519
```

### `memcached limit20`

1. Ghidra 导出成功，`attempted functions: 20`，`succeeded functions: 20`，`failed functions: 0`。
2. `notdec-heritage-module-check` 通过，`status: ok`。
3. `notdec-heritage-module-llvm` lowered 20 个 body，`failed function bodies: 0`。
4. `llvm-as` 通过。
5. lowering 时间：`real 1.75s`。

这个样本没有新问题。

### `lighttpd limit20`

1. Ghidra 导出成功，`attempted functions: 20`，`succeeded functions: 20`，`failed functions: 0`。
2. `notdec-heritage-module-check` 通过，`status: ok`。
3. lowering 时 18 个 body 成功，2 个 body 失败。
4. 失败函数：
   - `FUN_0010e78d ram:0010e78d`
   - `FUN_0010eb8a ram:0010eb8a`
5. `llvm-as` 仍通过，因为失败 body 被降级成 declaration。
6. lowering 时间：`real 0.26s`。

失败信息都是：

```text
parameter param_2 has no varnode
```

### `openssh limit20`

1. Ghidra 导出成功，`attempted functions: 20`，`succeeded functions: 20`，`failed functions: 0`。
2. `notdec-heritage-module-check` 通过，`status: ok`。
3. lowering 时 18 个 body 成功，2 个 body 失败。
4. 失败函数：
   - `FUN_0010dfe0 ram:0010dfe0`
   - `FUN_00112ca0 ram:00112ca0`
5. `llvm-as` 通过。
6. lowering 时间：`real 36.45s`。

失败信息分别是：

```text
parameter param_3 has no varnode
parameter param_1 has no varnode
```

## 证据

导出的 JSON 里，失败函数的参数条目确实有缺口：

### `lighttpd`

`FUN_0010e78d`

```json
{
  "index": 1,
  "name": "param_2",
  "type": "undefined8",
  "storage": "RSI:8"
}
```

`FUN_0010eb8a`

```json
{
  "index": 1,
  "name": "param_2",
  "type": "undefined8",
  "storage": "RSI:8"
}
```

也就是说，这两个参数有名字、有类型、有寄存器存储位置，但没有 `varnode`。

### `openssh`

`FUN_0010dfe0`

```json
{
  "index": 2,
  "name": "param_3",
  "type": "undefined8",
  "storage": "RDX:8"
}
```

`FUN_00112ca0`

```json
{
  "index": 0,
  "name": "param_1",
  "type": "undefined8",
  "storage": "RDI:8"
}
```

同样是参数存在，但没有 `varnode`。

## 根因

### 1. 导出侧本来就允许参数没有 `varnode`

`ExportHeritageModule.java::writeParam()` 和 `ExportHeritagePcode.java::writeParam()` 只有在：

```java
HighVariable highVariable = symbol.getHighVariable();
if (highVariable != null && highVariable.getRepresentative() != null) {
  ...
}
```

时才会写出 `varnode`。

也就是说，JSON schema 本身就把 `varnode` 当成可选项。`storage` 还在，说明这个参数位置是知道的，只是没有绑定到具体 SSA representative。

### 2. lowering 侧把“没有 varnode”当成硬错误

`HeritageToLLVM.cpp::mapParameters()` 现在是：

```cpp
for (const HeritageParam &param : Program.Function.Params) {
  if (!param.Varnode) {
    errorMessage = "parameter ... has no varnode";
    return;
  }
  Values[*param.Varnode] = &*arg;
  ++arg;
}
```

问题有两个：

1. `param.Varnode` 缺失本身不一定是错误，可能只是 Ghidra 没给出 representative。
2. 这里如果直接报错，整个函数体就不能 lower。

### 3. 这个问题和 body 语义不是一回事

这几个失败函数并没有在导出阶段失败，`module-limit20.check` 也通过了。

说明：

1. 函数签名是有的。
2. 函数体是有的。
3. 只是某些参数没有对应的 SSA 绑定。

这类情况更像“参数没被 materialize 成 high variable”，不该和真正的结构错误混在一起。

## 怎么修

### 最小修法

`HeritageToLLVM.cpp::mapParameters()` 不应该因为单个参数缺 `varnode` 就直接失败。

更合理的处理是：

1. 仍然按参数顺序消费 LLVM function arguments。
2. `param.Varnode` 存在时，把这个 LLVM argument 绑定到 `Values[*param.Varnode]`。
3. `param.Varnode` 不存在时，只跳过绑定，留一个 warning，不要 fatal。

关键点是：

- 不能简单地 `continue` 然后不动 `arg`
- 因为 LLVM function 的参数是按位置排列的，后面的参数还要继续对齐

也就是说，这里要做的是“位置对齐 + 选择性建映射”，不是“见到缺失就停”。

### 什么时候还应该报错

只有下面这种情况才值得继续报错：

1. `param.Varnode` 缺失，同时后续真的出现了需要这个参数的 SSA 使用，而且没有别的来源可以绑定。
2. LLVM function 的实际 argument 个数和 `Program.Function.Params` 个数不一致。

这两种才是结构错位，不是单纯缺少 high variable representative。

### 备选修法

如果后面发现某些函数里参数虽然没有 `varnode`，但 body 里其实需要显式表示它们的值，那可以再补一层导出：

- 在 Ghidra 导出侧给这类参数造一个稳定的 synthetic representative
- 或者给 JSON 增加更明确的“parameter slot”标记，避免 lowering 只能靠 `varnode` 这一种绑定方式

但就这次的样本看，先不需要做这一步。先把 lowering 放宽，最小改动就够。

## 判断标准

修完以后，至少要满足：

1. `lighttpd limit20` 不再因为 `param_2 has no varnode` 失败。
2. `openssh limit20` 不再因为 `param_1 / param_3 has no varnode` 失败。
3. `llvm-as` 结果仍然通过。
4. `memcached limit20` 不能被改坏，仍然保持 20/20 body 成功。

## 风险

1. 不能把“缺 varnode”直接静音成无脑跳过，否则可能掩盖真正的导出错位。
2. 不能在跳过绑定时忘了推进 LLVM argument iterator，不然后面的参数会整体错位。
3. 如果后面样本里出现“参数缺 varnode，但 body 里确实要用”的情况，还得回头补导出侧，不然 lowering 只能做假设。

## 2026-05-19 实施记录

本次按上面的方向实现了，不再把“参数缺 varnode”当 fatal，也没有在入口补 synthetic store。

修改文件：

1. `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritageModule.java`
   - 第 302 行起，参数导出增加 `registerName`。
   - 只要 `storage` 看起来是寄存器位，就把寄存器名写进 JSON。
2. `external/NotDec-bin2llvm/ghidra_scripts/ExportHeritagePcode.java`
   - 第 223 行起，单函数导出同步增加 `registerName`。
3. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritagePcode.h`
   - 第 49 行起，`HeritageParam` 新增 `RegisterName`。
4. `external/NotDec-bin2llvm/lib/HeritagePcode.cpp`
   - 第 94 行起，JSON 解析时读取可选的 `registerName`。
5. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 242 行起，在 lower 前先挂参数 metadata。
   - 第 369 行起，`mapParameters()` 改成遇到缺 `varnode` 只发 warning，不再直接失败。
   - 第 385 行起，新增 `notdec.param.register` metadata，记录这些缺 `varnode` 但有寄存器 storage 的参数。

验证结果：

1. `cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-llvm notdec-heritage-module-check -j4`
   - 通过。
2. `memcached limit20`
   - `lowered function bodies: 20`
   - `failed function bodies: 0`
   - `llvm-as` 通过
   - `real 1.79s`
3. `lighttpd limit20`
   - 之前的 `parameter ... has no varnode` 不再导致失败
   - `lowered function bodies: 20`
   - `failed function bodies: 0`
   - `llvm-as` 通过
   - `real 0.27s`
4. `openssh limit20`
   - 之前的 `parameter ... has no varnode` 不再导致失败
   - `lowered function bodies: 20`
   - `failed function bodies: 0`
   - `llvm-as` 通过
   - `real 38.76s`

判断：

1. 这次修法把“导出里没有 representative，但 storage 仍然是寄存器”的情况从 fatal 改成了可保留信息的 warning。
2. IR 里没有再塞入口 synthetic store。
3. 当前寄存器读写语义仍然由 `RegisterStorage` 维持，函数参数只作为附加信息保留在 metadata 里。

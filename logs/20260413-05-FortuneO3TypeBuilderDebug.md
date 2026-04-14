# Fortune O3 TypeBuilder Debug

## 背景

调试用例：

- `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll`

启动方式参考 `.vscode/launch.json` 里的 `dec` 配置：

- 输入为目标 `.ll`
- `--tr-level=2`
- `-g --work-dir=...`
- 携带 HOWARD 相关 override 环境变量

本次目标是定位该 case 在类型恢复阶段的崩溃原因，并逐层把阻塞点推进到更靠近真实根因的位置。

## 初始现象

最开始直接运行会在：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp`
- `TypeBuilder::convertStruct`

触发断言：

```text
assert(Ent.first.access.empty())
```

这说明 `convertStruct()` 进入“剩余字段必须已经不是数组访问”的阶段时，仍然残留了 `OffsetRange.access` 非空的 field。

## 第一层问题：遍历 `RemainingEntries` 时边迭代边 `erase`

原始代码在提取 `HasStrideEntries` 时这样写：

- 遍历 `std::vector<std::pair<OffsetRange, UTypePtr>> RemainingEntries`
- 如果当前 entry 含有 `MaxStride`
- 立刻 `RemainingEntries.erase(It)`

这会导致：

- `std::vector` 迭代器失效
- 后续元素可能被跳过
- 部分本应作为数组成员处理的 entry 残留在 `RemainingEntries`

因此先把逻辑改成：

- 命中时先拷贝到 `HasStrideEntries`
- 用 `It = RemainingEntries.erase(It)` 维持有效迭代器
- 非命中时再 `++It`

这是必要修正，但不是该 case 的唯一根因。

## 第二层问题：stride 匹配错误依赖了 `ArrayOffset::Count`

继续调试后发现，仅修复迭代器仍不足以让该 case 通过。原因是：

- `TypeBuilder::convertStruct()` 里用
  `std::find(It->first.access.begin(), It->first.access.end(), MaxStride)`
  判断某个 field 是否含有当前 stride
- 这里 `MaxStride` 是整数，会隐式构造成 `ArrayOffset{Size=MaxStride, Count=0}`
- 但 `OffsetRange::fromStr()` 之前把类似 `@0+4i` 解析为
  `ArrayOffset{Size=4, Count=-1}`

于是出现了 round-trip 不一致：

- `OffsetRange::str()` 对无显式 `[Count]` 的 access 输出 `+4i`
- `OffsetRange::fromStr()` 却把它读回成 `Count=-1`
- 导致 “同一个 stride” 只因为 `Count` 不同就比较不相等

这会让：

- 本应归入数组分组的字段没有被识别为 `HasStrideEntries`
- 最终再次残留到 `RemainingEntries`
- 回到最初的 `assert(Ent.first.access.empty())`

### 对应修复

1. `TypeBuilder::convertStruct()` 中改为只按 `ArrayOffset::Size` 判断是否含有 `MaxStride`
2. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/Range.h`
   的 `OffsetRange::fromStr()` 中，把无显式 `[Count]` 的默认值改为 `0`

这样做的理由：

- 当前代码其它路径基本都只使用 `Size`
- `Range.h` 注释本身也写的是 `0 represents unbounded`
- `str()` / `fromStr()` 应当尽量保持 round-trip 一致

## 第三层问题：`OffsetRange::maxAccess()` 错误假定 `offset > 0`

推进后该 case 不再停在上一个断言，而是继续触发：

```text
notdec-llvm2c/Interface/Range.h: assert(offset > 0)
```

发生点在 `OffsetRange::maxAccess()`。

这个假设不成立，因为当前系统本身允许：

- `offset == 0`
- `access.size() > 0`

例如：

- `normalizeOffsets()` 就显式处理了 `offset == 0 && access.size() == 1`

也就是说，形如 `@0+Ni` 的数组访问是合法形状，不应该在 `maxAccess()` 里直接被视为非法。

### 对应修复

将断言放宽为：

```text
assert(offset >= 0)
```

这一步的目的不是“放过坏数据”，而是让合法的 base-relative array access 可以继续进入后续布局逻辑。

### 第四层问题：`craftStruct()` 把 unsized array 当成 sized array

`gdb` 抓到的局部变量表明：

- `craftStruct()` 里出现了 `Ty->isArrayType()`
- 当前 field range 是 `{Start = 0, Size = 4}`
- `ValidRange` 是 `{Start = 0, Size = 12}`
- 但是 `ArrayType::getNumElements()` 是空的

也就是说，进入 `craftStruct()` 时，这个 field 的类型已经是：

- “元素类型已知”
- “这是数组”
- 但“元素个数未知”

这在当前 HType 体系里是合法状态，因为 `ArrayType` 本来就支持：

- `NumElements = 某个具体值`
- 或 `NumElements = std::nullopt`

后者就是 unsized array。

### 这段代码原本想做什么

`craftStruct()` 中这段逻辑的意图其实很明确：

1. 先根据当前 field 自己的 range 和相邻 field 的起点，估算一个 `ExpandEnd`
2. 如果当前 field 是数组，就尝试把数组从“当前最小覆盖范围”扩张到 `ExpandEnd`
3. 扩张方式依赖：
   - 当前 field 的总大小 `Ent.first.Size`
   - 数组已有元素个数 `NumElements`
   - 从而反推出单个元素大小 `ElemSize = Ent.first.Size / NumElements`
4. 最后重新计算新的 `NewCount`，再调用 `withSize()` 生成一个更大的定长数组

这个流程的前提是：

- 当前数组已经是 sized array
- 因而可以安全地从 “总大小 / 元素个数” 反推元素大小

如果这个前提不成立，整条推导链都会失效。

原始代码却直接做：

```text
Ty->getAs<ast::ArrayType>()->getNumElements().value()
```

因此触发：

```text
std::bad_optional_access
```

### 为什么这里会走到 unsized array

从这次 `fortune.o3.wasm.1.ll` 的调试轨迹看，unsized array 不是偶然脏值，而是上游推导过程自然产生的中间形态。

原因在于前面的 `convertStruct()` / `convertFieldType()` 在处理 array-like record access 时，经常只能先确认：

- “这是一个数组状访问”
- “成员类型大概是什么”
- “当前字段至少覆盖了多少字节”

但并不能总是立刻确认：

- 数组究竟有几个元素
- 这个 field 的边界究竟是语义上的数组边界，还是因为下一层 struct/union/递归展开暂时截出来的局部范围

因此，在 HType 层先保留一个：

- element type 已知
- NumElements 未知

的 `ArrayType`

本身是合理设计。问题不在于 unsized array 出现了，而在于 `craftStruct()` 把它当成“已经有精确元素个数”的数组继续做 layout 扩张。

### 为什么不能在这里临时猜一个元素个数

表面上看，似乎可以在这里根据：

- `Ent.first.Size`
- `ExpandEnd`
- `ValidRange`

去“反推”一个元素个数，然后把 unsized array 变成 sized array。

但这一步风险很大，原因有三点：

1. `Ent.first.Size` 在这里并不一定是“完整数组大小”
   - 它很多时候只是当前递归层刚合成出的 field range
   - 可能已经受上游 union 合并、padding、裁剪、递归展开影响

2. `ExpandEnd` 反映的是“当前 struct layout 中可以占到哪里”
   - 它是布局机会窗口，不是类型语义保证
   - 尤其当下一字段起点、`ValidRange`、char array merge 等规则介入时，`ExpandEnd` 更像保守上界，不是数组真实长度

3. 这个 case 后续还暴露了 `PointeeSize < synthesized layout size` 的问题
   - 说明当前这批递归子问题本来就存在边界和内部布局冲突
   - 如果在这里再强行猜元素个数，只会把错误进一步固化成“看起来合法的定长数组”

所以这里最稳妥的做法不是“替它决定长度”，而是承认：

- 目前只知道它是数组
- 但还不知道确切元素数

然后不要在 `craftStruct()` 这一步越权把它定死。

### 为什么这次修复选择“保持 unsized”，而不是直接降级成 `char[]`

这里有几种可能的修法：

1. 继续原逻辑，要求必须有 `NumElements`
2. 直接把所有 unsized array 降级成 `char[]`
3. 保留 unsized array，不做这一步扩张

这次选第 3 种，主要因为它最符合当前信息量：

- `craftStruct()` 当前只是在“合成 record layout”
- 它并没有比上游更多的数组语义信息
- 保持 unsized 可以尽量保留 element type 信息
- 同时避免把未知长度错误地钉死成某个具体 count

相比之下：

- 直接降级成 `char[]` 会丢掉已经推出来的元素类型
- 强行猜 count 又会把布局阶段的近似值误写成类型事实

因此最终修成：

- 只有 `NumElements` 已知且非 0 时，才执行数组扩张
- 否则保留当前 `ArrayType` 原状

这是一个明显更保守、也更符合当前抽象边界的行为。

### 对应修复

把数组扩张逻辑改成：

- 只有当 `ArrayType` 已经带有显式 `NumElements`
- 且元素个数非 0

时才计算 `ElemSize` 并调用 `withSize()` 扩张数组。

对 unsized array：

- 暂时保持原状
- 不再强行 `.value()`

这么做的原因是：

- 当前上下文并不总能可靠反推 element count
- `craftStruct()` 持有的是 layout 层信息，不足以替代上游数组语义判定
- 对 unsized array 盲目定型比“不扩张”更容易制造错误布局
- 保持 unsized 至少还能把 element type 信息继续传给后续阶段

### 第五层问题：byte-stride 子问题会生成“不可能的元素布局”

继续在 `convertStruct()` 的断点上看局部变量后，定位到：

- 当前递归层 `PointeeSize = 1`
- 但 `Fields[0]` 的范围是 `{Start = 0, Size = 4}`
- 对应 `RawFields[0]` 已经被剥成了 `offset = 0, access = {}`

这说明原始输入里存在类似：

- `@0+1i`

这样的 byte-stride 子问题。去掉 stride 以后，里面还保留了一个 4-byte payload type，于是当前递归元素大小只有 1 byte，却被要求容纳 4-byte 布局。

原始代码在这里直接断言：

```text
assert(!(PointeeSize.value() < Size) && "TODO")
```

### 这个问题在调用链上的具体位置

这一层不是一个孤立断点，而是 `TypeBuilder` 在处理“数组元素类型内部还是 record/recursive type”时自然走出来的结果。和这次问题直接相关的调用链是：

1. `src/TypeRecovery/mlsub/TypeBuilder.cpp`
   中的 `TypeBuilder::convertStruct()`
   - 位置：约 [672](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L672) 到 [686](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L686)
   - 作用：把带 stride 的 `OffsetRange` 拆成 `SubProblem`，并递归调用
     `convertStruct(T, SubProblem, MaxStride)`
   - 这里的 `MaxStride` 就会作为下一层子问题的 `PointeeSize`

2. 同文件中的 `TypeBuilder::convertFieldType()`
   - 位置：约 [209](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L209) 到 [215](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L215)
   - 作用：当某个 field 本身又是 `URecordType` 时，再次把 binarysub 的 field 集合转成 `RawFields`，继续走 `convertStruct()`

3. 同文件中的 `TypeBuilder::convertPointer()`
   - 位置：约 [927](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L927) 到 [944](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L944)
   - 作用：当 pointer 的 load/store side 或 record-like pointee 还需要进一步落到 HType 时，继续进入 `convertStruct()`

4. 同文件中的 `TypeBuilder::craftStruct()`
   - 位置：约 [386](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L386) 开始
   - 作用：在 `convertStruct()` 已经合成出 `Fields` 和 `ValidRange` 之后，真正把它们落成 record shell

5. 最后回到同文件 `TypeBuilder::convertStruct()`
   - 位置：这次实际修复点在 [894](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L894) 到 [915](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L915)
   - 这里会先根据 `Fields` 推出 `Size`
   - 然后拿 `PointeeSize` 和 `Size` 做一致性检查

所以，第五层问题本质上是：

- 上游 stride 拆分是在 `convertStruct()` 里发生的
- 中间 field 类型继续递归展开会经过 `convertFieldType()` / `convertPointer()`
- 最终“元素边界”和“内部布局大小”冲突，还是在 `convertStruct()` 的末尾被集中发现

### 为什么会出现 `PointeeSize = 1` 但 `Size = 4`

关键点在于 `convertStruct()` 对数组子问题的建模方式：

- 外层看到一个带 stride 的 field，例如可以抽象成 `@0+1i`
- 这说明“每个元素的步长”为 1 byte
- 因而递归处理子问题时，调用的是：
  `convertStruct(T, SubProblem, MaxStride)`
- 这里的 `MaxStride` 就是 `1`

也就是说，下一层递归从 API 语义上已经被告知：

- “你现在是在构造一个 1-byte 元素的内部类型”

但与此同时，`SubProblem` 里残留的 payload type 仍然可能来自更宽的 binarysub 记录或 pointer side，例如：

- 一个 32-bit primitive
- 一个 4-byte pointer-like payload
- 或一个递归 record 在这一层仍被视作 4-byte field

于是就出现了这次 `gdb` 看到的状态：

- 当前子问题的边界：`PointeeSize = 1`
- 当前子问题合成出的实际 layout：`Fields[0].Size = 4`
- 最终整体 `Size = 4`

这两者在语义上是不可能同时成立的：

- 如果这是一个 1-byte array element
- 那么它的 materialized field layout 就不能要求至少 4 byte

### 原始代码是怎么暴露这个冲突的

这次触发冲突的位置在：

- [TypeBuilder.cpp:894](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L894)
  到
- [TypeBuilder.cpp:900](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L900)

原始逻辑是：

1. 根据 `Fields` 的最大结束偏移求出 `Size`
2. 如果 `PointeeSize > Size`，就把 `Size` 扩到 `PointeeSize`
3. 但如果 `PointeeSize < Size`，直接断言失败

也就是它只支持：

- “推导布局太小，可以补大”

不支持：

- “推导布局太大，和既定元素边界冲突”

而这个 case 命中的正是后者。

### 对应修复

当发现：

- `PointeeSize` 已知
- 且 `PointeeSize < synthesized layout size`

时，不再断言退出，而是回退为：

- 保留当前 `PointeeSize` 边界
- 丢弃这个递归层里超宽的内部 field 细节
- 返回一个 sized opaque shell（通过 `craftStruct({}, ValidRange, ...)`）

### 这次具体改了哪里

本次第五层问题的修复只改了一个函数、一个位置：

- 文件：
  [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)
- 函数：
  `notdec::mlsub::TypeBuilder::convertStruct`
- 具体位置：
  [897](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L897)
  到
  [903](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L903)

修改前，这里是：

```cpp
if (PointeeSize.value() > Size) {
  Size = PointeeSize.value();
}
assert(!(PointeeSize.value() < Size) && "TODO");
```

修改后，这里变成：

```cpp
if (PointeeSize.value() < Size) {
  return craftStruct({}, SimpleRange{.Start = Fields.front().first.Start,
                                     .Size = PointeeSize.value()},
                     std::nullopt, &T);
}
if (PointeeSize.value() > Size) {
  Size = PointeeSize.value();
}
```

也就是说，这一刀只改变了 `convertStruct()` 在

- “已知 `PointeeSize`”
- 且 “`PointeeSize < 当前合成出的 layout size`”

时的行为。

### 修改后各个函数的职责关系

这次修完以后，相关函数之间的职责边界更清楚了：

1. `convertStruct()`
   - 负责从 `RawFields` 合成候选 `Fields`
   - 负责在 record layout 最终落地前，检查它是否和 `PointeeSize` 一致
   - 当发现 `PointeeSize < Size` 时，现在改为主动降级，而不是崩溃

2. `craftStruct()`
   - 仍然只负责“按给定边界把 struct 壳子造出来”
   - 这里被 `convertStruct()` 复用来创建一个只有 padding / opaque shell 的 record
   - 这次没有改 `craftStruct()` 的接口，只是改变了它被调用的时机和输入

3. `convertFieldType()`
   - 仍然负责把 field payload 的 `UType` 递归转成 HType
   - 这次没有直接修改它，但第五层问题会经过它的递归路径暴露出来

4. `convertPointer()`
   - 仍然负责 pointer-like payload 的递归展开
   - 这次也没有直接修改它，但它是把更宽 payload 带入子问题的重要通道之一

### 为什么这里选择 `craftStruct({}, range, ...)` 这种修法

修第五层问题有几种可能思路：

1. 保留 `Fields`，硬把它们裁剪到 `PointeeSize`
2. 把过宽 field 改写成更小的 primitive
3. 直接丢弃这一层内部 field 细节，但保留元素边界

这次选第 3 种，原因是它最符合当前代码现状：

1. 当前 `convertStruct()` 还没有可靠的“过宽 field 裁剪”实现
   - 同文件 [514](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L514)
     一带对 intersecting member 仍然是 `TODO`
   - 说明这条路径本来就没有完备的精确裁剪能力

2. `PointeeSize` 在这里是上游 stride 拆分给出的硬边界
   - 这个边界比当前递归层推出来的内部布局更可信
   - 当两者冲突时，优先保边界比优先保内部细节更安全

3. `craftStruct({}, ValidRange, ...)` 已经具备“造一个仅保留大小边界的 shell”能力
   - 在同文件 [405](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L405)
     到 [417](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L417)
   - 当 `Fields` 为空但 `ValidRange` 已知时，它会创建一个只含 padding 的记录壳
   - 这正好符合这里想表达的语义：
     “我知道这个元素有多大，但当前不知道怎样安全地把内部字段落出来”

所以，这次修复的实际语义是：

- 不再试图把“不可能的 4-byte 内部布局”塞进一个 1-byte 元素
- 改为显式承认这一层只能保住边界，暂时无法保住内部细节
- 用已有的 `craftStruct()` 能力，返回一个大小正确的 opaque record shell

### 这次修复的效果

在这处修改之后：

- `fortune.o3.wasm.1.ll` 不再因为 `PointeeSize < Size` 在类型恢复阶段崩溃
- 反编译链路可以继续进入后端
- 最终已经能生成 `/tmp/out.c`

因此，第五层修复的价值不是“把类型完全修准”，而是：

- 在已知边界和未知内部细节冲突时，选择了一个不会破坏边界、也不会让程序崩溃的保守落地策略

这样做的理由是：

- 这个子问题的真实边界是已知的
- 当前推导出来的内部布局与边界冲突，直接保留会构造出不可能存在的元素类型
- 用 opaque shell 降级至少能保住 aggregate boundary，避免整个类型恢复链路在这里崩溃

## 当前状态

在上述修复后，下面这条命令已经可以完成反编译并产出 C 文件：

```bash
env \
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

运行结果已到：

```text
Decompile result: /tmp/out.c
```

说明本次主问题已经从“类型恢复阶段崩溃”修到“可以完成反编译输出”。

## 额外发现：`dec` 调试配置本身还有一个输出路径问题

继续用 `.vscode/launch.json` 里 `dec` 配置的等价绝对路径命令测试后，发现程序虽然已经不再在类型恢复阶段崩溃，但仍会因为输出路径失败：

```text
Cannot open output file.
No such file or directory
```

原因是当前 `dec` 配置里写的是：

```json
"-o", "/tmp/${file}.c"
```

在 VS Code 中 `${file}` 会展开成绝对路径，例如：

- `/sn640/NotDec/test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll`

于是最终输出路径会变成：

- `/tmp//sn640/NotDec/test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll.c`

对应父目录并不存在，所以会报 “Cannot open output file”。

这属于调试配置问题，不是本次 `TypeBuilder` 崩溃的根因。

### 这个配置问题的修复方向

应把 `-o` 改成不会把整个绝对路径拼进 `/tmp/` 的形式，例如：

- 使用固定文件名
- 或只取 basename
- 或改到已有目录下

例如调试当前 case 时，像下面这样就是安全的：

```text
-o /tmp/fortune.o3.wasm.1.c
```

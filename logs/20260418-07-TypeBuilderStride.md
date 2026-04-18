# 2026-04-18 TypeBuilder Stride 问题总结

日期：2026-04-18

## 目的

围绕当前 `fortune.o3.wasm.4.ll` 中 `main` 的这条日志：

```text
[TypeBuilder::convertStruct] begin root=Func: main [stable=@main] path=pointer-record(fields=1, pointee_size=<none>) pointee_size=<none> raw_fields=1
  [0] range=@0+4i offset=0 access=[{size=4, count=0}] accessed_bits=64 accessed_bytes=8 utype='o92 & {@0+4i: 'p92 & {}, @4+4i: 'q92 & {}}
```

把当前现象背后更通用的问题抽象出来，重点回答：

1. 为什么一个外层 `@0+4i` 的 field，内部又会出现 `@0+4i` / `@4+4i`
2. 这类嵌套什么时候应该视为“同一条数组轴上的重写”，什么时候才真的是“元素内部 struct 内容”
3. 最合适的修复层级是在 `PNDiff / MLsub 输入`、`binarysub 输出规范化`，还是 `TypeBuilder` 末端兜底

## 本次修改

1. 新增 `logs/20260418-07-TypeBuilderStride.md`
   - 文件：`logs/20260418-07-TypeBuilderStride.md`
   - 行：全文
   - 作用：记录本次关于 stride / nested record / flattening 的设计分析

2. 本次未修改代码
   - 但本分析直接关联以下代码位置和函数：
   - `src/TypeRecovery/mlsub/PNDiff.cpp:174-231`
     - 函数：`matchOffsetRange`
   - `src/TypeRecovery/mlsub/PNDiff.cpp:300-325`
     - 函数：`PNIGraph::eraseConstraint`
   - `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:198-205`
     - 函数：`MLsubGenerator::setAsPtrAdd`
   - `external/NotDec-llvm2c/lib/notdec-llvm2c/Range.cpp:20-35`
     - 函数：`OffsetRange::operator+`
   - `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/Range.h:132-143`
     - 函数：`OffsetRange::maxAccess`
   - `src/TypeRecovery/mlsub/TypeBuilder.cpp:515-549`
     - 函数：`TypeBuilder::accessedPointeeSizeInBits`
   - `src/TypeRecovery/mlsub/TypeBuilder.cpp:767-923`
     - 函数：`TypeBuilder::convertStruct`
   - `src/TypeRecovery/mlsub/TypeBuilder.cpp:1124-1147`
     - 函数：`TypeBuilder::convertStruct`
       中的 `pointee/layout mismatch` 检查

## 当前 case 的最小反例

当前 `main` 中对应的 IR 片段在：

- `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll:396-429`
- `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll:500-509`

关键访问链可以概括为：

```llvm
%59  = %_arg_1 + 4 * %6
%69  = %59 + 4 * %_local_8.0
%71  = load i32, i32* %70

%103 = %59 + 4 * (%_local_8.0 + 1)
%105 = load i32, i32* %104
```

在 `binarysub-trace.log` 中，对应变成：

- `main::arg1 -> main::%bb.brif_next47.i1` reify 为 `@0+4i`
- `main::%bb.brif_next47.i1 -> main::%bb.loop_entry51.i2` reify 为 `@0+4i`
- `main::%bb.brif_next47.i1 -> main::%bb.blk_exit76.i1` reify 为 `@4+4i`

于是最终在 `TypeBuilder` 看到的不是：

- “一个 4-byte 元素内部有两个 4-byte 成员”

而是：

- “从某个已被视作 `@0+4i` 的中间基址出发，又沿着同一个 stride=4 的轴继续走了两次”

这在当前日志里被错误地物化成了：

- 外层 field：`@0+4i`
- 内层 payload：`{@0+4i, @4+4i}`

然后 `TypeBuilder::accessedPointeeSizeInBits()` 又把它解释成“最远覆盖到 offset 8”，于是出现：

- `accessed_bits=64`
- `accessed_bytes=8`
- `pointee_size=4`
- `synthesized_size=8`

## 更通用的问题是什么

### 1. 当前系统把“同轴重写”和“元素内部布局”混在了一起

`PNDiff` 在 `PNIGraph::eraseConstraint()` 中，一旦某条 `ptr + int -> ptr`
约束被解出来，就调用：

- `MLsubGenerator::setAsPtrAdd(basePtr, result, Off)`

而 `setAsPtrAdd()` 做的事情是：

1. 写入 `base <: record{Off: result}`
2. 写入 `result <: record{}`
3. `unify(basePtr, result)`

这意味着：

- “ptradd 结果仍然是同一个指针等价类”
- 但它同时又被编码成了“base 下挂了一个 `Off` 字段，字段值又是这个等价类里的一个节点”

如果这种 ptradd 连续发生，并且 stride 一样，那么图里自然会长出：

- 外层 `@0+4i`
- 内层再来一个 `@0+4i`
- 甚至再加一个 `@4+4i`

这时，类型图真正表达的更接近：

- “这是同一条数组轴上的 rebase / slice / sibling step”

而不是：

- “这是某个元素内部的嵌套 record”

### 2. `OffsetRange` 只保留了覆盖信息，没有保留 provenance

`matchOffsetRange()` 和 `OffsetRange::operator+()` 都能很好地描述：

- 常量偏移
- stride 访问
- add/mul/shl 组合后的覆盖形状

但它们没有保留一个关键语义：

- 这个 offset 是“在当前对象内部访问成员”
- 还是“把当前指针 rebasing 到同一条数组轴上的另一个观察点”

等到 `MLsubGenerator::setAsPtrAdd()` 把它写成字符串 field name 之后，
`TypeBuilder` 只看到：

- `@0+4i`
- `@4+4i`
- 某个嵌套 `URecordType`

已经看不到这条 field 最初来自：

- “一次 ptradd”
- “同一个 base object”
- “还是跨过 load / pointer dereference 进入了另一个对象”

这就是为什么越晚处理，这件事越容易退化成 heuristic。

### 3. `TypeBuilder` 会把所有嵌套 record 都尝试当成当前元素的 material layout

`TypeBuilder::convertStruct()` 当前的基本思路是：

1. 先按 `OffsetRange.access` 提取 stride 子问题
2. 对每个 stride 子问题递归调用 `convertStruct(T, SubProblem, MaxStride)`
3. 剩余 field 用 `accessedPointeeSizeInBits()` 算大小
4. 最后合成 layout

这个过程默认有一个很强的假设：

- 如果某个 field 的 payload 本身又是 `URecordType`
- 那它就是“当前字段内部的结构”

但当前 case 里这个假设不成立。  
内层 record 不是“元素内部结构”，而是“同轴 ptradd 重写后留下来的 slice 视图”。

## 角度一：从覆盖范围看，什么时候该嵌入，什么时候不该

### 当前外层 `@0+4i` 的语义

如果当前递归层的 `PointeeSize = 4`，那么外层 `@0+4i` 对应的“当前元素窗口”可以近似看成：

- 当前元素有效范围：`[0, 4)`

这只是一个 conservative window，但足够用来判断：

- 某个 child range 是不是“元素内部内容”

### 三类情况

#### A. 完全相同的重写：应该忽略或平坦化

例子：

- parent：`@0+4i`
- child：`@0+4i`

child 覆盖的仍然是：

- `[0, 4)`

这不是“元素内部新增了一个成员”，而是：

- “我又把当前元素当成同一条 stride=4 轴上的观察点重写了一次”

这种完全相同的重写，本质上是 self-like rewrite。

对当前问题而言，这种 child：

- 不应该继续嵌入成 struct field
- 更像应该被 drop / ignore / flatten

#### B. 落到相邻或更远 sibling window：也不该嵌入

例子：

- parent window：`[0, 4)`
- child：`@4+4i`

它保守覆盖的是：

- `[4, 8)`

这显然已经不在当前元素内部，而是：

- 当前元素的下一个 sibling window
- 或更一般地说，是“同一条 stride 轴上的另一个元素位置”

这类 child 也不该嵌入成当前元素内部 struct 内容。

它应该被视为：

- 同轴 flatten / hoist
- 而不是 inner member

#### C. 真正落在当前窗口内部：才允许嵌入

只有当 child 的覆盖范围真正在当前窗口内部时，才应该把它当成当前元素的物化内容。

例如在 `stride = 4` 的元素里：

- `@0`
- `@1`
- `@2`
- `@3`
- `@0+1i`
- `@1+1i`
- `@2+1i`
- `@3+1i`

这类 range 如果它们的 conservative coverage 完全落在 `[0, 4)` 内，
才有资格被嵌入成 struct field。

### 更通用的判定标准

在进入 `convertStruct(..., PointeeSize = S)` 的 stride 子问题时，
可以把“是否允许嵌入”为当前元素 field，抽象成下面的 containment 规则：

1. 如果 child 是“同 stride 的完全相同重写”
   - 例如 child 仍携带与当前层相同的 stride `S`
   - 且其覆盖窗口等于当前窗口
   - 那么它应视为 self-like rewrite，不物化为 inner field

2. 如果 child 的 conservative coverage 超出 `[0, S)`
   - 那么它不是当前元素内部内容
   - 应当 hoist / flatten / ignore，而不是嵌入

3. 只有当 child coverage 完全包含于 `[0, S)` 内时
   - 才允许它变成当前元素的 struct 内容

这比“只要 payload 是 record 就继续递归展开”更接近真实语义。

## 角度二：同一个值上的 `@0+ai` 递归，也许就不该分层

### 当前现象更像“同轴 re-entry”，不是“新的聚合层”

如果从某个值开始，连续出现：

- `@0+ai`
- 再来一个 `@0+ai`
- 再来一个 `@4+ai`

并且这些关系都来自：

- 同一个 base object 上的 ptradd
- 没有跨过 load / dereference / 新 pointee object

那么它们更像是在表达：

- 同一条数组轴上的 re-entry

而不是：

- 一层数组元素里又嵌套了一层同 stride 的数组元素

换句话说，这里的“递归深度”是实现细节，不应默认当成“语义层级”。

### 更激进但更统一的想法：同轴 stride 访问全部平坦化

对这类“从同一个值开始、无论递归深度如何出现的 `@0+ai` 重写”，
可以考虑一个更统一的 canonicalization：

- 只保留一条扁平化后的 stride 轴
- 不把重复的同轴重写解释成 nested record

对当前 case：

- 外层 `@0+4i`
- 内层 `@0+4i`

组合后仍然是：

- `@0+4i`

而：

- 外层 `@0+4i`
- 内层 `@4+4i`

则仍然是同一轴上的 sibling step：

- `@4+4i`

这和 `OffsetRange::operator+()` 的代数方向也是一致的：

- offset 相加
- stride 集合并

### 但这个结论有一个前提

“无论递归深度如何都平坦化”只有在下列前提下才安全：

- 当前仍然在同一个 base object 的同一条数组轴上

如果已经跨过：

- load 得到新的 pointer value
- 某个真实 pointer field 指向另一块内存
- 某个新的 pointee object

那么新的 `@0+ai` 可能真的是另一个对象上的数组访问，
此时就不能再按“同轴 flatten”处理。

所以更准确的说法不是：

- “所有 `@0+ai` 都该 flatten”

而是：

- “所有来自同一对象、同一条 ptradd 轴的同轴 `@0+ai` re-entry 都应该 flatten”

这正是当前系统最缺的 provenance 信息。

## 角度三：最好的修复层级在哪里

### 结论

最好的修复层级仍然是：

- `PNDiff / MLsub 输入` 侧

其次才是：

- `binarysub 输出到 TypeBuilder 之间的规范化`

最差但仍可兜底的是：

- `TypeBuilder` 末端 heuristic

### 为什么最应该在上游修

在 `PNDiff::eraseConstraint()` 和 `MLsubGenerator::setAsPtrAdd()` 这一层，
系统还知道：

1. 这条关系来自一条 `ptr + int -> ptr`
2. `basePtr` 和 `result` 属于哪条约束链
3. 当前是不是连续沿着同一个 base 做 ptradd
4. 当前有没有跨过 load / dereference

也就是说，只有在这一层，系统还拿着“层次关系的 provenance”。

如果能在这里处理：

- 哪些是同轴 re-entry
- 哪些是 sibling window
- 哪些才真是 inner layout

那后面的 `binarysub` 和 `TypeBuilder` 就不需要猜。

### 如果上游不改，越往后越像 heuristic

等到 `TypeBuilder` 阶段，手里只剩下：

- `OffsetRange` 字符串
- `URecordType`
- `PointeeSize`

已经不知道：

- 这个 nested record 是因为 ptradd-slice 造成的
- 还是因为真实 field/pointee 递归造成的

这时做 flatten 虽然可能有效，但本质上是在做：

- “基于 coverage 的结构猜测”

不是基于真实 provenance 的规范化。

## 三层可选方案

### 方案 A：在 `PNDiff / MLsubGenerator` 侧做 canonicalization

这是最推荐的方向。

核心思想是：

- 不要把“同轴 ptradd re-entry”直接编码成会诱导 nested record 的结构

可选实现方向：

1. 在 `setAsPtrAdd()` 之前维护一份 ptradd side table
   - 先只记录：
     - root/base
     - result
     - offset
     - axis/stride
   - 等这条链规范化完毕后，再统一下沉到 record 约束

2. 在 `setAsPtrAdd()` 内部识别“同轴 re-entry”
   - 如果 `basePtr` 自己已经是某个 root 上的同轴 slice
   - 那么 `basePtr --Off--> result` 不要变成新的 nested layer
   - 而是组合成 root 上的扁平 offset

3. 引入“轴”或“view”概念
   - 让 ptradd 产生的是：
     - array-axis rebasing
   - 而不是立即把它视为：
     - object-in-object 的 field path

这层修复的最大优点是：

- provenance 还在
- 能区分“同一对象的 slice”与“新 pointee object”

### 方案 B：在 binarysub 输出、进入 TypeBuilder 前做 record 规范化

如果上游暂时不好改，可以考虑加一个中间规范化 pass：

- 输入：`URecordType`
- 输出：做过 same-axis flatten 的 `URecordType`

这个 pass 可以做的事情包括：

1. 遍历 `record` field payload 中再次出现的 `URecordType`
2. 如果 child field 与 parent field 属于 same-axis re-entry 形状
   - 把 child range 组合回 parent
   - 例如按 `ParentRange + ChildRange` 进行扁平化
3. 对完全相同的 rewrite 直接去掉
4. 对超出当前 element window 的 child 做 hoist，而不是嵌入

这层的好处是：

- 改动面比 `PNDiff` 小
- 能显式把 nested record 规范化成扁平 field 集

但缺点也很明显：

- provenance 已经丢了一部分
- 只能依赖 shape / coverage / stride 重合来猜

### 方案 C：在 TypeBuilder 里做 containment-based fallback

这是最晚的一层，也是最 heuristic 的一层。

可以考虑在 `convertStruct()` 里增加一个“stride 子问题筛选”：

1. 当前层如果 `PointeeSize = S`
2. 看 child field 的 coverage 是否在 `[0, S)` 内
3. 如果是 self-like same-axis rewrite
   - drop / ignore
4. 如果超出边界
   - 不把它嵌入当前 struct
5. 只有真正 contained 的 child 才继续转成 inner field

这能直接避免当前这种：

- `pointee_size = 4`
- `synthesized_size = 8`

的 inflated layout。

但它的本质仍然是：

- “最后一层补丁”

它能止血，却很难保证对所有 case 的层次语义都判断正确。

## 当前更偏向的结论

### 结论 1：当前 bug 不只是 `TypeBuilder` 的 size 计算问题

`TypeBuilder` 的 8-byte inflate 只是最后暴露症状的位置。  
更深层的问题是：

- 上游把“同轴 ptradd re-entry”编码成了看起来像 nested record 的结构

### 结论 2：嵌入条件应该从“递归存在”改成“覆盖真的落在内部”

当前更合理的原则是：

- 不是因为 child 是 nested record，就把它当 inner field
- 而是只有 child coverage 真正 contained in 当前 element window，
  才能嵌入

### 结论 3：对同一对象上的同轴 `@0+ai` re-entry，应优先平坦化

当前 case 明显支持这个判断：

- `@0+4i` 下再来 `@0+4i`
- 不是更深一层 struct
- 而是同一条数组轴上的重复重写

### 结论 4：最佳修复层级仍然是 `PNDiff / MLsub 输入`

因为只有那一层还知道：

- 这是 ptradd
- 还是内存对象边界
- 有没有跨对象

一旦进入 `TypeBuilder`，这件事就只能靠 shape 猜。

## 建议的实际推进顺序

如果后续要落地实现，我当前更建议按下面顺序推进：

1. 先在 `TypeBuilder` 里做一个小型 containment-based trace / fallback
   - 目的是验证这套“same-axis rewrite 不该嵌入”的判断是否能稳定消除
     `fortune.o3.wasm.4.ll` 里的 8-byte inflate
   - 这一层不追求最终语义最优，只验证判定方向

2. 确认方向后，把规范化前移到 `PNDiff / MLsubGenerator`
   - 真正解决“同轴 ptradd 被错误分层”的根因

3. 如果前移改动过大，再考虑补一个 binarysub -> TypeBuilder 之间的
   `URecordType` flatten pass
   - 作为过渡方案

## 当前一句话总结

当前问题的本质不是“`@0+4i` 为什么会变成 8 byte”，而是：

- 系统把“同一条 stride 轴上的 ptradd 重写”误当成了“当前元素内部的嵌套布局”

因此，真正需要建立的规则不是：

- “看到 nested record 就继续展开”

而是：

- “只有 coverage 真正落在当前元素窗口内部的 child，才允许物化成
  struct 内容；同轴 self/sibling rewrite 应优先 flatten”

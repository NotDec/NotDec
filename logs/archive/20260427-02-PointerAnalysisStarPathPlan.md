# 2026-04-27 PointerAnalysis star path plan

## 背景

replace 模式下，PointerAnalysis 当前把内存对象位置表示成：

```cpp
MemoryLocKey {
  ExtValuePtr Root;
  std::vector<OffsetRange> Path;
  unsigned BitSize;
}
```

当循环里存在指针加法：

```llvm
%p = phi i32 [ %base, %entry ], [ %next, %loop ]
%next = add i32 %p, 1
```

PA 会不断生成：

```text
Root
Root.@1
Root.@1.@1
Root.@1.@1.@1
...
```

这样 points-to 集合不会收敛，`std::set<MemoryLocKey>` 还会反复比较越来越长
的 `Path`。

已新增复现用例：

- `test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll`

replace 模式下 10 秒超时，gdb 栈显示耗时在：

```text
PointerAnalysis::solve()
PointerAnalysis::addPointsTo()
std::set<MemoryLocKey>::insert()
MemoryLocKey::operator<()
OffsetRange::operator<()
```

## 范围

这里不做完整正则表达式代数，只做 PA path 需要的两件事：

1. 普通 field 串接。
2. 重复片段的 `star` 闭包和少量局部化简。

不需要 `Or`，因为 points-to 的多路径已经由 `std::set<MemoryLocKey>` 表达。
每个 `MemoryLocKey` 只需要描述一条对象位置路径。

完整正则代数会复杂很多，主要复杂在等价判断和化简规则，例如：

```text
(AB)*A
A(BA)*
(A*)*
(A|B)*
```

这些表达式之间有些语义等价，有些只是在特定上下文下可合并。当前 PA 不需要解决这些
一般问题。只要让循环 field path 收敛即可。

第一版只实现结构上完全相同的局部化简：

```text
Star(X) Star(X) => Star(X)
Star(X) X       => Star(X)
Star(Star(X))   => Star(X)
```

其中 `X` 是完全相同的 path atom 序列，不做复杂代数推导。

## 目标

让带循环 ptradd 的 points-to 集合收敛。

例如：

```text
@1 @2 @3 @1 @2 @3 ...
```

应归一化为：

```text
(@1 @2 @3)*
```

之后：

```text
(@1 @2 @3)* @1 @2 @3
```

继续归一化回：

```text
(@1 @2 @3)*
```

这样 `MemoryLocKey` 不再无限增长，PA worklist 能结束。

## 核心设计

### 1. Path 元素带 tag

当前 `Path` 只有 `OffsetRange`。需要让每段 field 带来源 tag：

```cpp
struct PAFieldTag {
  enum class Kind {
    IRPtrAdd,
    VirtualPtrAdd,
  };

  Kind K;
  ExtValuePtr Value;
  std::uint64_t ContextId = 0;
  std::uint64_t VirtualId = 0;
};

struct PAPathAtom {
  OffsetRange Offset;
  PAFieldTag Tag;
};
```

说明：

- `IRPtrAdd`：真实 IR / PNDiff ptradd reify 产生的 field。
- `VirtualPtrAdd`：summary 或 future virtual instruction 产生的 field。
- `ContextId`：区分多态 summary 实例化上下文。
- `VirtualId`：没有真实 IR value 时，给虚拟 ptradd 一个稳定身份。

tag 不能只用指令地址，原因：

1. 多态 summary 实例化时，同一条 summary 内指令会在不同调用上下文被复制使用。
2. summary 里可能直接描述 pointer offset 约束，没有真实 IR 指令。
3. 后续如果 summary 重构成虚拟指令流，tag 应来自虚拟指令事件。

tag 也不能每次传播都 fresh。循环中同一条 ptradd 必须得到同一个 tag，否则无法识别重复。

### 2. Path 支持 star

新增 PA 专用 path 类型：

```cpp
struct PAPathStar {
  std::vector<PAPathAtom> Body;
  OffsetRange Summary;
};

using PAPathElem = std::variant<PAPathAtom, PAPathStar>;

struct PAPath {
  std::vector<PAPathElem> Elems;
};
```

`PAPathStar::Summary` 是 `Body` offset 求和后 `mulx()` 的结果。

例子：

```text
Body = [@1, @2, @3]
sum  = @6
Summary = @6.mulx() = @0+6i
```

这里 `Summary` 只是粗粒度解释和 fallback 用。精确吸收后续片段时仍然用
`Body` 做序列匹配。

### 3. MemoryLocKey 改用 PAPath

从：

```cpp
std::vector<OffsetRange> Path;
```

改成：

```cpp
PAPath Path;
```

`formatMemoryLocKey()` 需要支持打印：

```text
root.@1
root.(@1.@2.@3)*
root.(@1.@2.@3)*.@1
```

## 比较函数

用户提到“实现好这种带 star 的 path 的小于和等于运算符，就可以比较容易地判断重复”，这个判断是对的。

需要给以下类型全部实现值语义：

```cpp
bool operator==(const PAFieldTag &, const PAFieldTag &);
bool operator<(const PAFieldTag &, const PAFieldTag &);

bool operator==(const PAPathAtom &, const PAPathAtom &);
bool operator<(const PAPathAtom &, const PAPathAtom &);

bool operator==(const PAPathStar &, const PAPathStar &);
bool operator<(const PAPathStar &, const PAPathStar &);

bool operator==(const PAPathElem &, const PAPathElem &);
bool operator<(const PAPathElem &, const PAPathElem &);

bool operator==(const PAPath &, const PAPath &);
bool operator<(const PAPath &, const PAPath &);
```

建议排序规则：

```text
PAFieldTag:
  tie(Kind, ContextId, VirtualId, Value)

PAPathAtom:
  tie(Tag, Offset)

PAPathStar:
  tie(Body, Summary)

PAPathElem:
  Atom < Star
  同 variant index 时比较内部值

PAPath:
  lexicographical_compare(Elems)
```

这样：

1. `std::set<MemoryLocKey>` 能稳定去重。
2. 判断 suffix 是否等于 star body 可以直接用 vector equality。
3. 查找 “当前 atom 的 tag 是否在已有 path 中出现过” 可以用 `operator==`。

注意：`Summary` 不应该替代 `Body` 参与语义判断。两个 star 的 `Summary` 一样，不代表
body 一样。例如 `@1,@2,@3` 和 `@2,@1,@3` 都可能 sum 成 `@6`，但后续吸收行为不同。

## 归一化规则

每次 append field 后，都调用：

```cpp
PAPath appendAndNormalize(PAPath Old, PAPathAtom Atom);
```

### 规则 A：形成新 star

如果 append 后，当前 `Atom.Tag` 在前面的普通 atom 中出现过：

```text
Prefix A B C A
```

其中最后一个 `A` 是新 append 的 atom，那么折叠成：

```text
Prefix (A B C)*
```

新 append 的 `A` 不再额外保留，因为它表示下一轮循环的开头，已被 `*` 覆盖。

多候选时选最近一次重复，也就是最短 body：

```text
A B A B A
```

优先折成：

```text
(A B)*
```

### 规则 B：star 吸收完整 suffix

如果 path 末尾是：

```text
Prefix Star(Body) Body
```

归一化为：

```text
Prefix Star(Body)
```

这对应用户描述的第二轮：

```text
Star(@1,@2,@3) @1 @2
```

暂时保留；再 append `@3` 后：

```text
Star(@1,@2,@3) @1 @2 @3
```

吸收回：

```text
Star(@1,@2,@3)
```

### 规则 C：简单 star 化简

完整正则表达式代数不做，但可以实现几个简单规则。

#### C.1 相邻相同 star 合并

如果出现：

```text
Star(A,B) Star(A,B)
```

归一化为：

```text
Star(A,B)
```

判断标准是 `Star.Body` 完全相等。

#### C.2 嵌套 star 压平

如果出现：

```text
Star(Star(X))
```

归一化为：

```text
Star(X)
```

第一版可以只处理 body 里唯一元素也是 star 的情况。复杂情况例如：

```text
Star(A, Star(B,C))
```

先不展开，因为它不一定能简单变成一个等价的普通 body。

#### C.3 star 吸收自身 body

如果出现：

```text
Star(X) X
```

归一化为：

```text
Star(X)
```

这条和规则 B 是同一个方向，只是写成通用化简规则，方便后续在
`normalizePAPath()` 里反复执行。

### 规则 D：保底限制

即使有 star，也保留一个硬上限，例如：

```text
MaxPathElems = 16
```

超过后打印warning并提醒用户。

## OffsetRange 与 star summary

`OffsetRange` 已经支持 stride：

```cpp
OffsetRange{.offset = 0, .access = {ArrayOffset(Stride)}}
```

并且已有：

```cpp
OffsetRange::mulx()
```

它能把固定偏移提升成数组访问：

```text
@4.mulx() => @0+4i
```

所以 star body 的 summary 可以这样算：

```cpp
OffsetRange Sum;
for (auto &Atom : Body) {
  Sum = Sum + Atom.Offset;
}
Summary = Sum.mulx();
```

注意：

1. `Body` 仍然必须保留。
2. `Summary` 可以用于打印、fallback、后续 TypeBuilder 解释。
3. 第一版不需要把 `Star(A,B,C)` 直接压平成单个 `OffsetRange`，否则会丢失
   `A,B,C` 的精确匹配能力。

## 代码改动计划

### 文件拆分

新 path 类型不要继续塞进 `PointerAnalysis.h/.cpp`。这两个文件已经同时负责边记录、
points-to 状态、求解和 trace 格式化，再把 star path 的比较和归一化放进去会过大。

新增专门文件：

```text
include/notdec/TypeRecovery/mlsub/PAPath.h
src/TypeRecovery/mlsub/PAPath.cpp
```

职责：

1. 定义 `PAFieldTag`、`PAPathAtom`、`PAPathStar`、`PAPathElem`、`PAPath`。
2. 实现这些类型的 `operator<` / `operator==`。
3. 实现 path 打印。
4. 实现 append 和 normalize。
5. 实现 star summary offset 计算。

`PointerAnalysis.h/.cpp` 只保留：

1. `MemoryLocKey`。
2. points-to 集合。
3. copy/field/load/store-ptr edges。
4. solve 流程。

这样后续给 path 加单测也更容易，不需要构造完整 `PointerAnalysis`。

### 1. `include/notdec/TypeRecovery/mlsub/PAPath.h`

计划：

1. 新增 `PAFieldTag`。
2. 新增 `PAPathAtom`。
3. 新增 `PAPathStar`。
4. 新增 `PAPathElem`。
5. 新增 `PAPath`。
6. 声明这些类型的 `operator<` / `operator==`。
7. 声明：

```cpp
std::string formatPAFieldTag(const PAFieldTag &Tag);
std::string formatPAPathAtom(const PAPathAtom &Atom);
std::string formatPAPathElem(const PAPathElem &Elem);
std::string formatPAPath(const PAPath &Path);

OffsetRange summarizeStarBody(llvm::ArrayRef<PAPathAtom> Body);
PAPath appendAndNormalize(PAPath Path, PAPathAtom Atom);
PAPath normalizePAPath(PAPath Path);
```

### 2. `src/TypeRecovery/mlsub/PAPath.cpp`

计划：

1. 实现所有 path 类型的 `operator<` / `operator==`。
2. 实现格式化函数。
3. 实现 `summarizeStarBody()`：

```cpp
OffsetRange Sum;
for (const auto &Atom : Body) {
  Sum = Sum + Atom.Offset;
}
return Sum.mulx();
```

4. 实现 `appendAndNormalize()`。
5. 实现规则 A/B/C：
   - 重复 atom 折成 star。
   - `Star(X) X => Star(X)`。
   - `Star(X) Star(X) => Star(X)`。
   - `Star(Star(X)) => Star(X)`，只处理 body 唯一元素也是 star 的简单情况。

### 3. `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h`

修改位置：

- 当前 `MemoryLocKey` 在 `PointerAnalysis.h:32` 左右。
- 当前 `FieldEdge` 在 `PointerAnalysis.h:61` 左右。

计划：

1. include 新文件：

```cpp
#include "notdec/TypeRecovery/mlsub/PAPath.h"
```

2. 把 `MemoryLocKey::Path` 从 `std::vector<OffsetRange>` 改为 `PAPath`。
3. `FieldEdge` 增加 tag：

```cpp
struct FieldEdge {
  ExtValuePtr Dst;
  ExtValuePtr Base;
  OffsetRange Field;
  PAFieldTag Tag;
  unsigned BitSize = 0;
};
```

4. `addField()` 签名改成：

```cpp
void addField(ExtValuePtr Dst, ExtValuePtr Base, OffsetRange Field,
              PAFieldTag Tag, unsigned BitSize);
```

也可以先让 `addField()` 内部默认用 `Dst` 构造 `IRPtrAdd` tag，减少调用点改动：

```cpp
void addField(ExtValuePtr Dst, ExtValuePtr Base, OffsetRange Field,
              unsigned BitSize);
```

第一版建议保留旧签名，在内部生成：

```cpp
PAFieldTag{.K = IRPtrAdd, .Value = Dst}
```

后续 summary virtual ptradd 再扩展重载。

### 4. `src/TypeRecovery/mlsub/PointerAnalysis.cpp`

修改位置：

- `formatMemoryLocKey()`
- `getFieldObject()`
- `addField()`
- `solve()`

计划：

1. 修改 `formatMemoryLocKey()` 调用 `formatPAPath()`。
2. 修改 `getFieldObject()`：

```cpp
MemoryLocKey PointerAnalysis::getFieldObject(MemoryLocKey Base,
                                             OffsetRange Field,
                                             PAFieldTag Tag,
                                             unsigned BitSize) const {
  Base.Path = appendAndNormalize(
      std::move(Base.Path),
      PAPathAtom{.Offset = std::move(Field), .Tag = std::move(Tag)});
  Base.BitSize = BitSize;
  return Base;
}
```

3. 修改 `solve()` 中 field edge 的传播：

```cpp
auto FieldLoc = getFieldObject(BaseLoc, Edge.Field, Edge.Tag, Edge.BitSize);
```

### 5. `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

修改位置：

- `setAsPtrAdd()` 当前调用 `PA.addField(result, basePtr, Off, getSize(result))`。

计划：

第一版不改调用点签名，只让 `PA.addField()` 内部用 `Dst` 生成 tag。

如果要显式传 tag，可以新增 helper：

```cpp
PAFieldTag makeIRPtrAddTag(ExtValuePtr Result) const;
```

但不是第一版必要项。

### 4. summary 后续重构

当前 summary override 有两类风险：

1. 只补 BinarySub 约束，没有同步补 PA 约束。
2. 直接写 offset type constraint，没有真实 IR value 可当 tag。

长期建议把 summary 改成虚拟事件流：

```text
ptradd dst, base, off
copy dst, src
load dst, addr, size
store addr, val, size
subtype lhs, rhs
equal lhs, rhs
pndiff ...
```

然后用同一套 visitor 入口消费：

```text
ptradd => BinarySub record constraint + PA field edge
copy   => subtype + PA copy
load   => ptr load view / object content flow + PA load-ptr if pointer
store  => ptr store view / object content flow + PA store-ptr if pointer
```

虚拟 ptradd 的 tag 用：

```cpp
PAFieldTag{
  .K = VirtualPtrAdd,
  .ContextId = SummaryInstantiationId,
  .VirtualId = EventId,
}
```

这部分不在第一版实现范围内，但 path tag 设计要给它留位置。

## 判断重复的算法细节

### append 后找重复 tag

伪代码：

```cpp
PAPath appendAndNormalize(PAPath Path, PAPathAtom Atom) {
  Path.Elems.push_back(Atom);
  absorbStarSuffix(Path);
  foldRepeatedAtom(Path);
  absorbStarSuffix(Path);
  return Path;
}
```

`foldRepeatedAtom()`：

```text
1. 最后一个 elem 必须是 Atom，记为 Last。
2. 从倒数第二个 elem 往前找 Atom。
3. 如果遇到 Star，第一版跳过或停止。
4. 找到相同 tag 且 offset 相同的 Atom。
5. Body = [that atom, ..., before Last]
6. 用 Star(Body) 替换 Body + Last。
```

例子：

```text
P A B C A
```

替换为：

```text
P Star(A,B,C)
```

### star 吸收 suffix

伪代码：

```text
如果末尾形如:
  ... Star(Body) Suffix
且 Suffix == Body
则删除 Suffix。
```

第一版只检查最后一个 star，不做跨多个 star 的复杂匹配。

### 为什么这个能收敛

对于：

```text
A B C A B C A B C ...
```

过程是：

```text
A
A B
A B C
Star(A,B,C)
Star(A,B,C) A
Star(A,B,C) A B
Star(A,B,C) A B C
Star(A,B,C)
```

循环后 points-to 插入的 key 回到旧 key，所以 `addPointsTo()` 返回 false，
worklist 不再继续扩展。

## 测试计划

### 1. 新增单元测试

如果当前没有合适的 PA 单测，可以新增到现有 gtest 目标，或者先放在
`TypeBuilderTest` 附近。

重点测：

1. 单步循环：

```text
@1 @1 => Star(@1)
Star(@1) @1 => Star(@1)
```

2. 三步循环：

```text
@1 @2 @3 @1 => Star(@1,@2,@3)
Star(@1,@2,@3) @1 @2 @3 => Star(@1,@2,@3)
```

3. 不同 tag 不折叠：

```text
A(@1) B(@1) 不折叠
```

4. 相同 tag 但 offset 不同不折叠：

```text
A(@1) A(@2) 不折叠
```

5. `operator<` 稳定：

```text
std::set<PAPath> 能去重相同 path。
```

### 2. 复现用例

命令：

```bash
timeout 10s env NOTDEC_POINTER_ANALYSIS_MODE=replace ./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/20_PointerAnalysisFieldCycle.ll \
  -o /tmp/20_field_cycle_replace.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-field-cycle-replace
```

修复后应不再超时。

### 3. fortune 用例

命令：

```bash
timeout 60s env NOTDEC_POINTER_ANALYSIS_MODE=replace ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-replace.ll --tr-level=2 --frozen-tr-input-ir \
  -g --work-dir=/tmp/notdec-fortune-replace
```

目标：先确认能结束，再看输出差异。

### 4. 默认回归

```bash
python3 test/run_type_recovery_suite.py \
  --binary build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-llvm-ir-star-path
```

默认 original suite 不应变化。

## 风险

1. tag 过粗会误折叠不同上下文的路径。
2. tag 过细会无法识别循环。
3. star path 如果直接影响 TypeBuilder 输出，可能导致新类型打印格式变化。
4. `operator<` 如果不稳定，会破坏 `std::map` / `std::set`。
5. summary virtual tag 如果没有稳定 ContextId，后续 summary 实例化仍可能误合并。

## 第一版边界

第一版只解决：

1. PA 内部 field path 无限增长。
2. 单个 path 里的重复片段 star 化。
3. `20_PointerAnalysisFieldCycle.ll` replace 模式收敛。

暂不解决：

1. 一般正则表达式等价判断。
3. 超出 `Star(X) Star(X)`、`Star(X) X`、`Star(Star(X))` 的复杂化简。
4. summary 虚拟指令流重构。
5. TypeBuilder 对 star 的高级显示。

## 结论

实现带 star 的 path，并给它完整 `operator<` / `operator==`，是当前最合适的
收敛机制。

新类型应拆到 `PAPath.h/.cpp`，避免继续撑大 `PointerAnalysis.h/.cpp`。

它的优点是：

1. 不需要回头修改已经加入的 PA 边。
2. 不需要预先标记全图 SCC。
3. 能处理多步循环，例如 `@1,@2,@3`。
4. 与现有 `MemoryLocKey` 的 set/map 去重机制兼容。
5. 能复用 `OffsetRange::mulx()` 表达循环段的 stride summary。

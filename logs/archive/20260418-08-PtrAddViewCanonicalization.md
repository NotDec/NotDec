# 2026-04-18 PtrAdd View Canonicalization 算法设计

日期：2026-04-18

## 背景

这是对
[logs/20260418-07-TypeBuilderStride.md](/sn640/NotDec/logs/20260418-07-TypeBuilderStride.md)
中“方案 A：在 `PNDiff / MLsubGenerator` 侧做 canonicalization”的进一步细化。

上一份日志已经确认：

1. 当前问题的根因，不只是 `TypeBuilder` 把 size 算大
2. 更深层的问题，是 `ptradd` 被逐步回写成了 nested record
3. 最理想的修复层级，是在 `PNDiff -> MLsub` 的回写入口处理层次关系

这份日志继续回答：

- 方案 A 如果真的落地，推荐用什么算法
- 这个算法应该挂在哪些函数
- 为什么不能直接复用 `PNIMap` 当 root
- 为什么建议“两阶段：先收集 ptr-view，再统一物化”

## 本次修改

1. 新增 `logs/20260418-08-PtrAddViewCanonicalization.md`
   - 文件：`logs/20260418-08-PtrAddViewCanonicalization.md`
   - 行：全文
   - 作用：把方案 A 进一步细化为可实现的算法设计

2. 本次未修改代码
   - 但本分析直接关联以下代码位置和函数：
   - `src/TypeRecovery/mlsub/PNDiff.cpp:300-325`
     - 函数：`PNIGraph::eraseConstraint`
   - `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:198-205`
     - 函数：`ConstraintsGenerator::setAsPtrAdd`
   - `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:90-114`
     - 函数：`ConstraintsGenerator::run`
   - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1688-1708`
     - 函数：`ConstraintsGenerator::addRemapType`
   - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2888-2904`
     - 函数：`ConstraintsGenerator::MLsubVisitor::visitCastInst`
   - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3084-3093`
     - 函数：`ConstraintsGenerator::MLsubVisitor::visitGetElementPtrInst`
   - `external/NotDec-llvm2c/lib/notdec-llvm2c/Range.cpp:20-35`
     - 函数：`OffsetRange::operator+`
   - `src/TypeRecovery/mlsub/PNDiff.cpp:174-231`
     - 函数：`matchOffsetRange`

## 一、先说结论

如果只从“算法是否更稳、更通用、更容易调试”来选，我当前最推荐的不是：

- 在 `setAsPtrAdd()` 里立刻把 `base <: record{Off: result}` 改成另一种即时写回

而是：

- 把 `ptradd` 先收集成一张独立的 `PtrViewFact` 表
- 在 `PG.solve()` 结束后，再按 `(root, total_offset)` 做统一规范化
- 最后一次性把规范化后的结果下沉成 mlsub 约束

也就是说，我更推荐的不是“即时 canonicalization”，而是：

- “延迟物化的 ptr-view canonicalization”

这是方案 A 的推荐主算法。

## 二、为什么不能直接复用 `PNIMap` 当 root

这是当前方案设计里最容易踩坑的点。

表面上看，`PNDiff` 已经有：

- `PNIGraph::PNIMap`
- `PNINode::unify`
- `DSUMap<ExtValuePtr, PNINode *>`

似乎可以把“某个值属于哪个 root”也建立在这套 DSU 上。

但当前 `setAsPtrAdd()` 的行为是：

1. 回写 `base <: record{Off: result}`
2. 回写 `result <: record{}`
3. `PG.unifyVar(basePtr, result)`

也就是说，`PNIMap` 的等价类里会同时放进：

- root 坐标上的值
- 偏移后的 view 值

在 `fortune.o3.wasm.4.ll` 的当前 case 里：

- `main::arg1`
- `main::%bb.brif_next47.i1`
- `main::%bb.loop_entry51.i2`
- `main::%bb.blk_exit76.i1`

最终都可能落到同一个 `PNI` 等价类。

但这几个值的“坐标角色”并不一样：

- `arg1` 是 root
- `%59` 是 `@0+4i`
- `%69` 还是 `@0+4i`
- `%103` 是 `@4+4i`

因此：

- `PNI leader` 适合表达“它们都是 pointer-like / alias-related”
- 但不适合表达“它们处于 root 坐标系中的哪个 offset”

所以方案 A 必须维护一份独立于 `PNIMap` 的 `PtrView` 状态。

一句话说：

- `PNIMap` 解决的是“是不是同一类 pointer-like value”
- `PtrView` 解决的是“在同一 root 坐标系里，它位于哪个 canonical offset”

这是两套不同的语义，不应混用。

## 三、方案 A 的核心抽象

### 1. ptradd 不创造新对象，只创造同一对象上的 view

只要当前关系来自：

- `ptr + int -> ptr`

而不是：

- load 得到新 pointer
- call 返回新 pointer
- pointer field 解引用得到新对象

那么这条关系的本质都应该理解为：

- “在同一个对象坐标系里，产生了一个新的 pointer view”

因此，ptradd 链应该尽量被 flatten 成：

- 一个 root
- 一个 cumulative offset

而不是层层嵌套成：

- `root -> @a -> node1 -> @b -> node2 -> ...`

### 2. 建议把每个值的视图状态表示成

```text
View(value) = (root, total_offset)
```

其中：

- `root`
  - 这条 ptradd 链的第一个非-ptradd anchor
  - 可以理解为“当前坐标系的原点”
- `total_offset`
  - 当前值相对于 `root` 的累计 `OffsetRange`

### 3. 递推规则

设：

- `Base = ptr-view(root, base_total)`
- `Result = ptradd(Base, step)`

那么：

```text
Result.root        = Base.root
Result.total       = normalize(Base.total + step)
```

这里的 `+` 可以直接复用现有：

- `OffsetRange::operator+()`

这在当前实现里有一个非常重要、而且对方案 A 有利的性质：

- 对 same-axis re-entry 是幂等的

例如：

- `@0+4i + @0+4i = @0+4i`
- `@0+4i + @4+4i = @4+4i`

这正是我们想要的：

- 相同的同轴重写会收敛到同一个 key
- sibling step 会收敛到新的同轴 key

## 四、推荐算法：两阶段 ptr-view canonicalization

### 阶段 1：在 `PG.solve()` 期间只收集事实，不直接物化 record

建议把当前：

- `Parent.setAsPtrAdd(base, result, off)`

改成语义上更接近：

- `Parent.recordPtrAddFact(base, result, off, inst)`

也就是说，在 `PNIGraph::eraseConstraint()` 中：

1. 仍然识别 `ptr + num -> ptr`
2. 仍然用 `matchOffsetRangeNoNegativeAccess()` 提取 `OffsetRange`
3. 但先不要立即往 mlsub 图里写 `record{...}`
4. 只把这条事实记入一张 `PtrViewFact` 表

推荐的数据结构：

```cpp
struct PtrViewState {
  ExtValuePtr Root;
  OffsetRange Total;
  ExtValuePtr ParentValue;
  OffsetRange Step;
  const llvm::Instruction *OriginInst;
  unsigned Depth;
};

struct PtrViewFact {
  ExtValuePtr Base;
  ExtValuePtr Result;
  ExtValuePtr Root;
  OffsetRange Step;
  OffsetRange Total;
  const llvm::Instruction *OriginInst;
};

struct PtrViewKey {
  ExtValuePtr Root;
  OffsetRange Total;

  bool operator<(const PtrViewKey &RHS) const {
    return std::tie(Root, Total) < std::tie(RHS.Root, RHS.Total);
  }
};
```

`ConstraintsGenerator` 侧新增建议成员：

```cpp
std::map<ExtValuePtr, PtrViewState> PtrViews;
std::vector<PtrViewFact> PtrViewFacts;
std::map<PtrViewKey, std::vector<ExtValuePtr>> PtrViewBuckets;
```

### 阶段 1 的 transfer 规则

伪代码：

```cpp
PtrViewState getImplicitRootView(ExtValuePtr V) {
  return {
    .Root = V,
    .Total = OffsetRange{},
    .ParentValue = V,
    .Step = OffsetRange{},
    .OriginInst = nullptr,
    .Depth = 0,
  };
}

PtrViewState getViewState(ExtValuePtr V) {
  if (PtrViews.count(V)) {
    return PtrViews.at(V);
  }
  return getImplicitRootView(V);
}

void recordPtrAddFact(ExtValuePtr Base, ExtValuePtr Result, OffsetRange Step,
                      const llvm::Instruction *Inst) {
  auto BaseView = getViewState(Base);
  auto Total = BaseView.Total + Step;
  PtrViewState ResultView{
      .Root = BaseView.Root,
      .Total = Total,
      .ParentValue = Base,
      .Step = Step,
      .OriginInst = Inst,
      .Depth = BaseView.Depth + 1,
  };
  PtrViews.insert_or_assign(Result, ResultView);
  PtrViewFacts.push_back({
      .Base = Base,
      .Result = Result,
      .Root = ResultView.Root,
      .Step = Step,
      .Total = Total,
      .OriginInst = Inst,
  });
  PtrViewBuckets[{ResultView.Root, Total}].push_back(Result);
}
```

注意：

- 这里 `Base` 不需要是 root
- 只要它已经有 `PtrViewState`，就沿着当前 view 链往上继承
- 如果它没有 `PtrViewState`，就把它当作这条 view 链的新 root

### 阶段 2：`PG.solve()` 结束后统一规范化并物化

推荐新增：

- `ConstraintsGenerator::flushPtrViewFacts()`

调用点建议放在：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:113`
  的 `PG.solve();` 之后

也就是 `ConstraintsGenerator::run()` 里：

```cpp
PG.solve();
flushPtrViewFacts();
```

这样做有三个明显好处：

1. 不会把新的 mlsub subtype 边回灌进当前 `PNDiff` 求解过程
2. 可以在 flush 时看到同一个 root 的全部 ptradd 事实，再做统一去重
3. 不依赖 solve 顺序，日志和结果都更稳定

## 五、flush 阶段的核心算法

### 1. 先按 `(root, total_offset)` 分桶

同一个 bucket 表示：

- “同一个 root 坐标系里的同一个 canonical view”

例如当前 case 里，理想分桶后会得到：

- key = `(arg1, @0+4i)` -> values = `{%59, %69}`
- key = `(arg1, @4+4i)` -> values = `{%103}`

而不是：

- `%59` 下再嵌 `%69`

### 2. 每个 bucket 只选一个代表值

推荐选代表值的规则：

1. 如果 `total_offset` 为零，优先选择 `root` 本身
2. 否则优先选择 bucket 中第一个出现的值
3. 若需要更强确定性，再用 `toStableString()` 做 tie-break

推荐的代表值选择不要依赖：

- `PNI leader`
- `SimpleType` 指针地址

原因：

- `PNI leader` 已混入不同坐标
- `SimpleType` 指针地址不够稳定，也不表达 source-level 含义

### 3. 对同 bucket 的其他值做 alias/等价约束

这里建议分两种情况：

#### 情况 A：其它值还没有 `SimpleType` 节点

如果某个 `Result` 还没被 `V2N` 物化，那么最干净的做法是：

- `addRemapType(Result, Representative)`

这样能直接复用代表值的 `SimpleType` 节点。

#### 情况 B：其它值已经有自己的 `SimpleType` 节点

这时不要只做 `V2N.merge()`，因为：

- `addMergeNode()` 目前只是映射 merge
- 不是真正的类型变量等价

因此更稳妥的做法是显式加双向 subtype：

```cpp
addSubtype(ResultNode, RepNode);
addSubtype(RepNode, ResultNode);
```

这样语义上表达的是：

- “这两个值是同一个 canonical view”

### 4. 对每个 unique key 只物化一次 record field

伪代码：

```cpp
void flushPtrViewFacts() {
  std::set<std::tuple<ExtValuePtr, std::string, ExtValuePtr>> Emitted;

  for (auto &[Key, Values] : PtrViewBuckets) {
    auto Root = Key.Root;
    auto Off = Key.Total;
    auto Rep = chooseRepresentative(Root, Off, Values);

    auto RootNode = getOrInsertNode(Root);
    auto RepNode = getOrInsertNode(Rep);

    for (auto V : Values) {
      if (V == Rep) {
        continue;
      }
      if (getNodeOrNull(V) == nullptr) {
        addRemapType(V, Rep);
      } else {
        auto VNode = getOrInsertNode(V);
        addSubtype(VNode, RepNode);
        addSubtype(RepNode, VNode);
      }
    }

    auto EdgeKey = std::tuple{Root, Off.str(), Rep};
    if (!Emitted.insert(EdgeKey).second) {
      continue;
    }

    addSubtype(RootNode,
               binarysub::make_record({{Off.str(), RepNode}}));
    addSubtype(RepNode, binarysub::make_record({}));
  }
}
```

最终效果是：

- root 上只出现扁平化后的 canonical field
- 同一个 `(root, total_offset)` 不会重复长出新的 nested layer

## 六、为什么“两阶段”比“即时改写 setAsPtrAdd”更好

### 1. 不受 solve 顺序影响

如果在 `eraseConstraint()` 里每识别一条 ptradd 就立刻写回：

- 你看到的是“当前这一步的局部视图”

如果 later 又来一条同 root、同 offset 的 ptradd，想 dedup 时就已经晚了，
因为前面的边已经下沉到 mlsub 图里了。

而两阶段算法的好处是：

- 先把所有 ptradd fact 收齐
- 再一次性做 canonical grouping

这是天然 order-independent 的。

### 2. 不把 mlsub subtype 写回和 PNDiff solve 纠缠在一起

当前 `PG.solve()` 的职责本来是：

- 解决 `ptr / num / unknown`

如果每条 solved ptradd 还顺带即时改写 mlsub 约束，就容易让：

- `PNDiff` 的求解时序
- `SimpleType` 图的扩张

耦合得更深。

延迟 flush 的好处是把两者分开：

1. 先解完 PNDiff
2. 再统一物化 ptr-view 结构

### 3. 更容易做调试和 trace

如果做两阶段，可以非常自然地增加两类 trace：

```text
[ptrview:fact] base=... result=... step=... root=... total=...
[ptrview:bucket] root=... total=... values=... rep=...
```

这样后面再看 `TypeBuilder` 的 field 形状时，可以直接对应回：

- 哪些值被折叠成了同一个 canonical view

## 七、方案 A 推荐依赖的一条重要不变量

### 不变量：ptr-view root 只通过“显式 alias-preserving 操作”继承

第一版不要试图把所有 alias 情况都建模进去。  
建议只让下列操作继承已有 `PtrViewState`：

1. `bitcast / ptrtoint / inttoptr`
   - 当前已在 `visitCastInst()` 里 remap
2. `zero-index GEP`
   - 当前已在 `visitGetElementPtrInst()` 里 remap
3. 显式 `addRemapType(Val, Target)` 这种 exact alias

这意味着：

- 如果一个值没有 `PtrViewState`
- 且它也不是显式 alias-preserving remap 得来的
- 那它就是新 root

这是一个很好的 conservative 边界。

### 为什么这个边界合理

因为真正会跨对象边界的地方，往往正是：

- load
- call result
- real pointer field dereference
- phi/select 混合不同来源

这些位置如果第一版都当作新 root，虽然可能少 flatten 一些情况，
但不会把不同对象坐标系错误地揉到一起。

## 八、第一版不要做的事情

### 1. 不要用 `PNI leader` 当 root

原因上面已经说过：

- 当前 `PNI` 等价类混合了不同坐标位置

### 2. 不要在第一版就做“通过 alias 分析跨 block/phi 追 root”

第一版建议只处理：

- ptradd 链
- 显式 alias remap

如果一开始就试图把：

- PHI
- select
- load/store alias
- interproc alias

都并进 ptr-view root 传播里，很容易把问题从“canonicalization”
变成“另一个 alias analysis”。

### 3. 不要在第一版就改 binarysub 的核心类型表示

当前问题其实可以先在 NotDec 侧解决：

- `PNDiff` 负责收集 ptr-view fact
- `MLsubGenerator` 负责 canonicalization 和下沉

没有必要第一步就去改：

- `binarysub::SimpleType`
- `binarysub::URecordType`

的内部结构。

## 九、当前推荐的实现步骤

### 第一步：把 `setAsPtrAdd()` 拆成“收集”和“flush”

目标函数：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`
- `src/TypeRecovery/mlsub/PNDiff.cpp`

建议：

1. 把 `setAsPtrAdd()` 改成只记录 `PtrViewFact`
2. 在 `ConstraintsGenerator::run()` 中 `PG.solve()` 之后调用
   `flushPtrViewFacts()`

### 第二步：加最小 view 状态传播

目标函数：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1688-1708`
  - `addRemapType`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2888-2904`
  - `visitCastInst`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3084-3093`
  - `visitGetElementPtrInst`

建议：

1. `addRemapType(Val, Target)` 时，如果 `Target` 有 `PtrViewState`
   - 复制给 `Val`
2. 如果 `Target` 没有 `PtrViewState`
   - 给 `Target` 先 materialize 一个零偏移 implicit root view
   - 再复制给 `Val`

这样 cast / zero-GEP 不会平白切断 root 链。

### 第三步：只做 root+offset flatten，不做 window/layout heuristic

第一版方案 A 的职责应保持单纯：

- 只解决“ptradd 链不该长成 nested record”

先不要在这里引入：

- element window
- field containment
- layout boundary

这些仍然留给后续：

- `TypeBuilder` 或中间 record normalization

## 十、为什么这套算法比当前形状更接近真实语义

当前的物化方式，等价于把：

```text
arg1 --@0+4i--> %59
%59  --@0+4i--> %69
%59  --@4+4i--> %103
```

解释成：

```text
arg1 has a field @0+4i,
and that field contains an inner record with @0+4i and @4+4i
```

而推荐算法会把它解释成：

```text
root = arg1
%59  -> (arg1, @0+4i)
%69  -> (arg1, @0+4i)
%103 -> (arg1, @4+4i)
```

最后只物化成：

```text
arg1 has canonical views at:
  @0+4i
  @4+4i
```

这更符合 ptradd 的本意：

- 坐标变换
- 不是对象嵌套

## 十一、复杂度与风险

### 复杂度

设某个 SCC 中被成功 reify 的 ptradd 事实数为 `P`。

第一版两阶段算法的开销大致是：

- 收集阶段：`O(P log P)` 或 `O(P)`，取决于容器
- flush 分桶：`O(P log P)`
- 代表值对齐与约束写回：`O(P)`

对当前规模的 `NotDec` 类型恢复来说，这个复杂度是完全可以接受的。

### 风险

#### 风险 1：alias-preserving 传播做得不够全，导致 root 碎片化

后果：

- 可能少 flatten 一些情况
- 但不会重新引入 nested same-axis record

这属于“质量保守”，不是“语义错误”。

#### 风险 2：同 bucket 的值已经提前 materialize 为不同 `SimpleType`

后果：

- 需要用双向 subtype 显式拉齐
- 不能只靠 `V2N.merge()`

这是实现细节风险，但可控。

#### 风险 3：`OffsetRange::operator+()` 仍然是 over-approx

例如：

- 多个不同 induction variable 的组合
- 多轴访问的合并

当前 `OffsetRange` 本来就不保留 coefficient / variable identity，
所以方案 A 并不会让这一点更差。  
它只是把原本已经存在的 over-approx，改成“扁平 over-approx”，而不是
“嵌套 over-approx”。

## 十二、当前一句话版本

如果方案 A 要做得更好，当前最推荐的算法不是：

- 每发现一次 ptradd，就立刻往当前 `base` 节点下面挂一层 record

而是：

- 把 ptradd 先建模成独立的 `PtrView(root, total_offset)` 事实
- 在 `PG.solve()` 结束后，按 `(root, total_offset)` 做统一 canonicalization
- 每个 canonical view 只物化一次 record field

这样可以在不改 binarysub 核心表示的前提下，从源头消掉：

- same-axis re-entry 被误解释成 nested record

这一类问题。

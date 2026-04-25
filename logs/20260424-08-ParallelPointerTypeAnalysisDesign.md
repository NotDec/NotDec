# Parallel Pointer and Type Analysis Interface Design

## 背景

当前 BinarySub 的指针 load/store 规则已经在类型图里隐含了一部分指针信息。
它不是直接得到 `pts(p) = {O}`，而是得到类似下面的值流闭包：

```text
store(v, p)
load(q) -> r

如果 p.store 能到 q.load
=> type(v) <: type(r)
```

这和 Andersen-style 指针分析很像。区别是：

- 指针分析保留对象身份：`p` 可能指向 `O.field4`。
- 当前类型推理保留值流结果：写入的值可能流到读取结果。
- 当前类型推理没有显式对象节点，所以难以回答“通过哪个对象连起来”。

因此，后续可以让 field-sensitive 指针分析和类型推理并行运行。指针分析负责对象身份，类型推理负责对象内容类型。

## 基本判断

如果两边采用同一套对象模型、字段模型和流非敏感语义，那么显式指针分析可以导出当前隐式指针规则需要的大部分类型约束。

旧规则可以看作：

```text
p.store -> q.load
```

新规则改成：

```text
type(v) <: ObjTy(O.field)
ObjTy(O.field) <: type(r)
```

当 `p` 和 `q` 都可能指向 `O.field` 时，自然得到：

```text
type(v) <: type(r)
```

新规则比旧规则多保留了 `O.field` 这个中间节点。它不会把“两个对象类型相同”误判为“两个指针指向同一个对象”。

## 设计原则

1. 指针分析和类型推理不要互相读内部图。
2. 交互只走事件、增量结果和稳定 ID。
3. 对象身份和对象内容类型分开。
4. load/store 的内存值流由 `ConstraintsGenerator` 根据 points-to 结果生成。
5. 先支持对比新旧两种方法是否一致，再考虑替换旧路径。
6. 方案直接复用当前 `ConstraintsGenerator` / `PNIGraph` / `ExtValuePtr` / `SimpleType`。

## 代码落点

这份设计直接使用项目里已有的几类对象：

- `ExtValuePtr`：表示 IR 里的值，也作为 pointer value slot。
- `SimpleType`：表示 BinarySub 类型节点，也表示对象内容类型节点。
- `ConstraintsGenerator`：仍然是类型恢复主入口，负责访问 IR、创建类型节点、添加类型约束。
- `PNIGraph`：仍然负责 pointer / number 区分，以及 ptradd 的 offset 识别。
- `PointerAnalysis`：新增的独立分析类，负责 points-to，不放进 BinarySub 类型图里。
- `MemoryLocKey`：新增的小结构，用来表示显式对象字段身份。
- `PointerSlotKey`：PointerAnalysis 内部 key，用来同时表示 SSA pointer 和内存里的 pointer cell。
- `MemoryAccessRecords`：放在 `ConstraintsGenerator` 里的小数据结构，记录 load/store 事件和对象内容类型。

`ConstraintsGenerator` 可以持有三个平级分析状态：

```cpp
struct ConstraintsGenerator {
  PNIGraph PG;
  PointerAnalysis PA;
  DSUMap<ExtValuePtr, SimpleType> V2N;
  MemoryAccessRecords MemoryAccesses;
};
```

这里 `V2N` 和 `binarysub::constrain()` 代表 BinarySub 类型推理状态。`ConstraintsGenerator` 不应该变成第四个分析，它只是 IR visitor 和调度器。

对象身份和对象内容类型分开：

```text
Pointer slot:
  SSA pointer value 用 ExtValuePtr
  memory pointer cell 用 MemoryLocKey

Object root:
  ExtValuePtr 里的 StackObject / HeapObject / ConstantAddr / GlobalValue

Object field:
  用 {root ExtValuePtr, vector<OffsetRange>, size} 组成一个 MemoryLocKey

Object content type:
  用 SimpleType
  通过 ConstraintsGenerator::addSubtype() 连接
```

`MemoryLocKey` 是指针分析内部的小结构，不需要一开始放进 `ExtValuePtr`：

```cpp
struct MemoryLocKey {
  ExtValuePtr Root;
  std::vector<OffsetRange> Path;
  unsigned BitSize = 0;
};
```

`Path` 表示从 root 开始经过的字段路径。需要用 vector，而不是单个 `OffsetRange`。原因是一个指针可能经过多次 ptradd：

```text
p1 = p0 + 4
p2 = p1 + 8
```

第一版先把它表示成：

```text
MemoryLocKey{Root = p0-root, Path = [4, 8]}
```

不把 `[4, 8]` 合并成 `[12]`。这样和当前文档里的简化假设一致：不处理复杂指针运算等价、访问范围重叠，只把不同路径当成不同字段。后续恢复结构体时再考虑路径归一化。

PointerAnalysis 内部需要一个能同时表示 SSA pointer 和 memory pointer cell 的 key：

```cpp
using PointerSlotKey = std::variant<ExtValuePtr, MemoryLocKey>;
```

不建议把所有 `ExtValuePtr` 都表示成 `Path=[]` 的 `MemoryLocKey`。`ExtValuePtr` 表示的是 SSA 值槽，`MemoryLocKey` 表示的是内存对象槽。两者可以都“存放指针值”，但身份不同。

例如 alloca 指令本身有两个身份：

```text
ExtValuePtr{&alloca}
  表示 alloca 指令返回的 SSA 指针值。

MemoryLocKey{Root = StackObject{&alloca}, Path = []}
  表示 alloca 分配出来的内存对象。
```

如果直接把 `ExtValuePtr{&alloca}` 当成空路径 `MemoryLocKey`，就会把“指针值”与“被指向的对象”混在一起。除非额外给 `MemoryLocKey` 的 root 加 kind 区分 value root 和 object root，否则本质上还是需要一个类似 `PointerSlotKey` 的区分。

对外 visitor 入口仍然尽量收 `ExtValuePtr`，只有 `dst = *addr` / `*addr = src` 这类间接规则内部才需要把 `MemoryLocKey` 转成 pointer slot。

## 当前代码依据

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:54` 已经定义 `StackObject`，`external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:65` 已经定义 `HeapObject`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:58` 已经在 `ConstraintsGenerator` 里持有 `PNIGraph PG`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:63` 已经用 `V2N` 管 `ExtValuePtr -> SimpleType`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:154` 的 `addSubtype()` 已经是类型约束入口，内部调用 `binarysub::constrain()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:202` 的 `setAsPtrAdd()` 已经是 PNDiff 反哺 field 信息的入口。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2077` 的 `createNode()` 同时创建 BinarySub 节点和 PNDiff 节点。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2098` 的 `addRemapType()` 同时 remap `V2N` 和 `PG`，适合顺手加 PA copy。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3540` / `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3563` 是 load/store 类型约束入口。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3586` 和 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3458` 的 malloc 分支是对象创建入口。
- `src/TypeRecovery/mlsub/PNDiff.cpp:297` 到 `src/TypeRecovery/mlsub/PNDiff.cpp:323` 已经在 ptradd 解出后调用 `Parent.setAsPtrAdd()`。

## 组件职责

### PointerAnalysis

`PointerAnalysis` 应该是单独类，和 `PNIGraph`、BinarySub 类型推理平级。它只负责 points-to。

它维护：

```text
pts(PointerSlotKey slot) = set<MemoryLocKey>
```

它不关心 `int`、`struct`、函数类型，也不读 BinarySub 的上下界。

基础接口：

```cpp
class PointerAnalysis {
public:
  // SSA value 用 ExtValuePtr，内存里的 pointer cell 用 MemoryLocKey。
  using Slot = PointerSlotKey;

  Slot valueSlot(ExtValuePtr value);
  Slot memorySlot(MemoryLocKey loc);

  MemoryLocKey getRootObject(ExtValuePtr root, unsigned bitSize);
  MemoryLocKey getFieldObject(MemoryLocKey base, OffsetRange field,
                              unsigned bitSize);

  void addAddrOf(ExtValuePtr dst, MemoryLocKey obj);
  void addCopy(ExtValuePtr dst, ExtValuePtr src);
  void addField(ExtValuePtr dst, ExtValuePtr base, OffsetRange field,
                unsigned bitSize);

  void addLoadPtr(ExtValuePtr dst, ExtValuePtr addr);
  void addStorePtr(ExtValuePtr addr, ExtValuePtr src);

  void addPointsToDeltaCallback(PointsToDeltaCallback cb);

  bool solve();
};
```

接口语义：

```text
dst = &obj
=> pts(valueSlot(dst)) includes obj

dst = src
=> pts(valueSlot(dst)) includes pts(valueSlot(src))

dst = base + field
=> for O in pts(valueSlot(base)):
     pts(valueSlot(dst)) includes appendPath(O, field)

dst = *addr
=> for O in pts(valueSlot(addr)):
     pts(valueSlot(dst)) includes pts(memorySlot(O))

*addr = src
=> for O in pts(valueSlot(addr)):
     pts(memorySlot(O)) includes pts(valueSlot(src))
```

当前简化假设下，就还是按照当前的offset range。暂时不处理重叠访问等复杂情况，不同 offset range 就是不同 field。

### ConstraintsGenerator

类型侧继续走当前入口：

```cpp
SimpleType ConstraintsGenerator::getOrInsertNode(ExtValuePtr Val);
void ConstraintsGenerator::addSubtype(SimpleType lhs, SimpleType rhs);
SimpleType ConstraintsGenerator::addRemapType(ExtValuePtr Val,
                                              ExtValuePtr Target);
void ConstraintsGenerator::setAsPtrAdd(ExtValuePtr basePtr,
                                       ExtValuePtr result,
                                       OffsetRange Off);
```

`ConstraintsGenerator` 里补一个对象内容类型表和 load/store 记录表即可，不需要强制单独拆出新的 bridge 类：

```cpp
struct MemoryAccessRecords {
  std::map<MemoryLocKey, SimpleType> LocTypes;
  std::map<ExtValuePtr, std::vector<RecordedLoad>> LoadsByAddr;
  std::map<ExtValuePtr, std::vector<RecordedStore>> StoresByAddr;

  SimpleType getOrCreateLocType(ConstraintsGenerator &CG,
                                const MemoryLocKey &Loc) {
    auto It = LocTypes.find(Loc);
    if (It != LocTypes.end()) {
      return It->second;
    }
    auto Ty = binarysub::fresh_variable(CG.lvl, Loc.BitSize);
    LocTypes.insert({Loc, Ty});
    return Ty;
  }
};
```

这样对象内容类型仍然是 `SimpleType`，所有新约束仍然走：

```cpp
CG.addSubtype(lhs, rhs);
```

`getOrCreateLocType()` 返回对象内容类型变量。这个类型变量后续可以被用户 override：

```text
ObjTy(O.field4, 32)
```

如果同一个对象字段路径被不同 size 访问，初期可以按 `(root, path, bitSize)` 分开建类型变量。后续再考虑路径归一化、重叠和裁剪。

### 内存事件协调

不强制新增类。先让 `ConstraintsGenerator` 持有 `MemoryAccessRecords`，并提供几个小方法：

它记录内存读写点：

```cpp
class ConstraintsGenerator {
  MemoryAccessRecords MemoryAccesses;

public:
  void recordLoad(ExtValuePtr addr, SimpleType resultTy, unsigned bitSize,
                  llvm::Instruction *source);

  void recordStore(ExtValuePtr addr, SimpleType valueTy, unsigned bitSize,
                   llvm::Instruction *source);

  void onPointsToDelta(ExtValuePtr addr, MemoryLocKey loc);

  void flushPointerDerivedTypeConstraints();
};
```

当指针分析产生增量：

```text
addr -> loc
```

`ConstraintsGenerator` 补类型约束：

```text
load addr -> result:
  typeOfLoc(loc, size) <: resultTy

store value -> addr:
  valueTy <: typeOfLoc(loc, size)
```

对应代码形状：

```cpp
void ConstraintsGenerator::onPointsToDelta(ExtValuePtr addr,
                                           MemoryLocKey loc) {
  for (auto &L : MemoryAccesses.LoadsByAddr[addr]) {
    SimpleType objTy =
        MemoryAccesses.getOrCreateLocType(*this, withSize(loc, L.BitSize));
    addSubtype(objTy, L.ResultTy);
  }

  for (auto &S : MemoryAccesses.StoresByAddr[addr]) {
    SimpleType objTy =
        MemoryAccesses.getOrCreateLocType(*this, withSize(loc, S.BitSize));
    addSubtype(S.ValueTy, objTy);
  }
}
```

实际接收 `PointsToDelta` 时，只处理 `Slot` 是 `ExtValuePtr` 的 delta，因为 IR load/store 的地址操作数来自 SSA value。`Slot` 是 `MemoryLocKey` 的 delta 只给 PointerAnalysis 自己的间接规则使用。

这样旧的隐式路径：

```text
p.store -> q.load
```

被改成：

```text
type(v) -> ObjTy(O) -> type(r)
```

对象身份在 `MemoryLocKey`，对象内容类型在 `SimpleType`。

### replace 模式下的 BinarySub 退化

如果进入 replace 模式，BinarySub 里和“指针别名传播”有关的部分应该退化掉。类型推理仍然存在，但它不再自己通过 `PtrLoad` / `PtrStore` 猜测内存读写是否相遇。

需要停止的入口：

```text
visitLoadInst()
  停止添加：addSubtype(PtrVal, make_ptr_load(RetVal, BitSize))
  改为记录：recordLoad(addr, RetVal, BitSize, &I)

visitStoreInst()
  停止添加：addSubtype(PtrVal, make_ptr_store(StoreVal, BitSize))
  改为记录：recordStore(addr, StoreVal, BitSize, &I)
```

也就是说，IR load/store 不再把 `directLoad` / `directStore` 放进指针值的类型上界里。内存值流只通过 PointerAnalysis 的 `pts(addr)` 加：

```text
store v, p:
  type(v) <: ObjTy(O)

load p -> r:
  ObjTy(O) <: type(r)
```

当 `p` 和 `q` 可能指向同一个 `O` 时，才得到：

```text
type(v) <: ObjTy(O) <: type(r)
```

这样 BinarySub 不再需要这些“把 store 和 load 在类型图里接起来”的规则：

```text
constrain(PtrStore(v1), PtrLoad(v2)) -> v1 <: v2

变量 upperBounds 里新增 PtrStore 时，扫描已有 PtrLoad 并添加 store.to <: load.to

变量 upperBounds 里新增 PtrLoad 时，扫描已有 PtrStore 并添加 store.to <: load.to

TMemObject directStore / directLoad 之间的内源性传播：
  lr.directStore.to <: rr.directLoad.to
```

这些规则在 original 模式里仍然要保留，因为旧算法依赖它们。replace 模式不建议一开始删除实现，而是加模式开关，让 IR visitor 不再产生新的 `PtrLoad` / `PtrStore` 约束。这样旧测试、override、调试输出还可以继续使用原规则。

仍然要保留的类型推理能力：

```text
普通 SSA 值流：
  addSubtype(src, dst)

函数类型：
  参数逆变、返回值协变

record / field 约束：
  BaseNode <: { Off : ResultNode }

primitive / set type / level / polymorphism：
  继续由 BinarySub 处理

对象内容类型：
  ObjTy(MemoryLocKey) 仍然是 SimpleType
```

`setAsPtrAdd()` 里的 record 约束是否保留，可以分阶段处理：

```text
第一阶段保留：
  BaseNode <: { Off : ResultNode }
  同时通知 PA.addField(result, base, Off)

后续如果 PA 的 field 结果足够稳定：
  可以考虑弱化 BinarySub 里的指针字段形状约束
```

这里要小心：record 约束现在不只是 alias 传播，也影响最后结构体形状的打印和 TypeBuilder，所以不能和 PtrLoad/PtrStore 一起粗暴删除。

值得单独强调的是，replace 模式会让责任边界更清楚：

```text
PointerAnalysis:
  决定哪些地址可能是同一个对象字段。

BinarySub:
  决定对象字段里存的值是什么类型。
```

这也意味着调试时可以分别看两个问题：

```text
类型错了，但 pts 对：
  优先查 BinarySub 类型约束。

load/store 没连上：
  优先查 PointerAnalysis 的对象抽象、copy、field、load/store-ptr 规则。
```

### PNDiff

PNDiff 继续负责指针和数字区分，以及简单指针偏移识别。

第一版不需要新增 callback。当前已经有现成路径：

```text
PNIGraph::eraseConstraint()
  -> ConstraintsGenerator::setAsPtrAdd(basePtr, result, Off)
```

所以只需要扩展 `setAsPtrAdd()`：

```cpp
void ConstraintsGenerator::setAsPtrAdd(ExtValuePtr basePtr,
                                       ExtValuePtr result,
                                       OffsetRange Off) {
  // 保留现有类型侧 record 约束。
  ...

  // 新增：通知 PointerAnalysis。
  PA.addField(result, basePtr, Off, getSize(result));
}
```

如果 PNDiff 判断某个常量是地址，可以创建全局对象字段：

```text
const 0x1234 is pointer
=> MemoryLocKey global.0x1234
=> PointerAnalysis.addAddrOf(slot(const), global.0x1234)
```

## IR 事件到接口的映射

### alloca / malloc / global

```text
p = alloca
```

当前落点：

```text
visitAllocaInst()
  现有：createNode(&I), setPointer(&I), ContraVariantValues.insert(&I)
  新增：PA.addAddrOf(&I, MemoryLocKey{StackObject{&I}, Path=[]})
```

malloc 走现有 `visitCallBase()` 里的 `isHeapAllocationCall()` 分支：

```text
visitCallBase(malloc)
  现有：createNode(&I), setPointer(&I), ContraVariantValues.insert(&I)
  新增：PA.addAddrOf(&I, MemoryLocKey{HeapObject{&I}, Path=[]})
```

全局变量可以先按 `GlobalValue*` 当 root。`ConstantAddr` 已经有特殊路径，后续可以映射到 `MemoryType` 下的字段。

### 指针 copy

```text
q = p
```

当前代码里有两类 copy：

```text
addRemapType(q, p)
  例子：bitcast、ptrtoint、inttoptr、zero-index gep

addSubtype(p, q)
  例子：phi、select、函数实参与返回等普通数据流
```

第一版指针分析可以只从明确 copy 开始：

```cpp
ConstraintsGenerator::addRemapType(Val, Target)
  -> PA.addCopy(Val, Target)

MLsubVisitor::handlePHINodes()
  -> PA.addCopy(phi, incoming)

MLsubVisitor::visitSelectInst()
  -> PA.addCopy(select, trueValue)
  -> PA.addCopy(select, falseValue)
```

后续如果要更接近当前类型闭包，可以在 `binarysub::ConstrainPairObserver` 里观察变量到变量的约束，再把映射回 `ExtValuePtr` 的 pair 转成 PA copy。第一版不建议这么做，因为容易把非指针普通值流也放进 points-to。

### gep / ptradd

```text
q = p + field4
```

当前已经通过 PNDiff 反哺：

```text
PNIGraph::eraseConstraint()
  -> ConstraintsGenerator::setAsPtrAdd(basePtr, result, Off)
```

所以只扩展 `setAsPtrAdd()`：

```text
现有：BaseNode <: { Off : ResultNode }
新增：PA.addField(result, basePtr, Off, getSize(result))
```

### 非指针 load

```text
r = load p
```

处理：

```text
visitLoadInst()
  PtrVal = getOrInsertNode(pointer operand)
  RetVal = getOrInsertNode(&I)
  BitSize = getPointerElemSize(pointer type)

  original/shadow:
    保留 addSubtype(PtrVal, make_ptr_load(RetVal, BitSize))

  shadow/replace:
    recordLoad(pointer operand ExtValuePtr, RetVal, BitSize, &I)
```

`ConstraintsGenerator` 等待 `pts(p)` 增量，再补：

```text
ObjTy(O, size) <: type(r)
```

### 非指针 store

```text
store v, p
```

处理：

```text
visitStoreInst()
  PtrVal = getOrInsertNode(pointer operand)
  StoreVal = getOrInsertNode(value operand)
  BitSize = getPointerElemSize(pointer type)

  original/shadow:
    保留 addSubtype(PtrVal, make_ptr_store(StoreVal, BitSize))

  shadow/replace:
    recordStore(pointer operand ExtValuePtr, StoreVal, BitSize, &I)
```

`ConstraintsGenerator` 等待 `pts(p)` 增量，再补：

```text
type(v) <: ObjTy(O, size)
```

### 指针 load

```text
r = load p
```

如果 `r` 是指针值，需要同时通知 PointerAnalysis，并记录普通类型 load：

```text
Pointer.addLoadPtr(result ExtValuePtr, addr ExtValuePtr)
recordLoad(addr ExtValuePtr, RetVal, size, &I)
```

第一条传播指针对象身份，第二条传播普通类型。

### 指针 store

```text
store v, p
```

如果 `v` 是指针值，需要同时通知 PointerAnalysis，并记录普通类型 store：

```text
Pointer.addStorePtr(addr ExtValuePtr, value ExtValuePtr)
recordStore(addr ExtValuePtr, StoreVal, size, &I)
```

第一条传播 points-to，第二条传播对象内容类型。

## Worklist 调度

当前 BinarySub 不是外层统一 worklist。`ConstraintsGenerator::addSubtype()` 每次都会同步调用 `binarysub::constrain()`，PNDiff 则在 `ConstraintsGenerator::run()` 末尾调用 `PG.solve()`。

因此第一版调度不要改成另一个异步 type worklist。更贴近当前代码的流程是：

```cpp
ConstraintsGenerator::run() {
  // 1. 创建函数、参数、返回值节点。
  ...

  // 2. MLsubVisitor 扫 IR。
  //    这里继续调用 addSubtype()，同时记录 PA 和内存事件。
  ...

  // 3. 处理 PHI。
  //    这里继续调用 addSubtype()，同时记录 PA copy。
  ...

  // 4. PNDiff 解 ptr/num 和 ptradd。
  //    eraseConstraint() 会调用 setAsPtrAdd()。
  PG.solve();

  // 5. PA 解 points-to。
  PA.solve();

  // 6. 根据 points-to delta 补类型约束。
  //    shadow 模式只记录对比结果。
  //    replace 模式调用 addSubtype()。
  flushPointerDerivedTypeConstraints();
}
```

BinarySub 不需要反向影响 PointerAnalysis。某个值是不是指针，应该来自 PNDiff 或 LLVM 低层类型，而不是从 BinarySub 的最终类型反推。这样可以避免把普通值类型关系污染到 points-to 里。

## 增量回调

`PointsToDeltaCallback` 建议只传新增边：

```cpp
struct PointsToDelta {
  PointerSlotKey Slot;
  MemoryLocKey Loc;
  ConstraintReason Reason;
};
```

要求：

- PointerAnalysis 内部去重。
- `ConstraintsGenerator` 只处理新增 delta。
- `MemoryAccessRecords` 自己也要去重，避免重复添加同一条类型约束。
- 每条导出的类型约束带 reason，方便 workdir dump 和日志解释。

reason 可以长这样：

```text
load %r = load %p
because pts(%p) contains stack_obj.field4
```

或者：

```text
store %v -> %p
because pts(%p) contains malloc@foo:12.field0
```

## 运行模式

### original 模式

这是当前默认路径，不启用新 PointerAnalysis。

```text
visitLoadInst()
  addSubtype(PtrVal, make_ptr_load(RetVal, BitSize))

visitStoreInst()
  addSubtype(PtrVal, make_ptr_store(StoreVal, BitSize))
```

这个模式保留旧行为，用作回归基线。

### shadow 模式

PointerAnalysis 跑起来，但不改变类型推理结果。

当前代码里 shadow 模式可以使用两条观测路径：

```text
1. PA 自己输出 pts(slot)。
2. 扩展 ConstrainPairObserver，观察旧算法产生的 directStore -> directLoad。
3. `MemoryAccessRecords` 根据 PA 结果计算新 store-load relation，但不调用 addSubtype()。
```

`ConstraintsGenerator::addSubtype()` 已经给 `binarysub::constrain()` 传了 observer。现在 observer 只调用 `maybeUnifyPNDiffTypeVariablePair()`。可以把它扩展成：

```cpp
binarysub::constrain(lhs, rhs, cache,
  [this](const SimpleType &Lhs, const SimpleType &Rhs) {
    maybeUnifyPNDiffTypeVariablePair(Lhs, Rhs);
    if (MemoryAccesses.isShadowEnabled()) {
      MemoryAccesses.observeOldTypeEdge(Lhs, Rhs);
    }
  });
```

`observeOldTypeEdge()` 只记录这种边：

```text
Lhs has directStore(size)
Rhs has directLoad(size)
=> old store/load may-flow
```

这个方式不用改 `binarysub::constrain_impl()`。

用途：

- 对比新 PA 导出的 store-load 关系和旧 PtrLoad/PtrStore 推出的关系。
- 找对象建模差异。
- 验证 field-sensitive 规则。

输出可以是：

```text
old: store S may reach load L
new: store S may reach load L through MemoryLocKey O.field
```

这是最安全的第一步。

### replace 模式

新 PA 接管内存别名传播。类型推理不再用 PtrLoad/PtrStore 推 alias。

当前代码里的 replace 开关主要落在两个函数：

```text
visitLoadInst()
  不再调用 addSubtype(PtrVal, make_ptr_load(...))
  改由 recordLoad()

visitStoreInst()
  不再调用 addSubtype(PtrVal, make_ptr_store(...))
  改由 recordStore()
```

`setAsPtrAdd()` 里的 record 约束可以先保留一段时间，因为它影响结构体字段形状。等 PA field 结果稳定后，再考虑只保留 PA 侧 field。

保留：

```text
普通 SSA 类型流
函数参数/返回类型
对象内容类型 ObjTy(Loc)
load/store 由 ConstraintsGenerator 根据 pts 生成的类型约束
```

删除或弱化：

```text
通过 PtrStore/PtrLoad 在类型图里隐式传播指针别名
```

这是目标模式。

## 和当前类型算法的相似点

当前算法：

```text
指针 copy 产生 p <: q
store/load 的逆变/协变规则产生 p.store -> q.load
```

Andersen-style PA：

```text
指针 copy 产生 pts(q) includes pts(p)
store/load 根据 pts(addr) 产生对象内容传播
```

二者相似的地方是：

- 都是 flow-insensitive。
- 都是 inclusion-based。
- 都通过 worklist 做闭包。
- 都会因为 copy 边传递大量关系。

主要差别是：

- 当前类型算法传播的是类型边。
- PA 传播的是对象集合。
- 当前类型算法的 store/load 闭包是 PA 结果的压扁投影。

可以把旧算法的隐式结果理解为：

```text
exists O. O in pts(p) and O in pts(q)
```

然后直接导出：

```text
store(v, p) -> load(q)
```

新算法保留 `O`，所以更容易解释和调试。

## 等价性条件

想让新接口导出的类型约束覆盖旧算法，需要满足：

1. 两边都是 flow-insensitive。
2. field-sensitive 规则一致。
3. alloca、global、malloc site 的对象抽象一致。
4. 函数参数和返回值的对象模型一致。
5. 外部函数 summary 一致。
6. 指针 load/store/copy 的语义一致。
7. 不让普通值流污染 points-to。

其中第 7 点最重要。

例如：

```c
p = &O1;
q = &O2;
x = *p;
*q = x;
```

这里应该得到：

```text
ObjTy(O1) <: type(x) <: ObjTy(O2)
```

但不能得到：

```text
q points-to O1
```

所以 `ConstraintsGenerator` 只能把 points-to 转成类型约束，不能把类型约束反推回 points-to。

## 建议的落地顺序

### 第一步：只建接口和 shadow 输出

实现最小 PA：

- `addr-of`：从 `visitAllocaInst()` 和 malloc 分支来。
- `copy`：先从 `addRemapType()`、PHI、select 来。
- `field`：从 `setAsPtrAdd()` 来。

先不接管类型推理。

输出：

```text
pts(value) = {loc...}
```

以及：

```text
store-load relation through Loc
```

同时用 `ConstrainPairObserver` 输出旧算法的 store-load relation。第一步的目标不是改类型结果，而是确认：

```text
旧：store S may reach load L
新：store S may reach load L through MemoryLocKey O
```

### 第二步：生成 shadow 对比结果

让 `MemoryAccessRecords` 根据 `pts(addr)` 计算新方法会生成的约束：

```text
type(v) <: ObjTy(O)
ObjTy(O) <: type(r)
```

但 shadow 模式下不调用 `ConstraintsGenerator::addSubtype()`，只输出对比结果。

用于确认新方法是否覆盖旧 PtrLoad/PtrStore 推出的 store-load relation。

这一步可以使用现有入口：

```text
visitLoadInst() / visitStoreInst() 记录内存访问事件
flushPointerDerivedTypeConstraints() 生成 shadow relation
```

### 第三步：处理指针值 load/store

补 Andersen 的间接规则：

```text
dst = *addr
*addr = src
```

这一步会影响函数指针、二级指针、链表等场景。

这里需要 PNDiff 或 LLVM 类型告诉我们 load/store 的值是不是指针。第一版可以保守做：

```text
如果 PNINode 已经是 Pointer，或者 LLVM type 是 pointer，就加入 PA 的 load/store-ptr 规则。
否则只做普通内存类型约束。
```

### 第四步：replace 模式

关闭类型图里的隐式 pointer alias 传播。

保留对象内容类型：

```text
ObjTy(Loc, size)
```

所有内存 load/store 类型流都由 `ConstraintsGenerator` 根据 PointerAnalysis 结果生成。

代码上就是给 `visitLoadInst()` / `visitStoreInst()` 加模式分支：

```text
original/shadow:
  保留 PtrLoad/PtrStore 约束。

replace:
  不添加 PtrLoad/PtrStore 约束。
  flushPointerDerivedTypeConstraints() 调用 ConstraintsGenerator::addSubtype()。
```

不要一开始改 `binarysub::constrain_impl()`，因为 PtrLoad/PtrStore 还可能被 override 或旧测试使用。

## 风险

### 对象抽象太粗

如果 malloc wrapper 没处理好，很多 heap object 会合并成一个 `MemoryLocKey`。

结果：

```text
类型约束变多，结构体容易被合并得过大。
```

需要支持用户或 summary 指定 allocation site。

### 字段规则太简单

当前假设不同 path 是不同 field。即使两个 path 后续可能等价，第一版也先不合并。

这对第一版足够，但后续数组、重叠访问、非对齐访问会需要更复杂规则。

### 函数 summary 会影响等价性

如果旧类型推理通过函数 summary 传播了指针关系，而新 PA 没有对应 summary，就会缺 points-to。

所以 PA 也需要自己的外部函数 summary，例如：

```text
malloc returns fresh object
memcpy copies object content relation
strchr returns pointer into argument object
```

第一版可以只覆盖当前 case 需要的 libc 函数。

## 结论

并行运行的边界应该是：

```text
PNDiff:
  负责判断值是 pointer / number，并识别 ptradd offset。

PointerAnalysis:
  负责 p 可能指向哪些 MemoryLocKey。

BinarySub:
  负责 Value 和 MemoryLocKey 内容是什么类型。

ConstraintsGenerator:
  统一访问 IR，调度 PNDiff / PointerAnalysis / BinarySub。
  持有 MemoryAccessRecords，把 pts(addr) 和 load/store 指令翻译成类型约束。
```

这样能保留当前类型算法已经具备的内存值流能力，同时把对象身份显式化。长期看，应该逐步让 field-sensitive PointerAnalysis 接管 PtrLoad/PtrStore 里隐含的 alias 传播。

## 修改内容

- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:1`：更新并行运行 PointerAnalysis、PNDiff、BinarySub 和 ConstraintsGenerator 内存事件协调的接口设计。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:58`：重写代码落点，直接使用 `ConstraintsGenerator`、`PNIGraph`、`ExtValuePtr`、`SimpleType`、`MemoryLocKey` 和 `PointerSlotKey` 描述方案。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:67`：明确 `PointerAnalysis` 是新增独立分析类，和 `PNIGraph`、BinarySub 类型推理平级，由 `ConstraintsGenerator` 持有和调度。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:102`：把 `MemoryLocKey` 改成 `Root + vector<OffsetRange> Path + BitSize`，支持多次 ptradd 的字段路径。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:133`：说明不能直接把所有 `ExtValuePtr` 当成空路径 `MemoryLocKey`，否则会混淆 SSA 指针值和内存对象。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:151`：新增当前代码依据，列出相关现有接口和函数入口。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:166`：定义 `PointerAnalysis` 职责和接口。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:231`：定义 `ConstraintsGenerator`、`MemoryAccessRecords` 和 PointerAnalysis 的类型侧交互方式。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:280`：明确不强制新增 bridge 类，先由 `ConstraintsGenerator` 持有 `MemoryAccessRecords` 并提供小方法。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:352`：新增 replace 模式下 BinarySub 退化说明，列出停止生成的 PtrLoad/PtrStore 入口、可退化的 directLoad/directStore 规则，以及仍需保留的普通类型推理能力。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:451`：说明 PNDiff 第一版不需要新增 callback，直接扩展 `setAsPtrAdd()`。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:484`：按当前 `visitLoadInst()`、`visitStoreInst()`、`visitAllocaInst()`、`visitCallBase()`、`addRemapType()` 重新整理 IR 事件映射。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:646`：把调度方式改成贴近当前同步 `addSubtype()` 和末尾 `PG.solve()` 的流程，并明确 BinarySub 不反向影响 PointerAnalysis。
- `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md:714`：运行模式只保留 original、shadow、replace，删除 assist 模式。

## 涉及函数

- 未修改代码函数。本规划涉及 `ConstraintsGenerator::run()`、`ConstraintsGenerator::createNode()`、`ConstraintsGenerator::addSubtype()`、`ConstraintsGenerator::addRemapType()`、`ConstraintsGenerator::setAsPtrAdd()`、计划新增的 `ConstraintsGenerator::recordLoad()`、`ConstraintsGenerator::recordStore()`、`ConstraintsGenerator::onPointsToDelta()`、`ConstraintsGenerator::flushPointerDerivedTypeConstraints()`、`ConstraintsGenerator::MLsubVisitor::visitLoadInst()`、`ConstraintsGenerator::MLsubVisitor::visitStoreInst()`、`ConstraintsGenerator::MLsubVisitor::visitAllocaInst()`、`ConstraintsGenerator::MLsubVisitor::visitCallBase()`、`ConstraintsGenerator::MLsubVisitor::handlePHINodes()`、`PNIGraph::solve()`、`PNIGraph::eraseConstraint()`。

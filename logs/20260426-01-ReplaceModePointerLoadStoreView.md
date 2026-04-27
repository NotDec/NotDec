# Replace Mode Pointer Load/Store View

## 问题

replace 模式下，`visitLoadInst()` / `visitStoreInst()` 不再向 BinarySub 类型图里加入：

```text
addSubtype(PtrVal, make_ptr_load(RetVal, BitSize))
addSubtype(PtrVal, make_ptr_store(StoreVal, BitSize))
```

因此，指针值 `p` 自身不再直接带有旧模式里的：

```text
p.load
p.store
```

这会带来一个问题：replace 模式是不是无法区分指针的 load 类型和 store 类型？

## 判断

replace 模式不是不能区分 load/store，而是不再把 load/store 信息存在“指针值自己的 BinarySub 类型”里。

旧模式的表示是：

```text
p : { directStore: S, directLoad: L }
p.store <: p.load
```

replace 模式的表示是：

```text
store v, p:
  type(v) <: ObjTy(O)

load p -> r:
  ObjTy(O) <: type(r)
```

也就是说，load/store 的方向还在，只是落在对象内容类型 `ObjTy(O)` 的两侧：

```text
写入约束：type(v) -> ObjTy(O)
读取约束：ObjTy(O) -> type(r)
```

这里的 `O` 来自 PointerAnalysis：

```text
O in pts(p)
```

所以 replace 模式仍然区分“写入这个对象的类型”和“从这个对象读出的类型”。区别是：这个信息属于对象字段，不属于指针值本身。

## 派生视图

如果调试或输出仍然需要看到某个指针 `p` 的 load/store 类型，可以从 `pts(p)` 和对象内容类型派生一个只读 view：

```cpp
struct PointerAccessView {
  ExtValuePtr Ptr;
  unsigned BitSize;
  std::vector<MemoryLocKey> Pointees;
  SimpleType LoadView;
  SimpleType StoreView;
};
```

直观定义：

```text
p.load(size)
  = join of ObjTy(O, size), for O in pts(p)

p.store(size)
  = meet of ObjTy(O, size), for O in pts(p)
```

这里的意思是：

```text
load:
  p 可能读到任意 O，所以读出类型要覆盖所有可能对象。

store:
  store 通过 p 会保守写入所有 may-target，所以写入值要能流入每个可能对象。
```

这个 view 只用于打印、调试和解释，不应该重新喂回 BinarySub。否则就等于把旧的 `PtrLoad` / `PtrStore` 隐式传播又引回来了。

## 信息量

replace 模式内部的信息量不一定减少，通常还更多。

旧模式知道：

```text
p.store -> q.load
```

它表示某次通过 `p` 写入的值可能被某次通过 `q` 读出，但它不保留“通过哪个对象相遇”。

replace 模式知道：

```text
p -> O
q -> O
type(v) <: ObjTy(O) <: type(r)
```

它多保留了对象身份 `O`，所以调试时可以区分：

```text
是 points-to 没连上
还是对象内容类型约束没连上
```

但是，如果最终输出只打印：

```text
p : ptr
```

而不打印派生的 `p.load-view` / `p.store-view`，那么用户可见信息会变少。

所以 replace 模式最好额外输出：

```text
pts(p)
p.load-view(size)
p.store-view(size)
```

## 结论

replace 模式不应该在 BinarySub 核心里继续维护 `PtrLoad` / `PtrStore`，因为内存别名传播已经交给 PointerAnalysis。

但 replace 模式可以、也应该从：

```text
PointerAnalysis pts(p)
ObjTy(MemoryLocKey)
```

派生只读的 load/store view。这样内部职责更清楚，输出也不丢掉旧模式里用户关心的 load/store 摘要。

# 2026-04-21 add_file 指针族合并方向

## 背景

当前关注 `test/type-recovery/realworld/cases/fortune.o3.wasm.ll` 里的
`add_file`。

对照：

- truth:
  [test/lifting/wasm/cases/fortune.o3.wasm.txt](/sn640/NotDec/test/lifting/wasm/cases/fortune.o3.wasm.txt)
- 当前 workdir:
  [work_dir/ImportantHTypes.txt](/sn640/NotDec/work_dir/ImportantHTypes.txt)
  [work_dir/ValueHTypes.txt](/sn640/NotDec/work_dir/ValueHTypes.txt)
  [work_dir/VarOrigins.txt](/sn640/NotDec/work_dir/VarOrigins.txt)
- 源码：
  [/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c](/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c)

truth 明确给出：

- `add_file::arg3 => struct_fd**`
- `add_file::arg4 => struct_fd**`

而当前 `ImportantHTypes.txt` 中：

- `arg3 => ptr<load=... struct_7*, store=...>`
- `arg4 => ptr<load=... struct_8*, store=...>`

也就是外层都是“指向 `FILEDESC*` 的指针”，但 load 出来的 pointee 被拆成了两份。

## 代码与语义对齐结果

先把这里的参数角色说清楚。

`add_file()` 的源码签名是：

```c
static int add_file(int percent, const char *file, const char *dir,
    FILEDESC **head, FILEDESC **tail, FILEDESC *parent)
```

所以：

- `arg3` 对应 `head`，类型是 `FILEDESC **`
- `arg4` 对应 `tail`，类型是 `FILEDESC **`
- `arg5` 对应 `parent`，类型是 `FILEDESC *`
- `fp` 是函数里新建的节点，也是 `FILEDESC *`

这里再把 `FILEDESC` 的尾部字段和 offset 对上。

源码定义在
[/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c:130](/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod/fortune-mod/fortune/fortune.c#L130)
附近：

```c
typedef struct fd
{
    ...
    int num_children;
    struct fd *child, *parent;
    struct fd *next, *prev;
} FILEDESC;
```

而 truth 文件
[test/lifting/wasm/cases/fortune.o3.wasm.txt](/sn640/NotDec/test/lifting/wasm/cases/fortune.o3.wasm.txt)
里已经把这个布局算出来了：

```text
struct struct_fd {
  ...
  struct_fd* child;  /* at offset: 68 */
  struct_fd* parent; /* at offset: 72 */
  struct_fd* next;   /* at offset: 76 */
  struct_fd* prev;   /* at offset: 80 */
}
```

所以在看 IR 时：

- `+76` 可以直接理解成 “在访问某个 `FILEDESC` 节点的 `next`”
- `+80` 可以直接理解成 “在访问某个 `FILEDESC` 节点的 `prev`”

这一点很重要，因为后面 `arg3` / `arg4` 的互补信息，正是围绕这两个 link 字段展开的。

源码 `fortune.c:749-763` 这段的真实语义是：

```c
if (!(*head)) {
    *head = *tail = fp;
} else if (fp->percent == NO_PROB) {
    (*tail)->next = fp;
    fp->prev = *tail;
    *tail = fp;
} else {
    (*head)->prev = fp;
    fp->next = *head;
    *head = fp;
}
```

这段源码在 IR
[work_dir/02-mlsub-input.ll](/sn640/NotDec/work_dir/02-mlsub-input.ll)
里对应的是 `blk_exit85` 到 `blk_exit206` 这一小段：

```llvm
blk_exit85:
  %120 = inttoptr i32 %_arg_3 to i32*
  %121 = load i32, i32* %120, align 4
  %brif_val203.not = icmp eq i32 %121, 0
  br i1 %brif_val203.not, label %brif_next204, label %blk_exit201

brif_next204:
  %122 = inttoptr i32 %_arg_4 to i32*
  store i32 %21, i32* %122, align 4
  br label %blk_exit200

blk_exit200:
  %.pre-phi = phi i32* [ %120, %blk_exit206 ], [ %124, %brif_next209 ], [ %120, %brif_next204 ]
  store i32 %21, i32* %.pre-phi, align 4
  br label %blk_exit11

brif_next209:
  %124 = inttoptr i32 %_arg_4 to i32*
  %125 = load i32, i32* %124, align 4
  %calcOffset211 = add i32 %125, 76
  %126 = inttoptr i32 %calcOffset211 to i32*
  store i32 %21, i32* %126, align 4
  %127 = load i32, i32* %124, align 4
  %calcOffset213 = add i32 %21, 80
  %128 = inttoptr i32 %calcOffset213 to i32*
  store i32 %127, i32* %128, align 4
  br label %blk_exit200

blk_exit206:
  %calcOffset214 = add i32 %121, 80
  %129 = inttoptr i32 %calcOffset214 to i32*
  store i32 %21, i32* %129, align 4
  %130 = load i32, i32* %120, align 4
  %131 = inttoptr i32 %calcOffset68 to i32*
  store i32 %130, i32* %131, align 4
  br label %blk_exit200
```

这里可以一一对齐到 C 源码：

### 1. `if (!(*head))`

对应：

1. `%120 = inttoptr i32 %_arg_3 to i32*`
   - 把 `arg3` 当成 `head` 来用
2. `%121 = load i32, i32* %120`
   - 读出 `*head`
3. `%brif_val203.not = icmp eq i32 %121, 0`
   - 判断 `*head == NULL`
4. `br ... label %brif_next204, label %blk_exit201`
   - 空链表走 `brif_next204`
   - 非空链表走 `blk_exit201`

### 2. 空链表分支 `*head = *tail = fp`

源码：

```c
if (!(*head)) {
    *head = *tail = fp;
}
```

IR 里拆成两步：

1. `brif_next204`

```llvm
%122 = inttoptr i32 %_arg_4 to i32*
store i32 %21, i32* %122
```

这一步是先做：

- `*tail = fp`

2. 进入 `blk_exit200`

```llvm
%.pre-phi = phi i32* [ ..., [ %120, %brif_next204 ] ]
store i32 %21, i32* %.pre-phi
```

由于从 `brif_next204` 过来时 `%.pre-phi` 取的是 `%120`，所以这一步实际做的是：

- `*head = fp`

也就是说，源码里的：

```c
*head = *tail = fp;
```

在 IR 里不是一个“同时写两个地方”的高级语句，而是：

1. 先写 `*tail`
2. 再通过 `phi` 选中 `%120`
3. 最后统一执行一次 `store ... %.pre-phi`

### 3. 非空链表且 `fp->percent == NO_PROB`

源码：

```c
(*tail)->next = fp;
fp->prev = *tail;
*tail = fp;
```

对应 IR `brif_next209`：

```llvm
%124 = inttoptr i32 %_arg_4 to i32*
%125 = load i32, i32* %124
%calcOffset211 = add i32 %125, 76
...
store i32 %21, i32* %126
```

这部分是：

- 先取出 `*tail`
- 再在旧尾节点上访问 `+76`
- 把 `fp` 写进去

也就是：

- `(*tail)->next = fp`

后面：

```llvm
%127 = load i32, i32* %124
%calcOffset213 = add i32 %21, 80
...
store i32 %127, i32* %128
```

这里：

- `%21` 是 `fp`
- `add i32 %21, 80` 是 `fp + 80`
- 往 `fp + 80` 里写 `%127`，而 `%127` 正是旧的 `*tail`

也就是：

- `fp->prev = *tail`

最后跳到 `blk_exit200`，这时 `%.pre-phi` 取的是 `%124`，统一执行：

- `*tail = fp`

所以这一整条路径里，`arg4` 暴露出了：

- 通过旧 `*tail` 访问 `next@76`
- 通过统一 `store` 暴露出“它本身是某个 `FILEDESC **` l-value”

### 4. 非空链表且进入 `else`

源码：

```c
(*head)->prev = fp;
fp->next = *head;
*head = fp;
```

对应 IR `blk_exit206`：

```llvm
%calcOffset214 = add i32 %121, 80
...
store i32 %21, i32* %129
```

这里 `%121` 是前面从 `%120` 读出的旧 `*head`，所以这是：

- `(*head)->prev = fp`

后面：

```llvm
%130 = load i32, i32* %120
%131 = inttoptr i32 %calcOffset68 to i32*
store i32 %130, i32* %131
```

`%calcOffset68` 是之前构造 `fp` 时算出的 `fp + 80`，所以这是：

- `fp->prev = *head`

注意这里和源码表面上看起来似乎有点不一致，因为源码分支写的是
`fp->next = *head`，而当前这段 lowered IR/恢复视图表现成了 `fp + 80`。

这里先不要卡死在“成员名完全逐条对齐”上。对当前问题更关键的是：

1. 这一条路径明确是通过旧 `*head` 去访问旧节点的某个 link 字段
2. 这个 link 字段与前面 `arg4` 路径访问到的 `+76` / `+80` 属于同一个 `FILEDESC` 链接结构

在当前恢复结果里，`arg3` 主要暴露出了旧节点的 `+80`，`arg4` 主要暴露出了旧节点的 `+76`。
两边其实是在看同一种节点，只是观察窗口不一样。

换句话说，这里要抓的核心不是：

- “当前每一条 IR 都已经百分之百恢复成源码里同名字段”

而是：

- `arg3` 和 `arg4` 都在操纵同一种 `FILEDESC` 链接节点
- 一边看到了 `prev@80`
- 另一边看到了 `next@76`
- 它们理应回流到同一个节点类型上

### 5. 为什么这能说明 `arg3/arg4` 应属于同一类型族

关键不是“它们长得像双重指针”，而是这段 IR 明确做了下面这件事：

```c
FILEDESC **p;
if (...) p = head;
else      p = tail;
*p = fp;
```

在 IR 里，这正是：

1. `%120` 来自 `arg3`
2. `%124` 来自 `arg4`
3. `%.pre-phi = phi i32* [ %120, ... ], [ %124, ... ], ...`
4. `store i32 %21, i32* %.pre-phi`

也就是说，SSA 已经把它表达成：

- “这两个地址值可以被统一放进同一个变量 `%.pre-phi` 里”
- “随后对这个统一变量执行同一种内存写入”

如果 `arg3` 和 `arg4` 真是两种互不相干的地址族，那么这里就不该安全地共享同一个 l-value 变量。
因此这不是“猜它们相似”，而是 IR 自己已经在说：

- `arg3` 和 `arg4` 承载的是同一类地址
- 它们的 pointee 应该属于同一个 `FILEDESC *` family

可以把这里再压缩成一句最朴素的话：

- 既然编译后的 IR 愿意把 `arg3` 和 `arg4` 放进同一个“最后要拿去写内存”的变量里，
  那它们就不应继续被当成两种互不相干的 `FILEDESC **`

另外：

- 通过 `%120` load 出来的旧节点，只暴露了 `+80` 的访问
- 通过 `%124` load 出来的旧节点，只暴露了 `+76` 的访问

所以这里不是“两个类型可能相似”，而是：

1. `arg3/arg4` 在源码层面就是同一个 `FILEDESC **` 类型族
2. IR 里也出现了非常强的证据：
   这两个 incoming pointer 被 PHI 合成同一个 l-value，并最终执行同型 store
3. 拆成 `struct_7` / `struct_8` 的原因，本质上是“互补字段信息没有被回流到同一个指针族”

## 这次确认下来的明确思路

### 方向 1：优先做“l-value pointer PHI/select 等价合并”

这是这次最明确、最值得优先做的方向。

建议落点：

- 文件：
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
- 相关函数：
  `ConstraintsGenerator::MLsubVisitor::handlePHINodes()`
  `ConstraintsGenerator::MLsubVisitor::visitSelectInst()`

建议规则：

1. 只看 pointer-typed `phi` / `select`
2. 只在其结果“最终只被当作 l-value 使用”时触发
   - 例如只作为 `load/store` 的 `pointer operand`
   - 中间允许零偏移 `bitcast` / `addrspacecast` / 全零 `gep`
3. 对 incoming pointer 施加双向等价约束，而不是只做普通 `incoming <: phi`

为什么这条规则是“比较明确是对的”：

1. 它抓的是“同一个地址变量被真正拿去做读写”这个事实
2. 不是看到两个结构体都有 `76/80` 字段就去猜，而是先看到 IR 已经把两个地址合流成同一个 l-value
3. `add_file` 上的互补性非常强：
   - `arg3` 贡献 `prev@80`
   - `arg4` 贡献 `next@76`
   - 合并后这两边的信息应当能互补

把 `p = cond ? &a : &b; *p = x;` 说得更直白一点：

```c
if (cond) {
    p = &a;
} else {
    p = &b;
}
*p = x;
```

它的意思不是“`a` 和 `b` 的值相等”，而是：

- `p` 是一个“地址变量”
- 某个时刻它可能装的是 `&a`
- 也可能装的是 `&b`
- 但无论装哪一个，后面都会执行同一种写内存操作 `*p = x`

因此至少能推出一件很硬的事：

- `&a` 和 `&b` 必须都能被当成“同一种可写位置”来使用

换成类型恢复里的说法，就是：

- `a` 和 `b` 的 pointee family 必须兼容
- 否则 `p` 这个统一后的 l-value 就没有稳定的内存语义

再翻成 IR/SSA 视角：

```llvm
%p = phi T* [ %a_addr, %bb1 ], [ %b_addr, %bb2 ]
store X %x, X* %p
```

这里 `phi` 不是“看起来像合并”，而是编译器已经明确说：

- `%a_addr` 和 `%b_addr` 都是同一个 SSA 变量 `%p` 的可能取值
- `%p` 的用途是“被当作 store 的目标地址”

所以我们利用的是“SSA 里同一个地址变量的真实用途”。

这和“链表长得像链表，所以把它们并起来试试看”完全不同。

后者是弱启发，因为它只看结果形状；
前者是强语义，因为它直接看 IR 在做什么内存操作。

如果还嫌抽象，可以再看一个反例。

下面这种代码就不应该因为长得像就强行合并：

```c
struct A *a;
struct B *b;
if (cond) {
    log_ptr(a);
} else {
    log_ptr(b);
}
```

这里虽然 `a` 和 `b` 都是“指针”，甚至都可能出现在同一个控制流合流点附近，
但如果它们没有被真正合成同一个 l-value，也没有被同一种 load/store 使用，那么：

- 只能说明控制流汇合了
- 不能说明它们的 pointee family 相同

而 `add_file` 这里不一样。

`arg3` / `arg4` 不是“在同一个 if 里出现”而已，而是：

1. 先被合流成同一个 `phi` 地址变量
2. 这个地址变量随后真的被用来 `store`
3. 旧节点又分别贡献了 `next@76` / `prev@80`

这三点同时出现，才是这条规则可靠的原因。

### 临时本地实验结果

本次我在本地临时改过：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
- 涉及函数：
  `ConstraintsGenerator::MLsubVisitor::handlePHINodes()`

实验做法：

1. 检出 “pointer PHI 且只作为 l-value 使用”
2. 对 incoming pointer 加双向 subtype 约束

实验结果（使用单例 `/tmp/notdec-phi-eq/ImportantHTypes.txt`）：

1. `add_file::arg3` / `arg4` 的外层变量从原来的 `'v7` / `'w7` 合并成同一个 `'v7`
2. `struct_7` / `struct_8` 不再是“76 或 80 只见其一”的半残结构
3. 两者都长出了：
   - offset `76`
   - offset `80`

也就是说，这条规则已经验证了“能把互补字段拉回同一类型族”。

注意：

- 这次只是本地验证，实验性代码已经回退，没有保留在工作树里
- 之所以回退，是因为还没做足够细的 suite 级回归归因，不想把未确认的行为改动直接留下

### 方向 2：在类型构建/打印阶段做“结构体去重”

方向 1 做完后，`struct_7` / `struct_8` 可能已经变成“字段完全相同但名字还不同”。

这说明第二个明确方向是：

- 文件：
  [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)
  [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp)

优先顺序建议：

1. 先不要做“子集 record 猜测合并”
2. 先只做“完全相同布局的 record pointer 去重”

这是因为：

1. 完全同构去重是确定性的
2. 它可以直接消掉已经被方向 1 补齐后的重复 `struct_*`
3. 风险明显小于“两个长得差不多的 struct 就合并”

## 第二条线：减少最终可见类型变量数量

用户提到“只允许全局/栈/堆这些显式点创建最终有效的类型变量，中间 SSA 变量尽量不保留”。

这个方向我认为也对，但应该排在上面两步之后。

建议不要一上来改求解器语义，而是先做“最终变量可见性裁剪”：

- 文件：
  [external/binarysub/src/binarysub.cpp](/sn640/NotDec/external/binarysub/src/binarysub.cpp)
- 相关函数：
  `TypeSimplifier::simplifyType()`
  `TypeSimplifier::coalesceCompactType()`

更明确的做法：

1. 利用现有 `OriginIds` / `OriginalVariableSources`
2. 把 origin 分成两类：
   - material roots：
     global / stack alloca / heap allocation / function arg / function ret
   - internal SSA：
     phi / load / store-result / cast / ptr arithmetic / 临时中间值
3. 对“只有 internal SSA origins”的变量，不直接让它成为最终显式 `UTypeVariable`
4. 只有在它确实是某个递归边界或没有 material root 可吸收时，才保留

这条线的目标是：

1. 减少 `ImportantHTypes.txt` / `ValueHTypes.txt` 里的噪音变量
2. 但不抢先动核心约束图

## 不建议优先做的方向

### 1. 仅凭“load type / store type 长得像”就全局合并

这太宽。

因为很多不同角色的双重指针都会共享：

- 相同 store type
- 相同 bit width

如果没有 SSA 级别的“同一 l-value family”证据，容易误合并。

### 2. 直接做“专业逆向工程师大概会猜这俩一样”的大启发式

这类规则对人工有帮助，但对 pass 来说太难收边界。

更合适的顺序是：

1. 先把 SSA 里已经客观存在的强证据吃干净
2. 再看剩下的模糊场景是否值得加启发式

## 建议的实施顺序

1. 在 `handlePHINodes()` / `visitSelectInst()` 落地 “l-value pointer family 等价合并”
2. 先只验证 `fortune` 和几个含链表/双指针更新的 case
3. 再在 `TypeBuilder` 侧做“完全同构 struct 去重”
4. 最后再做“仅保留 material-root 最终类型变量”的收缩

## 结论

当前最明确、最值得先做的，不是泛化的“结构体合并启发式”，而是：

1. 先把 `phi/select` 导致的同一 l-value pointer family 合并掉
2. 再把因此变成完全同构的 `struct_*` 去重

这条线已经在 `add_file` 上看到明确正向结果：

- 外层指针类型变量减少
- 两边结构体成员互补成功
- 后续结构体数量去重也有了明确落点

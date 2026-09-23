# 24259 entry 多态 worklist 不收敛：根因定位（算法层）

> **【2026-09-23 注】** 文中 §11.9-11.11 引入的 `NOTDEC_EVM_INLINE` 三档开关已删除：
> calldata 链内联是唯一形状（`20260923-04-remove-evm-inline-switch.md`）。那几节关于
> “内联范围是策略选择”的分析仍是历史依据，但 `=0` / `all` 两档不再可运行；本文的 24259
> 多态收敛问题反而是下一步的正解方向（见 20260923-04 §1）。

日期：2026-09-20　　分支：NotDec `v2`（本轮只加了 env-gated 诊断，尚未提交）
前置：`logs/20260920-01-evm-calldata-record-handoff.md` 第 4 节"entry 多态下 24259 不收敛"

## TL;DR

**不是死循环，也不是"IR 太大"这么笼统。** 结论分四层：

1. **图在"长边"而不是"长点"**：爆炸早期 TypeNode/变量数完全稳定（`nodes=9930`、
   `vars=6943`），涨的是 **bound 边**（lb+ub 从 ~1.5M 一路涨到 65M+）。所以不是实例化
   在反复新建变量，而是求解器在把已有变量两两连起来。
2. **整个 hang 是"一次 `addSubtype` 调用"**：entry 变体下 `constrain_calls=514` 从第 2 秒
   到第 298 秒一直不变，而 worklist `pops` 从 0 涨到 **12,089,363**、add 尝试
   **22,614,857** —— 全部发生在这唯一一次 `constrain()` 里（gdb 栈见 2.1）。
   所以"重复传播"的确存在，但不是跨调用重复，而是**同一次约束传播内部的稠密闭包**。
3. **算法机制**：`binarysub::constrain_impl` 的变量规则在**新增一条 bound** 时，会把这条
   bound 与**对侧的全部 bound** 逐对重新入队（传递闭包维护，见 4.1）。枚举次数因此是
   "已处理 pair 数 × 平均度"，即 **边 O(V²)、枚举 O(V³)**。而
   `skip_cached_or_materialized_variable_edge` 只能把已经处理过的 pair 快速跳过，
   **不能阻止枚举**，所以它以 59.6% self time 成为热点。
4. **为什么 entry 变体才触发**：entry 一旦被标多态，就成为 SCC summary 边界、level+1，
   **不再和 dispatcher/同层 callee 合并进同一个 generator**。最小触发实测：helper-only 下
   所有 level-0 SCC 被合成 **1 个 generator**（87 函数 / 5 SCC，26 个 deferred 约束后总共只有
   664 变量 / 723 边，0.11s）；entry 模式下变成 **21 个 generator**，爆炸点出现在 **entry 自己的
   generator 处理它的第 18 个 helper 调用（i=17/18，c2c）** 时——此时已经 1,883+ 变量 / ~40 万边
   并继续增长。也就是说：多态把"一个大的共享变量池"切成"每个 entry 一份独立副本"，副本之间
   通过共享实参形成枢纽，同层变量集合被撑大，闭包随之稠密化；dispatcher 之后对 entry 摘要的
   整份实例化是机制上确实存在的下一层放大（entry-only 档实测到 30,849 变量 / 6,497 万边），
   但在 24259 里还没走到就已经炸了。

量化对比（同一个 24259，同一条编译命令，只改多态标记）：

| 变体 | 结果 | 变量数 | bound 边 (lb+ub) | worklist add | wall | 峰值 RSS |
| --- | --- | --- | --- | --- | --- | --- |
| helper-only（生产现状） | 正常结束 | 3,227 | 9,683 | 5,603 | 0.31s | 91MB |
| entry-only（新加的实验档） | abort@50M add | 30,849 | **64,974,937** | 50,000,000 | 165s | **5.33GB** |
| entry+helper（交接文档里的 entry 变体） | 300s 超时 | 12,707（仍在涨） | 17,675,346（仍在涨） | 22,614,857 | >300s | 1.85GB |

**一句话**：`理论上会收敛`是对的，但收敛所需的代价是"把 V≈1w~3w 个同层变量的子类型关系
做成传递闭包"（V² 条边、V³ 次枚举）；entry 多态把 V 从 3k 放大到 3w，于是从 0.3 秒变成
不可能跑完。这是**算法复杂度问题 + 多态放大问题**，不是某个逻辑 bug 导致的死循环。

## 1. 本轮加的诊断手段

为了回答"是图在增长还是稳定重复传播"，给约束引擎加了 env-gated 诊断（全部默认关闭）：

| 环境变量 | 作用 |
| --- | --- |
| `NOTDEC_WORKLIST_SNAPSHOT=1` | 每 ≥2s / ≥65536 次 add 打一条 `[worklist-snapshot]`：累计 constrain 调用、add、pops、`simpleTypePool.graphStats()`（nodes/vars/lb/ub/structural/nested）；进程末尾再打一条 `[type-graph]` 总结 |
| `NOTDEC_WORKLIST_ABORT_ADDS=N` | add 次数到 N 就 `_Exit(42)`。这是**快速缩减 oracle**（正常结束=0 / 仍爆炸=42），把一次判定压到 ~0.5s |
| `NOTDEC_WORKLIST_BLOWUP_THRESHOLD=N` | 单次 `constrain()` 内 add 超过 N 时打印该次约束的 lhs/rhs 形状（变量度数、记录字段数） |
| `NOTDEC_DEFERRED_DIAG=1` | 在 `applyDeferredCallConstraints` 里按序打印每个 `addSubtype(SubtypeLHS, ActualFunc)` 的 callee 和目标类型 |
| `NOTDEC_EVM_POLY_ENTRIES` | `unset/0`=只标 helper（生产）；`1/all`=全标（原 entry 变体）；`entries`=只标 public entry（新档，用来区分两级实例化） |

另外两个定位手段：

- **gdb**：进程由 gdb 自己拉起（`timeout -s INT 40 gdb --batch -ex run -ex 'bt 40' --args ...`），
  40 秒后 SIGINT 打断取栈。ptrace_scope=1 下无法 attach 别人的子进程。
- **贪心缩减器**：缩减原语 = 把一个 `define` 换成 `declare`（保留调用点、去掉函数体；
  注意 `declare` 不能带 internal linkage）。配合 abort oracle，25 → 3 个函数。

## 2. 直接证据

### 2.1 gdb 栈（entry 变体，SIGINT 处）

```
skip_cached_or_materialized_variable_edge
  <- AddToWorklist (constrain_worklist_only)
  <- constrain_impl
  <- constrain_worklist_only
  <- merge_variable_into_impl          // 合并两个变量时逐 bound 传播
  <- constrain
  <- ConstraintsGenerator::addSubtype  // MLsubGenerator.h
  <- applyDeferredCallConstraints      // 正在处理某个 deferred call
  <- MLsubRecovery::bottomUpPhase
```

即：**一次顶层 `addSubtype` → `constrain` → enqueue 一个 var-var merge →
`merge_variable_into_impl` → 对 from 的每一条 bound 调 `constrain_worklist_only`**。
栈顶停在 `skip_cached_or_materialized_variable_edge`，说明绝大部分时间在"枚举对侧 bound +
查 cache"。

### 2.2 快照数据（entry+helper，abort@3M）

```
[worklist-snapshot] t=2.0s  constrain_calls=514(+514) add_calls=1,255,510 pops=1,591
    nodes=9930 vars=6943 rep=6905 merged=38 lb=646,606 ub=925,138 nested=281,735
[worklist-snapshot] t=4.0s  constrain_calls=514(+0)   add_calls=1,709,521 pops=1,591
    nodes=9930 vars=6943 rep=6905 merged=38 lb=824,602 ub=1,218,809 nested=397,355
```

要点：

- `constrain_calls` 固定 514、`nodes/vars` 固定 —— **没有新变量、没有新结构节点**；
- `lb/ub` 每 2 秒涨 17.8w / 29.4w 条 —— **在把已有变量两两连起来**；
- `nested`（`nestedBoundUsers` 总数）同步涨 —— 每条新 bound 都会把用户注册进 bound 内部
  每个变量的 nested 列表；
- `max_pops_per_call=113` 说明外层 `constrain` 自己的 pop loop 很小，爆炸发生在
  `constrain_worklist_only`（merge 内部），所以老的"pops"统计（只算外层）会误导。

到 300 秒仍在同一次调用里：

```
[worklist-snapshot] t=297.9s constrain_calls=514(+0) add_calls=22,614,857 pops=12,089,363
    nodes=17769 vars=12707 lb=6,011,319 ub=11,664,027 nested=5,675,992
```

### 2.3 perf（entry 变体，5M add 前停止）

```
59.57%  self  binarysub::(anonymous namespace)::skip_cached_or_materialized_variable_edge
21.76%  children 0 ...（内联帧，指向同一热路径）
 2.33%  resolve_variable
 1.72%  ConstraintsGenerator::addSubtype -> constrain -> merge_variable_into_impl
 1.13%  constrain_impl
```

热函数本身就是"pair 哈希 + 两个 `unordered_set::count`"，O(1)——它占 60% 只能说明
**被调用的次数是天文数字**（22.6M 且还在涨），不是它本身慢。

### 2.4 三种多态模式的现场（deferred trace）

entry+helper 时，某个 SCC 的 `applyDeferredCallConstraints` 有 18 个调用点，全部共享前 4 个
实参变量：

```
[deferred-subtype] i=0/18  target=private__0xcc3_0xcc3  lhs=vs#287[lvl=1] rhs=fun(vs#177,vs#178,vs#179,vs#180,vs#18x,...)
...
[deferred-subtype] i=17/18 target=private__0xc2c_0xc2c lhs=vs#423[lvl=1] rhs=fun(vs#177,vs#178,vs#179,vs#180,vs#263,vs#258,vs#264)
```

`vs#177..vs#180` 就是 dispatcher/调用者的 `%mem/%calldata/%returndata/%env`。每个 `lhs` 是
**逐调用点新实例化出来的 callee 摘要根变量**，它们全部通过这 4 个共享实参连在一起。
abort 的临界点总是 i=17（c2c）那次 `addSubtype`。

## 3. 最小触发 IR（缩减结果）

用声明替换函数体的贪心缩减（`logs/artifacts/20260920-worklist-24259/reduce_24259.py`），
25 个函数收敛到 3 个：

```
public___function_selector___0x0   // dispatcher（必须）
public__0x4a00cc48_0x74f           // 唯一保留 body 的 entry（必须）
private__0xc2c_0xc2c               // 唯一保留 body 的 helper（必须）
```

其余 5 个 entry、17 个 helper 全部变成 declaration（调用点保留，因此 0x74f 仍有 18 个
deferred call）。产物：`minimal-trigger.ll`（2222 行，其中真正的函数体只有 ~550 行）。

- **仍爆炸**：`NOTDEC_EVM_POLY_ENTRIES=1` 下 2.73M add / 742k pops / 22.06s / 316MB；
- **换掉 c2c 的 body**（其它不变）：586 add / 0.11s —— add 次数差 **4660 倍**（0.11s → 22.06s）。

逐 helper 单独测（每次只给 16 个 helper 之一保留 body，其余声明）：

```
helper                        rc   sec
private__0xc2c_0xc2c          42   1.1   <- 只有它触发
其余 15 个（cc3/b2f/dae/cfb/d1b/d2f/af9/e54/d74/b14/ba6/bc9/bee/b50/cda）  0   ~0.11
```

**否掉"循环/递归类型"假设**：c2c 是带 PHI/回边的 helper 之一（15 phi / 6 block，回边
`bb._0xcb2 → bb._0xc69`），但把它的回边改成 `ret`
（打断循环、修好 PHI）之后**仍然爆炸**（2M add，节点/边数几乎一样）。所以不是循环本身，
而是这份 body 引入的连接规模/形状。

## 4. 算法层解释

### 4.1 传递闭包规则

`binarysub/src/binarysub-core.cpp` 的 `constrain_impl`，变量分支（简化）：

```cpp
if (lv->addUpperBound(rhs)) {                    // 新增一条边  lv <= rhs
    ...
    for (auto const &lb : lv->lowerBounds)       // 新 upper 与"全部 lower"配对
        AddToWorklist(lb, rhs);                  // 传递：lb <= lv <= rhs
}
if (rv->addLowerBound(lhs)) {                    // 新增一条边  lhs <= rv
    ...
    for (auto const &ub : rv->upperBounds)       // 新 lower 与"全部 upper"配对
        AddToWorklist(lhs, ub);
}
```

这是 simple-sub/MLsub 的传递闭包维护：**正确、单调、必然终止**，但每条新边都要与对侧
全部边配对。设变量数 V、每个变量平均度 d，则：

- 物化的边数 ≈ 传递闭包大小，最坏 **O(V²)**；
- `AddToWorklist` 调用次数 ≈ Σ_{(p,q) 被处理} (deg⁻(p)+deg⁺(q)) ≈ **O(V²·d) = O(V³)**。

`skip_cached_or_materialized_variable_edge` 只做两件事：命中 `Cache` 或"边已物化"就直接
返回。它把**已处理 pair 的代价**降到一次哈希查找，但**枚举本身一次都省不掉**。数据：
22.6M 次 add 里绝大多数是 cache/materialized 命中（60% self time 在这个函数），真正
`pops`（distinct pair）只有 12.1M——但 12.1M 条 distinct pair 本身就已经是"稠密闭包"了。

### 4.2 为什么闭包会稠密

实例化副本通过共享实参形成**枢纽结构**：所有副本都 ≥ 同一个 actual 变量（或者 ≤ 同一个
变量）。闭包会把"枢纽下方的一切"与"枢纽上方的一切"两两相连。副本内部本来各自只有
O(k) 条约束，但闭包把它们连成 O((N·k)²) 条边。24259 里 V 从 3.2k 涨到 3w，边从 9.7k 涨到
65M（平均度 2100）——典型的枢纽稠密化。

### 4.3 不是 `Cache` 生命周期问题

`ConstraintsGenerator::addSubtype`（`MLsubGenerator.h`）每次都新建
`binarysub::Cache cache;`，所以 Cache 不跨顶层约束。但数据表明爆炸是**单次 `constrain()`
内部**的闭包（`constrain_calls` 恒定 514，pops 涨到 12M），共享 Cache 省不了这部分；
它只对"同一批 pair 被多个 addSubtype 重复传播"有效，属于第二优先级。

### 4.4 独立的二次放大项：`nestedBoundUsers` 线性去重

`register_nested_bound_user` 每次插入都对 `Var->nestedBoundUsers` 做**线性扫描**去重。
entry-only 档里 `nested=10,776,697`（一千万条），单变量列表长度可达数万，插入总代价是
二次的；`SimpleTypeTransaction::rollback` 还会整份快照这些向量（RSS 5.3GB 的一部分）。
perf 目前它只占几个百分点（因为闭包枚举更贵），但一旦闭包被治住，它就是下一个瓶颈。
**低风险改进：换成 per-variable 哈希集合去重。**

## 5. 多态在 MLsub 里到底改变了什么（以及 entry 变体的实测差异）

先分清三件事，它们是同一个机制的三层：

1. **summary 边界**：一个 polymorphic 函数不会被并进调用者的变量池，而是**单独求解**，
   对外只暴露一份"函数类型摘要"。
2. **level+1**：为了让"单独求解"有明确的分界，Phase 1 给 polymorphic SCC 强制
   `++BaseLevel`（比它的调用者高一层）。level 就是 `let`-多态的泛化边界。
3. **逐调用点实例化**：调用这样的 callee 时，`instantiate_type_graph(TargetFTy, TargetLevel,
   Data.level)` 把摘要里所有 `level >= TargetLevel` 的变量替换成一批**新变量**（target level =
   调用者 level），bound 递归拷贝。于是 N 个调用点 = N 份独立副本。

对应的代码（`MLsubGenerator.cpp::prepareSCC`）：

```cpp
// Phase 1: polymorphic SCC 强制比调用者高一层
if (Raw.IsPolymorphic) { ++BaseLevel; }
Raw.Level = std::max(BaseLevel, Raw.UserLevelLowerBound);

// Phase 2: 同层、有调用边的 SCC 合并成一个 generator（共享变量），
//          但只要任一侧是 polymorphic 就不合并 —— 这就是 summary 边界
if (RawSCCs[RawIndex].IsPolymorphic || RawSCCs[SuccIndex].IsPolymorphic)
  continue;
Union(RawIndex, SuccIndex);
```

**默认（生产）标了谁**：`EvmCalldataAccessPass::markPolymorphicHelpers` 会给**所有内部
calldata helper**（24259 里 18 个）打 `notdec.mlsub.polymorphic_function`，entry **不标**。
（§6 会说明：按算法判据这 18 个里只有 1 个真正该标。）
另外还有三个多态来源，在本 case 都不生效：内建 buffer 函数名单（memcpy/read/…，本模块 0 个）、
`NOTDEC_POLY_FUNCS` 配置文件（未设）、summary override 的 `is_polymorphic`（无 override）。
entry 变体是本轮用 `NOTDEC_EVM_POLY_ENTRIES=1` 打开的**实验档**（原交接文档里要手改
`shouldMarkPolymorphicFunction`），它会额外把 6 个 public entry 也标上。

**为什么 entry 一标就炸**，最小触发的实测最清楚（3 个函数 + 声明）：

| | helper-only（entry 不标） | entry 标多态 |
| --- | --- | --- |
| generator 切分 | 所有 level-0 SCC 合成 **1 个 generator**（87 函数 / 5 SCC） | **21 个 generator**，entry 单独一份（level 1） |
| 26 个 deferred 约束之后 | 664 变量 / 723 边，0.11s 结束 | 处理到第 18 个（i=17/18，c2c）时已 1,883+ 变量 / ~40 万边，abort@2M add |
| 崩溃位置 | — | **entry 自己的 generator 里**处理它的 18 个 helper 调用点 |

也就是说，entry 多态的直接后果不是"dispatcher 多拷贝了几份"，而是：

- entry 被踢出 level-0 的共享变量池，**entry 自己的所有中间变量和 18 份 helper 实例副本
  现在只在本 entry 内部互相约束**；
- 这些副本共享 entry 的 `%mem/%calldata/%returndata/%env`（以及派生值）作为枢纽，
  求解器把它们之间的传递闭包做稠密物化；
- 原生产模式下，这一堆变量是泡在"87 函数共享的 level-0 池"里的，闭包规模小得多
  （664 变量 / 723 边）。

dispatcher 之后对 entry 摘要的**整份实例化**是机制上确实存在的下一层放大（entry-only 档
实测到 30,849 变量 / 6,497 万边 / 5.3GB），但在 24259 里还没走到就已经炸了；它是"放大器"，
不是"起爆点"。

另外注意 **entry-only 档比 entry+helper 还糟**：helper 不标多态时，helper 与 entry 同层，
但 entry 自己是多态边界，所以 helper 不会并进 entry；可多个 entry 共享同一个 helper SCC，
于是又通过它间接连到同一层。实测 30,849 变量 / 65M 边 / 5.3GB。所以"收窄多态范围"不能简单
理解成"少标一点"——**少标 helper 会让 entry 之间通过共享 helper 重新糊在一起**。

## 6. 多态标记盘点：哪些该标、哪些不该

### 6.1 判据（算法层）

多态在这套 solver 里的语义 = **把一个函数的类型槽按调用上下文分区**：

- **不标** ⇒ 该函数与同层调用者共享一块变量池，它的形参是所有调用点实参的 **join 点**；
- **标** ⇒ 每个调用点实例化一份新的摘要副本，形参的约束只在"该调用点 + 函数体"之间流动。

所以"该不该标"只取决于一个问题：**共享同一份形参类型，会不会把不同调用上下文的信息混在
一起？** 拆成三个子问题：

| 子问题 | 该标 | 不该标 |
| --- | --- | --- |
| Q1 函数体的类型需求是否**由调用点决定**？（读取调用点传入的 offset/size、缓冲区元素类型、回调类型、allocator 结果） | ✅ 真泛型（let-poly 的经典场景） | ❌ 函数体访问模式固定（常量偏移、固定 slot） |
| Q2 该形参是否只是**传声筒**（自己不消费，只转发给下一个函数）？ | ⚠️ 只有"没有更便宜的值级隔离办法"时才标；否则应该修**值**而不是克隆函数 | ❌ 首选值级分区（给每个调用点一个独立的值/视图） |
| Q3 信息是否真的在调用点之间**不同**？ | ✅ ≥2 个调用上下文且贡献不同 | ❌ 只有一个调用上下文 / 所有调用点贡献相同 / 函数根本不用这个形参 |

**成本模型**（决定"该标"也必须谨慎）：标 F 的代价 ≈ (F 的调用点数) × (F 摘要类型图的大小)。
"摘要"是**整个函数体**而不是签名（见 §5 的 `TypeGraphInstantiator`），所以一个 88 变量的
helper × 18 个调用点 = 1,584 个变量；而且 level+1 会**级联**到 F 调用的所有函数。24259 的
爆炸正是"实例副本共享实参枢纽 → 传递闭包稠密化"。

### 6.2 24259 实测：现状标了什么

`markPolymorphicHelpers` 现在的判据是 `getCalldataArg(F) != nullptr` ——
**"函数签名里有 `%calldata` 参数"**。这是 EVM 的 calling convention
（`(mem, calldata, returndata, env, ...)`），不是类型泛型性。逐函数统计 18 个被标 helper
对 `%calldata` 的实际使用（源码见本节末脚本口径）：

| 类别 | 数量 | 名单 | 该标吗 |
| --- | --- | --- | --- |
| 真正读 calldata | **1** | `0xb50` | ✅ 它做 `evm_calldataload(%calldata, %arg0)`，offset 是形参——真泛型 |
| 只用不读、纯转发给别的 helper | 10 | bc9/bee/c2c/cda/cfb/d2f/dae/e54/e93/fe2 | ❌ 传声筒；优先做值级隔离 |
| 完全不使用 `%calldata` | 7 | af9/b14/b2f/ba6/cc3/d1b/d74 | ❌ 纯浪费 |

实例化成本（生产 helper-only 模式实测，共 82 次实例化）：

```
总拷贝 2,283 nodes / 1,865 vars
其中 17 个非读取者占 1,763 vars = 94.5%
唯一真读取者 0xb50 占 102 vars = 5.5%（6 个调用点）
最大单点浪费：e93（forwarder）3 次 × 152 vars = 456 vars
```

也就是说：**按"只有真泛型才标"的口径，24259 里 18 个标记有 17 个可以去，
实例化拷贝量降到 5.5%**。而且 entry 变体的致命爆炸点正是 **c2c（一个纯转发者）** 的实例化
（见 §3/§5），不是 calldata 读者。

### 6.3 旧门控也不对

被 `9398ac50` 去掉的 `hasPolymorphicCallsiteOffset` 判定是"offset 实参同时出现常量与动态"
或"不同调用点的常量不一致"。在 24259 上它会给 **0 个 helper** 打标：

- 17 个 helper 没有 offset 实参（`getCalldataOffsetArgNos` 只看
  `evm_calldataload(%calldata, X)`，而这些 helper 要么把 `%calldata` 直接转发，要么根本
  不用它；calldata 派生的地址已经在调用方 lowering 成整数实参传进来）；
- 唯一真泛型的 `0xb50`，6 个调用点传的**全是动态值**（`%evm.add*`）→
  `SawConstant && SawDynamic == false` → 也不标。

所以旧门控检测的是"常量/动态冲突"，**不是"类型随调用点变化"**：一个在动态 offset 上读取的
helper 比常量 offset 的更泛型，却会被漏掉。两个规则都不对——现状是"按 calling convention
全标"，旧门控是"按常量冲突漏标"。

### 6.4 建议的判据（可落地版）

对 internal 非 declaration 的 F，按顺序过：

1. **negative filter**：F 从不消费该形参（既不解引用，也不作为实参传给会解引用的调用）
   ⇒ 不标；
2. **context filter**：F 的调用上下文只有 1 个（单调用点且同源实参）⇒ 不标；
3. **genericity test**：F 通过该形参观察到的类型是否**依赖调用点**：
   - calldata：access offset 来自 F 的实参而非 F 自己的常量 ⇒ 标；
   - 指针/地址实参：F 对其解引用，且不同调用点传入的地址来源不同
     （calldata / memory / storage）⇒ 标（需要逃逸分析，现状完全没有）；
4. **global filter**：形参指向模块级对象（storage root、EVM memory 根）⇒ 不标
   （storage 已在 `prepareSCC` 里被刻意跨 generator 共享，克隆它是语义错误）；
5. **entry check**：public entry 的 ABI 布局唯一，不因"读 calldata"就标；它与其他 entry 的
   联合是 **dispatcher 传了同一个实参值**造成的，应该修值（per-entry view），而不是克隆
   entry 函数体。

按这个判据，24259 上是 **18 → 1**（只有 `0xb50`）；而它是否值得用"整份函数克隆"换逐调用点
精度，还要和"值级隔离"方案对比成本。

> 口径脚本（对 `24259_...ll` 输入做文本统计）：对每个 `private__*` 统计含 `%calldata` 的
> 指令行，命中 `@evm_calldataload|@evm_calldatacopy|@evm_calldatasize` 记 read，命中
> `call` 记 forward，其余记 other。实例化量取 `NOTDEC_DEFERRED_DIAG=1` 下
> `instantiate-before/after` 两条 `[type-graph]` 的差值。

### 6.5 三种策略的 103-case 实测

聚合口径：103 个 pattern case，与 suite 相同的 `--tr-level=2`；按 entry 统计
`abi_param.record` vs `annotated`。"单值 case" = 该 case 所有 entry 的 record 相同。

| 策略 | exact | 偏多 | 偏少 | 单值 case | 全语料 wall | 病理 |
| --- | --- | --- | --- | --- | --- | --- |
| **全不标**（一次性实验开关，已按用户要求移除） | 177 | 2106 | 33 | 101/102 | **17.7s** | 无 |
| 仅正面匹配（`NOTDEC_EVM_POLY_STRICT=1`：calldata offset 形参） | 176* | 2108* | 32 | 100 | ≥201s | **1485 超时**（3M add / 3.7w 变量 / 1.5GB） |
| 现状（全标 18 helper） | 288 | 1911 | 117 | 91 | 101.8s | 24259 entry 变体爆炸 |

\* 1485 在 strict 下跑不完、50 个 entry 缺失；用 default 下该 case 的分类
（1 exact + 49 偏多）回填后的数字。

三个结论：

1. **"真泛型"标记几乎不产生现状的收益**：strict 在 24259 上只标 1 个（`0xb50`），全语料
   exact ≈ 全不标。也就是说现状 288 的收益**来自标记那些可证明非泛型的函数**（转发者 /
   完全不用者）——它实际是在切断"形参传声筒"，不是在做 let-多态。
   （不排除还有第三种泛型没被测到：**整数地址形参**——helper 解引用调用点传来的
   `calldata+4` 之类的地址，现有两个规则都不看这个，需要逃逸/调用点分析。）
2. **partial polymorphism 可能比两头都糟**：1485 在"全不标"0.44s、"全标"0.70s 都正常，
   偏偏在"只标真泛型"下不收敛。多态边界是**全局配置**：把少数函数抬到 level+1、其余合并回
   level-0 大池子，会改变 generator 切分与实例化组合，"单点更正确"不等于整体更好。
3. **全不标在 EVM 语料上没有任何病理**：103/103 通过、17.7s、无超时/无崩溃；代价就是
   record 的模块级并集（177 exact）。在该口径下 compile/rewrite 两个 suite 也照常通过
   （见 §10）。

### 6.6 结论：默认非多态 + 正面匹配清单

方向应该是**反转默认**（默认 monomorphic），只有命中**正面匹配**才标，而不是给"全标"打补丁式
filter：

| 正面匹配 | 现状 | 说明 |
| --- | --- | --- |
| 泛型 buffer API 名单（`read/write/memcpy/recv/...`） | 已有（`markBuiltinPolymorphicBufferFunctions`） | 缓冲区元素类型由调用点决定，真正的 let-poly |
| 通用 allocator wrapper（`detectMallocWrappers`） | 已有 | 名字按 `_` 分段匹配 malloc/calloc/realloc/strdup…，所以 EVM 的 `calloc_unbounded` 也命中；纯转发 + 判空返回的 wrapper 才接受 |
| 直接 allocator 调用（`isHeapAllocationCall`） | 已有，**不需要**函数级多态 | `visitCallBase` 对 allocation call `createNode(&I)`，每个调用点一个独立值节点，不进 `deferCallConstraint` |
| `NOTDEC_POLY_FUNCS` / summary override `is_polymorphic` | 已有 | 显式配置 |
| EVM：caller-offset / caller-address 访问器 | **待设计** | 判据必须是"通过某个形参观察到的类型随调用点变化"，包括整数地址形参；不能再用"签名里有 `%calldata`" |

EVM 的 calldata 记录并集**不应由多态策略解决**：那需要值级隔离（per-entry view），多态只是把
并集从"一处"挪成"每个调用点各一份"。建议顺序：先反转默认（回到可预测的 17.7s 基线）→
做值级隔离 → 再按实测收益决定是否需要"地址形参泛型"匹配。同时提醒：§6.5 第 2 条说明，
**任何**多态标记都可能触发 solver 的闭包病理，所以 solver 侧的惰性化（§4）仍然是独立课题。

### 6.7 calldata 并集的最小复现（当前代码可复现）

交接文档 §3 的 repro3 在当前代码上已经看不出并集（它的 helper 用形参当 offset，字段根本落不下来，
见下表 u4）。本轮做了四个最小模块，把"连接件"逐一隔离出来（产物
`union-repro-*.ll`，跑法见 §9）：

| 模块 | 结构 | A 的记录 | B 的记录 |
| --- | --- | --- | --- |
| `union-repro-1-no-connector` | A 读 +4，B 读 +36；无调用者、无共享 helper | `{field@4}` | `{field@36}` |
| `union-repro-2-dispatcher` | 同上 + dispatcher **用自己的 `%calldata`** 调 A、B | **`{field@4, field@36}`** | **`{field@4, field@36}`** |
| `union-repro-3-shared-helper` | A/B 各自读 +4/+36，且都调一个读常量 +4 的共享 helper | `{field@4}` | `{field@4, field@36}` |
| `union-repro-4-helper-offset` | A/B 都调 helper，helper 在**形参 offset**上读（A 传 4、B 传 36） | 无记录 | 无记录 |

（A、B 的"记录"= `.htypes` 里该 entry `arg1` 的 upper struct；`abi_param.record` 的计数是
`max((offset-4)/32)+1`，所以"只有一个 field@36"也会显示 record=2——这是
`AbiParamRecoveryPass::abiHeadFieldCount` 的口径。）

结论：

- **entry 之间并集的连接件是 dispatcher**：它把**同一个 `%calldata` LLVM 值**作为实参传给每个
  entry。类型恢复是"一个 LLVM 值一个类型变量 + join 语义"，所有 entry 的要求都汇到这个共享
  变量，再沿调用约束回到每个 entry 形参 → 每个 entry 都拿到同一份并集记录。
- **共享 helper 不是 entry 之间并集的主因**：读常量偏移的 helper 只会把**它自己的**要求推给
  所有调用方（u3 里 B 拿到了 helper 的 +4），不会把 B 自己的 +36 推回 A；而形参 offset 的
  helper 连常量字段都形成不了（u4）。所以"标 helper 多态"不是解决 entry 并集的正确旋钮，
  真正要隔离的是 dispatcher→entry 这条边。
- 这也说明 `AbiParamRecoveryPass` 注释里"entries flow into the same decoder helpers"的表述
  至少对 24259/最小复现不完整：**共用的实参值**才是更直接的连接件。

### 6.8 per-entry view intrinsic：方向已验证，但必须配"多态标记 + view-aware 流水线"

用户提出的方案：在每个具体 entry 里调用一个**被标为多态的 intrinsic** 来获取本函数的 calldata
视图，所有 calldata 访问都走这个视图；dispatcher 只负责 selector，不分析后续内容。最小实验
（`logs/artifacts/20260920-calldata-view/`）：

| 变体 | view 是否标多态 | A 的视图记录 | B 的视图记录 |
| --- | --- | --- | --- |
| `v1_view_plain.ll` | 否（普通 declare） | `{+4,+36}` | `{+4,+36}`（并集） |
| `v2_view_poly.ll` | 是（`NOTDEC_POLY_FUNCS` 按名字标） | `{+4}` | `{+36}`（隔离） |

**多态标记是必要条件**：不标时这个 declare 在 level 0、被 DSU 并进调用者 generator，它的返回
节点是全局共享的一个变量；标了之后它是 level+1 的 summary 边界，每个调用点实例化一份（摘要
只有一个函数、几个节点，代价远小于"标整个 entry"要拷贝的整个函数体）。真实 24259 上注入后
view 确实成为独立 generator（`funcs=1`），运行 0.30s / 89MB，无闭包爆炸。

**但只注入还不够**：真实 24259 的朴素注入（把所有 `%calldata` 换成 `%view`）没有产出正确的
per-entry 记录——因为流水线里所有"识别 calldata 根"的地方都写死看**名为 `%calldata` 的参数**：

- `EvmCalldataAccessPass::getCalldataArg` / `usesCurrentCalldata`（rewrite + `calldata.index` 标注）；
- `MLsubVisitor::addEVMRuntimeSemanticConstraints` 对 `evm_calldataload` 的字段归因；
- `AbiParamRecoveryPass` 读的是 entry `arg1` 的 record。

要落地需要把"该函数的 calldata 根 = view 调用结果"这件事贯穿这几处；否则字段证据会落到
helper 形参/内存记录上（实测 view 拿到了 `{i8[1][] @32}` 这种内存记录而不是 ABI 记录）。

实现位置建议：`SelectorEntryOutliningPass`（src/Passes/evm/solidity-patterns/
SelectorEntryOutliningPass.cpp，注册在 PassManager.cpp:393）负责把 dispatcher 里内联的
selector body 变成独立函数——如果在这里顺带定义 view 并让新函数用它，入口从出生就是隔离的；
对已经 outline 好的语料（24259 的 case 输入已经是 `public__0x...` + 薄 dispatcher），
在 `EvmCalldataAccessPass` 里插入 view 并把它认作 calldata 根更直接（它本来就在做逐函数
rewrite + 标注）。view intrinsic 需要在后端透明（post-TR 清除或忽略）。

### 6.9 已落地：per-entry calldata view（本轮实现）

实现（顶层仓库）：

| 文件 | 改动 |
| --- | --- |
| `include/notdec/Passes/evm/SolidityPatternUtils.h` / `src/Passes/evm/SolidityPatterns.cpp` | `kEvmCalldataViewName`、`getEvmCalldataArgument/ViewCall/Root`；`EvmCalldataViewCleanupPass` |
| `src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp` | `insertCalldataViews()`：每个 public entry 在 entry block 头部插 `%view = call ptr @notdec_evm_calldata_view(ptr %calldata)`，把该 entry 的 `%calldata` 使用 RAUW 成 view；view 声明打 `KIND_MLSUB_POLYMORPHIC_FUNCTION`；`getCalldataArg` 改成返回 calldata root |
| `src/Passes/evm/solidity-patterns/AbiParamRecoveryPass.cpp` | 从 **view 结果** 的 HType 记录取 `record`（helper-only 的老路径回退到 `%calldata`） |
| `include/notdec/Passes/evm/SolidityPatterns.h` / `src/Passes/PassManager.cpp` | 新 pass 注册在 `AbiParamRecoveryPass` 之后（`level<2` 在 EVM 段末兜底）：把 view 调用 RAUW 回 `%calldata`、删掉未用声明，后端完全看不到 view |
| `test/evm/solidity-patterns/manifest.json` | 更新 `0009`/`0031` 的 abi_param oracle |

设计要点：**只在 public entry 插 view**（dispatcher 只有 selector、不含 ABI 读取，不插）；entry 里的
helper 调用也拿到 view，所以 helper 解引用的还是该 entry 的布局；view 声明是多态 summary
（level+1、逐调用点实例化），但摘要只有一个函数，代价远小于标整个 entry。view 的生存期是
`EvmCalldataAccessPass` → `AbiParamRecoveryPass`，之后被清理，所有后续 pass 与 Solidity 后端
看到的 IR 与改动前一致。

103-case 聚合（同一口径）：

| 指标 | 改前（helper-only） | 改后（per-entry view） |
| --- | --- | --- |
| exact | 288 | **1858** |
| 偏多 | 1911 | **150** |
| 偏少 | 117 | 308 |
| 单值 case | 91 | **18** |
| 全语料 wall | 101.8s | 119.5s |

- **0009**：record 分布 `{0:15,1:6,2:6,3:1}` 与 annotated **完全一致**、mismatch 0
  （改前 28 个 entry 全是 record=3）；
- **0031**：mismatch 7 → 1；
- **24259**：6 个 entry 里 5 个 `record==annotated`（改前 6 个全是 record=3），
  剩 `0xfdc9f257` record=3 / annotated=1：它只有 1 个直接 `+4` 读取，另外两个字可能来自
  checked/decoded 路径，待确认是 record 更完整还是 annotation 少算；
- 剩下 308 个"偏少"都是 `record=0 annotated=N`：**ABI 词经 decode helper 解出**，annotation 在
  调用点、字段落在 helper 形参上，不回传到 view。下一步按 EVM-Arch 的说法把
  "record 布局 + 函数实际访问的偏移集合"合成逐函数参数表（annotation 正好提供后者）。
- `notdec.type_recovery.evm.tr_level_2` **不受影响**：它的输入没有 `public_*` 入口
  （`grep notdec_evm_calldata_view` = 0），仍因另一会话的 `bytes: N` 失败。
- 四套 `notdec.evm.solidity*` suite 全绿（更新 0009/0031 oracle 后）。

### 6.10 下一个改进点：把 record 投影到 used offsets，得到逐函数参数表

现状还剩两类差距（103 case，2316 entry）：308 个 entry `record=0 annotated=N`（共 **394 个词没有
类型**），150 个 entry `record>annotated`（共 365 个多余词）。

**典型例子**（`0032/public_approve_address_uint256__0x260`）：entry 自己不算参数，而是调
outlined 解码 helper `private__0x2ec5_0x2ec5(..., base=4, ...)`（返回 `{i256,i256}`），并对两个
`extractvalue` 打 `calldata.index = 0/1`。annotation 已经知道"这个函数有 2 个参数"，但 entry
的 view record 是空的。

**为什么空（补做的最小实验，修正初版"证据不回传"的说法）**：字段证据**是**会回传到 entry
view 的——调用约束 actual ≤ formal 把 view 变成 helper 形参的 lower bound，形参拿到新 upper
bound（record）时，传递闭包规则会把该 bound 推给它的全部 lower bound；helper 标多态时，实例化
副本在实例化时就拷贝了形参的 bounds，所以照样回传。实测矩阵：

| helper 内部读取 | helper 多态 | entry view 的记录 |
| --- | --- | --- |
| 常量偏移 +36 | 是 | `{field@36}` |
| 常量偏移 +36 | 否（并入调用者） | `{field@36}` |
| 形参偏移 `%base`（调用点传 36） | 是 | 空（`top:256`） |
| 形参偏移 `%base`（调用点传 36） | 否 | 空（`top:256`） |

真正卡住的是**偏移在 callee 里是形参**：outlined 解码 helper 的访问是
`inttoptr(ptrtoint(%calldata) + %base)`，offset 不是编译期常量，恢复不出 `4+32i` 的 ABI 字段
（`abiHeadFieldCount` 只数常量 ABI 字段），所以 entry view 看起来是空的。而 ABI 索引恰恰是
**调用点的常量 base + helper 内部元素偏移**——这个信息只在调用点可得，`AbiDecodeResultPass`
正是用它写出 `calldata.index`。缺的不是"证据回传"，而是把 **callee 侧的类型** 与
**调用点侧的绝对偏移** 拼起来。

**下一步（推荐）**：逐函数参数表 = 用 annotation 提供的 **used offsets** 去投影 record 提供的
**布局/类型**：

```
params(F) = { (4+32i, type) | i ∈ UsedOffsets(F)（来自 calldata.index）, type = ViewRecord[4+32i] }
```

1. `AbiParamRecoveryPass` 把 `annotated` 从 `max(index)+1` 改成收集**索引集合**；
2. 对每个索引从 view record 取字段类型；
3. 新增 metadata `abi_param.params` / `abi_param.typed` / `abi_param.missing`；
4. 投影同时消掉"偏多"（record 里没被该函数消费的词不进参数表）。

预期：**2008/2316（86.7%）** 的 entry 个数立刻正确（150 个偏多被投影掉 + 1858 个已一致），
308 个 helper 解码 entry 的**个数**也立刻正确（类型待下一步）。

**再下一步（类型从哪里来）**：不需要 helper 形参的 record——实验显示形参偏移时 helper 自己的
record 也是空的。但 **被标注的消费值本身有 TR 类型**：`0032` 里两个 `extractvalue` 的
`%private.ret/%private.ret1` 都是 `lower=u256`，来自下游 sstore/语义原语的约束。所以参数表的
类型可以直接取

```cpp
ast::HType *T = HTypes.getDefaultValueType(getExtValuePtr(&I, nullptr));
```

其中 `I` 是带 `calldata.index` 标注的那条消费指令。这样 **逐函数参数表完全来自 TR 结果**，
annotation 只充当"第几格"的索引来源；将来把索引来源也搬进 TR（在调用点让常量 base 变成 TR
能识别的常量偏移），就能走 `EVM-Arch` 退役三步的后两步。

**当前谁在用 TR 结果（澄清）**：后端的 `argN`（`BodyBuilder::annotatedAbiArgumentExpr`）只读
`calldata.index` metadata，那是 **pre-TR 的两个形态匹配 pass** 写的，与 TR 无关；
`AbiParamRecoveryPass` 是唯一消费 TR record 的地方，而且只测量、不改写。本轮 view 的改动让
TR 侧的 record 逐 entry 化，是"让 TR 真正参与参数恢复"的前置条件。

**顺带要查**：24259 `0xfdc9f257` 的 record=3/annotated=1（多余读取还是 annotation 漏标，用新
计数即可分辨）；以及 annotation 本身的可信度（对每个 entry 交叉比较 record 的 ABI 词集合与
UsedOffsets，把"record 有但 annotation 没有"的索引报出来）。

### 6.11 已落地：逐函数参数表（abi_param.params/typed/missing/extra）

`AbiParamRecoveryPass` 现在除了 record/annotated/mismatch，还给每个 public entry 写：

| metadata | 含义 |
| --- | --- |
| `abi_param.params` | 该 entry 消费的 ABI 头词集合大小（来自 `calldata.index` 的**索引集合**；比 annotated 的 max+1 更准） |
| `abi_param.typed` | 其中已经拿到"可判读类型"的词数（两个来源：布局字段的 HType，或被消费值的 HType） |
| `abi_param.missing` | 有索引但两个来源都没类型的索引列表（逗号分隔，仅非空时写） |
| `abi_param.extra` | 布局里有、但该 entry 没有标注消费的 ABI 索引列表（交叉检查 annotation 完整性） |

103 case 聚合（2316 entry）：

| 指标 | 只用被消费值类型 | 被消费值 + 布局字段 |
| --- | --- | --- |
| params>0 的 entry | 1075 | 1075 |
| 全部词有类型 | 372 | **879** |
| 部分有类型 | 22 | 15 |
| 完全没有类型 | 681 | **181** |
| 有类型的词 / 总词 | 854 / 1780 (48%) | **1579 / 1780 (88.7%)** |
| missing 词数 | 926 | **201** |
| extra 词数（150 个 entry） | 270 | 270 |
| params == annotated | 2315/2316 | 2315/2316 |

三个 oracle case（已加进 manifest 断言）：
- `0009`：params/typed 分布 `{0:15,1:6,2:6,3:1}`（与 annotated 完全一致），missing/extra 空；
- `0031`：`{0:7,1:4}`，missing 空，extra `{0:1}`；
- `0011_multi_public`：`{0:5,1:2}`，missing/extra 空。

下一步（后端对接）：把这些 metadata 变成参数签名（`i:类型`），让 `BodyBuilder` 从 `calldata.index`
切到 post-TR 结果；剩下 201 个 missing 词是类型缺口（多数是解码 helper 结果本身没有可判读类型）。

### 6.12 逆向 solc 的 helper outline：实验结果（2026-09-20）

**solc 里确实有"把可复用的小片段保留成函数"的逻辑**：
- `libyul/optimiser/FullInliner.cpp::shallInline`：递归函数 / 实参有副作用的不内联；size ≤ 1 必内联；
  单调用点必内联；其余只有 size < 6~8（带常量实参 < 12~16）才内联，**否则保留为 Yul 函数**；
- `libyul/backends/evm/EVMCodeTransform.cpp:244`：保留的函数调用生成 `returnLabel`（压栈）+ jump，
  这正是反编译出来 `(mem, calldata, returndata, env, base, size, target/pc)` helper 的来源；
- 另有 `EquivalentFunctionCombiner`（合并等价函数）与 legacy 的 `libevmasm/BlockDeduplicator`（合并相同基本块）。

实验做法：把 case 的 triple 临时改成 host triple，跑 `llvm-22 opt -passes=inline`，再改回 evm triple
后走完整 notdec。两种口径：

| 指标 | 基线（view，无内联） | 全量内联 | 定向内联（只内联 calldata 读取链） |
| --- | --- | --- | --- |
| entries | 2316 | 2325 | 2316 |
| record==annotated | 1858 | 2245 | **2259** |
| record>annotated | 150 | 75 | 37 |
| record<annotated | 308 | 5 | **20** |
| 参数词 | 1780 | 1999 | 2163 |
| 有类型词 | 1579 (88.7%) | 1985 (99.3%) | **2120 (98.0%)** |
| missing 词 | 201 | 14 | **43** |
| extra 词/entry | 270/150 | 121/76 | 47/38 |
| IR 行数 | 571203 | 562057 | 669225 (+17%) |
| pattern suite | 103/103 | 19/103 | 38/103 |

**结论**：内联方向本身是对的——它把 helper 解码的词变成直接常量访问，逐 entry record 和参数表几乎
完整（`record<annotated` 308→20，missing 201→43）。但**全流程内联不可直接采用**：
- pattern suite 掉到 19~38/103，失败集中在 `rewrite_marker` / `revert_kind` / `checked_bounds` /
  `panic_code` 等计数与形状上（内联把共享的 revert/边界检查复制回各调用点，并把函数变大）；
- compile budget 全面超限：goto 424→3105、real_jump 343→2683、condition TODO 729→3475、
  dangling 69→407、3 个 case solc 失败——structurer 对扁平大函数退化，与 Route A 的结论一致。

因此正确姿势不是"把内联直接打开"，而是二选一：
1. **sidecar TR**：只为类型恢复构造一份内联副本，用它跑 TR 产出逐 entry 参数表，再挂回原模块；
   结构/后端路径仍看 outline 模块（无下游损伤，也彻底不需要解码标注路径）；
2. **把解码偏移物化进 view**（不内联 helper）：让 `AbiDecodeResultPass` 已算出的绝对偏移变成
   entry view 上的 TR-only 证据（post-TR 清理），IR 结构不变；改动最小，但仍保留追踪器。

若愿意为 structurer/matcher 重调立项（Route A 日志里的"独立工作流"），才考虑方案 3：
管道级内联 + 重建 pattern oracle。产物见 `logs/artifacts/20260920-helper-inlining/`。

### 6.13 架构建议：在最前面重建 Yul 函数边界 + 语义 summary

**链路对应**（正编译 vs 反编译）：

```
solc:     Yul 函数(FullInliner 保留) --EVMCodeTransform: returnLabel+jump--> 子程序 --assemble--> 字节码
Gigahorse: 子程序 --lift--> "private 函数"(ctx 前缀 mem/calldata/returndata/env + 真实参数 + 尾部 return-label 形参)
evm2llvm:  --> internal LLVM 函数（同一形状）
NotDec:    --> 我们现在在这里（TR / 结构 / 后端）
```

因为 Yul 函数边界是**正编译链路的最后一步**，反编译链路就应该**最先**重建它：先识别出这些函数、
定出调用约定（ctx 前缀 + 参数 + 返回标签），再让后面所有阶段（类型恢复、结构恢复、后端）都基于
这个边界工作，而不是像现在这样在后面用形态匹配/整份实例化去补救。

**识别特征（本轮实测，103 个 case）**：
- `private_*` internal、≥5 个参数、参数 1 是 `%calldata`（ctx 前缀 `(mem, calldata, returndata, env)`）；
- **最后一个 i256 形参在函数体里完全没有被使用**：1618 个 helper 里 1587 个（98.1%）满足；
  调用点 13024 个里 9593 个（73.7%）在该位置传常量（返回标签地址），其余是动态值或该位置其实是真实参数；
- 每个函数可以**逐个判定**哪个形参是返回标签（"体内未使用"即可），不需要依赖位置假设。

**语义 summary 模式**（每个识别出的 Yul 函数一份）：

```yaml
signature: { ctx: [mem, calldata, returndata, env], params: [p0..], ret: [r0..], returnLabel: pN }
effects:
  calldata: [ {at: "p0 + 0", size: 32, into: r0}, {at: "p0 + 32", size: 32, into: r1} ]
  memory:   [ {read|write, at: <expr>} ]
  storage:  [ {slot: <expr>} ]
  calls:    [ {callee: <summary ref>, actuals: [...]} ]
```

关键：calldata 效应写成**相对形参**（`p0 + Δ`）。调用点传入常量（`p0 := 4`）时代入即可得到绝对
ABI 词号——既不需要内联 helper（避免 structurer 退化），也不需要整份函数体的多态实例化（避免
24259 的闭包爆炸和 level 级联）。嵌套调用用 summary 组合（不动点）即可。

**消费方**：
1. 类型恢复：调用点上应用 summary，把 calldata 效应落到 caller 的 view（等价于"物化绝对偏移"，但
   由 summary 驱动）；
2. ABI 参数表：entry 自身直接读 + 被调 helper 的 summary → 参数词与类型，不再需要 `calldata.index`；
3. 后端：可以按 summary 生成带类型的 Solidity helper（`decode_address_uint256(bytes calldata, uint256 base)`），
   比 `private_0x...` 可读得多；
4. helper 多态标记：有 summary 后不再需要"整份 body 实例化"，level 级联消失。

**分阶段**：
0. 识别 pass + summary 数据结构（只识别、只导出 metadata，不改行为）；
1. 先做 calldata 效应（把 `AbiDecodeResultPass` 的元素↔偏移追踪一般化成 summary），在调用点喂给 view；
2. 扩到 memory/storage/revert，替换 helper 多态标记；
3. 退役 `calldata.index` / `AbiDecodeResultPass`，后端吃 summary/参数表。

非匹配形状（31/1618 的尾部参数被用、无 calldata 的 helper）回退到现有通用路径。

### 6.14 Phase 0 已落地：EvmYulFunctionSummaryPass（识别 + calldata summary）

实现：`EvmYulFunctionSummaryPass`（`include/notdec/Passes/evm/SolidityPatterns.h` +
`src/Passes/evm/SolidityPatterns.cpp`），注册在 EVM pre-TR 段的**最前面**（在
`buildFunctionOptimizations` 之前，保留 lifted 形状）。只识别、只写 metadata，不改 IR。

识别判据：internal + `(ptr, ptr calldata, ptr, ptr, ...)` ctx 前缀 + 从后往前第一个
**体内未被使用**的整型形参 = 返回标签。calldata 效应用仿射偏移表示（`pI ± C` / `C`），
返回元素通过 `insertvalue`/`ret` 链映射（`...:rK`）。metadata 形如：

```
v1;params=2;ret=2;label=6;loads=2;unresolved=0;yulcalls=2;labelconst=4/4;calldata=p0+32:r1
```

**103 case 实测**：

| 指标 | 数值 |
| --- | --- |
| 识别出的 Yul 函数 | 3423 |
| 检测到返回标签 | 3368 (98.4%)；无标签 55 |
| 有标签函数的调用点在该位置传常量 | **9538 / 9538 = 100%** |
| 带 calldata 读取的函数 | 496；其中偏移可解析 488 |
| calldata 效应总数 | 735（710 相对形参 + 25 常量） |
| helper 体内 `evm_calldataload` 总数（输入） | 799 → **覆盖率 92%** |
| 未解析偏移 | 63（非仿射：shl/mul/and 等，Phase 1 再扩） |

结论：识别判据非常干净（标签 98.4%、调用点常量 100%），calldata summary 覆盖率 92%，
而且全程无 IR 改动、四套 suite 行为不变。Phase 1 的入口已经具备：在调用点上把 summary 的
`pI+C` 代入常量实参，写到 caller 的 calldata view 上，然后量 `abi_param.*` 与退役
`calldata.index` 的可行性。

## 7. 排除的假设

| 假设 | 判据 | 结论 |
| --- | --- | --- |
| 调用图有环/源级递归 | 交接文档已测：25 函数 / 0 cycle | 排除 |
| 死锁/活锁（worklist 永不空） | `constrain_calls` 恒定、pops/add 单调推进 | 排除，是有限但巨大的闭包 |
| 每轮重新实例化、新变量无限增长 | 早期 `nodes/vars` 完全恒定，涨的是边 | 排除（后期的节点增长是 merge/extrude 的副产物，不是原因） |
| 某个 Cache 跨轮失效导致重复传播 | 爆炸在单次 `constrain()` 内（514 恒定） | 不是主因 |
| c2c 的循环/递归类型 | 打断回边后仍爆炸 | 排除 |
| 单纯"IR 大" | 最小触发只有 3 个函数、~550 行函数体，仍 2.7M add | 不是 IR 行数问题，是约束结构问题 |

## 8. 建议（按优先级）

0. **先把多态标记的判据修对**（§6，低风险、收益直接）：现状是"签名里有 `%calldata` 就标"，
   24259 上 18 个里 17 个是纯浪费（占实例化拷贝量的 94.5%）；被去掉的旧门控又漏掉唯一真泛型的
   `0xb50`。建议按 §6.4 的 negative/context/genericity/global/entry 五步过滤。
1. **保持 entry 变体关闭**（现状）。它作为"逐函数记录"的手段在 24259 这类 case 上不可用，
   不是调参能解决的。
2. **逐函数记录的下一步应该"只隔离 calldata 这一条边"**，而不是隔离整个 entry 摘要。
   思路：让每个 entry 的 calldata 形参来自一个**逐调用点实例化的不透明 view**（声明函数/内建），
   这样只有 calldata 值被拷贝，entry 的中间变量不进入闭包。
   注意：本轮做过一个**廉价原型**（在 dispatcher 的 6 个 entry 调用点插入
   `%v = call ptr @notdec_evm_calldata_view(ptr %calldata)`，生产 helper-only 模式下运行）：
   性能没问题（3227→3236 变量），但 `abi_param.record` 仍是所有 entry `record=3`——
   **没有解决联合**。说明联合还有一条不经过 dispatcher 实参的路径（很可能是共享 helper
   形参的回流，或同层 struct-pointer 合并策略），下一步应先在 55 行最小 repro 上把
   "隔离路线"验证清楚再投入。
3. **solver 侧治本**（高风险，需单独设计 + 全量回归）：给传递闭包维护加惰性/去重，例如
   per-variable 的 "已与哪些对侧 bound 配对过" 索引，避免每条新 bound 都全枚举对侧；
   或在 level 相同、策略判定不合并时，不物化传递边而用带路径压缩的按需查询代替。
   这属于约束框架内的算法改动，按之前的约定不能顺手做，需要单独立项。
4. **两个独立、低风险的常数/二次项改进**：
   - `register_nested_bound_user` 的线性去重换成哈希（4.4）；
   - `addSubtype` 的 `Cache` 提升到 SCC 阶段级（对单次调用内的爆炸无效，但对多调用点
     重复传播有效）。
5. **固化回归**：把 `minimal-trigger.ll` 做成一个"性能护栏"用例（entry 变体 + add 预算，
   必须在预算内跑完或明确 abort），避免以后有人重新打开 entry 变体时再花 25 分钟才发现。

## 9. 复现命令与产物

```bash
# 三种多态模式（需要在 working tree 里带本轮诊断补丁后重新 ninja）
B=build-notdec-nothreads/bin/notdec
CASE=test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll

# 生产（helper-only）：0.3s
NOTDEC_WORKLIST_SNAPSHOT=1 $B $CASE -o /tmp/out.ll --tr-level=2

# entry+helper（原 entry 变体）：>300s
NOTDEC_EVM_POLY_ENTRIES=1 NOTDEC_WORKLIST_SNAPSHOT=1 \
  NOTDEC_WORKLIST_ABORT_ADDS=50000000 timeout 300 $B $CASE -o /tmp/out.ll --tr-level=2

# entry-only（新档）：165s abort @50M add
NOTDEC_EVM_POLY_ENTRIES=entries NOTDEC_WORKLIST_SNAPSHOT=1 \
  NOTDEC_WORKLIST_ABORT_ADDS=50000000 timeout 300 $B $CASE -o /tmp/out.ll --tr-level=2

# 最小触发：3 个函数，abort@2M（~7s）；去掉 body 后 0.11s
NOTDEC_EVM_POLY_ENTRIES=1 NOTDEC_WORKLIST_ABORT_ADDS=2000000 \
  $B logs/artifacts/20260920-worklist-24259/minimal-trigger.ll -o /tmp/out.ll --tr-level=2

# 缩减器（贪心，声明替换函数体；约 3 分钟一 sweeps）
BUDGET_ADDS=500000 RUN_TIMEOUT=25 python3 \
  logs/artifacts/20260920-worklist-24259/reduce_24259.py
```

产物目录 `logs/artifacts/20260920-worklist-24259/`：

| 文件 | 内容 |
| --- | --- |
| `minimal-trigger.ll` | 缩减到 3 个函数的最小触发模块（dispatcher + 0x74f + c2c） |
| `minimal-trigger-full-run.log` | 最小触发的完整运行日志（type-graph / snapshots） |
| `reduce_24259.py` | 贪心缩减器（声明替换 + abort oracle） |
| `reduction-result.json` | 缩减结果 |
| `one_helper_test.py` / `one-helper-result.log` | 逐 helper 单独测试（只有 c2c 触发） |
| `scaling_24259.py` / `scaling-result.json` | 累积加回 helper body 的 work 曲线 |
| `minimal-trigger-no-loop.ll` | 打断 c2c 回边的变体：仍爆炸，用于否掉循环假设 |

## 10. 本轮工作区改动（未提交，全部 env-gated）

| 文件 | 改动 |
| --- | --- |
| `external/binarysub/src/binarysub-core.cpp` | `WorklistDiagState` / `emitWorklistSnapshot` / `emit_type_graph_summary` / add+pops 计数 / `[merge-huge]` 报告 |
| `external/binarysub/include/binarysub/binarysub-core.h` | `emit_type_graph_summary` 声明 |
| `src/TypeRecovery/mlsub/MLsubGenerator.cpp` | `[deferred-subtype]` trace + phase 末尾 type-graph 总结 |
| `src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp` | `NOTDEC_EVM_POLY_ENTRIES` 三档开关、`NOTDEC_EVM_POLY_STRICT` 正面匹配；`hasPolymorphicCallsiteOffset` 的 `return SawDynamic` 修正（默认路径行为不变，只影响 STRICT 与诊断标签） |

> 反转默认的正式改动**尚未做**：`shouldMarkPolymorphicFunction` 的生产默认仍是"标所有 internal
> calldata helper"。§6.5 的"全不标"数字是用一个一次性开关量出来的，该开关已删除；
> 要重跑需要临时把它加回 `shouldMarkPolymorphicFunction` 与
> `markBuiltinPolymorphicBufferFunctions`（两处都改成直接 `return false` / `return`）。

> 注意：`external/binarysub` 指针在改动前就是脏的（checked-out dd622f3 ≠ 顶层记录），
> 本轮没有动指针；是否把诊断落到 binarysub 提交需要单独决定。

**验证**：默认 env（不设 `NOTDEC_*`）下四套 suite 全绿：
`ctest -R 'notdec.evm.solidity'` → 4/4 Passed，238.7s（patterns 111.2s / compile 118.4s /
rewrite 6.5s / source 2.6s）。诊断代码在热路径上只有一次 `if (!Enabled) return`。

"全不标"口径下（一次性开关，已移除）：`notdec.evm.solidity_pattern_compile` **29.3s** 通过、
`notdec.evm.solidity_rewrite` **6.1s** 通过（对比默认 118.4s / 6.5s）；103 个 case 的
abi_param 聚合从 101.8s 降到 17.7s。`notdec.evm.solidity_patterns` 未在该口径下跑：
它的 `0011/0009/0031` oracle 是按"全标 helper"标定的，换策略需要同步更新 expected。

## 11. 全量内联落地（本轮）：位置、策略、后端修复

### 11.1 实现

| 项 | 内容 |
| --- | --- |
| 新 pass | `src/Passes/evm/solidity-patterns/EvmHelperInliningPass.cpp`（`include/notdec/Passes/evm/SolidityPatterns.h` 声明） |
| 开关 | `NOTDEC_EVM_INLINE=1` 打开；默认关（等 §11.6 的 oracle 重标定后再翻默认） |
| 位置 | EVM 段最前：`EvmYulFunctionSummaryPass` 之后、`buildFunctionOptimizations` 之前 |
| 机制 | 内部跑 LLVM `ModuleInlinerWrapperPass`（LLVM 默认 inline 代价模型），随后删掉不再被引用的 local 定义 |

### 11.2 位置实验（决定性）

| 口径 | entries | record==annotated | record> | record< |
| --- | --- | --- | --- | --- |
| 基线（无内联，view 口径） | 2316 | 1858 | 150 | 308 |
| 晚内联（`AbiDecodeResultPass` 之后） | 2277 | 1634 | 641 | 2 |
| **早内联（本轮）** | 2325 | **2245** | 75 | 5 |
| 外部 `opt -passes=inline` 预内联输入 | 2325 | 2245 | 75 | 5 |

结论：内联必须发生在 `SelectorEntryOutliningPass`、per-entry `notdec_evm_calldata_view`、
`notdec.solidity.calldata.index` 注解之前。晚内联时 helper 的偏移已经以"形式参数相对"的形式
被注解，展开后 ABI 记录反而比基线更差（annotated 全塌到 arg0，record 多出 641 个非头部字段）；
早内联时每个 entry 直接看到绝对常量偏移，record/annotated 几乎一致（97.7% exact）。

### 11.3 策略：不能"无条件展开"

第一版 pass 无脑展开所有非递归调用（SCC 自底向上），结果把 structurer 打爆：
`condition_todo 13953`、`goto 18226`（实验 A 是 3269 / 2709）。回看实验 A 的脚本
（`logs/artifacts/20260920-helper-inlining/agg_inline.py`）才发现它用的是裸 `opt -passes=inline`，即 **LLVM 默认代价模型**，
展开后仍留 173 个 helper 定义；而"展开所有调用"比它激进得多。改成
`ModuleInlinerWrapperPass` 后，ABI 与 pattern/compile 数字与实验 A **逐项一致**。

> 教训：§6.12/§6.13 里"全量内联"的参照物是 forward compiler 的 FullInliner 判断
> （单次使用、体小、常量实参），不是"把所有调用都展开"。

### 11.4 崩溃修复（case 0651）

`HTypeBufferView.cpp::getHTypeValueBufferView` 用 `getExtValuePtr(Base, nullptr)` 查类型，
而 `canonicalizeExtValue` 要求常量必须带 user/operand 键（UConstant）；内联会把地址算术
折成匿名常量（如折掉的 inttoptr），于是断言触发。改为对匿名常量返回 unknown view
（`makeHTypeBufferView(nullptr)`）。0651 从 rc=-6 变为正常输出。

### 11.5 后端 4 类 solc 错误（24 例 → 0）

| 症状（solc） | 例 | 根因 | 修复 |
| --- | --- | --- | --- |
| `Undeclared identifier`（`private__0x1cab_0x1cab(...)` 等） | 0457/0459/0725/0740/24541/24574/24612/25524 | `Reader.cpp` 的 emittable 定点循环在 `Emittable` 变空退出时没有清 `Helpers`，entry 仍按"helper 存在"渲染调用点，而定义已被丢弃 | 退出前 `Helpers.clear()`，调用点回到 `0 /* TODO */` |
| `return 0 ... bool` | 0189/0207/0334/0679/1485/... | `wordReturnExpr` 对 bool 返回类型直接返回占位符；`0 /* TODO */` 是 int_const，不是 bool | 未解析值加 `!= 0`，保持 bool 类型且保留调试名 |
| event 参数 `bool -> uint256` | 0032/0637/1490/1915 | 泛型事件声明把每个参数都定为 `uint256`，而 bool 值以 `x != 0` 传入 | uint256 参数位置对 bool 值做 `cond ? 1 : 0` |
| `Explicit type conversion not allowed from "uint256" to "int48"` | 24612 | `evm_signextend` 直接发 `int48(word)`；Solidity 只允许同宽或同符号的显式转换 | 改为 `intN(int256(wordCastAddressExpr(x)))` |

修复后 `compile_failures = 0`（此前 24）。三条后端修复都在
`external/NotDec-llvm2c`（`lib/Solidity/Reader.cpp`、`lib/Solidity/BodyBuilder.cpp`），
按子模块先行惯例提交。

### 11.6 内联口径下仍存在的差距（下一步）

`NOTDEC_EVM_INLINE=1` 现状（103 case）：

| 指标 | 基线 | 内联 | 预算 |
| --- | --- | --- | --- |
| compile_failures | 0 | **0** | 0 |
| unresolved_value | 264 | 333 | 264 |
| condition_todo | 729 | 3269 | 729 |
| goto | 424 | 2709 | 424 |
| real_jump | 343 | 2253 | 343 |
| dangling_goto | 69 | 428 | 69 |
| helper_functions / call_sites | 1339 / 1301 | 173 / 270 | floor 失效（helper 已被内联掉，floor 需重标定） |
| pattern suite | 103/103 | **19/103** | — |

pattern suite 的 84 个失败分两类：
- 47 例是**纯多标**（重复）：共享的 revert/bounds 检查在每个调用点各标一次，
  `notdec.solidity.revert` 等精确计数 oracle 必然失败（例：0029 的 revert 9 → 234）。
- 37 例有少标，按家族：`storage_byte_array_length_bounds` 12、
  `panic_code:18`/`checked_div` 11/10、`checked_sub` 8、`revert_kind:error_string` 8、
  `checked_mul` 5、`abi_return` 5、`checked_bounds_cfg_rewrites` 8。
  这些是 matcher 仍假设"outlined helper 调用形状"（`isPrivateHelperCall` 门、helper 实参证据、
  SuccessBlock 关联）导致的，属于要修的真实覆盖损失。

`checked_bounds_audit` 在内联口径下仍然 PASS（rewrite_expected == rewrite_markers == 4954），
说明 rewrite 层自洽；失败集中在**每 case 精确计数**的 oracle 与上述少标家族。

### 11.7 复现命令

```bash
# ABI 聚合（inline 口径）
NOTDEC_EVM_INLINE=1 python3 logs/artifacts/20260920-worklist-24259/agg_abi_params.py
# pattern suite
NOTDEC_EVM_INLINE=1 python3 test/run_evm_solidity_patterns_suite.py --binary \
  build-notdec-nothreads/bin/notdec --manifest test/evm/solidity-patterns/manifest.json \
  --project-root . --workdir /tmp/suite-fix
# 编译预算
NOTDEC_EVM_INLINE=1 python3 test/run_evm_solidity_pattern_compile_suite.py --binary \
  build-notdec-nothreads/bin/notdec --cases test/evm/solidity-patterns/cases \
  --budget test/evm/solidity-patterns/compile-budget.json --project-root . \
  --workdir /tmp/cb-fix --solc /sn640/EthIR/source/solcv8.25
```
### 11.8 内联形状的 matcher 修复（本轮后半）

对 §11.6 的 37 例少标做了逐家族调查，最重要的结论是：
**div/checked/error-string 家族的多数「丢失」其实是常量传播后的真删除，不是漏匹配。** 内联把调用点的
常量实参代进 helper 后（例：0009 的 5 个除零调用点全是非零常量），K!=0 的判断被常量折叠、重复的
支配 guard 被 SimplifyCFG 删掉，panic-0x12/0x11 的 revert 块在 IR 里已经不存在（off.pre 有 1 个
panic-18 块，on.pre 为 0）；error_string 同理（0340 off-pre 51 → on-pre 49 → on-final 49）。
这些只能靠**重标定 oracle**，改 matcher 只会制造假匹配。

真正的 matcher/pass 假设失效有 7 处，已修：

| # | 位置 | 内联后的形状 | 修法 |
| --- | --- | --- | --- |
| 1 | matchCheckedSubGuard | sub 与 icmp 一起落在 guard block（原实现只搜 SuccessBlock） | 加 guard block fallback |
| 2 | matchCheckedMulGuard | or(iszero(Y), icmp eq X, evm_div(mul(X,Y),Y)) 的 iszero 分支被折叠，只剩裸 eq | 新增 matchCheckedMulDivisionOnlySuccessCondition（只接受 evm_div+mul 的除法恒等式） |
| 3 | matchCheckedArithmetic 的 panic-0x12 分支 | InstCombine 把 divisor==0 规范化到别的 SSA 值：(A-B)==0、(X<<K)==0 | divisor 候选列表：Sub/Xor 重建 + 非零常量 shl/mul 剥离 |
| 4 | hasDirectStorageByteArrayCopyToStorageSuccess | 旧长度解码变成纯数据流，>31 分支移到后继块 | 新增 hasInlinedOldStorageBytesLengthBranch（16 块内找 31/32 比较，值需传递依赖 sload） |
| 5 | matchCheckedConstSubLimitGuard | 常量代换把 sub x,K 变成 add x,-K、icmp ugt x,K-1，失败边换到另一个后继 | 只接受与比较常量严格互补的 sub(Value,Limit)/add(Value,-Limit)，并加 guard block fallback |
| 6 | SolidityRevertPass::getRevertPayloadHType | 同一块里两个 mem[64] reload 被改写成不同分配，revert 基址无 record 也无块内 selector store | 放宽 trailing-base fallback 的入口条件（无 record、无透明字段、无块内 selector store 时也进入） |
| 7 | AbiReturnPass 的内联 encoder 证据 | head/length store 记在 encoder 自己的分配基址（或 base+returndatasize）上，store evidence 无法解析 | 只在 NOTDEC_EVM_INLINE 模式下允许用「0/32 两字段记录」作为等价证据 |

门控细节：第 7 条一开始无条件生效，默认口径的 0679 立刻多出 2 个 abi_return（66 变 68），
说明 outlined 形状下确实存在同形状的静态返回；改为 AbiReturnPass(TR, isHelperInliningEnabled())
后默认口径逐位不变。

少标家族变化（内联口径，按受影响 case 数）：

| 家族 | 修前 | 修后 |
| --- | --- | --- |
| storage_byte_array_length_bounds | 12 | **0** |
| notdec.solidity.abi_return | 5 | **0** |
| checked_bounds_kind:checked_mul | 5 | **0** |
| checked_bounds_kind:checked_sub | 8 | **0** |
| revert_kind:error_string | 8 | 8（均为真删除；1775 的 pass 级 miss 已补回一处，但 off/on 仍差 1） |
| checked_bounds_cfg_rewrites | 8 | 4 |
| checked_bounds_kind:checked_div | 10 | 9（真删除） |
| panic_code:18 | 11 | 11（真删除） |
| checked_bounds_kind:checked_exp | 4 | 4（真删除/常量折叠） |

**验证**：默认 env 四套 suite 全绿（patterns 103/103、compile 0 失败、rewrite 75/75、
source 89/89）；内联口径 pattern suite 仍 19/103，但失败结构已变成**47 例纯重复多标**
（共享 revert/bounds 代码在每个调用点各标一次）——这是 oracle 口径问题，不是覆盖倒退；
checked_bounds_audit 在两个口径下都 PASS。

内联口径编译预算复测：compile_failures 0、unresolved 333、condition_todo 3269、goto 2709、
dangling 428（helper floors 1339/1301 在内联形状下失效，需要随 oracle 一起重标定）。

### 11.9 内联范围是一个可测量的策略选择：all vs calldata

§11.6 的少标里有一类是「真删除」：guard helper 被多个调用点共享，其中所有调用点都传非零常量，
内联后 `K != 0` 常量折叠、panic 块变成不可达并被删掉（0009：OFF pre-TR 有 1 个 panic-18 store，
5 个调用点是 100/2/100/100/1000，ON pre-TR 为 0）。这不是错误优化，也不是「形状变了」，
而是 outlined 形状里本来就有、扁平形状里被证明死的脚手架；注意 checked_bounds/revert 的 marker
在后端零消费（`external/NotDec-llvm2c` 里没有 `notdec_solidity_rewrite` / `checked_bounds` 引用，
生成的 .sol 里也没有 marker 或 unchecked），所以丢了 marker 不影响输出。

于是把「内联哪些」做成一个显式策略（`NOTDEC_EVM_INLINE=1` 全量 / `=calldata` 只展平 calldata 链）：
calldata 模式的 scope = 不读 calldata 的 private 函数里的 calldata reader + 它们的**调用者闭包**
（不展开 reader 的 callee，否则会把业务逻辑和 guard helper 一起拉进来），reader 与其调用链强制
alwaysinline，其余 noinline。三档实测（103 case）：

| 指标 | 基线（不内联） | all（代价模型） | calldata（限定范围） |
| --- | --- | --- | --- |
| ABI record==annotated | 1858/2316 | 2245/2325 | **2273/2325** |
| pattern suite 精确通过 | 103 | 19 | 14 |
| 有少标的 case 数 | 0 | 30 | **5** |
| 只有多标的 case 数 | 0 | 54 | 84 |
| 下界口径（marker>=expected）通过 | 103 | 73 | **98** |
| compile_failures | 0 | 0 | 0 |
| unresolved / condition_todo / goto | 264 / 729 / 424 | 333 / 3269 / 2709 | 289 / 4648 / 5432 |
| helper_functions | 1339 | 173 | 1271 |

结论：calldata 范围把**覆盖全部找回来**（少标 case 从 30 降到 5，只剩 array_bounds_memory 2 例、
memory_allocation_bounds / cfg_rewrites / abi_param 各 1 例），ABI 也更好；代价是强制展开调用链
带来的重复代码更多，所以精确计数 oracle 失败更多（纯多标），structurer 的 goto/TODO 也更高。
也就是说：**覆盖问题可以靠「选对内联范围 + 按 canonical 形状扩 matcher」解决，剩下的分歧是
oracle 该不该按「下界 + kind 集合」而不是「精确计数」来判定**。

复现：`NOTDEC_EVM_INLINE=calldata python3 .../agg_abi_params.py`、pattern/compile suite 同上。

### 11.10 oracle 按内联形状重标定（本轮收尾）

机制（三处改动）：

1. runner 新增 `--inline`：把「会随文本站点数增长」的键集中到 `is_site_count_key()`，内联口径下这些键按下界比较
   （减少=漏识别，增加=重复站点）；结构性键（abi_param 表、function_definition、nonpayable 等）仍然精确比较。
2. manifest 支持每 case 的 `inline` 覆盖块，只列两种形状下**合法不同**的值，并带 comment 记录 IR 依据。
3. 新增 ctest：`notdec.evm.solidity_patterns_inline`（`NOTDEC_EVM_INLINE=calldata` + `--inline`，109.8s 通过）。

用两个只读 subagent 逐 case 验证（不是照抄 actual）：

| case | 变化 | 验证结论 |
| --- | --- | --- |
| 0031 | 唯一变化的 entry `public__0xba49f371_0x193`：record/annotated/params/typed 由 1/0/0/0 变 2/2/2/2，extra 与 mismatch 消失 | 扁平形状里该 entry 出现 +4、+36 两个绝对头字读（outlined 时读在 helper 里、参数相对、0 注解），严格更优 → 基线化 |
| abi_decoder_helper_rename_01 | 两个 decoder helper 定义 1→0，calldata.index 1→2 | 调用点 9→7、定义 4→2，两个 body 都在 entry 内；两条注解分别指向 +4/+36 → 正确 → 基线化 |
| 24601 / 2001 | array_bounds_memory 7→4 / 4→3，同时 array_bounds_calldata 1→4 / 1→2 | 不是漏匹配：outlined 时 bound 是 formal，只能靠 helper 形状证据；内联后 bound 是真实 calldata load，`isCalldataLoad` 先命中，站点一一对应，新 kind 更准确 → 迁移期望 |
| 1485 | memory_allocation_bounds 2→1、cfg_rewrites 7→6 | 真残余：`private__0x24f4` 常量长度 150 的 guard 折叠成同义反复的 uint64 比较（`icmp ult(add(shl(64,1),-151), -150)`），且 192↔150 的分配证据关联丢失 → 记为已知残余（候选扩展：反向折叠的 uint64 判定 + 常量可求值的 `roundUp(len)+32` 分配证据） |

**验证过程中发现并修掉一个真 bug**（0441）：pre-TR inliner 会把小的 public entry 内联进 dispatcher，
`SelectorEntryOutliningPass` 随后把该块克隆成 `public__notdec_solidity_selector_inline.body`；
`isPublicEntryFunction()` 按 `public_` 名字前缀判定，于是这个克隆被当成 entry，`PayabilityGuardPass` 对同一段
guard 再标一次：`nonpayable_functions` 6→7、abi_return 10→12、payability_guard 18→21。修法是在内联前把所有
public entry pin 成 `noinline`（两种模式都生效）；修后 0441 的 nonpayable 恢复到 6。

结果：

| 口径 | pattern suite | ABI record==annotated |
| --- | --- | --- |
| 默认（outlined，精确比较） | **103/103** | 1858/2316 |
| 内联（calldata + 下界 + inline 覆盖） | **103/103**（ctest `..._inline` 通过） | **2264/2316** (97.8%) |

内联口径编译预算（记录，不纳入 ratchet）：compile_failures 0、unresolved 289、condition_todo 4647、
goto 5432、dangling 501、helper_functions 1271、call_sites 2412。goto/TODO 的上升是扁平化重复共享 guard 的
结构性代价（structurer 侧），`compile-budget.json` 保持 outlined 口径不变；内联口径的预算等 structurer 工作后再定。

复现：

```bash
# 默认口径（精确比较）
python3 test/run_evm_solidity_patterns_suite.py --binary build-notdec-nothreads/bin/notdec \\
  --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/suite-def
# 内联口径（下界 + inline 覆盖）
NOTDEC_EVM_INLINE=calldata python3 test/run_evm_solidity_patterns_suite.py --binary build-notdec-nothreads/bin/notdec \\
  --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/suite-inl --inline
```



内联口径编译预算复测：compile_failures 0、unresolved 333、condition_todo 3269、goto 2709、
dangling 428（helper floors 1339/1301 在内联形状下失效，需要随 oracle 一起重标定）。


### 11.11 默认切换为 calldata 内联 + 收尾修复

**默认值**：`NOTDEC_EVM_INLINE` 不设 = `calldata`（只展开 calldata 链），`=0` 保留 outlined，
`=1`/`all` 走全模块代价模型。

这一轮的四项修复：

| 修复 | 内容 | 效果 |
| --- | --- | --- |
| 1485 matcher 扩展 | 认折叠后的 uint64-fit：`(2^64-(C+1)) >= (2^256-C)` → 常量长度 C；分配证据接受常量 `roundUp(C+32)` | 三个内联副本全部恢复 `memory_allocation_bounds`，1485 的 inline 残余覆盖已删除 |
| 空 scope 漏洞 | 模块内没有 calldata reader 时此前直接 return（完全不 pin），代价模型把小 helper 全内联 | 现在照样把非 scope 的 helper 全部 noinline；source 的 `private_helper_call` / `named_internal_helper_call` 两例恢复 |
| 测试接线 | `notdec.evm.solidity_patterns` = 默认扁平 + `--inline`（下界 + 覆盖）；新增 `notdec.evm.solidity_patterns_outlined` = pin `=0` 精确比较 | 两个形状都在 CI 里 |
| 编译预算 | 按默认扁平口径重标定 `compile-budget.json` | 预算与产品形状一致 |

**新发现的后端 bug（source 套件，已 pin 到 outlined 待修）**：扁平后 entry 里出现 calldata 最小长度 guard，
pass 已经正确识别它（pre-TR IR 里有 `%calldata.checked = call ptr @notdec_evm_calldata_min_size(ptr %view, i256 68)`，
MinSize=68 正确），但后端没有用 marker 渲染，而是把原分支渲染成极性相反的
`require(msg.data.length + uint256((type(uint256).max - 3)) <= 63)`（等价于 `x <= 67`，应为 `x >= 68`）。
涉及 `abi_decode_result_public_entry_01`、`abi_decode_nested_result_public_entry_01`。
IR：`%available = add size, -4; %too.short = icmp sgt %available, 63; br %too.short, ok, revert`。
待修方向：后端改为渲染 `notdec_evm_calldata_min_size`（`BodyBuilder.cpp:1470-1500`）而不是重新渲染该分支，
或修正有符号比较在折叠取反后的极性。

**编译预算（默认扁平口径重标定）**：

| 指标 | outlined | 扁平（新预算） |
| --- | --- | --- |
| compile_failures | 0 | 0 |
| unresolved_value | 264 | 289 |
| condition_todo | 729 | 4647 |
| goto / real_jump / dangling | 424 / 343 / 69 | 5437 / 4604 / 501 |
| helper_functions / call_sites（floor） | 1339 / 1301 | 1271 / 2412 |

提升的理由写在 `compile-budget.json` 的 comment 里（扁平化把共享 guard 复制到每个调用点，structurer 更容易放弃），
下一步的 structurer 工作是把它压回去。
- 后端修复（已做）：`BodyBuilder` 的 require 折叠对 else 分支用错极性（`require(!cond)`），已改为 `require(cond)`；
  `EvmCalldataAccessPass` 把匹配到的最小长度 guard 改写成规范形式（`revert iff calldatasize < MinSize`），
  于是后端渲染出 `require(msg.data.length >= MinSize)`。source 套件不再 pin，3 个 expected 更新为
  `require(msg.data.length > 67)` / `>= 100`；两处提交：后端 `5355022`、顶层 `24fa8d25`。

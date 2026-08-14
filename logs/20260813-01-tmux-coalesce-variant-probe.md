# tmux coalesce 路径相关重复展开探针与两个意外发现

## 用户原始 prompt

> 之前推荐的优化方式是什么来着？再说一下……（按祖先 fold 上下文缓存的思路）
> 按这个试一试，看看是不是这种情况

## 背景

全量 tmux simplify 卡在一个 recvars≈670 的巨型 group 展开（go_calls 千万级）。
假设是：少数 (ty,pol) key 被高频重复展开、每次结果只有个位数变体，因此
treeMemo 升级成“按祖先 fold 上下文缓存”能把重复展开砍掉。先用探针验证，不
直接改缓存。

## 探针（binarysub，未提交通用前仅诊断）

- `TypeSimplifier::coalesceCompactType`（`src/binarysub.cpp:4286` 起）新增
  `NOTDEC_COALESCE_VARIANT_PROBE=1` 门控的 per-key 统计：每个
  `(CompactType*, pol)` key 的 `Expansions`（真正展开数）、
  `PathHitExpansions`（子树命中路径）、`InProcessHits`、`TreeMemoHits`、
  `SharedHits`，以及结果 `utypeNodeHash` 的去重变体集合（上限 32）。
- 每 20 万 go 条目打一次 `[coalesce-probe:periodic]`，group 结束打
  `[coalesce-probe:final]`；小 group（展开 <1000 且 key <1000）不打印。
- dump 用单个 `ostringstream` 一次写 stderr，避免 8 线程 `<<` 交错打碎行。
- 默认关闭，零开销。

## 实验与数据

输入 `/tmp/tmux-slices/slice1275.bc`（1064 defs，复现爆炸），8 线程。

爆炸 group 的特征（periodic dump 趋势）：

| 指标 | 值 |
| --- | --- |
| go_calls（进入次数） | 1.6M（仍在涨） |
| 真正展开次数 | ~44k |
| 展开中 pathhit 占比 | 98-99% |
| keys 数 | ~2200 |
| multi_expansion_keys（展开>1） | ~480（22%） |
| multi_variant_keys（变体>1） | ~370（17%） |
| top key 展开次数 / 变体数 | 4328 / 1；4174 / 10；2799 / 2 |

结论：

1. “少数 key 高频重复、变体个位数”基本成立，但不完全和 go1 的 286+get 一样：
   top key 确实重复数千次且变体只有 1-10 个，祖先上下文缓存能把 4328 次重复
   压到 1-10 次；但 17% 的 key 有多变体，不是“几乎全部单变体”。
2. 更重要的结构事实：1.6M 次 go 进入里只有 ~44k 次是真正展开，97% 是
   treeMemo/inProcess 命中——现有 memo 已经吃掉了大头；剩下的 44k 全是
   pathhit（路径相关），正好是 treeMemo 不敢缓存的那批。
3. 卡住的代价分两层：44k 次路径相关展开各自重建整棵子树（深 record +
   PtrLoad/PtrStore 链），每次都要哈希、建临时 record、查重再丢；另外同一时刻
   其它 group 的 analyzeOccurrences（35.6%）+ SimpleTypeStableIdentityLess
   （30.3%）也在烧 CPU。memo 缓存能砍重复展开，但砍不掉单次展开的哈希税。

## 意外发现 1：realloc/strdup 系多态标记消掉了 tmux 的巨型 group

把 xrealloc/xreallocarray/xrecallocarray/xstrdup 纳入 malloc wrapper 白名单
（上一轮 e650bcc5）后，slice1275 从“巨型 group 卡 13+ 分钟”变成 **90 秒跑完、
没有大 group**（两次复现）。原因：这几个 wrapper 此前把全程序的字符串/缓冲区
调用点强制统一到一个函数类型上，形成 700+ recvars 的巨型互递归类型；按
callsite 实例化后不再全局统一，类型图大幅变小。这是 tmux 时间爆炸的更直接
解法，优先级可能高于祖先上下文缓存。

## 意外发现 2：确定性 μ 命名的重绑定断言（~50% 竞态）

白名单扩展后 slice1275 在 TypeBuilder 崩溃：

```
TypeBuilder.cpp:1021 convertRecursive: Assertion
`Recursive type name rebound to a different binder' failed
```

4 次运行崩 2 次。根因：确定性 μ 命名（`μ<structuralHash>`）+ 并行 share_mu
让两个不同 binder 拿到同一个 μ 名（结构同构或路径相关 μ 绑定差异），
TypeBuilder 的 name→binder 表按名字 1:1 假设被打破。旧计数器命名（每个 μ
唯一）不会撞名。`NOTDEC_MU_DETERMINISTIC=0` 可绕过，但确定性命名是 share_mu
跨组共享的前提，不能一关了之。这个断言必须修，否则白名单扩展不能落地。

## 下一步

1. 修 TypeBuilder 重绑定断言：同名 binder 结构同构时允许共享（UType
   hash-cons 保证 body 指针相同），不同构才报错。
2. 修好后用白名单扩展版跑全量 tmux 确认是否真正“跑通”（不再有大 group）。
3. 若仍有卡点，再上祖先上下文缓存；探针数据支持按 (ty,pol,祖先上下文) 缓存。

## 追加：TypeBuilder 重绑定断言修复（2026-08-13 完成）

采用嵌套遮蔽而非“同构才共享”：

- `external/binarysub/src/TypeBuilder.cpp:1014-1043`
  `TypeBuilder::convertRecursive()`：把 `assert(名字不重复绑定)` 改成保存外层
  binder、用内层 binder 遮蔽 `RecursiveTypeNames[T.name]`，转换完内层 body 后
  恢复外层映射。
- 语义：内层同名 binder 的 body 里对 `μ<hash>` 的自引用必须绑定到内层 binder；
  遮蔽保证这一点，且不依赖两个 binder 是否结构同构，路径相关 μ 绑定差异也能
  正确处理。结构同构时遮蔽结果与共享外层 binder 结构一致，输出不变。

验证：

- slice1275（白名单扩展版、确定性 μ 默认开）：修复前 4 次崩 2 次；修复后
  无探针 4 次 + 带探针 2 次全部 exit=0，无 rebind。
- fortune / memcached merge-eval 指标与历史一致（fortune bad_unions=0 frag=8
  cov 2.85%；memcached bad_unions=1 frag=133），正常路径不受影响。

提交：binarysub `763c0fe`；顶层指针随本日志一起更新。

## 追加：祖先 fold 上下文缓存实现与全量结果（2026-08-13 第二轮）

### 实现（binarysub `src/binarysub.cpp`，coalesceCompactType）

按上一节的设计实现首版，默认开，`NOTDEC_DISABLE_PATH_MEMO=1` 关闭对照：

- 每个 `(ty, pol)` 存 `SubtreeKeys`（无 memo 时会遍历到的完整输入键，排除根
  自身，按 (节点,极性) 去重）+ 每个结果变体的 `FoldedKeys`（本次展开真正
  折叠到的祖先键）。
- 命中条件精确：所有 FoldedKeys 都在当前 inProcess 上，且当前 inProcess 落在
  SubtreeKeys 里的键都被折叠过。展开命中过内层 PathMemo 或跨组共享 memo 的
  变体不缓存（结果嵌入了未记录的外来 μ 绑定）。
- 折叠键按展开栈记录并向上合并（合并时按父级根键过滤，自引用折叠不参与父级
  有效性条件）。计数器 μ 命名模式下禁用（μ 名随调用顺序变化）。
- `collectExpansionInputKeys` 镜像 go 的遍历（record/function/ptr/var 的
  `fromOnlyVariable` 节点和 recVar bound 子树），多项式代价。

### 验证

- fortune/memcached merge-eval：指标与历史一致（bad_unions=0/1、frag=8/133），
  正常路径不受影响。
- slice1275（白名单版）缓存开/关各两次：输出 IR 字节一致；ValueTypes.txt 的
  开关差异（39404 行）与 run-to-run 噪声（39428/39406 行）同量级。
- 全量 tmux（白名单版 + 缓存开）首次完整跑通：**44:26，峰值 RSS 6.0GB**，
  eval coverage 3.27%、bad_unions=8、fragmented_nodes=967。8 个 bad_unions
  全是 L0 的 explicit/policy merge（不同 struct 被并），是 merge-policy 问题，
  无路径相关 μ 特征。
- 对照（缓存关）同时启动：同一输入形成 recvars=602 的 group，go_calls 冲到
  1420 万后卡住（缓存开时最大 group 只有 160 万）。缓存把大 group 的重复展开
  压了约 9 倍，是“跑通”的关键。该对照 run 在后台继续，完整 bad_unions A/B
  待它结束。

### 已知代价

- 大 group 里 PathMemo 查找/校验是新增热点（perf 一度 PolarCompactTypeHash
  33% + hashtable 18%）：每个 go 入口多一次 find，命中校验要遍历 inProcess。
  目前 44 分钟跑完可以接受，但校验可以再优化（比如按路径签名先做 O(1) 预筛，
  只在签名命中时才做 O(路径) 精确校验）。
- SubtreeKeys 按 key 存储抬高了峰值内存（6GB vs 之前的 3GB 级），可以只给
  真正反复查询的 key 收子树键。

## 追加：缓存关 A/B、两项优化、go1 栈溢出（2026-08-13 第三轮）

### 缓存开/关全量 A/B（都跑完了）

| 配置 | wall | 峰值 RSS | bad_unions | polluted | frag | coverage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 缓存开（43fd2cb） | 44:26 | 5.9GB | 8 | 8 | 967 | 3.268% |
| 缓存关 | 1:36:51 | 12.6GB | 9 | 9 | 951 | 3.268% |

bad_unions 对比：8 个完全相同，缓存关多 1 个（`cmd_list_print::arg0` vs
`load_cfg_from_buffer::arg3`，reverse_edge）。缓存没有引入任何新的错误合并，
coverage 完全一致。缓存把全量从 1.6 小时/12.6GB 压到 44 分钟/5.9GB。

### 两项优化（binarysub `src/binarysub.cpp`）

1. **校验按较小一侧迭代**：命中条件 `P ∩ SubtreeKeys == FoldedKeys` 改为两边
   成员关系逐元素比较，迭代 `min(|SubtreeKeys|, |inProcess|)`。热点小 key 的
   SubtreeKeys 通常只有几个键、路径可能很深，从 O(路径) 降到 O(几个键)。
2. **SubtreeKeys 惰性收集**：store 时不再收子树键，只在 key 被再次查询时才收
   一次；存了不再复用的 key 不占内存。

验证：fortune/memcached eval 指标不变；slice1275 缓存开/关 ValueTypes 差异
（39428 行）与 run-to-run 噪声同量级；全量 tmux 复跑 44:27 完成，bad_unions
8 个与 v1 完全一致（0 新增 0 消失），coverage 相同。perf 采样中 v1 大 group
阶段的 PolarCompactTypeHash 33% + 哈希表 18% 热点消失，变成 analyze 主导。

### 偶发 SIGSEGV：canonicalizeType go1 深递归栈溢出（与本次改动无关）

全量 tmux 有一次跑批在 `canonicalizeType` 的 go1 崩溃（383+ 层 go1/
transformChildren 递归，TBB worker 线程默认 8MB 栈被撑爆，栈里没有 coalesce
帧）。同输入换 `ulimit -s 65536` 后 44:27 完整跑通，确认是栈溢出而非本次
改动。这是约束求解 run-to-run 变体偶发触发的既有隐患，修复方向是把 go1
递归改成显式栈或给 TBB worker 配大栈。

## 追加：perf 归因与交集微优化（2026-08-14）

全量 tmux（缓存版）两个阶段 perf 采样一致：`analyzeOccurrences` 41% +
`SimpleTypeStableIdentityLess` 28% + PolarVar 哈希表 7% + `applySimplificationPlan`
5%。analyze 的共现交集是 CPU 主体。

微优化（binarysub）：

- `intersect_occurrence_var_lists`（`src/binarysub.cpp`）：双指针合并从
  `less(lhs,rhs)`+`less(rhs,lhs)` 两次比较改成一次三分比较，直接比较
  VariableState 的 (id, level)，语义与 `SimpleTypeStableIdentityLess` 一致。
- `SimpleTypeStableIdentityLess`（`include/binarysub/binarysub-core.h`）：去掉
  `std::tie`，直接比较 id/level。

验证：fortune/memcached eval 指标不变；全量 tmux 45:17 → **38:08**（约 -16%），
bad_unions=8 与优化前完全一致（0 新增 0 消失），coverage 相同。峰值 RSS
7.7-10.4GB 随 run-to-run 分组波动。

剩余大头仍是 analyze（~69% CPU）里的跨组重复：大量 group 共享同一批 recVar
bound 子树，却各自重新遍历和求交。下一步候选是把 recVar bound 闭包的共现
贡献按 (var, pol) 预计算一次，group 侧只遍历根 DAG 再合并贡献（交集满足
结合律/幂等，可精确拆分）；这是结构性改动，需单独设计验证。

## 追加：跨组 analyze 缓存实现与三路实验（2026-08-14 第二轮）

### 16 线程实验：负结果

`NOTDEC_BINARYSUB_THREADS=16` 全量：61 分钟仍在跑（峰值 RSS 12.4GB），被
杀掉。8 线程 38-45 分钟。说明 wall 时间受单线程 coalesce 大 group 关键路径
限制，加线程只增加并发内存，不缩短墙钟。保持 8 线程。

### 跨组 analyze 缓存（binarysub，默认开，`NOTDEC_DISABLE_ANALYZE_CACHE=1` 关闭）

设计（`buildBoundOccurrenceCache` + `analyzeOccurrences` 回放）：

- 每个 (递归变量, 极性) 预计算一次其 bound DAG 的共现贡献（只含 bound 自身
  节点的位置，不含嵌套递归变量的 bound），记录 `FirstRefs`（本 bound 内每个
  递归变量第一次出现的极性）。
- group 侧根遍历不再就地展开 bound，只记录首次出现的 (var, 极性)；根遍历后
  按首次出现顺序回放：合并预计算的 map，FirstRefs 里未处理的变量追加队列。
- 精确性：交集结合律/幂等 + “节点被先访问则其递归变量必已处理” + DFS 顺序
  一致，回放与原地展开等价（含 processedRecVars 的首次语义）。

验证：

- fortune/memcached eval 指标不变（bad_unions=0/1、frag=8/133）。
- 全量 tmux 第一次跑 16:27（之前 38:08），bad_unions=8 与上一轮逐条一致
  （0 新增 0 消失），coverage 相同；期间 CPU 从 ~700% 掉到 ~190%，analyze
  工作量基本消失。
- 关缓存对照同输入跑到 74 分钟（go_calls 580 万级的大组）被杀。注意 run 间
  分组差异大（缓存版 60 万级 go_calls），墙钟对比混有噪声，但 CPU 表现和
  perf 归因互相印证。
- 峰值内存 16.7GB（缓存 map + 分组波动），比无缓存高；时间收益约 2.3 倍，
  内存是代价。

### utype_pool 分片锁（binarysub `include/binarysub/binarysub.h`）

`make_utype_node` 原来全程持一把全局 `utype_pool_mutex`，8 线程 coalesce 时
所有节点构造串行化。改为按哈希分 64 片、每片独立锁，deque 增长单独用
`utype_pool_mutex`。同构节点哈希相同必落同片，去重语义不变；同片第二个线程
在分片锁下重扫桶命中新节点，不会重复 push。

验证：fortune/memcached eval 指标不变。全量 tmux 复跑赶上重分组离群样本
（coalesce 关键路径 48 分钟还在跑，CPU ~150%，与 t16/缓存对照的重分组同类），
单样本无法给出分片收益；分片只减少锁竞争，语义与哈希去重不变，保留。

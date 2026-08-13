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

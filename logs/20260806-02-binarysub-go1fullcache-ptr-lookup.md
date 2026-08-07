# binarysub go1FullCache lookup 改为指针返回

## 背景

memcached 切片定位（见 `20260731-03`）显示 286+get（286 切片 + process_get_command）
canonicalize 时间爆炸。perf 采样（sudo gdb attach 确认卡点 + perf record 热点）：
`TypeSimplifier::lookupGo1FullCache` self 占 **29.75%**，是最大热点。

根因：`lookupGo1FullCache` 按值返回 `std::optional<Go1FullCacheEntry>`，而
`Go1FullCacheEntry` 含 `missingKeys`（`std::vector<PolarCompactTypeKey>`，展开路径多时很大），
每次缓存命中都整体拷贝整个 vector，内存带宽饱和。

## 修改内容

### external/binarysub/include/binarysub/binarysub.h

- `TypeSimplifier::lookupGo1FullCache` 声明（~955 行）：`std::optional<Go1FullCacheEntry>`
  → `const Go1FullCacheEntry *`，注释说明指针有效期（见下）。

### external/binarysub/src/binarysub.cpp

- `TypeSimplifier::lookupGo1FullCache`（2235 行起）：TBB 分支 `return Accessor->second` →
  `return &Accessor->second`；非 TBB 分支 `return it->second` → `return &it->second`；
  未命中返回 `nullptr`。
- `canonicalizeType` go1 lambda 调用点（2861 行）：`if (auto cached = lookup...)` →
  `if (const auto *cached = lookup...)`，`cached->missingKeys` / `cached->result` 用法不变。

### 指针生命周期论证（语义不变）

- TBB 分支：`const_accessor` 在函数返回时释放读锁，但 `concurrent_hash_map` 节点地址
  在锁外稳定——canonicalize 期间只 insert（同 key 只存一次，`storeGo1FullCache` 仅
  Inserted 时赋值）、从不 erase，节点到 `TypeSimplifier::clear()` 才销毁。
- 非 TBB 分支：`std::unordered_map` 的 rehash 不使元素引用/指针失效。
- 调用点只读 `cached->result` / `cached->missingKeys`，无修改。

## 验证

- binarysub 单测（TBB 构建）全过；TSAN 报告数与基线同形态（std::thread 并发测试 0 报告，
  TBB 阶段为既有调度噪声，无 Go1FullCache 相关竞争）。
- 166 切片（`/tmp/slice-cum-assoc_init.bc`，1 线程 frozen，RelWithDebInfo）：
  `out.ll` md5 `568216bdec2f56c376007a5b5e8831cd`、`ValueTypes.txt` md5
  `2547a3241083f68513fd7a7bf2516134`，与改动前逐字节一致；instructions
  1947→1942 亿（-0.3%）。
- **286+get 切片仍 1500s 超时**：30% 拷贝只是放大器，真正的瓶颈是 go1 递归折叠路径
  （引用图环上的节点）每 root 重复展开的本身工作量，该优化不能解决爆炸，后续方向见
  `20260731-03` 的"下一步候选"（环检测 + 非环展开缓存）。

## 评分

- 实现效果：消除 29.75% 热点，语义不变；对 missingKeys 大的输入收益明显，但不足以
  解决 memcached 286+get 的爆炸。
- 复杂度：低（返回类型 + 调用点各一处）。
- 维护成本：低，生命周期注释已写明前提（canonicalize 期间不 erase）。
- 更优方案：none（本次就是针对该热点的最小改动）。

## 追加：SelfRef 过度拒绝修复（2026-08-06，已提交）

### 背景（fc 探针数据，286+get 切片 300s 内）

`lookupGo1FullCache` 命中校验失败原因分布（1.55 亿次 lookup）：

| 失败原因 | 次数 | 占比 |
|---|---|---|
| SelfRef（merged 树含自身） | 9889 万 | 63.7% |
| DepsInvalid（missingKeys 失效） | 763 万 | 4.9% |
| InputOverlap（输入与 active path 重叠） | 399 万 | 2.6% |
| AlreadyRecursive | 84 万 | 0.5% |
| 有效命中 | 4383 万 | 28.3% |

进一步统计：**SelfRef 拒绝 6419 万次中 6419 万次（99.99998%）是
`MergedCached->base == pty.first`**——`treeContainsNode` 的 `ty == target`
匹配了 base 自身。

### 根因

`transformChildren` 展开的是 merged base 的 **children**（record 字段等），
**从不重新展开 base 自身**。因此 `base == pty.first` 时展开中根本不会折叠
自身——缓存条目是路径无关的，却被 SelfRef 检查误拒，每个 root 都重新展开
（6419 万次重复）。只有 **children 引用自身**（base 的后代含 pty.first）才
会在展开中折叠自身，需要拒绝。

### 修改内容

### external/binarysub/src/binarysub.cpp（commit 182406a）

- `canonicalizeType` go1 的 SelfRef 检查（原 `treeContainsNode(MergedCached->base,
  pty.first, ...)`）：改为只遍历 base 的 record 字段 / function args+result /
  ptrLoad / ptrStore 子树查找 pty.first（base 自身不参与匹配）。
- 保留"自身 key 已在 recursive 表时缓存可复用"的豁免（freshVar 身份固定；
  该分支当前为防御性死代码——真正的 children 自引用展开从不 store 无折叠缓存）。

### 验证

- fc 探针：SelfRef 从 6419 万 → **1**，有效命中率从 28% → **88.2%**。
- 166 切片（`/tmp/slice-cum-assoc_init.bc`，1 线程 frozen，RelWithDebInfo）：
  `out.ll` md5 `568216bdec2f56c376007a5b5e8831cd`、`ValueTypes.txt` md5
  `2547a3241083f68513fd7a7bf2516134` 与修复前逐字节一致。
- binarysub 自测全过。
- **286+get 仍超时**：剩余 miss（DI 1850 万 + OV 966 万 + AR 203 万 ≈ 3000 万）
  全部是折叠展开（展开中会折叠，无折叠缓存天生覆盖不了），是下一步的主目标。

### 思路 A（折叠展开缓存）实验结论（已回退）

- 实现：probe 预测 DFS（按展开规则遍历 merged 树收集折叠集合）+ 按 (key,
  foldSet) 分条目缓存 + 展开后实际折叠对比才 store。
- 失败原因（数据）：probe 完整遍历成本 ≈ 展开成本（perf 热点 ~67%）；foldSet
  分化（fs3+ 为主，命中率仅 11.5%）；probe 与展开系统性不一致（mismatch
  315 万/341 万展开：normalizeDirectPointer 树差异、展开动态 inProcess 等）。
- 结论：折叠展开的 foldSet 展开前不可知（子帧递归折叠），probe 是唯一预测
  途径但成本不可接受。**后续方向**：类型图 SCC 预折叠（环上节点预折叠成
  递归变量，环外全部可缓存）或工程化 opaque 缓解。

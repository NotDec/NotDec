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

# binarysub canonicalize 折叠展开优化探索（回退记录）

## 目标

解决 memcached 286+get 切片（286 切片 + process_get_command）的 canonicalize 时间爆炸
（此前 >15min 永不完成，286 本身 93s 可完成）。定位见 `logs/20260731-03` 与
`logs/20260806-02`。

## 探索历程（按时间）

### 1. 探针：折叠展开的重复结构

420s 内"有折叠的展开"4600 万次，按 (CompactType, 极性) 分组仅 73 个 key（平均每 key
重复 63 万倍）。进一步（foldSet 实验）揭示更细结构：**45.7 万种折叠配置 × 平均 7.5 次
重复**——主成本是折叠配置的组合多样性（幂集），不是同配置重复。

### 2. SelfRef 过度拒绝修复（已提交 182406a）

go1FullCache 命中校验失败原因探针（300s，1.55 亿次 lookup）：SelfRef 拒绝 9889 万
（63.7%），其中 99.99998% 是 `merged base == pty.first`——`transformChildren` 只展开
base 的 children，从不重新展开 base 自身，base==pty 的条目路径无关却被误拒。
修复：SelfRef 只查 descendants 引用自身 + 自身已折叠豁免。命中率 28%→88%，
166 切片 md5 不变。

### 3. 思路 A：foldSet 预测缓存（已回退）

- 实现：probe 预测 DFS（按展开规则遍历 merged 树收集折叠集合）+ 按 (key, foldSet)
  分条目缓存 + 展开后实际折叠对比才 store。
- 结果：420s 仍超时。失败原因：probe 完整遍历成本 ≈ 展开成本（perf 热点 ~67%）；
  foldSet 分化（fs3+ 为主，命中率仅 11.5%）；probe 与展开系统性不一致（mismatch
  315 万/341 万展开，normalizeDirectPointer 树差异 + 展开动态 inProcess）。
- 结论：折叠展开的 foldSet 展开前不可知（子帧递归折叠），probe 是唯一预测途径但
  成本不可接受。

### 4. 环代表折叠（本次实验，已回退）

思路：折叠 key 统一为环的固定代表（ringRep），同环节点共享一个递归变量 → 折叠展开
的 foldSet 固定（至多 {rep}）→ 无需 probe 即可缓存。

实现：ringRep（懒 DFS 找环段 + memo，预算 512）、折叠分支 key=rep、finalize 认 rep、
onCycle 折叠展开缓存（foldSet={rep} + missingKeys 校验）。

**直接版**（无环等价性检查）：
- 286+get 首次跑通（6m31s，此前永不完成）；166/fortune md5 不变；输出通过 llvm-as。
- **binarysub 自测 def 9 类型破坏**（consume2）：递归列表变成
  `(('a -> int:32) -> int:32) as 'a`——record（strm）与 function（go）互引成环，
  折叠 key 统一后 bound 被 go 的展开覆盖。

**同类检查版**（只统一 record↔record / function↔function 同类环段）：
- 测试全过（仅 2 处等价表示变化更新 golden：递归绑定位置/层数）；166/fortune 不变。
- **286+get 1h6m 未完成**：memcached 环段被 bounds 边引入的异类节点污染 → 环全被
  排除 → 折叠缓存失效 → 爆炸（比无优化更慢）。

### 5. 根本结论

1. **折叠 key 统一（同环节点 → 同一递归变量）只在"等价环"上 sound**——"引用图环"
   ≠ "类型等价"：record↔function 互引（consume2）明显不等价，统一破坏类型；
   record↔record 不同结构互引也不一定等价。
2. **memcached 的爆炸环大概率异构**：286 切片（无 get）无需环统一即可完成；
   process_get_command 引入的环统一后 6m31s 跑通，但输出类型无法在 IR 层验证
   （ptr opaque），有类型被破坏的风险，无法确认 sound。
3. **同类检查（保守正确）在 memcached 上性能不可接受**（1h+）。
4. **预折叠（思路 B）同样面临异构环问题**（环上节点直接折叠成递归变量，异构环
   形状丢失）——风险高，暂不追。

## 最终状态

- binarysub 工作树回退到已提交的 `182406a`（3 个修复：PersistentSet 并发 `3e5f0b4`、
  指针返回 `c20e879`、SelfRef 过度拒绝 `182406a`）。
- 286+get 走 opaque 工程方案（process_get_command 等标 opaque_body，55s 完成，
  函数体黑盒但类型正确）。
- 折叠展开的组合爆炸（DI/OV/AR ≈ 3000 万次折叠展开）在 canonicalize 内仍无
  sound 且高效的解法。

## 遗留

- 用户全量 401 任务（opaque-full-get-profile，8-06 启动）已跑 20h+，仍在
  canonicalize 阶段，建议终止。
- 若后续再攻折叠展开：先需可靠验证"memcached 环的等价性"（如最小 IR 上对比
  opaque 版与展开版的 HType 输出），再决定是否值得实现等价环的统一。

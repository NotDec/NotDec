# 折叠展开爆炸的图特征探针（静态预检实验）

## 用户原始 prompt

> 深度思考：给定一个图，能否准确衡量最终是否会内存或时间爆炸？即当前复杂度爆炸的点，特征在哪里？比如能否找到一下图上的特征？其次，能否根据这些特征发现潜在的可以合并类型的节点？
>
> 对，按照这个试试吧

## 背景

memcached 286+get 切片在 canonicalize 折叠展开阶段时间爆炸（>15min 永不完成），
166/286 切片可完成。此前尝试（foldSet 缓存、环代表折叠）均失败。本次目标：
用图分析回答"爆炸能否从图上预测、特征是什么、能否反推可合并节点"。

## 目标

1. 在 canonicalize 之前 dump 类型引用图（SimpleType 层、CompactType 层各一版）。
2. 静态分析 SCC/路径计数/节点贡献，对比 166/286/286+get 的区分度。
3. 验证或否定"静态图可预测爆炸"的假设，给出可计算特征和合并候选。

## 探针实现（已提交）

- `external/binarysub/include/binarysub/binarysub.h`：`dumpTypeGraph()` 自由函数
  （SimpleType 层，roots 可达引用图，含极性传播）；`CompactGraphState` 进程级
  共享状态（genTypes 按 SCC 多次构造 TypeSimplifier，探针输出必须跨实例聚合）；
  `TypeSimplifier::setCompactGraphDump()/recordCompactGraph()/recordCompactFold()
  /recordCompactRoot()`。
- `external/binarysub/src/binarysub.cpp`：
  - `dumpTypeGraph()`（文件末尾）：SimpleType 层边表，`N/R/E` 行，内容摘要列
    （primitive name:size、record 字段名、function args 数、变量 size）。
  - `setCompactGraphDump()`：打开输出文件（首次）。
  - `recordCompactGraph()`：go1 构建 merged snapshot 后记录 CompactType 层
    结构边（不递归展开，成本多项式）。
  - `recordCompactFold()`：go1 折叠分支（`inProcess.count(pty)` 与
    `finalizeRecursiveVar` 返回 freshVar）记录折叠事件（`F` 行，独立去重）。
  - `recordCompactRoot()`：canonicalizeType 入口记录 root（`R` 行）。
  - go1 中 `storeGo1Cache` 后、两处折叠分支、canonicalizeType root 处挂接。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6026-6033`：`NOTDEC_DUMP_TYPE_GRAPH`
  与 `NOTDEC_DUMP_COMPACT_GRAPH` 环境变量控制两个探针，均在
  `bulkSimplifyDetailed` 之前/之中启用，不影响正常输出。
- `scripts/analyze-type-graph.py`：SimpleType 层分析（Tarjan SCC、变量闭包
  收缩、结构分区细化、路径计数、无缓存/带缓存展开模拟）。
- `scripts/analyze-compact-graph.py`：CompactType 层分析（折叠 key 数、折叠
  key 入度、SCC、路径计数）。

验证命令（全部 1 线程 frozen，`build/bin/notdec`）：

```bash
NOTDEC_BINARYSUB_THREADS=1 NOTDEC_SUMMARY_OVERRIDE=test/type-recovery/realworld/support/memcached.source.summary.json \
NOTDEC_DUMP_COMPACT_GRAPH=/tmp/cg2-166.txt ./build/bin/notdec /tmp/slice-cum-assoc_init.bc \
  -o /tmp/out.ll --tr-level=2 --merge-struct-ptr-load-store --gen-work-dir --fast-work-dir \
  --work-dir=/tmp/wd --frozen-tr-input-ir
```

## 实验结果

### SimpleType 层图（求解后、bounds 合并前）：无法区分爆炸

| 指标 | 166 | 286 | 286+get |
|---|---|---|---|
| 节点 | 32782 | 56204 | 56898 |
| 非平凡 SCC | 57 | 90 | 92 |
| 最大 SCC | 46 | 46 | 51 |
| 路径计数(log10) | 12.6 | 14.4 | 14.8 |

166（37s 完成）与 286+get（>15min）的图几乎一样大、环数接近，路径计数都在
10^12 量级——静态上界严重高估，无区分度。原因：**SimpleType 层没有结构
hash-cons**（同构结构是不同对象），爆炸的结构共享只在 bounds 合并后的
CompactType 层出现；变量闭包可达集几乎每变量唯一，收缩无效果。

### CompactType 层 merged 结构图：无环

| 指标 | 166 | 286 | 286+get |
|---|---|---|---|
| 结构节点 | 30115 | 48252 | 3553* |
| 结构边 | 6206 | 7976 | 2622* |
| root 数 | 25437 | 42997 | 892* |
| 折叠 key | 153 | 148 | 72* |

*286+get 只记录了前 892 个 root（canonicalize 卡在第 893 个 root）。

merged 结构层 **nontrivial SCC = 0**：递归引用被折叠机制吸收（折叠分支在
merged snapshot 构建前返回），静态结构图里看不到环。结构规模与爆炸无直接
对应（166 结构边 6206 > 286+get 的 2622）。

### 有效信号：折叠事件密度与单 root 爆发

| 指标 | 166（完成） | 286（尾部卡住） | 286+get（爆炸） |
|---|---|---|---|
| 折叠 key / root | 153/25437 = 0.60% | 148/42997 = 0.34% | 72/892 = 8.1% |

- 286+get 前 892 个 root 就产生 72 个折叠 key（密度 8.1%），166 全部 2.5 万
  root 才 153 个（0.6%）。折叠密度是三个样本中区分度最强的指标。
- **爆炸发生在单个 root 内部**：286+get 第 893 个 root 展开中折叠 key 从
  58 快速增至 72 后卡死（>20min 无进展）。折叠 key 在单 root 内的高速新增 =
  该 root 展开树里的互锁递归组合爆发。
- 折叠 key 的入度（被多少结构引用）166 与 286+get 接近（31 vs 29），单 key
  汇聚不是区分点。

## 结论

1. **静态图无法准确预测爆炸**：SimpleType 层无结构合并、CompactType merged
   层无环，两个静态图都不含爆炸信息。爆炸是"展开过程"的路径组合，必须靠
   动态信号（折叠密度、折叠 key 增长速率）或展开模拟。
2. **可计算特征**：折叠密度（前 N 个 root 的 F/R）、单 root 折叠 key 产生
   速率、折叠 key 入度（互锁汇聚点）。前两个是强风险指示，第三个用于找合并
   候选。
3. **合并候选 = 折叠 key 等价类**：折叠 key 就是递归结构，等价 key 统一可
   直接减少折叠组合（与 ringRep 同思路，但判定标准必须是结构等价）。当前
   粗签名（merged 层 detail）无法区分折叠 key（153 个全"等价"是假象），需要
   在 finalizeRecursiveVar 时记录 freshVar 的 bound 结构（探针扩展点）再做
   互模拟判定。
4. 探针附带价值：**爆炸 root 可定位**（R 行顺序 + F 增长），后续可对
   特定 root 做 opaque 或合并，不必全量黑盒。

## 评分

- 实现效果：7/10。两个探针 + 两个分析脚本拿到关键数据，修正了"静态图可
  预测"的假设，定位爆炸为单 root 折叠组合爆发；探针本身多项式成本。
- 复杂度：5/10。compact 探针挂在 go1 三处折叠/合并点，跨 SCC 聚合用进程级
  共享状态，理解需要知道 canonicalize 展开机制。
- 维护成本：4/10。两个环境变量、两套分析脚本；后续扩折叠 key bound 记录
  只需在 finalizeRecursiveVar 加一行。
- 更优方案：无。SimpleType 层探针可删（无效），compact 探针保留。

## 遗留

- 折叠 key 的精确等价类统计（互模拟）未做：需在 finalizeRecursiveVar 输出
  freshVar bound 树，重跑 286+get 前 1000 个 root 即可。
- 286 尾部卡住的原因未定位（折叠密度正常 0.34%，可能是非折叠热点，与 get
  不同源）。
- 166 在当前 build/bin（8-06 起）上 canonicalize 从 40s 退化到 >10min，
  未排查是否与探针 hook 相关（不带探针也超时，疑似版本退化或环境差异）。

## 追加：折叠 key bound 等价类量化（2026-08-08 第二轮）

在 `finalizeRecursiveVar` 设置 bound 后记录 `B <freshVar> <pol> <bound>` 行
（`recordCompactBound()`，输出 bound 树到 N/E 行），量化折叠 key 的结构等价类
与重复展开。修复两处探针 bug：`recordCompactBound` 持锁调 `emitCompactTree`
导致单线程死锁（futex_wait 卡死）；B 行与 N 行共享 seen 导致 F 行被抑制。

实测（166 完成 / 286+get 前 892 root）：

| 指标 | 166 | 286+get |
|---|---|---|
| 折叠 key（F 去重） | 153 | 74 |
| B 记录（折叠 key 重复终结次数） | 5819 | 62904 |
| bound 结构去重（hash-cons 后） | 180 | 88 |
| 单 key 最多重复终结 | 459 | 18435 |
| 单 key 最多不同 bound | 3 | 6 |
| 单 bound 的 key 占比 | 128/153 (84%) | 66/74 (89%) |

结论：

1. **折叠 key 之间几乎没有合并空间**：bound 结构数（180/88）≈ 折叠 key 数
   （153/74），跨 key 共享结构极少。"合并得够多"在折叠 key 层不成立。
2. **爆炸 = 单个折叠 key 的高频重复展开**：286+get 的 top key 被正常展开
   1.8 万次（B 记录 = finalizeRecursiveVar 调用数），166 top 仅 459 次。
3. **展开结果高度可复用**：66/74 的 key 全程只有 1 个 distinct bound，最多
   6 个变体。1.8 万次展开产生 ≤6 种结果——这是"折叠展开结果缓存"可行性的
   铁证。此前 foldSet 缓存失败是因为依赖键过细（45.7 万种），而实际结果
   多样性只有个位数，有效依赖远比 foldSet 小。
4. 方向修正：下一步不是"合并折叠 key"，而是**缓存折叠展开结果**（按
   (折叠 key, 结果变体/稳定依赖)），把 1.8 万次重复降到个位数展开 + 缓存
   命中。

## 追加：bound 变体差异根因分析（2026-08-09 第三轮）

探针扩展（`external/binarysub/src/binarysub.cpp` `emitCompactTree`）：
N 行加极性（`N <ptr> <pol> <kind> <detail>`），V 节点 detail 追加变量 id
列表（`vars=N id1 id2 ...`），使互模拟签名能区分变量引用。同步修
`scripts/analyze-compact-graph.py` 的 N 行解析（原 key 用 kind、E 行用
pol，两者不匹配）。

新增 `scripts/analyze-bound-variants.py`：B 行变体等价性（迭代签名）、
差异根因分类（kind/detail/v-ref/arity）、局部性（shared/diff 节点数）、
变体频率。数据：166 切片仅拿到前 1057 条 B 行（canonicalize 卡死），
286+get 拿到 61528/62904 条（接近完整）。

结果（286+get，8 个多变体 key，26 对变体）：

1. 变体两两结构不等价（0/26 等价对），但**差异节点全部是 V 与 '?'**
   （ptrLoad/ptrStore 包装），没有 R/P/F 结构差异——record/prim/function
   结构在所有变体间一致或共享（hash-cons 同 ptr）。
2. 差异形态两类：V 节点变量集不同（同位置 vars=15 vs vars=1，id 集合
   不同）与 arity（某节点子节点数不同，展开深度不同）。无 kind 差异、
   无纯极性差异。
3. 局部性：多数 pair 差异 8-31 节点；有 pair 300 节点只差 6 个；也有
   69 vs 3 节点的深浅差异（折叠位置路径相关：同一 key 的环在不同 root
   路径上出现在不同位置）。
4. 变体频率高度偏斜：主变体占 70-99.9%（top key 6 变体 2488 次：
   1809/511/103/49/15/1）。

结论：

1. 变体差异 = 展开路径相关的 V 变量集 + 展开深度，不是结构本质差异。
   折叠点输出（freshVar V 节点）全局确定，重复的是祖先路径的重新
   展开/merge。
2. 缓存策略候选（按风险排序）：
   - 变体级缓存：key = (折叠 key, 变体特征)。主变体 70-99.9% 命中，
     低频变体（1-49 次）走慢路径，成本可控。
   - 结构模板缓存：把 bound 的 V 节点当槽位，缓存结构 + 运行时填变量
     集；需确认 V 变量集差异是否影响 simplify 输出（recVars 的 bound
     会被展开参与化简，见 `TypeSimplifier` simplify 阶段）。
   - 预折叠（首次展开定 bound）：语义风险最大，未验证。
3. 用户"合并节点"直觉的对应：变体差异只在 V 变量集时，对差异 V 节点
   取变量并集可统一变体（over-merge 方向），但需先确认 sound。

遗留：

- 166 版本退化确认：不带探针也卡死（3 分钟 2.8GB RSS 100% CPU，
  timeout 900 未完成；此前 37s 完成）。与探针无关，需 bisect
  binarysub 8-06 后提交（候选 182406a/8b65571/3e5f0b4）。
- 286 尾部卡住未定位（折叠密度正常，疑似非折叠热点）。
- 低频变体（1-49 次）的来源与语义必要性未分析。

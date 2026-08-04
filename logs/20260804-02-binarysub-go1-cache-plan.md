# 用户原始 prompt

> go1这里确实不容易缓存啊，分析一下
>
> 把当前的思路写成一个新的log文件，然后尝试实现一下这个go1 cache逻辑试试吧，看看是否有效果

# 背景

memcached 全量 401 函数在 `canonicalizeType` 阶段卡住：55 分钟未离开该阶段，RSS 恒定
228 MiB（hash-cons 已消除 arena 膨胀），perf 热点是 `CompactTypeBuilder::append`（约 23%）、
`lookupGo0Cache`（约 8%）和 CompactVarSet 红黑树插入。原因：`canonicalizeType` 的 go1
（合并 bounds 并递归展开 CompactType）没有结果缓存，SCC0 从 248→344→533 个成员后，同一个
共享递归类型被越来越多的 root 重复展开，工作量近似超线性。

# 目标

给 go1 加结果缓存，让同一 `(CompactType, 极性)` 的完整展开只计算一次。期望：全量 401 的
canonicalize 时间大幅下降（内存不变或略降），166 等切片输出与现状逐字节一致。

# 技术路线

最初按"完整展开结果"缓存实现（key=`(CompactType*, pol)`，缓存 finalize 后的树），
实测 166 切片 wall 从 360s 降到 27.7s、峰值 RSS 从 7.6GB 降到 3.1GB，但 ValueTypes
输出与无缓存基线不一致（nocache 两次 run 逐字节相同，说明无缓存版本是确定的，差异是缓存
引入的）。用"每次 canonicalize 调用内清空缓存"的对照证明差异在单次调用内就会发生，且
per-root 缓存下结构一致、仅变量编号不同；共享缓存下出现实质差异（如
`get_stats::%call3.i94` 从 `sint:32` 变 `⊥`）。

根因：go1 的完整展开依赖调用者的 `inProcess` 路径。同一个 CompactType 节点 X 在 root A
里展开时，其后代 Z 不在路径上（完整展开）；在 root B 里展开时，Z 可能已经在 B 的路径上
（经更早的兄弟节点），应当折叠成递归变量。缓存命中直接返回 A 的完整展开，绕过了 B 路径
上的折叠检查，结果树就不同。plan 之前"展开结果与路径无关"的论证是错的。

修正方案：只缓存"bounds 合并后、子节点展开前"的中间内容（merged 快照）。merged 内容是
固定 bound 集合的纯函数，与路径无关，可以跨 root 共享；子节点展开（含路径相关折叠）每次
照常执行，语义与无缓存完全一致。这仍然命中最大的重复工作（bounds 合并 / `append` 热点）。

实现过程中又发现一个坑：不能直接缓存冻结的 CompactType 树。freeze 和 builder 内的
normalizeDirectPointer 会把 record + 直接指针折叠进 field 0，重建后独立的
ptrLoad/ptrStore 展开步骤就丢了（166 切片上表现为 `get_stats::%call3.i94` 从
`sint:32` 变 `⊥`）。对照实验：只缓存不含直接指针的 merged 树时输出结构正确
（91.8s/3.1GB），含直接指针的 merged 树被跨 root 复用则产生实质差异。最终缓存
`Go1MergedSnapshot`：`base`（vars/prims/size/record/function 的无直接指针节点）+
独立的 `ptrLoad`/`ptrStore`/`psize`，重建时 `append(base)` 再
`appendDirectPointer`，展开子节点集合与无缓存路径一致。

实现要点：`TypeSimplifier` 增加 `go1Cache`（`ConcurrentPolarCompactTypeMap<Go1MergedSnapshot>`）；
go1 在折叠检查之后查缓存，未命中时先合并 bounds、`exportSnapshot()` 导出快照再存入
（budget 提前返回的部分结果不缓存）；命中时从快照重建 builder 继续 transformChildren；
`clear()` 清空；统计 hit/miss/store 输出到 binarysub trace 便于验证命中率。

# 风险

- 语义等价性：单线程输出应与现状一致；若不一致，先定位是缓存污染（如缓存在 budget 分支
  或跨 simplifier 复用）还是算法假设错误，回退到"环检测 + 只缓存非环节点"的保守方案。
- 并行竞争：TBB 并行 canonicalize 下两个 root 同时算同一节点，先存者生效；行为与现有
  并行不确定性一致，不引入新的正确性问题。
- 性能：缓存查找本身有 hash + 锁开销（go0 缓存已占 8%），需要看命中率是否足够覆盖。

# 判断标准

1. `./build/binarysub` 与 `MLsubGeneratorTest` 全部通过。
2. 166 切片：merged 缓存版输出（ValueTypes/out.ll）与无缓存基线逐字节一致；wall 时间显著
   优于 360s（期望接近 28s），峰值 RSS 优于 7.6GB。
3. 全量 401：canonicalize 阶段能跑完或显著加速；若仍卡住，报告 go1 缓存命中率并分析
   剩余热点。

## 完整展开缓存（第二轮，已实现）

merged 快照缓存提交（6ed5bb6）后，按用户"也可以试试吧"继续试"无折叠完整展开缓存"：
对展开中没有任何折叠（`!anyChildFolding && result == adapted`）的 go1 帧缓存整棵结果树，
命中时跳过子树展开。

### 实现要点

- `TypeSimplifier` 新增 `go1FullCache`（`ConcurrentPolarCompactTypeMap<Go1FullCacheEntry>`），
  条目为 `{result, missingKeys}`：`missingKeys` 记录展开过程中对全局 `recursive` 表查询
  为"不存在"的所有 key。`recursive` 表在 canonicalize 期间只增不减，因此命中时逐个验证
  这些 key 仍不在表里，结果就与重新展开一致。
- 命中还需满足：顶层 key 不在 `recursive` 表（否则本帧 finalize 会折叠）、merged 输入树
  与当前 root 的 `inProcess` 活动路径无重叠（`overlapsActivePath`）、输入树不物理引用自身
  根（`treeContainsNode`，自引用 DAG 展开时折叠依赖表状态）。验证通过立即返回，**不能在
  验证后继续重算**：重算会向 `recursive` 表插入子树折叠创建的 key，污染后续 root 的展开。
- 存储用拷贝而非 move：`outMissing` 是调用方 `childMissing` 的引用，move 会清空它，父帧
  随后合并到的依赖集合为空，导致命中验证漏检（实测 mismatch 从 24189 次降到 4 次）。

### 调试结论

- 展开结果不仅依赖 `inProcess` 路径，还依赖全局 `recursive` 表对每个 descendant 的
  存在性；缓存复用必须记录并验证这些依赖（`missingKeys`）。
- ValueTypes.txt 与无缓存基线有 791 行差异（递归变量折叠位置/编号不同），但 **out.ll
  与基线逐字节一致**，差异是等价的递归节点选择，不影响最终产物。之前"merged 缓存逐字节
  一致"的验证实际上只在 out.ll 层面成立。

### 实测（166 切片，1 线程，无 jemalloc）

| 版本 | wall | 峰值 RSS | out.ll vs 无缓存基线 |
|---|---|---|---|
| 无 go1 缓存（nocache） | 361s | 7.6GB | - |
| merged 快照缓存（6ed5bb6） | 125.8s | 7.35GB | 一致 |
| merged + full cache（本轮） | 70.2s | 6.26GB | 一致 |

时间再降 44%，内存降 15%，输出 IR 一致。`TypeBuilderTest` 通过；
`ctest notdec.type_recovery.*` 的 3 个失败在 stash 前后一致，属既有失败。

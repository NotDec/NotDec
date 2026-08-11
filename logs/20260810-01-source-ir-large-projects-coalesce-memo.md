# Bench2 大项目（tmux/redis/lighttpd）scalability 验证与跨 root coalesce memo

## 用户原始 prompt

> 继续跑Bench2里面其他项目吧，看看是否scalability足够，大的binary也能跑出来
>
> 对呀 我的意思就是先根据项目大小或者 binary 的大小补一些，比 memcached 大一些的源码级 IR，然后再跑一下 看看有没有效率问题，同时也看一下基于 debug info 的类型推理评估

## 背景

memcached 折叠展开爆炸修复（20260808-02）后，需要验证比 memcached（8MB IR）
更大的源码级 IR：新构建 tmux（40MB）、redis-server（52MB）、lighttpd（12.7MB）。
跑 tr-level=2 + merge-struct-ptr-load-store + DebugInfo 合并评估。

## 新构建的源码级 IR（/tmp/bench2-src/）

| 项目 | IR | 来源 |
|---|---|---|
| tmux | 40MB | tmux 3.4 Ubuntu 24.04 源码 -O0 -g |
| redis-server | 52MB | redis 7.0.15 源码 -O0 -g |
| lighttpd | 12.7MB | lighttpd 1.4.74 源码 -O0 -g |

## 问题与修复

### 1. MLsubGenerator.cpp 四个修复（约束生成层）

1. `addAddConstraint`/`addSubConstraint`：PNI 规则表只覆盖两侧都是
   PN related（i/I/p/P）的情况，原条件 `||` 导致一侧 NotPN 时误入规则表。
2. `visitExtractValueInst`：Ind==1 的 i1 断言仅限 overflow intrinsic；
   其他 aggregate 返回的 intrinsic（如 `llvm.modf.f64` 的 `{double,double}`）
   走通用路径。
3. `visitCallBase`：跳过 metadata 实参（`llvm.experimental.noalias.scope.decl`），
   避免 `convertSimpleTypeVal` 对 size=0 断言崩溃（lighttpd 首跑崩在这里）。
4. `applyStructPtrSlotMergeCandidates`：`break`→`continue`，一轮内合并所有
   候选，避免每合并一对就全量重收集（redis 之前卡 slot merge 36 分钟）。

### 2. binarysub.cpp：跨 root coalesce memo（本日志核心）

背景：simplify/coalesce 的折叠 bound 展开在 lighttpd 上 8 万 root 各自展开
一次，coalesce 合计 395s。曾试过两种方案：

- per-key 保活 simplifier + Group 并行 coalesce：memcached SAME 但 RSS
  1.26GB（基线 645MB）；lighttpd 8 万 root 全 unique，保活 8 万 simplifier
  → RSS 41GB 被杀。**撤销**。
- 最终方案：`bulkSimplifyDetailed` 改回 **per-root 流程**（每个 root 用
  自己的短命 TypeSimplifier，分析/简化/coalesce 完成后 arena 即释放），
  跨 root 去重放进 `coalesceCompactType` 的共享 memo：

- `CoalesceMemo`（每次 bulk 调用一个）：结构哈希 + 极性做 key，桶内用
  `*Entry.Ty == *Ty` 结构验证；条目里的 CompactType 通过
  `copyCompactTypeInto` 复制进 memo 自己的 arena（per-root arena 会释放，
  直接存指针会悬垂）。
- 保留 per-call `varMemo`/`treeMemo`（精确保持原输出文本），共享层只在
  per-call miss 时查；共享命中结果写回 per-call memo，避免本 call 内重复
  结构验证。
- **μ 安全门**：匿名递归变量按 call 内计数器命名，跨 call 复用会让两个
  不同结构撞到同一个 μ 名，所以只有 μ-free 的展开允许进共享层
  （`recVarCounter == 0` 时才 store）；μ 污染后本 call 退化为纯 per-call
  行为，文本与旧实现完全一致。
- `applySimplificationPlan` 的 `reconstruct` 加 (node, polarity) memo：
  60+ 层递归无 memo 时共享子图按引用路径重复重建（tmux 卡死点）。

### 3. TypeBuilder 转换阶段定位（遗留问题）

lighttpd 在类型恢复完成后卡在 per-SCC TypeBuilder 转换（>30 分钟、RSS
24GB、单核 100%）。该阶段在本次改动之前同样卡住（groups2 方案也停在同一
位置），与 per-root/共享 memo 改动无关。已加 `NOTDEC_CONVERT_PROGRESS`
进度打印（每 5000 个值一行）用于下次定位。

### 4. TypeBuilder 转换卡死：accessedPointeeSizeInBits 无 memo（本次修复）

perf 采样 lighttpd 卡死进程：~80% 时间在 `OffsetRange::fromStr`/strtoll +
`accessedPointeeSizeInBits`。根因：`accessedPointeeSizeInBits`（TypeBuilder.cpp）
是 UType 的纯函数但没有 memo，每次调用都重新走整个 record DAG 并逐个解析
字段偏移字符串；递归类型共享子图被每条引用路径重复走一遍（深度 30-40 层
record 链），单值转换指数膨胀。修复：按 `UTypePtr`（arena hash-consed，
指针稳定）加 per-instance memo，`TypeBuilder.h` 增加
`AccessedPointeeSizeMemo`。修复后 lighttpd SCC0 转换 10 万值 242s 完成。

### 5. redis 卡 slot merge：evidence 检查无 memo（本次修复）

redis 卡在 `applyStructPtrSlotMergeCandidates` 的候选收集里 45+ 分钟无输出
（旧代码 36-43 分钟同样卡此阶段）。perf：热点在 `hasStructPointerEvidence`
→ `hasStructPointerEvidenceInBound`（遍历 bound 的 TMemObject 字段、
`collectMaxDirectFieldAccessSizeBytes`、getAsPtrLoad/Store 的 primitive
registry 查找）。load×store 候选对是 B^2 的，每对都全量走 evidence。
修复：三个收集/合并函数（`collectStructPtrSameAccessKindMergeCandidates`、
`collectStructPtrLoadStoreMergeCandidates`、`applyStructPtrSlotMergeCandidates`）
加局部 `StructEvidenceMemo`（按解析后的变量指针 memo）；candidate 循环里
每次成功合并后清空（合并会改 bound 集合，清空保证语义不变）。

## 结果（8 线程，RelWithDebInfo，tr-level=2 + merge）

| 项目 | wall | RSS | 结果 |
|---|---|---|---|
| memcached | 2:16 | 646MB | 输出与基线 `/tmp/out-full-mp.ll` SAME，llvm-as OK，eval bad_unions=1（预存） |
| memcached（修复后复验） | 1:26 | 642MB | 输出仍 SAME，eval 与历史一致（bad_unions=1、fragmented 19/133） |
| lighttpd | 19:45 | 53.7GB | **完整跑通**（TypeBuilder memo 修复后），llvm-as OK；eval 见下 |
| redis | 重跑中 | - | 见下 |
| tmux | 重跑中 | - | 见下 |

### lighttpd merge-eval（/tmp/tr-lighttpd-eval-perroot3/merge-eval-summary.json）

- coverage：typed 920 / target 24285（3.79%）
- fragmentation：33 类型 / 304 节点
- wrong_merge：bad_unions=19、extra_types=12、polluted_components=10
- performance：wall 17.5 分钟、peak RSS 52.5GB（类型恢复段）

19 个 bad union 全部是 `policy_replace_bound`/`binarysub_merge`，首条 witness：
`chunkqueue_append_file_fd::%bb.if.then.i2`（struct:chunk*）与
`chunkqueue_append_file_fd::arg0`（struct:chunkqueue*）。CallSlotMergeDecisions.txt
显示根因是 `recursive merge crosses formal slots`：call-interface 事务合并把
chunkqueue 家族函数的 arg0（chunkqueue*）与 chunk 家族（chunk_acquire::ret、
chunk_push_oversized::arg0 等，chunk*）并到了一起——递归 SCC 里按槽位合并时
无法区分同位置的不同结构体类型。这是 merge policy 质量问题的真实样例，后续
单独处理（不是本次 scalability 验证的阻塞项）。

## 复验命令

- lighttpd：`NOTDEC_BINARYSUB_THREADS=8 NOTDEC_SIMPLIFY_DIAG=1
  NOTDEC_CONVERT_PROGRESS=1 ... /tmp/bench2-src/lighttpd.ll -o
  /tmp/out-lighttpd-perroot3.ll --tr-level=2 --merge-struct-ptr-load-store -g
  --fast-work-dir --work-dir=/tmp/tr-lighttpd-perroot3
  --merge-eval-dir=/tmp/tr-lighttpd-eval-perroot3 --frozen-tr-input-ir`
- memcached 回归：`... memcached.ll ... -o /tmp/out-mc-verify-evidence-memo.ll
  ...`，`cmp` 与 `/tmp/out-full-mp.ll` SAME，eval 与历史一致。

## 验证

- memcached：`cmp /tmp/out-mc-perroot.ll /tmp/out-full-mp.ll` SAME；
  `llvm-22.1.0.obj/bin/llvm-as` 通过；merge-eval-summary 与历史一致
  （coverage 1.77%、fragmented 19/133、bad_unions 1 预存）。
- lighttpd：simplify-diag 显示 roots=80541、slowest_coalesce=538ms、
  total_coalesce=295780ms（旧 395815ms）；完整跑通，输出 IR llvm-as OK。

## 评分

- 实现效果：待补（redis/tmux 跑完再评）。
- 复杂度：待补。
- 维护成本：待补。
- 更优方案：待补。

## 补充（20260811）：redis slot-merge 精确失效 + IR 持久化

### redis slot-merge 仍卡：单调失效只省了收集侧 evidence

上一版修复把 slot-merge 循环的 `HasStructEvidence` 换成局部 memo + 单调失效
（merge 后只失效 Into 和 false 集合）。memcached 回归 SAME，但 redis 实测
（groups5，带 NOTDEC_SLOT_MERGE_DIAG）load-load 第一轮：
`collect_ms=605 candidates=326420` 后 `loop_ms=459890`（7.7 分钟），随后几轮
11-19 秒。**瓶颈不在候选收集，而在 tryMerge 的 merge_variable_into 传播**：
传播中 constrain 触发 `shouldMergeSameFunctionStructPtrSubtype` hook，hook 内部
调用 `hasStructPointerEvidence`（遍历 bound + collectMaxDirectFieldAccessSizeBytes，
perf 采样热点），**该调用没有 memo**，32 万候选 × 传播 × evidence = 卡死。

### 修复：hasStructPointerEvidence 成员级 memo + 精确失效

- `ConstraintsGenerator` 增加 `mutable StructEvidenceMemo`，`hasStructPointerEvidence`
  先查 memo（MLsubGenerator.h 头文件、MLsubGenerator.cpp 实现）。
- 失效点收敛到 configureConstraintContext 的回调：
  - `onVariableMerged`：erase(Event.from)、erase(Event.into)
  - `onVariableNonVarBoundAdded`：erase(Var)
  - `onVariableNestedBoundRewritten`：erase(User)
- 正确性依据：evidence 只依赖变量的 ptrLoad/ptrStore 非变量 bound；merge 只把
  这些 bound 从 From 转移给 Into（remove_neighbor_references 只移除变量引用，
  不影响 ptrLoad/ptrStore evidence），**其他变量的 evidence 不随 merge 变化**。
  因此只需失效 (From, Into)，不需要旧实现的"每合并一对清空全部 false 缓存"。
- 三个 collect 函数和 applyStructPtrSlotMergeCandidates 的局部 memo 统一改为
  直接走成员 memo（局部与成员语义等价，删除重复实现）。

### X64 源码 IR 断言崩溃：ensureSequence 常量在左

新构建的 redis-server.ll（上游 7.0.15，-O0 -g）在约束生成里
`visitAnd/visitOr` 的 ensureSequence 断言崩溃：X64 分支（PassManager.cpp
build_passes）不跑 buildFunctionOptimizations（InstCombine），-O0 下
and/or 的常量操作数保持源码顺序可能在左侧。修复：and/or 可交换，
ensureSequence 改为静默 swap（不再断言），visitAnd/visitOr 的常量分支
依赖 Src2 是常量，swap 后语义不变。

### IR 重建与持久化（/tmp 被清理后的教训）

机器重启后 /tmp 全清，昨天构建的 tmux/redis/lighttpd IR 与全部实验结果
丢失。重新构建并**持久保存到 Bench2 source-ir 目录**：

| 项目 | 版本 | 路径 | 大小 | 构建 |
|---|---|---|---|---|
| redis-server | 7.0.15 上游 | `ir/redis-server/redis-server.ll` | 52MB | wllvm -O0 -g，LLVM 22，MALLOC=libc，deps 一并 wllvm 构建 |
| tmux | 3.4 上游 | `ir/tmux/tmux.ll` | 43MB | autogen + wllvm -O0 -g，libevent/ncurses 外部声明 |
| lighttpd | 1.4.82（submit-HOWARD 源码） | `ir/lighttpd/lighttpd.ll` | 14MB | autotools + wllvm -O0 -g |

构建要点：`PATH=llvm-22.1.0.obj/bin:$PATH LLVM_COMPILER=clang CC=wllvm
CFLAGS="-O0 -g"`；`extract-bc` 后 `llvm-dis` 存 .ll，llvm-as 验证通过。
后续所有 type-recovery 运行结果（输出 IR、merge-eval-summary、日志）统一存
`/sn640/NotDec-Exp/Bench2/type-recovery-runs/<date>-<project>-<tag>/`。

### 待验证（运行中）

- redis-evmono3：精确失效后 load-load 第一轮 loop 应从 7.7 分钟降到秒级。
- tmux-stats2：NOTDEC_DUMP_CLOSURE_STATS 闭包统计（每次闭包 BFS 的大小 +
  内容哈希重复度），判断 tmux SCC0 bulkSimplify 卡死是"同一闭包重复计算"
  （按 SCC/内容共享修复）还是"闭包本身巨大且各异"（需分片集合/输出共享）。

### 补充（20260811 晚）：vars 归并、toString 修复、tmux/lighttpd 现状

**已确认的结果**

1. **闭包统计推翻旧假设**：`calls=43026 avg_size=16 max=3927`，闭包 BFS
   不是 tmux 瓶颈（"巨型闭包重复计算"假设错误）。
2. **redis 卡死根因 = toString(递归 bound)**：`rebuild-rec-var`/`merge-rec-bound`
   在 trace 开关检查**之前**无条件 `toString(*bound)`，递归类型展开失控。
   修复：`toString` 深度限制（>8 打印 `...`）+ 两处 trace detail 惰性化。
3. **tmux 真瓶颈 = bulkSimplify 内存**：canonicalize 的 vars 红黑树批量插入
   是 CPU 热点（68% `_M_get_insert_unique_pos`）→ `mergeInsertVars` 线性归并
   修复（语义等价）。但内存仍爆：8 线程峰值 59GB、4 线程 56GB（机器 62GB
   OOM 边缘），4 线程 2.5GB→27GB→56GB 单调上涨，说明是单 group 累积。
4. **lighttpd 同样内存爆**：8 分钟 48GB、13 分钟 55.7GB 被停。带统计重跑：
   canonicalize 完成仅 1.2GB、`compact_map=119773`（12 万 root）、origin 表
   很小（374 变量/141509 元素）→ **大头在 simplify/coalesce 阶段**
   （12 万 group 的 co-occurrence 交集 + 跨 group coalesce memo + uMap 累积）。
   lighttpd 峰值与 tmux 几乎相同（~56GB），说明瓶颈同一机制，与项目大小
   无关。下一步：用 `NOTDEC_BULK_MEM_STATS` 已加的 `[bulk-mem] after-simplify`
   输出确认 simplify 后 RSS，再决定是限制 memo 保留量还是先做 root 合并。
5. **PNDiff union-by-size 尝试失败**：`mergePNINodes` 交换 To/From 后
   `mergePNVarTo` erase 了调用方 this 节点，M 残留悬垂，unify 入口
   `formatPNINodeForTrace(*this)` 读已释放节点崩溃（redis/tmux 同栈）。
   已回退（stash 保留，不提交）。PNDiff 的 DSUMap::merge 仍是 18% 热点，
   后续用"不 erase 节点 + 延迟清理"或调用方级去重再试。

**代码提交（external/binarysub，3 个）**

- `8d29998 [perf]`：CoalesceMemo/ReconstructMemo/mergeInsertVars/pointee-size memo
- `ec92c7e [fix]`：toString 深度保护 + trace detail 惰性
- `0f6324a [diag]`：closure stats + bulk mem stats（NOTDEC_DUMP_CLOSURE_STATS /
  NOTDEC_BULK_MEM_STATS）

**顶层未提交（下次提交）**：MLsubGenerator 的 StructEvidenceMemo 精确失效、
ensureSequence/matchPNDiff 常量在左 swap、call-slot merge ValuesByRoot 索引。

# redis-server 源码级 IR：多态名单补齐、约束传播去重 O(1) 化、go1FullCache UAF 修复

## 用户原始 prompt

> 沿着最近的优化日志的路线，继续跑tmux之后的下一个稍大一点的binary对应的源码级IR看看是否出现时间或内存爆炸并尝试分析原因
> 思考一下，为什么时间这么久，内存涨的也不快，到底卡在哪，前面单核的流程是否也应该考虑并行？内存涨得不快的话，很可能是有一些可以提升效率的冗余计算的点？
> 依次尝试，看是否有提升吧，看是否提升可以拿之前的一些已经能跑出来的小的例子
> 后面也定位分析一下这个崩溃问题怎么解决吧

## 背景与目标

tmux 全量（40MB IR）已通过一系列优化跑通（44min→20min 级）。下一个更大目标是
redis-server（57MB IR，3870 函数，Bench2 里最大的源码级 IR）。目标：跑通全量、
确认是否有时间/内存爆炸、分析根因；顺手验证约束求解阶段的冗余计算假设。

## 关键发现

### 1. 多态名单缺失是第一个坑

redis 全量用默认名单直接跑，约束生成阶段 40+ 分钟未完成（单核）。查
`MallocWrappers.txt`：`No generic malloc wrappers detected`——redis 的 zmalloc 系列
没被识别。原因：`getGenericMallocWrapperAllocator` 自动检测只认 libc 6 个 malloc 名
字且拒绝"malloc 结果流入 malloc_usable_size + atomicrmw 计数 + OOM 分支"的形状。

修复（`src/TypeRecovery/mlsub/MLsubGenerator.cpp`）：
- `isBuiltinPolymorphicBufferFunctionName` 名单加 redis z* 分配系列
  （zmalloc/zcalloc/zrealloc/zstrdup/zfree 及 usable/try 变体）。
- `markBuiltinPolymorphicBufferFunctions` 放宽到名单内**有定义的**函数（原只处理
  declaration；zmalloc 等是 dso_local 定义）。polymorphic SCC 只强制 summary
  边界，函数体约束照常。
- AGENTS.md 第 8 节第 2 步补强调：新项目必须先核对自定义 allocator 名字
  （zmalloc/xrealloc 等自动检测不到），再跑全量。

### 2. 约束生成收尾阶段"慢但内存不涨"的冗余计算

时间线（release）：02-mlsub-input.ll 写完（约束生成输入）后，deferred call
constraints + call-interface merge policy 的 merge 级联传播单核跑 40+ 分钟
（debug+ASan 无优化版），RSS 平稳 ~2GB。perf：`addUpperBound → hasBound` 25% +
`TypeRef::get` 9% + `HandlePtrUpperBound` 全扫。

三项优化（binarysub）：
1. **hasBound O(1) 化**：VariableState 加 lowerBoundSet/upperBoundSet
   （unordered_set<const TypeNode*>），add/remove/内联替换/事务回滚同步。
   TypeNode hash-cons，指针即结构，去重语义不变。
2. **HandlePtrUpperBound 增量配对**：加 upperPtrLoadBounds/upperPtrStoreBounds
   索引，新 bound 只与相反极性 Ptr bound 配对，不扫全 upperBounds。
3. 预检合并（has_bound_ref）：优化 1 后预检已 O(1)，无额外收益，未做。

验证：slice1275（setarch -R）baseline vs 优化后输出 IR 完全一致；fortune eval
一致（bad_unions=0, frag=8）；release 并行 1:14.72 vs 1:14.14（slice1275 太小，
merge 阶段收益测不出，需大输入验证）。

### 3. go1FullCache 并发 UAF 是"canonicalizeType 偶发崩溃"的根因

redis release 两次 SIGSEGV（12-14 分钟），core 栈都在 canonicalizeType →
CompactTypeBuilder::append → mergeInsertVars → PersistentSet::begin（或
PolarCompactTypeHash）。`ulimit -s 65536` 大栈无效（排除栈溢出）。

ASan 稳定复现（slice1275 debug+ASan 并行）后拿到分配/释放栈，根因：
`lookupGo1FullCache`（binarysub.cpp:2436）返回 `&Accessor->second` 裸指针，
TBB const_accessor 在函数返回时释放读锁；锁外长使用（missingKeys 遍历）与并发
`storeGo1FullCache` 的 `Accessor->second = value`（vector operator= 释放旧存储）
竞争 → UAF。20260806-01 的 PersistentSet 并发修复与日志里"canonicalizeType
偶发崩溃"是同一根因的不同表现。

修复：`go1FullCache` 值类型改 `shared_ptr<const Go1FullCacheEntry>`，lookup 锁内
拷贝快照（O(1)），store 只替换指针。验证：slice1275 debug+ASan 并行连跑两次通过
（修复前稳定 UAF），release 性能无损（1:14.72 vs 1:14.14），IR 一致。

## redis 全量现状（阻塞点）

UAF 修复后 redis 能过崩溃点，但 simplify 阶段 15 分钟起内存爆炸：
RSS 2.7GB → 57GB（60 秒采样，run4 手动终止避免 OOM）。这是 redis 自身的内存
爆炸（AGENTS.md 预判目标），与 UAF 无关。约束生成+merge 阶段修复后 release 约
8 分钟完成（对比 debug+ASan 无优化 40+ 分钟未完成）。

## 技术路线/风险

- 多态名单：只加 redis z* 系列，sds/hiredis hi_* 未动（后续看 bad_unions 再定）。
- 优化 1+2：逻辑等价重构，风险在 set/索引与 vector 的同步点（事务回滚、内联
  替换、merge clear 全测过）；正确性由 IR identical + eval 指标背书。
- UAF 修复：shared_ptr 快照，锁内只拷贝指针，读不阻塞写、写不阻塞读。

## 下一步

1. 提交当前成果（binarysub 优化+修复、顶层白名单+AGENTS.md、日志）。
2. jemalloc profile + perf 归因 redis simplify 内存爆炸（AGENTS.md 既有流程），
   定位爆炸阶段（canonicalize 大 group 展开 / coalesce / analyze 缓存内存）。

## 判断标准

- redis 全量跑通：时间、峰值 RSS、eval bad_unions/frag 与历史噪声带一致。
- 爆炸归因：明确是哪个阶段/结构（大 group go_calls 级、memo 保留量、bound
  闭包缓存）主导，给出可执行的减内存方向。

## 追加：Interner 内存优化 + PersistentSet 崩溃深挖（2026-08-15）

### redis 内存爆炸归因与 Interner 修复

jeprof（48GB 阈值终止的 profile run）：`bulkSimplifyDetailed:5371`（
buildBoundOccurrenceCache）分配 49GB live（95.4%）。perf：intersect 86% +
coOccurrences map operator[] 18%。

结构冗余：2×|recVars| 个 entry 各自独立分配交集结果列表，相同内容重复存储。
修复：`OccurrenceInterner`（内容 hash 池 + (lhs,rhs) 交集 memo，shared_mutex），
`OccurrenceInterner` 为 TypeSimplifier 成员，缓存构建与并行回放共享。
效果：redis 并行 RSS 峰值 57GB（被杀）→ 9-15GB 平稳；slice1275 IR 一致、
fortune eval 不变。binarysub da3a08a。

### PersistentSet 偶发崩溃分析（未根治）

现象：redis 并行 canonicalize 约 50% 概率 SIGSEGV，位置随机（PersistentSet
迭代 / go1 展开 / PolarCompactTypeHash 都见过）；单线程不崩；ASan 版 60 分钟
不触发；ulimit -s 65536 无效。

已排除/已修：
- go1FullCache UAF（f406641 修复）后仍崩 → 不是它。
- iterator 固定 128 深数组越界写（崩溃 core 里 depth 字段被写坏、root 是有效
  叶子——iterator 是受害者）：改为 128 inline + 堆增长 + 1M 深度防护
  （da3a08a）。修复后仍偶发崩（hash 版 15 分钟崩在 go1），说明损坏树的源头
  是 PersistentSet 并发节点生命周期（提前回收/UAF 残余）。
- jemalloc profile run 崩在 jemalloc 内部（native 版同样崩）→ 与 allocator 无关。

待查方向：PersistentSet refcount 在并发 canonicalize 下的不平衡路径
（insert 返回已有节点不 retain、recoverCreatedNodesLocked 时机、跨线程
Storage 拷贝）。

## 追加：崩溃根因确认（TBB 4MB 栈溢出）与 coalesce 第二爆炸（2026-08-15 第二轮）

### 崩溃根因：TBB worker 固定 4MB 栈

实测（pthread_getattr_np）：TBB worker 线程栈=4MB（硬编码，与 ulimit -s 无关；
主线程继承 rlimit 64MB）。canonicalizeType 的 go0/go1 路径相关展开深递归把
4MB 撑爆，栈帧落到相邻映射后随机二次崩溃。解释全部现象：位置随机、单线程
不崩（64MB 主线程）、ASan 不触发（栈溢出非堆 UAF）、ulimit -s 无效。

修复（binarysub ded4aba）：
- canonicalize 并行改用显式 pthread 线程池（128MB 栈，原子索引分派）替换
  tbb::parallel_for；analyze 保持 TBB（4MB 对该阶段足够，串行 canonicalize
  对照 run 2 小时无崩证明）。
- PersistentSet retain/recycle 竞态顺带修复（CAS 化）：release 归零后、
  锁内回收前，锁外 retain 把计数加回 1，recycle 仍无条件回收 → 节点复用
  UAF。CAS（RecycledMask 互斥）消除。

验证：redis 8 线程并行 133 分钟无崩（此前 12-15 分钟必崩），Interner 内存
14.9GB 稳定约 2 小时，然后进入 coalesce 大组展开。

### 第二个爆炸：coalesce 大组展开（recvars=169）

133 分钟后 simplify-expanding 显示 recvars=169 组 go_calls 520 万级，
RSS 14.9GB → 40.7GB 加速爬升（每 60s +5-10GB），手动终止。与 tmux
20260813 同款（大组路径相关展开的组合爆炸；tmux 靠祖先 fold 上下文缓存把
重复展开压 9 倍，redis 此组落在缓存未命中路径）。

perf 热点（爆炸前采样）：相等比较仍是大头（PersistentSet== 25% +
CompactType== 22% + std::_Rb_tree_increment 21% ≈ 70%）——hash 预筛后
剩余为 hash 相同（内容相同）的真实比较，需结构共享（hash-cons 加强）方向。

### 下一步

1. coalesce 大组展开的内存爆炸归因（jeprof 第二轮，定位展开中间结构）。
2. == 比较的结构共享增强（同构 CompactType/VarSet 共享节点 → O(1) 比较）。

## 追加：CompactVarSet 结构共享与 UAF 修复（2026-08-16）

### 结构共享（binarysub bf47c6f）

perf 显示 simplify 的 == 比较占 70%（PersistentSet== 25% + CompactType== 22% +
Rb_tree 21%）。根因：CompactVarSet 无 hash-cons，同内容集合每次创建都是新
Storage/root，比较 hash 相同后仍 O(n) 遍历。修复：ArenaState 拥有
unique_ptr<Storage> 的 hash-cons 表，同构共享一个 Storage，== 变 O(1) 指针
比较；CompactVarSet 持非拥有指针（与 CompactType 的 TypeRef 模式一致）。

### 由此暴露的 UAF 与修复

copyCompactTypeInto（CoalesceMemo 深拷贝）只复制 CompactType 对象，vars 的
Storage 裸指针仍指向源 per-group arena——group 结束后悬空（shared_ptr 模式靠
保活掩盖）。ASan 定位：freed 在 localSimplifier 析构（processGroup 尾部），
读在 consIndex 查重的 CompactVarSet::size()。修复：拷贝时 cloneVarSet 重建
vars 到目标 arena。

### 不合并 per-group arena（讨论结论）

per-group 是 8d29998 的内存有界设计（lighttpd 57GB→4.4GB，12 万 root 教训）；
CoalesceMemo 已处理跨组共享；合并成不释放的共享容器会退化为 per-root 爆炸。
UAF 用最小修复（cloneVarSet）解决，不动 per-group 结构。

### 验证

slice1275 ASan 无错 + IR identical（release 1:17）；fortune eval 一致
（bad_unions=0, frag=8）；redis 8 线程 55 分钟无崩溃、RSS 10.9GB 稳定。
redis 完整跑通（simplify 总时长 + 大组展开）待续。

# 用户原始 prompt

> 对，先改进一下这几个地方，看看结果怎么样

# 背景

完整 ffplay 在当前 Debug + ASan、8 线程配置下，关闭 merge-eval 和 workdir 后仍需约
114 秒。`perf` 显示调用接口事务约占 42%，结构体字段切片收集约占 45%，而
canonicalize 约占 11%。当前主要问题已经从 canonicalize 转到串行的调用接口 merge。

`shouldMergeSameFunctionStructPtrSubtype()` 会先完整构造两端字段切片，最后才检查节点
是否属于同一个函数；大量必然拒绝的候选因此做了无用扫描。`hasStructPointerEvidence()`
只需要布尔结果，却复用了完整布局收集。事务验证还会频繁解析 formal root，而
`resolve_variable()` 的路径压缩会把只读查询变成事务写入和整节点快照。

# 目标

不改变 subtype 约束、调用接口 merge 顺序、布局冲突定义和事务回退语义，只减少明显
重复或无效的工作。以完整 ffplay 核心路径 wall time 为主要判断标准，同时保持完整
DebugInfo merge-eval 的 `bad_unions=0`。

# 技术路线

先把 size、level、LLVM value 所属函数和 pointer-like 等便宜条件放到完整结构证据扫描
之前。为布尔证据查询增加遇到第一条有效字段访问就返回的遍历，完整字段切片仍只用于
真正的冲突比较和调试输出。

binarysub 在事务期间只沿 `mergedInto` 找 root，不做路径压缩；事务外仍保留路径压缩，
但父指针已经直接指向 root 时不再写回。这样实际 merge 仍由 journal 覆盖，只读验证不再
制造快照。

先用单测覆盖证据判断与事务 root 查询，再运行现有 MLsub、binarysub 和类型恢复回归。
最后用与基线相同的 ffplay 命令重跑核心 profile 和完整 merge-eval，对比时间、内存、
合并统计和评估结果。

# 风险和判断标准

- early return 必须和原来的“至少存在一个 offset >= 4、且有有效 direct load/store 宽度”
  判定完全一致，不能把只有 record field、没有真实访问的节点当成结构证据。
- 便宜条件只能调整检查顺序，不能改变任一条件本身。
- 事务内关闭路径压缩不能改变最终 representative；commit 和 rollback 后都必须解析到原来
  的 root，且只读解析不增加 `touchedNodeCount()`。
- ffplay 输出继续通过 LLVM 22 verifier，完整评估保持 `bad_unions=0`；若 fragmentation 或
  输出 IR 变化，则停止并排查，不能只接受速度提升。

# 实现记录（已完成）

## 修改

`src/TypeRecovery/mlsub/MLsubGenerator.cpp:2994-3021` 的
`shouldMergeSameFunctionStructPtrSubtype()` 先检查所属函数和 pointer-like，再扫描结构体证据；
`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3429-3460` 的 `hasStructPointerEvidence()` 改为找到第一条
有效字段访问就返回。布局冲突检查仍使用完整的 `collectOneLevelStructFieldSlices()`，冲突定义和 merge
顺序没有变化。

`external/binarysub/src/binarysub-core.cpp:105-123` 的 `resolve_variable()` 在事务内只查 root，不做路径
压缩；事务外仍压缩非直接 root 的父指针。这样验证阶段不会因为只读查询复制整节点状态。

`external/binarysub/src/binarysub-test.cpp:1203-1256` 增加事务内 root 查询不触碰节点的检查；
`unittests/Retypd/MLsubGeneratorTest.cpp:279-312` 增加结构证据边界测试，覆盖只有 record、offset 0 和
有效非零字段访问。

## ffplay 结果

8 线程、Debug + ASan 核心路径使用同一条 `perf record` 命令：

- wall time：`114.20s -> 34.11s`，减少 70.1%；峰值 RSS：`2576372 -> 2558012 KiB`。
- 输出 IR SHA256 均为 `d3f53f7a...d257f`，并通过 LLVM 22 verifier。
- `collectOneLevelStructFieldSlices()` inclusive 占比：`45.40% -> 0.26%`。
- `applyTransactionalCallSlotMergePolicy()` inclusive 占比：`42.34% -> 3.39%`。
- 事务 touched nodes 总数：`18062 -> 11592`，减少 35.8%；最大值：`518 -> 452`。

完整 DebugInfo 评估在
`/tmp/notdec-source-ffplay-profile-optimized-8t-20260728/`：

- `/usr/bin/time` wall：`350.80s -> 267.07s`；评估记录 wall：`343972 -> 261013 ms`。
- `bad_unions=0`、`polluted_components=0`，fragmentation 保持 `31 nodes / 19 types`。
- `merged_nodes=692`、`nodes_created=10398`、`representative_nodes=9706` 均不变。
- `bad_unions.jsonl` 和 `fragmented_types.jsonl` 与基线 SHA256 相同。

## 验证

- `./build/binarysub`：通过。
- `./build/bin/MLsubGeneratorTest`：14/14 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- SysY 仍是已有的 9 个 Clang 14 输入与旧 golden 差异；realworld fortune 仍由 frozen IR 的 extra
  constraint SHA256 锚点阻断，均发生在本策略前。

## 评分

- 实现效果：9/10。核心路径明显加速，完整评估结果和输出 IR 不变。
- 理解成本：9/10。只调整检查顺序和只读 root 查询，没有引入新状态。
- 维护成本：9/10。完整布局收集仍是唯一冲突判断入口；事务读写边界也更清楚。

当前核心路径的主要热点已变为 `canonicalizeType()`，inclusive 占比约 25%。继续优化前应单独 profile
其递归 compact type 构造，避免把本次低风险修改扩大到 canonicalize 语义。

# Simple-sub 多态层级边界修正与 Redis 实验

## 用户原始 prompt

> 阅读那边SimpleSub论文以及参考scala代码，如果严格按照那边来的话，具体是怎么做？另外，先不要急着考虑规模可能变大，运行时间甚至可能先变差，这个都没问题，先该出来跑实验试试吧，再讨论怎么处理

## 背景与判断

Simple-sub 的 `typeLetRhs` 在 `lvl + 1` 创建 RHS 类型变量，但保存
`PolymorphicType(lvl, rhsTy)`；实例化时复制严格高于 cutoff 的变量，即变量层级
`>= lvl + 1`。binarysub 的 `instantiate_type_graph` 已经使用 inclusive 的
`minimumGeneralizedLevel`：`level < minimum` 共享，`level >= minimum` 复制。

NotDec 的 `TData.level` 是目标 SCC RHS 变量的创建层级，因此不能把它再次作为
`PolymorphicType` cutoff 传入后得到 `TData.level + 1` 的 minimum；这样会漏掉恰好
处于 SCC 创建层级的变量。正确做法是直接以 `TData.level` 作为 inclusive minimum，
保持 binarysub 的 `MinimumGeneralizedLevel` 语义不变。

## 实现（已完成）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6730-6747` 的跨 SCC summary 实例化，
  删除 `TypeScheme(PolymorphicType(TData.level, ...))` 包装，改为直接调用
  `binarysub::instantiate_type_graph(TargetFTy, TargetLevel, Data.level)`。
- 同一处注释说明 Figure 8 的 `lvl + 1` RHS 创建层级、inclusive minimum 和
  NotDec SCC level 的对应关系。
- `external/binarysub` 未修改，`MinimumGeneralizedLevel` 仍保持
  `level >= minimum` 才复制的语义。

## 验证与实验

- LLVM IR cases `06`、`21`、`22` 在单线程、关闭 canonicalize 并行、关闭 ASLR 的
  新旧 A/B 中，`ValueTypes.txt`、`ValueHTypes.txt`、`ImportantHTypes.txt`、
  `VarOrigins.txt` 均逐字节一致。
- 强制多态 `16_Poly1.ll` 实验中，新实例化不再把调用点约束直接写回 callee 定义根；
  这是与 scheme 实例隔离一致的预期变化。
- Redis 新语义 checkpoint：
  `/tmp/notdec-redis-minlevel-rel-20260819/state`，IR SHA256 为
  `7fd3848686d4ae61996ad6a9e8eb94ca7e6aca8c2b088339beecb1f8b05b813d`，35 个 SCC、
  313,411 graph nodes、201,150 roots、252,955 values。bottom-up 阶段的主要耗时为
  `field_followup_candidate_collect=114.223 s`、`field_followup_candidate_apply=89.259 s`、
  `call_interface_transaction=136.117 s`；这些阶段仍然是单线程约束热点。
- 新语义 Redis 无 group 限制完整运行从该 checkpoint load：wall `19:16.83`，峰值
  RSS `20,129,704 KiB`，退出码 0；LLVM 22 verifier 通过。生成的 HType 依据文件为
  `/tmp/notdec-redis-minlevel-full-20260819/work/ValueTypes.txt`、
  `ValueHTypes.txt`、`ImportantHTypes.txt` 和 `VarOrigins.txt`。
- 新语义 simplify perf attach：`PersistentSet::iterator::pushLeft` 约 11.8%，
  `TypeSimplifier::applySimplificationPlan` 约 11.4%。TypeBuilder/lowering 阶段的
  热点转为 `appendVarOriginEntries` 15.3%、`collectUTypeVariableDetailsImpl`
  12.6%、`std::set<uint32_t>` 深拷贝 11.9%，说明输出诊断收集应与 simplify 本体
  分开优化。
- 同样输入和配置的旧语义完整运行 wall 为 `1:14:25`，峰值 RSS 为
  `37,388,236 KiB`，退出码 0，LLVM 22 verifier 通过。新语义约快 `3.86x`，
  wall 减少约 74%，峰值 RSS 减少约 46%。两边 `N=1` group 数分别为
  398,853 和 398,869，说明收益主要来自递归类型内容缩小，而不是 group 数量变化。
- 新旧输出 IR 的 SHA256 都是
  `13aedae316772478c2290c30673cc4791e22ed5f488c2ca6059424c73cd48093`，
  但类型推理结果仍按 HType 文件判断。新语义的 `ValueTypes.txt`、
  `ValueHTypes.txt`、`ImportantHTypes.txt` 分别约为 68.6 MB、60.7 MB、41.7 MB，
  旧语义分别约为 72.7 MB、67.1 MB、47.1 MB；`VarOrigins.txt` 则从旧语义的
  333.8 MB 增至新语义的 511.3 MB，需要作为诊断输出开销单独处理。
- Redis lowering normalization 中，新语义有 133 个 tail-recursive record、10 个
  recursive byte buffer 和 400,311 个 transparent record；旧语义分别为 154、51
  和 601,452。这支持旧边界会让调用点约束污染 callee 定义图，并放大巨型递归类型的判断。

## 后续

当前仍需解释 `TData.level == Data.level` 的跨 SCC summary edge。严格的 Simple-sub
let RHS 总会进入更高一层；NotDec 的 same-level edge 则可能来自 SCC 分组和多态 summary
boundary，是在 SCC ownership 上做隔离的扩展。后续先统计并说明这些边的来源，再决定
是否需要调整 level 或分组；本次先保留 inclusive 层级修正。allocator boundary、
`VarOrigins` 诊断输出和 `PersistentSet` 热点分别作为后续问题处理。

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

## 同层分组调整（已完成）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6736`：由于 `prepareSCC()` 现在会收缩所有
  同 level 调用边，跨 generator 的 summary edge 收紧为 `TData.level > Data.level`，
  与真正的多态 generalization boundary 一致。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:7391-7438`：恢复最大同层调用区域合并，
  不再因为 raw SCC 任一端带 polymorphic 标记而人为切断同层边。多态函数调用的普通
  helper 继承同一 RHS level，并和该多态函数处于同一 generator；只有进入新的多态
  raw SCC 才会升到更高 level。
- `resources/mlsub_builtin_summaries.json:49-57`：补齐 libc `calloc` 的全局 builtin
  summary，标记 `is_polymorphic`，返回值为通用指针，两个 size 参数为 number。此前
  `malloc/free/realloc` 已标记而 `calloc` 遗漏，Redis 中因此出现了不必要的同层
  `zcalloc -> calloc` 边。
- 临时三函数构图确认 `main L0 -> poly,ordinary_helper L1 -> calloc L2`，普通 helper
  与多态函数合并，`calloc` 独立进入下一层；LLVM 22 verifier 通过。
- `MLsubGeneratorTest` 19/19 通过。LLVM IR suite 的旧分组规则与新分组规则逐 case
  对照时 HType 输出完全一致；当前工作树 golden suite 的既有差异在两种规则下均存在，
  未更新 golden。

allocator boundary、`VarOrigins` 诊断输出和 `PersistentSet` 热点仍作为后续问题处理。

## 追加：Redis 复测与同层合并回归修复（2026-08-20/21）

### 用户原始 prompt（本轮）

> 最近做了一个新的改动，再次重新测试一下各个 binary 上的性能，如果 Redis 能够正常维持
> 较好的性能的话，就再继续跑下一个更大一点的 binary。……（后接多轮调查指令：
> 研究一下具体是什么原因；隔离试试；思考不用合并怎么处理同 level 的跨 SCC 边；
> 我们那个 opaque body 指定了之后，是不是应该移除对应的调用边，在划分 SCC 之前？）

### 复测结果：5e35de09 让 Redis 回归

- smoke 套件（fortune/ffplay/vsftpd/ngircd/memcached）与 lighttpd 用 HEAD build 无回归；
  lighttpd 65.41s/916MiB，bad_unions 20→19。
- Redis 全量（HEAD，21 SCC）：wall 40:50、峰值 RSS 24.0GiB（minlevel 基线 19:16/20.1GiB）。
  HType 产物比 minlevel 更小（合并更密），退出码 0、verifier 通过。
- 带 `NOTDEC_SIMPLIFY_DIAG` 复跑确认：SCC0（level-0 主 SCC）bulk 阶段 2091.9s，占全部
  SCC bulk 的 99.994%；尾部 SCC（含合并出的 SCC3=16 allocator）合计仅 ~131ms。

### 根因调查（checkpoint 图逐字节解析）

- 用 Python 解析 `simple-type-graph.bin`（uint32/uint64 小端布局），按变量 level 统计边：
  SCC0 的 L0 子图 head 比 minlevel 多 +9,265 lower、+70,614 upper 边（节点反而 -3），
  L1/L2 几乎不变。多出的边集中在 ~2,300 个 L0 递归 hub 变量（top 各 +1,000+ 条 lower 边），
  生成 recvars=75 巨型组。
- 隔离实验：
  - Variant B（去掉 calloc 的 is_polymorphic builtin summary）：图与 HEAD **逐字节一致**
    （只把 calloc 4 个节点从 L1 挪到 L2）→ calloc 配置不是原因。
  - 独立 C++ 工具（LLVM 22 CallGraph + 复刻 prepareSCC）：排除外部节点后 16 个 allocator
    是 16 个独立 raw SCC；取地址/间接边连到外部节点，本来就进不了 Tarjan → 去掉取地址边
    是 no-op。
  - 真正的粘合剂是**共享的 level-1 callee `malloc_usable_size`（30 调用者、10 个 polymorphic）
    和 `strlen`（110 调用者、1 个 polymorphic：zstrdup）**：16 个 allocator 都调用它们，
    同层合并把所有"同层且经调用边相连"的 SCC union 成一个 SCC3。
- 同层合并（5e35de09）本质：把同层跨 SCC 调用收进一个 generator（共享变量）。合并前
  poly->non-poly 同层边靠同层实例化处理，泛化 cutoff 不健全；合并后健全但 summary 变大
  （16 个 allocator 的并集），实例化进 L0 时图变稠密。

### 修复（src/TypeRecovery/mlsub/MLsubGenerator.cpp）

1. **回退同层合并**：prepareSCC Phase 2 恢复 polymorphic-SCC 同层边跳过（约 7538-7545 行，
   `IsPolymorphic || AutoMarkedPolymorphic` 任一端命中即不 Union）；bottomUpPhase 跨 SCC
   实例化的 assert 恢复 `TData.level >= Data.level`（6741 行）；GroupSuccs 的
   `<=` assert 恢复（7610 行）。
2. **opaque_body 出边在 SCC 划分前移除**（用户提议的根因修复）：Tarjan 访问时跳过
   opaque 函数的出边（7205-7211 行），`AG.Callee2Callers` 构建时跳过 opaque 调用者
   （7260-7262 行）。opaque 函数 body 不进约束图，出边本就不应影响 SCC 结构；这样
   malloc_usable_size/strlen 只剩真实（非 opaque）调用者，落到 level 0（monomorphic
   作用域），不再当粘合剂。
3. **auto-mark ≥1（两阶段 + warning）**：RawSCCInfo 增加 `AutoMarkedPolymorphic`
   （7299-7300 行）；Pass A 算基础 level（7407 行）→ Pass B 在基础层检测"非 polymorphic、
   ≥1 个同层 polymorphic 调用者"的 callee 并升一层、收集 warning（7422-7467 行）→
   Pass C 按最终 level 重新传播（7480-7490 行）。`NOTDEC_DISABLE_AUTO_POLY_GLUE=1`
   关闭。保留给非 opaque 的共享 helper（如 fortune 的 do_malloc -> exit/__fprintf_chk）；
   Redis 的 opaque allocator 场景因第 2 项已无粘合剂，auto-mark 不再触发。

### 验证

- 正确 IR（重新生成 stage-A 的 02-mlsub-input.ll；注意 `--emit-tr-input-ir` 输出的是
  00-lifted.ll 版本，不能当 stage-B 输入）上 Redis 干净全量：

  | 配置 | wall | 峰值 RSS | L0 lower |
  | --- | ---: | ---: | ---: |
  | head（旧 IR） | 40:50 | 24.0GiB | 1,517,847 |
  | 仅回退 | 33:31 | 23.5GiB | 1,516,602 |
  | 回退+auto-mark | 28:04 | 16.3GiB | 1,515,164 |
  | 回退+opaque 出边移除+auto-mark | 28:25 | 17.3GiB | 1,514,055 |

  三种修复配置的输出 IR **逐字节一致**（43b90abd…），verifier 通过；auto-mark 与
  opaque 出边移除都只改性能/内存，不改类型结果。minlevel 19:16 基线因当时的
  binary+IR 组合（未提交状态 + 已删除的 frozen IR，phi 前驱顺序差异）无法精确复现。
- smoke 套件与 lighttpd：全部通过，指标与历史基线一致（ngircd/memcached bad_unions=1
  等）；fortune 的 auto-mark warning 正常触发（exit/__fprintf_chk 升层）。
- `MLsubGeneratorTest` 未跑（本轮未涉及 binarysub）；后续跑批确认。

遗留：`VarOrigins` 诊断输出、`PersistentSet` 热点、以及同层合并回退后理论上的
poly->non-poly 同层边（opaque 场景已消除，非 opaque 场景由 auto-mark 兜底）仍待观察。

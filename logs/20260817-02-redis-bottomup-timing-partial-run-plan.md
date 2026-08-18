# Redis bottomUp 计时与部分运行计划

## 用户原始 prompt

> 先形成一个计划文档，首先是先做好这个“补 bottomUpPhase 的 SCC/约束阶段计时和约束计数，再加一个诊断专用的“跑到第 N 个 group 后退出”控制”，然后把当前的并行化优先级这里的几个思路记录一下。然后再开始实现补计时和退出控制以及验证收益

## 背景

Redis 的类型恢复有 35 个 SCC，其中 SCC0 包含绝大多数函数，并在 top-down
simplify 中产生约 39.9 万个 group。现有诊断已经能记录 `genTypes()` 和
`bulkSimplifyDetailed()` 的连续阶段 wall、进程 CPU、平均核数和 RSS，但
`bottomUpPhase()` 仍只有零散日志。约束遍历、`binarysub::constrain()` 的 worklist
传播、PNDiff solve、summary 实例化和 merge policy 因而混在一起，无法判断单线程
时间具体花在哪里。

完整 Redis 还会在主 SCC 之后处理 allocator 等小 SCC，并生成十几 GB 的类型调试
文件。每次等到全流程结束既慢，也会把主 simplify、后续类型转换和输出成本混在一起。
需要一个只用于性能诊断的固定 group 停止点，使相同输入、相同调度口径下的两个版本
可以在有限时间内比较进度、CPU 利用率和内存。

## 目标

第一阶段补齐 bottom-up 的可比较数据。每个 SCC 分开记录函数节点准备、函数体约束
生成、SCC 内 deferred call、PNDiff solve、各类 merge policy、override、跨 SCC
summary 实例化和 post-summary merge 的 wall、进程 CPU、平均核数、结束 RSS；同时
记录递归约束传播的工作量，而不是只看顶层 `addSubtype()` 调用次数。

第二阶段增加诊断专用的“处理到第 N 个 simplify group 后停止”。停止应发生在完整
group 边界，明确不生成部分 HType 或伪装成成功的反编译结果；日志必须保留已完成
group 数、总 group 数、阶段 wall/CPU/RSS 和已有 simplify 工作量计数，供限时 A/B
使用。普通运行不设置开关时，行为和输出不变。

第三阶段用固定 Redis stage-B IR 和 allocator 配置验证最新 substitution-chain 优化。
先用较小样例检查诊断开关和停止语义，再在 Redis 上比较相同 group 上限下的 wall、
CPU、平均核数和 RSS。部分运行只判断性能，不作为 HType、merge-eval 或输出 IR 的
正确性证据。

## 技术路线

### bottom-up 阶段计时

沿用现有阶段计时口径：`steady_clock` 记录 wall，`getrusage(RUSAGE_SELF)` 记录包含
工作线程的进程 CPU，`cpu / wall` 得到平均核数，`/proc/self/statm` 第二列记录
resident RSS。只在阶段边界取快照，不在每条约束上调用系统时钟。

`ConstraintsGenerator::run()` 返回 SCC 内部阶段统计；`bottomUpPhase()` 再补充
override、跨 SCC summary 实例化、post-summary deferred constraint 和 merge policy
的阶段，并输出每 SCC 记录与全 SCC 汇总。输出使用 SCC index、稳定 SCC 名、函数数、
节点数等字段，避免只能靠长函数名判断运行位置。

### 递归约束工作量

`addSubtype()` 每次进入 `binarysub::constrain()` 后，会在局部 constraint worklist 和
merge worklist 中继续传播。诊断计数放在这两个真实循环中，记录顶层 constrain 调用、
constraint task、merge task、新增上下界、direct variable edge fast path 命中和最大
worklist 深度。此前的重复 upper/lower bound 计数只表示失败的 bound-set 查找，不能
直接代表新增工作量，已从统计接口移除。计数通过可选诊断对象传入，不开启时只保留
一次空指针判断，不引入原子操作或全局锁。

阶段时间回答“哪一段慢”，worklist/merge/bound 计数回答“慢是因为工作量增大，还是
每个任务变贵”。两者结合 perf 才能判断后续优化方向。

### 固定 group 停止

停止上限放在 `bulkSimplifyDetailed()` 的 group 调度层。实现时采用更简单的严格前缀：
串行循环和 TBB `parallel_for` 都只覆盖 `[0, min(N, group_count))`，因此并行模式也准确
完成 N 个完整 group，不存在并发尾部。严格 A/B 仍要固定线程数、ASLR 和 canonicalize
设置，使 group 顺序稳定。

达到上限后，binarysub 返回一个明确的 diagnostic-stop 状态，而不是不完整的类型 map。
NotDec 在类型转换前识别该状态，打印最终统计并以专门的非零退出码结束。性能脚本把该
退出码视为“按计划停止”，但普通 CLI 和测试仍把其他非零状态视为失败。

## 并行化后续优先级

1. 优先测 `group_processing` 从 8 到 16、32、64、128、256 线程的 wall、平均核数和
   RSS。若平均核数随线程增长，但 wall 被单个 300 秒级 group 限制，下一步应处理
   group 内部关键路径，而不是继续增加外层线程。
2. 单独测 canonicalize 的线程扩展性。它使用大栈 pthread worker，线程数增加会同时
   增加栈地址空间和活动内存，不能只看 CPU 利用率。
3. `group_setup`、lower、upper、debug output 当前基本串行。只有 Redis 新计时显示其
   wall 占比明显后再并行，尤其要把十几 GB 调试输出和类型转换分开判断。
4. 约束生成不能直接把 `addSubtype()` 并发执行，因为它会共同修改 VariableState、
   bound 索引、merge 状态和 PNDiff 图。可行候选是并行收集每函数的只读约束事件，
   再按稳定顺序应用；是否值得做由本轮 visitor 与递归传播的时间占比决定。
5. merge policy 可先并行做只读候选收集，实际 merge 仍按确定顺序串行应用。只有
   candidate collection 成为主要成本时才实施。跨 SCC 同层并行对 Redis 的巨型
   SCC0 帮助有限，不作为首选。

## 风险

- 固定前缀能严格完成 N 个 group，但 group 顺序仍可能受输入顺序和 canonicalize 分组
  影响；日志必须同时记录实际完成数和总数。
- 部分运行没有完整类型 map，不能继续 lower/upper/debug output，也不能比较 HType。
- 进程 CPU 是全进程口径。后续若新增无关后台线程，需要重新解释平均核数。
- 约束计数若放入热循环可能影响性能；默认关闭，开启时也只做线程本地或单线程普通
  整数累加。
- Redis 的 group 顺序可能受 parallel canonicalize 影响。严格 A/B 与生产并行吞吐
  必须分开报告。
- 当前 binarysub 有已知 parsing sample 4 失败。本轮不能把它误判为新回归，也不能
  因此忽略新增测试中的其他失败。

## 判断标准

- 诊断关闭时，fortune/lighttpd 的 HType、merge-eval 和 LLVM verifier 与基线一致，
  且没有新增日志。
- 诊断开启时，每个 bottom-up SCC 都有完整阶段记录，汇总 wall/CPU 等于各 SCC 对应
  阶段之和；递归约束计数非零且与小型构造测试的预期一致。
- group 上限开启后只在完整 group 边界停止，输出明确的 diagnostic-stop 状态，不进入
  lower/upper/debug output，不留下可被误当成成功结果的输出 IR。
- 单线程和并行固定 group A/B 的实际完成 group 都必须准确等于 N。
- Redis A/B 使用同一 stage-B IR、allocator override、线程数、ASLR 和 group 上限，
  报告 latest 与 parent 的 wall、CPU、平均核数、RSS、工作量计数和 perf 热点变化。

## 实现记录（已完成）

### 代码

- `external/binarysub/include/binarysub/binarysub-core.h:593` 增加
  `ConstraintSolverStats`；`src/binarysub-core.cpp` 的 `constrain()` 和
  `constrain_worklist_only()` 统计真实 worklist、merge、bound 新增、direct variable
  edge fast path 命中和最大深度。重复 upper/lower bound 字段及其输出已删除。
- `external/binarysub/include/binarysub/binarysub.h:1115` 扩展
  `BulkSimplifyResult/Options`；`src/binarysub.cpp:5928` 的
  `bulkSimplifyDetailed()` 严格处理前 N 个 group，停止时返回空类型表和明确状态。
- `external/binarysub/src/binarysub-test.cpp:1271`、第 1309 行分别增加约束计数和串行/
  并行严格停止测试。binarysub 提交为 `a53bf55`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:78` 增加 12 个 bottom-up 阶段的
  统计结构和停止异常；`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3186` 的
  `ConstraintsGenerator::run()` 分段，`bottomUpPhase()`（第 6344 行）补 SCC 级和全局
  汇总，`genTypes()`（第 6564 行）在 lower 前处理停止状态。
- `src/NotDec.cpp:156` 校验 `NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS`，`main()` 第 431 行把
  诊断停止转换为退出码 75。Redis 的 SCC 名只保留前 80 字符，避免单行日志达到几十 KB。

实现与原计划有一处调整：并行停止不再允许 N 加 worker 数的尾部，而是用 TBB 的固定
range 准确处理前 N 个 group。这样代码更少，A/B 也不需要解释实际完成数差异。

### 验证

关键命令：

```bash
cmake --build build --target notdec TypeBuilderTest binarysub -j4
./build/bin/TypeBuilderTest
NOTDEC_CONSTRAINT_DIAG=1 NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS=1 \
  ./build/bin/notdec test/type-recovery/llvm-ir/cases/10_BottomUp1.ll \
  --tr-level=2 -o /tmp/notdec-stop.ll
```

- `cmake --build build --target notdec TypeBuilderTest binarysub -j4` 和
  RelWithDebInfo 同目标构建通过；最后的 SCC 日志截断修改也在两个构建目录重链通过。
- `TypeBuilderTest` 9/9 通过。新增的 `test_constraint_solver_stats` 和
  `test_bulk_simplify_diagnostic_stop` 单独通过。完整 `binarysub` 可执行程序仍先停在已有
  parsing sample 4 差异：期望 `⊤ -> ⊤ -> ⊤ -> {}`，实际
  `⊤ -> … -> … -> {}`，不是本轮新增失败。
- `10_BottomUp1.ll` 在诊断关闭/开启时，`ValueTypes.txt`、`VarOrigins.txt`、
  `ValueHTypes.txt`、`ImportantHTypes.txt` 逐字节一致，两个输出都通过 LLVM 22 verifier。
  `N=1` 时准确完成 `1/17` group、退出 75，不进入 lower/upper/debug、不生成输出 IR；
  非法值 `bad` 退出 1。最终停止日志最长行 426 字符。
- 前一轮 LLVM IR suite 为 14 通过、8 个 golden mismatch；本次拆分诊断上下文后重跑为
  13 通过、9 个 mismatch。抽查 `06_SimpleRecursive2`、`09_OffsetLoop`、
  `20_PointerAnalysisFieldCycle`，修改前后的 HType 文件逐字节一致；这些 mismatch 是
  当前分支已有的 golden 漂移，不能作为本次诊断改动的语义回归证据。

## Redis 有限运行

固定输入：

- stage-B IR：`/tmp/notdec-source-ir-perf-redis-current-20260817/redis-server/work/02-mlsub-input.ll`，
  SHA256 `82841a085e004edc07d7947ffef8d8d970fc63e65d9c08e9af40075ff6925cb5`。
- allocator override：`/tmp/redis-allocator-summary.json`，SHA256
  `064724c092f12608335451e6af1eaef608c2846c4fd448a14a26e634ab7a7ef9`。
- 关闭 ASLR，8 线程，开启 parallel canonicalize，`N=100`。本机只有 16 核、62 GiB
  内存，不代表目标 256 核、1 TB 机器的扩展性。

优化前是 `a8d818a` 加相同计时补丁，优化后是 `2d42f4a` 加相同计时补丁。两边都准确
完成 `100/399083` group、退出 75、没有输出 IR。

优化后运行命令如下；优化前只替换可执行文件为
`/tmp/notdec-redis-ab-parent-build/bin/notdec`：

```bash
setarch x86_64 -R env \
  NOTDEC_SUMMARY_OVERRIDE=/tmp/redis-allocator-summary.json \
  NOTDEC_BINARYSUB_THREADS=8 \
  NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1 \
  NOTDEC_CONSTRAINT_DIAG=1 \
  NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS=100 \
  /usr/bin/time -v ./build-relwithdebinfo-20260731/bin/notdec \
  /tmp/notdec-source-ir-perf-redis-current-20260817/redis-server/work/02-mlsub-input.ll \
  --tr-level=2 --frozen-tr-input-ir --merge-struct-ptr-load-store \
  -o /tmp/notdec-redis-partial/out.ll
```

| 指标 | 优化前 | 优化后 | 变化 |
| --- | ---: | ---: | ---: |
| canonicalize wall | 300.262 s | 274.628 s | -8.5% |
| canonicalize CPU / 平均核数 | 2252.680 s / 7.50 | 2059.553 s / 7.50 | CPU -8.6% |
| group setup wall | 127.183 s | 101.003 s | -20.6% |
| bulk wall | 429.679 s | 377.899 s | -12.1% |
| 整次 wall | 1106.74 s | 1036.46 s | -6.4% |
| 峰值 RSS | 7,745,816 KiB | 7,494,732 KiB | -3.2% |

`2d42f4a` 直接改的是 canonicalize 中的 substitution 查找，因此可以直接归因的是
canonicalize wall/CPU 下降约 8.5%。`group_setup` 和整体 wall 还包含 run-to-run 分配
顺序及缓存波动，不能把表中的全部下降都算成该优化收益。另一次带 perf 的优化后运行
canonicalize 为 255.839 s，方向一致，但不用于正式百分比。

### bottom-up 阶段

采用优化后干净运行。12 个阶段合计 613.351 s，全部约为 1 核：

| 阶段 | wall | bottom-up 占比 |
| --- | ---: | ---: |
| `function_nodes` | 0.046 s | <0.1% |
| `visitor_constraints` | 1.674 s | 0.3% |
| `scc_deferred_calls` | 149.889 s | 24.4% |
| `pndiff_solve` | 0.061 s | <0.1% |
| `call_interface_merge` | 193.936 s | 31.6% |
| `struct_slot_merge` | 133.736 s | 21.8% |
| `field_followup_merge` | 132.360 s | 21.6% |
| `overrides` | 0.031 s | <0.1% |
| `summary_instantiation` | 0.004 s | <0.1% |
| `post_summary_deferred_calls` | 1.043 s | 0.2% |
| `post_summary_call_interface_merge` | 0.100 s | <0.1% |
| `post_summary_field_followup_merge` | 0.471 s | 0.1% |

前四个重阶段合计 609.921 s，占 bottom-up 99.4%，占这次有限运行 58.9%。普通 visitor
只有 1.674 s，不值得优先并行。`scc_deferred_calls` 入队 8683 万个约束任务；三个主
merge 阶段又处理了约 2913 万个约束任务。旧诊断曾额外统计 upper/lower bound 失败查找，
但这些计数不等于新增 worklist 或完整传播，后续已删除；当前以 task 数、merge 数和
fast path 命中数作为更直接的工作量指标。

30 秒 perf 样本中，`resolve_variable` 占 23.8%，
`collectMaxDirectFieldAccessSizeBytes` 占 8.4%，约束去重哈希插入约 7%，
`constrain_worklist_only` 合计约 9%，PNDiff 分组相关约 10%。数据在
`/tmp/notdec-redis-partial-20260817-current-n100/perf-bottomup.data`。另一份采样进入了
canonicalize，`CompactTypeBuilder::mergeInsertVars` 占 52.1%，不能算作 bottom-up 热点。

### 诊断收尾与最新重盘点（20260818）

诊断上下文已从 `ConstraintsGenerator` 成员移到
`src/TypeRecovery/mlsub/MLsubGenerator.cpp:107` 的 thread-local 变量，避免开启诊断
改变核心对象布局和指针顺序。`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:93`
增加 deferred call、call interface、struct slot、field follow-up 的候选收集/应用
子阶段；`bottomUpPhase()`（当前约第 6500 行）分别输出主 SCC 子阶段和
post-summary 子阶段。小 IR 已验证两类日志分开，诊断停止仍在完整 group 边界返回 75。

Redis 最新一次固定 `N=100` 运行目录为
`/tmp/notdec-redis-diagnostic-20260818-BSDKr3`，输入、allocator override、ASLR、8
线程和上一轮一致，完成 `100/399083` 后退出 75。下面的阶段 wall 来自该次运行；平均
核数均约为 1.00，阶段 RSS 是阶段结束时的 RSS（不是阶段内峰值）：

| 阶段 | wall | 阶段结束 RSS |
| --- | ---: | ---: |
| `function_nodes` | 0.048 s | 0.44 GiB |
| `visitor_constraints` | 1.697 s | 0.65 GiB |
| `scc_deferred_calls` | 155.344 s | 0.93 GiB |
| `pndiff_solve` | 0.065 s | 0.93 GiB |
| `call_interface_merge` | 203.776 s | 1.23 GiB |
| `struct_slot_merge` | 137.977 s | 1.25 GiB |
| `field_followup_merge` | 135.432 s | 1.28 GiB |
| `overrides` | 0.031 s | 1.28 GiB |
| `summary_instantiation` | 0.004 s | 1.28 GiB |
| `post_summary_deferred_calls` | 1.051 s | 1.28 GiB |
| `post_summary_call_interface_merge` | 0.100 s | 1.30 GiB |
| `post_summary_field_followup_merge` | 0.487 s | 1.30 GiB |

12 个阶段合计 `636.012 s`，占这次 `20:06.61` 的有限运行 wall 约 52.7%；四个重阶段
（`scc_deferred_calls`、三个主 merge）合计 `632.529 s`，占 bottom-up 的 99.45%。
整次进程峰值 RSS 是 `15,263,120 KiB`（约 14.56 GiB），主要出现在后续 simplify，
不能用 bottom-up 阶段结束 RSS 代替。

`scc_deferred_calls` 的准确含义是“同一 SCC 内调用边的 deferred subtype 传播”：

1. `MLsubVisitor::visitCallBase()` 发现调用目标和当前函数属于同一 SCC 时，不立即把
   actual/formal 函数类型连起来，而是由 `deferCallConstraint()` 保存一条
   `DeferredCallConstraint`。
2. 当前 SCC 的所有函数 visitor 完成后，`ConstraintsGenerator::run()` 调用
   `applyDeferredCallConstraints()`（当前约第 5160 行），逐条执行
   `addSubtype(Deferred.SubtypeLHS, Deferred.ActualFunc)`。
3. `addSubtype()` 进入 binarysub 的 constraint worklist，递归传播上下界；已经物化的
   变量间直接边现在在入队前走 fast path。所以这 149 秒旧数据、或本次重跑的 155 秒，
   主要仍需结合 task/merge 数和 perf 判断，不再用失败 bound 查找计数推断传播成本。

本次主 SCC 的历史计数是 23,479 次顶层 constrain、86,833,407 个 constraint task 入队；
旧表里的 `149.889 s` 是同一阶段在另一轮运行中的 wall；本次为 `155.344 s`，差异来自
大图的分配/指针顺序和缓存状态，语义没有变化。旧 upper/lower duplicate 计数已删除，
后续比较应使用新的 direct-edge fast path 命中数和 task/merge 统计。post-summary 的
deferred 阶段只有 `1.051 s`，必须与主 `scc_deferred_calls` 分开看。

本轮 Redis 的 subphase 汇总是在上下文拆分前采集的，曾把 post-summary 子调用混入同名
汇总；因此这里以 12 个连续阶段作总 wall 口径。拆分后的主/post 输出已用小 IR 验证，
后续若要比较 subphase，应直接使用新二进制重跑同一 `N`，不能把旧 subphase 汇总与本表
相加。

## 并行化优先级结论

1. 第一优先是三个 merge policy 和 SCC deferred-call 传播。先把每个 merge 阶段再分成
   候选收集、预检查、实际 merge/递归传播三段。候选收集可按函数、call slot 或 struct
   slot 并行，输出稳定排序的候选；实际 merge 暂时串行应用，先测可并行部分的上限。
2. deferred-call 不能直接并发修改 `VariableState`。先用现有 bound set 识别已经物化的
   变量直接边，避免重复进入 worklist；再根据 fast path 命中率决定是否需要更大的跨顶层
   增量队列。若 Redis 大 SCC 最终只有一个连通分量，就不做分量并行。
3. `group_setup` 单核 101 s，占整次有限运行 9.7%，在 256 核上会更突出。可把 root 的
   key 计算和局部分组并行，再稳定排序/合并；必须同时测内存，因为该阶段 RSS 增长约
   5.8 GiB。
4. canonicalize 已用到 7.5/8 核。下一步在目标机测 8、16、32、64、128、256 线程的
   wall、平均核数和 RSS；若平均核数停止增长，再看 shared arena/allocator 争用。
5. 这次前 100 个 group 只用 2 ms，不能代表完整 Redis 的慢 group。外层 group 并行仍需
   用更大的 N 测扩展性；若尾部由单个巨型 group 决定，再考虑 group 内部并行。

## 评价

- 实现效果 9/10：不跑完整 Redis 已能定位 10 分钟单核 bottom-up，并量出最新优化在
  canonicalize 上约 8.5% 的收益。
- 理解成本 4/10：新增 12 个固定阶段和一组直接对应 solver worklist 的计数，没有引入
  新的调度框架。
- 维护成本 3/10：诊断默认关闭；group 停止沿用现有 bulk 边界，停止状态禁止产生部分
  类型结果。

下一步更好的做法不是马上并发 `addSubtype()`，而是先细分三个 merge policy 的候选收集
和实际递归传播时间。当前计数已经证明重心不在普通 visitor，直接并发共享约束图的风险
大于可预期收益。

## transaction root cache 实验（20260818）

### 实现

- `external/binarysub/src/binarysub-core.cpp:17-36` 的
  `SimpleTypeTransactionState` 增加 transaction-local `resolvedRoots`。`resolve_variable()`
  （第 161-208 行）在 transaction 内只读遍历一次并缓存路径，不再为了路径压缩给只读节点
  建 snapshot。
- `SimpleTypeTransaction::commit()`（第 109-127 行）把缓存过的路径写回最终 root；
  `rollback()`（第 129-149 行）直接清掉缓存。`merge_variable_into_impl()`（第 1693-1699
  行）每次真实 parent 更新后清空缓存，避免旧 root 失效。
- `external/binarysub/src/binarysub-test.cpp:1912-1959` 增加 transaction 内不改 parent、
  commit 压缩路径的断言；独立 smoke 还验证了 rollback 和 merge 后 cache 失效。

### 验证与判断

- `cmake --build ./build-relwithdebinfo-20260731 --target binarysub TypeBuilderTest notdec -j4`
  通过；`TypeBuilderTest` 9/9。独立 smoke 用当前 binarysub 库编译运行通过。
- 完整 `binarysub` 仍在既有 parsing sample 4 处 abort，未进入本测试段：期望
  `⊤ -> ⊤ -> ⊤ -> {}`，实际 `⊤ -> … -> … -> {}`。
- Redis 固定 stage-B IR、ASLR、8 线程、allocator override、`N=10`：cache 版本主 SCC
  `call_interface_transaction=182.653s`，`call_interface_merge=202.450s`，
  `struct_slot_merge=136.938s`，`field_followup_merge=135.157s`；完整通过 5182/870/618
  个对应 merge policy pair 后以退出码 75 停止。
- 无 cache parent 同口径运行：`call_interface_merge=196.343s`，
  `struct_slot_merge=134.486s`，`field_followup_merge=144.417s`；但 parent 构建和当前
  构建不是同一提交，且大图指针容器顺序有噪声。与此前同诊断构建的无 cache 数据相比，
  transaction 阶段只变化约 0.5%，暂不能证明 Redis 有稳定收益；峰值 RSS 也只下降约 1%。

### 复杂度评估

- 实现效果：4/10；解决了 transaction 内路径压缩的回滚约束，但当前 Redis 未显示确定
  加速。
- 理解成本：4/10；新增一个局部 map，失效规则集中在 parent 写入点。
- 维护成本：4/10；必须保持所有 `mergedInto` 写入都经过统一失效点，未来若出现第二个写入
  点需要同步更新。

结论：先保留这个小实验作为 transaction 语义基础，但不把它当成主要性能优化。下一步应
  用命中/失效计数确认 cache 是否被频繁清空；若命中率低，优先转向减少重复 constraint
  传播或做只读候选收集并行化。

## direct variable edge fast path（20260818）

### 实现

- `external/binarysub/include/binarysub/binarysub-core.h` 的 `ConstraintSolverStats`
  删除 `upperBoundDuplicates` / `lowerBoundDuplicates`；这两个字段统计的是失败的
  bound-set 查找，容易被误读为新增传播工作。新增
  `directVariableEdgeFastPathHits`，只统计实际跳过 worklist 入队的变量直接边。
- `external/binarysub/src/binarysub-core.cpp` 的
  `skip_cached_or_materialized_variable_edge()` 复用变量已有的
  `upperBoundSet/lowerBoundSet`。变量 pair 已经物化时，先写入本次调用 Cache、调用一次
  Observer，再跳过 constraint worklist；未知 merge policy 默认不启用该跳过，NotDec
  通过 `mergePolicyRechecksOnEvidenceChange` 明确声明其 evidence hook 会重新扫描邻边。
- 两个 constraint worklist 的 `AddToWorklist()` 都使用同一 fast path，因此既覆盖顶层
  重复 seed，也覆盖函数/record 展开产生的重复变量 pair。新增 bound、nested rewrite 和
  merge 的现有传播路径不变。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 汇总和输出 fast path 命中数，并删除旧的
  duplicate 字段；`external/binarysub/src/binarysub-test.cpp` 增加重复函数边、重复顶层
  变量边、Observer 可见性、反向边自动 merge，以及晚绑定 merge policy 的测试。

### 验证

- `cmake --build build --target notdec binarysub TypeBuilderTest -j4` 通过。
- 独立调用 `test_constraint_solver_stats` 通过；`TypeBuilderTest` 9/9 通过。
- 完整 `./build/binarysub` 仍在既有 parsing sample 4 处 abort：期望
  `⊤ -> ⊤ -> ⊤ -> {}`，实际 `⊤ -> … -> … -> {}`；失败发生在新增 solver 测试之前，
  与本改动无关。

## 无 group 上限完整 run 与 perf attach（20260818）

### 运行口径

- 输入：`/tmp/notdec-source-ir-perf-redis-current-20260817/redis-server/work/02-mlsub-input.ll`，
  allocator override：`/tmp/redis-allocator-summary.json`。
- `setarch x86_64 -R`、`NOTDEC_BINARYSUB_THREADS=8`、
  `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1`，没有设置
  `NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS`；开启 `NOTDEC_CONSTRAINT_DIAG=1` 和
  `NOTDEC_SIMPLIFY_DIAG=1`。
- 可执行文件：`build-relwithdebinfo-20260731/bin/notdec`。运行目录：
  `/tmp/notdec-redis-full-20260818`。

### 结果

- 主 SCC 完成全部 `398869/398869` group，`values=246639`、`roots=398869`。
- `/usr/bin/time -v`：wall `45:31.58`，user `12365.14 s`，system `1118.83 s`，
  平均约 `4.93` 核；峰值 RSS `24967156 KiB`。
- `gen-types` lower 阶段 `28.487 s`、RSS `10582400 KiB`；upper 阶段
  `54.533 s`、RSS `24809368 KiB`。随后在继续生成完整类型/workdir 输出时因可用内存
  降至约 `768 MiB` 主动发送 SIGTERM，未把部分 `ValueTypes.txt` 或输出 IR 当作完整结果。

### perf 数据

- bottom-up 120 秒：`/tmp/perf-redis-full-20260818-bottomup.data`，11862 samples。
  平面热点为 `binarysub::resolve_variable` `17.39%`、`std::_Rb_tree_increment`
  `7.56%`、`skip_cached_or_materialized_variable_edge` `5.50%`，其次是
  `unifyPNDiffValueGroups`、`getPNIValue`、`collectMaxDirectFieldAccessSizeBytes` 和
  约束去重哈希。该阶段仍是单线程，优先方向是减少重复变量解析/树遍历和约束任务，而非
  直接并发修改共享图。
- simplify/group 300 秒：`/tmp/perf-redis-full-20260818-simplify.data`，186246
  samples。混合阶段报告中 `CompactTypeBuilder::mergeInsertVars` `48.86%`、
  `std::_Rb_tree_increment` `21.88%`，说明 canonicalize/group setup 仍被有序集合
  合并主导。
- bulk-late 180 秒：`/tmp/perf-redis-full-20260818-bulk-late.data`，86821 samples。
  热点转为 `TypeSimplifier::applySimplificationPlan` `6.35%`、PersistentSet
  `pushLeft` `5.10%`、红黑树查找 `4.94%`、PersistentSet `recycleNodeLocked`
  `4.73%`、`makeNode` `3.89%`；`pthread_mutex_lock/unlock` 与
  `__lll_lock_wait` 合计约 `3.5%`，另有约 `12%` 未解析内核样本。
- 同阶段 `perf stat -p` 60 秒：平均 `5.247/8` 核，IPC `0.61`，cache miss
  `15.77%`，context switch `21,739,021`（约 `69k/s`），CPU migration `49,052`。
  这更符合指针密集型红黑树/arena 访问与共享 factory 锁/调度等待，而不是算术执行
  饱和。

### 优化判断

1. bottom-up 的 `resolve_variable`、`std::_Rb_tree_increment` 和约束任务重复传播是
   独立的单线程优化方向；应先按已物化 direct edge、调用边和 merge 候选做去重，再评估
   候选收集并行化，不能直接并发 `addSubtype()`。
2. simplify 早期的 `mergeInsertVars()` 当前为了保持类型结果使用有序 `std::set` 合并；
   直接替换为无序/向量容器曾改变 HType 结果。可以设计保持
   `SimpleTypeStableIdentityLess` 顺序的批量 sorted-vector merge，再做严格 HType A/B，
   不能只看 wall time。
3. bulk-late 的 PersistentSet factory 在 `CoalesceMemo::Arena` 复制/回收时共享 mutex；
   可考虑按 worker 分片 arena、批量 clone 后再发布 memo，或减少跨 root 结构复制。该方向
   有较大收益潜力，但必须保持 hash-cons、共享 memo 生命周期和类型结果不变。
4. `gen-types` upper 阶段已成为内存瓶颈（约 24.8 GiB），后续应单独 profile
   `TypeBuilder`/ValueTypes 输出，不能把 simplify 的 7.8 GiB RSS 当成完整 run 峰值。

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
merge worklist 中继续传播。诊断计数放在这两个真实循环中，至少记录顶层 constrain
调用、constraint task、merge task、新增上下界、重复上下界和最大 worklist 深度。
计数通过可选诊断对象传入，不开启时只保留一次空指针判断，不引入原子操作或全局锁。

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
  `ConstraintSolverStats`；`src/binarysub-core.cpp:1941` 的 `constrain()` 和第 2074 行的
  `merge_variable_into()` 统计真实 worklist、merge、bound 新增/重复和最大深度。
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
- LLVM IR suite 为 14 通过、8 个 golden mismatch。抽查 `06_SimpleRecursive2`、
  `09_OffsetLoop`、`20_PointerAnalysisFieldCycle`，修改前后的 HType 文件逐字节一致；这些
  mismatch 是当前分支已有的 golden 漂移。

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
只有 1.674 s，不值得优先并行。`scc_deferred_calls` 入队 8683 万个约束任务，其中
upper-bound 重复 7853 万次、lower-bound 重复 2600 万次；三个主 merge 阶段又处理了
约 2913 万个约束任务。这里的主要成本是递归传播和重复 bound，不是生成 LLVM 指令
对应的顶层约束。

30 秒 perf 样本中，`resolve_variable` 占 23.8%，
`collectMaxDirectFieldAccessSizeBytes` 占 8.4%，约束去重哈希插入约 7%，
`constrain_worklist_only` 合计约 9%，PNDiff 分组相关约 10%。数据在
`/tmp/notdec-redis-partial-20260817-current-n100/perf-bottomup.data`。另一份采样进入了
canonicalize，`CompactTypeBuilder::mergeInsertVars` 占 52.1%，不能算作 bottom-up 热点。

## 并行化优先级结论

1. 第一优先是三个 merge policy 和 SCC deferred-call 传播。先把每个 merge 阶段再分成
   候选收集、预检查、实际 merge/递归传播三段。候选收集可按函数、call slot 或 struct
   slot 并行，输出稳定排序的候选；实际 merge 暂时串行应用，先测可并行部分的上限。
2. deferred-call 不能直接并发修改 `VariableState`。7853 万次重复 upper bound 表明先减少
   重复传播可能比加锁并行更划算。可考虑带版本号的跨顶层约束去重，或先按互不相交的
   约束图分量分批；若 Redis 大 SCC 最终只有一个连通分量，就不做分量并行。
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

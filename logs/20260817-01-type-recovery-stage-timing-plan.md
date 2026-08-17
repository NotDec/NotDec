# 类型恢复阶段耗时与平均核数计划（已完成）

## 用户原始 prompt

> 尝试补全每阶段wall_ms/cpu_ms/平均核数吧。规划一下

## 背景

当前 `NOTDEC_SIMPLIFY_DIAG=1` 已能在 `ConstraintsGenerator::genTypes()` 的
bulk、lower、upper、debug 等检查点报告 resident RSS，但没有说明每段实际耗时和
CPU 使用量。binarysub 的 `BulkSimplifyTiming` 也不完整：canonicalize 已有 wall 和
进程 CPU 时间，`analyze/origin/simplify/coalesce` 则是各 group 局部耗时的累加。
后四项在多线程下会互相交错，不能直接当成阶段 wall time，更不能据此计算平均核数。

本计划暂把“每阶段”限定为类型恢复和 binarysub simplify 的现有诊断范围，不扩到
NotDec 的所有 LLVM pass，也不改变普通运行输出。

## 目标与口径

每个真正连续的阶段统一报告：

- `wall_ms`：`steady_clock` 的阶段增量。
- `cpu_ms`：`getrusage(RUSAGE_SELF)` 的 user + system CPU 增量，包含工作线程。
- `avg_cores`：用未取整的 `cpu / wall` 计算，保留两位小数。它表示该阶段平均实际
  占用的核数，不受配置线程数代替。
- `rss_kb`：阶段结束时的 resident RSS，继续沿用当前 `/proc/self/statm` 第二列口径。

内部至少保留微秒精度，最后才换算成毫秒，避免短阶段先取整后把平均核数算成 0 或
异常大。`getrusage` 失败时明确输出 unavailable，不把失败伪装成 0 CPU。

## 技术路线

### 1. genTypes 外层阶段

在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 中，把当前检查点改成完成阶段的记录：

- `prepare`：归一化 V2N、收集待求解 roots，以及可选图 dump。
- `bulk`：完整 `bulkSimplifyDetailed()`。
- `lower`：lower HType 转换。
- `upper`：memory/storage 和 upper HType 转换。
- `debug_output`：`ValueTypes.txt`、`VarOrigins.txt` 等 SCC 级调试输出。

每个 SCC 仍输出一组记录，便于定位 Redis 这类由单个大 SCC 主导的情况；
`topDownPhase()` 再按阶段汇总所有 SCC。SCC 是顺序处理的，因此各 SCC 的 wall/CPU
增量可以直接相加，汇总 `avg_cores` 再用总 CPU / 总 wall 计算，不能平均各 SCC 的
比值。现有 `bulk-start/bulk-done` RSS 信息要保留等价能力，避免丢失阶段前后的内存
变化。

### 2. binarysub 内层阶段

在 `external/binarysub` 的 `BulkSimplifyTiming` 和
`TypeSimplifier::bulkSimplifyDetailed()` 中补齐实际存在的连续执行区间：

- `canonicalize`：沿用现有 wall/CPU 计时，补统一的 `avg_cores` 输出。
- `group_processing`：覆盖整个 group `parallel_for` 或单线程循环，统一记录 wall、
  进程 CPU 和平均核数。
- `finalize`：收集 `LocalResults`、构造返回 map 和阶段收尾。
- `total`：覆盖一次 bulk simplify，作为以上阶段和未单列小段的总口径校验。

现有 `analyze/origin/simplify/coalesce` 发生在每个 group 内并互相交错。继续保留它们
用于判断工作量分布，但字段和注释明确为 `*_work_ms`；兼容旧 trace 时可以暂时同时
输出旧字段，不能给这些值配一个虚假的 `avg_cores`。若以后确实需要四项各自的 wall，
应另做采样或事件区间合并，不能为了计时给执行路径增加四次全局 barrier。

### 3. 输出与开销

第一步仍放在 `NOTDEC_SIMPLIFY_DIAG` / binarysub trace 诊断输出中，不改
`merge-eval-summary.json` 的稳定结构。输出使用固定字段名，便于长跑日志直接用
脚本提取；完成后再根据实际使用情况决定是否把阶段汇总写入 JSON。

计时只做阶段边界快照，不在每个 root/group 内调用 `getrusage`，避免计时本身影响
fortune、lighttpd 等性能 A/B。新增的阶段统计结构只保存整数计数，不持有类型节点或
字符串。

## 风险

- 进程 CPU 时间会包含同一进程内其他工作线程，正是计算阶段平均核数所需；若以后
  引入与当前阶段无关的后台线程，需要重新说明口径。
- 多线程 group 子步骤的累计 work time 可大于 bulk wall time，这是正常现象。字段
  命名和日志说明必须避免把两者相加。
- 毫秒显示值存在取整误差，判断平均核数必须使用内部高精度值。
- 增加汇总时不能延长 binarysub arena 或 HType 的生命周期，否则内存数字会被诊断
  代码本身改变。

## 验证与判断标准

- 单线程 fortune 中，各主要阶段 `avg_cores` 应接近 1，wall/CPU 数量级一致；类型
  文件、merge-eval oracle 和 LLVM 22 verifier 与基线一致。
- 8 线程 lighttpd 中，parallel canonicalize 或 group processing 的平均核数应明显
  高于 1，lower/upper/debug-output 通常接近 1；不能出现用配置线程数硬填的固定值。
- `total` wall 应覆盖各连续阶段，允许少量未单列开销；外层 `bulk` 与 binarysub
  `total` 应只差调用边界和取整误差。
- SCC 汇总的 wall/CPU 等于各 SCC 对应阶段的高精度增量之和，汇总平均核数按总量
  重算。
- 开关关闭时不增加诊断输出；开关开启后的 wall/RSS 相对基线没有可观察的系统性
  回归。

## 实现记录

### binarysub 连续阶段

- `external/binarysub/include/binarysub/binarysub.h:1073` 的
  `BulkSimplifyPhaseTiming` 以微秒保存 wall/进程 CPU，并在 `BulkSimplifyTiming` 中
  分开 canonicalize、group setup、group processing、finalize 和 total。
- `external/binarysub/src/binarysub.cpp:150` 的计时与格式化函数使用
  `getrusage(RUSAGE_SELF)`；失败时输出 unavailable，平均核数用未取整微秒计算。
- `TypeSimplifier::snapshotBulkSimplifyTiming()`（约第 2622 行）保存阶段数据和原缓存
  统计；`bulkSimplifyDetailed()`（约第 5927、6250-6344 行）在真实连续边界取快照，
  向 `NOTDEC_SIMPLIFY_DIAG` 输出 `[simplify-phase]`，并扩展 binarysub trace。
- 原 `analyze/origin/simplify/coalesce` 明确改称 `*_work_ms`；旧 trace 字段暂时同时
  输出，避免破坏现有日志读取。

实现时发现 canonicalize 与 group 并行循环之间的 occurrence cache、root 分组准备在
lighttpd 上并非可忽略开销，因此相对原计划新增 `group_setup`。它只增加两个阶段边界
快照，不改变执行顺序，也不增加 barrier。

### genTypes 外层阶段

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:59` 新增只含计数的
  `GenTypesPhaseTiming` / `GenTypesTiming`；`ConstraintsGenerator::genTypes()` 改为返回
  本 SCC 的五阶段数据。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:99-180` 增加进程 CPU、平均核数、RSS 和
  SCC 汇总辅助函数。
- `ConstraintsGenerator::genTypes()`（约第 6311-6493 行）报告 prepare、bulk、lower、
  upper、debug_output 的 `wall_ms/cpu_ms/avg_cores/rss_kb`。
- `MLsubRecovery::topDownPhase()`（约第 6561-6605 行）按阶段累加所有 SCC 的微秒
  数据，再用总 CPU / 总 wall 重算 `[gen-types-summary]`，不平均各 SCC 比值。

## 验证结果

- `cmake --build ./build --target notdec -j4`：通过。
- `cmake --build ./build-relwithdebinfo-20260731 --target notdec TypeBuilderTest binarysub
  -j4`：通过。
- `./build-relwithdebinfo-20260731/bin/TypeBuilderTest`：9/9 通过。
- fortune 单线程诊断开关 A/B：`ValueTypes.txt`、`ValueHTypes.txt`、
  `ImportantHTypes.txt`、`VarOrigins.txt`、三个 eval 明细和输出 IR 均逐字节一致；
  merge-eval oracle 一致，LLVM 22 `opt -passes=verify` 通过。关闭开关时没有新增阶段
  日志，开启后五个外层阶段、五个 binarysub 阶段和 SCC 汇总字段完整。
- lighttpd 8 线程完整运行：wall `82.41s`、峰值 RSS `1425948 KiB`，LLVM verifier
  通过，merge-eval 为 `bad_unions=20`、`fragmented_nodes=361`，与当前基线一致。
  主 SCC 的 canonicalize 平均 `6.37` 核、group setup `1.00` 核、group processing
  `7.09` 核、binarysub total `6.50` 核；外层 bulk `6.47` 核，lower/upper/debug
  均为 `1.00` 核。
- `./build-relwithdebinfo-20260731/binarysub` 仍在已有 parsing sample 4 失败：期望
  `⊤ -> ⊤ -> ⊤ -> {}`，实际 `⊤ -> … -> … -> {}`。该失败与本次计时路径无关，
  也已在前序性能日志中记录。

实现效果 9/10：阶段 wall、CPU、平均核数和 RSS 已能互相对照，并实际区分了
lighttpd 的并行与单线程区间。理解成本 3/10：新增两个小计时结构和固定阶段字段，
没有改求解流程。后期维护成本 2/10：计时集中在边界，新增阶段时只需补一个快照和
输出字段。当前没有更简单且还能正确处理并行交错子步骤的方案。

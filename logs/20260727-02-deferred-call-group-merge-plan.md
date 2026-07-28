# 用户原始 prompt

> 能不能限制一下，仅分析部分函数作为当前测试用例，不然这个太慢了。我现在需要按照这个方向改进：当前是子类型关系，会在访问到函数调用的时候直接加上，然后有一个双向合并策略会在约束生成后再加上去。当前我希望改成在访问函数调用的时候直接先不加上约束，而是留到后面和双向合并策略同时一起处理。但是也是先增加所有的子类型关系，再处理节点合并。这里可以先commit一下。保持子类型关系增加得不变，但是处理节点合并的时候，改成，对每个参数或返回值，分析是否所有的实参和形参、返回值和真实的返回值之间的结构体布局存在冲突，如果不冲突则全部合并，如果冲突则全部不合并。另外，根据源码，找到ffplay那边是否存在malloc wrapper，有的话得标记为多态，同时关注是否存在socket相关的函数，raw memory read等，如果读入数据结构的话，也得标记为多态。

# 背景

当前同 SCC 的直接调用在 visitor 中立即增加 `formal function <: actual function`，同时记录
actual/formal 和 return/function-return 的晚期合并候选。约束生成结束后，现有策略逐对判断并合并。
逐对处理的问题是：同一个形参先与一个兼容实参合并后，后面才发现另一个实参布局冲突时，前面的
合并已经不能撤销。返回值也有同样问题。

ffplay whole-module IR 有 108 个函数，串行 bottom-up 数分钟仍未完成，不适合每次修改后直接全量
验证。它的网络输入由动态 libavformat 实现，ffplay 模块没有直接 `socket`、`recv` 或 POSIX
`read`；模块内仍有 `av_fifo_read/write` 这类裸缓冲区 API，以及多种 FFmpeg allocator API。

# 目标

把直接调用的子类型约束从 instruction visitor 延后到本 SCC 的 IR 约束收集完成之后。子类型方向和
数量保持不变，并保证所有调用子类型约束先增加，再运行任何调用参数或返回值节点合并。

调用参数按“同一个 formal slot”分组，返回值按“同一个 function-return slot”分组。先检查组内
formal 和全部 actual/真实 return 的一层结构体字段布局；全部兼容才合并整组，任一冲突则整组不合并。
多态函数实例的 formal slot 相互独立，不跨 callsite 合并。

从 ffplay 提取少量有 DebugInfo 的函数及其直接依赖，形成快速评估输入。补齐 ffplay 实际使用的
通用 allocator/裸缓冲区声明的多态标记，避免一个外部声明把无关 callsite 的 buffer 类型串起来。

# 技术路线

同 SCC 调用只在 visitor 中构造 actual function type 并保存待处理调用。visitor 全部结束后，先遍历
待处理调用增加原来的 subtype，再统一记录参数/返回值候选，最后求解并执行分组合并。跨 SCC 的
summary instantiate 维持同样顺序：先实例化并增加全部 subtype，再运行分组合并。

分组键使用解析后的 formal root，而不是只用函数名和参数序号。这样普通函数的所有 callsite 会进入
同一组，真正的多态实例仍各自成组。合并前对组内所有不同 root 做两两字段布局兼容检查；预检查失败
时只写 skip trace，不改变节点。

ffplay 快速样例优先选 packet queue 和 allocator helper 周边函数，保留 DebugInfo，不把动态 libav
实现拉进来。外部 API 名单只加入源码中确实承担通用分配或裸字节搬运的声明；带固定结构语义的普通
FFmpeg API 不因“有 pointer 参数”就一概标成多态。

# 源码审计边界

- 通用 allocator 声明：`av_calloc`、`av_mallocz`、`av_malloc_array`、`av_realloc_array`、
  `av_fast_malloc`。
- 裸 FIFO buffer：`av_fifo_read`、`av_fifo_write`；数据元素由 caller 决定，适合作为多态边界。
- `stream_open` 分配并初始化固定的 `VideoState`，属于 factory，不是通用 malloc wrapper。
- `get_codecs_sorted`、`grow_array`、`allocate_array_elem` 都带额外容器或 out-parameter 语义，先不放宽
  现有“纯转发 allocator 结果”的通用 wrapper 检测规则。
- 当前 ffplay executable IR 没有直接 socket/POSIX raw read；这些发生在动态 libavformat 中。

# 风险

- 若分组只按函数名，会破坏多态实例隔离；必须按 formal root 分组。
- 字段布局预检查通过后，底层 variable merge 仍可能因其他约束失败。当前 merge API 没有事务，不能
  在部分执行后回滚；实现需要在测试中覆盖，并把意外失败视为策略错误而不是静默留下半组结果。
- 把固定类型的外部 API 过度标成多态会丢失有用的跨调用约束，因此名单必须限于真正通用的 allocator
  和 raw buffer 操作。
- 裁剪 IR 只能验证策略和 DebugInfo correctness，不能替代 whole-module 性能结论。

# 判断标准

- visitor 不再直接增加同 SCC call subtype；延后阶段增加的 subtype 数量和方向与旧逻辑一致。
- 同一 formal slot 有多个候选时，兼容组全部合并，含一个冲突时整组零合并。
- 多态 summary 的不同实例不因分组策略重新合到一起。
- ffplay 裁剪样例能在可接受时间内产出 merge eval 汇总，且 `bad_unions = 0`。
- workdir 能列出 ffplay 命中的 allocator/raw buffer 多态声明；不存在的 socket/POSIX read 不伪造记录。

# 实现进度

## 阶段一：调用 subtype 延后（已完成）

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h` 的
  `ConstraintsGenerator` 新增 `DeferredCallConstraint`，分别保存原 subtype 左端、callsite
  function type 和供 late merge 使用的 formal function type；`run()` 在全部 instruction visitor
  完成后统一 flush。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 新增 `deferCallConstraint()` 和
  `applyDeferredCallConstraints()`。后者固定执行两遍：第一遍增加所有原有 subtype，第二遍才记录
  参数和返回值合并候选。
- `MLsubVisitor::visitCallBase()` 不再直接调用 `addSubtype()`；跨 SCC 的
  `MLsubRecovery::bottomUpPhase()` 也先完成全部 summary instantiate，再统一 flush。两处 subtype
  的方向和数量均未改变。

验证：

```bash
cmake --build ./build --target MLsubGeneratorTest notdec -j8
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure -j8
./build/bin/MLsubGeneratorTest --gtest_filter=MLsub.PhiNodeCanBeUsedByAnEarlierListedBlock
```

构建、LLVM IR suite 和 PHI 定向测试通过。完整 `MLsubGeneratorTest` 中两个旧 PNDiff 测试仍用
LLVM 指针直接查询已经改成 opaque handle 的 `PNIGraph`，在本次修改前即会断言，不属于本阶段回归。

## 阶段二：按 formal slot 整组判断（已完成）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3443` 新增
  `applyGroupedCallSlotMergePolicy()`。分组键是 resolved formal root；先检查组内所有 root 的
  level/size，再对全部一层字段切片做两两冲突检查。任一冲突只写 group-skip trace，整组不改。
- 参数和跨 callsite return 继续要求组内至少有一份结构体布局证据，避免把普通标量和只有直接 load
  的多态值强行单态化。函数体 `ReturnInst` operand 保留原来的无条件 return-root 合并；若组内已有
  结构体证据，则真实 return 和全部 call return 一起合并。
- 预检查通过后依次把成员并入 formal root。底层 merge 若仍失败会立即报 fatal error，避免返回带
  半组修改的结果。每次策略执行后清空已处理候选，跨 SCC summary 阶段只处理新实例。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:89` 的内建多态名单新增 ffplay 实际使用的
  `av_calloc`、`av_mallocz`、`av_malloc_array`、`av_realloc_array`、`av_fast_malloc`、
  `av_fifo_read`、`av_fifo_write`。另外加入 `qsort`：它按 element size 解释裸数组并接收多态
  comparator；不隔离时 comparator formal 的 PNDiff 会被具体函数值改成 `func`，与 qsort summary
  的 `ptr` 标记冲突。
- `unittests/Retypd/MLsubGeneratorTest.cpp:104` 起新增参数冲突、返回兼容、返回冲突和不同 formal root
  隔离测试。两处旧 PNDiff 测试改走 `ConstraintsGenerator::getPNINode()`，完整单测恢复可运行。

## ffplay 裁剪与审计结果

用 LLVM 22 `llvm-extract --recursive` 从完整 ffplay bitcode 提取 6 个函数：
`packet_queue_flush`、`packet_queue_init`、`packet_queue_put`、`allocate_array_elem`、`grow_array`、
`get_codecs_sorted`。产物为：

- `/sn640/NotDec-Exp/Bench2/source-ir/ir/ffplay/ffplay-call-merge-small.bc`：92 KiB。
- `/sn640/NotDec-Exp/Bench2/source-ir/ir/ffplay/ffplay-call-merge-small.ll`：252 KiB，保留 3 个
  `DICompileUnit` 和 44 个 struct DebugInfo。

源码和 IR 中没有直接 `socket`、`recv*` 或 POSIX `read`，网络读取在动态 libavformat 内。
`av_read_frame` 写固定 `AVPacket`，`stream_open` 返回固定 `VideoState`，均未标多态。
`MallocWrappers.txt` 确认没有符合纯转发规则的 malloc wrapper。

最终裁剪评估：

```bash
./build/bin/notdec \
  /sn640/NotDec-Exp/Bench2/source-ir/ir/ffplay/ffplay-call-merge-small.ll \
  --tr-level=2 --merge-struct-ptr-load-store \
  -g --work-dir=/tmp/notdec-source-ffplay-small-group-r7-work \
  --merge-eval-dir=/tmp/notdec-source-ffplay-small-group-r7-eval \
  -o /tmp/notdec-source-ffplay-small-group-r7-out.ll
```

结果：1.38 秒，峰值 356 MiB；`bad_unions=0`、`polluted_components=0`、
`fragmented_nodes=3`、`fragmented_types=2`、`merged_nodes=7`。输出 IR 通过 LLVM 22 `opt -passes=verify`。
本裁剪模块的 `PolymorphicBufferFunctions.txt` 命中 `av_realloc_array`、`av_calloc`、`av_mallocz`、
`qsort`、`av_fifo_read`、`av_fifo_write`；完整 ffplay 中还声明了名单里的 `av_malloc_array` 和
`av_fast_malloc`。

## 完整 ffplay 与函数规模实验

完整 108 函数 IR 已跑完。默认 binarysub 线程数为机器的 16 个硬件线程，canonicalize 并行生效：

- eval：`/tmp/notdec-source-ffplay-full-group-r8-eval/merge-eval-summary.json`。
- 结果：`wall_ms=445557`、`peak_rss_mb=5976`、`nodes_created=10239`、
  `typed_target_nodes=107/5539`。
- 合并正确性：`bad_unions=2`、`polluted_components=2`、`extra_types=2`；
  `fragmented_types=16`、`fragmented_nodes=28`。
- 输出 `/tmp/notdec-source-ffplay-full-group-r8-out.ll` 通过 LLVM 22 `opt -passes=verify`。

两条不同 DebugInfo strict 类型的错误合并是：

1. `avcodec_alloc_context3::<ret>` 的 `AVCodecContext*` 与
   `check_stream_specifier::arg0` 的 `AVFormatContext*`。前 93 个函数时没有，加入第 94 个
   `stream_component_open` 后出现，记录为 `explicit / binarysub_merge`。
2. `configure_filtergraph::arg3` 的 `AVFilterContext*` 与其 `arg0` 的 `AVFilterGraph*`，记录为
   `policy_replace_bound / binarysub_merge`。只分析 `configure_filtergraph` 或只加
   `video_thread` 调用方不会出现；`configure_audio_filters + configure_filtergraph` 的最小组合可复现。

递增实验按 IR 定义顺序保留前 N 个函数体，并用 `--rglob='.*'` 保留完整模块的 611 个全局定义，
避免 `llvm-extract` 把全局变成无 initializer 的声明。每档显式设置
`NOTDEC_BINARYSUB_THREADS=8`，产物在 `/tmp/notdec-ffplay-scale-20260728/n<N>g/`：

| 函数数 | 最后加入的函数 | 节点数 | 耗时 | 峰值 RSS | bad unions |
| ---: | --- | ---: | ---: | ---: | ---: |
| 83 | `packet_queue_init` | 5580 | 117.0 s | 808 MiB | 0 |
| 84 | `read_thread` | 6375 | 143.1 s | 875 MiB | 0 |
| 85 | `stream_close` | 6488 | 172.5 s | 1828 MiB | 0 |
| 86 | `video_display` | 7373 | 209.9 s | 2142 MiB | 0 |
| 93 | `stream_component_close` | 7829 | 249.3 s | 2689 MiB | 0 |
| 94 | `stream_component_open` | 8132 | 307.6 s | 3263 MiB | 1 |
| 98 | `decoder_start` | 8714 | 344.5 s | 3949 MiB | 1 |
| 99 | `video_thread` | 9222 | 380.2 s | 4331 MiB | 1 |

没有函数使耗时出现数量级爆炸。最明显的复杂度拐点是第 85 个 `stream_close`：节点只增加 1.8%，
但 RSS 增为 2.09 倍，耗时增加 20.5%，说明它引入的类型结构比节点数本身更影响 canonicalize。
第 94 个 `stream_component_open` 也偏重：节点增加 3.9%，耗时增加 23.4%，并首次产生 wrong merge。
相比之下，`read_thread` 和 `video_thread` 的耗时增长与节点增长更接近连续规模效应。

## 验证

```bash
cmake --build ./build --target MLsubGeneratorTest notdec -j8
./build/bin/MLsubGeneratorTest
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

8 个 MLsubGenerator 单测和 LLVM IR 的 21 个类型恢复用例全部通过。sysy suite 当前由 Clang 14
重新生成的输入与旧 golden 不同；realworld fortune 的 extra-constraint SHA256 锚点与当前 frozen IR
不一致，两者都在进入本次策略前失败或产生基线差异，未改这些无关文件。

## 评分

- 实现效果：7/10。满足 subtype 全量先加、slot 整组冲突判断和多态实例隔离；裁剪 ffplay 无 wrong
  merge。全量 ffplay 已完成，但发现 2 条不同 DebugInfo 类型的错误合并，后续仍需修正。
- 理解成本：6/10。新增一个参数/返回共用的分组 helper，并保留真实 return 与跨调用 return 的既有
  证据差异，逻辑集中但有一层必要分支。
- 维护成本：5/10。API 名单需要随新的通用内存库扩充；组内预检查和 fatal invariant 有定向测试，
  后续改布局规则只需改一个 helper。

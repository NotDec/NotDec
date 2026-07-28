# 用户原始 prompt

> 首先，之前改进中提到的“处理节点合并的时候，改成，对每个参数或返回值，分析是否所有的实参和形参、返回值和真实的返回值之间的结构体布局存在冲突，如果不冲突则全部合并，如果冲突则全部不合并” 这一块的逻辑在哪里，最好加一个log或者增加一个debug dir里面的统计文件，列举出所有没有合并的参数点或者返回值点出来。然后，再根据刚才出现错合并的小的IR组合，分析一下两个错合并具体是怎么回事，为什么会错误合并？

# 背景

参数和返回值的整组合并已经集中在 `applyGroupedCallSlotMergePolicy()`。它按 formal root 分组，
在第一次破坏性合并前检查组内全部 root 的层级、大小、结构体证据和一层字段切片冲突。当前只有
binarysub trace 会记录部分 skip，而且默认不开启、只显示组首个 callsite，无法直接审计某个槽位
为什么没有合并。

ffplay 完整评估发现两条不同 DebugInfo 类型的错误合并。现有 `bad_unions.jsonl` 能指出 merge reason
和两端 DebugInfo 类型，但不能直接说明是哪条上层策略、哪些 callsite 和哪些布局证据促成了合并。

# 目标

在 workdir 增加稳定的调用槽合并统计，列出每个参数或返回值 formal slot 的全部候选、布局、最终决策
和原因，尤其保证所有未合并槽位可直接检索。普通未开启 workdir 的运行不增加文件或额外输出。

用最小 ffplay IR 组合和 binarysub trace 对两条 wrong merge 做因果分析，确认错误发生在调用槽整组策略、
其他显式 merge policy，还是 subtype 求解触发的 binarysub bound replacement。

# 技术路线

在 `ConstraintsGenerator` 内保存结构化的调用槽决策记录。整组策略在候选归组、预检查失败和合并完成时
各记录一次，保留 policy、formal root、全部 callsite、全部成员 root、一层字段切片、decision 和 reason。
`MLsubRecovery::run()` 在所有 bottom-up/top-down 阶段结束后统一写入
`CallSlotMergeDecisions.txt`，避免多次打开文件，也避免依赖可选的 binarysub trace。

单测直接检查冲突组、兼容组和缺少结构体证据组的记录。集成验证使用 ffplay 小组合生成 workdir 文件，
再对两条 bad union 开启 trace，对照 merge event 前后的 subtype、policy merge 和 bound replacement。

# 风险

- 一个 generator 的策略可能在 summary instantiate 后执行多次；记录必须追加而不能覆盖，并标明 SCC 名。
- root 会在后续继续合并；统计必须在决策发生时把字符串和布局快照保存下来，不能最后再解析 live node。
- “没有合并”既包括布局冲突，也包括缺少证据、类型层级不符和候选本来已同 root；统计需区分原因。
- DebugInfo bad union 可能是整组策略之后的其他规则造成，不能仅凭最终两个节点推断调用槽策略有错。

# 判断标准

- 开启 workdir 后稳定生成 `CallSlotMergeDecisions.txt`，关闭 workdir 时行为不变。
- 每个被考虑的参数/返回值组都有一条决策；skip 条目包含全部 callsite、成员布局和明确原因。
- 现有参数冲突整组不合并、返回值整组合并测试继续通过，并新增统计内容断言。
- 两个 ffplay 最小组合能说明错误合并的直接规则和前置约束，不把相关性写成因果。

# 实现情况（已完成）

## 代码位置

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:204` 的
  `CallSlotMergeDecision` 保存一次决策发生时的字符串快照，避免后续 root 合并改变统计结果。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3515` 的
  `applyGroupedCallSlotMergePolicy()` 是参数和返回值共用的整组逻辑。它先按 formal root 分组，
  检查组内全部 root 的 level、size 和一层字段切片，再开始任何破坏性合并；发现冲突时整组跳过。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3846` 的 `applyReturnValueMergePolicy()` 负责返回值入口，
  `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3861` 的 `applyCallArgStructPtrMergePolicy()` 负责参数入口。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:350` 的 `ConstraintsGenerator::run()` 先执行
  `applyDeferredCallConstraints()`，使所有调用子类型关系先进入求解器；`PG.solve()` 后才依次执行返回值
  和参数整组合并。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1993` 的 `writeCallSlotMergeDecisions()` 汇总所有 SCC，
  `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4434` 的 `MLsubRecovery::run()` 在 bottom-up/top-down 完成后写
  `CallSlotMergeDecisions.txt`。
- `unittests/Retypd/MLsubGeneratorTest.cpp:104`、`:143`、`:210` 分别覆盖冲突整组跳过、兼容返回值整组合并、
  缺少结构体证据时记录跳过。
- `DEBUG.md:206` 的 `CallSlotMergeDecisions.txt` 小节说明了文件字段和排查顺序。

统计文件记录 `policy`、`decision`、`reason`、formal root、全部 callsite/return 点，以及每个 root 在决策前的
level、位宽、字段切片和 action。搜索 `decision: skipped`、`decision: partial` 可以直接列出未合并点；
`already-merged` 用来暴露轮到整组策略之前已发生的合并。

## ffplay 错合并分析

### `AVCodecContext*` 与 `AVFormatContext*`

最小组合为 `stream_component_open`、`filter_codec_opts`、`check_stream_specifier`，并保留全部 globals。
`bad_unions.jsonl` 报告 `avcodec_alloc_context3::<ret>` 与 `check_stream_specifier::arg0`，reason 为 `explicit`。

直接原因是 `av_log(void *avcl, ...)` 的第 0 个参数被当成单态 formal 槽。这个最小组合里有 6 个
`av_log` callsite：`check_stream_specifier`/`filter_codec_opts` 传入 `AVFormatContext*` 一侧，
`stream_component_open` 还传入 `avcodec_alloc_context3` 产生的 `AVCodecContext*`。统计文件的 Decision 29
显示这 6 个 actual 共用 formal `vs#4`，其中 `vs#37` 最终包含 `check_stream_specifier::arg0`，`vs#236`
来自 `avcodec_alloc_context3::<ret>`。

当前冲突检查只拒绝“字段区间重叠但大小不同”。`vs#37` 的已观察切片是 `[@16:8]`，`vs#236` 是
`[@12:4,@24:4,@80:4,@352:4,@360:4,@632:4,@716:8]`，两者没有重叠宽度冲突，于是整组被判为
`compatible-layouts`，先后显式并入 `av_log::arg0`。这里不是检查漏看了某个已知冲突，而是布局兼容不足以
证明语义类型相同。`av_log` 的 `void *avcl` 本来就允许不同 FFmpeg 上下文对象，应按多态上下文参数处理，
不能用一个 formal root 串起全部调用点。

### `AVFilterGraph*` 与 `AVFilterContext*`

最小组合为 `configure_audio_filters` 和 `configure_filtergraph`。`bad_unions.jsonl` 报告
`configure_filtergraph::arg0` 与 `configure_filtergraph::arg3`，reason 为 `policy_replace_bound`。

这条不是参数 3 的整组策略主动合并。统计文件的 Decision 57 显示参数 0 的 actual `vs#277`
（切片 `[@8:8,@16:4,@36:4]`）被判定兼容并入 formal `vs#246`（切片 `[@8:8,@16:4]`）。合并前，
`vs#277` 已有子类型上界 `vs#249`，即 `configure_filtergraph::arg3`。`merge_variable_into()` 把该 bound
重写为 `vs#246 <: vs#249` 后，`shouldMergeSameFunctionStructPtrSubtype()` 看到两端同位宽、均为指针、
至少一端有结构体证据，而且 external value 都属于 `configure_filtergraph`，于是排队执行
`PolicyReplaceBound`，把参数 3 合入参数 0。

该 hook 只检查“属于同一函数”，不检查是否为同一参数/返回槽，也不调用字段布局冲突检查。因此轮到参数 3
的整组策略时已经太晚，Decision 60 只能记录 `already-merged`，actual/formal 都已解析成 `vs#246`。
这说明后续修复应收紧 same-function subtype hook，不能只调整调用槽整组预检查。

## 验证

已完成：

```bash
cmake --build ./build --target MLsubGeneratorTest notdec -j8
./build/bin/MLsubGeneratorTest
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
./llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/notdec-ffplay-bad-union-analysis-20260728/codec/out.ll
./llvm-22.1.0.obj/bin/opt -passes=verify -disable-output \
  /tmp/notdec-ffplay-bad-union-analysis-20260728/filter/out.ll
```

9 个 `MLsub` 单测全部通过；`notdec.type_recovery.llvm_ir.tr_level_2` 通过；两个最小输出均通过 LLVM 22
verifier。两个最小 ffplay 组合都生成了 `CallSlotMergeDecisions.txt`，并保持原来的各 1 条 wrong merge，
便于确认本次修改只增加统计，没有改变合并结果。

## 评分

- 实现效果：9/10。未合并槽可直接检索，两个错例都能对到具体决策；尚未把统计接入 merge-eval JSON。
- 理解成本：8/10。复用现有整组入口，只新增一个记录结构和一个文本 writer；字段较多，但都对应排查所需事实。
- 维护成本：8/10。格式是简单文本，策略新增 decision/reason 时需同步文档和单测。

更好的长期方案是让 binarysub merge event 带上触发它的上层 call-slot decision id，这样
`bad_unions.jsonl` 能直接反查统计记录。但这会改 binarysub 的事件接口，不适合和本次观测补充混在一起。

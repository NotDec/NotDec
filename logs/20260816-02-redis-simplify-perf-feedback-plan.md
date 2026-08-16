# Redis simplify 性能与 source-IR 快速反馈框架计划

## 用户原始 prompt

> 阅读logs/20260816-01-redis-opaque-body-handoff.md。当前Redis要跑很久，应该是存在着不小的冗余，计算应该还是有提升性能的空间。跑起来后尝试用perf attach上去看看热点在哪里？然后思考是否有机会改进性能？
>
> 其次，当前每次改进的反馈都太慢了。尝试构建一个机制，比如用一个脚本把类似的引入的几个binary的类型推理并行跑起来，特别是先跑那些没那么大的，从而每次修改之后，就可以迅速获得性能变化的反馈。

确认后的实施顺序：

> 对，先构建实验框架，然后再思考怎么改进当前的效率

## 背景

redis-server 源码级 IR 有 57 MB、3870 个函数。当前 opaque-body 实验已经进入
8 线程 simplify，内存稳定，但运行超过一小时仍未完成。现场 perf 显示主要 CPU
消耗在 `CompactType`、`CompactVarSet` 的相等比较和集合遍历。完整 Redis 每轮
反馈太慢，直接在它上面试错很难判断修改是有效、无效还是只改变了运行噪声。

现有 Bench2 source-IR 已有从 fortune 到 memcached 的多个较小输入，也有
DebugInfo merge-eval 和 HType 产物。它们适合先组成快速反馈集合。wrk 和 libuv
分别受长浮点、通用 buffer/回调接口影响，默认不作为当前指针类型恢复性能判断
样本；lighttpd、tmux、redis 留作扩大验证。

## 目标

第一阶段增加一个可重复运行的并行脚本，用相同参数同时执行多个较小 source-IR
项目，统一收集运行时间、峰值内存、退出状态、LLVM verifier、merge-eval 指标和
HType 文件摘要。脚本应支持读取一次旧结果作为基线，直接显示本次性能变化和类型
结果是否变化。

第二阶段根据 Redis perf 和快速集合结果，处理 simplify 中重复的精确结构比较。
先选择生命周期有界、不改变类型相等语义的做法；如果实验没有稳定收益，就撤掉，
不为了保留实验而增加长期复杂度。

## 技术路线

快速集合默认使用 fortune、ffplay、vsftpd、ngircd、memcached。每个项目使用独立
输出目录，项目之间并行，单项目仍使用 binarysub 线程。运行固定关闭 ASLR，并默认
关闭 canonicalize 并行，减少递归分组波动；后续 simplify 仍按指定线程数并行。实测
多线程 simplify 仍可能产生 HType 文本变体，因此多线程性能口径完整报告 HType 哈希，
但用 merge-eval oracle 作自动门槛；单线程口径的 HType 文本可重复，全部类型文件哈希
都作为硬门槛。结果保存为机器可读 JSON 和便于终端查看的表格；比较时先检查输入、
可执行文件和运行参数，再比较性能与 oracle，避免拿不同口径的数据算百分比。

性能实验优先考虑在单个 group 的生命周期内记住已经确认相等的 memo/local
CompactType 节点对。这样可以复用精确比较结果，又不会把所有 per-group arena 合并
成全局常驻 arena。先用快速集合判断时间和内存，再决定是否进入完整 Redis 复验。

## 风险

- 多个进程并行会互相争抢 CPU，单项目 wall time 只能和相同并发参数的基线比较。
- binarysub 并行和指针顺序可能使 HType 文本存在已有的 run-to-run 差异；摘要变化
  必须结合 merge-eval 指标和具体 diff 判断，不能只看输出 IR。
- ngircd、memcached 已有 bad union，判断标准是与基线一致，不是假定为零。
- 精确比较缓存可能用额外内存换时间；缓存必须限制在单 group，且需要报告峰值 RSS。
- 快速集合不能代替 Redis、tmux、lighttpd 的最终验证，只用于尽快筛掉无效方案。

## 判断标准

- 默认集合能并行完成，并为每个项目生成完整的状态、时间、内存、verifier、eval 和
  HType 摘要。
- 给定同口径基线时，报告能清楚列出 wall/RSS 变化和 oracle 是否一致；任一运行或
  verifier 失败时整体返回失败。
- 性能修改先通过 binarysub 单元测试和默认快速集合，不增加新的 bad union，不让
  HType 结果出现无法解释的变化。
- 只有快速集合显示稳定收益、峰值内存可接受时，才重跑完整 Redis；最终仍以完整
  Redis 的 wall、峰值 RSS、merge-eval 和 HType 文件为准。

## 实现记录（反馈框架）

已完成 `scripts/run-source-ir-perf-smoke.py`：

- `CASE_INPUTS` 和 `TYPE_RESULT_FILES`（约第 30-55 行）固定 source-IR 用例和类型产物。
- `run_case()`（约第 270-399 行）为每个项目创建独立 work/eval 目录，使用
  `setarch -R`、`/usr/bin/time`、超时和独立进程组，运行后用 LLVM 22 `llvm-as`
  验证输出，并计算 HType/eval 文件 SHA-256。
- `add_baseline_comparison()`（约第 402-448 行）比较输入、merge-eval oracle、
  HType 文件和 wall/RSS 百分比变化。
- `main()`（约第 535-687 行）用 `ThreadPoolExecutor` 并行项目，输出 `report.json`
  和 `summary.txt`。多线程运行以 merge-eval 为硬门槛；`--threads 1` 时所有类型
  文件哈希也作为硬门槛。

验证：

- `python3 -m py_compile scripts/run-source-ir-perf-smoke.py`：通过。
- fortune 单例：`0.61s`、`153.5 MiB`、`bad_unions=0`、LLVM 22 verifier 通过。
- fortune 单线程严格 A/B：HType、eval、verifier 一致。
- fortune/ffplay/vsftpd 三项目并行（3 jobs、每项目 1 thread）：suite wall `8.83s`；
  第二次 A/B wall 变化 `+0.0%/+0.6%/-0.7%`，RSS 变化均小于 `0.3%`，全部类型
  产物和 eval 一致。

当前基线目录：

- `/tmp/notdec-source-ir-perf-small-baseline-20260816`
- `/tmp/notdec-source-ir-perf-small-compare-20260816`

Redis 仍在原 opaque-body run 的后处理阶段，尚未启动包含 memcached/ngircd 的完整
基线；性能实验应使用相同脚本参数重新建立基线。

## 实验结果与收尾

### Redis perf attach

原 opaque-body 进程 `3707891` 于 2026-08-16 运行 `2:12:42` 后仍未完成，日志中的
simplify 展开计数到 `go_calls=7,000,000`，峰值 RSS 为 `44,479,916 KiB`。在其 8
线程 simplify 阶段采样：

- `/tmp/perf-redis-opaque-20260816.data`：`PersistentSet::operator==` 19.5%、
  `std::_Rb_tree_increment` 14.76%、`CompactType::operator==` 14.19%、
  `applySimplificationPlan` 8.30%、`PersistentSet::iterator::pushLeft` 7.18%。
- dwarf 重采样 `/tmp/perf-redis-opaque-20260816-dwarf.data`：
  `PersistentSet::operator==` 27.36%、`CompactType::operator==` 19.48%、
  `std::_Rb_tree_increment` 18.49%。

这说明主要开销是跨 per-group arena 的精确结构比较和持久有序集合遍历，不是 LLVM
输出或 verifier。旧进程在采样后结束，避免与后续实验争抢约 44 GiB 内存。

### 性能候选 A/B

在 `external/binarysub/src/binarysub.cpp` 的 `coalesceCompactType()` 两个跨 arena
memo 查重点，曾加入仅限单次调用生命周期的 `(memo CompactType*, local CompactType*)`
精确比较缓存，并用 `NOTDEC_SIMPLIFY_DIAG` 记录命中率。memcached 快速 A/B 结果：

- 8 threads：旧版 `22.13s/396.3 MiB`，实验版 `22.89s/398.2 MiB`，wall `+3.4%`，
  equality lookup 仅 16 次、命中 1 次，merge-eval 一致。
- 1 thread：旧版 `23.49s/378.9 MiB`，实验版 `23.69s/378.9 MiB`，wall `+0.9%`；
  `ValueTypes` 和 merge-eval 一致，HType 差异仅为匿名 `rec_*` 编号变化。

该候选没有稳定收益，且会增加共享/编号顺序变化，因此已从 binarysub 子模块撤回，
没有留下性能代码改动。Redis 不再进行同一候选的全量长跑。

### 验证与判断

- `scripts/run-source-ir-perf-smoke.py`：`py_compile`、`--list-cases` 通过；
  fortune/ffplay/vsftpd 并行严格 A/B 的 wall 变化为 `+0.0%/+0.6%/-0.7%`，RSS
  变化小于 `0.3%`，HType、eval、LLVM 22 verifier 全一致。
- `cmake --build ./build-relwithdebinfo-20260731 --target binarysub -j4` 通过。
- 默认五项目最终 smoke（`jobs=2`、`threads=1`）通过：fortune `0.60s`、ffplay
  `4.97s`、vsftpd `5.96s`、ngircd `7.79s`、memcached `23.30s`，suite wall
  `38.07s`；LLVM 22 verifier、merge-eval 和严格类型产物均完整。
- 现有 `build-relwithdebinfo-20260731/binarysub` parsing sample 4 仍失败：期望
  `⊤ -> ⊤ -> ⊤ -> {}`、实际 `⊤ -> … -> … -> {}`；该失败发生在本次缓存候选未涉及
  的 parsing 测试路径，不能作为候选收益依据，后续应单独修复或更新测试预期。

本次实现效果评分 8/10（快速反馈已覆盖性能、内存、verifier、oracle 和类型产物），
理解成本 3/10（脚本约第 30-687 行，流程集中），后期维护成本 3/10（只依赖现有
`/usr/bin/time`、LLVM 22 和 workdir 产物）。下一轮优化应先围绕 `PersistentSet`/`CompactType`
相等比较设计更低开销的统计或缓存，并先在该脚本的单线程严格 A/B 上证明收益，再进入 Redis。

## 大样例复验（lighttpd）

脚本的 `CASE_INPUTS`（第 35-45 行）本来就保留了 lighttpd、tmux、redis-server；本轮
用 `--cases lighttpd` 把 lighttpd 加入性能口径，未把它们放进默认快速集合。lighttpd
有 80,111 个 simplify group，适合验证小样例看不到的跨 arena 比较复用。

使用旧版和之前保留的 equality-cache 实验 binary，均固定 `jobs=1`、`threads=8`、
`NOTDEC_SIMPLIFY_DIAG=1`，结果如下：

- 首轮旧版 `88.88s/1911.5 MiB`、实验版 `76.70s/1414.9 MiB`，但旧版复跑为
  `78.13s/1355.8 MiB`，说明首轮是运行离群值。
- 第二轮旧版 `78.13s/1355.8 MiB`、实验版 `80.61s/1572.2 MiB`，实验版反而
  `+3.2% wall/+16.0% RSS`。实验版在最大 group 累计 `1,563,932` 次 equality
  lookup、命中 `1,057,997` 次，说明候选确实被充分触发，但没有转化成收益。
- 单线程严格复验：旧版 `161.05s/1898.4 MiB`、实验版 `157.99s/1897.5 MiB`，
  wall 仅 `-1.9%`、RSS 基本不变；merge-eval 的 `bad_unions=20`、fragmentation
  `361` 一致，但 `ValueTypes`、`ValueHTypes`、`ImportantHTypes`、`VarOrigins`
  均不一致。

结论：lighttpd 已经证明“样例太小”不是唯一原因。当前 equality cache 即使在百万级
命中下也没有稳定收益，并且会改变类型产物的共享/编号结果，继续跑 tmux 或 Redis
没有足够依据。该实验 binary 只存在于 `/sn640/NotDec/build-equality-cache-20260816`
构建目录，binarysub 子模块和主线源码均未加入这段代码。后续应先寻找不需要为每次
比较维护哈希表的 `PersistentSet`/`CompactType` 优化，再用 lighttpd 单线程严格 A/B
作为第一道门槛。

## DAG 相等比较实现与 Redis 复验

此前否决的是“跨多次比较缓存结果”的 heap hash table；本轮改为只在一次比较内部
去重 DAG 节点对，不改变跨调用共享和遍历顺序：

- `external/binarysub/src/binarysub.cpp:4533-4602` 新增
  `CompactTypeEqualityScratch`，使用 thread-local 16K 定长开放寻址表和待检查栈。
- `external/binarysub/src/binarysub.cpp:4604-4677` 新增
  `compactTypeDAGEquals()`；共享子节点对只检查一次，容量不足回退原精确比较。
- `lookupSharedMemo()` 和 `storeSharedMemo()`（约第 4991、5026 行）改用该比较。

验证结果：

- lighttpd 8 线程：旧版 `78.13s/1355.8 MiB`，新版 `74.22s/1172.4 MiB`；wall
  `-5.0%`、RSS `-13.5%`，coalesce 累计 `30.303s -> 12.709s`，最慢 group
  `8.596s -> 1.564s`。eval 与 LLVM verifier 一致。
- lighttpd 单线程：`161.05s -> 161.03s`，总 wall 持平；coalesce
  `11.482s -> 9.361s`。eval 一致。
- 默认五项目 fixed-scratch A/B：`ValueTypes.txt`、`VarOrigins.txt` 全部一致，
  eval、verifier 全部一致；其余 HType 差异只有已有的匿名类型声明编号/顺序变化。
- 临时双算校验同时执行新旧比较并在不一致时 abort；fortune、ffplay、vsftpd、
  ngircd、memcached 和完整 lighttpd 均未发现分歧。验证代码已删除。

Redis 用较早的 heap-scratch 同算法版本运行 `2:14:06` 后终止，峰值 RSS
`44,017,948 KiB`。`recvars=110` 巨型 group 到 `go_calls=13M`，旧版同时间约
`7M`，展开吞吐约提高 1.8 倍，但 44 GB 内存爆炸仍在。perf
`/tmp/perf-redis-dag-eq-rec110-20260816.data` 显示旧的 CompactType 相等热点已经
消失，约 93% 转到 PathMemo 的哈希表查找，其中 `FoldedKeys.count(PathKey)` 对应
的 set 查找占 54.8%，活动路径 `inProcess.find` 约占 31%。

否决实验：给 `PersistentSet` 加元素指针快路径/`CompactVarSet::sameRootAs()` 后
lighttpd 单线程慢 0.4%；把精确交集条件改成只比较交集计数会改变 shared hit 和
类型结果，因为 `FoldedKeys ⊆ SubtreeKeys` 并非恒成立。两项都已撤回。

本轮实现效果 8/10（跨 arena DAG 比较明显降时降内存且保持精确语义），理解成本
4/10（新增一个局部、定长 scratch），维护成本 3/10（仅两个调用点，满表有保守
回退）。更好的下一步不是继续修改相等比较，而是先给 PathMemo 增加集合规模、变体
数和校验查询数诊断，再减少 `P ∩ SubtreeKeys == FoldedKeys` 的重复成员查询。

# 用户原始 prompt

> 那试一下这个memchached
>
> 等一下，当前遇到的UAF问题是什么情况，有没有修复
>
> 怎么做比较好呢？这样做是否有什么负面作用？
>
> 其次，思考其他部分是否存在同样的问题？
>
> 那修复一下试试
>
> memcached，有什么办法profile一下内存消耗，考虑如何减少内存占用吗？必须要根据测量出来的数据
>
> 一般用什么？heaptrack 或 Valgrind能用apt安装的话都可以安装一下。直接用经常用的策略，没必要为了安装工具而妥协。
>
> 继续跑memcached的内存profiling，好像是valgrind Massif跑得特别慢，那能不能提前停止获得一部分结果呢？Heaptrack已经有初步结果了吗？可以先分析一下
>
> 那只完成简单的第一阶段吧，继续吧
>
> 怎么采样到更后面总内存更大的情况？
>
> 继续

# 背景

libuv 是通用跨平台运行时库，whole-library IR 中包含大量文件、网络、线程和系统接口包装，
不适合作为下一轮判断多态 SCC 边界的主要样本。Bench2 按实际 ELF 大小在 libuv 之后的目标是
`/usr/bin/memcached`（262,344 B），对应 Ubuntu Noble `memcached 1.6.24-1build3`。Bench2
已保存匹配 build-id 的 dbgsym，DebugInfo 指向同版本源码，但 `source-ir` 尚没有该目标的源码和
whole-program LLVM IR。

# 目标

用现有 LLVM 22、wllvm、`-g -O2` 口径构建 memcached 自身的 whole-program bitcode，并确认
DebugInfo、链接范围和 LLVM verifier 有效。随后以固定 8 线程运行类型恢复与 DebugInfo merge-eval，
检查多态函数是否保留独立边界，记录耗时、峰值内存、coverage、wrong merge 和 fragmentation。

# 路线

获取与 Bench2 包一致的 Ubuntu Noble source package 和 Debian 补丁，先确认构建系统及依赖。构建时
只链接 memcached 自己的源文件；libevent、SASL、OpenSSL 和 libc 仍保留为外部声明，避免把动态依赖的
实现混入评估模块。产物写入 Bench2 `source-ir/src`、`ir/memcached` 和 `logs`，并更新 source-ir
manifest 与 README。

运行前从源码和 IR 核对 allocator/free、item/value buffer、网络读写、协议解析、libevent callback
arg 和 `void *` context。仅将确实跨调用点承载不同布局的接口标为多态；固定的 memcached item、连接和
线程结构体路径保留跨调用约束。评估沿用 `--tr-level=2`、结构体指针 load/store 合并、fast workdir 和
独立 merge-eval 目录。

# 风险和判断标准

- 源码版本、Debian 补丁和构建配置必须与 `1.6.24-1build3` 对齐；本地 rebuilt ELF 不要求复现原 build-id。
- memcached 有 libevent callback、线程和网络 buffer；需要区分协议 buffer 的通用性与固定 `conn`、`item`
  等业务结构，不能因为存在 `void *` 就整体放宽。
- 当前高 level SCC 修正若在 memcached 上触发 binarysub 断言，先定位并修复该运行时问题，再解释性能或
  merge-eval 结果。
- 最终 IR 必须有可用 `DICompileUnit` 和 struct DebugInfo，并通过 LLVM 22 assembler/verifier；wrong
  merge 必须优先清零或逐条解释，不能用降低 fragmentation 掩盖错误合并。

# 实现记录（SCC 探针与 UAF 修复已完成）

- Bench2 `source-ir` 已加入 memcached 的源码、构建记录、`ir/memcached/memcached.bc` 和
  `ir/memcached/memcached.ll`，并更新外部目录的 manifest/README。BC SHA-256 为
  `b660fb973f7389755aea7bb2f6981f83d2ef3f4da056778b7da111861d9008fb`，LL SHA-256 为
  `987fc46ee295b4b984bee4f460c59b17c38d4293ee112e9480f1f9027494e0d0`。IR 有 401 个定义、28 个
  `DICompileUnit`、538 个 struct DebugInfo。
- `test/type-recovery/realworld/support/memcached.source.summary.json:1-22` 标记 `calloc`、cache/slab
  allocator/free、`pthread_create`、`event_set` 和运行时布局由参数决定的 socket/buffer API。`tcp_*`、
  `ssl_*`、`restart_*` 仍保持单态：它们在 memcached 内部使用固定 `conn` 或固定数据结构。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6317-6374` 的 function-only Tarjan 避开 synthetic
  CallGraph 节点；`src/TypeRecovery/mlsub/MLsubGenerator.cpp:6552-6658` 保证多态 SCC 不会被 same-level
  union 吞进单态邻居组；`src/TypeRecovery/mlsub/MLsubGenerator.cpp:6701-6705` 在分组完成后 flush
  `SCCs.txt`，使被停止的大样本也能留下分区证据。
- 带 memcached override 的 SCC 探针写入
  `/tmp/notdec-source-memcached-poly-probe-work/SCCs.txt`：level 0 只有一个 533-node 组；level 1 有
  34 组、35 个节点，最大组仅 `do_slabs_newslab,do_slabs_free` 两个节点；level 2 的
  `malloc`、`free`、`realloc` 都是单节点组。`event_set`、`pthread_create`、socket API、cache/slab API
  也各自独立于 level 1。故多态函数确实形成了更高 level 的独立 SCC，而不是留在 level 0 大组里。

## `extrude()` UAF 修复

memcached 的单线程 ASan 运行在 18.30 秒稳定报 `heap-use-after-free`。读栈位于
`binarysub::extrude()` 入口，释放栈是递归 extrusion 通过 `VariableState::addUpperBound()` 扩容源变量的
bound vector。根因是 `extrude()` 遍历一个方向的 bounds 时，嵌套函数参数会切换极性，并向同一个变量的
对应 vector 追加 bridge；扩容后，外层迭代器和作为递归实参的 vector 元素引用都已失效。负极性路径对称。

- `external/binarysub/include/binarysub/binarysub-core.h:598-601` 和
  `external/binarysub/src/binarysub-core.cpp:790-882`：`extrude()` 按值持有 pointer-sized `SimpleType`；进入
  每个极性分支时记录初始 bound 数量，每轮递归前再按值取出当前元素。这样递归扩容不会留下悬空引用，
  同时本轮仍只复制进入 extrusion 前已有的 bounds，不把新 bridge 重新卷入本轮。实现只需 O(1) 额外空间，
  没有复制整条 bound vector。
- `external/binarysub/src/binarysub-test.cpp:1329-1387` 新增
  `test_extrude_bound_snapshot()`，分别覆盖正、负极性。测试把输入 vector 填到 `size == capacity`，再构造
  含源变量的递归函数，保证反极性递归追加 bridge 时发生扩容；同时检查旧 bounds 被复制一次、bridge
  方向正确。`external/binarysub/include/binarysub/binarysub-test.h:18` 和
  `external/binarysub/src/binarysub-test-main.cpp:47` 接入测试。

没有改成 `deque`，也没有预留一个猜测容量：前者会改变所有 bound 遍历的内存布局，后者不能保证深层递归
不会再次扩容。当前修复依赖 `extrude()` 对源 bounds 只追加、不删除或重排；该约束已写在循环前注释里。
`SimpleType` 是非 owning 的单指针 handle，按值传递和每轮元素快照只增加一次指针复制，没有引用计数和
整表复制。

审计 `external/binarysub` 中其余 lower/upper bound 循环后，没有发现第二处相同问题：
`TypeGraphInstantiator::instantiate()` 只写新图；`merge_variable_into_impl()` 和邻居删除路径已经先复制源 bounds；
`constrain_impl()` 的相关循环只向 worklist 排队，policy merge 也延后执行；simplify/canonicalize 路径只读
bounds。因而本次没有扩大修改范围。

## 性能结论

未带项目 override 的完整基线在 11:50.26、峰值 62,956,388 KiB 时被 SIGKILL，未生成
`merge-eval-summary.json`。该次运行发生在 function-only Tarjan 修正前，不能作为修正后的性能数字。

修正后虽然去除了 synthetic 节点造成的假 raw SCC，但
`MLsubRecovery::prepareSCC()` 的 level-0 合并仍会把所有 level-0 raw SCC 放到一个 generator，当前就是
533 个节点。这是为了保持全局变量和共享 memory 的跨函数约束；直接按调用图拆开会改变语义。因此本轮没有
再用约 60 GiB 的完整运行去伪造性能结论，也还没有 coverage、wrong merge 或 fragmentation 指标。

## 内存 profiling（阶段性完成）

所有类型恢复采样都固定 `NOTDEC_BINARYSUB_THREADS=1`，使用修复后的
`build-relwithdebinfo-20260731/bin/notdec` 和 frozen stage-B bitcode。主动停止的 Heaptrack
`total memory leaked` 只表示停止时仍存活，不能当作泄漏。

- 原生 bitcode 基线运行 120 秒后由 `timeout` 停止。`/usr/bin/time -v` 的最大 RSS 为
  61,904,992 KiB（约 59.0 GiB）；同一 cgroup 的匿名内存增量从约第 15 秒开始增长，20 到 70 秒间
  大致每秒增加 1 GiB。cgroup 还包含其他进程，只用于看增长阶段，峰值以目标进程的 `time -v` 为准。
  修复后的程序没有再次出现 UAF，但完整类型恢复仍未完成。
- 修复后、保留完整 DebugInfo 的 Heaptrack 运行在 30.38 秒时主动停止。报告位于
  `/tmp/notdec-memcached-profile-20260801/heaptrack-direct-bc-fixed-30s/report.txt`：共记录
  20,105,986 次分配，峰值 live heap 为报告值 727.56M，Heaptrack 自身在内的峰值 RSS 为 996.56M。
  `external/binarysub/src/binarysub.cpp:683-799` 的 `CompactTypeArena::merge()` 调用栈占约
  611M 峰值存活分配，即约 84%；整个 `canonicalizeType()` 调用栈覆盖约 630M。
  主要对象是 `SimpleVarSet` 红黑树节点、`CompactType` 和 record map 节点。
- `external/binarysub/src/binarysub-core.cpp:1052-1089` 的 `register_nested_bound_uses_impl()`
  仍有高频分配，但峰值只保留报告值约 5.13M；`contains_variable_ref_impl()` 相关路径约有
  401 万次分配，停止时为 0 B live。它们会拖慢运行，不是持续涨到 60 GiB 的主要来源。
- 完整 DebugInfo bitcode 的 Massif 在 180 秒时仍停在 LLVM bitcode/DebugInfo metadata reader，
  没有生成 `SCCs.txt`。提前发送 `SIGTERM` 后，`massif.out` 正常可读，guest heap 峰值约
  3.93 MiB；约 979 MiB 的 host RSS 主要是 Valgrind 自身，不能当作目标 heap。这份结果只覆盖解析阶段。
- 为让 Massif 测到类型恢复核心，另用 LLVM 22 `--strip-debug` 生成 680 KiB profiling bitcode；
  401 个函数定义不变且 verifier 通过，同时关闭 merge-eval。Massif 在 33.85 秒生成
  `ValueTypes.txt`，进入 bulk simplify 后再采 60 秒，于 94.06 秒主动停止。报告位于
  `/tmp/notdec-memcached-profile-20260801/massif-nodebug-bc-fixed-partial/report.txt`：最后快照的
  useful heap 为 752,659,151 B，allocator extra 为 187,374,225 B，总计约 896.49 MiB。
  其中 `CompactType` 对象、变量 set 节点和 record map 节点合计 597,339,264 B，占 useful heap
  约 79%。停止栈在 `canonicalizeType()` 展开 bound 的位置，与 Heaptrack 结论一致。该运行只能
  归因类型恢复内存，不能用于 DebugInfo coverage、wrong merge 或 fragmentation。

当前主因不是普通内存泄漏，而是 `external/binarysub/include/binarysub/binarysub.h:280-307` 的不可变
`CompactType` 在 canonicalize 中反复合并：`CompactTypeArena::merge()` 每次复制 set/map 并创建新节点，
当前 arena 为保证 raw handle 有效，会把创建过的全部中间节点保留到整个 bulk simplify 结束。

## 第一阶段：局部 CompactType builder（已完成）

只实现简单的批量聚合，不重写 CompactType，也不做递归子 builder。

- `external/binarysub/include/binarysub/binarysub.h:441-446` 新增
  `CompactTypeArena::mergeAll()`；`external/binarysub/include/binarysub/binarysub.h:689-692` 和
  `src/binarysub.cpp:1696-1699` 增加对应的 `TypeSimplifier` 入口。
- `external/binarysub/src/binarysub.cpp:621-790` 新增局部 `CompactTypeBuilder`。它原地维护顶层
  vars/prims、size、record、function、ptrLoad/ptrStore/psize；正极性 record 仍取交集，负极性仍取
  并集，函数参数仍使用反极性，ptrStore 仍使用反极性。字段、参数和指针槽位的嵌套冲突继续调用
  原 `CompactTypeArena::merge()`，因此没有改变递归类型语义。每次 append 后执行和旧 `make()/merge()`
  相同的 direct-pointer/field-0 归一化，最后只在 `freeze()` 生成一个 arena-owned 不可变结果。
- `src/binarysub.cpp:1853-1869` 将 `canonicalizeType()` 中的
  `empty -> merge(bound) -> merge(bound)` left-fold 改为先收集非变量 bounds，再调用 `mergeAll()`。
  空输入仍返回 empty；新增记录字段在此前交集为空后仍可重新出现。
- `include/binarysub/binarysub-test.h:13`、`src/binarysub-test-main.cpp:42`、
  `src/binarysub-test.cpp:508-580` 新增结构等价回归，覆盖空输入、正负极性、record 交集、直接指针
  折入字段 0、函数参数逆变和 ptrStore 逆变，并逐项对比旧 left-fold 结果。

### 第一阶段测量结果

使用同一 frozen stage-B bitcode、同一单线程设置和同一 Heaptrack 采样口径：

| 指标 | 旧版 | builder 版 | 变化 |
| --- | ---: | ---: | ---: |
| `CompactTypeArena::make()` 调用 | 688,954 | 389,124 | -43.5% |
| `CompactType` 构造峰值分配 | 176.37M | 99.62M | -43.5% |
| Heaptrack peak live heap | 727.56M | 636.89M | -12.5% |
| Heaptrack peak RSS | 996.56M | 911.89M | -8.5% |
| 原生目标 RSS（阶段采样） | 61,904,992 KiB（约 59.0 GiB，120 s） | 62,319,308 KiB（约 59.4 GiB，80 s） | 未下降 |

builder 确实减少了 canonicalize 的早期 `CompactType` 中间对象，但在这两个不同停止时刻的原生采样中，
目标进程仍达到约 59 GiB；这不是严格同一时刻的性能 A/B，不能声称最终峰值有改善。
Heaptrack 中剩余主要分配仍来自 `CompactTypeBuilder::mergeRecord()` 调用的旧递归
`CompactTypeArena::merge()`，也就是重叠 record/function/pointer 子槽位的二元合并，以及多个
canonicalize 状态分别保留大结果。不能把这次局部收益解释为已经解决总体峰值。递归子 builder、结果缓存和
arena 生命周期重做留到后续阶段，本次不实现。

## 16 GiB 后段采样（已完成）

Heaptrack 支持运行中附加，但本机 `kernel.yama.ptrace_scope=1` 不允许同级进程 ptrace。本轮没有修改系统
安全设置，而是安装 Ubuntu `libjemalloc-dev 5.2.1-4ubuntu1`，用 jemalloc 自带的采样 profiler 从进程启动
时记录调用栈：

```text
MALLOC_CONF=prof:true,prof_active:true,lg_prof_sample:19,lg_prof_interval:30,prof_prefix:<dir>/jeprof
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2
```

仍使用同一 frozen stage-B bitcode、修复后的 RelWithDebInfo `notdec` 和单线程 binarysub。外层每 100 ms
读取目标进程 RSS，每秒读取 `smaps_rollup`，RSS 达到 16 GiB 时发送 `SIGTERM`。结果在
`/tmp/notdec-memcached-jemalloc-late-20260801`：共 16 个 heap dump，最后一个是
`jeprof.1683424.15.i15.heap`。jemalloc 改变了 allocator，所以它只用于归因；最终绝对峰值仍需用原生
allocator 校准。

运行在 27.999 秒达到 16,893,572 KiB RSS 后停止；最近一次 PSS 是 16,235,304 KiB，private 是
16,234,668 KiB，swap 为 0。20.073 到 27.999 秒的 RSS 从 7,400,624 KiB 增至 16,893,572 KiB，仍以
约 1.142 GiB/s 增长。dump 使用 512 KiB 采样率，逐栈按 jemalloc/jeprof 的 Poisson 公式校正。最后 dump
的 live heap 为 16,079,792,680 B（14.975 GiB），和同一时段 PSS 接近，因此已经覆盖后段主要私有内存，
不是早停 Heaptrack 的亚 GiB 局部结果。

| dump | 约经过时间 | 校正 live heap | `SimpleVarSet` 的 `std::set` 红黑树节点 | `CompactType` 本体 |
| --- | ---: | ---: | ---: | ---: |
| i4 | 17.71 s | 4.101 GiB | 3.291 GiB | 0.670 GiB |
| i8 | 21.10 s | 8.088 GiB | 6.270 GiB | 1.632 GiB |
| i12 | 24.54 s | 12.044 GiB | 9.212 GiB | 2.567 GiB |
| i15 | 27.11 s | 14.975 GiB | 11.397 GiB | 3.297 GiB |

最后 dump 的主要存活对象如下。估算对象数按对应 jemalloc size class 计算，只用于说明数量级。

| 分配对象 | 校正存活量 | 占 live heap | 估算数量 |
| --- | ---: | ---: | ---: |
| `SimpleVarSet` 的 `std::set` 红黑树节点 | 11.397 GiB | 76.10% | 约 2.55 亿个 48 B 节点 |
| `CompactType` | 3.297 GiB | 22.01% | 约 1383 万个 256 B 对象 |
| arena 的 `types` vector 容量 | 0.125 GiB | 0.83% | 16,777,216 个指针槽位 |
| record map 红黑树节点 | 0.037 GiB | 0.24% | 约 49 万个 80 B 节点 |

`SimpleVarSet` 的 `std::set` 分配栈还能继续拆开。这里的 `SimpleType` 只是指向 `TypeNode` 的指针大小
handle；profile 中的“节点”是 `std::set` 为保存这个 handle 分配的红黑树节点，不是新的 `TypeNode`：

| 路径 | 存活量 | 占总 live heap |
| --- | ---: | ---: |
| `CompactTypeBuilder::mergeRecord()` 第 711 行的重叠字段递归 merge | 6.205 GiB | 41.44% |
| `normalizeDirectPointer()` 第 771 行把 direct pointer 反复并入 field 0 | 3.355 GiB | 22.40% |
| `mergePointerSlot()` 第 750 行的嵌套 merge | 1.535 GiB | 10.25% |
| 其余 `SimpleVarSet` 路径 | 0.302 GiB | 2.02% |

这些路径最后都落到 `CompactTypeArena::merge()` 第 871-872 行：先完整复制 lhs 的 vars set，再插入 rhs；
随后第 839-845 行创建新的 `CompactType` 并把它放进 arena。第一阶段 builder 只消除了第 1698、1869 行
顶层 bounds left-fold 的前缀，子字段冲突仍调用旧二元 merge。时间序列也给出了优先级：pointer-slot 路径
在 live heap 约 4 GiB 时已经接近 1.53 GiB，之后基本不增长；record 路径则从 1.36 GiB 继续长到
6.21 GiB，field-0 归一化从 0.25 GiB 长到 3.36 GiB。

因此下一阶段不应先优化 record map 容器本身，它在 15 GiB 时只有约 37 MiB。更直接的候选是让 record
子字段和 field-0 也在同一次 `mergeAll()` 中保持可变累积状态，最后再 freeze，避免每次字段冲突都复制
整棵 `SimpleVarSet` 并生成 arena-owned `CompactType`。这比只改 pointer slot 更可能降低后段峰值，但会涉及
嵌套 polarity、递归类型和逐步 direct-pointer 归一化语义，属于第二阶段，本轮不实现。

## 验证

- LLVM 22 `llvm-as` 与 `opt -passes=verify` 验证 memcached IR 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `./build/binarysub`：全部通过，包含新增的正、负极性扩容回归。
- g++-14 Debug + ASan 独立构建的 `binarysub`：全部通过，未报告 sanitizer 错误。
- `./build/bin/MLsubGeneratorTest`：18/18 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- 修复后的 memcached 原 frozen stage-B 输入固定单线程运行 90 秒，退出码 124 仅来自主动超时，stderr
  没有 ASan/UAF；旧版在同一路径 18.30 秒即报 UAF。该结果只证明越过原崩溃点，完整类型恢复仍未完成，
  不能作为性能或内存结论。
- 一次包含 realworld suite 的 CTest 中，fortune 因 `ir_anchor.sha256` 与当前输入不匹配而在类型恢复前失败；
  这是已有测试数据问题，与本次 UAF 路径无关。相关 LLVM IR suite 已单独重跑并通过。

## 复杂度和维护成本评估

- 实现效果：8/10。消除了 CallGraph synthetic 节点导致的假 SCC，并直接在 memcached 上确认高 level
  多态边界。
- 理解成本：5/10。分区现在有 raw SCC、same-level group 和收缩图三个阶段，代码注释明确了每一步的
  调用方向和原因。
- 后期维护成本：5/10。Tarjan 只遍历有 `Function` 的节点；新增 CallGraph 节点类型时需要保持该过滤，
  但不依赖 LLVM 内部 synthetic 节点的具体连边。
- UAF 修复效果：9/10。ASan 的释放栈直接对应 vector 扩容，稳定小回归和 memcached 原路径均不再复现。
- UAF 修复理解成本：2/10。只记录初始长度并在递归前复制一个 handle。
- UAF 修复维护成本：2/10。需要继续保持 extrusion 对源 bound vector 只追加；若以后允许删除或重排，
  这里应改为整表快照。
- 第一阶段 builder 实现效果：7/10。减少了 43.5% 的 `CompactType` 构造和约 12.5% 的采样 live heap，
  但没有降低 memcached 的最终原生峰值；它是有测量支持的局部优化，不是完整解决方案。
- 第一阶段 builder 理解成本：4/10。新增一个只在 `mergeAll()` 生命周期内存在的可变顶层聚合器，
  nested merge 仍复用旧实现，边界清楚。
- 第一阶段 builder 维护成本：4/10。需要保证 builder 的字段规则与 `CompactTypeArena::merge()`
  同步；因此测试保留了旧 left-fold 结构等价对比。

## 第一阶段验证（2026-08-01）

- `cmake --build build --target binarysub notdec TypeBuilderTest -j4`：通过。
- `./build/binarysub`：通过，包含 `test_compact_type_builder_merge_all`。
- `./build/bin/TypeBuilderTest`：9/9 通过。
- `./build/bin/MLsubGeneratorTest`：18/18 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- binarysub 工作树 `git diff --check`：通过。

## field-0 builder 重跑（2026-08-02）

使用当前 field-0 builder 版本、同一 frozen stage-B 输入和单线程 binarysub，重新跑到 RSS 约 16 GiB：

```text
/tmp/notdec-memcached-field0-builder-20260802-16g-055911
```

运行 49.425 s 后主动发送 `SIGTERM`，RSS 为 16,856,112 KiB，最近一次 PSS 为 16,854,742 KiB；共生成
24 个 jemalloc dump，最后一个是 `jeprof.2475165.23.i23.heap`。`jeprof` 使用固定的
`/opt/addr2line` 完整符号化成功，耗时 1:00.27，报告为
`/tmp/jeprof-gimli-field0-16g.txt`，校正后的最后 dump live heap 为 16,970,871,387 B（15.805 GiB）。

当前 live heap 的调用树累计归因仍集中在 direct-pointer 和不可变集合复制；这些不是进程生命周期的累计分配量：

| 路径 | 调用树累计 live heap |
| --- | ---: |
| `normalizeDirectPointer` | 8.618 GB（50.8%） |
| `CompactTypeArena::makeDirectPointer` | 7.855 GB（46.3%） |
| `CompactTypeBuilder::mergeRecord` | 1.471 GB（8.7%） |
| `mergePointerSlot` | 0.743 GB（4.4%） |
| `CompactTypeArena::mergeAll` | 12.365 GB（72.9%） |

上一轮 record-builder dump 的校正 live heap 为 16,114,838,378 B；两轮均以 RSS 16 GiB 停止，但使用的
可执行文件和监控脚本不完全相同，因此只能作方向性参考。当前结果没有证明峰值下降，且
`normalizeDirectPointer` 仍是首要优化目标；field-0 仍在反复创建不可变 direct-pointer 节点。完整命令和
RSS/PSS 时间序列保留在上述目录中。

### 指标和释放语义

最新 heap 文件的 `heap_v2` 记录使用 `curobjs:curbytes` 表示当前存活采样对象；本轮没有设置
`prof_accum:true`，所以每条记录的 `[cumobjs:cumbytes]` 都是 `[0:0]`。`jeprof` 默认显示
`--inuse_space`，表格中的 `cum` 是调用树的包含子调用累计值，不是进程生命周期累计分配量。因此
`mergeAll` 包含其内部的 `normalizeDirectPointer`、`makeDirectPointer` 和最终 `make`，这些百分比不能相加。
当前采样在对象释放后会从 `curbytes` 中扣除，但它看不到已经分配又释放的瞬时峰值；RSS/PSS 还会保留
jemalloc 的 arena 元数据和未归还给内核的页。

`CompactTypeArena::types` 保存每个 `make()` 产生的 `unique_ptr<CompactType>`，`CompactTypePtr`/`TypeRef`
只是非拥有指针。`merge()` 产生新节点后，旧节点及其 record/set 容器不会按引用计数回收，只有 arena
整体 `clear()` 或析构才释放。因此被合并节点的子节点指针本身通常只有指针大小，真正占空间的是旧
`CompactType` 对象以及复制出的 `SimpleVarSet`/`SimplePrimSet` 红黑树和 record map 节点；此前 15 GiB
dump 已量到约 11.4 GiB 的 `SimpleVarSet` 节点和约 3.3 GiB 的 `CompactType` 对象。要继续降峰值，
优先应消除 `normalizeDirectPointer()` 中反复 freeze 后调用 `makeDirectPointer()` 的路径，再考虑
扩大可变 builder 的覆盖范围或按安全生命周期拆分 arena。

## 扩大 mutable builder（2026-08-02，已完成）

本轮把第一阶段 builder 从顶层 bounds 扩大到 record 子字段、field-0 和 direct-pointer 的 load/store
累积。`external/binarysub/src/binarysub.cpp:626-769` 的 `CompactTypeSlot` 和
`CompactTypeBuilder` 支持移动合并私有状态；`mergeRecord()`（827-902）在同级字段冲突时直接合并子
slot，`normalizeDirectPointer()`（971-994）把 pointer load/store slot 移入 field 0，只有最终
`freeze()`（772-808）才生成不可变 arena 节点。`CompactTypeArena::mergeAll()`（1250-1257）仍保持
左到右语义。`external/binarysub/include/binarysub/binarysub.h:443-451` 增加 `nodeCount()`，
`external/binarysub/src/binarysub-test.cpp:621-681` 增加未规范化 record/direct-pointer 输入，分别检查正、
负极性与旧 left-fold 等价，并确认最终只发布 5 个节点。

同一 frozen stage-B 输入、同一 RelWithDebInfo 可执行文件和单线程下，jemalloc 16 GiB 截止采样为
`/tmp/notdec-memcached-expanded-builder-20260802-yBU1cH`，到达 16 GiB 用时 69.744 s；最后 live heap
为 16,931,255,366 B。`makeDirectPointer` 不再出现在主要存活调用栈，`normalizeDirectPointer` 和
`appendDirectPointer` 各约 1.181 GB（7.0%），`CompactTypeArena::merge` 约 4.196 GB（24.8%），
`mergeRecord` 约 2.970 GB（17.5%）。与旧 field-0 builder 在同一 RSS 阈值的 48.4 s 相比，新版为
69.7 s；这是路径分配减少但总体运行变慢，剩余主因已集中到 `SimpleVarSet` 的 set 节点复制，不能把
局部 direct-pointer 收益当作峰值已经下降。

## Profiling 脚本（2026-08-02，已完成）

新增 `scripts/profile-memcached-memory.sh:11-169`，封装当前 profiling 命令，默认使用
`/tmp/notdec-memcached-stage-b-20260801.bc`、`build-relwithdebinfo-20260731/bin/notdec`、单线程、
16 GiB RSS 阈值和 jemalloc 采样；`--native` 可切换回原生 allocator。脚本接受 `--input`、`--notdec`、
`--output-dir`、`--threshold-gib` 和 `--threads`，并把完整展开后的命令写入 `command.txt`。

`scripts/profile-memcached-memory.sh:165-249` 用 `setsid` 建立独立进程组，沿 `/proc` 子进程树找到真正的
`notdec` PID 后读取 `smaps_rollup`，避免只监控 `/usr/bin/time` 外壳；达到阈值时同时终止外壳和目标进程。
脚本写出 `rss-pss.csv`、`events.log`、`pid`、`target-pid`、`exit-code`、标准输出/错误和 work/eval 目录；
主动达到阈值时脚本返回 0，但保留目标进程的实际信号退出码。native 模式会清除继承的 `MALLOC_CONF` 和
`LD_PRELOAD`，避免 allocator 口径被环境变量悄悄改变。

验证：`bash -n scripts/profile-memcached-memory.sh`、`--help`，以及用 `/bin/echo` 分别跑 native 和
jemalloc smoke test 均通过；`git diff --check` 通过。此前手工 native 命令监控了 `time` wrapper 而不是
目标进程，未在 16 GiB 停止，最终约 62,778,100 KiB RSS 后以 137 结束；该目录
`/tmp/notdec-memcached-expanded-native-20260802-ovEdiJ` 只作失败的过程记录，后续应使用新脚本。

本轮复杂度评估：实现效果 8/10（固定命令和停止口径可复用，且修正了监控 PID 问题）；理解成本 3/10（参数、
进程组、CSV 输出均在一个脚本内）；维护成本 3/10（默认路径仍可通过参数覆盖，新增 allocator 时只需扩展
环境数组和帮助文本）。

## expanded builder 后段复测（2026-08-02）

用新脚本运行同一 frozen stage-B 输入、同一 RelWithDebInfo 可执行文件、单线程和 16 GiB RSS 阈值：

```text
scripts/profile-memcached-memory.sh \
  --output-dir /tmp/notdec-memcached-profile-latest-20260802
```

脚本在 69.423 s 主动停止，RSS 为 16,837,748 KiB，PSS 为 16,834,234 KiB；最后 heap dump 是
`jeprof.2571339.29.i29.heap`。`/opt/addr2line` 生成的
`/tmp/jeprof-gimli-latest-16g.txt` 显示 live heap 为 16,753,245,533 B（15.603 GiB）。上一版 field-0
builder 在相近 RSS（16,856,112 KiB）下为 49.425 s、16,970,871,387 B（15.805 GiB）。因此当前能确认的是：
到达同一 RSS 阈值慢了 40.5%，最后采样 live heap 小了约 207.5 MiB（1.28%）；不能据此声称最终峰值已下降。

direct-pointer 局部优化有效且稳定：`makeDirectPointer` 已不在主要 live 调用树中；
`normalizeDirectPointer` 从 8.618 GB（50.8%）降到 1.157 GB（6.9%），约少 86.6%。但低层分配从
`std::make_unique`（11.127 GB -> 6.244 GB）转移到 `std::__new_allocator::allocate`
（5.810 GB -> 10.467 GB），后者主要是 `SimpleVarSet` 的 `std::set` 红黑树节点，说明问题已从反复发布
direct-pointer `CompactType` 转为复制/保存变量集合。

最新行级 profile 给出了当前热点的确切位置，以下 cumulative 数值在调用关系上会重叠，不能相加：

| 位置 | live heap | 含义 |
| --- | ---: | --- |
| `CompactTypeBuilder::append()` 第 689 行 | 3.601 GB | 对每个输入的 `rhs->vars` 做 `std::set::insert`。 |
| `CompactTypeBuilder::mergeRecord()` 路径 | 2.939 GB | record 子字段递归累积；其中会再调用 child builder。 |
| `CompactTypeArena::merge()` 第 1148 行 | 2.197 GB | `SimpleVarSet vars = lhs->vars` 的整集合复制。 |
| 同一 `merge()` 第 1185 行 | 0.454 GB | record map 的赋值复制，不是首要问题。 |
| 同一 `merge()` 第 1245 行 | 1.506 GB | 复制后发布的不可变 `CompactType`。 |
| `canonicalizeType()` 第 2205 行 | 3.451 GB | 用 `res->vars`、`res->prims` 按值重建 adapted type。 |

`canonicalizeType()` 第 2151 行的 bounds fold 已使用 `mergeAll()`（7.477 GB cumulative），但紧接着第
2153 行仍调用二元 `mergeCompactTypes(pol, ty, bound)`（4.157 GB cumulative）。这正是上表第 1148/1185/1245
行的来源：先 freeze `bound`，再复制 `ty` 的集合、生成另一个 `res`。下一步较小的候选是先用测试证明
`merge(ty, mergeAll(bounds))` 和同一左到右 builder 内 `ty + bounds` 等价，再让两者在同一个 builder 中完成，
从而去掉该中间 `bound` 与二元 merge。它仍不能解决第 2205 行的 immutable `SimpleVarSet` 复制。

第 2205 行说明更大的后续问题：`CompactType` 的 `vars` 是
`external/binarysub/include/binarysub/binarysub.h:280-307` 中按值保存的 `const std::set`。只要 adapted type
保留相同变量集合，就必须完整复制红黑树。若小范围 builder 合并后仍是主热点，应单独设计共享的不可变
`SimpleVarSet` 表示；简单链式 union 不合适，因为查找、遍历、hash 和 equality 会随链深变慢。引用计数的
共享集合没有明显的所有权环（集合内只是 non-owning `SimpleType` handle），但会增加 shared ownership 和
API 改造成本，需先做语义/性能对照后再实施。

## CompactType 持久集合与独立 arena（2026-08-02，已完成）

本轮只改变 `CompactType` 的临时生命周期；`UType`、`HType` 的分配器和所有权没有改变。

`external/binarysub/include/binarysub/binarysub.h:285-400` 新增 `CompactVarSet`，用 LLVM
`ImmutableSet` 保存变量集合，并保留原 `SimpleVarSet` 的 `(variable id, level)` 排序和相等语义。
每个集合根用共享 `ArenaState` 保住其 AVL factory 和 `BumpPtrAllocator`，所以 adapted node 可以安全复用
旧集合根。`Storage` 中 `owner` 必须声明在 `set` 前（381-388）：成员逆序析构时先释放 set root，后释放
factory。最初顺序相反，ASan 在 source arena `clear()` 后 destination arena 仍持有集合根的回归中报 UAF；
现已修复。

`external/binarysub/CMakeLists.txt:36-45` 为 standalone build 补上 LLVM Support，顶层 build 继续复用
`notdec_llvm_deps`。`src/binarysub.cpp:667-670` 保持 builder 自己的 `SimpleVarSet` 可变，第一次导入
persistent root 时按迭代器复制，避免 builder 的每次 append 都做 AVL 路径复制。

`CompactTypeArena` 在同一头文件 544-607 改为
`SpecificBumpPtrAllocator<CompactType>`，不再保存 `vector<unique_ptr<CompactType>>`。
`external/binarysub/src/binarysub.cpp:1104-1148` 在 arena 内构造和合并 persistent var set；二元 merge
复用左侧根，只为新增变量创建 AVL 路径。1175-1228 的 `makeFromCompactVars()` / `makeImpl()` 让
`canonicalizeType()` 在 2321-2326 只改 child 时复用 `res->vars`，不再重建整个变量集合。2250-2271 还把
`bounds` 与 `ty` 放入同一个 `mergeAll()`，但严格保持旧顺序 `bounds` 在前、`ty` 在后，避免正极性 record
intersection 改变结果。3383-3390 在 `UType` 都已 materialize 后调用 `TypeSimplifier::clear()`，一次释放
CompactType、递归缓存和对应两套 arena。

`external/binarysub/src/binarysub-test.cpp:724-759` 新增跨 arena 生命周期回归；900-985 进一步检查真实
`bulkSimplifyDetailed()` 返回后 `TypeSimplifier::isClear()` 为真。测试覆盖 source/destination arena 共享
集合根、合并、清理和 serial/parallel bulk 返回；`src/binarysub-test-main.cpp:42-46` 已接入。

同一 frozen stage-B input、同一 RelWithDebInfo binary、jemalloc、单线程、16 GiB RSS 阈值的复测目录为
`/tmp/notdec-memcached-immutable-set-20260802`。旧 expanded-builder 基线是
`/tmp/notdec-memcached-profile-latest-20260802`：

| 时间 | 旧 RSS | persistent-set RSS |
| --- | ---: | ---: |
| 30 s | 4.887 GiB | 2.456 GiB |
| 50 s | 10.681 GiB | 5.157 GiB |
| 69.4 s | 16.058 GiB（停止） | 7.784 GiB |

新版本在 130.049 s 才到 16.025 GiB RSS，旧版在 69.423 s 到 16.058 GiB；相同时间点 RSS 约减半，达到固定
阈值前可多运行 87.3%。两次最后 jemalloc live heap 分别为 15.607 GiB 和 15.603 GiB，几乎相同，因为两者
都被同一个 RSS 阈值打断，不能据此声称完整运行的最终峰值已经下降。此次 memcached 没有走到
`bulkSimplifyDetailed()` 返回，因此 arena 的整块释放由单元回归验证，而非这次被主动终止的 profile。

最后 dump 的旧 `std::set` 红黑树热点已经消失；新的调用树主要是 `ImmutableSet::Factory::add` 10.182 GB
（60.8%）和 `SpecificBumpPtrAllocator::Allocate` 4.811 GB（28.7%）。两项在调用树上有重叠，不能相加。
它说明路径复制已替代整棵红黑树复制，但 builder 的 mutable `SimpleVarSet` 在最终 freeze 时仍由
`makeVarSet()` 逐元素导入 persistent set（10.863 GB cumulative）。下一步若继续，应先测量“builder 直接
构造 persistent set”的插入路径；那会把当前一次 freeze 的全量导入变成每次 append 的路径复制，不应凭直觉
直接改。

验证：`cmake --build build --target binarysub -j4`、`./build/binarysub`（含 ASan 生命周期回归）、
`./build/bin/TypeBuilderTest`（9/9）和
`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 均通过；
`cmake -S external/binarysub -B /tmp/notdec-binarysub-standalone-20260802 -G Ninja` 的 standalone 配置、
构建和 `binarysub` 全量测试也通过；
Release `notdec` 已重建并完成上述 profile，`git -C external/binarysub diff --check` 通过。
系统符号化工具已把 `/usr/bin/addr2line` 指向 `/opt/addr2line` 0.23.0，原 Binutils 链接保留为
`/usr/bin/addr2line.binutils`；完整 `jeprof` 符号化耗时 59.04 s。

本轮评估：实现效果 8/10（相同时间的在途内存约减半，并真正复用集合子树）；理解成本 6/10（集合根与
factory 生命周期需要明确维护）；后期维护成本 5/10（新 API 集中在 CompactType，不影响 UType/HType，
但必须保留跨 arena 生命周期回归）。

## vsftpd 完整 profile（2026-08-02，已完成）

为了在一个能完整结束的较小 Bench2 项目上确认 `CompactType` arena 的释放，使用
`/sn640/NotDec-Exp/Bench2/source-ir/ir/vsftpd/vsftpd.bc` 生成 stage-B 输入
`/tmp/notdec-vsftpd-stage-b-20260802.bc`。stage A 用时 2.11 s、最大 RSS 158,580 KiB；该 BC 经 LLVM 22
`opt -passes=verify` 验证，含 550 个定义函数。

`scripts/profile-memcached-memory.sh` 的注释和 usage 改为通用 source-IR profile，并新增
`--frozen-tr-input-ir`（第 26、43、95、145、174 行）。此前脚本虽然创建了 stage-B 输入，却没有把该标志
传给 notdec；默认仍是 normal input，只有显式传该选项才启用 frozen 模式，因此不会改变历史 memcached 命令。

完整运行目录是 `/tmp/notdec-vsftpd-profile-20260802`，命令使用 RelWithDebInfo `notdec`、jemalloc、
单线程和 `--frozen-tr-input-ir`。运行正常结束：wall 12.74 s，`/usr/bin/time` 最大 RSS 588,748 KiB
（约 575 MiB），RSS/PSS 逐秒采样的最高点是 9.548 s 的 571,976 KiB（约 558.6 MiB）。后者低于 time 的
峰值是采样间隔遗漏短暂峰值，不是两种内存口径冲突。merge eval 为 `wall_ms=10725`、`peak_rss_mb=574`、
`nodes_created=13124`、`representative_nodes=12217`、`merged_nodes=907`，且 `bad_unions=0`；输出
`out.ll` 已经 LLVM 22 verifier 验证，SHA-256 为
`0fe289fe5b135ecd5003940e2a45601f72482d2c8ff24b2d66a14ba5df66063c`。

jemalloc 的接近峰值阶段 dump `jeprof.2756606.4.i4.heap` 有 377,705,832 B（约 360.2 MiB）live heap，
其中调用树经过 `CompactTypeArena::make` 的 cumulative 分配为 254,589,974 B（67.4%），经过
`canonicalizeType` 的为 286,033,595 B（75.7%），经过 `ImmutableSet::Factory::add` 的为 184,602,749 B
（48.9%）。这些是同一调用树的 cumulative 值，彼此重叠，不能相加。退出 dump
`jeprof.2756606.5.i5.heap` 只剩 87,931,551 B（约 83.9 MiB），不再有显著的 `CompactTypeArena` 或
`ImmutableSet` 路径，主要是最终 UType/HType 转换。峰值阶段到退出少了约 276 MiB live heap，说明临时
`CompactType` 与 persistent set arena 在类型恢复完成后确实不再存活。退出前 RSS 仍约 542 MiB，是 jemalloc
保留已分配页的正常表现，不能把 RSS 未回落误判为 arena 没有释放。

vsftpd 因此适合作为 13 秒左右的完整回归输入，用来检查释放、输出验证和错误合并；它没有复现 memcached
的多 GiB 压力，不能代替后者评估峰值内存优化。

## memcached 函数切分与完整峰值（2026-08-04，已完成）

为了把 whole-program 在 56 GiB 阈值前仍未完成的压力对应到输入函数，从 frozen stage-B
`/tmp/notdec-memcached-stage-b-20260801.bc` 用 LLVM 22 `llvm-extract` 只保留选中函数的定义；未选中
函数保留为声明。每个切片均经过 LLVM 22 `opt -passes=verify`，再用 RelWithDebInfo `notdec`、单线程、
jemalloc 和 `--frozen-tr-input-ir` 运行。这个方法保留选中函数之间的接口约束，但不是 full memcached 的
语义等价替代物；它用于定位增长来源，不用于替换完整回归。

| 切片 | 定义数 | wall | `/usr/bin/time -v` 峰值 RSS | 结论 |
| --- | ---: | ---: | ---: | --- |
| `drive_machine` | 1 | 0.56 s | 0.15 GiB | 状态机本体不大 |
| ASCII handler 子图（不含 `drive_machine`） | 46 | 27.62 s | 3.13 GiB | 可完整结束 |
| `drive_machine` + binary 子图 | 48 | 19.02 s | 2.00 GiB | binary 分支不是触发项 |
| ASCII item core（不含 `lru_pull_tail`） | 31 | 35.33 s | 3.97 GiB | 可完整结束 |
| 上一行加 `lru_pull_tail` | 32 | 38.44 s | 4.35 GiB | 多出明显但会释放的临时峰值 |
| 上一行加 `slabs_alloc` | 33 | 45.57 s | 4.86 GiB | slab freelist 已继续放大 |
| 再加 `do_slabs_newslab`、`do_slabs_free` | 35 | 69.03 s | 7.92 GiB | 已复现主体高峰 |
| 完整 ASCII + `drive_machine` 子图 | 47 | 71.98 s | 8.11 GiB | 完整结束；此前 8 GiB 阈值恰好过早停止 |

关键调用和字段关系在 memcached 源码中很直接：

- `proto_text.c:134-` 的 `complete_nread_ascii()` 从 `conn::item` 取 `item *` 并进入存储路径；
  `memcached.c:1486` 的 `drive_machine()` 调它。
- `items.c:174-212` 的 `do_item_alloc_pull()` 交替调用 `lru_pull_tail()` 和 `slabs_alloc()`。
- `items.c:1102-1275` 的 `lru_pull_tail()` 通过 `tails[id]` 和 `item::prev/next` 遍历、摘除和重新链接
  `item`；`items.c:420-496` 的队列辅助函数也读写同一组字段。
- `slabs.c:406-447` 的内联 `do_slabs_alloc()` 从 `slabclass::slots` 取出同一个 `item`，而
  `slabs.c:498-523` 的 `do_slabs_free()` 把它重新作为 freelist 节点写回 `next/prev`；
  `do_slabs_newslab()` 在 369-403 行把新页切成这条 freelist。

因此高峰不是某个 memcached 函数在运行时分配了数 GiB，而是同一个 raw `item` 在 LRU 双向链和 slab
freelist 两种状态下复用 `next/prev`，并由 ASCII `conn -> item` 路径和分配/回收路径连到同一组约束。
这会形成真实的递归 record/union 形状，不能为了省内存把这些调用边或字段约束直接丢掉。单独的
`drive_machine`、最短 7 函数调用链、`lru_pull_tail` 加队列函数都只有约 0.16 GiB；高峰需要已有 item
子图和 slab allocation core 同时存在，故结论是函数组合而非单一坏函数。

完整 47 函数切片的 RSS/PSS 采样在 68.382 s 达 8.01 GiB，随后在类型恢复后段快速降至约
3.13 GiB；`time -v` 捕获到采样间隔遗漏的 8.11 GiB 最大值。峰值前最后一个 jemalloc dump
`jeprof.756488.26.i26.heap` 的 live heap 为 7,719,577,446 B：

- `TypedBumpArena::create/addBlock` 路径为 6.230 GiB（80.7%）。
- `canonicalizeType()` 调用树覆盖 7.202 GiB（93.3%）；`PersistentSet::Factory::add` 覆盖 2.501 GiB
  （32.4%）；`CompactTypeArena::makeImpl` 覆盖 3.729 GiB（48.3%）。

后三项是嵌套调用树的 cumulative 值，不能相加。它们和之前 whole-program profile 一致：高峰仍是
canonicalize 中的 `CompactType`、persistent set 与 arena 块，而不是最终 `UType/HType` 常驻对象。该切片
仍有 `bad_unions=1`，所以它可用于内存定位，不能在未检查该 union 前作为语义正确性的性能 golden。

`scripts/profile-memcached-memory.sh:37-264` 同时补齐 `--threshold-gib 0` 表示不设 RSS 上限，并启用
jemalloc `prof_final:true` 保留正常退出时的最后 heap dump；`bash -n` 和以 `/bin/true` 为 target 的启动/退出
smoke 均通过。后续优化应针对 canonicalize 的递归 record 展开和 persistent set root 大小做计数归因；
不要据这个切片直接切断 LRU/slab 的类型约束。

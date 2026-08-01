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

| dump | 约经过时间 | 校正 live heap | `SimpleVarSet` 节点 | `CompactType` 本体 |
| --- | ---: | ---: | ---: | ---: |
| i4 | 17.71 s | 4.101 GiB | 3.291 GiB | 0.670 GiB |
| i8 | 21.10 s | 8.088 GiB | 6.270 GiB | 1.632 GiB |
| i12 | 24.54 s | 12.044 GiB | 9.212 GiB | 2.567 GiB |
| i15 | 27.11 s | 14.975 GiB | 11.397 GiB | 3.297 GiB |

最后 dump 的主要存活对象如下。估算对象数按对应 jemalloc size class 计算，只用于说明数量级。

| 分配对象 | 校正存活量 | 占 live heap | 估算数量 |
| --- | ---: | ---: | ---: |
| `SimpleVarSet` 红黑树节点 | 11.397 GiB | 76.10% | 约 2.55 亿个 48 B 节点 |
| `CompactType` | 3.297 GiB | 22.01% | 约 1383 万个 256 B 对象 |
| arena 的 `types` vector 容量 | 0.125 GiB | 0.83% | 16,777,216 个指针槽位 |
| record map 红黑树节点 | 0.037 GiB | 0.24% | 约 49 万个 80 B 节点 |

`SimpleVarSet` 分配栈还能继续拆开：

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

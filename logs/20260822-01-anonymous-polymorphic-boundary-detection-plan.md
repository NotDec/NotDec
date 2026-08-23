对，重建然后把tmux跑起来。allockind/allocsize/allocptr/noalias 等非名字 IR 语义这些是什么？应该二进制里面是没有这些的，所以应该不能用？我们目标是二进制lift出来的IR上识别。列一个规划文件试试可能的识别多态函数的思路吧

# 无符号二进制 lift IR 的多态边界识别规划

## 背景

NotDec 当前能用函数名内置名单和 summary override 把 allocator、裸 buffer、回调
context 等函数标为调用点多态，并可对通用 allocator wrapper 设置 `opaque_body`。
这能避免一个公共形式参数或返回值把许多本来无关的调用点类型粘成巨型约束分量，
但函数名和源码 DebugInfo 在真实二进制 lift IR 中通常都不可用。

LLVM 源码级 IR 还可能带有以下契约：

- `allockind` 描述函数具有 allocate、reallocate、free、zeroed 等哪类内存行为；
- `allocsize(i[, j])` 指出哪一个或哪两个参数决定分配字节数；
- 参数属性 `allocptr` 指出被 realloc/free 一类操作处理的旧分配指针；
- 返回值 `noalias` 表示返回指针不与调用者当前可见的其他对象别名，allocator 常用它。

这些都不是机器指令本身携带的语义。编译到二进制后，它们至多间接影响优化结果，
一般无法由 lifter 完整、可靠地恢复。因此它们只能用于建立源码 IR oracle、生成候选
真值或做消融对照；生产检测器不得读取它们，测试也必须证明去掉这些属性和所有名字、
DebugInfo 后仍可工作。

## 目标与边界

目标是在无函数名、无 DebugInfo、无 allocator 属性的 lifted LLVM IR 上，为每个函数
和具体参数/返回槽位给出多态边界候选及置信度，优先识别：

1. 通用 allocate/reallocate/deallocate 函数及其 wrapper；
2. 布局由长度、命令或运行时 tag 决定的裸 buffer 读写函数；
3. 被多个不相关调用者复用的 `void *` callback/context 传递边界。

检测结果应区分“调用点实例化”和“隐藏函数体”两个决策。只有能确认函数体只是通用
分配/释放机制、且不固定初始化某个业务对象布局时，才自动建议 `opaque_body`。固定结构
factory、容器节点构造器和资源对象创建函数即使内部调用 allocator，也不能因此被隐藏。

## 核心判断

单纯在 Call Graph SCC 上寻找割点不够。通用 allocator wrapper 经常位于调用图叶侧，
调用关系也是无环的，并不是图论上的 articulation point。真正造成类型污染的是许多调用
实参/返回值共同连接到同一个 callee formal/return slot。因此：

- Call Graph SCC 用于递归函数摘要的不动点求解、传播顺序和候选范围裁剪；
- 是否应多态化，主要在 call-slot/type-constraint graph 上判断；
- 对候选边界做反事实切断：模拟每个调用点独立实例化其参数与返回槽位，度量约束分量
  是否明显拆小、互不相容的 caller use-shape 是否被分离，同时检查是否会丢失稳定的
  跨调用业务结构约束。

这比“调用图里切断一个节点”更接近当前类型爆炸的直接原因，也能覆盖无递归 wrapper。

## 可从 lifted IR 提取的证据

### 函数内语义摘要

对每个函数做跨基本块的轻量数据流摘要，并在 Call Graph SCC 内迭代到稳定：

- 返回指针的来源：参数、全局、栈地址、子调用返回、整数转指针、未知内存；
- size-like 整数依赖：返回对象或被操作区间是否依赖一个长度参数、两个参数的乘积、
  对齐取整、溢出检查或最小块大小；
- 失败路径：空指针检查、零长度特判、错误码、OOM/abort 分支；
- 内存效果：按动态长度 copy/zero/read/write，是否消费旧指针，是否返回替代指针；
- allocator 状态：bump pointer、free list、size class、chunk header、全局统计计数等更新；
- 指针对齐和范围判断，以及新旧区间是否发生搬移或保留内容；
- wrapper 纯度：是否主要转发参数和返回值，除统计、锁、错误处理外是否缺乏业务字段访问。

这些特征只由指令、CFG、常量、调用关系和内存访问获得，不依赖符号或元数据。对 lifter
产生的统一内存模型，需要先把等价的地址计算、load/store helper 和直接 LLVM 内存操作
归一为同一摘要事件。

### 调用者使用形状

从每个 call site 向前后做有界 slice，把实际参数和返回值的用途编码成 use-shape：

- 常量/区间 GEP 偏移集合、访问宽度、读写方向和重复步长；
- 是否先判空、是否很快被释放、是否转交给另一个 ownership 操作；
- 是否只参与按动态长度的字节传输，还是持续访问稳定的固定字段；
- callback 是否与某个 context 一起保存、转发并在间接调用时重新配对；
- reverse fan-in、调用者 SCC/模块区域的多样性，以及各调用点 use-shape 的聚类距离。

同一返回槽位被大量互不相关的调用者使用，而且形成多个差异明显、各自内部稳定的布局
簇，是通用 allocator 的强证据。相同参数槽位跨调用点承载不同长度和不同访问布局，则是
裸 buffer/context 多态的强证据。

### 负面证据

下列情况应降低置信度或直接否决自动 `opaque_body`：

- 返回值稳定来自某个输入参数、全局对象或栈对象，而不是新存储；
- 函数内部以固定偏移初始化一组一致字段，调用者也呈现同一布局；
- 调用者很少，或所有调用点 use-shape 高度一致；
- 返回对象总进入同一种 vtable/tag/resource 生命周期；
- 切断后虽然图变小，却会拆散跨调用点一致且有 DebugInfo oracle 支持的业务类型；
- 只是 getter、interior-pointer accessor、固定结构 serializer 或某种专用 factory。

图规模下降本身不能作为语义正确性的证据，否则容易把任何高 fan-in 工具函数误标成
多态边界。

## 候选分类与打分

第一阶段采用可解释的规则分数，不先训练黑盒模型。每个候选保存证据明细，分别输出：

- allocator/reallocator/deallocator/raw-buffer/context 的类别分数；
- 具体应实例化的参数/返回槽位，而不是默认隔离整个函数签名；
- 反事实切断前后的最大分量、连通分量数、约束边数和 caller cluster 纯度；
- 支持与反对证据，以及 high/medium/low 置信度。

High 才允许自动启用调用点多态；只有 allocator 语义和 wrapper 纯度同时很强时才允许自动
`opaque_body`。Medium 写入审计报告供人工确认，Low 不改变恢复行为。先使用规则和阈值是
为了能从 bad union 的 first witness 追溯误判原因；样本积累后再评估学习排序模型，但仍
保留 hard negative gate。

## 实验设计

使用 tmux、Redis、lighttpd、memcached 四个较大的源码级 IR 建立第一轮实验。人工结合
原始函数名和源码审阅建立边界真值，但检测器只接收匿名化输入。每个项目至少准备：

1. 原始源码 IR，仅用于 oracle 和现有手工 summary 基线；
2. 删除函数/局部值名字和 DebugInfo 的匿名版本；
3. 在匿名版本上进一步移除 `allockind`、`allocsize`、`allocptr`、`noalias` 及其他可能泄漏
   allocator 身份的属性，作为更接近 lifted IR 的主消融版本；
4. 有可用 bin2llvm 产物时，最终回到真实 lifted IR 验证，而不是把去属性源码 IR 当终点。

同一版本分别运行无边界、名字/源码真值边界、自动候选三组。固定 8 线程、关闭 ASLR，
使用新的 work/eval 目录，并以 HType/eval 文件而非输出 IR 的文本相等判断恢复结果。

## 指标与判断标准

- 识别质量：按函数类别和槽位统计 precision、recall、PR 曲线；自动模式优先保证 precision；
- 类型正确性：`bad_unions`、`polluted_components` 和 first witness 不得比手工边界基线恶化；
- 图规模：最大 call-slot/constraint component、约束数、SimpleType 节点数及拆分收益；
- 完整度：`fragmented_types`、`fragmented_nodes`，但不能用降低 fragmentation 换 wrong merge；
- 可扩展性：constraint generation、analyze/simplify wall time 与峰值 RSS；
- 鲁棒性：重命名、去 DebugInfo、去 allocator 属性前后候选应基本一致；
- 可解释性：每个 high 候选必须能输出足以人工复核的正负证据和关键 call site。

第一阶段成功标准是：四个项目上 high 候选保持高精度，覆盖主要自定义 allocator wrapper，
去属性后召回没有显著坍塌，并能显著压低 tmux/Redis 的最大约束分量和峰值内存；随后再将
同一检测逻辑接到真实 bin2llvm IR 上验证 lifter 形状差异。

## 风险与控制

- 优化可能内联、拆分或消除 allocator 行为。检测单位需允许函数簇或 SCC 摘要，不能假定
  一个源码函数对应一个 lifted 函数。
- 间接调用会造成 Call Graph 缺边。先保守构建可能目标集合，并把不确定性写入置信度；
  不能因缺边把 callback/context 自动判成 high。
- 自定义 arena/slab allocator 与固定类型 object pool 很相似。caller use-shape 多样性、固定
  字段初始化和资源生命周期是主要区分依据，object pool 默认不自动 opaque。
- 反事实图分析可能本身昂贵。先在压缩后的 call-slot 图上估算局部切断收益，只对 medium/high
  候选做精确重放。
- 源码 IR 去属性仍不等价于二进制 lift IR。它只负责快速 A/B 和 oracle 评估，最终判断必须
  包含真实 lifted IR。

## 当前 tmux 基线

本轮先用人工审阅的 `test/type-recovery/realworld/support/tmux.source.summary.json`
启动完整评估。通用 `xmalloc`/`xrealloc`/string allocation wrapper 等同时标记
`is_polymorphic` 与 `opaque_body`；裸 buffer 和 callback/context 边界只标记
`is_polymorphic`。该基线既用于当前可扩展性验证，也作为后续匿名检测的人工 oracle，
但函数名字本身不得输入未来检测器。

## 本轮实施记录（2026-08-22）

### 文件与生效位置

- 新增 `test/type-recovery/realworld/support/tmux.source.summary.json`。第 4-14 行是
  11 个 `is_polymorphic + opaque_body` 通用分配/释放 wrapper；第 16-40 行是
  25 个外部 allocator、buffer、socket 和内存边界；第 42-58 行是 17 个 tmux
  自定义裸 buffer 边界；第 60-80 行是 21 个 callback/context 边界。共 74 个
  `is_polymorphic` 函数。
- 本轮没有修改 C++ 实现。配置沿用
  `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 的
  `MLsubRecovery::isSummaryOverridePolymorphic()`、
  `MLsubRecovery::isOpaqueBody()` 和 `MLsubRecovery::prepareSCC()`；约束生成主循环把
  opaque 函数加入 `OpaqueBodies`，并输出 `opaque body skipped` 诊断。
- 人工逐一审阅 tmux IR 的 1848 个定义和 232 个声明后，再用配置 key 与两份函数清单
  做集合差；74 个名字全部存在，差集为空。`jq` 校验结果为 74 个 polymorphic、
  11 个 opaque。

### 构建与验证

- 初次增量构建返回 `ninja: no work to do`，但旧二进制时间早于当前 revert commit，
  因此不能作为有效重建。随后运行
  `cmake --build build-relwithdebinfo-20260731 --target notdec --clean-first -j4`，清理
  127 个旧产物并完成 108/108 个构建步骤。
- 新二进制为 `build-relwithdebinfo-20260731/bin/notdec`，SHA-256 为
  `b2fab57408ac66dccbf0c31bbe1297796bce7270ebe780bd9ae9d00ffcf3437e`。
- `llvm-22.1.0.obj/bin/opt -passes=verify -disable-output` 验证 tmux 输入通过；输入
  `tmux.bc` SHA-256 为
  `adc2be96ebc74bdc379951402fb087d2f69817ee8e6e580c19c15413d2a6f919`。
- 构建并运行 `build-relwithdebinfo-20260731/bin/MLsubGeneratorTest`，19/19 通过，
  包括 `MLsub.OpaqueBodySkipsInstructionConstraints`。summary JSON 也通过 `jq empty`。

### tmux 长跑

2026-08-22 03:14 UTC 在 detached screen
`notdec-tmux-poly-20260822` 中启动：

```bash
setarch x86_64 -R env \
  NOTDEC_BINARYSUB_THREADS=8 \
  NOTDEC_SIMPLIFY_DIAG=1 \
  NOTDEC_SUMMARY_OVERRIDE=/sn640/NotDec/test/type-recovery/realworld/support/tmux.source.summary.json \
  /usr/bin/time -v /sn640/NotDec/build-relwithdebinfo-20260731/bin/notdec \
  /sn640/NotDec-Exp/Bench2/source-ir/ir/tmux/tmux.bc \
  --tr-level=2 --merge-struct-ptr-load-store \
  -g --fast-work-dir \
  --work-dir=/tmp/notdec-source-tmux-poly-20260822-work \
  --merge-eval-dir=/tmp/notdec-source-tmux-poly-20260822-eval \
  -o /tmp/notdec-source-tmux-poly-20260822-out.ll
```

screen 日志是 `/tmp/notdec-source-tmux-poly-20260822.log`。启动后配置中的 74/74
函数都出现 `Applying MLsub summary override`，11/11 opaque 函数都出现
`opaque body skipped`；没有配置函数缺失或字段类型错误。注意 workdir 的
`PolymorphicBufferFunctions.txt` 只记录内置名字检测，不记录 summary override，不能用它
否定本配置；`MallocWrappers.txt` 另行自动识别到 `xmalloc`、`xcalloc`、`xrealloc`、
`xreallocarray`、`xrecallocarray`、`xstrdup` 六条简单 wrapper 链。

03:16 UTC 进程 PID 为 1953335，处于单线程约束生成阶段，CPU 约 100%，RSS 约
15.6 GiB；`00-lifted.ll`、`02-mlsub-input.ll`、CallGraph、SCC 和 DebugInfo oracle
均已写出。运行仍在继续，后续以同一日志、work/eval 目录记录最终 wall time、峰值 RSS、
wrong merge 和 fragmentation；长跑期间不要重链上述 `notdec` 二进制。

### 匿名审计器实现记录（2026-08-23）

- 新增 `include/notdec/TypeRecovery/mlsub/AnonymousPolyBoundaryAnalysis.h:12-97`
  与 `src/TypeRecovery/mlsub/AnonymousPolyBoundaryAnalysis.cpp:48-1188`。核心函数
  `analyzePointerUses()`、`analyzeRawBufferSlot()`、`analyzeDeallocatorSlot()`、
  `analyzeAnonymousPolymorphicBoundaries()` 和
  `writeAnonymousPolymorphicBoundaryReport()` 只读取 LLVM 操作、数据流、调用者
  多样性与内存访问形状；返回值/参数和 raw-buffer/deallocator 证据按槽位保存，函数名
  仅作为审计输出标签。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:91-98,5993-6031` 增加
  `NOTDEC_ANONYMOUS_POLY_AUDIT` 与 `NOTDEC_ANONYMOUS_POLY_AUDIT_ONLY`。默认恢复行为
  不变；显式 audit-only 才在约束生成前返回，避免大项目实验误触发生产路径。
- `src/CMakeLists.txt:33` 注册新分析器；`unittests/Retypd/MLsubGeneratorTest.cpp:652-874`
  增加匿名构造模块，验证 allocator/factory/raw-buffer/deallocator 的正负证据，并在
  全部函数重命名后确认分数和置信度不变。
- 验证：`cmake --build build --target MLsubGeneratorTest -j4` 成功；
  `./build/bin/MLsubGeneratorTest` 的 `20/20` 测试通过。该实现当前只生成审计报告，
  尚未把源码级匿名候选自动接入 SCC/多态恢复。

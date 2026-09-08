# DEBUG.md

本文档集中说明 `NotDec` 当前调试流程里的工作文件夹和中间产物，方便在使用 `debugmcp`、`launch.json` 或手工命令行调试时快速定位问题。

## 1. 工作文件夹的基本用法

工作文件夹由命令行选项开启，不再由环境变量控制。

常见做法：

```bash
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 --gen-work-dir
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --work-dir=work_dir
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --fast-work-dir
./build/bin/notdec input.bc --emit-tr-input-ir=/tmp/tr-input.ll -g --work-dir=work_dir
./build/bin/notdec /tmp/tr-input.ll -o /tmp/out.ll --tr-level=2 --frozen-tr-input-ir
```

其中：

- `--gen-work-dir` / `-g`
  - 开启工作文件夹输出
- 默认路径
  - 使用输入文件同名并追加 `.notdec`，例如 `cases/foo.ll.notdec/`
- `--work-dir=<path>`
  - 用于覆盖默认工作文件夹路径
- `--fast-work-dir`
  - 必须和 `--gen-work-dir` / `-g` 一起使用
  - `ValueTypes.txt` 和 `VarOrigins.txt` 只写 stable value label，不补完整 LLVM value 文本
  - 适合完整项目跑批；需要看具体指令文本时不要开启
- `--emit-tr-input-ir=<path>`
  - 跑完 pre-type-recovery 标准化 pass，导出 stage-A IR
  - 支持输出到 `.ll` 或 `.bc`
  - 导出后直接退出，不进入 `MLsubRecoveryMain`
  - 它不是包含标量化等 MLsub 入口准备的最终 checkpoint；需要可复现的实际
    MLsub 输入时，用正常 `-g --work-dir=<dir>` 运行后取
    `<dir>/02-mlsub-input.ll`
- `--frozen-tr-input-ir`
  - 声明当前输入已经是 `--emit-tr-input-ir` 导出的冻结 IR
  - 用于显式进入阶段 B 语义
  - 当前会跳过 pre-type-recovery 标准化 pass，直接进入 `MLsub`
  - 只接受 `.ll` / `.bc` 输入
  - 当前启用 `NOTDEC_EXTRA_CONSTRAINTS` 时必须同时传入

仓库内的 `run.sh` 和 `.vscode/launch.json` 里的常用配置现在也统一走这套命令行参数。

## 2. 工作文件夹里的主要内容

工作文件夹统一承载原先两类中间产物，主要覆盖：

- 主 pass pipeline 初始 IR dump
- stack / memory recovery 前的 IR dump
- 类型恢复阶段的优化后 IR
- CallGraph 文本与 dot
- SCC 划分信息
- binarysub trace
- 最终 `IR Value -> binarysub UType` 对照表
- `llvm2c` 后端在 demote SSA 前后的 IR dump

如果问题表现为：

- 类型恢复异常
- 某个值被推成了奇怪的类型
- binarysub 约束传播/简化结果不对

那么优先看这个目录。

## 2.1 EVM / PHI 调试

调 Gigahorse 的 PHI 来源时，默认把 inliner 关掉。

原因很直接：

- 先看原始 CFG 和 block-level PHI 事实。
- inline 以后，return edge 和 PHI 来源会混在一起，不容易分清是导出问题还是 inline 问题。
- 只有明确要查 inline 行为时，再单独打开。

对应的 wrapper 常用参数是 `--gigahorse-extra-arg=--disable_inline`。

## 3. 工作文件夹里常见文件及作用

下面按当前仓库里实际会出现的文件说明。不是每次运行都会生成全部文件；是否出现取决于输入、`tr-level`、是否走到 `llvm2c`、以及具体 pass 是否执行到对应阶段。

### `00-lifted.ll`

- 来源：主 pass pipeline 早期 dump
- 作用：看“输入被载入/提升后”的 LLVM IR 初始状态
- 典型用途：确认前端读入 `.ll` / `.bc` / `.wasm` 后，IR 是否已经在最开始就有问题

### `01-1-BeforeStackAlloca.ll`

- 来源：`LinearAllocationRecovery`
- 作用：看 stack alloca 恢复前的模块状态
- 典型用途：排查 stack pointer 识别失败、栈增长方向判断异常、stack rewrite 前后差异

### `02-mlsub-input.ll`

- 来源：
  - `MLsubRecovery::run()` 入口处的模块 dump
  - 或 `--emit-tr-input-ir=<path>` 对应的导出内容
- 作用：看 `mlsub` 真正吃到的 LLVM IR
- 典型用途：判断问题是在更早的优化/恢复阶段就产生了，还是在类型恢复阶段才出现
- 备注：如果后续 JSON 约束注入要绑定某个固定 IR，这份文件比泛化的 “Optimized” 更接近实际锚点

### `02-mlsub-input.anchor.json`

- 来源：`MLsubRecovery::run()` 在导出 `02-mlsub-input.ll` 后同步生成
- 作用：记录当前 `mlsub` 输入 IR 的阶段名、数据布局、target triple 和内容摘要
- 典型用途：给后续约束文件或调试脚本提供机器可读的 IR 锚点，快速确认 selector 是否仍对着同一份冻结 IR
- 当前摘要字段：只保留 `ir_anchor.sha256`
- 当前校验规则：`NOTDEC_EXTRA_CONSTRAINTS` 要求 `ir_anchor.sha256` 与当前
  `02-mlsub-input.ll` 的 SHA-256 一致
- 当前阶段约束：`NOTDEC_EXTRA_CONSTRAINTS` 还要求命令行显式传入
  `--frozen-tr-input-ir`，表示当前运行是“冻结 IR + MLsub”的阶段 B 模式；
  该环境变量不能与 `--emit-tr-input-ir` 同时使用
- 相关环境变量：`NOTDEC_EXTRA_CONSTRAINTS` 当前已经支持读取一个 JSON 文件，并在 `MLsub` 开始前校验其中的 `ir_anchor` 是否匹配当前冻结 IR；函数级 `actions` 已支持 `kind = "pndiff" | "subtype" | "equal"`，`target` 当前支持 `arg` / `ret` / `named_value` / `inst` / `operand` / `binding`；其中 `named_value` 表示当前函数里第一个同名非 `void` instruction result，`inst` 的 `id` 和 `operand.inst` 都使用 `toStableString()` 风格的指令 id，`operand` 现在支持两种 instruction 定位方式：
  - `operand(inst="main::%foo", index=N)`：按 stable id 定位
  - `operand(name="foo", index=N)`：按当前函数里第一个同名非 `void` instruction result 定位
  `binding` 表示引用当前函数 `bindings` 段里的局部 selector 别名

### `CallGraph.txt`

- 来源：类型恢复阶段构造 CallGraph 后导出
- 作用：文本形式查看调用图
- 典型用途：排查函数是否被识别为直接调用、某条调用边是否存在、某个函数是否被纳入分析

### `CallGraph.dot`

- 来源：类型恢复阶段构造 CallGraph 后导出
- 作用：Graphviz dot 格式的调用图
- 典型用途：把调用关系可视化，快速看 SCC、递归、调用边结构
- 特点：默认隐藏没有函数体的 external declaration，便于先看主链路

### `CallGraphFull.dot`

- 来源：类型恢复阶段构造 CallGraph 后导出
- 作用：Graphviz dot 格式的完整调用图
- 典型用途：排查对 libc / wasm import / 其他 external declaration 的调用边是否存在
- 特点：保留没有函数体的 external declaration，并用浅灰虚线节点和有函数体的函数区分

### `SCCs.txt`

- 来源：类型恢复阶段的 SCC 划分逻辑
- 作用：记录类型恢复按什么 SCC 顺序/分组进行分析
- 典型用途：排查多态函数复制、bottom-up / top-down 分析边界、某个函数为什么被分到当前 SCC

### `ValueTypes.txt`

- 来源：类型恢复阶段 `bulkSimplify` 结束后导出
- 作用：记录最终的 `IR Value -> binarysub UType` 映射
- 典型用途：直接看某个 LLVM Value 最后对应的 `UType` 是什么，适合调试“为什么这个值被推成这个类型”

当前格式特点：

- 以 `## SCC: ...` 分块，便于按 SCC 查看
- `"[+]"` / `"[-]"` 表示当前条目在 binarysub 简化时采用的极性
- value 标签优先打印 stable id，并在括号里补 verbose `ExtValuePtr`
- 开启 `--fast-work-dir` 时只保留 stable id，避免为每个 value 调用 LLVM verbose printer
- `[memory] <memory> => ...` 表示 memory 总类型
- `ValueTypes.txt` 本身只保留 `value => UType`；如果需要继续追某个
  `UType` 变量对应的 `ut#id / vs#originId / ExtValuePtr`，去同目录下的
  `VarOrigins.txt`

### `VarOrigins.txt`

- 来源：类型恢复阶段 `bulkSimplify` 结束后，与 `ValueTypes.txt` 同步导出
- 作用：把每个最终 `UType` 类型变量展开为 `utype var -> original variable state -> original ExtValuePtr`
- 典型用途：当 `ValueTypes.txt` 里看到某个 `'a` / `'b` 很难判断“它最早是从哪个 value 长出来的”时，直接在这里查
- 当前格式里 `appears-in:` 表示这个最终 `ut#id` 出现在哪些顶层结果里，不表示原始来源；真正的原始来源仍看后面的 `vs#originId => ...`

### `ValueHTypes.txt`

- 来源：类型恢复阶段 `MLsubRecovery::genASTTypes()` 汇总最终结果后导出
- 作用：记录最终的 `IR Value -> HType` 映射，以及对应 `decls` / `memory` 快照
- 典型用途：在 `ValueTypes.txt` 看完 `UType` 后，继续确认最终落到 `HTypeResult` 里的值类型、结构声明名、memory 总类型

当前格式特点：

- 直接复用 `HTypeResult::print()` 的 snapshot 格式
- `[types]` section 按 stable value key 排序，打印最终 `Value -> HType`
- `[decls]` / `[memory]` section 会一并输出，方便读 record / memory 的 canonical 名

### `ImportantHTypes.txt`

- 来源：类型恢复阶段 `MLsubRecovery::genASTTypes()` 汇总最终结果后导出
- 作用：给人工对比用的精简 HType 视图，只保留 memory 大结构体、函数参数/返回值，以及引用到的 `decls`
- 典型用途：和 `test/tools/extract_wasm_dwarf_truth.py` 产出的文本 truth 并排看，先对齐全局偏移和函数签名，再去后面的 `decls` 查具体字段布局

当前格式特点：

- `[memory]` 只给出 memory 总类型和对应的 memory decl，具体绝对偏移直接去看该 decl 的 field 注释
- `[functions]` 只列函数 value 的最终类型，按函数名排序，并拆开显示 `ret` / `argN`
- `[decls]` 仍复用 snapshot formatter 的稳定声明名，方便回查前两部分里出现的 `struct_*`

### `SelectableValues.txt`

- 来源：`MLsubRecovery::run()` 在导出 `02-mlsub-input.ll` 后同步生成
- 作用：列出当前冻结 IR 上可直接写进 `NOTDEC_EXTRA_CONSTRAINTS` 的 selector 提示
- 典型用途：手写 `inst` / `operand` / `named_value` / `arg` / `ret` selector 时直接抄 workdir 里的示例
- 当前格式特点：
  - 每个函数以 `## Function: ...` 分块
  - 直接列出 `arg` / `ret` / instruction stable id
  - 对有名非 `void` instruction 额外列出 `named_value(...) -> ...`
  - 对每条 instruction 列出 `operand(inst="...", index=N)` 示例
  - 对有名非 `void` instruction 额外列出 `operand(name="...", index=N)` 示例

### `binarysub-trace.log`

- 来源：`external/binarysub`
- 生成条件：仅当 `NOTDEC_BINARYSUB_TRACE=1` 且启用了 `--gen-work-dir` 时生成
- 作用：记录 binarysub 与 `PNDiff` 的内部事件流，包含约束、extrude、simplify、
  `PNDiff` 加减法求解、unify 等 trace
- 典型用途：当 `ValueTypes.txt` 只能看到“结果不对”，但还不知道“约束传播过程中哪一步歪了”时，继续往这个文件追

### `CallSlotMergeDecisions.txt`

- 来源：事务化的参数/返回值调用槽整组合并策略
- 作用：记录每个目标函数的参数和返回槽候选、递归合并过程和最终决定，默认不需要开启
  `NOTDEC_BINARYSUB_TRACE`
- 生成条件：启用 `--gen-work-dir` 或 `--work-dir`
- `transaction` / `target`：同一个目标函数的参数和返回槽共用事务编号；其中任一递归合并冲突时，
  该事务已执行的槽会一起回退
- `policy`：`call-arg-slot-group` 表示实参/形参，`return-slot-group` 表示调用返回值或
  函数内真实返回值
- `decision`：`committed`、`precheck-skipped`、`rolled-back` 或 `preexisting-alias`
- `reason`：布局冲突、缺少结构体证据、层级/大小不一致、跨 formal 槽递归传播等直接原因
- `touched-nodes`：提交或回退前被事务首次修改的 `SimpleType` 节点数
- `entries`：该 formal 槽对应的全部 callsite 或 `return` 点
- `roots`：决策发生前的 root、位宽、一层字段切片和每个 root 的处理结果
- `recursive-merges`：事务实际遇到的显式、反向边和策略触发的递归 merge；跨函数 formal 标签按
  `target::argN` / `target::ret` 显示

查所有没有合并的槽位时，先搜索 `decision: precheck-skipped`、`decision: rolled-back` 和
`decision: preexisting-alias`。`rolled-back` 表示冲突是在递归传播中才发现，图和合并评估证据均已
恢复到事务开始前；`preexisting-alias` 表示污染发生在事务之前，本轮无法撤销，需要继续对照
更早的 decision 或 `binarysub-trace.log`。

### `LocalSubtypeMergeStats.txt`

- 来源：同函数 subtype 合并 hook 和 deferred call subtype 处理流程
- 生成条件：启用 `--gen-work-dir` 或 `--work-dir`
- 作用：统计同层同位宽候选、同函数候选、pointer-like 候选、实际 policy merge，以及
  `addSubtype()` 被 solver 拒绝的原因；用于判断“先加 subtype、后 merge”是否产生了明显的临时工作
- `early-call-interface-merged`：在实验顺序下，加入调用 subtype 前已合并的 actual/formal 对数
- `policy-replace-bound-merge-events`：首次处理 var-var subtype 时直接触发的合并数
- `policy-auxiliary-merge-events`：后到结构体或 pointer bound 使旧 var-var 边满足策略后触发的合并数
- 以上 event 表示 solver 已实际执行的 merge 工作，包含之后被整函数事务回退的尝试；最终代表节点变化
  仍以 `merge-eval-summary.json` 为准
- `subtype-constraint-failures`：`addSubtype()` 原本就会忽略并继续的 solver 拒绝；本文件只是把原因暴露出来，
  不能把该数字当成实验模式新增的冲突

下面的环境变量只用于性能和正确性 A/B，不改变默认策略：

- `NOTDEC_LOCAL_SUBTYPE_MERGE_MODE=pointer`：同函数、同层、同位宽且两端 pointer-like 时允许 merge，
  不再要求结构体字段访问证据
- `NOTDEC_LOCAL_SUBTYPE_MERGE_MODE=all-local`：同函数、同层、同位宽的 var-var subtype 都允许 merge
- `NOTDEC_EARLY_CALL_INTERFACE_MERGE=1`：先按整函数事务策略尝试 actual/formal 槽合并，再加入 deferred
  call subtype

默认 `NOTDEC_LOCAL_SUBTYPE_MERGE_MODE=struct-pointer`，并且先加入全部 call subtype，再执行事务合并。
提前合并只能看到当时已有的布局证据，后到约束可能说明原合并不该发生；因此这些实验开关不能作为
常规跑批配置。

### `PolymorphicBufferFunctions.txt`

- 来源：类型恢复进入 SCC 划分前的内置多态 API 标记
- 作用：列出按调用点实例化的通用 buffer、allocator 和 `void *` context API；这些函数不会用一个
  declaration formal 把不同调用点的数据结构合在一起
- 当前 ffplay 相关例子：`av_log`、`av_opt_set`、`av_opt_set_int`、`av_opt_set_bin`、
  `av_fifo_read`、`av_fifo_write`

### `llvm2c-before-demotessa.ll`

- 来源：`llvm2c` 后端
- 作用：看 `llvm2c` 在 demote SSA 之前收到的模块
- 典型用途：排查“类型恢复输出正常，但 C 后端行为异常”的问题

### `llvm2c-after-demotessa.ll`

- 来源：`llvm2c` 后端
- 作用：看 `llvm2c` 执行 reg2mem / demote SSA 之后的模块
- 典型用途：排查 phi、局部变量重建、SSA 降级后导致的结构恢复问题

## 4. 推荐阅读顺序

如果遇到反编译问题，通常可以按下面顺序看：

1. `00-lifted.ll`
   - 确认输入 IR 是否一开始就异常
2. `01-1-BeforeStackAlloca.ll`
   - 如果问题与栈/内存恢复相关，先看这里
3. `02-mlsub-input.ll`
   - 确认类型恢复实际输入
4. `02-mlsub-input.anchor.json`
   - 确认当前 workdir 里的 `mlsub` 输入 IR 身份信息
5. `CallGraph.txt` / `SCCs.txt`
   - 确认分析范围、递归分组、多态边界
6. `ValueTypes.txt`
   - 直接看某个值最后被推成什么 `UType`
7. `binarysub-trace.log`
   - 继续追 binarysub 内部传播细节
8. `llvm2c-before-demotessa.ll` / `llvm2c-after-demotessa.ll`
   - 如果 `.ll` 看起来正常，但 `.c` 输出异常，再看这里

## 5. Bench2 本机跑法

如果要看当前这台机器上的 Bench2 真实项目跑法，请去：

- [`external/NotDec-bin2llvm/DEBUG.md`](/sn640/NotDec/external/NotDec-bin2llvm/DEBUG.md)

主仓库这里只保留通用的调试说明，不重复写 bin2llvm 的本机命令。

## 6. 冻结输入和环境变量

涉及 frozen stage-A/stage-B 输入、约束注入或类型恢复 A/B 时，先在本节确认
阶段边界。`--emit-tr-input-ir=<path>` 运行 pre-type-recovery pipeline，写出 `.ll`
或 `.bc` 后退出；`--frozen-tr-input-ir` 把 stage-A 导出产物作为 stage-B 输入，跳过
这段预处理。
`NOTDEC_EXTRA_CONSTRAINTS` 只可用于 stage-B，必须与 `--frozen-tr-input-ir` 一起用，
不能和 `--emit-tr-input-ir` 混用。

若同时需要 workdir，请以工作目录重发射的 `02-mlsub-input.ll` 作为实际 MLsub
checkpoint：它包含 type-recovery 入口的标量化等输入准备，可能与
`--emit-tr-input-ir` 写出的 stage-A 文件不同。`02-mlsub-input.anchor.json` 是约束
文件对齐 checkpoint 的哈希锚点。

常用环境变量如下：

- `NOTDEC_SUMMARY_OVERRIDE`：summary/lower-bound override，方向为
  `OverrideTy <: F`。
- `NOTDEC_SIGNATURE_OVERRIDE`：函数自身签名 upper-bound override，方向为
  `F <: OverrideTy`。
- `NOTDEC_EXTRA_CONSTRAINTS`：冻结 stage-B IR 的额外 MLsub/PNDiff 约束。
- `NOTDEC_BINARYSUB_THREADS`：bulk simplify 线程数；`1` 表示单线程，未设置时用
  硬件线程数。
- `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=0`：只关闭 oneTBB
  canonicalize 并行；更早的 bottom-up 约束生成仍是串行。若要在编译期关闭
  binarysub oneTBB，重新配置 CMake 并传
  `-DNOTDEC_ENABLE_BINARYSUB_PARALLEL=OFF`。

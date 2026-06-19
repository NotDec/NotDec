# 原始 prompt

调研一下Angr /sn640/angr ，Ghidra这两个项目内部的控制流结构恢复算法的框架，看他们是怎么设计支持多种算法的抽象接口的。然后把Angr,Ghidra以及我们项目当前的设计都总结到新的logs/文档中，同时规划：重构使得架构上接近Angr的控制流结构恢复，方便后续将他那边支持的结构恢复算法都复现过来。

# 背景

当前 backend structuring 已经有最小公共接口：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:21`
  用 `PayloadRef` 隔离目标语言 AST，structuring 层只搬 payload id。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:43`
  `CFGBlock` 保存 block id、statement payload、terminator、condition、successor 和 switch case。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:65`
  `StructuredNodeKind` 已有 sequence、basic block、if、switch、goto、break、continue、loop、return、unreachable。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Structurer.h:11`
  `Structurer` 现在是一个很小的整图接口：`StructuredCFG -> StructuredTree`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:163`
  Solidity 通过 `LLVMFunctionCFGBuilder` 构造 `StructuredCFG`，再跑 `GotoStructurer`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:63`
  C 试点路径从旧 C CFG 转成 `StructuredCFG`，再跑 `GotoStructurer`。

这个接口能让 C/Solidity 共用 Goto fallback，但离 Angr 的结构恢复框架还差一层：Angr 不是直接对整图跑一个算法，而是先做 region 识别，再递归结构化每个 region，并允许 Phoenix、Dream、SAILR 这些算法互换。

# Angr 框架

Angr 的核心目录是 `/sn640/angr/angr/analyses/decompiler/`。

## 算法注册

`/sn640/angr/angr/analyses/decompiler/structuring/__init__.py:8` 定义 `STRUCTURER_CLASSES`：

- `sailr -> SAILRStructurer`
- `phoenix -> PhoenixStructurer`
- `dream -> DreamStructurer`

`/sn640/angr/angr/analyses/decompiler/structuring/__init__.py:14` 默认算法是 `SAILRStructurer`。

`/sn640/angr/angr/analyses/decompiler/decompilation_options.py:211` 把 structuring algorithm 暴露成 decompiler option，候选值是 `SAILR`、`Phoenix`、`DREAM`，并用 `structurer_class_from_name()` 转成类。

## 递归结构化入口

`/sn640/angr/angr/analyses/decompiler/structuring/recursive_structurer.py:28` 定义 `RecursiveStructurer`，它的职责不是具体识别 if/while，而是：

- 接收 `RegionOverlay`。
- 按 region tree 从内到外递归处理。
- 对每个 region 调一个可替换的 `StructurerBase` 子类。
- 做 post process，比如删冗余 jump、删空节点、删无用 label。

`recursive_structurer.py:100` 是真正调用算法的位置：`self.project.analyses[self.structurer_cls]`。

`recursive_structurer.py:127` 到 `136` 说明它支持两种算法形态：

- Phoenix 这类算法会 destructively reduce region graph，结果节点还在 region members 里，然后 `finalize()`。
- Dream 这类算法可以不改共享 graph，而是产出一个 external result，然后 `collapse_to()`。

这点很重要：Angr 的抽象不是强迫所有算法同一种内部写法，而是用 region overlay 把“破坏式规约”和“生成新树”都包住。

`recursive_structurer.py:137` 到 `140` 还有 undo/rollback 机制，保证结构化过程改共享 graph 后，region identifier 的结果还能给后续消费者用。

## StructurerBase

`/sn640/angr/angr/analyses/decompiler/structuring/structurer_base.py:49` 定义 `StructurerBase`。

它不是纯算法接口，而是一个带上下文的 analysis：

- `region`
- `parent_map`
- `condition_processor`
- `func`
- `case_entry_to_switch_head`
- `parent_region`
- `jump_tables`
- `ail_manager`

`structurer_base.py:88` 保存 `virtualized_edges`，也就是被算法改成 goto 的边。

所以 Angr 的算法边界大致是：

```text
RegionOverlay + 条件处理 + jump table 信息 + AIL manager
  -> StructurerBase 子类
  -> result BaseNode / SequenceNode / ConditionNode / LoopNode / SwitchCaseNode
```

不是：

```text
Raw CFG -> C code
```

## Decompiler 和算法的关系

`/sn640/angr/angr/analyses/decompiler/decompiler.py:317` 没指定 structurer 时使用 `DEFAULT_STRUCTURER`。

`decompiler.py:323` 判断是否是 Phoenix 系算法。如果是，会调整前面的 graph/loop 处理策略：

- `force_loop_single_exit = False`
- `expose_loop_head_backedges = True`
- 允许把 call expr 折进 condition

`decompiler.py:449` 调 `RecursiveStructurer`，传入 region、condition processor、function、AIL manager 和算法参数。

`decompiler.py:578`、`decompiler.py:643` 还会按算法名过滤优化 pass。也就是说，Angr 不只是“算法可替换”，而是整条 decompiler pipeline 会根据 structuring algorithm 调整前后处理。

## Phoenix / SAILR / Dream 的关系

`/sn640/angr/angr/analyses/decompiler/structuring/phoenix.py:77` 定义 `PhoenixStructurer(StructurerBase)`，它是 schema-based reducer，工作在 region graph 上。

`/sn640/angr/angr/analyses/decompiler/structuring/sailr.py:13` 定义 `SAILRStructurer(PhoenixStructurer)`。它不是完全另起炉灶，而是在 Phoenix 基础上替换 edge virtualization 排序和配套优化。

`sailr.py:14` 的注释说明 SAILR 的核心差异：

- 递归结构化。
- 做 deoptimization，撤销会制造 goto 或误结构化的编译器优化。
- 用更强的 virtualize edge heuristic。

所以后续如果要复现 Angr，不能只搬 Phoenix reducer。还需要保留“算法名影响前置/中间优化 pass”的入口。

# Ghidra 框架

Ghidra 的核心源码在 `/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/`。

## 固定规约器

`/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/blockaction.hh:183` 到 `191` 注释写得很直接：`CollapseStructure` 从基本块 CFG 开始，反复匹配结构子图，折叠成一个结构节点；如果卡住，就删边并标成 unstructured goto。

`blockaction.hh:192` 定义 `CollapseStructure`，它持有：

- 当前 `BlockGraph`
- loop body 列表
- likely goto 列表
- dataflow change count

`blockaction.hh:209` 到 `219` 是主要规则：

- `ruleBlockGoto`
- `ruleBlockCat`
- `ruleBlockOr`
- `ruleBlockProperIf`
- `ruleBlockIfElse`
- `ruleBlockIfNoExit`
- `ruleBlockWhileDo`
- `ruleBlockDoWhile`
- `ruleBlockInfLoop`
- `ruleBlockSwitch`
- `ruleCaseFallthru`

这和 Phoenix 类似，都是规则规约式算法。但 Ghidra 没有像 Angr 那样暴露多个可替换 structurer。

## 流程入口

`/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_process.cc:347` 的 `StructureGraph::rawAction()` 展示了独立结构化流程：

```text
BlockGraph.buildCopy()
structureLoops()
calcForwardDominator()
CollapseStructure.collapseAll()
orderBlocks()
```

`/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/blockaction.hh:294` 到 `305` 还定义了 `ActionPreferComplement`，用于结构化后选择更自然的 if/else 方向。

`blockaction.hh:268` 到 `277` 的 `ActionStructureTransform` 用于结构化后的最终变换，比如设置 for-loop。

所以 Ghidra 的 pipeline 是固定阶段组合，不是算法注册表。

## 输出边界

Ghidra 的结构结果不是直接生成 C 字符串，而是形成 `FlowBlock` 子类树。

`/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/printlanguage.hh:498` 到 `508` 定义打印接口：

- `emitBlockBasic`
- `emitBlockGraph`
- `emitBlockCopy`
- `emitBlockGoto`
- `emitBlockLs`
- `emitBlockCondition`
- `emitBlockIf`
- `emitBlockWhileDo`
- `emitBlockDoWhile`
- `emitBlockInfLoop`
- `emitBlockSwitch`

这点对我们有用：结构恢复层应该输出结构树，语言后端再渲染。Ghidra 的算法不可替换性不值得学，但它的“结构节点树 + 打印层 visitor”值得学。

# NotDec 当前设计

当前 NotDec 的新 structuring 层已经有三个优点：

- `PayloadRef` 已经把语言 AST 和 structuring 层隔开。
- `Structurer` 已经是算法接口，`GotoStructurer` 是第一个实现。
- C/Solidity 都已经能接同一个 `StructuredTree`。

但缺少 Angr 的三层：

1. 没有 `RegionIdentifier`。
   现在算法直接吃整张 `StructuredCFG`，后续 Phoenix/SAILR/Dream 很难按 region 递归结构化。

2. 没有 `RecursiveStructurer`。
   没有统一处理“先结构化内层 region，再把结果折回父 region”的框架。

3. 没有 region graph / overlay / mutable graph。
   旧 C Phoenix 是 destructive reducer；新 `StructuredCFG` 基本是只读输入。要迁 Phoenix，需要一个算法内部可变图，而不是让 Phoenix 直接改 C CFG 或 LLVM CFG。

当前 C 试点还有一个临时状态：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:19`
  `StructuredGotoAdapter` 同时做 C CFG adapter、调用 `GotoStructurer`、渲染 Clang AST。
- `StructuredGoto.cpp:63`
  从旧 C CFG 构造 `StructuredCFG`。
- `StructuredGoto.cpp:105`
  直接把 `StructuredTree` 渲染回 Clang statements。

这能验证链路，但不是长期边界。长期应把 C/Solidity 的渲染器移到 backend，structuring 层只输出结构树。

# 目标架构

重构目标是接近 Angr，而不是接近 Ghidra：

```text
backend-specific CFG builder
  -> StructuredCFG
  -> RegionIdentifier
  -> RegionTree / RegionOverlay
  -> RecursiveStructurer
  -> RegionStructurer implementation
       - Goto
       - Phoenix
       - SAILR
       - Dream
       - GhidraCollapse later
  -> StructuredTree
  -> backend renderer
       - C renderer
       - Solidity renderer
```

公共 structuring 层只处理：

- block id
- edge
- region
- condition payload id
- statement payload id
- structure node
- goto/break/continue 标记

它不应该 include Clang，也不应该知道 Solidity。

# 建议重构路线

## 阶段 1：补算法注册和配置

新增一个 structurer registry：

```text
name -> factory
```

先注册：

- `goto`

保留现在 CLI 上的 C `--algo=structured-goto`，但内部改成从 registry 找算法。

判断标准：

- C/Solidity 都能指定 `goto`。
- 默认行为不变。
- 不引入 region 逻辑。

## 阶段 2：引入 Region 数据结构

新增最小 region 类型：

- `Region`
  - head block
  - member block ids
  - successor block ids
  - child regions
- `RegionTree`
  - root region
  - postorder traversal

先实现非常保守的 region identifier：

- 整函数一个 root region。
- 可选识别自然循环 region。
- 暂时不做复杂 switch/if region。

判断标准：

- `RecursiveStructurer` 可以只处理一个 root region，输出和当前 Goto 基本一致。
- 后续增加 region 识别不会影响 backend renderer。

## 阶段 3：新增 RecursiveStructurer

新增一个公共入口：

```text
RecursiveStructurer::structure(StructuredCFG, RegionTree, RegionStructurer)
```

先支持两种结果模式，参考 Angr：

- destructive reducer：算法把 region graph 折叠成一个 node。
- external result：算法直接产出一个 `StructuredNode` / subtree。

但实现上先做最小版本：

- Goto 直接 external result。
- region tree 只有 root。

判断标准：

- `GotoStructurer` 从“整图算法”迁成一个 `RegionStructurer`。
- C/Solidity smoke 不变。

## 阶段 4：引入 MutableRegionGraph

新增算法内部可变图：

- node 可以是 basic block，也可以是已经结构化的 subtree。
- 支持 pred/succ 查询。
- 支持 collapse nodes。
- 支持 virtualize edge 为 goto/break/continue。
- 支持 dominator/postdominator 查询。

这个图只属于 structuring 层，不暴露给 backend。

判断标准：

- 能在这个图上实现 sequence collapse 和 if/if-else collapse。
- 不依赖 Clang AST。

## 阶段 5：迁移 Phoenix

迁移顺序：

1. sequence
2. if / if-else
3. while / do-while / infinite loop
4. switch
5. irregular edge virtualization
6. multi-statement-expression 相关逻辑先不迁，或者作为 C 后端渲染优化处理

关键原则：

- Phoenix 只输出 `StructuredNode`。
- Phoenix 不创建 `clang::IfStmt`、`clang::WhileStmt`、`clang::SwitchStmt`。
- Phoenix 不读写旧 C CFG。

判断标准：

- 同一个 PhoenixStructurer 能被 C 和 Solidity 调用。
- C 输出能比 Goto 少明显 goto。
- Solidity 输出至少能打印结构化注释，不要求一开始生成真实 Solidity AST。

## 阶段 6：复现 SAILR

SAILR 不应作为完全独立实现，而是像 Angr 一样继承或包装 Phoenix：

- 复用 Phoenix schema reducer。
- 替换 edge virtualization 排序。
- 逐步补 SAILR 需要的前置 deoptimization pass。

这里还要预留算法相关 pass 过滤机制，类似 Angr 的 `pass_.STRUCTURING`。

判断标准：

- 可以按算法名启用/禁用某些 pre-region 或 during-region pass。
- SAILR 的行为差异集中在 edge heuristic 和配套 pass，不复制整份 Phoenix。

## 阶段 7：再考虑 GhidraCollapse

Ghidra 的 `CollapseStructure` 可以作为另一个 `RegionStructurer`：

- 输入 `MutableRegionGraph`
- 使用 rule list 折叠
- 卡住时 select goto

但这应该在 Phoenix/SAILR 路线跑通后做。否则会同时背两套 reducer。

# 风险

1. RegionIdentifier 太早做复杂会拖慢主线。
   第一版应该只做 root region + 最小自然循环。

2. Phoenix 迁移容易把 Clang AST 带进 structuring 层。
   这是必须避免的边界问题。

3. C backend 现在还有旧 Phoenix 默认路径。
   新 PhoenixStructurer 未稳定前，不应替换默认。

4. Angr 的算法依赖 AIL 语义和很多优化 pass。
   我们不能一次复刻全部，只能先复刻框架，再按算法需求补 pass。

5. Solidity 当前还没有真正 statement AST。
   短期可以继续输出结构化注释，先验证 CFG/region/算法。

# 判断标准

这次重构算成功，需要满足：

- structuring 层有算法 registry，可以按名字选择算法。
- 有 `RegionTree` 和 `RecursiveStructurer`，即使第一版只处理 root region。
- Goto 能通过新 recursive 路径输出，C/Solidity smoke 不退化。
- Phoenix 迁移时不 include Clang，不依赖旧 C CFG。
- 文档和接口能清楚说明 backend 负责 payload 和渲染，算法只负责结构树。

# 2026-06-19 实现记录：最小 RecursiveStructurer 和 Goto 迁移

本轮完成阶段 1 到阶段 3 的最小版本：新增 structurer registry、root-only region tree、recursive structurer，并让现有 `GotoStructurer` 通过这条路径运行。没有迁移 Phoenix，也没有开始 SAILR。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:15`
  新增 `Region` / `RegionTree`，先支持 root region，字段保留 `Head`、`Blocks`、`Successors`、`Children`。
- `external/NotDec-llvm2c/lib/Structuring/Region.cpp:62`
  实现 `RegionTree::addRegion()`。
- `external/NotDec-llvm2c/lib/Structuring/Region.cpp:71`
  实现 `RegionTree::getRegion()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionIdentifier.h:10`
  新增 root-only `RegionIdentifier`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:5`
  `RegionIdentifier::identifyRoot()` 把整张 `StructuredCFG` 放进一个 root region。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionStructurer.h:8`
  新增可替换的 region 级算法接口 `RegionStructurer`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RecursiveStructurer.h:27`
  新增 recursive driver 声明。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:27`
  `RecursiveStructurer::structure()` 目前只处理 root region，并把结果设为 `StructuredTree` root。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructurerRegistry.h:49`
  新增默认算法名 `goto` 和 `createStructurer()`。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:48`
  registry 第一版只注册 `goto -> GotoStructurer`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoStructurer.h:12`
  `GotoStructurer` 同时实现整图 `Structurer` 和 region 级 `RegionStructurer`。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:15`
  `GotoStructurer::structure()` 改成 `RegionIdentifier::identifyRoot()` 后调用 `RecursiveStructurer`。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:20`
  原来的整图 Goto fallback 逻辑移到 `structureRegion()`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:165`
  Solidity body builder 改为从 registry 创建默认 structurer。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:33`
  C 试点 `StructuredGotoAdapter` 改为从 registry 创建默认 structurer。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:2`
  `notdec-backend-structuring` 加入新增实现文件。

当前保留的限制：

- `RegionIdentifier` 只生成整函数 root region，还没有自然循环、switch 或 if region。
- `RecursiveStructurer` 还没有真正的 children postorder 递归，也没有 overlay/rollback。
- registry 只注册 `goto`。
- Phoenix 仍是旧 C backend 默认路径，没有迁到新 `RegionStructurer`。
- SAILR 还没开始。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-recursive-goto-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.14s`，输出仍是 label/goto 形态。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-recursive-goto-smoke.sol --tr-level=2` 通过，耗时约 `17.90s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

评分：

- 实现效果：6/10。架构入口已经变成 registry + root region + recursive driver，后续 Phoenix/SAILR 可以接这个方向。
- 复杂度：4/10。新增了几层很薄的框架，但每层现在都只做最小事情。
- 维护成本：4/10。短期多了一层转发；等 region 识别和 Phoenix 迁移后，这层才会真正发挥作用。

# 2026-06-19 实现记录：新增 MutableRegionGraph

本轮完成阶段 4 的第一步：给 structuring 层新增内部可变图 `MutableRegionGraph`。它还没有接入 Goto 输出路径，目的是给 Phoenix/SAILR 迁移准备一个不依赖 Clang、不依赖旧 C CFG 的 reducer 工作图。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:17`
  新增 `VirtualEdgeKind`，先支持 `Goto`、`Break`、`Continue`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:23`
  新增 `VirtualEdge`，记录被虚拟化的边。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:29`
  新增 `MutableRegionNode`，节点可以代表原始 block，也可以代表已经结构化出的 subtree。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:41`
  新增 `MutableRegionGraph`，提供 active node、edge、collapse、virtualize 查询和修改接口。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:22`
  `MutableRegionGraph::build()` 从 `StructuredCFG + Region` 建内部图，只保留 region 内边。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:86`
  `addEdge()` 同步维护 succ/pred。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:102`
  `removeEdge()` 同步删除 succ/pred。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:117`
  `virtualizeEdge()` 删除真实边，并记录虚拟边。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:126`
  `collapseNodes()` 把一组 active 节点折叠成一个新节点，并重连外部 pred/succ。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:184`
  `getNodeForBlock()` 支持从原始 block id 找当前 active graph node。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:5`
  `notdec-backend-structuring` 加入 `MutableRegionGraph.cpp`。

当前保留的限制：

- 还没有接入任何 structurer，所以这轮不改变输出。
- 没有 dominator/postdominator，Phoenix 迁移前还要补。
- `collapseNodes()` 只做图层折叠，不负责生成 `StructuredNode`；算法需要先创建 subtree，再把 `NodeId` 传进来。
- 没有 overlay/rollback，仍是单次 reducer 的内部状态。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-mutable-graph-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.13s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-mutable-graph-smoke.sol --tr-level=2` 通过，耗时约 `17.88s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

评分：

- 实现效果：5/10。内部 mutable graph 已有 Phoenix 迁移需要的基本形状，但还没跑任何 reducer。
- 复杂度：4/10。数据结构简单，主要是 pred/succ 一致性和 collapse 重连。
- 维护成本：5/10。后续如果 Phoenix 需要更复杂的 edge metadata 或 region overlay，这里还会继续扩展。

# 2026-06-19 实现记录：公共 Phoenix 入口和 sequence reducer

本轮开始迁移 Phoenix 到新 structuring 架构，但没有继续强行迁移 if/loop/switch 规则。原因是当前 `StructuredNode::If` / `While` / `Switch` 只有通用 `Children`，还不能清楚表达 true body、false body、follow、loop body、break/continue target、switch default/case body。继续迁旧 Phoenix 会把 C 侧的 Clang 语义塞进公共 structuring 层，所以这里停在接口决策点。

已完成内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:9`
  新增公共 `PhoenixStructurer`，同时实现整图 `Structurer` 和 region 级 `RegionStructurer`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:159`
  `PhoenixStructurer::structure()` 走 `RegionIdentifier -> RecursiveStructurer`，和 Goto 使用同一套入口。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:164`
  `PhoenixStructurer::structureRegion()` 从 `StructuredCFG + Region` 构造 `MutableRegionGraph`，先跑 sequence reducer，再 fallback 到 label/goto 输出。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:67`
  `reduceSequenceOnce()` 实现第一条 Phoenix-style 规约：`A -> B`、`B` 只有一个前驱、`A` 是单后继 fallthrough 时，把两个 graph node 折叠成一个 sequence subtree。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:35`
  `MutableRegionNode` 新增 `Blocks`，折叠后保留原始 block 列表，便于 reducer 生成 subtree 和 fallback terminator。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:145`
  `collapseNodes()` 合并成员节点的 `Blocks`，新节点继续代表折叠后的原始 block 序列。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:12`
  registry 增加 `phoenix -> PhoenixStructurer`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:17`
  C backend 新增 `--algo=structured-phoenix`，专门验证公共 Phoenix 链路；旧 `--algo=phoenix` 仍走旧 C Phoenix。
- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuredGoto.h:13`
  `StructuredGoto` adapter 改成可以传 structurer 名称，默认仍是 `goto`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:34`
  C adapter 通过 `createStructurer(SA.getStructurerName())` 选择公共算法。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2137`
  `SA_StructuredPhoenix` 复用 C adapter，传入公共 `phoenix`。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:9`
  `notdec-backend-structuring` 加入 `PhoenixStructurer.cpp`。

当前明确限制：

- 这不是完整 Phoenix，只迁了算法入口和 sequence reducer。
- 真实 Phoenix 的 if/if-else/while/do-while/switch/virtual edge 规则还没迁。
- SAILR 还没开始，因为 SAILR 依赖 Phoenix 的 virtual edge 选择策略，必须等公共结构树接口先定下来。
- `RegionIdentifier` 仍是 root-only，后续要补 loop/switch/if region，才能接近 Angr 的递归结构化效果。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-goto-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.15s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-smoke.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.14s`，输出从 40 行变成 39 行，说明 sequence reducer 已经折叠了一个 fallthrough 块。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-structuring-smoke.sol --tr-level=2` 通过，耗时约 `17.76s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

下一步需要先决定公共结构树怎么表达分支和循环：

- `If` 是否明确分成 `Then`、`Else`、`Follow`，还是继续用 children 下标约定。
- `Loop` 是否需要显式 `Body`、`ConditionBlock`、`Follow`、`BreakTarget`、`ContinueTarget`。
- `Switch` 是否需要 case body 节点和 default body 节点，而不是当前的 case target 列表。
- `VirtualEdge` 记录的是 graph node id 还是原始 block id；SAILR 后续需要稳定比较和排序，原始 block id 更容易跨 collapse 使用。

评分：

- 实现效果：5/10。公共 Phoenix 路径能跑，sequence reducer 已经使用 `MutableRegionGraph`，但还没覆盖 Phoenix 的核心规则。
- 复杂度：5/10。新增代码集中在一个 structurer 和一个 C 试点算法名，没有改默认行为。
- 维护成本：5/10。当前停在合适边界；继续前需要先把结构树接口补清楚，否则后面会越来越难维护。

# 2026-06-19 实现记录：按 Angr 边界补结构树字段

本轮按 Angr 的边界继续重构：公共 structuring 层开始显式表达 `ConditionNode` / `LoopNode` / `SwitchCaseNode` 需要的语义字段，renderer 优先支持这些字段，但 fallback 节点继续走旧 `Children`，避免改变 Goto fallback 的输出。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:43`
  新增 `StructuredSwitchCase`，case 可以直接指向结构化后的 body。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:87`
  `StructuredNode` 新增 `BreakTarget`、`ContinueTarget`、`Then`、`Else`、`Body`、`Default`、`StructuredCases`，对应 Angr 的 condition、loop、switch case 节点边界。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:23`
  `VirtualEdge` 新增 `FromBlock` / `ToBlock`，保留原始 block id，后续 SAILR 的 edge 排序不依赖会变化的 graph node id。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:120`
  `virtualizeEdge()` 删除边前记录原始 source tail block 和 destination head block。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:48`
  Goto fallback 在保留旧 `Children` 的同时填 `If.Then` / `If.Else`。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:59`
  Goto fallback 在保留旧 `Children` 的同时填 `Switch.Default` 和 `StructuredCases`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:66`
  Solidity renderer 支持语义 `If.Then/Else`；只有 `Children` 为空时才启用，fallback 仍按旧 children 输出。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:81`
  Solidity renderer 支持语义 `Switch.StructuredCases/Default`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:114`
  Solidity renderer 支持 loop 的 `Body` 字段。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:127`
  C renderer 支持语义 `If.Then/Else`，并保留旧 fallback 渲染。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:202`
  C renderer 支持语义 `Switch.StructuredCases/Default`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:54`
  `isFallthroughTo()` 改用 collapsed node 的 tail block 判断 fallthrough，sequence reducer 可以继续处理已折叠节点。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:92`
  新增保守的单臂 if reducer：只处理 true 分支是 body、false 分支是 follow、body 无外部入口且唯一后继是 follow 的形态。

当前保留限制：

- 单臂 if reducer 不处理 false 分支是 body 的情况，因为当前还没有公共的条件取反 payload 表达。
- 没有迁移 if-else、loop、switch 和 virtual edge 规则。
- SAILR 还没开始；下一步至少需要 dominator/postdominator 和 virtual edge 排序入口。
- 现有测试样例没有明显触发单臂 if reducer，当前主要验证编译、fallback 不变和 sequence reducer 仍正常。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-goto-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.14s`，输出 40 行，保持旧 fallback 形态。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-smoke.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.13s`，输出 39 行，sequence reducer 仍生效。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-angr-boundary-smoke.sol --tr-level=2` 通过，耗时约 `17.50s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

新的决策点：

- 条件取反应该放在哪里：在 payload 层提供 `invertCondition(PayloadRef)`，还是在 structuring 层增加 `ConditionNegated` 标志。
- 如果按 Angr 的 `ConditionProcessor` 方向，最好由 backend 提供 condition processor，structuring 层只记录原条件和是否取反。
- 没有这个决策，false-branch if、if-else 合并、while 条件方向都会卡住。

评分：

- 实现效果：6/10。公共结构树已经能承载 Angr 风格节点，Phoenix 有了第二条保守 reducer。
- 复杂度：5/10。字段增加较多，但都集中在公共结构节点和 renderer。
- 维护成本：5/10。保留旧 `Children` 兼容 fallback，短期会有双路径；等 reducer 迁完后可以收窄。

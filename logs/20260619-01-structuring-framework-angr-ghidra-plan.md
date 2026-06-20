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

# 2026-06-19 实现记录：条件取反和 if-else reducer

本轮按 Angr 的方向处理条件方向：structuring 层只记录“这个条件需要取反”，不理解 payload 语义。具体怎么把 `cond` 变成 `!cond`，交给 C/Solidity renderer 处理。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:93`
  `StructuredNode` 新增 `ConditionNegated`，用于表达 Angr `ConditionNode` 里的条件方向。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:43`
  新增 `conditionText()`，在 Solidity 注释输出里处理 `ConditionNegated`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:72`
  `If` 注释输出改为使用 `conditionText()`。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:118`
  `While` / `DoWhile` 注释输出也改为使用 `conditionText()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:167`
  C adapter 新增本地 `invertCond()`，优先反转比较运算符，反转不了再生成 `!cond`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:196`
  C adapter 新增 `conditionExpr()`，只有语义结构节点渲染时才使用 `ConditionNegated`，fallback 仍走旧逻辑。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:199`
  `getNodeForBlock()` 改成从 active node 的 `Blocks` 列表里查找，支持在已折叠节点里继续匹配后续规则。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:107`
  `reduceIfOnce()` 从 true/false 两个 successor 找当前 active graph node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:120`
  支持 true 分支是 body、false 分支是 follow 的单臂 if。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:124`
  支持 false 分支是 body、true 分支是 follow 的单臂 if，并设置 `ConditionNegated`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:129`
  支持 true/false 两个分支有共同 follow 的 if-else。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:158`
  生成 `IfNode.ConditionNegated`、`Then`、可选 `Else`。

当前保留限制：

- 只匹配 pred/succ 非常干净的 if 形态，复杂共享入口、多个出口、异常 switch 入口仍不碰。
- 还没有 loop reducer、switch reducer、virtual edge fallback。
- SAILR 还没开始；下一步需要加 dominator/postdominator 和 virtual edge ordering 接口。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-goto-cond-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.14s`，输出 40 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-cond-smoke.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.14s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-cond-smoke.sol --tr-level=2` 通过，耗时约 `17.92s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

新的判断：

- 条件方向问题可以先用 `ConditionNegated` 解决，不需要让 structuring 层持有 backend-specific condition processor。
- 真正接近 Angr 的下一步不是继续加字段，而是给 `MutableRegionGraph` 补 dominator/postdominator，并把 virtual edge ordering 做成 `PhoenixStructurer` 可覆盖的方法，让 SAILR 只改排序策略。

评分：

- 实现效果：6/10。条件方向已解开，if reducer 覆盖面扩大，但仍是保守规则。
- 复杂度：5/10。`ConditionNegated` 简单，C adapter 里多了一份局部条件反转逻辑。
- 维护成本：5/10。以后如果 C/Solidity 都需要复杂条件处理，可以再抽 backend condition processor；现在先不用过早抽象。

# 2026-06-19 实现记录：图分析和 virtual edge ordering 接口

本轮继续按 Angr 的方向推进 SAILR 需要的基础设施，但没有开始真正删边。原因是删边以后必须决定 virtualized edge 怎么落回 `StructuredTree`，否则容易把 CFG 边删掉却没有对应的 goto/break/continue 输出。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:46`
  新增 `MutableRegionGraphAnalysis`，保存 entry、exit、dominator、postdominator 和 node order。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:53`
  新增 `dominates()` / `postDominates()` 查询。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:75`
  `MutableRegionGraph` 新增 `analyze()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:82`
  新增迭代式 dominator 计算。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:121`
  新增迭代式 postdominator 计算。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:168`
  实现 `MutableRegionGraphAnalysis::dominates()` / `postDominates()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:374`
  `MutableRegionGraph::analyze()` 基于 active nodes 生成 entry、exit、dominators、postdominators 和 DFS postorder 派生的 node order。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:22`
  `PhoenixStructurer` 新增可覆盖的 `orderVirtualizableEdges()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:180`
  新增 `collectVirtualizableEdges()`，先收集 active graph 的候选边，不实际删边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:280`
  `PhoenixStructurer::orderVirtualizableEdges()` 默认按 Angr Phoenix 的 Chick ordering 思路排序：目标节点顺序、目标入度、源出度、原始 block id。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:329`
  `structureRegion()` 调用 `Graph.analyze()` 和 `orderVirtualizableEdges()`，但暂不使用结果改变输出。

当前暂停点：

- 需要决定 virtualized edge 在公共结构树里如何落地。
- Angr 的 `_virtualize_edge()` 会改 region graph，同时把 conditional jump 拆成 `ConditionNode + Jump`。我们这里没有 AIL statement，只有 payload id 和结构节点，所以不能直接照搬。
- 可选方向：
  - 在 source node 的结构树尾部追加 `Goto/Break/Continue` 节点。
  - 给 `MutableRegionGraph::virtualizeEdge()` 返回一个待渲染的 `VirtualEdge`，由 reducer collapse 时显式插入到 source subtree。
  - 在 `StructuredNode` 上增加 `TrailingControl` 一类字段。
- 这个点需要先定，不然继续实现 Phoenix last-resort edge virtualization 会有语义缺口。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-goto-order-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.15s`，输出 40 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-order-smoke.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.15s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-order-smoke.sol --tr-level=2` 通过，耗时约 `17.83s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

评分：

- 实现效果：5/10。SAILR ordering 的公共入口有了，但还没接真正 virtualize edge。
- 复杂度：5/10。dominator/postdominator 是简单迭代算法，够当前 region graph 使用。
- 维护成本：5/10。接口边界接近 Angr，下一步需要把 virtual edge 到结构树的落地方式定清楚。

# 2026-06-19 实现记录：Phoenix virtual edge fallback 落地

本轮把上一个暂停点先按保守方式接上：Phoenix 在普通 reducer 不再变化时，按
`orderVirtualizableEdges()` 选一条边调用 `MutableRegionGraph::virtualizeEdge()`。
最后 fallback 输出仍按原 CFG terminator 渲染，只有 virtualized edge 不在原始
terminator successor 里时，才额外追加 `goto` / `break` / `continue`。这样不会因为
region graph 删边改掉原始 branch/switch 的基本语义。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:27`
  新增 `virtualizeOneEdge()`，供 Phoenix 和后续 SAILR 子类复用同一套删边入口。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:204`
  新增 `groupVirtualEdgesBySource()`，把 graph 记录的 virtual edge 按 source node 分组。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:213`
  新增 `appendControlTransfer()`，把 virtual edge kind 落成公共 `StructuredNode`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:240`
  `appendFallbackNode()` 接收 source node 对应的 virtual edges。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:316`
  fallback 只在目标不属于原始 CFG successor 时追加额外控制转移，避免重复 goto。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:354`
  实现 `PhoenixStructurer::virtualizeOneEdge()`，先分析 graph，再用可覆盖的 ordering 选边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:377`
  `structureRegion()` 的 reducer loop 增加 virtual edge fallback，并保留 1000 次上限。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:391`
  最终 fallback 输出前读取 `Graph.virtualEdges()`，把已删边补回结构树。

当前保留限制：

- virtual edge kind 现在仍统一从 `Goto` 开始，`Break` / `Continue` 的识别还没迁。
- Phoenix 的 loop/switch reducer 还没迁；这次只解决“删边后不丢输出”的最小闭环。
- SAILR 还没实现；下一步应该加 `SAILRStructurer`，先只 override ordering。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-goto-virtual-smoke.c --tr-level=2 --algo=structured-goto` 通过，耗时约 `0.14s`，输出 40 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-virtual-smoke.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.15s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-virtual-smoke.sol --tr-level=2` 通过，耗时约 `17.81s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

评分：

- 实现效果：6/10。Phoenix 现在可以实际删 virtual edge，且 fallback 不丢控制流。
- 复杂度：5/10。新增逻辑集中在 Phoenix fallback，没有改 renderer 和公共 node 字段。
- 维护成本：5/10。短期还有 fallback 补 goto 的兼容逻辑；等 loop/switch reducer 迁完后可以减少这条路径。

# 2026-06-19 实现记录：接入 SAILR structurer 骨架

本轮按 Angr 的边界接入 SAILR：SAILR 继承 Phoenix，不复制 reducer，只覆盖 virtual edge
ordering。当前复刻的是 Angr `SAILRStructurer._order_virtualizable_edges()` 的三步启发式：
少 sibling edge、删边后 postdominator 数更多、目标是 return。最后仍回退到 Phoenix 的
Chick ordering 做稳定排序。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:24`
  `orderVirtualizableEdges()` 增加 `StructuredCFG` 参数，让 ordering 能读取 block payload。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:28`
  `virtualizeOneEdge()` 同步接收 `StructuredCFG`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:325`
  Phoenix ordering 签名同步调整，默认不使用 CFG。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:356`
  `virtualizeOneEdge()` 调用新的 ordering hook。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRStructurer.h:11`
  新增 `SAILRStructurer`，继承 `PhoenixStructurer`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:37`
  实现 H1：优先选择 sibling edge 更少的候选边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:67`
  实现 H2：小图上复制 graph、删除候选边、重新计算 postdominator pair 数。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:93`
  实现 H3：目标 node 的 tail block 是 `Return` 时优先。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:108`
  `SAILRStructurer::orderVirtualizableEdges()` 串起 H1/H2/H3，并回退到 Phoenix ordering。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:10`
  把 `SAILRStructurer.cpp` 加入 `notdec-backend-structuring`。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:17`
  registry 支持 `"sailr"`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:19`
  CLI 增加 `--algo=structured-sailr`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2140`
  C backend 接入 `StructuredGoto(..., "sailr")`。

当前保留限制：

- Angr 的 H3 判断的是 structured node simple return；这里先用 CFG tail `Return` 近似。
- SAILR 论文里配套的 deoptimization / duplication reverter 还没有接入。
- 当前 Phoenix reducer 还缺 loop/switch 规则，所以 SAILR ordering 的收益还受限。
- Solidity 仍走默认 `"goto"` structurer；是否切到 SAILR 需要后续单独比较。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-smoke.c --tr-level=2 --algo=structured-sailr` 通过，耗时约 `0.14s`，输出 39 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-sailr-baseline.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.15s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-sailr-smoke.sol --tr-level=2` 通过，耗时约 `17.60s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

评分：

- 实现效果：6/10。SAILR 已作为独立算法入口接入，并复用 Phoenix 的 reducer/fallback。
- 复杂度：5/10。新增类很小，唯一接口变化是 ordering hook 多拿一个 `StructuredCFG`。
- 维护成本：5/10。后续可以继续在 SAILR 类里补 Angr 的策略，不需要再改 C/Solidity renderer。

# 2026-06-19 实现记录：保守 switch reducer

本轮先迁一个很窄的 Phoenix switch reducer：只处理 switch head 的每个 successor 都是
独立 case/default body，并且这些 body 都单出口到同一个 follow 的形态。暂不处理 case
目标就是 follow、tail case、case fallthrough、irregular entry 和嵌套 switch。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:22`
  新增 `makeBreak()`，给 structured switch case body 补 `break`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:188`
  新增 `buildSwitchCaseBody()`，把 case node 的已有结构树或 block 列表包成 case body。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:207`
  新增 `reduceSwitchOnce()`，只匹配共同 follow、单入口、单出口的 switch region。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:258`
  校验所有 case/default body 只能由 switch head 进入，并且只流向共同 follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:281`
  生成公共 `StructuredNodeKind::Switch`，填充 `Default` 和 `StructuredCases`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:297`
  collapse switch head 和 case/default body，保留到 follow 的 graph 边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:510`
  Phoenix reducer loop 接入 `reduceSwitchOnce()`。

当前保留限制：

- 真实 `fortune.o3.wasm.ll` 输出里仍只看到 fallback switch，说明这个 reducer 当前只覆盖很窄形态。
- 用临时 `/tmp/notdec-simple-switch.ll` 走顶层 `notdec -o .c` 会先触发现有 MLsub 空指针断言，没法作为正向集成测试。
- `notdec-llvm2c` standalone 工具后来已接入 shared structuring 算法；见下一节记录。
- 下一步要么补一个直接调用 structuring library 的小单测，要么先迁 loop reducer，再用真实样例观察收益。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-backend-c notdec -j4` 通过。
- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-switch-final.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `0.14s`，输出 39 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-switch-smoke.c --tr-level=2 --algo=structured-sailr` 通过，耗时约 `0.14s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-switch-smoke.sol --tr-level=2` 通过，耗时约 `18.05s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-structured-phoenix-switch.c --tr-level=2 --algo=structured-phoenix` 通过，耗时约 `308.82s`，输出 26996 行、2 个 `switch`、562 个 `goto`。

评分：

- 实现效果：4/10。公共 switch 节点生成链路接上了，但真实样例还没有明显触发。
- 复杂度：5/10。规则保守，代码集中在 Phoenix reducer；没有改公共节点和 renderer。
- 维护成本：5/10。后续需要补 structuring 级单测，否则正向覆盖不够稳定。

# 2026-06-19 实现记录：standalone notdec-llvm2c 默认接入新 structuring

本轮按“旧结构恢复后续会被重构掉”的方向处理 standalone 工具：`notdec-llvm2c` 直接暴露
shared structuring 算法，并把默认算法从旧 `phoenix` 切到 `structured-sailr`。旧
`goto` / `phoenix` 选项暂时保留，方便对比和回归。

修改内容：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:10`
  旧 `goto` / `phoenix` 的 CLI 描述标成 legacy。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:13`
  standalone `--algo` 暴露 `structured-goto`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:16`
  standalone `--algo` 暴露 `structured-phoenix`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:19`
  standalone `--algo` 暴露 `structured-sailr`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:23`
  默认值改成 `SA_StructuredSAILR`。

验证：

- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c --help-hidden` 显示
  `structured-goto`、`structured-phoenix`、`structured-sailr`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-structured-phoenix.c --algo=structured-phoenix`
  通过，耗时约 `0.09s`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-default.c`
  通过，耗时约 `0.09s`，证明默认路径可用。

当前判断：

- standalone 已经可以作为新 structuring 的主要验证入口。
- 最小 switch 样例仍输出 fallback switch 里的 `goto follow`，这是 reducer 覆盖面问题，不再是工具入口问题。

评分：

- 实现效果：7/10。standalone 路径已切到新算法，后续验证不用绕顶层 MLsub。
- 复杂度：2/10。只改 CLI 默认值和描述。
- 维护成本：3/10。旧选项保留做对比；等新链路稳定后再删除旧实现入口。

# 2026-06-19 实现记录：standalone 验证 simple switch reducer

本轮用 standalone `notdec-llvm2c` 验证 shared structuring，修掉 simple switch 仍退回
`goto follow` 的问题。核心原因有三个：switch 的所有 case/default 可能直接指向同一个
follow；C adapter 会把原 CFG terminator 当普通 payload 带进 basic block；C renderer
还没有真正输出公共 `Break` / `Continue` 节点。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:238`
  switch target 允许没有 successor，用于表达 case/default 直接指向 follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:254`
  如果所有 switch successor 都是同一个节点，把它作为 follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:265`
  collapse 成员跳过 follow，只 collapse switch head 和真实 case body。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:294`
  case/default 目标是 follow 时生成空 body + `break`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:374`
  新增 `nodeTreeContainsKind()`，用于判断 collapsed subtree 是否已包含 semantic switch。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:411`
  对已生成 semantic switch 的 collapsed node，不再追加原 switch terminator fallback。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:77`
  C adapter 收集 payload 时跳过原 CFG terminator，避免把旧 switch/if 当普通语句重复输出。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:153`
  C renderer 支持公共 `Break` 节点。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:157`
  C renderer 支持公共 `Continue` 节点。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-structured-phoenix.c --algo=structured-phoenix`
  通过，耗时约 `0.17s`，输出 0 个 `goto`、3 个 `break`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-default.c`
  通过，耗时约 `0.17s`，默认 `structured-sailr` 输出 0 个 `goto`、3 个 `break`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-rootterm2-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.14s`，输出 39 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-rootterm2-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`，输出 39 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-rootterm2-smoke.sol --tr-level=2`
  通过，耗时约 `17.72s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

当前判断：

- standalone 已能正向验证 shared switch reducer。
- `appendFallbackNode()` 现在只对包含 semantic switch 的 collapsed node 跳过原 terminator；if/loop 等 reducer 还没完全迁完，所以暂时继续保留原 terminator fallback，避免丢控制流。

评分：

- 实现效果：6/10。simple switch 有了正向验证，C adapter 能渲染 break/continue。
- 复杂度：5/10。fallback 跳过条件仍是过渡逻辑，等 if/loop reducer 完整后应继续收窄。
- 维护成本：5/10。standalone 覆盖更可靠，但还需要把临时 `/tmp` 样例沉淀成正式测试。

# 2026-06-19 实现记录：self-loop / do-while reducer

本轮继续迁 Phoenix loop 里的最小闭环：支持单块自环。无条件自环生成
`InfiniteLoop`，条件分支里一边回到自己时生成 `DoWhile`，另一边保留为 follow。
同时修了 sequence reducer 丢失已结构化 subtree 的问题，否则 loop 会先生成、再被
`entry -> loop` 的 sequence 合并打回普通 block。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:251`
  `addEdge()` 允许 self-edge，让 region graph 能表达自环。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:62`
  新增 `appendRegionNode()`，sequence 合并时优先保留已有 `StructuredRoot`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:74`
  新增 `buildSequenceNode()`，替代 sequence reducer 里只按 block 重建的路径。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:87`
  新增 `buildLoopBody()`，loop body 复用已有 structured subtree。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:348`
  新增 `reduceSelfLoopOnce()`，匹配 self-edge。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:366`
  单 successor 自环生成 `StructuredNodeKind::InfiniteLoop`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:368`
  条件自环生成 `StructuredNodeKind::DoWhile`，false 分支回环时设置 `ConditionNegated`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:476`
  `nodeTreeContainsStructuredControl()` 把 switch/while/do-while/infinite-loop 统一识别为已结构化控制流。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:617`
  Phoenix reducer loop 接入 `reduceSelfLoopOnce()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:161`
  C renderer 接入公共 `While`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:164`
  C renderer 接入公共 `DoWhile`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:167`
  C renderer 接入公共 `InfiniteLoop`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:215`
  新增 `trueExpr()`，用于渲染 `while (1)`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:340`
  新增 `renderWhile()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:348`
  新增 `renderDoWhile()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:356`
  新增 `renderInfiniteLoop()`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop.c --algo=structured-phoenix`
  通过，耗时约 `0.08s`，输出 1 个 `do`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop-default.c`
  通过，耗时约 `0.09s`，默认 `structured-sailr` 输出 1 个 `do`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-loop-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.17s`，输出 0 个 `goto`、3 个 `break`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-loop2-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.14s`，输出 38 行，原自环 goto 被结构化成 `do ... while`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-loop2-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`，输出 38 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-loop2-smoke.sol --tr-level=2`
  通过，耗时约 `17.62s`。
- Solidity smoke 输出仍有 471 个 `// block_...`、454 个 `// goto block_...`、12 个 `emit Event_...`、160 个 `revert();`。

当前保留限制：

- 只支持单块 self-loop；`Header -> Body -> Header` 形式的 while 还没迁。
- `continue` / `break` edge kind 识别还没迁，当前主要依赖 structured loop 自身消掉回边。
- 临时无条件无限循环 IR 会在 standalone 前置 LLVM pass 里触发 RAUW 断言，未作为本轮验证标准。

评分：

- 实现效果：6/10。do-while 已能正向验证，sequence 合并不再丢 structured subtree。
- 复杂度：5/10。self-edge 支持会影响 graph 分析，但这是 loop structuring 必需能力。
- 维护成本：5/10。loop 覆盖面仍窄，后续要继续迁多块 loop 和 break/continue 分类。

# 2026-06-19 实现记录：C backend 旧入口改接共享 structuring，补两块 while reducer

这轮把 `notdec-llvm2c` 的旧算法入口也收进 shared structuring 路线里，避免 C backend 还有一部分路径落回旧 `Goto` / `Phoenix` 类。顺手补了一个很保守的两块 while 规则，让最简单的 `Header -> Body -> Header` 能先结构化出来，再继续靠后续 reducer 处理复杂边界。

修改内容：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2127`
  `SA_Goto` 现在直接走 `StructuredGoto`，不再走旧 `Goto` 类。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2130`
  `SA_Phoenix` 现在也改成 `StructuredGoto(*this, "phoenix")`，旧入口只是别名。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:10`
  旧 `goto` / `phoenix` 描述改成 alias，`structured-goto` / `structured-phoenix` / `structured-sailr` 继续保留。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:348`
  新增 `reduceTwoBlockWhileOnce()`，只匹配很窄的两块循环：header 只有一个 block、`branch` 终结、两条后继里有一条是 body，body 再单独回到 header。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:403`
  两块 while 折叠时只 collapse header 和 body，不碰 follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:702`
  reducer 顺序改成每轮只做一个动作，优先 while / self-loop，再做 sequence / if / switch，避免先把 loop header 吸进 sequence。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-two-block-call.ll -o /tmp/notdec-while-two-block-call.c --algo=structured-phoenix`
  通过，耗时约 `0.08s`，输出 1 个 `while`，保留 1 个 `goto` 到 follow。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop-structured-phoenix.c --algo=structured-phoenix`
  通过，耗时约 `0.09s`，输出 1 个 `do`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-structured-phoenix.c --algo=structured-phoenix`
  通过，输出 1 个 `switch`、3 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-legacy-goto.c --algo=goto`
  通过，说明旧名字已经转到 shared structuring 路径。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-alias-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.15s`，输出 38 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-legacy-phoenix-alias-smoke.c --tr-level=2 --algo=phoenix`
  通过，耗时约 `0.14s`，输出 38 行。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-alias-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`，输出 38 行。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-structuring-alias-smoke.sol --tr-level=2`
  通过，耗时约 `17.76s`，输出仍是 `471` 个 `// block_...`、`454` 个 `// goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 旧 C backend 入口已经不再分叉到旧 structurer，结构恢复主线统一到了 shared structuring。
- 两块 while 现在能出最小闭环，但 follow 还会保留一个显式 `goto`，这和后续 break/continue 分类、以及 fallback 收口还有关，不算当前这轮的失败点。

评分：

- 实现效果：6/10。入口收拢了，最小两块 while 也能跑。
- 复杂度：4/10。只是把旧分支改成别名，代价小。
- 维护成本：4/10。后续再迁别的算法时，不用再照顾两套入口。

# 2026-06-19 实现记录：收窄 semantic control fallback

上一轮两块 while 已能结构化，但 `Header -> Body -> Header` 后面的自然 follow 还会被补成
`goto exit`。原因是 fallback 判断只看 collapsed node 的最后一个原始 block；两块 while collapse 后最后一个 block 是 body，它不直接连 follow，于是把 header 的正常 false edge 当成缺失边补了出来。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:513`
  新增 `nodeHasSuccessorTarget()`，检查 collapsed region 里任一原始 block 是否连到目标。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:570`
  semantic control subtree 的 virtual edge fallback 改用 region 级 successor 判断，避免自然 follow 被重复打印成 `goto`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-two-block-call.ll -o /tmp/notdec-while-two-block-call-fallback.c --algo=structured-phoenix`
  通过，耗时约 `0.09s`，输出 1 个 `while`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-fallback-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.17s`，输出 1 个 `switch`、3 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop-fallback-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.09s`，输出 1 个 `do`、0 个 `goto`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-fallback-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.14s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-fallback-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-fallback-smoke.sol --tr-level=2`
  通过，耗时约 `17.64s`，输出仍是 `471` 个 `// block_...`、`454` 个 `// goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这个改动没有实现 break/continue 分类，只是把已经结构化控制流的自然后继从 fallback 里排除。
- 后续仍要补真正的 loop exit / continue edge 分类，用于多出口 loop 和 virtualized backedge。

评分：

- 实现效果：6/10。两块 while 的明显冗余 goto 消掉了。
- 复杂度：3/10。判断范围从 tail block 扩到 collapsed region，逻辑很小。
- 维护成本：4/10。仍是过渡 fallback，但比只看 tail block 更接近 region graph 的真实含义。

# 2026-06-19 实现记录：线性多块 while reducer

这轮继续迁旧 Phoenix 的 while 结构恢复，但只做保守版本：支持
`Header -> Body1 -> ... -> Latch -> Header` 这种线性 body。body 内每个中间节点必须是单后继 fallthrough，body entry 只能由 header 进入，latch 只能回到 header。复杂 loop、多个出口、continue/break 分类仍不在这轮处理。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:348`
  新增 `collectLinearLoopBody()`，从 body entry 沿单后继链收集 loop body。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:385`
  新增 `reduceLinearWhileOnce()`，生成公共 `StructuredNodeKind::While`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:439`
  while body 用 `buildSequenceNode()` 保留已有 structured subtree。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:817`
  reducer 顺序里先尝试线性多块 while，再回退到原两块 while。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.c --algo=structured-phoenix`
  通过，耗时约 `0.10s`，输出 1 个 `while`、0 个 `goto`，body 内保留 `a(); b();`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-two-block-call.ll -o /tmp/notdec-while-two-block-call-linear-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.08s`，输出 1 个 `while`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-linear-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.18s`，输出 1 个 `switch`、3 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop-linear-regress.c --algo=structured-phoenix`
  通过，耗时约 `0.08s`，输出 1 个 `do`、0 个 `goto`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-linear-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.14s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-linear-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-linear-smoke.sol --tr-level=2`
  通过，耗时约 `17.55s`，输出仍是 `471` 个 `// block_...`、`454` 个 `// goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- shared Phoenix 的 while 覆盖面从两块 loop 扩到线性多块 loop。
- 这还不是旧 Phoenix 的完整 `refineLoop()`；多出口 loop、非线性 body、break/continue edge kind 仍待迁。

评分：

- 实现效果：6/10。覆盖面扩大，但仍是保守 reducer。
- 复杂度：5/10。增加了一个 body 收集函数，没有改 graph 公共接口。
- 维护成本：5/10。后续做 natural loop reducer 时，这段可以作为线性 fast path 保留，也可以被更通用逻辑替换。

# 2026-06-19 实现记录：terminal if/else reducer 和 C adapter return 识别

这轮补一个简单但常见的 Phoenix if 规约：条件分支的两个 successor 都是终止块时，直接生成
`if/else`，不再输出 `if (...) goto ret1; goto ret0;`。实现时发现 C adapter 里 Clang CFG 的 return block 会带人工 successor，shared structuring 看不到真正的终止块，所以先修 adapter，再补 reducer。

修改内容：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:75`
  C adapter 收集 block payload 时记录是否存在 `ReturnStmt`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:87`
  含 `ReturnStmt` 的 block 映射成 `TerminatorKind::Return`，不再把 Clang CFG 的人工 successor 带进 `StructuredCFG`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:162`
  `reduceIfOnce()` 新增 terminal node 判断，只接受 successor 为空且 terminator 是 `Return` 或 `Unreachable` 的节点。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:187`
  支持 true/false 两个分支都是 terminal block 的 if/else。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:468`
  删除重复的两块 while reducer；现在由 `reduceLinearWhileOnce()` 覆盖两块和线性多块 while。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:773`
  reducer 顺序里不再调用已删除的两块 while reducer。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return-after.c --algo=structured-phoenix`
  通过，输出 1 个 `if`、0 个 `goto`、2 个 `return`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-two-block-call.ll -o /tmp/notdec-while-two-block-call-terminal-regress.c --algo=structured-phoenix`
  通过，输出 1 个 `while`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-terminal-regress.c --algo=structured-phoenix`
  通过，输出 1 个 `while`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-terminal-regress.c --algo=structured-phoenix`
  通过，输出 1 个 `switch`、3 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-do-loop.ll -o /tmp/notdec-do-loop-terminal-regress.c --algo=structured-phoenix`
  通过，输出 1 个 `do`、0 个 `goto`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-terminal-smoke.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.13s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-terminal-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`，输出 38 行、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-terminal-smoke.sol --tr-level=2`
  通过，耗时约 `17.66s`，输出仍是 `471` 个 `// block_...`、`454` 个 `// goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- C backend 转 shared CFG 时，return 语义更准确；这会让后续 Phoenix/SAILR reducer 少看到人工 exit 边。
- terminal if/else 已覆盖，但普通 if-return + fallthrough、early return、非终止 else 还没迁。

评分：

- 实现效果：6/10。减少了一类明显 goto，并修正了 adapter 的 return 形状。
- 复杂度：4/10。adapter 变更很小，reducer 只加一个 terminal 分支。
- 维护成本：4/10。删除重复两块 while reducer后，loop reducer 分支更少。

# 2026-06-19 实现记录：C renderer 只保留 goto 目标 label

shared structuring 逐步生成 `if/else`、`while`、`switch` 后，C 输出里还保留了很多内部 label，例如 `if { ret1: return ... }` 或 `while { body: ... }`。这些 label 没有 goto 引用，继续输出会让结构化结果看起来仍像 goto fallback。这轮只在 C renderer 层做保守清理：先遍历 `StructuredTree` 收集所有 `Goto.Target`，渲染 `Label` 时只有被 goto 引用才输出。

修改内容：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:31`
  新增 `TargetedLabels`，记录仍被 `Goto` 节点引用的 block。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:42`
  structuring 完成后先调用 `collectGotoTargets()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:71`
  新增 `collectGotoTargets()`，递归遍历 sequence、if、switch、loop body。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:157`
  渲染 `Label` 时检查 `TargetedLabels`，没有 goto 指向的 label 不输出。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return-label-clean.c --algo=structured-phoenix`
  通过，输出 0 个 label、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-label-clean.c --algo=structured-phoenix`
  通过，输出 1 个 `while`、0 个 label、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-label-clean.c --algo=structured-phoenix`
  通过，输出 1 个 `switch`、3 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return-structured-goto-label.c --algo=structured-goto`
  通过，输出 2 个 label、2 个 `goto`，说明仍被 goto 引用的 label 没被删。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-label-clean.c --tr-level=2 --algo=structured-phoenix`
  通过，耗时约 `0.14s`，输出从 38 行降到 34 行，0 个 label、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-sailr-label-clean.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`，输出 34 行，0 个 label、0 个行首 `goto`、1 个 `do`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-label-clean.sol --tr-level=2`
  通过，耗时约 `17.73s`，输出仍是 `471` 个 `// block_...`、`454` 个 `// goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这个改动不改变 shared tree，只改变 C renderer 的 label 输出。
- 对 fallback goto 路径仍保留必要 label；对已经结构化的 if/loop/switch 清掉无用 label。

评分：

- 实现效果：6/10。结构化输出更接近正常 C，明显减少 label 噪音。
- 复杂度：3/10。只多一次 tree 遍历。
- 维护成本：3/10。逻辑集中在 C renderer，不影响 Solidity renderer。

# 2026-06-19 实现记录：linear while + break reducer

这轮继续迁 Phoenix 的循环恢复，但只补一个窄形状：`while (cond)` 的线性 body 中有一条条件边跳到循环出口时，生成 `if (...) break;`。这个形状在 C adapter 里还会遇到“两个 return block 被拆成不同 terminal node”的情况，所以 reducer 只在两个出口都是 terminal `return/unreachable` 时把额外 terminal 出口并入 loop，避免误吃普通多出口循环。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:402`
  新增 `LoopBreakSite`，记录 break 所在 graph node、break 出口 node 和条件是否取反。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:408`
  新增 `isTerminalGraphNode()`，只接受无 successor 且 terminator 是 `Return` 或 `Unreachable` 的 terminal node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:420`
  新增 `isLoopBreakExit()`，允许 break 目标是 loop follow，或是只被当前 body node 指向的 terminal clone。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:436`
  新增 `buildBreakIfNode()`，生成 semantic `If` + `Break`，不生成 goto。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:449`
  新增 `buildLoopBodyWithBreak()`，把 break-if 插回线性 body 中。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:477`
  新增 `collectLinearLoopBodyWithBreak()`，只接受线性 loop body 和最多一条 break edge。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:624`
  新增 `reduceLinearWhileWithBreakOnce()`，生成 `While` 节点并 collapse header、body 和可安全并入的 terminal break 出口。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:997`
  reducer 顺序里在普通 linear while 之后调用 break fast path。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-out.c --algo=structured-sailr`
  通过，输出 1 个 `while`、1 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-phoenix.c --algo=structured-phoenix`
  通过，输出 1 个 `while`、1 个 `break`、0 个 `goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-out.c --algo=structured-sailr`
  通过，输出 1 个 `while`，没有新增 `break/goto`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return-out.c --algo=structured-sailr`
  通过，输出 0 个 `goto/label`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-out.c --algo=structured-sailr`
  通过，输出 1 个 `switch`、3 个 `break`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-linear-break-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`，输出仍没有行首 `goto`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-linear-break-smoke.sol --tr-level=2`
  通过，耗时约 `17.50s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这个 reducer 是 Phoenix/SAILR 共享的，所以 `structured-phoenix` 和 `structured-sailr` 都受益。
- 仍未覆盖非线性 loop body、多条 break、continue、真实多出口 loop。这些后续应该放到更通用的 natural loop reducer 或 edge kind 分类里，不继续堆很多特例。

评分：

- 实现效果：6/10。覆盖一个常见早退出循环，明显减少 goto。
- 复杂度：5/10。新增代码偏多，但边界收得比较窄，没有改公共接口。
- 维护成本：5/10。后续做通用 loop reducer 时可以替换这段；在那之前它是一个安全 fast path。

# 2026-06-19 实现记录：RegionIdentifier 识别 natural loop child region

这轮先补 Angr 方向需要的 region 信息，但暂时不改变输出：`RegionIdentifier` 不再只返回 whole-function root，还会用 dominator + backedge 识别 natural loop child region，并挂到 root 的 `Children`。`RecursiveStructurer` 还没有消费这些 child region；后续要补 region overlay / collapse 边界后，再让 Phoenix/SAILR 按 child region 递归结构化。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:35`
  `RegionTree` 新增可写 `getRegion()`，用于 `RegionIdentifier` 给 root 追加 child。
- `external/NotDec-llvm2c/lib/Structuring/Region.cpp:23`
  实现可写 `RegionTree::getRegion()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:24`
  新增 `buildPredecessors()`，从 `StructuredCFG` 构造 predecessor map。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:36`
  新增 `computeDominators()`，用于判断 backedge。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:80`
  新增 `collectNaturalLoop()`，从 latch 反向收集 natural loop block。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:114`
  新增 `collectSuccessors()`，记录 loop region 的外部 successor。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:131`
  新增 `identifyNaturalLoopChildren()`，把每个去重后的 natural loop region 加到 root children。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:192`
  `identifyRoot()` 创建 root 后调用 natural loop child region 识别。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-region.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-region.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-region.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-region-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-region-smoke.sol --tr-level=2`
  通过，耗时约 `17.53s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步是基础设施，不改变 C/Solidity 输出。
- 下一步需要定清楚 region overlay 的 collapse 方式：父 region 里 child blocks 是整体替换成 child structured root，还是保留 root graph 只给算法提供辅助信息。这个边界没定之前，不应该让 RecursiveStructurer 直接递归消费 child region。

评分：

- 实现效果：5/10。RegionTree 开始有 natural loop 信息，但还没进入实际递归结构化。
- 复杂度：4/10。只加了本地 dominator/backedge 识别，没有改 renderer。
- 维护成本：5/10。当前 dominator 逻辑和 `MutableRegionGraph` 有重复；后续可以抽公共 helper，但现在先避免大范围抽象。

# 2026-06-19 实现记录：natural loop region 记录 latch/follow

上轮尝试让 `RecursiveStructurer` 直接消费 child region 时发现一个边界问题：natural loop child region 如果不带 follow/exit 信息，单独结构化时看不到 loop 出口，容易把正常 `while` 误处理成无限循环，或者漏掉 loop 后面的 `return/follow`。这轮先补元信息，不改变输出。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:22`
  `Region` 新增 `Latch`，记录 natural loop 的回边来源。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:23`
  `Region` 新增 `Follow`，只在 loop 外 successor 唯一时填写；多出口 loop 保持 `InvalidBlockId`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:163`
  先收集 natural loop 的外部 successors。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:166`
  natural loop region 填充 `Latch`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:167`
  单出口 natural loop region 填充 `Follow`。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-region-meta.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-region-meta.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-region-meta.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`、1 个 `return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-region-meta-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-region-meta-smoke.sol --tr-level=2`
  通过，耗时约 `17.60s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步解决了后续递归消费 loop region 前必须有的边界信息。
- `RecursiveStructurer` 仍未消费 child region；下一步应基于 `Head/Latch/Follow/Successors` 设计 overlay，而不是把 child region 当完全封闭图直接结构化。

评分：

- 实现效果：4/10。只是元信息，但补上了上次卡住的关键缺口。
- 复杂度：2/10。字段和赋值很小。
- 维护成本：3/10。字段语义直接，后续 overlay 会用到。

# 2026-06-19 实现记录：MutableRegionGraph 保留 region 外 successor

继续补 loop child region 后续递归消费前需要的边界信息。`MutableRegionGraph::build()` 之前只保留 region 内边，region 外 successor 被丢掉；如果后续用它结构化 natural loop child，就没法知道 body 内哪些边跳到 loop follow。现在每个 `MutableRegionNode` 记录 `ExternalSuccs`，collapse 时也保留这部分信息。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:44`
  `MutableRegionNode` 新增 `ExternalSuccs`，记录当前 region 图外部 successor 的 block id。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:29`
  新增 `appendUniqueBlock()`，避免重复记录同一个外部 successor。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:207`
  `MutableRegionGraph::build()` 遇到不在当前 region 的 successor 时写入 `ExternalSuccs`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:323`
  `collapseNodes()` 聚合 member node 的 `ExternalSuccs`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:345`
  被 collapse 的旧节点清空 `ExternalSuccs`。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-external-succ.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-external-succ.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-external-succ.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`、1 个 `return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-external-succ-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-external-succ-smoke.sol --tr-level=2`
  通过，耗时约 `17.84s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步不改变当前 Phoenix/SAILR 输出，只补 region graph 边界信息。
- 后续如果让 child region 单独结构化，可以用 `ExternalSuccs` 判断跳到 `Follow` 的 break/exit 边，而不是把 follow block 强行纳入 child region。

评分：

- 实现效果：4/10。仍是基础设施，但补上了 child region overlay 需要的外部边信息。
- 复杂度：3/10。字段和维护逻辑较小。
- 维护成本：3/10。信息直接挂在 graph node 上，后续 reducer 可以按需读取。

# 2026-06-19 实现记录：region kind 和外部 successor 查询

继续把 child region overlay 前置条件补清楚。现在 region tree 里已经有 root 和 natural loop child，但类型没有显式区分；同时 `ExternalSuccs` 只有字段，没有统一查询入口。后续 Phoenix/SAILR 需要判断“这个 child 是 natural loop 吗”“这个 node 是否跳到 loop follow”，所以先补小接口。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:15`
  新增 `RegionKind`，当前有 `Root` 和 `NaturalLoop`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Region.h:26`
  `Region` 新增 `Kind`，默认是 `Root`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:165`
  natural loop child region 标记为 `RegionKind::NaturalLoop`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:46`
  `MutableRegionNode` 新增 `hasExternalSuccessor()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:175`
  实现 `hasExternalSuccessor()`，用于后续 reducer 判断 region 外跳转目标。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-region-kind.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-region-kind.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-region-kind.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`、1 个 `return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-region-kind-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-region-kind-smoke.sol --tr-level=2`
  通过，耗时约 `17.73s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步仍不改变输出。
- 后续可以只对 `RegionKind::NaturalLoop` 做 overlay，并用 `hasExternalSuccessor(R.Follow)` 判断 break/exit，而不是猜测所有 child region 都是 loop。

评分：

- 实现效果：4/10。补的是后续递归 structuring 的类型和查询入口。
- 复杂度：2/10。只是 enum 和只读 helper。
- 维护成本：2/10。字段语义简单，使用点集中。

# 2026-06-19 实现记录：MutableRegionGraph child overlay build API

继续补 Angr-style recursive structuring 需要的 overlay 构图能力。之前 `MutableRegionGraph::build(Cfg, R)` 只能把 region 里的每个 block 建成一个节点；后续如果先结构化 child region，需要 parent region 的 graph 能把 child blocks 合成一个已有 `StructuredRoot` 的节点。这轮新增 overload，但现有 Phoenix 仍走旧入口，所以当前输出不变。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:63`
  新增 `MutableRegionGraph::build(Cfg, Regions, R, StructuredChildren)`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:32`
  新增 `containsBlock()`，用于判断 child block 是否属于 parent region。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:231`
  实现 child overlay build：有 structured root 的 child region 会成为一个 graph node，并用 child blocks 填充 block-to-node 映射。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:257`
  parent region 中不属于 child overlay 的 block 仍建普通节点。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:263`
  构边时，同一 child overlay 内部的边不再加成自边；跨 overlay 的边照常加；跳出 parent region 的边保留到 `ExternalSuccs`。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-overlay-build.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-overlay-build.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-overlay-build.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`、1 个 `return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-overlay-build-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-overlay-build-smoke.sol --tr-level=2`
  通过，耗时约 `17.77s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步提供了 parent region overlay 构图入口，但尚未让 `RecursiveStructurer` 使用它。
- 下一步可以让 `RecursiveStructurer` 先结构化 child，再让 Phoenix parent 通过 overlay build 消费 child root；需要先保证 natural loop child 的 `Follow/ExternalSuccs` 在 reducer 中被正确使用。

评分：

- 实现效果：5/10。真正补上了 child root 合入 parent graph 的构图能力。
- 复杂度：4/10。新增一个 overload，旧入口不变。
- 维护成本：4/10。构图逻辑和旧 build 有重复；后续稳定后可抽公共构边 helper。

# 2026-06-19 实现记录：MutableRegionGraph immediate dom/postdom 查询

继续补 SAILR/Angr-style structuring 后续会用到的图查询能力。之前 `MutableRegionGraphAnalysis` 只有 `dominates()` 和 `postDominates()` 布尔查询，无法直接拿 immediate dominator/postdominator。SAILR 的 edge heuristic 和后续 region overlay 都需要更精确的支配关系入口，所以补两个查询函数。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:58`
  新增 `immediateDominator()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:59`
  新增 `immediatePostDominator()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:182`
  新增 `findImmediateDominatorInSet()`，从 dominator/postdominator 集合里选最深的严格支配节点。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:211`
  实现 `immediateDominator()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:216`
  实现 `immediatePostDominator()`。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break-idom.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body-idom.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，loop 后 `return 0` 仍保留。
- `./build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch-idom.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`、1 个 `return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-idom-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-idom-smoke.sol --tr-level=2`
  通过，耗时约 `17.71s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步不改变 reducer 行为，只补查询 API。
- 后续 SAILR edge ordering 可以直接用 immediate postdominator，而不是只统计 postdominator 集合大小。

评分：

- 实现效果：5/10。补齐了图分析 API，直接贴近 SAILR 复现需求。
- 复杂度：3/10。查询基于已有集合，没有改分析数据结构。
- 维护成本：3/10。逻辑集中在 analysis 层。

# 2026-06-19 实现记录：MutableRegionGraph 复用 LLVM generic dominator tree

前一版 `MutableRegionGraph` 自己用集合迭代计算 dominator/postdominator。这个逻辑能跑，但容易在多出口、不可达块、后续 edge virtualization 后出细错。按当前 goal 的前置调整，改成复用 LLVM 的 `llvm::DominatorTreeBase<NodeT, IsPostDom>`，对外仍保留 `MutableRegionGraphAnalysis` 的 ID-based API。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:54`
  `MutableRegionGraphAnalysis` 新增 `ImmediateDominators`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:55`
  `MutableRegionGraphAnalysis` 新增 `ImmediatePostDominators`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:19`
  新增 `detail::DomOverlayNode`，作为 LLVM generic dom tree 使用的指针节点。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:36`
  新增 `detail::DomOverlayGraph`，保存 overlay 节点、entry、exit 和 synthetic root。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:52`
  为 `DomOverlayNode *` 补 `llvm::GraphTraits`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:62`
  为 `Inverse<DomOverlayNode *>` 补反向边 `GraphTraits`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:73`
  为 `DomOverlayGraph *` 补节点枚举和 entry 查询。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:98`
  为 `Inverse<DomOverlayGraph *>` 补 postdom 需要的反向图入口。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:206`
  新增 `buildDomOverlay()`，把 active `GraphNodeId` 图转成 LLVM 可分析的 pointer graph，并用 synthetic root 连接无前驱 root。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:288`
  新增 `fillDomAnalysis()`，通过 `DominatorTreeBase` 计算 dom/postdom、immediate dom/postdom，并回填到 `MutableRegionGraphAnalysis`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:337`
  `immediateDominator()` 改为读取 LLVM 结果回填的 `ImmediateDominators`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:343`
  `immediatePostDominator()` 改为读取 LLVM 结果回填的 `ImmediatePostDominators`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:626`
  `MutableRegionGraph::analyze()` 改为先建 overlay，再调用 LLVM-backed analysis。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、1 个 `break`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `switch`、3 个 `break`、1 个 `return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return.out.c --algo=structured-sailr`
  通过，输出仍是 `if/else return`，没有 `goto`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-smoke.out.sol --tr-level=2`
  通过，耗时约 `17.64s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步不改变 reducer 行为，只把 dom/postdom 的实现换成 LLVM 维护的 generic 算法。
- `MutableRegionGraphAnalysis` 的外部接口没有变，Phoenix/SAILR 调用侧不用动。
- postdom 多出口由 LLVM generic postdom 的 virtual root 处理，比旧版手写 `firstWithoutActiveSucc()` 更可靠；`Result.Exit` 仍保留给现有排序和调试语义。

评分：

- 实现效果：7/10。去掉了手写支配关系算法，后续 SAILR/Phoenix 复现可以站在 LLVM 算法上。
- 复杂度：5/10。多了一层本地 overlay 和 `GraphTraits`，但没有把 LLVM 类型泄漏给外部。
- 维护成本：4/10。适配代码集中在一个文件；后续如果 `MutableRegionGraph` 直接变成 pointer graph，可以再删掉 overlay。

# 2026-06-19 实现记录：SAILR H2 改为数 immediate postdom 边

对照 `/sn640/angr/angr/analyses/decompiler/structuring/sailr.py` 后，确认 Angr 的 H2 不是数所有 transitive postdominator pair，而是数 postdom graph 的边。现在 `MutableRegionGraphAnalysis` 已经有 LLVM-backed immediate postdominator，所以把 H2 从集合大小统计改成 immediate postdom 边数量，更贴近 Angr。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:24`
  `postDominatorPairCountAfterRemoving()` 改名为 `postDominatorEdgeCountAfterRemoving()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:31`
  H2 统计改为遍历 `Analysis.ImmediatePostDominators`，只计数非 `InvalidGraphNodeId` 的 immediate postdom 边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:81`
  `filterByMostPostDominators()` 改用新的 immediate-edge 计数函数。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、1 个 `break`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.out.c --algo=structured-sailr`
  通过，输出仍有 1 个 `switch`、3 个 `break`、1 个 `return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return.out.c --algo=structured-sailr`
  通过，输出仍是 `if/else return`，没有 `goto`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-smoke.out.sol --tr-level=2`
  通过，耗时约 `17.69s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步会影响 SAILR tie-break，但只是在 H2 里把统计口径改成 Angr 的 postdom graph edge 口径。
- Phoenix 和 Solidity 默认 goto 输出不受影响。

评分：

- 实现效果：6/10。SAILR H2 更接近 Angr，但还没有补 SAILR 的 deoptimization pass。
- 复杂度：2/10。只改一个计数函数。
- 维护成本：2/10。直接使用已有 `ImmediatePostDominators`。

# 2026-06-20 实现记录：递归 structurer 接入 child region overlay

这轮开始让 `RecursiveStructurer` 真正递归处理 child region。这里没有无条件把 child root 喂给 parent：natural loop child 当前不包含 follow block，child 自己可能退化成 goto 序列；如果直接 overlay 到 parent，会丢掉 parent 侧继续识别 loop/follow 的机会。所以这轮只把“已经形成结构化控制节点”的 child root 传给 parent。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionStructurer.h:15`
  新增 `supportsChildRegions()`，默认返回 `false`，避免 `GotoStructurer` 这类旧实现被 child overlay 改变输出。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/RegionStructurer.h:18`
  新增带 `RegionTree` 和 `StructuredChildren` 的 `structureRegion()` overload，默认回退到旧接口。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:19`
  `PhoenixStructurer` 显式 opt-in child region。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:22`
  `PhoenixStructurer` 实现新的 child-aware `structureRegion()` overload。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:8`
  新增 `containsStructuredControl()`，只认可包含 `Switch`、`While`、`DoWhile`、`InfiniteLoop` 的 child root。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:36`
  新增 `structureRegionRecursive()`，递归结构化 child region，并缓存已结构化 region。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:45`
  只在算法 opt-in 时递归 child；只有结构化成功的 child root 才写入 `StructuredChildren`。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:77`
  root region 改走递归 driver。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:988`
  旧 `structureRegion(Cfg, R, Tree)` 保留，转调新 overload。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1000`
  Phoenix 在有 `StructuredChildren` 时使用 `MutableRegionGraph::build(Cfg, Regions, R, StructuredChildren)`，否则继续旧构图。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.recursive.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、1 个 `break`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.recursive.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.recursive.c --algo=structured-sailr`
  通过，输出仍有 1 个 `switch`、3 个 `break`、1 个 `return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return.recursive.c --algo=structured-sailr`
  通过，输出仍是 `if/else return`，没有 `goto`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-recursive-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-recursive-smoke.sol --tr-level=2`
  通过，耗时约 `17.56s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步接上了 recursive structuring 的数据流，但暂时不强行消费未结构化好的 child region。
- natural loop child 要真正带来收益，后续需要让 child structuring 拿到 follow/exit 上下文，或者给 natural-loop region 单独的 loop-aware 入口。
- 当前改动保持 `GotoStructurer` 输出不变，Phoenix/SAILR 才 opt-in child overlay。

评分：

- 实现效果：6/10。递归框架开始实际传递 child root，但保守过滤导致当前 smoke 输出基本不变。
- 复杂度：4/10。新增一个 overload 和递归 driver，没有改 reducer 主体。
- 维护成本：4/10。child root 过滤规则集中在 `RecursiveStructurer`，后续补 loop context 时需要回到这里调整。

# 2026-06-20 实现记录：Phoenix virtual edge fallback 识别 continue

shared Phoenix 的 `VirtualEdgeKind` 已经有 `Goto`、`Break`、`Continue`，renderer 也能输出 `break/continue`，但 fallback 收集边时一直全部标成 `Goto`。这轮先补一个保守分类：只有当前 region 明确是 `NaturalLoop`，且 virtualized edge 目标是这个 loop 的 `Head`，才把边标成 `Continue`。`Break` 先不做，因为 natural-loop child graph 里跳到 follow 的边目前存在于 `ExternalSuccs`，不是普通 `Succs`；在 parent overlay 里 child->follow 又是正常后继，不能直接猜成 break。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:35`
  `virtualizeOneEdge()` 增加 `Region` 参数，用于判断当前 virtual edge fallback 的 region 类型和 loop head。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:737`
  `collectVirtualizableEdges()` 增加 `Region` 参数。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:752`
  只有 `RegionKind::NaturalLoop && ToBlock == R.Head` 时，把 virtual edge kind 设置为 `Continue`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:972`
  `virtualizeOneEdge()` 调用新的 `collectVirtualizableEdges(R, Graph)`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1030`
  reducer fallback 调用 `virtualizeOneEdge(Cfg, R, Graph)`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.continue.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、1 个 `break`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.continue.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.continue.c --algo=structured-sailr`
  通过，输出仍有 1 个 `switch`、3 个 `break`、1 个 `return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-if-two-return.ll -o /tmp/notdec-if-two-return.continue.c --algo=structured-sailr`
  通过，输出仍是 `if/else return`，没有 `goto`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-continue-fallback.ll -o /tmp/notdec-loop-continue-fallback.c --algo=structured-sailr`
  通过；该样例被现有 while/if reducer 完整结构化，没有触发 fallback continue，但证明普通 loop 输出没有回退。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-continue-kind-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-continue-kind-smoke.sol --tr-level=2`
  通过，耗时约 `17.76s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步补上了 continue 分类能力，但现有 smoke 没有触发 fallback continue。
- break 分类需要先把 loop follow/exit 上下文传进 child graph，不能靠 parent overlay 猜。

评分：

- 实现效果：4/10。补齐了一个安全的 virtual edge kind，但还不是完整 irregular loop exit 迁移。
- 复杂度：2/10。只多传 `Region` 并改一个分类条件。
- 维护成本：2/10。逻辑局部，后续补 break 时可以沿用同一个 region-aware 收集入口。

# 2026-06-20 调整记录：break fallback 暂停点

继续迁移 Phoenix irregular loop exit 时，尝试过把 natural-loop child graph 的 `ExternalSuccs` 到 `Follow` 作为 `Break` virtual edge，再把 natural-loop child root 包成 `While`。临时样例 `/tmp/notdec-loop-external-break-fallback.ll` 暴露了一个更早的边界问题：当前 `RegionIdentifier` 会为多 latch / 重叠 natural loop 生成重叠 child region，parent overlay 还没有处理“重叠 child region 该选哪个、如何合并或嵌套”的策略。

因此这轮没有提交 break fallback 代码，只保留已验证的 stable 状态。直接把外部 follow 边标成 `Break` 不够，因为：

- natural-loop child graph 里跳到 follow 的边在 `ExternalSuccs`，不是普通 `Succs`。
- parent overlay 里的 child->follow 是正常后继，不能猜成 `Break`。
- child root 只有被 parent overlay 消费后，`break/continue` 才有语义容器；重叠 loop child 会让这个消费关系不明确。

当前判断：

- 下一步不能继续只补 `VirtualEdgeKind::Break`。
- 需要先定 region 边界策略：至少要过滤或合并重叠 natural-loop child，保证 parent overlay 只消费不重叠、可嵌套的 child region。
- 这是计划里没有细化的技术决策点，goal 保持 active，不标记 complete。

验证：

- 未提交实验代码已撤回，`external/NotDec-llvm2c` 工作树干净。
- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。

# 2026-06-20 实现记录：natural loop region 改为 laminar child tree

上一轮 break fallback 的停点是重叠 natural loop child：同一个函数里可能因为多个 backedge / 多个 header 生成部分重叠的 child region，parent overlay 没法安全决定消费哪个 child。这里先把 `RegionIdentifier` 改成更保守的 laminar region tree：同 header backedge 合并成一个 loop；不同 header 的 loop 如果只是部分重叠，就不生成后来的候选；只有完全包含关系才转成嵌套 child。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:16`
  新增 vector 版 `contains()`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:20`
  新增 `isSubset()`，用于判断 region 包含关系。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:29`
  新增 `overlaps()`，用于过滤非 laminar 的部分重叠 loop。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:153`
  新增 `LoopCandidate`，把 natural loop 候选先收集到临时结构。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:168`
  新增 `mergeLoopCandidate()`，同 header 多 backedge 合并成一个 loop 候选。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:191`
  新增 `collectLoopCandidates()`，集中收集 backedge 产生的 natural loop。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:217`
  新增 `filterLaminarLoops()`，过滤部分重叠 loop，只保留不重叠或包含关系。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:248`
  新增 `makeLoopRegion()`，从候选生成 `RegionKind::NaturalLoop`。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:281`
  `identifyNaturalLoopChildren()` 改为先生成候选，再建立 loop region。
- `external/NotDec-llvm2c/lib/Structuring/RegionIdentifier.cpp:294`
  给每个 loop child 选择最小的包含 parent，形成 root/child 嵌套关系。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.laminar.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、1 个 `break`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.laminar.c --algo=structured-sailr`
  通过，输出仍有 1 个 `while`、loop 后 `return 0`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.laminar.c --algo=structured-sailr`
  通过，输出仍有 1 个 `switch`、3 个 `break`、1 个 `return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-external-break-fallback.ll -o /tmp/notdec-loop-external-break-fallback.laminar.c --algo=structured-sailr`
  通过；该多 backedge 样例输出仍未改善，但 region tree 已不再给 parent overlay 暴露任意重叠 child。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-laminar-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-laminar-smoke.sol --tr-level=2`
  通过，耗时约 `17.51s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步解决的是 recursive parent overlay 的前置边界：child region 不能部分重叠。
- break fallback 还没继续接回去；下一步可以在 laminar region tree 基础上重新实现 natural-loop child 的 follow/break 处理。

评分：

- 实现效果：6/10。减少了 region tree 的不确定性，是继续迁移 Phoenix irregular loop exit 的前置。
- 复杂度：5/10。`RegionIdentifier` 多了候选阶段、过滤阶段和 parent 选择阶段。
- 维护成本：5/10。策略保守，后续如果要支持 irreducible/overlap loop，需要在 `filterLaminarLoops()` 里扩展。

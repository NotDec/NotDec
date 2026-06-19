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

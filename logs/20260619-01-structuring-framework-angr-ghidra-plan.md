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

# 2026-06-20 调整记录：natural-loop wrapper 仍需定边界

在 laminar region tree 之后，重新尝试把 natural-loop child 的 `ExternalSuccs` 到 `Follow` 落成 `Break`，并在 child region 外包一层 `While`。这次没有遇到重叠 child 的问题，但 `/tmp/notdec-while-linear-body.ll` 出现回退：原本完整的 `while` 输出里多出了 `goto head`。

原因是现有 root-level Phoenix reducer 已经能在整图上识别简单 while；如果 child region 也提前包 loop，会和 root-level reducer 重复处理同一段 loop，造成 backedge 仍以 fallback 形式留在 body 中。也就是说，`break` fallback 不能只靠“child region 自己包 loop”解决，还需要先明确：

- 哪些 natural-loop child 应该由 child structurer 独立包 loop。
- 哪些 loop 应继续交给 parent/root-level reducer。
- parent overlay 消费 child loop 后，head/follow/backedge 这些边如何从 parent graph 中删除或保留。

当前处理：

- 未提交的 `virtualizeExternalEdge()`、external follow `Break`、natural-loop wrapper 代码都已撤回。
- `external/NotDec-llvm2c` 工作树保持干净。
- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。

当前判断：

- laminar loop region 是必要前置，但还不足以安全接 `Break` fallback。
- 下一步应先让 `RecursiveStructurer` 或 `PhoenixStructurer` 明确“child loop ownership”：简单 whole-loop 由 parent/root reducer 处理，只有 parent 无法规约或 child 被明确消费时，才启用 child loop wrapper。

# 2026-06-20 实现记录：root 不消费 natural-loop child

上一步的问题是 root-level Phoenix reducer 和 natural-loop child wrapper 会抢同一个 loop。这里先定一个保守边界：root region 不把 natural-loop child 的结构化结果传回 parent overlay。这样简单 whole-loop 仍由 root-level reducer 处理，避免 child 先包 loop 后留下额外 backedge。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:36`
  新增 `shouldPassChildToParent()`，root parent 遇到 `RegionKind::NaturalLoop` child 时返回 false。
- `external/NotDec-llvm2c/lib/Structuring/RecursiveStructurer.cpp:61`
  `StructuredChildren` 只接收允许传给 parent、且确实包含结构化控制节点的 child root。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.ownership.c --algo=structured-sailr`
  通过，输出有 `while`、`return`，没有额外 `goto head`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.ownership.c --algo=structured-sailr`
  通过，输出有 `while`、`break`、`return`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-external-break-fallback.ll -o /tmp/notdec-loop-external-break-fallback.ownership.c --algo=structured-sailr`
  通过，输出仍偏 goto-heavy，和本次修改前一致。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.ownership.c --algo=structured-sailr`
  通过，输出有 `switch`、3 个 `break`、`return`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-ownership-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-ownership-smoke.sol --tr-level=2`
  通过，耗时约 `17.57s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步只解决 ownership 冲突，不改善 irregular loop。
- 后续 break fallback 应只在会被非 root parent 消费的 child loop 上启用，或者等 root 无法规约时再兜底，不能无条件给所有 natural-loop child 包 `While`。

评分：

- 实现效果：5/10。避免了已确认的简单 while 回退。
- 复杂度：2/10。只加了一个过滤条件。
- 维护成本：3/10。规则保守，后续如果要让 root 消费某些 child loop，需要把 ownership 条件再细分。

# 2026-06-20 实现记录：root natural-loop fallback

上一步只定了 root 不消费 natural-loop child，irregular loop 仍会落到纯 goto。这里在 root-level reducer 里补一个保守兜底：普通 while/if/switch reducer 都没处理掉的 natural loop，会被折成 `InfiniteLoop`；loop 内回到 header 的边转成 `continue`。如果 natural loop 只有一个外部 successor，该外跳才转 `break`；多出口 loop 继续保留 `goto`，避免把不同出口错误合成同一个 break。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:16`
  新增 `makeControlTransfer()`，统一创建 `Goto` / `Break` / `Continue` 节点。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:789`
  新增 `classifyNaturalLoopExit()`，natural loop 回 header 转 `Continue`，单出口 follow 转 `Break`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:849`
  `appendFallbackNode()` 增加 region 参数，fallback 生成 `If` / `Switch` 时直接填 `Then` / `Else` / `StructuredCases`，不再同时填旧 `Children`，避免 C 渲染器走旧 goto 路径。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1030`
  新增 `reduceNaturalLoopFallbackOnce()`，root region 上把尚未结构化的 natural-loop child 折成 `InfiniteLoop`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1183`
  reducer 顺序里在 virtual edge fallback 前尝试 natural-loop fallback。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1215`
  最终 fallback 输出把当前 region 传给 `appendFallbackNode()`，使 child natural-loop fallback 也能复用同一套分类。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-while-linear-body.ll -o /tmp/notdec-while-linear-body.loop-fallback-final.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`，没有 `goto head`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-break.ll -o /tmp/notdec-loop-break.loop-fallback-final.c --algo=structured-sailr`
  通过，输出仍是 1 个 `while`、1 个 `break`，没有 `goto`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-simple-switch.ll -o /tmp/notdec-simple-switch.loop-fallback-final.c --algo=structured-sailr`
  通过，输出仍是 1 个 `switch`、3 个 `break`，没有 `goto`。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c /tmp/notdec-loop-external-break-fallback.ll -o /tmp/notdec-loop-external-break-fallback.loop-fallback6.c --algo=structured-sailr`
  通过，输出从纯 goto 变成 1 个 `while(1)`、2 个 `continue`、6 个 `goto`；因为该样例有两个外部出口，未把出口强行转成 `break`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-loop-fallback-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-loop-fallback-smoke.sol --tr-level=2`
  通过，耗时约 `17.79s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这一步让 Phoenix fallback 开始利用 natural-loop region 信息，不再只靠虚拟 goto 边。
- 多出口 loop 暂时只消掉回边；要把多出口也结构成 `break`，需要先引入带目标的 break 或更强的 follow 合并策略。

评分：

- 实现效果：6/10。改善 irregular loop 的可读性，但多出口仍保守保留 goto。
- 复杂度：5/10。fallback 多了一条 root-level natural-loop 规约路径。
- 维护成本：5/10。逻辑仍集中在 Phoenix fallback，后续迁 SAILR follow 选择时可以替换这一段。

# 2026-06-20 实现记录：structuring smoke 测试

前面几个 Phoenix/SAILR 行为一直靠 `/tmp` 小 IR 手动检查，后续继续迁 SAILR 时容易回退。这里把核心 smoke 固化到 `NotDec-llvm2c` 子模块的 CTest 里，先覆盖当前已支持的最小行为。

修改内容：

- `external/NotDec-llvm2c/test/CMakeLists.txt:2`
  增加 `find_package(Python3 REQUIRED COMPONENTS Interpreter)`。
- `external/NotDec-llvm2c/test/CMakeLists.txt:5`
  接入新的 `structuring` 测试目录。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:1`
  新增 `structuring-smoke` 测试，直接使用构建出的 `notdec-llvm2c-exe`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:9`
  新增 4 个小 IR：线性 while、线性 while+break、多出口 loop fallback、简单 switch。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:144`
  每个 case 跑 `--algo=structured-sailr`，按输出文本检查关键结构和计数。

验证：

- `python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c` 通过。
- `cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S . -B ./build -G Ninja` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `ctest --test-dir build -R structuring-smoke --output-on-failure` 通过，1 个测试，耗时约 `0.41s`。

当前判断：

- 这不是完整 correctness 测试，只是防止当前 Phoenix/SAILR 迁移过程中最基础的 loop/switch 行为回退。
- 多出口 loop 的断言保持保守：检查 `while(1)` 和两个 `continue`，不要求把外部出口转成 `break`。

评分：

- 实现效果：6/10。补上了最缺的回归保护。
- 复杂度：2/10。一个小脚本和一个 CTest 入口。
- 维护成本：3/10。后续每迁一个 reducer 可以继续往同一个脚本加 case。

# 2026-06-20 实现记录：SAILR 无入口图回退 Phoenix ordering

对照 Angr 的 `SAILRStructurer._order_virtualizable_edges()` 后发现一个边界差异：Angr 找不到 graph entry 时不会启用 H1/H2/H3，而是直接回 Phoenix 默认排序。当前实现无条件跑 SAILR heuristic。这里补齐这个边界，避免入口不明确的 graph 上用不可靠的 sibling/postdom 统计。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:124`
  如果 `MutableRegionGraphAnalysis::Entry` 是 `InvalidGraphNodeId`，直接调用 `PhoenixStructurer::orderVirtualizableEdges()`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R structuring-smoke --output-on-failure` 通过，1 个测试，耗时约 `0.42s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-sailr-entry-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.14s`。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-sailr-entry-smoke.sol --tr-level=2`
  通过，耗时约 `17.70s`，输出仍是 `471` 个 `// block_...`、`454` 个 `goto block_...`、`12` 个 `emit`、`160` 个 `revert`。

当前判断：

- 这是 SAILR ordering 的小边界修正，不改变 SAILR 主体能力。
- 下一步更大的缺口仍是 SAILR 的 deoptimization/pass 入口，以及更完整的 Phoenix schema 迁移。

# 2026-06-20 实现记录：Phoenix 预处理 hook

为了给后续 SAILR 的 deoptimization 留一个位置，这里给 `PhoenixStructurer` 加了一个空的 region preprocess hook，并把它接到 reducer 循环前。当前默认不做任何事，所以 Phoenix 输出不变；但后面如果要按算法名注入预处理，就不用再改主循环。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:29`
  新增 `preprocessRegionGraph()`，默认空实现。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1168`
  `structureRegion()` 在 reducer 循环前调用 `preprocessRegionGraph()`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R structuring-smoke --output-on-failure` 通过，1 个测试，耗时约 `0.42s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-prehook-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，耗时约 `0.13s`。

当前判断：

- 这还不是 SAILR deoptimization，只是把算法相关前处理的入口补出来。
- 后面如果要模拟 Angr 的 pass 过滤和去优化，应该优先覆盖这个 hook，而不是继续往 reducer 主循环里塞特例。

# 2026-06-20 实现记录：删除旧 C Phoenix 入口和编译单元

现在 `llvm2c` 的 structuring 入口已经按 Angr 的方向收口到共享 structuring 骨架，这里把旧版 C Phoenix 的独立入口和编译单元删掉，只保留 `goto` / `structured-goto` / `structured-sailr` 这条新路径。这样对外就不再有旧 `phoenix` / `structured-phoenix`，也不会再编译旧的 `notdec-llvm2c/Phoenix.cpp`。

修改内容：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:17`
  删除 `SA_Phoenix` 和 `SA_StructuredPhoenix`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:9`
  删除 `phoenix` / `structured-phoenix` CLI 选项。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2126`
  只保留 `goto` / `structured-goto` / `structured-sailr` 三条 structuring 路径。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/CMakeLists.txt:2`
  删除旧 `Phoenix.cpp` 编译单元。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Phoenix.h`
  删除旧 C Phoenix 头文件。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Phoenix.cpp`
  删除旧 C Phoenix 实现。

验证：

- `cmake --build ./build --target notdec-backend-c notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R structuring-smoke --output-on-failure` 通过，1 个测试，耗时约 `0.45s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-no-legacy-phoenix-smoke.c --tr-level=2 --algo=structured-sailr`
  通过，说明新共享 structuring 路径不受影响。

当前判断：

- 旧 C Phoenix 已经不再是对外算法入口，也不再单独编译。
- 后续如果要继续贴 Angr，可以直接围绕共享 `PhoenixStructurer` / `SAILRStructurer` 往前走，不必再保留旧 Phoenix 的双入口语义。

# 2026-06-20 实现记录：legacy phoenix 入口拒绝测试

旧 C Phoenix 的代码已经删掉了，这里再补一条负面测试，直接确认 `notdec-llvm2c` 不再接受 `--algo=phoenix`。这样删入口不是只看源码，而是有运行时验证。

修改内容：

- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:6`
  新增 `legacy-phoenix-removed` 这个 ctest。
- `external/NotDec-llvm2c/test/structuring/run_legacy_phoenix_removed.py:13`
  直接运行 `notdec-llvm2c --algo=phoenix ...`，要求命令失败且输出包含 `Cannot find option named 'phoenix'`。

验证：

- `python3 external/NotDec-llvm2c/test/structuring/run_legacy_phoenix_removed.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed' --output-on-failure` 通过，2 个测试都通过，耗时约 `0.51s`。

当前判断：

- 旧 Phoenix 入口已经从代码和测试两边同时去掉。
- 后面要做的是把共享 `PhoenixStructurer` 再收敛到更接近 Angr 的实现，而不是恢复老 Phoenix 行为。

# 2026-06-20 实现记录：恢复 shared structured-phoenix 入口

前一轮删除旧 C Phoenix 时，把 shared `structured-phoenix` 入口也一起删掉了。按新的目标，应该删除的是旧版 C Phoenix 代码和 `--algo=phoenix`，但保留 `--algo=structured-phoenix`，让它指向共享 `PhoenixStructurer`。这样后续可以继续按 Angr 的 `PhoenixStructurer` 方向改，而不是让用户只能通过 `structured-sailr` 间接测 Phoenix 基线。

修改内容：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:19`
  恢复 `SA_StructuredPhoenix`，但没有恢复旧 `SA_Phoenix`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:12`
  恢复 `structured-phoenix` CLI 选项；旧 `phoenix` 仍不存在。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2129`
  `SA_StructuredPhoenix` 通过 `StructuredGoto(*this, "phoenix")` 调到共享 structuring registry。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:11`
  新增 `structured-phoenix-available` ctest。
- `external/NotDec-llvm2c/test/structuring/run_structured_phoenix_available.py:13`
  运行 `notdec-llvm2c --algo=structured-phoenix`，确认 shared Phoenix 入口可用。

验证：

- `cmake --build ./build --target notdec notdec-llvm2c-exe -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `0.64s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structured-phoenix-shared-smoke.c --tr-level=2 --algo=structured-phoenix` 通过。
- `build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=phoenix /tmp/notdec-while-linear-body.ll -o /tmp/legacy-phoenix-check.c` 失败且输出 `Cannot find option named 'phoenix'`，符合预期。

当前判断：

- 对外入口现在清楚了：`phoenix` 是被删除的旧 C Phoenix；`structured-phoenix` 是共享 Angr-style Phoenix reducer。
- 下一步继续改 `PhoenixStructurer` 本身，使它更贴近 Angr 的 Phoenix 流程，然后在同一套继承关系上补 SAILR。

# 2026-06-20 实现记录：Phoenix 调度拆成 Angr 式阶段

这轮先不增加新 reducer，而是把 shared `PhoenixStructurer` 的主循环拆成更接近 Angr 的阶段：acyclic schema、cyclic schema、cyclic refinement、last-resort refinement。之前这些规则都直接塞在 `structureRegion()` 里，后续 SAILR 或其他 Angr 算法如果只想替换某一阶段，会被迫复制整个循环。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:33`
  新增可覆写的 `analyzeAcyclic()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:36`
  新增可覆写的 `analyzeCyclic()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:39`
  新增可覆写的 `refineCyclic()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:42`
  新增可覆写的 `lastResortRefinement()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1126`
  `analyzeAcyclic()` 串起 sequence / if / switch reducer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1138`
  `analyzeCyclic()` 串起 while / while-with-break / self-loop reducer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1150`
  `refineCyclic()` 暂时承载现有 natural-loop fallback。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1158`
  `lastResortRefinement()` 承载 virtual edge fallback。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1209`
  `structureRegion()` 改成按这些阶段调度，不再直接列出所有 reducer。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `0.65s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-phoenix-staged-smoke.c --tr-level=2 --algo=structured-phoenix` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-sailr-staged-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步让 shared Phoenix 的控制流程更像 Angr，但还没有补完整 Phoenix schema。
- SAILR 后续可以只覆写 `preprocessRegionGraph()`、`orderVirtualizableEdges()` 或某个阶段函数，不需要复制 Phoenix 主循环。
- 实现效果：6/10。阶段边界更清楚，但具体 reducer 能力还没增加。
- 复杂度：4/10。只多了几个虚函数，主循环反而更短。
- 维护成本：4/10。后续迁 Angr 规则时有明确落点，代价是调用层级多了一层。

# 2026-06-20 实现记录：SAILR 启用 improved Phoenix schema

对照 Angr 的 `SAILRStructurer.__init__()`，SAILR 会用 `improve_phoenix=True` 调 Phoenix，而普通 Phoenix 默认不开这些改进规则。当前 shared Phoenix 把 `while (...) { if (...) break; }` 这条 fast path 放在普通 Phoenix 里。这里把它收回到 SAILR/improved Phoenix 边界下，避免普通 `structured-phoenix` 和 `structured-sailr` 继续混成同一个行为。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:44`
  新增 `useImprovedCyclicSchemas()`，默认返回 `false`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRStructurer.h:13`
  `SAILRStructurer` 覆写 `useImprovedCyclicSchemas()`，返回 `true`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1144`
  `reduceLinearWhileWithBreakOnce()` 只在 improved schema 开启时运行。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:181`
  新增同一 IR 的 SAILR/Phoenix 对比：SAILR 应输出 `break;`，Phoenix 不应使用这条 improved break schema。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `0.86s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-sailr-improve-smoke.c --tr-level=2 --algo=structured-sailr` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-phoenix-no-improve-smoke.c --tr-level=2 --algo=structured-phoenix` 通过。

当前判断：

- 这一步不是 SAILR deoptimization，只是把已有 improve-only schema 放到 Angr 一样的开关后面。
- 后续如果继续搬 Angr 里 `if self._improve_algorithm` 的规则，可以落到同一个 hook 下。
- 实现效果：6/10。SAILR 和 Phoenix 的行为边界更接近 Angr。
- 复杂度：3/10。只加一个布尔 hook。
- 维护成本：3/10。后续 improved Phoenix 规则有明确入口。

# 2026-06-20 实现记录：shared structuring 默认算法改为 SAILR

Angr 的 `DEFAULT_STRUCTURER` 是 `SAILRStructurer`，而 shared structuring registry 里默认还停在 `goto`。C backend 的 CLI 默认已经是 `structured-sailr`，但 Solidity backend 通过 `DefaultStructurerName` 取默认值，所以仍会落到最弱的 goto。这里把 registry 默认改成 `sailr`，让跨语言 backend 的默认行为和 Angr/C backend 保持一致。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructurerRegistry.h:11`
  `DefaultStructurerName` 从 `"goto"` 改成 `"sailr"`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-backend-solidity notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `0.84s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-default-sailr-registry-smoke.c --tr-level=2 --algo=structured-sailr` 通过。
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-solidity-default-sailr-smoke.sol --tr-level=2` 通过，耗时约 `19.10s`。

当前判断：

- 这一步不改变 C CLI 的默认值，因为它已经是 `structured-sailr`。
- Solidity backend 以后默认走 SAILR，而不是 Goto fallback，更符合“共享 structuring 给多语言后端复用”的目标。
- EVM smoke 时间和之前同口径约 18s 接近，未见明显性能风险。

# 2026-06-20 实现记录：Phoenix early terminal if schema

继续补 Angr `_match_acyclic_ite()` 里的一个保守形状：条件分支的一边是 terminal return/unreachable，另一边继续执行时，可以折成 `if (...) return;` 后接 follow。当前 shared Phoenix 之前只支持 then-fallthrough、if-else common follow、两边都 terminal，漏了 early return 这种常见形状。

实现时遇到一个边界：这个 acyclic schema 不能抢循环里的 `if (...) break`。一开始直接匹配 terminal+follow 会把 `while` header 或 loop body 里的 break 分支先折成普通 if，导致后面的 cyclic while / improved break schema 失效。最终加了保守保护：继续分支不能直接回 header，也不能下一跳回 header。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:198`
  `reduceIfOnce()` 增加 true 分支 terminal、false 分支继续的 early terminal if schema。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:204`
  增加 false 分支 terminal、true 分支继续的对称 schema，并取反条件。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:142`
  新增 `early_return_if` 小 IR，要求输出 `if (x == 0)` 和两个 `return`，且不能退回 `goto ret` / `goto cont`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `0.92s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-early-terminal-phoenix-smoke.c --tr-level=2 --algo=structured-phoenix` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-early-terminal-sailr-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这个 schema 是 Angr ITE 的一个子集，不处理外部 region successor 和复杂 switch head。
- 回边保护是必要的，否则 acyclic if 会抢 cyclic loop schema。
- 实现效果：6/10。补了常见 early return，但还没有迁 short-circuit condition。
- 复杂度：4/10。规则本身小，主要复杂度在避免抢 loop。
- 维护成本：4/10。后续如果引入 Angr 一样的 full graph / region overlay，可以把这个保护换成更准确的 cyclic 判断。

# 2026-06-20 实现记录：Phoenix 线性 do-while schema

继续补 Angr `_match_cyclic_dowhile()` 的一个窄形状：线性 body 的最后一个 latch 条件分支一边回到 body entry，另一边到 follow。此前 shared Phoenix 只支持 self-loop do-while，多块 body 会落到 fallback 或更弱的结构。

这次不处理多入口、多 latch、body 内 break/continue，也不做 Angr 的 multi-statement expression 优化。候选选择上显式尝试两个 successor，只有能从 successor 线性走到 latch 的才作为 do-while body entry。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:714`
  新增 `collectLinearDoWhileBody()`，从候选 entry 沿单后继 fallthrough 线性走到 latch。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:748`
  新增 `reduceLinearDoWhileOnce()`，生成 `StructuredNodeKind::DoWhile` 并 collapse body 节点。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1266`
  `analyzeCyclic()` 在 while / improved while-break 后、self-loop 前尝试线性 do-while。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:163`
  新增 `linear_do_while` 小 IR，要求输出 `do { ... } while (x == 0);`，不能退回 `goto body1` 或 `while (1)`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `1.04s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-linear-dowhile-phoenix-smoke.c --tr-level=2 --algo=structured-phoenix` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-linear-dowhile-sailr-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这只是 do-while 的线性子集，还不是完整 Angr cyclic do-while。
- 规则没有引入新条件表达式，只复用 latch 原条件，所以适合 shared structuring 当前的 opaque payload 模型。
- 实现效果：6/10。补了一个常见 loop schema。
- 复杂度：5/10。新增 helper 较长，但逻辑局限在线性 body。
- 维护成本：4/10。后续完整 do-while 可以替换这个 helper，测试保留为基础回归。

# 2026-06-20 实现记录：SAILR postdom heuristic 参数收口

对照 Angr `SAILRStructurer.__init__()`，postdom heuristic 有 `postdom_max_edges=10` 和 `postdom_max_graph_size=50` 两个参数。当前实现把这两个值写死在 helper 里。这里把它们收进 `SAILRStructurer` 对象，默认值保持 10/50，方便后续接 backend option 或继续复现 Angr heuristic。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRStructurer.h:13`
  新增构造函数参数 `PostDomMaxEdges` / `PostDomMaxGraphSize`，默认 `10` / `50`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRStructurer.h:29`
  新增成员保存这两个限制。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:70`
  `filterByMostPostDominators()` 改为接收限制参数，不再写死 `10` / `50`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRStructurer.cpp:132`
  `orderVirtualizableEdges()` 使用对象上的限制参数。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available' --output-on-failure` 通过，3 个测试，耗时约 `1.01s`。

当前判断：

- 行为不变，只把 Angr SAILR 的参数边界放到算法对象上。
- 后续如果要暴露 SAILR 参数，不需要再改 helper 内部。

# 2026-06-20 实现记录：structurer registry 改成表驱动

Angr 的结构恢复入口是按 structurer class 表选择算法。当前 shared structuring registry 还是手写 `if` 链，后续继续接 Phoenix / SAILR / 其他 Angr 算法时不够直观。这里把 registry 改成小表，行为保持不变。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructurerRegistry.h:6`
  引入 `llvm::ArrayRef`，避免使用项目当前 C++17 不支持的 `std::span`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructurerRegistry.h:15`
  新增 `registeredStructurerNames()`，后续 CLI/help/options/test 可以复用同一份算法名列表。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:24`
  新增 `StructurerRegistration`，把算法名和 factory 放在同一张表里。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:29`
  当前注册 `goto`、`phoenix`、`sailr`，和原有可用算法一致。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:43`
  `registeredStructurerNames()` 返回算法名列表。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:47`
  `createStructurer()` 改为遍历注册表，不再维护手写 `if` 链。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:16`
  新增 `shared-structurer-registry` 回归测试。
- `external/NotDec-llvm2c/test/structuring/run_shared_structurer_registry.py:24`
  新增一个小 IR，分别跑 `structured-goto`、`structured-phoenix`、`structured-sailr`，确认三个注册名都能生成输出。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，4 个测试，耗时约 `1.30s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-registry-table-sailr-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步只是入口整理，不是新算法实现。
- registry 形式更接近 Angr 的 `STRUCTURER_CLASSES`，后续加算法只需要补注册项和测试。
- C++ 标准仍是 C++17，所以没有使用 `std::span`。

# 2026-06-20 实现记录：Phoenix acyclic switch 顺序和 terminal case

对照 Angr `PhoenixStructurer._match_acyclic_schemas()`，acyclic schema 的尝试顺序是 switch、sequence、ITE。当前 shared Phoenix 是 sequence、if、switch。这个顺序会让 case 块先被 sequence 合并，后面 switch reducer 就失去原始 case body。这里把顺序改成和 Angr 一致，并补一个保守的 terminal switch case 形状。

实现时发现一个现有缺口：当所有 case/default 都直接 return 时，`reduceSwitchOnce()` 找不到共同 follow，于是退回 fallback switch，输出 `case: goto case1;` 再在 switch 外放 label。这个形状可以安全折叠，因为每个 target 都是 header 的唯一后继分支目标，而且没有后继边。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:328`
  `reduceSwitchOnce()` 增加 `HasTerminalCasesOnly` 分支，允许所有 switch target 都是 terminal node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:350`
  成员检查根据是否 terminal case 分流：普通 switch 仍要求 case 指向共同 follow；terminal switch 要求 case 没有后继。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1259`
  `analyzeAcyclic()` 的 schema 顺序改为 switch、sequence、if，和 Angr 一致。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:142`
  新增 `switch_before_sequence`，要求 terminal case switch 直接生成 case body，不能退成 `goto case1` / `goto case2` / `goto default`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，4 个测试，耗时约 `1.38s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-switch-terminal-sailr-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这不是完整 Angr switch 迁移，只补了最容易证明正确的 terminal case 子集。
- schema 顺序已经和 Angr 对齐，后续继续补 switch schema 时不会被 sequence 先抢掉。
- 实现效果：5/10。修掉一个真实 fallback 退化，但 switch 复杂形状还没迁。
- 复杂度：3/10。只在现有 reducer 内加一个分支。
- 维护成本：3/10。规则边界清楚，后续完整 switch reducer 可以覆盖它。

# 2026-06-20 实现记录：Phoenix last-resort edge bucket

继续对照 Angr `PhoenixStructurer._last_resort_refinement()`。Angr 不是把所有可虚拟化边直接交给排序，而是先分成三类：两端互不支配的边、source 不支配 target 的 secondary 边、其他边。优先级是第一类、第二类、最后才是其他边。当前 shared Phoenix 直接把所有边交给 `orderVirtualizableEdges()`，这会让 Phoenix/SAILR 的排序启发式在错误的候选集合上工作。

实现时还确认了一点：SAILR H2 里 Angr 统计的是 `PostDominators.post_dom` 的边数，而 Angr 这个 `post_dom` 本身是 immediate postdom tree，不是完整闭包。所以当前 NotDec 统计 immediate postdom 数量不需要改。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:904`
  新增 `filterByAngrLastResortPriority()`，按 Angr 的三类 last-resort edge bucket 过滤候选。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1336`
  `virtualizeOneEdge()` 先应用 bucket 过滤，再把候选交给 Phoenix/SAILR 的 `orderVirtualizableEdges()`。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，4 个测试，耗时约 `1.38s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-lastresort-priority-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步对齐的是 Phoenix last-resort 的候选边优先级，SAILR 会继续复用这个候选集合，然后再执行自己的 H1/H2/H3 排序。
- 目前没有单独的结构层测试 harness，所以专门断言“选中了 type1 边而不是 type2 边”还不方便；这次用现有 structuring smoke 和主链路 smoke 防回归。
- 实现效果：5/10。边选择流程更接近 Angr，但还缺 edge virtualization hints 和 root acyclic cycle fallback。
- 复杂度：3/10。只增加一个候选过滤函数。
- 维护成本：3/10。分类逻辑独立，后续补 Angr hints/fallback 时可以继续接在 last-resort 阶段。

# 2026-06-20 暂停点：root acyclic cycle fallback 需要先补 acyclic view

继续对照 Angr `PhoenixStructurer._last_resort_refinement()` 时，尝试补 type4：root acyclic region 里出现 cycle debris 时，Angr 会从 `to_acyclic_by_order()` 丢掉的 cycle-closing edges 里选一条虚拟化。

当前 shared structuring 还没有显式的 acyclic graph view，也没有记录“为了转成 acyclic view 被丢掉的边”。试过用 `MutableRegionGraphAnalysis::NodeOrder` 近似判断 `From` 到 `To` 的回边，但一个合法 root 后继环用例会输出很差：

```llvm
entry -> head
head -> a / b
a -> c
b -> c
c -> head / exit
```

这个用例能跑完，但会生成 `goto head` 加 `while (1)`，并且把 `c -> head` 处理成不正确的 `goto c` 形状。说明只靠当前 node order 猜 cycle-closing edge 不可靠，不能提交。

后续要继续复刻 Angr type4 fallback，先做下面这个边界：

- 给 `MutableRegionGraph` 或 `MutableRegionGraphAnalysis` 增加一个 acyclic view 构造，语义对齐 Angr `to_acyclic_by_order()`。
- 明确返回被移除的 cycle edges，type4 fallback 只从这些边里选。
- root acyclic fallback 只在 root region 使用；非 root region 仍让外层 cyclic region 接管。
- 给结构层补一个小 harness，直接断言被虚拟化的边，而不是从最终 C 文本猜。

当前判断：

- 这是计划没展开的技术决策点，不能继续用临时近似推进。
- 已回退试探性代码，当前没有代码改动。
- goal 不标完成；也不标 blocked，因为还能先做 acyclic view 设计/实现后继续。

# 2026-06-20 实现记录：补 acyclic dropped edge 并接入 type4 fallback

接上一个暂停点，先不在 Phoenix 里猜回边，而是在 `MutableRegionGraphAnalysis` 里记录构造 acyclic DFS view 时因为指向递归栈内节点而丢掉的边。这个语义对应 Angr `to_acyclic_by_order()` 后 type4 fallback 使用的 cycle-closing edges。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:52`
  `MutableRegionGraphAnalysis` 新增 `AcyclicDroppedEdges`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:190`
  `dfsOrder()` 增加递归栈参数；遇到指向栈内节点的 active edge 时记录到 dropped edges，不继续递归。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:648`
  `MutableRegionGraph::analyze()` 把 dropped edges 填进分析结果。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:904`
  `filterByAngrLastResortPriority()` 在 type1/type2 都为空时，root region 才返回 `AcyclicDroppedEdges`，非 root 不用这个 fallback。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:21`
  新增 `structuring-analysis-test`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:19`
  新增结构层测试，构造 `entry -> head -> a/b -> c -> head/exit`，直接断言 dropped edge 是 `c -> head`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.42s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-acyclic-dropped-edge-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步补上了 Angr type4 fallback 需要的数据来源，比之前试探的 node order 近似更稳。
- 现在 type4 只在 root region 使用，符合 Angr “内层失败交给外层 cyclic region 处理”的边界。
- 实现效果：6/10。last-resort 更接近 Angr，但还没有 edge virtualization hints。
- 复杂度：4/10。增加了分析结果字段和一个小测试目标。
- 维护成本：4/10。测试直接断言结构层数据，后续改 Phoenix 输出时不容易误伤。

# 2026-06-20 实现记录：补 edge virtualization hints 消费入口

继续对照 Angr `PhoenixStructurer._last_resort_refinement()`。Angr 在普通 last-resort bucket 之前，会先消费 `_edge_virtualization_hints`。当前 Angr 代码里能看到初始化和消费点，但没有在 structuring 模块内看到明确生产者；所以这一步只补 shared structuring 的消费边界，不编造 hint 来源。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:52`
  新增 protected hook `edgeVirtualizationHints()`，默认用于 Phoenix/SAILR 在 last-resort 前提供优先虚拟化边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1290`
  默认实现返回空列表。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1348`
  `virtualizeOneEdge()` 先消费 still-active hint edge，再走已有 last-resort bucket。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:10`
  新增 `HintStructurer` 测试派生类，覆盖 hook。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:63`
  新增 `testEdgeVirtualizationHints()`，直接断言 hint edge 优先被虚拟化。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.44s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-edge-hints-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步只补消费机制，不新增 hint 生产策略。
- 这样后续如果从 SAILR deoptimization 或 improved Phoenix schema 里产生 hint，可以接到同一个 hook。
- 实现效果：4/10。架构边界补齐，但功能上还没有真实 hint producer。
- 复杂度：2/10。新增一个 hook 和测试。
- 维护成本：3/10。默认空实现不会影响 Phoenix 行为。

# 2026-06-20 实现记录：head-controlled self-loop 输出 while

继续补 Angr Phoenix `_match_cyclic_while()` 的窄形状。Angr 会根据条件跳转在节点开头还是末尾区分 `while` 和 `do-while`。当前 shared Phoenix 的 self-loop reducer 对条件自环统一输出 `do-while`，导致 LLVM IR 里空 header 条件自环：

```llvm
head:
  %cond = icmp eq i32 %x, 0
  br i1 %cond, label %head, label %exit
```

输出成 `do { } while (x == 0);`。这个形状没有循环体语句，应该按 head-controlled loop 输出 `while (x == 0) {}`。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:733`
  `collectLinearDoWhileBody()` 跳过 `EntryId == LatchId`，避免 linear do-while schema 抢走单块 self-loop。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:861`
  `reduceSelfLoopOnce()` 对 `Tail->Statements.empty()` 的条件自环输出 `While`，非空 block 仍输出 `DoWhile`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:221`
  新增 `self_loop_while`，要求输出 `while (x == 0)`，不能输出 `do {` 或 `goto head`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.53s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-self-loop-while-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这是 Angr head-controlled self-loop 的保守子集，只用 `Tail->Statements.empty()` 区分。
- 多语句 self-loop 仍按 do-while 保留，避免把循环体前置语句错误挪到条件前。
- 实现效果：5/10。修掉单块条件自环，但还不是完整 `_match_cyclic_while()`。
- 复杂度：2/10。只改两个局部判断。
- 维护成本：3/10。有 smoke 覆盖。

# 2026-06-20 暂停点：type4 fallback 还缺虚拟化后的 terminator 改写

补完 `AcyclicDroppedEdges` 以后，又用 root 后继环重新看了输出：

```llvm
entry -> head
head -> a / b
a -> c
b -> c
c -> head / exit
```

当前仍会输出 `goto head`、`while (1)`，并且在 `c` 后生成错误的 `goto c`。说明只是知道 type4 应该虚拟化 `c -> head` 还不够。Angr 的 `_virtualize_edge()` 会改写 source block 的条件跳转，把被移除的 edge 从原始 conditional jump 里拿掉；shared structuring 当前只记录 `VirtualEdge`，fallback 渲染时仍按原始 `CFGBlock::Successors` 生成两个方向，所以会把已虚拟化的边又通过原始 terminator 渲染出来。

这轮试过两个局部修法，都没有提交：

- fallback 渲染 branch/switch/fallthrough 时跳过 virtualized successor。
- `MutableRegionGraph::collapseNodes()` 后把 `VirtualEdge.From/To` 映射到 collapsed node。

它们没有解决 root cycle 输出，原因是 virtualized edge 和 collapse 后的 fallback node 对应关系还不完整。继续修需要先明确一个边界：shared structuring 是否应该像 Angr 一样在虚拟化时生成一个“改写后的 terminator view”，而不是只靠 `VirtualEdge` 在末尾补 `goto`。

后续建议：

- 给 `MutableRegionNode` 或 `StructuredNode` 增加 terminator override/removed edge 信息，表示某个原始 successor 已经被 virtualized。
- fallback branch/switch 渲染只依据这个 view 输出仍然保留的 successor。
- collapse 节点时要合并这个 view，而不是只 remap `VirtualEdge` 的 graph id。
- 先用结构层测试断言 `c -> head` 被 virtualized 后，`c` 的 fallback terminator 不再包含 `head` successor；再看最终 C 文本。

当前判断：

- 这是 Angr `_virtualize_edge()` 语义在 shared model 里的缺口，计划里没有展开。
- 已回退试探性代码，当前没有代码改动。
- goal 不标完成；也不标 blocked，因为可以先设计 terminator view 再继续。

# 2026-06-20 暂停点更新：virtualized edge 需要改写 source node

继续对照 Angr `PhoenixStructurer._virtualize_edge()` 和 root-cycle case：

```llvm
entry -> head
head -> a / b
a -> c
b -> c
c -> head / exit
```

现象：当前 shared Phoenix/SAILR 仍会在 `c()` 后输出错误的 `goto c`。结构层 dump 显示这个 `Goto target=c` 已经出现在 natural-loop 子区域的 tree 里，不是 C AST 适配层，也不是 root fallback 后加出来的。

这轮确认过几个方向，暂不提交代码：

- 给 `MutableRegionNode` 记录 `RemovedSuccs`，并在 collapse 时合并。这个只能说明某条 successor 被删过，不能阻止已写进 structured tree 的旧 `goto`。
- 在最终 fallback 阶段跳过“目标已经在当前 collapsed node 内”的 virtual edge。无效，因为错误 `goto c` 已经在 natural-loop 子区域 collapse 前写进 tree。
- 让 root 直接复用 natural-loop child structured node。这个会破坏已有 while/do-while schema，`structuring-smoke` 里多个 while case 退化成 `while (1)` 或丢失 break。
- 放宽 `reduceSequenceOnce()`，允许已有 structured root 和 follow 节点合并。这个会把 loop backedge 吃掉，输出更差。

Angr 的真实做法更明确：

- `/sn640/angr/angr/analyses/decompiler/structuring/phoenix.py:3035` 的 `_virtualize_edge()` 不只是记录边。
- 如果 source 末尾是 `ConditionalJump`，Angr 会把它拆成 `ConditionNode(Jump to removed dst) + Jump to other dst`，然后 `detach_edge(src, dst)`，必要时 `replace_nodes_both(src, new_src)`。
- cyclic refinement 成功后，Angr 还会把 loop body 里的 head 跳转重写成 `ContinueNode`，把 loop exit 重写成 break 形状。

因此 shared model 缺的不是“removed successor 集合”，而是 virtualized edge 生命周期：虚拟化一条边时，source node 的末尾控制流也要形成一个可被后续 reducer/renderer 识别的 structured representation。只在 `MutableRegionGraph::VirtualizedEdges` 里记账，然后等 fallback 尾部补 `goto`，会把已经应该顺序化的 follow edge 变成错误 goto。

下一步建议：

- 在 shared structuring 层增加一个 source-node replacement/terminator override 能力，语义对齐 Angr `_virtualize_edge()`：虚拟化条件边时，把 source 末尾 conditional 分裂成“被移除边的条件跳转节点”和“保留边跳转节点”。
- 这个 representation 应该进入 `StructuredTree`，而不是只保存在 `MutableRegionGraph`。
- `lastResortRefinement()` 调 `Graph.virtualizeEdge()` 后，要能返回或安装这个 replacement node；后续 collapse 使用 replacement 作为 source 的 `StructuredRoot`。
- root-cycle 应加到 `test/structuring/run_structuring_smoke.py`，至少断言不能出现 `goto c;` 这种 self-target goto。

当前状态：

- 试探性代码已全部回退，`external/NotDec-llvm2c` 当前无未提交代码改动。
- goal 继续保持 active；还没有达到“Angr-style virtual edge 生命周期”这个实现点。

# 2026-06-20 实现记录：virtualized source 与 collapsed tail block

继续补 Angr `_virtualize_edge()` 在 shared model 里的缺口。这次没有完整复刻 Angr 的 AIL node replacement，但补了两个必要边界：

- graph node 记录真实 tail block，避免 collapse 后用 `Blocks.back()` 猜 tail。
- last-resort virtualize 条件边时，可以给 source node 安装 structured replacement。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:39`
  `MutableRegionNode` 新增 `TailBlock`，表示 fallback 应该参考的尾部 terminator。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/MutableRegionGraph.h:81`
  新增 `MutableRegionGraph::setStructuredRoot()`，给 virtualize 阶段替换 source node 用。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:370`
  `addNode()` 初始化 `TailBlock`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:540`
  实现 `setStructuredRoot()`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:548`
  `virtualizeEdge()` 的 `FromBlock` 改用 `TailBlock`。
- `external/NotDec-llvm2c/lib/Structuring/MutableRegionGraph.cpp:564`
  `collapseNodes()` 合并 `TailBlock`。如果成员有出到 collapsed node 外部的 edge 或 external successor，优先用这个成员的 tail。这个修掉 root-cycle 中 `{c, head, a, b}` 被误认为 tail 是 `b` 的问题。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:42`
  `lastResortRefinement()` 和 `virtualizeOneEdge()` 接收 `StructuredTree &Tree`，为 source replacement 准备 tree node。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:990`
  新增 `nodeContainsBlock()`，fallback 追加 virtual edge 时跳过已经在 collapsed node 内部的目标。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:995`
  新增 `buildVirtualizedBranchSource()`。source tail 是二分支时，把 removed edge 表示成 `if (...) control-transfer`，再补保留边的 control-transfer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1075`
  `appendFallbackNode()` 用 `TailBlock` 找 tail，不再用 `Blocks.back()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1189`
  `hasDirectLoopControlTransfer()` 递归检查 nested `break/continue`，让 `if (...) continue; else break;` 能触发 natural-loop wrapper。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1407`
  `lastResortRefinement()` 传入 tree。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1417`
  `virtualizeOneEdge()` 在 `Graph.virtualizeEdge()` 前尝试安装 source replacement。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:38`
  新增 `branchBlock()` helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:67`
  更新 hint 测试，断言 virtualize 后 source node 有 `StructuredRoot`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:239`
  新增 `root_cycle_follow`，要求不能输出 `goto c;`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-virtualized-source-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

root-cycle 当前输出核心形状：

```c
while (1) {
  head:
    if (x == 0) { a(); } else { b(); }
    c();
    if (y == (char **)0) { continue; } else { break; }
}
```

当前判断：

- 这一步修掉了 `Blocks.back()` 误作 tail 导致的 `goto c`，并把 source replacement API 接进 last-resort。
- 这还不是完整 Angr `_virtualize_edge()`：现在只处理 branch tail，switch 和普通 jump 还没有 source replacement。
- 实现效果：6/10。root-cycle 回归修掉，virtual edge 生命周期向 Angr 靠近了一步。
- 复杂度：5/10。新增了 `TailBlock` 状态和 virtualize 阶段 tree 写入。
- 维护成本：5/10。TailBlock 需要后续 reducer collapse 时继续维护，测试已覆盖当前暴露的错误。

# 2026-06-20 实现记录：补齐 virtualized source 的 switch/fallthrough 形状

上一轮只给 branch tail 做了 source replacement。这轮继续对齐 Angr `_virtualize_edge()` 的边界：source tail 不是条件分支时，也应该能把 virtualized edge 写进 structured tree，而不是只在 graph 里记账。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:995`
  新增 `appendSourceBody()`，统一把已有 `StructuredRoot` 或原始 block body 放进 replacement sequence。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1007`
  `buildVirtualizedBranchSource()` 改成接收已解析的 tail block，只负责 branch 形状。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1039`
  新增 `buildVirtualizedSwitchSource()`，当 removed edge 命中 switch default/case 时生成 structured switch，并在 switch 后补保留 successor 的 control-transfer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1087`
  新增 `buildVirtualizedFallthroughSource()`，fallthrough edge 被 virtualized 时生成 source body + control-transfer。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1107`
  新增 `buildVirtualizedSource()`，按 tail terminator 分发 branch/switch/fallthrough。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:14`
  `HintStructurer` 改成可指定 from/to，方便覆盖不同 source tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:50`
  新增 `switchBlock()` helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:111`
  新增 switch source replacement 测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:134`
  新增 fallthrough source replacement 测试。

验证：

- `cmake --build ../../build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir ../../build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.63s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-virtualized-source-shapes-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- virtualized source replacement 现在覆盖 branch/switch/fallthrough 三类 tail，比上一轮更接近 Angr `_virtualize_edge()` 的节点替换语义。
- 仍未覆盖 return/unreachable，当前不需要；这些 terminator 没有正常 successor，通常不应被 virtualized。
- 实现效果：5/10。补齐 source replacement 的基础形状，但还不是 Angr 那种精确 AIL statement 拆分。
- 复杂度：4/10。主要是 helper 分拆和结构层测试。
- 维护成本：4/10。逻辑集中在 `buildVirtualizedSource()`，后续扩展比较直接。

# 2026-06-20 实现记录：整理 structurer registry

对照 Angr `structuring/__init__.py` 的 `STRUCTURER_CLASSES` 和 `structurer_class_from_name(name.lower())`，这轮整理 shared backend 的 structurer registry。目标是让后续继续接 Dream 或其它 Angr structurer 时，只维护一份注册表。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:34`
  继续使用 `StructurerRegistration` 表作为唯一注册源。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:40`
  新增 `makeStructurerNames()`，从注册表生成 `registeredStructurerNames()` 返回的 name array，去掉手写重复表。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:50`
  新增 `equalsIgnoreCase()`。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:63`
  `createStructurer()` 改成大小写不敏感查找，行为更接近 Angr 的 `name.lower()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:156`
  新增 `testStructurerRegistryNames()`，断言注册名顺序和大小写不敏感创建。

验证：

- `cmake --build ../../build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir ../../build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-registry-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步不改变命令行 `--algo` 的枚举值，只整理 shared registry 内部行为。
- 后续接 Dream 或其它算法时，只需要往 `Structurers` 表里加一项。
- 实现效果：3/10。是架构清理，不新增算法能力。
- 复杂度：2/10。局部改动。
- 维护成本：2/10。去掉重复名字表后更低。

# 2026-06-20 实现记录：收紧 C backend 到 shared structurer 的入口

上一轮 shared registry 已经集中维护 `goto/phoenix/sailr`，但 C backend 的
`SAFuncContext::run()` 里仍按 enum 手写多段 `StructuredGoto` 分支。这轮保持 CLI
行为不变，只把 enum 到 shared structurer 名字的映射集中到接口层。

修改内容：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:25`
  新增 `getStructurerName()`，把 `SA_Goto` / `SA_StructuredGoto` 映射到
  `goto`，`SA_StructuredPhoenix` 映射到 `phoenix`，`SA_StructuredSAILR`
  映射到 `sailr`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2126`
  `SAFuncContext::run()` 改成统一解析 structurer 名字，再调用一次
  `StructuredGoto` adapter，去掉算法分支里的重复执行逻辑。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.62s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-structurer-entry-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这一步没有开放自由字符串 `--algo`，避免改变现有非法算法报错和测试行为。
- C backend 入口现在只关心“选择哪个 shared structurer”，更接近 Angr 的
  name-to-structurer 边界。
- 实现效果：3/10。主要是入口清理，不新增 reducer 能力。
- 复杂度：2/10。改动集中在接口映射和调用点。
- 维护成本：2/10。后续新增算法时，至少不用再复制 `StructuredGoto` 执行分支。

# 2026-06-20 实现记录：确认多 latch 自然循环的 region 边界

继续看 Angr 风格 cyclic refinement 时，先检查 multi-exit loop 输出异常是不是
`RegionIdentifier` 漏掉了同一个 head 下的第二条 latch 路径。结论：不是。当前 region
识别能把 `head -> a -> b -> head` 和 `head -> a -> c -> d -> head` 合并成一个
natural loop，并把唯一外部 successor 识别为 `exit`。

修改内容：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3`
  引入 `RegionIdentifier`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:174`
  新增 `testMergedNaturalLoopKeepsAllLatchPaths()`，构造同一个 loop head、两条 latch
  路径、一个 exit 的 CFG，断言 natural loop blocks 是 `{0,1,2,3,4}`，successor 和
  follow 都是 `5`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.62s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-region-boundary-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- multi-exit loop 现在仍不是 Angr 期望的完成态：parent reducer 可能先把 loop 内块和
  follow 块折成一个 `StructuredRoot`，后续 natural-loop fallback 不能只裁掉外部
  return/goto。
- 试过两个直接修法都不合适：把 natural-loop child 直接传给 root 会让简单 while/do-while
  大幅退化；在 fallback 里按原始 block 重渲染跨边界 node 会丢条件结构。
- 下一步需要先定边界：要么禁止 acyclic reducer 跨 natural-loop 边界 collapse，要么让
  `StructuredTree` 支持从 collapsed source 里裁出 loop 内部分。这个点计划里没有细化，
  不应继续靠局部补丁硬猜。
- 实现效果：2/10。新增的是定位测试，不是算法修复。
- 复杂度：1/10。只加测试。
- 维护成本：2/10。这个测试能防止后续把 region 识别误诊成问题来源。

# 2026-06-20 实现记录：保护 natural-loop exit 边界

接上一个边界问题，这轮选择更接近 Angr region 边界的方案：不让 root 里的 acyclic
sequence/if/switch reducer 把 natural loop 内部节点和 loop exit 节点折成同一个
`StructuredRoot`。这样后续 natural-loop fallback 还能在 loop 内把出口渲染成
`break`，而不是把 `c` 这类 loop 内块留到 loop 外。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h:34`
  `analyzeAcyclic()` 增加 `RegionTree` 和当前 `Region` 参数，让 acyclic schema 能看到
  region 边界。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:123`
  新增 block membership helper。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:147`
  新增 `collapseCrossesNaturalLoopBoundary()`。当前只保护 root 下 natural-loop 的 exit
  边界，避免挡住普通 preheader 到 loop head 的边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:195`
  `reduceSequenceOnce()` 在 collapse 前检查 natural-loop exit 边界。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:218`
  `reduceIfOnce()` 在 collapse 前检查 natural-loop exit 边界。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:360`
  `reduceSwitchOnce()` 在 collapse 前检查 natural-loop exit 边界。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1046`
  新增 terminal block 判断，用于识别 C backend `RetDupPass` 生成的 return clone。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1064`
  `classifyNaturalLoopExit()` 改成接收 `StructuredCFG`，把 natural loop 的 terminal
  exit clone 也分类成 `break`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:104`
  multi-exit loop smoke 改为断言两个 `break`、两个 `continue`，并禁止 `goto exit`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:33`
  放宽简单 loop 用例对 preheader `goto head/body` 的格式要求。这个 goto 是当前 fallback
  形状，不影响本轮要验证的 loop exit 边界。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:335`
  不再要求 `structured-phoenix` 一定没有 `break`。Phoenix 的 natural-loop fallback
  也可以合法地产生 break，不能把 break 当成 SAILR-only 行为。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-loop-boundary-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- multi-exit loop 现在的核心控制流更接近 Angr：loop 内两个出口都变成 `break`，两个回边变成
  `continue`，`c` 不再掉到 loop 外。
- 输出里仍可能有 preheader `goto head` 和重复 terminal `return`。这是 C fallback/RetDup
  后处理问题，不是这轮的 structuring 边界问题。
- 实现效果：5/10。修掉 multi-exit loop 的主要结构问题，但还没做到漂亮输出。
- 复杂度：4/10。新增了 region-aware acyclic guard，但只在 root natural-loop exit 上生效。
- 维护成本：4/10。后续如果要更接近 Angr，应继续把 region overlay 做实，而不是扩大这个 guard。

# 2026-06-20 实现记录：补强 multi-exit loop 回归断言

上一轮修掉了 multi-exit loop 里 `c` 掉到 loop 外的问题。这轮没有继续改算法，只把 smoke
断言补强，避免后续回归时仍因为只检查 `break/continue` 数量而漏掉块顺序问题。

修改内容：

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:307`
  新增通用 `ordered` 断言，检查一个输出片段必须出现在另一个片段之前。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:107`
  multi-exit loop 用例新增 `("c();", "return 0;")` 顺序断言，防止 `c` 再次落到 loop 后面。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.64s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-loop-boundary-order-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 重复 terminal `return` 仍存在，但它来自 C backend 的 return duplication / payload 渲染，不适合在 shared structuring 层靠节点去重硬修。
- 实现效果：2/10。只是加回归保护。
- 复杂度：1/10。测试脚本小改。
- 维护成本：1/10。断言简单，后续可复用于其它结构顺序回归。

# 2026-06-20 实现记录：去掉紧邻 infinite loop 的入口 goto

继续收窄 root-cycle 输出。`root_cycle_follow` 里 root sequence 可能出现
`goto head; while (1) { head: ... }`。这不是一般 goto 清理，只是 fallback 把 loop
节点排在入口跳转后面导致的冗余。按 Angr 的结构化结果，loop 本身就是顺序中的下一条语句，
这个 goto 可以删掉。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1436`
  新增 `dropGotoIntoFollowingInfiniteLoop()`，只删除同一个 `Sequence` 里直接相邻的
  `Goto(target=head)` + `InfiniteLoop(block=head)`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1761`
  在 root fallback 生成 sequence 后调用该 helper。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:271`
  `root_cycle_follow` smoke 增加 `goto head` absence 断言。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.62s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-drop-loop-entry-goto-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这是一个很窄的结构树 cleanup，不处理一般 goto，也不跨 label。
- root-cycle 输出更接近 Angr：函数直接进入 `while (1)`，不再先 `goto head`。
- 实现效果：3/10。只改善一个 fallback 输出形状。
- 复杂度：2/10。局部 sequence 后处理。
- 维护成本：2/10。规则窄，误伤风险低。

# 2026-06-20 实现记录：补 SAILR edge ordering 策略测试

回到整体算法主线。这轮不继续清理 C 输出细节，而是把 SAILR 的关键差异固定到测试里。
当前 shared SAILR 已经实现 Angr `SAILRStructurer._order_virtualizable_edges()` 的 H1/H2/H3
入口，但之前没有直接测试这些策略，后续改 Phoenix last-resort 时容易误伤。

修改内容：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4`
  引入 `SAILRStructurer`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:39`
  新增 `TestSAILRStructurer`，只用于暴露 protected 的 `orderVirtualizableEdges()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:203`
  新增 `testSAILROrderPrefersLeastSiblingEdges()`，覆盖 SAILR H1：优先选择 sibling edge
  更少的目标。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:232`
  新增 `testSAILROrderPrefersReturnTargetTieBreak()`，关闭 postdom 过滤后覆盖 SAILR H3：
  tie-break 时优先选择 return target。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure` 通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-sailr-ordering-smoke.c --tr-level=2 --algo=structured-sailr` 通过。

当前判断：

- 这不是新 reducer，但它把 SAILR 的核心 edge virtualization 策略固定住了。
- H2 的 postdom 计数还没有单独断言，后续如果继续改 dominator/postdom graph，可以再补。
- 实现效果：2/10。测试补强。
- 复杂度：1/10。只暴露 protected 方法给测试。
- 维护成本：2/10。后续重构 SAILR ordering 时能直接抓回归。

# 2026-06-20 实现记录：补 SAILR H2 postdom ordering 覆盖

继续补 SAILR 和 Angr 对齐的关键行为。这轮只补测试，不调整算法。目标是直接覆盖
Angr `SAILRStructurer._order_virtualizable_edges()` 的 H2：H1 打平后，优先选择删除后
postdominator 关系最多的 virtual edge。

修改内容：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:232`
  新增 `testSAILROrderPrefersMostPostDominators()`。用一个小 CFG 让两条候选边在 H1
  下打平，再验证 H2 选择 `1 -> 4`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:302`
  在 `main()` 里接入该测试。

验证：

- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.62s`。

当前判断：

- H1/H2/H3 现在都有直接测试，后续继续补 Phoenix/SAILR reducer 时不容易误改 edge ordering。
- 实现效果：2/10。只补算法策略覆盖。
- 复杂度：1/10。单个测试用例。
- 维护成本：1/10。没有新增生产代码。

# 2026-06-20 实现记录：补 graph-level natural loop refinement

继续按 Angr 的 `PhoenixStructurer._refine_cyclic_core()` 方向推进。这轮只补最小 natural-loop
refinement：从当前 `MutableRegionGraph` 里的 back edge 找 loop head/latch，收集被 head 支配的
loop body，确认最多一个 loop successor 后折成 `InfiniteLoop`。更复杂的 while/do-while 判定、
多 successor 选择和 outgoing edge rewrite 还没有做。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1499`
  新增 `collectNaturalLoopMembers()`，用当前 graph 的 dominator 信息从 latch 反向收集 natural
  loop 成员。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1527`
  新增 `collectNaturalLoopSuccessors()`，统计 loop body 之外的 successor block。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1555`
  新增 `reduceGraphNaturalLoopOnce()`。它构造临时 `NaturalLoop` region，复用
  `appendFallbackNode()` 生成 body，让已有 break/continue 分类继续生效。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1776`
  `PhoenixStructurer::refineCyclic()` 先尝试 graph-level natural-loop refinement，再走旧的
  root region fallback。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:45`
  新增 `TestPhoenixStructurer`，只用于测试 protected 的 `refineCyclic()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:209`
  新增 `testRefineCyclicReducesGraphNaturalLoop()`，验证 refinement 能把当前 graph 中的
  natural loop 折成 `InfiniteLoop`，并保留到 follow block 的 successor。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.62s`。
- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-graph-natural-loop-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这是 Angr cyclic refinement 的第一步，不是完整 `_refine_cyclic_core()`。
- 保守限制为最多一个 successor，避免现在就处理多出口 successor 选择。
- 实现效果：4/10。shared Phoenix/SAILR 现在能不依赖预先识别的 root loop 做一次 graph-level
  natural-loop refinement。
- 复杂度：3/10。新增逻辑局部，但开始接近 Angr refinement 主路径。
- 维护成本：3/10。后续补 while/do-while refinement 时可能会调整这些 helper。

# 2026-06-20 实现记录：graph-level refinement 生成 while

继续补 Angr while-refinement 的保守子集。上一轮 graph-level natural-loop refinement 只能生成
`InfiniteLoop`。这轮在 loop head 是纯条件分支、两个后继里一个进 loop body、一个到唯一 successor
时，直接生成 `While`。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1558`
  新增 `makeGraphWhileLoop()`。它检查 head 条件分支和 successor，生成 `While`，body 里跳过
  head block。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1610`
  新增 `makeGraphInfiniteLoop()`，保留旧 fallback。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1690`
  `reduceGraphNaturalLoopOnce()` 优先调用 `makeGraphWhileLoop()`，失败时仍生成 `InfiniteLoop`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:240`
  `testRefineCyclicReducesGraphNaturalLoop()` 改为断言生成 `While`，并检查 loop block 是 head
  block `1`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.64s`。
- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-graph-while-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这只覆盖 Angr `_refine_cyclic_is_while_loop()` 的简单情况。
- do-while refinement、多 successor 选择、outgoing edge rewrite 还没做。
- 实现效果：4/10。graph-level refinement 不再总是退到 `while (1)`。
- 复杂度：3/10。新增逻辑局部，判断条件保守。
- 维护成本：3/10。后续补 do-while 时可能需要把 loop body 构造再拆细。

# 2026-06-20 实现记录：graph-level refinement 合并多 latch

继续补 natural-loop refinement 的算法边界。之前 `reduceGraphNaturalLoopOnce()` 按单条 back edge
收 loop body，多 latch loop 会漏掉其它 latch 路径。这轮改成按 head 收集所有 back edge，再合并
body，和 RegionIdentifier 里对同一 head loop 的处理保持一致。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1645`
  `reduceGraphNaturalLoopOnce()` 从“遍历 latch 的 succ”改成“遍历 head 的 pred”，对同一 head
  的多个 back edge 分别调用 `collectNaturalLoopMembers()`，再合并成员。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1565`
  `makeGraphWhileLoop()` 放宽 head 节点必须单 block 的限制，但只接受 head 前缀 block 无语句，
  避免吞掉真实 loop body。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:253`
  新增 `testRefineCyclicMergesMultipleLatches()`，验证两个 latch 都保留在折叠后的 while 节点里。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.63s`。
- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-multilatch-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这是算法层修正，不保证现有完整 C 输出马上从 `while (1)` 变成条件 `while`。
- 多 successor 选择和 outgoing edge rewrite 仍未实现，后面应继续按 Angr `_refine_cyclic_core()`
  后半段推进。
- 实现效果：4/10。多 latch natural loop 不再只看第一条 back edge。
- 复杂度：3/10。逻辑仍局限在 graph refinement。
- 维护成本：3/10。后续实现多 successor 时会继续用这套 head/latch 合并结果。

# 2026-06-20 实现记录：graph-level refinement 处理非 follow 出边

继续补 Angr `_refine_cyclic_core()` 里 outgoing edge 的最小路径。之前 graph-level natural loop
遇到多个 successor 会直接放弃。这轮选择一个 successor 作为 loop follow，其它 loop exit 在
body 内虚拟化成 `goto`，并从 graph 边里断开，避免折叠后的 loop 节点保留多个后继。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1278`
  新增 `nodeTreeContainsControlTransfer()`，避免已经虚拟化过的 source node 被 fallback 再按原始
  terminator 渲染一次。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1565`
  新增 `chooseNaturalLoopSuccessor()`。当前先保守选择 block id 最小的 successor。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1572`
  新增 `virtualizeNonFollowLoopExits()`，把非 follow exit 通过 `buildVirtualizedSource()` 写进
  source node，再调用 `Graph.virtualizeEdge()` 断开 graph 边。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1732`
  `reduceGraphNaturalLoopOnce()` 不再因为多个 successor 直接放弃，而是设置单一 follow 并虚拟化
  其它 exit。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:294`
  新增 `testRefineCyclicVirtualizesNonFollowExits()`，验证非 follow exit 被 virtualize，折叠后的
  while 节点只保留一个 follow successor。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:104`
  `multi_exit_loop_fallback` 期望从 `while (1)` 调整为 `while (x == 0)`，head exit 被条件吸收后
  `break;` 数量从 2 变为 1。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.62s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-outgoing-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这只是 outgoing edge rewrite 的最小版本；Angr 里按 edge count 选择 successor、parent region
  限制、continue edge rewrite 还没有完整复刻。
- `multi_exit_loop_fallback` 仍有 `goto b/c/d`，那是后续 acyclic/sequence reducer 的问题，不在
  这次 outgoing rewrite 里硬修。
- 实现效果：5/10。多出口 loop 现在能在 shared refinement 里继续收敛到条件 while。
- 复杂度：4/10。开始修改 graph 边和 source replacement，后续需要更完整的 Angr 规则兜住。
- 维护成本：4/10。successor 选择策略后续要替换成 Angr 的 edge-count 规则。

# 2026-06-20 实现记录：按 Angr edge-count 选择 natural-loop successor

继续对齐 Angr `_refine_cyclic_core()` 的 successor 选择。上一轮多 successor 时先选 block id
最小的 successor，这和 Angr 不一致。Angr 在 successor 还没确定时，会统计 outgoing edge 目标，
选择 edge 数最多的目标，打平后选地址最低的目标。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1565`
  `chooseNaturalLoopSuccessor()` 改为接收 graph 和 loop members，按 exit edge 数最多选 successor，
  打平后选 block id 最小。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1603`
  新增 `findGraphWhileSuccessor()`。如果 loop head 是纯条件分支，先用 head 的出 loop 分支作为
  while successor；只有自然 loop fallback 才用 edge-count 规则。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1798`
  `reduceGraphNaturalLoopOnce()` 先调用 `findGraphWhileSuccessor()`，失败后再调用
  `chooseNaturalLoopSuccessor()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:338`
  新增 `testRefineCyclicPrefersMostCommonExitAsFollow()`，验证自然 loop 有多个 exit 时选择 edge
  数更多的 target 作为 follow。

验证：

- `cmake --build ./build --target structuring-analysis-test -j4` 通过。
- `cmake --build ./build --target notdec-llvm2c-exe notdec -j4` 通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-successor-edge-count-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- successor 选择现在更接近 Angr：while head 已确定 successor 时不被 edge-count 覆盖；natural-loop
  fallback 才用 edge-count。
- parent region 限制、dangling successor 检查、continue edge rewrite 还没完整复刻。
- 实现效果：4/10。修正一处 Angr 行为差异。
- 复杂度：3/10。局部策略替换。
- 维护成本：3/10。后续补 parent/dangling 规则时会继续复用该策略。

# 2026-06-20 实现记录：多 continue edge 虚拟化

继续补 Angr `_refine_cyclic_core()` 的 continue edge rewrite。Angr 在 loop body 有多条回到
loop head 的边时，会保留拓扑上最后一条，其他回边改成 `continue`。这轮先按 block 顺序保留
最后一个 latch，其余回边通过 `VirtualEdgeKind::Continue` 虚拟化。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1680`
  新增 `virtualizeExtraContinueEdges()`，收集 loop body 内所有回到 head 的边，保留最后一个，
  其它边用 `buildVirtualizedSource()` 改写 source node 后调用 `Graph.virtualizeEdge()`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1888`
  `reduceGraphNaturalLoopOnce()` 在处理非 follow exit 后调用 `virtualizeExtraContinueEdges()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:294`
  新增 `testRefineCyclicVirtualizesExtraContinues()`，验证多 latch loop 中额外回边会变成
  `VirtualEdgeKind::Continue`。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.62s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-extra-continue-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这是 continue rewrite 的最小实现；Angr 用 fullgraph quasi-topological order 选择保留边，
  当前先用 block 顺序近似。
- switch-case head 的特殊排除、找不到 source block 时只断边等细节还没有完整复刻。
- 实现效果：4/10。多 latch 不再全部作为真实回边保留。
- 复杂度：3/10。复用现有 virtualized source 机制。
- 维护成本：3/10。后续补 Angr 的精确排序时需要调整保留边选择。

# 2026-06-20 实现记录：graph-level do-while refinement

继续补 Angr `_refine_cyclic_is_dowhile_loop()` 的保守子集。已有 `reduceLinearDoWhileOnce()` 只处理
线性链路；这轮在 graph-level refinement 里识别“唯一 latch 是条件分支，两个 successor 分别是
loop head 和 follow”的情况，生成 `DoWhile`。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1791`
  新增 `makeGraphDoWhileLoop()`，检查唯一 latch、latch 条件分支和唯一 follow，生成 `DoWhile`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1947`
  `reduceGraphNaturalLoopOnce()` 在 while 识别失败、且原始 successor 不超过一个时尝试
  `makeGraphDoWhileLoop()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:319`
  新增 `testRefineCyclicBuildsDoWhileFromLatchCondition()`，验证 latch 条件能生成 `DoWhile`，并保留
  follow successor。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.61s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-graph-dowhile-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这是 do-while refinement 的保守入口，不处理多出口 do-while。
- 如果同一个 loop 同时像 while 和 do-while，目前仍优先 while；Angr 里还有 parent-region 下的
  tie-break 规则，后续再补。
- 实现效果：4/10。graph-level refinement 可以从 latch 条件生成 `DoWhile`。
- 复杂度：3/10。局部 loop-kind 判定。
- 维护成本：3/10。后续补 while/do-while tie-break 时会调整优先级。

# 2026-06-20 实现记录：while/do-while tie-break

继续补 Angr `_refine_cyclic_core()` 里 while 和 do-while 同时成立时的取舍。Angr 会避免把
do-while 的 latch 条件浪费成 while body 里的 break。这轮加入保守规则：唯一 back edge 时，
如果 head 条件能形成 while、latch 条件也能形成 do-while，且二者 follow 不同，则优先 do-while。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1638`
  新增 `findGraphDoWhileSuccessor()`，只读取 latch 条件分支，判断 do-while follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1903`
  `reduceGraphNaturalLoopOnce()` 记录 back edge 数量，只在唯一 back edge 时参与 do-while
  tie-break。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1932`
  如果 while follow 和 do-while follow 不同，设置 `PreferDoWhile`，优先使用 do-while follow。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1981`
  loop kind 生成时尊重 `PreferDoWhile`，否则仍保持普通 while 优先。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:360`
  新增 `testRefineCyclicPrefersDoWhileWhenLatchConditionWouldBecomeBreak()`，验证 latch 条件不会被
  while 方案吞成 break。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.63s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-dowhile-tiebreak-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这只是 Angr tie-break 的一个核心子集；parent region 里的 successor 顺序比较还没做。
- 多 back edge 情况仍不参与 do-while tie-break。
- 实现效果：4/10。避免一个常见 do-while 条件被降级成 break。
- 复杂度：3/10。只加判定，不改结构树接口。
- 维护成本：3/10。后续补 parent-region 顺序时会扩展这个判断。

# 2026-06-20 实现记录：补 outgoing edge dangling successor 检查

继续补 Angr `_refine_cyclic_core()` 的 outgoing edge sanity check。Angr 在移除 outgoing edge 前会检查：
如果某个目标节点的所有入边都会被切掉，说明当前 refinement 还没准备好，应放弃本轮。这轮在
graph-level refinement 里加入同类保护。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1704`
  新增 `wouldDetachAllPredecessorsOfNonFollowSuccessor()`，统计即将虚拟化掉的 non-follow exit，
  如果会切掉某个非 terminal target 的所有 predecessor，则返回 true。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2008`
  `reduceGraphNaturalLoopOnce()` 在虚拟化 non-follow exit 前调用该检查，命中时跳过本轮
  refinement。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:446`
  新增 `testRefineCyclicKeepsDanglingNonFollowExit()`，验证非 terminal exit 会被切成 dangling
  target 时，本轮 refinement 放弃且不产生 virtual edge。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.63s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-dangling-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- terminal exit 不触发 dangling 保护，否则会破坏现有 loop break/return 收敛。
- 这只是 root-like graph refinement 的保护；Angr parent-region 限制还没补。
- 实现效果：4/10。补上 outgoing edge rewrite 的一个关键安全边界。
- 复杂度：2/10。局部检查。
- 维护成本：2/10。后续 parent-region 规则可以放在同一处前置检查。

# 2026-06-20 实现记录：child region 多 successor refinement guard

继续补 Angr `_refine_cyclic_core()` 的 parent-region 限制。Angr 在有 parent region 时，如果 cyclic
refinement 需要处理多个 successor，会直接放弃，避免在 child region 内擅自决定外层 follow。
当前 shared structuring 没有完整 parent region 对象，这轮用 `R.Kind != Root` 作为保守边界。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1926`
  `reduceGraphNaturalLoopOnce()` 增加 `const Region &R` 参数，用于判断当前是否是 root region。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1963`
  如果当前 region 不是 root 且 loop 有多个 successor，则跳过 graph-level refinement。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2183`
  `PhoenixStructurer::refineCyclic()` 传入当前 region。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:470`
  新增 `testRefineCyclicRejectsChildRegionMultipleSuccessors()`，验证 child natural-loop region 中多个
  successor 不会被当前 refinement 强行处理。

验证：

- `cmake --build ./build --target structuring-analysis-test notdec-backend-structuring notdec-llvm2c-exe notdec -j4`
  通过。
- `ctest --test-dir build -R 'structuring-analysis|structuring-smoke|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry' --output-on-failure`
  通过，5 个测试，耗时约 `1.64s`。
- `./build/bin/notdec test/type-recovery/llvm-ir/cases/14_Equality1.ll -o /tmp/notdec-c-child-multisuccessor-refine-smoke.c --tr-level=2 --algo=structured-sailr`
  通过。

当前判断：

- 这是 parent-region 限制的保守版本，不等价于 Angr 的完整 parent graph 判断。
- root region 仍允许多 successor refinement，保持之前 multi-exit loop 的改善。
- 实现效果：3/10。补上一个避免 child region 过早决策的边界。
- 复杂度：1/10。只增加 region kind 检查。
- 维护成本：2/10。后续如果引入显式 parent region，可替换这个判断。

# 2026-06-20 实现记录：continue edge 保留顺序改用 graph order

继续把 graph-level loop refinement 往 Angr 的图顺序靠。之前多条回头边时，
`virtualizeExtraContinueEdges()` 按 block id 排序并保留最大 block id 的真实边。
这和 Angr 基于 region graph 顺序处理边的方式不一致，也会让结果受输入 block 编号影响。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1743`
  `virtualizeExtraContinueEdges()` 增加 `MutableRegionGraphAnalysis` 参数。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1756`
  continue source 排序优先使用 `Analysis.NodeOrder`，只在顺序缺失或相同时回退到 block id。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2031`
  `reduceGraphNaturalLoopOnce()` 传入本轮 graph analysis，避免重新分析。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:316`
  `testRefineCyclicVirtualizesExtraContinues()` 增加 `FromBlock == 4` 断言，锁住当前按 graph order
  保留最后一条回头边的行为。

验证：

- `cmake --build ./build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。

当前判断：

- 这只是替换一处 block-id tie-break，没有实现完整 Angr quasi-topological sort。
- `MutableRegionGraphAnalysis::NodeOrder` 仍来自当前 DFS 后序，后续如果补更接近 Angr 的 region
  order，这里可以直接复用。
- 实现效果：3/10。减少一个和 Angr 不一致的局部决策。
- 复杂度：1/10。只改排序依据和一条测试断言。
- 维护成本：1/10。接口只多传已有分析结果。

# 2026-06-20 实现记录：区分 known 和 registered structurer

继续整理 Angr 式多算法入口。plan 里已经把 Dream 作为后续算法接入口，但当前代码不能把
`dream` 注册成可执行算法，否则会误导调用方以为 DREAM 已实现。这轮只把“已规划算法名”和
“当前可创建算法”分开。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructurerRegistry.h:15`
  新增 `knownStructurerNames()`。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:37`
  新增 `KnownStructurers`，包含 `goto`、`phoenix`、`sailr`、`dream`。
- `external/NotDec-llvm2c/lib/Structuring/StructurerRegistry.cpp:69`
  实现 `knownStructurerNames()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:170`
  `testStructurerRegistryNames()` 验证 `dream` 是 known name，但 `createStructurer("dream")`
  仍返回空。

验证：

- `cmake --build ./build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。

当前判断：

- 这不是 DREAM 实现，也不改变 CLI `--algo` 枚举。
- 后续如果实现 Dream，只需要把 factory 放进 `Structurers` 并开放对应 CLI/test。
- 实现效果：2/10。只是把多算法接入口边界补清楚。
- 复杂度：1/10。只加名字表。
- 维护成本：1/10。减少后续在文档、CLI、registry 之间重复写算法名的风险。

# 2026-06-20 实现记录：switch source 的 continue rewrite guard

继续补 Angr `_refine_cyclic_core()` 里 continue rewrite 的保守边界。Angr 在找不到能安全替换的
jump block 时，如果 source 是 jump table head，可以只 detach edge，不往 source 里插
`ContinueNode`。NotDec 当前没有完整 jump table 元数据，这轮先用“source 已经是结构化 switch
子树”作为保守判断：断掉额外回头边，但不替换 source 的 structured root。

修改内容：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1284`
  新增 `sourceContainsStructuredSwitch()`，判断 source 的 `StructuredRoot` 是否包含
  `StructuredNodeKind::Switch`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1795`
  `virtualizeExtraContinueEdges()` 遇到这种 source 时跳过 `buildVirtualizedSource()` 和
  `setStructuredRoot()`，但仍调用 `Graph.virtualizeEdge()` 断边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:327`
  新增 `testRefineCyclicSkipsSwitchSourceContinueRewrite()`，验证 switch source 会产生
  continue virtual edge，但原 structured root 保持不变。

验证：

- `cmake --build ./build --target structuring-analysis-test -j4`
  通过。
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
  通过。

当前判断：

- 这不是完整 jump-table case-head 识别，只是避免把 `continue` 错塞进已经结构化的 switch。
- 后续如果 `StructuredCFG` 携带 jump table / case-head 元数据，可以把判断从“含 switch 子树”
  收窄到精确的 case-head。
- 实现效果：3/10。补了 Angr continue rewrite 的一个安全分支。
- 复杂度：1/10。局部 guard。
- 维护成本：2/10。后续有精确信息时需要替换这个保守判断。

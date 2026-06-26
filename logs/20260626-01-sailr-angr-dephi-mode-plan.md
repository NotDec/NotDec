# 原始 prompt

> 详细规划一下到一个新的logs/文件，如何增加一个新的模式，关闭当前的Phi消除流程，改为实现Angr的这一套处理

# 背景

当前 NotDec 的 SAILR 复刻里，Phi 处理还是走旧链路：在结构恢复前先把 LLVM Phi demote 掉，再让后面的 structuring 和 renderer 假设 Phi 不存在。这个做法能跑，但它把“变量合流”和“结构恢复”绑死了，copied / virtual block 的值来源、payload rewrite、重复边合流这些语义都不够清楚。

angr 的做法不是在算法里硬删 Phi，而是保留 SSA 形态到后面，再用一层 dephication 把 `phi -> vvar` 的关系、复制出来的变量、以及需要重写的 block 一起处理掉。这样 structuring 算法只管结构，不直接背 Phi 细节。

# 目标

新增一个独立模式，切换后关闭当前的 Phi 预消除流程，改成按 angr 的思路处理：

1. structuring 阶段不直接依赖 LLVM Phi 消除结果。
2. Phi / vvar 的合流关系在 shared 层统一收集、统一重写。
3. copied / virtual block 的身份、来源、payload、边关系在 shared CFG 里表达，不靠 renderer 猜。
4. C 和 Solidity 后端只消费 shared 结果，不参与 Phi 语义决策。

# 现状

现在已经有的东西：

- `demoteSSAFixHT()` 会在结构恢复前先 demote Phi，并把 `HType` 迁到 demoted LLVM Value 上。
- shared structuring 里已经有 copied block、rollback、materialize、trial、quality guard 这些边界。
- `StructuredCFG` 已经能表达一部分 copied / synthetic block 身份。

现在缺的不是“再加一个 renderer 特判”，而是一个和 angr 接近的 shared dephication 层。它要能回答三个问题：

- 哪些值是同一个逻辑变量的不同版本。
- 哪些地方必须复制变量，不能直接合并。
- copied block 的 payload 和边关系该怎么重写，才能保持语义。

# 路线

## 阶段一：把 Phi 处理做成可切换模式

先加一个明确的模式开关，默认保留旧流程，新增一个 angr 风格模式。

要做：

- 把当前 `demoteSSAFixHT()` 这条前置 Phi 消除链路收口成一个单独 mode。
- 在新 mode 下，不再把“Phi 已经消失”当成 structuring 前提。
- 让 structuring 算法层只依赖 shared dephication 结果，不直接依赖 LLVM Phi erase 的副作用。

判断标准：

- 两种模式都能跑。
- 旧模式行为不变。
- 新模式不会偷偷落回 renderer 特判。

## 阶段二：补 shared dephication / vvar 映射层

参考 angr 的 `GraphDephicationVVarMapping` 思路，在 shared 层新增一层变量映射和复制决策。

要做：

- 收集 `phi dst -> source vvar` 的关系。
- 结合 liveness / interference / predecessor 关系判断能不能共用变量。
- 必要时创建 copied vvar，避免把不同前驱来的值硬塞到同一个身份里。
- 把 `copied_var_ids`、`vvar_to_vvar`、body rewrite 信息暴露给后续 shared 流程。

判断标准：

- 一个最小合流图能在 shared 层复现 angr 那种“去 phi 但保留变量身份”的效果。
- copied 变量和原变量不会混名。
- 不需要 C / Solidity renderer 参与判断。

## 阶段三：把 payload rewrite 挪到 shared materialize

copied block 不能只复用原 body。要让它在 shared 层按来源和复制关系生成新 payload。

要做：

- 把 `materializeBlockBody()` 扩成真正的 rewrite 入口。
- 让它能处理 Phi / vvar 相关的 payload 迁移。
- copied block 的 body、case 值、goto source-target、predecessor 绑定都按 copy 身份生成。

判断标准：

- 同一个原 block 被复制后，两个 copy 可以有不同的值来源。
- payload rewrite 和 CFG 身份不打架。
- rollback 后不会留下半改的 payload。

## 阶段四：接入 structuring / variable recovery 边界

新模式下，dephication 不应混到 renderer 里，而应在 structuring 结果稳定后、变量恢复前后按 shared 边界接入。

要做：

- 明确 shared dephication 放在什么阶段执行。
- 让变量恢复读取 `vvar_to_vvar` 这类映射，而不是默认假设 Phi 已被物理删除。
- copied block、virtual edge、switch / goto 相关重写都走同一套 trial / guard。

判断标准：

- 结构恢复层不再直接依赖“Phi 已经被删掉”这个隐含前提。
- 变量恢复能消费 shared 映射结果。
- shared trial 可以拒绝坏的复制和重写。

## 阶段五：补测试和对照

先做小样例，再看复杂 case。

要做：

- 加最小 Phi 合流样例。
- 加 copied block 需要不同来源值的样例。
- 加 rollback 失败样例。
- 加 ang r 风格 dephication 和旧 demote 模式的对照样例。

判断标准：

- 新模式下样例语义稳定。
- 旧模式和新模式差异可解释。
- 没有把问题藏进后端 fallback。

# 风险

最大风险是层次切错：

- 如果只是把 Phi 延后删除，但没补 vvar 映射，最后还是会卡在 copied block 和 payload rewrite。
- 如果把 dephication 做成 renderer 侧特判，就会把 C / Solidity 绑进算法语义，后面很难收。
- 如果 shared CFG 里没有独立的 copy 身份，structuring 结果看起来能过，但值来源会错。

# 明确完成条件

## 功能完成条件

1. 新 mode 可以单独开启，旧的 Phi demote 流程可以单独关闭。
2. structuring 算法层不再直接处理“Phi 已经被删”的前提。
3. shared 层能表达 `phi -> vvar` 映射、copied vvar、copied block 身份和来源。
4. payload rewrite 在 shared 层完成，不依赖 renderer 特判。
5. C 和 Solidity 都消费同一份 shared 结果。

## 语义完成条件

1. 一个 Phi 合流点在新模式下能被正确拆成普通变量关系。
2. copied block 的值来源和边关系在复制后仍然正确。
3. 复杂 region 回滚后不会留下半套映射或半改 payload。
4. 新模式下的结果能解释成 angr 风格 dephication，而不是旧 demote 逻辑的变体。

## 测试完成条件

1. 有覆盖 Phi 合流、copied vvar、payload rewrite、rollback 的单元测试。
2. 有至少一个新模式和旧模式的同样例对照测试。
3. 结构恢复和后端渲染都能通过，不靠 renderer fallback 兜底。

## 2026-06-26 实现记录：SAILR angr dephication 最小入口

这轮先完成阶段一的一部分，并补了一个最小 Phi 合流样例。新模式还不是完整
`GraphDephicationVVarMapping`，但已经不再靠 `reg2mem` 先删 Phi。

改动：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:22`：新增
  `SAILRDephicationMode`，默认 `LegacyDemoteSSA`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Commandlines.def:57`：新增
  `--sailr-dephication-mode=legacy|angr`，并写入 `Options`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1656`：
  `decompileModule()` 只在 legacy 模式跑旧的 `demoteSSAFixHT()` / `demoteSSA()`。
- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:397`：
  新增 `PendingPhiRewrite`，记录 Phi、目标 block、LLVM incoming block 和
  incoming 下标。
- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:697`：
  `CFGBuilder::visitPHINode()` 在 angr 模式下为 Phi 建局部变量，并登记每条
  incoming 边。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1136`：
  新增 `SAFuncContext::registerPhiRewrite()` 和
  `SAFuncContext::materializePhiRewrites()`，在 CFG 建边后给每条 Phi incoming
  插入一个小 CFG edge block，payload 是 `phi_var = incoming_value`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:2211`：
  `SAFuncContext::run()` 在 CFG 建边后、CFGCleaner 前 materialize Phi rewrite。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:277`：
  新增 `sailr_angr_dephication_phi`，用
  `--sailr-dephication-mode=angr` 验证最小 Phi 合流输出包含 `int x;`、
  `x = a;`、`x = b;`、`return x;`，且不出现 `phi` / `reg2mem`。

验证：

```bash
cmake --build build --target notdec-llvm2c -j4
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

当前完成度：

- 新 mode 可以单独开启，旧 demote 路线仍是默认。
- 最小 Phi 合流样例在 angr 模式下能先降成普通变量赋值。
- 还没完成 shared vvar 映射、copied vvar、复杂 copied block payload rewrite、
  rollback 对照和 Solidity 共用验证。

## 2026-06-26 实现记录：shared CFG Phi edge payload

这轮把上一轮只在 C CFG 侧做的 Phi incoming 赋值，推进到 shared
`LLVMFunctionCFGBuilder`。现在 shared CFG 能在 `pred -> merge` 边上插入
SAILR dephication synthetic edge block，payload 仍由后端提供。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/LLVMFunctionCFGBuilder.h:14`：
  forward declare `llvm::PHINode`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/LLVMFunctionCFGBuilder.h:35`：
  `PayloadProvider` 新增 `getPhiAssignment()`，后端只负责生成 assignment
  payload。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:52`：
  `CFGBlockCreator` 新增 `SAILRDephication`，标记这类 synthetic edge block。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:17`：
  新增 `valueName()`，给 unnamed value 和常量提供 shared fallback 名字。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:83`：
  `LLVMFunctionCFGBuilder::build()` 收集每个 merge block 开头的 Phi incoming，
  按 `(pred, merge)` 聚合 assignment payload。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:114`：
  为每条含 Phi assignment 的边创建 synthetic forwarder block，并用
  `StructuredCFG::replaceEdge()` 把原边改到 edge block。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:277`：
  Solidity payload provider 实现 `getPhiAssignment()`，当前格式是
  `phi = incoming;`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:455`：
  新增 shared Phi 测试辅助函数和字符串 payload provider。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1421`：
  新增 `testLLVMFunctionCFGBuilderMaterializesPhiEdgePayloads()`，验证 shared
  CFG 插入 `SAILRDephication` edge block，且 payload 是 `x = a;` /
  `x = b;`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1455`：
  新增 `testSolidityBodyBuilderReadsSharedPhiAssignments()`，验证 Solidity
  `BodyBuilder::readBody()` 能读到 shared Phi assignment。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.49`。

当前完成度：

- shared CFG 已能表达最小 Phi incoming assignment，不再只靠 C renderer 侧
  临时 CFG。
- Solidity 已通过 shared `LLVMFunctionCFGBuilder` 消费同一份 edge payload。
- C 后端仍保留上一轮的临时 C CFG Phi rewrite，尚未整体迁到 shared builder。
- 还没完成 shared vvar 映射、copied vvar、复杂 copied block payload rewrite
  和 rollback 失败样例。

评分：

- 实现效果：6/10。最小 shared Phi edge payload 打通了，但还不是完整
  `GraphDephicationVVarMapping`。
- 复杂度：5/10。新增回调和 edge block 较直接，但 C/Solidity 暂时存在两条
  Phi rewrite 路线，会增加理解成本。
- 维护成本：5/10。后续应把 C 后端也迁到 shared CFG，避免长期双路径。

## 2026-06-26 实现记录：最小 shared vvar 映射

这轮补了 shared dephication 的最小数据层。它还不做 liveness /
interference，也不创建 copied vvar；先让 shared CFG 能明确记录
`phi -> vvar` 和每条 incoming assignment 属于哪条 edge block。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:18`：
  新增 `VVarId` 和 `InvalidVVarId`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:116`：
  新增 `DephicationVVar` / `DephicationIncoming`，记录 Phi 目标 vvar、
  merge block、incoming block、edge block、assignment payload 和 incoming
  名字。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:204`：
  `StructuredCFG` 暴露 `dephicationVVars()` /
  `dephicationIncomings()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:189`：
  新增 `addDephicationVVar()` 和 `addDephicationIncoming()`。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:17`：
  新增 `PendingPhiAssignment`，在 builder 内暂存 vvar、payload 和
  incoming 名字。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:104`：
  每个 Phi 先登记一个 shared vvar，再把 incoming assignment 聚合到
  `(pred, merge)`。
- `external/NotDec-llvm2c/lib/Structuring/LLVMFunctionCFGBuilder.cpp:144`：
  synthetic edge block 创建后，把 incoming 映射登记到 `StructuredCFG`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1454`：
  扩展 `testLLVMFunctionCFGBuilderMaterializesPhiEdgePayloads()`，验证 `x`
  这个 vvar 和来自 `a` / `b` 的 incoming 映射都在 shared CFG 里。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.50`。

当前完成度：

- shared CFG 已有最小 `phi -> vvar` 和 incoming assignment 映射。
- 这一步只记录数据，不做变量合并、干扰判断或 copied vvar。
- 后续需要让 materialize / rollback 消费这份映射，而不是只把它当测试数据。

评分：

- 实现效果：5/10。补上了 shared vvar 表的入口，但还没进入 copy-vvar 和
  rewrite 决策。
- 复杂度：4/10。数据结构很小，放在 shared CFG 里，暂时没有影响现有 pass。
- 维护成本：4/10。后续要小心让 copy/materialize 使用这份表，避免它变成旁路
  metadata。

## 2026-06-26 实现记录：shared dephication metadata 随 copy/rollback 走

这轮继续补 shared 层的稳定性，不做完整 copied vvar 语义，只让
`DephicationIncoming` 能跟着 `duplicateRegion()`、`materializeBlockBody()` 和
`removeBlock()` 一起更新或回滚，避免 dephication metadata 留悬空引用。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:214`：
  新增 `duplicateDephicationIncomings()`、
  `rewriteCopiedDephicationIncomings()`、
  `rewriteDephicationIncomingAssignments()` 和
  `removeDephicationBlockReferences()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:152`：
  `duplicateBlock()` 在 `addBlock()` 后复制 source edge 上的 dephication incoming
  记录。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:531`：
  `duplicateRegion()` 在 copy 的 block 全部就位后，重写 copied edge 的
  `IncomingBlock` / `MergeBlock`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:560`：
  `removeBlock()` / `removeBlocks()` 成功后同步清理 dephication incoming。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:629`：
  新增 `rewriteCopiedDephicationIncomings()` 和
  `rewriteDephicationIncomingAssignments()` 的实现。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2092`：
  新增 `testStructuredCFGDuplicateDephicationEdgeCopiesMetadata()`，验证
  copy edge 记录、materialize 后 assignment 重写。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2146`：
  新增 `testStructuredCFGRemoveBlockMaintainsDephicationMetadata()`，验证失败回滚
  不污染 metadata，成功删除会清掉悬空记录。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.52`。

当前完成度：

- shared dephication metadata 已经能跟着 copy / materialize / remove 走。
- 还没做真正的 copied vvar 选择，也没把映射接到 structuring 的变量恢复决策。
- 这轮只是在 shared 层把半套 metadata 的风险压住，避免 rollback 后留下悬空记录。

评分：

- 实现效果：6/10。至少 copy / rollback 现在不会把 dephication metadata 搞坏。
- 复杂度：5/10。增加了几条 helper，但都在 `StructuredCFG` 内，语义边界还算清楚。
- 维护成本：5/10。后面要把复制规则继续收紧，避免 metadata 复制和 block 复制出现双轨。

## 2026-06-26 实现记录：edge-scoped dephication materialize context

这轮把 shared dephication 映射继续往下收口。现在 `PayloadMaterializeContext`
不再拿整张表，而是只带当前 edge block 相关的 `DephicationIncoming` 和对应
`DephicationVVar`，这样 copied block 的 payload rewrite 只消费自己这条边的
shared 结果。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:88`：
  `DephicationIncoming` 新增 `SourceIncomingBlock` / `SourceMergeBlock` /
  `SourceEdgeBlock`，把 provenance 和当前位置拆开。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:100`：
  `PayloadMaterializeContext` 新增 `DephicationVVars` /
  `DephicationIncomings`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:200`：
  `addDephicationIncoming()` 初始化 provenance 字段。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:300`：
  `materializeBlockBodyImpl()` 改成只把当前 edge 的 dephication 子集传给 hook。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:637`：
  `duplicateDephicationIncomings()` / `rewriteCopiedDephicationIncomings()` 按
  provenance 规则更新 copied 记录。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:719`：
  新增 edge 过滤和 vvar 收集 helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2109`：
  扩展 dephication edge copy 测试，验证 copied 记录保留 source provenance，
  且 materialize hook 只看到当前 edge 的 shared 映射。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.49`。

当前完成度：

- shared dephication 记录现在有 source/当前位置两层语义。
- materialize hook 只消费当前 edge 的映射，不再拿整张表。
- 还没把这套规则接到真正的 copy-vvar 选择和变量恢复里。

评分：

- 实现效果：6/10。copy edge 的 provenance 现在不打架了。
- 复杂度：6/10。字段和 helper 多了一层，但语义比前面清楚。
- 维护成本：5/10。下一步要把这份 edge-scoped context 真正用到变量恢复，而不是只停在 materialize hook。

## 2026-06-26 实现记录：去掉多余的 vvar 来源字段

这轮没有推进到真正的 copied vvar 选择，只是把前面一版里多出来的
`DephicationVVar::SourceId` 收掉了，保留 `SourceMergeBlock` 作为 provenance。
这样 shared 变量身份还是单一的，不会再假装同一个 vvar 有两套来源。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:82`：
  删掉 `DephicationVVar::SourceId`，只保留 `SourceMergeBlock`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:192`：
  `addDephicationVVar()` 不再初始化 `SourceId`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:643`：
  `duplicateDephicationIncomings()` 只复制 incoming provenance，不再尝试生成
  一个假的 vvar 来源身份。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:660`：
  `rewriteCopiedDephicationIncomings()` 仍只更新 copied edge 上的当前块位置。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2109`：
  维持 copied edge 和 materialize hook 的检查，证明 provenance 收缩后现有
  shared 边 materialize 仍然稳定。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.52`。

当前完成度：

- shared dephication 里暂时只保留一个 vvar 身份，再加一个 merge provenance。
- 没有推进 copy-vvar 选择，也没有进入变量恢复决策。
- 这轮是在把前面试出来的歧义收掉，避免后面继续分叉。

评分：

- 实现效果：4/10。只是把多余身份收掉，没有增加新能力。
- 复杂度：3/10。删除字段比加字段更干净。
- 维护成本：3/10。后面实现 copy-vvar 时不会被旧的 source id 误导。

## 2026-06-26 实现记录：复制 region 时同步复制 shared vvar

这轮继续往前接 shared dephication 的真实消费点。现在 `duplicateRegion()`
不只复制 edge 上的 incoming 记录，还会给被复制的 merge block 生成一份新的
`DephicationVVar`，让 copied edge 的 materialize context 能拿到自己的 vvar
身份，而不是一直沿用原 merge 的那份记录。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:253`：
  新增 `duplicateDephicationVVars()` 和
  `rewriteCopiedDephicationIncomingTargets()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:544`：
  `duplicateRegion()` 现在会先复制 shared vvar，再重写 copied incoming 的
  vvar 目标。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:664`：
  新增 `duplicateDephicationVVars()` 和
  `rewriteCopiedDephicationIncomingTargets()` 的实现。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2109`：
  扩展 `testStructuredCFGDuplicateDephicationEdgeCopiesMetadata()`，验证复制
  merge 后会多出一份新的 shared vvar，且 copied edge 的 materialize context
  看到的是 copied vvar，不再是原来的 vvar。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.50`。

当前完成度：

- shared dephication 的 edge copy 现在能把 incoming、merge 和 vvar 一起带过去。
- copied edge 的 materialize context 不再只看到原始 vvar。
- 还没把这条 copied vvar 直接接到更高层的变量恢复判断里。

评分：

- 实现效果：6/10。shared copy 这一步终于不是只复制边记录。
- 复杂度：5/10。多了一个 copied vvar 生成点，但还算在 `StructuredCFG` 内部。
- 维护成本：5/10。后面接变量恢复时，能直接读这份 copied vvar 表。

## 2026-06-26 实现记录：删除 copied merge 时退役 shared vvar

这轮把 copied vvar 的回滚补上了。现在如果复制出来的 merge block 被删除，
对应的 `DephicationVVar` 不会被硬删掉，也不会继续作为活动身份参与后续
materialize，而是标成 `Retired`。这样 shared vvar 表能保留稳定 id，回滚时
也不会留下还在活动的脏身份。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:85`：
  `DephicationVVar` 新增 `Retired` 标记。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:262`：
  新增 `removeDephicationVVarReferences()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:577`：
  `removeBlock()` / `removeBlocks()` 成功后同步带回 `DephicationVVars`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:625`：
  `removeBlockInPlace()` 删除 block 时顺手退役对应 merge 的 vvar。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:698`：
  新增 `removeDephicationVVarReferences()`，只给命中的 vvar 打退役标记。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:747`：
  `dephicationVVarsForIncomings()` 跳过已退役的 vvar。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2167`：
  新增 `testStructuredCFGRemoveCopiedDephicationMergeRetiresVVar()`，验证删掉
  copied merge 后原始 vvar 仍在，copied vvar 退役，活动 incoming 只剩原始那份。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.53`。

当前完成度：

- copied vvar 的生成和回滚现在都接上了。
- remove 路径不会把 vvar id 打乱，只会把失效的那条标退役。
- 还没把这套 copied vvar 结果继续往更上层的变量恢复决策里接。

评分：

- 实现效果：6/10。回滚不再漏掉 copied vvar。
- 复杂度：5/10。只加了一个退役标记，没有改 id 体系。
- 维护成本：5/10。后续变量恢复只要忽略 retired 就行。

## 2026-06-26 实现记录：edge materialize 直接带 copied vvar 映射

这轮没有再往 `DephicationVVar` 里塞新的身份字段，而是把 copied edge 对应的
`original vvar -> copied vvar` 映射直接放进 `PayloadMaterializeContext`。这样
shared 层仍然只保留一份原始 vvar 表和一份 copied vvar 表，但边级 materialize
已经能直接读到自己这条边的复制关系，后面变量恢复或 payload rewrite 不需要
再从别的地方猜。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:109`：
  `PayloadMaterializeContext` 新增 `DephicationVVarCopies`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:264`：
  新增 `dephicationVVarCopiesForIncomings()` 声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:304` 和 `:372`：
  `materializeBlockBodyImpl()` 为当前 edge 计算 `DephicationVVarCopies`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:811`：
  新增 `dephicationVVarCopiesForIncomings()`，按当前 edge 的 incoming 收集
  原 vvar 与 copied vvar 的对应关系。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2147`：
  扩展 copied edge 测试，验证 materialize context 里能看到 `VVar -> CopyVVar`
  的映射。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.60`。

当前完成度：

- copied edge 的 materialize 现在能直接拿到自己的 vvar 映射。
- shared 层仍然保留原始 vvar 和 copied vvar 两张表，没有再引入第三套身份。
- 还没把这份映射接到更上层的变量恢复判断或额外 rewrite 逻辑里。

评分：

- 实现效果：7/10。edge 级复制关系终于能直接读，不再靠猜。
- 复杂度：5/10。多了一张 edge 级映射表，但还是 shared 层内的东西。
- 维护成本：5/10。后续变量恢复能直接消费这张表，不必再拆字段。

## 2026-06-26 实现记录：copied vvar 映射按 source target 对齐

这轮修正了上一轮 edge 级 copied vvar 映射的隐患。之前映射是从 merge block
反推原 vvar 和 copied vvar，单 Phi 没问题，但同一个 merge 里有多个 Phi 时会串。
现在 `DephicationIncoming` 记录 `SourceTarget`，复制 edge 后仍保留原 vvar id，
materialize context 里的 `DephicationVVarCopies` 直接按
`SourceTarget -> Target` 生成，不再靠 merge block 猜。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:91`：
  `DephicationIncoming` 新增 `SourceTarget`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:207`：
  `addDephicationIncoming()` 初始化 `SourceTarget`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:657`：
  `duplicateDephicationIncomings()` 保留 copied edge 的原始 vvar provenance。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:730`：
  copied incoming 重写 target 时按 `SourceTarget` 查 copied vvar。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:812`：
  `dephicationVVarCopiesForIncomings()` 直接返回 `SourceTarget -> Target`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:509`：
  shared Phi 测试改成同一个 merge 里两个 Phi。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1464`：
  验证两个 Phi 的 vvar 和 incoming 都分别记录 source target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2149`：
  手写 copied edge 测试验证原 incoming 和 copied incoming 的 `SourceTarget`。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.58`。

当前完成度：

- copied vvar 映射已经能区分同一个 merge 里的多个 Phi。
- edge materialize context 不再靠 merge block 反推 copied vvar。
- 还没把这张映射接到更上层的变量恢复或真实 payload rewrite 决策。

评分：

- 实现效果：7/10。修掉了多 Phi 同 merge 的明显歧义。
- 复杂度：5/10。多了一个 provenance 字段，但语义直接。
- 维护成本：5/10。后续消费映射时可以按 vvar id 对齐，不必按 block 猜。

## 2026-06-26 实现记录：shared materialize 标记 dephication assignment

这轮把 copied vvar 映射往真实 payload rewrite 决策再推进一步。现在
`materializeBlockBodyImpl()` 会识别当前 edge 上的 dephication assignment，并用
新的 `PayloadMaterializeKind::DephicationAssignment` 调用 payload hook。这样后端
hook 仍然只读 shared context，但已经能区分“普通 statement”和“Phi dephication
assignment”，不需要自己根据 Phi 或 block 关系猜。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:69`：
  `PayloadMaterializeKind` 新增 `DephicationAssignment`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:40`：
  新增 `isDephicationAssignment()`，只用当前 edge 的
  `DephicationIncomings` 判断 payload 是否是 dephication assignment。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:400`：
  statement materialize 时，如果 payload 是当前 edge 的 assignment，就用
  `DephicationAssignment` 调 hook，否则仍用普通 `Statement`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2178`：
  copied edge 测试验证 hook 能看到 `DephicationAssignment`，并据此生成新的
  assignment payload。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.54`。

当前完成度：

- shared materialize 已经真正消费 dephication incoming，能给 hook 明确的
  assignment rewrite 信号。
- 后端不需要参与 Phi 语义判断，只按 shared 给出的 kind 和 context 处理 payload。
- 还没把 C 后端的临时 Phi rewrite 整体迁到 shared builder。

评分：

- 实现效果：7/10。payload rewrite 终于有了 shared 层的真实入口。
- 复杂度：4/10。只是新增一个 materialize kind 和一个局部判断。
- 维护成本：4/10。后续 C/Solidity 可以沿用同一 kind，不用再加 renderer 特判。

## 2026-06-26 实现记录：C Phi edge metadata 接入 shared CFG

这轮把 C 路径里临时生成的 Phi incoming edge block 标成 shared 能识别的
SAILR dephication synthetic edge，并修掉同一条 LLVM incoming 边上多个 Phi 时会
重复替换原 CFG 边的问题。现在 C CFG 仍然负责先生成 Clang assignment payload，
但进入 `StructuredGotoAdapter` 后，这些 edge block 会带上 shared 的
`CreatedBy = SAILRDephication`、source/target 信息，后续 structuring 可以按
shared block 身份处理。

改动：

- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:212`：
  `CFGBlock` 新增 `IsSAILRDephicationEdge`、source block、target block 元数据。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:441`：
  新增 `isSAILRDephicationEdge()`、source/target getter 和
  `setSAILRDephicationEdge()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1149`：
  `SAFuncContext::materializePhiRewrites()` 改成按
  `(incoming CFG block, merge CFG block)` 分组，一条边只创建一个 synthetic edge
  block，同边多个 Phi assignment 按顺序放进同一个 block。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1196`：
  新 edge block 调 `setSAILRDephicationEdge()` 保存原始 incoming/merge 身份。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:127`：
  `StructuredGotoAdapter::buildCFG()` 把 C CFG 的 dephication edge 元数据转成
  shared `CFGBlockOrigin::Synthetic`、`SyntheticForwarder` 和
  `CFGBlockCreator::SAILRDephication`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:299`：
  新增 `sailr_angr_dephication_multi_phi_same_edge`，覆盖同一个 merge 里两个 Phi
  共享 incoming edge 的情况。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.61`，和前几次 `2.54`、`2.58`、
`2.60` 同口径，未看到明显回退。

当前完成度：

- C angr dephication 模式下，同边多个 Phi 不再创建互相断开的 edge block。
- C CFG 生成的 Phi assignment edge 已经带 shared synthetic/dephication 身份。
- 这还不是完整迁移：C 路径仍然先在 `SAFuncContext::materializePhiRewrites()` 里
  生成 edge block，后续还需要继续把 vvar 映射和 copied payload rewrite 更多地挪到
  shared builder / shared materialize。

评分：

- 实现效果：7/10。修掉了多 Phi 同边的真实 CFG 问题，也让 C edge 能被 shared 层识别。
- 复杂度：5/10。只加了小块 metadata 和分组逻辑，没有重写 C CFG 构造。
- 维护成本：5/10。短期仍有 C 侧临时 edge block，但 shared adapter 已经有明确入口。

## 2026-06-26 实现记录：C Phi edge 生成 shared vvar/incoming 表

这轮把 C 路径的 Phi edge metadata 往 shared dephication 表再推进一步。之前
`StructuredGotoAdapter` 只能把 C CFG 里的 Phi edge block 标成
`SAILRDephication` synthetic block，但 shared CFG 里还没有对应的
`DephicationVVar` / `DephicationIncoming`。现在 C CFG edge block 会记录每条
assignment 对应的目标变量名和 incoming 名，adapter 转 shared CFG 时用这些记录
创建 shared vvar 和 incoming。

改动：

- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:216`：
  `CFGBlock` 新增 `SAILRDephicationAssignment`，记录 statement index、target
  name、incoming name。
- `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:456`：
  新增 `getSAILRDephicationAssignments()` 和
  `addSAILRDephicationAssignment()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:96`：
  新增 `sharedValueName()`，给 C edge metadata 取稳定 incoming 名；常量直接用
  文本，其他 unnamed value 暂用 `incoming`，不推进正常 temp 计数。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1206`：
  `SAFuncContext::materializePhiRewrites()` 在 append assignment statement 时同步
  记录 dephication assignment metadata。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:122`：
  `StructuredGotoAdapter::buildCFG()` 新增局部 vvar 表，按 `(merge, target name)`
  复用 shared vvar。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:179`：
  转换 C dephication edge block 后，按 assignment metadata 调
  `StructuredCFG::addDephicationVVar()` 和 `addDephicationIncoming()`。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.58`，和前几次 `2.54`、`2.58`、
`2.60`、`2.61` 同口径，未看到明显回退。

当前完成度：

- C edge 现在不只带 synthetic/source/target 身份，也会在 shared CFG 里生成
  vvar 和 incoming 表。
- 后续 shared materialize/copy 路径能看到 C 路径生成的 dephication incoming。
- C 路径仍然先在 `SAFuncContext::materializePhiRewrites()` 里生成 Clang
  assignment payload，后续还需要继续减少这部分 C 侧临时逻辑。

评分：

- 实现效果：7/10。C 路径开始真正接入 shared vvar/incoming 表。
- 复杂度：5/10。增加了 edge metadata 传递，但没有扩大到 renderer 语义判断。
- 维护成本：5/10。metadata 字段还在 C CFG 上，后续迁到 shared builder 后可以删除。

## 2026-06-26 实现记录：shared dephication edge context 查询

这轮给 shared 层补了一个稳定查询入口。之前 `DephicationIncomings`、
`DephicationVVars`、`DephicationVVarCopies` 只在 `materializeBlockBodyImpl()` 里
临时拼成 `PayloadMaterializeContext`，后续 pass 或变量恢复如果想读同一份关系，
只能自己重复拼。现在 `StructuredCFG::dephicationEdgeContext()` 可以按 edge block
直接返回当前 edge 的 incoming、活动 vvar 和 copied vvar 映射，materialize 本身也
改成复用这个接口。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:105`：
  新增 `DephicationEdgeContext`，包含 `VVarCopies`、`VVars`、`Incomings`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:228`：
  `StructuredCFG` 新增 `dephicationEdgeContext(BlockId)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`：
  实现 `StructuredCFG::dephicationEdgeContext()`，统一调用现有 edge incoming、
  copied vvar 映射和 active vvar 查询。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:322`：
  `materializeBlockBodyImpl()` 的 self-body 路径改成复用
  `dephicationEdgeContext()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:391`：
  copied-body materialize 路径同样复用 `dephicationEdgeContext()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2209`：
  新增 `testStructuredCFGQueriesDephicationEdgeContext()`，覆盖原 edge、copied edge
  和删除 copied merge 后的 retired/removed 状态。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.64`，和前几次 `2.54` 到 `2.61`
同口径，未看到明显回退。

当前完成度：

- shared dephication 关系现在有了独立只读入口，后续变量恢复或 structuring pass
  可以直接消费，不必靠 renderer 或 materialize hook 临时猜。
- materialize 的 context 拼装和外部查询共用同一套逻辑。
- 还没把变量恢复实际切到这个接口上。

评分：

- 实现效果：6/10。补上了消费入口，但还没接具体变量恢复 pass。
- 复杂度：3/10。只是公开已有组合关系，没有引入新算法。
- 维护成本：3/10。减少重复拼 context 的机会，接口语义比较窄。

## 2026-06-26 实现记录：避免 copied edge 伪造 copied vvar 映射

这轮修了一个 shared context 里的边界问题：如果只复制 dephication edge block，
没有复制对应 merge block，那么 copied edge 仍然指向原来的 vvar。这种情况下
`DephicationVVarCopies` 不应该暴露 `VVar -> 同一个 VVar`，否则后续变量恢复会误以为
真的创建了 copied vvar。

改动：

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:835`：
  `dephicationVVarCopiesForIncomings()` 现在跳过 `SourceTarget == Target` 的
  incoming，只在 copied edge 确实指向新 vvar 时返回映射。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2255`：
  新增 `testStructuredCFGQueriesCopiedDephicationEdgeWithoutCopiedMerge()`，覆盖只
  复制 incoming + edge、不复制 merge 的情况，确认 context 仍能看到原 vvar，但
  `VVarCopies` 为空。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.57`，同口径未看到明显回退。

当前完成度：

- copied edge 未复制 merge 时，不再产生假的 copied vvar 映射。
- dephication edge context 可以更可靠地被变量恢复或后续 pass 消费。
- 还没接入真正的变量恢复 pass。

评分：

- 实现效果：6/10。修掉一个会误导后续消费方的映射问题。
- 复杂度：2/10。只是过滤 self-copy 映射。
- 维护成本：2/10。语义更清楚，后续消费时少一个特殊判断。

## 2026-06-26 实现记录：materialize hook 暴露当前 dephication incoming

这轮把 materialize hook 从“知道这是 dephication assignment”，推进到“知道当前
assignment 对应哪条 incoming”。后续变量恢复或 payload rewrite 不需要再按 payload id
自己回查 `DephicationIncomings`。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:136`：
  `PayloadMaterializeContext` 新增 `CurrentDephicationIncoming`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:43`：
  新增 `dephicationAssignmentIncoming()`，按 assignment payload 找到对应 incoming。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:416`：
  `StructuredCFG::materializeBlockBodyImpl()` 调用 hook 前设置当前 incoming，调用后清空。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2195`：
  `testStructuredCFGDuplicateDephicationEdgeCopiesMetadata()` 覆盖 copied edge 的
  `CurrentDephicationIncoming`，确认 target 和 assignment 都是 copied 后的结果。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.60`，和前几次同口径结果接近，未看到明显回退。

当前完成度：

- materialize hook 可以直接消费当前 dephication incoming。
- shared 层对 payload rewrite 暴露的信息更完整。
- 变量恢复还没有实际切到 `dephicationEdgeContext()` / 当前 incoming 上。

评分：

- 实现效果：6/10。减少后续消费方重复匹配 incoming 的逻辑。
- 复杂度：2/10。只是在已有 context 里补当前 statement 的精确信息。
- 维护成本：2/10。接口语义窄，后续 hook 更容易写对。

## 2026-06-26 实现记录：copied merge materialize 暴露 vvar copy context

这轮补了 shared 层的 block 级 dephication context。之前 copied dephication edge
能拿到 `原 vvar -> copied vvar`，但 copied merge body materialize 时拿不到这份映射。
后续如果要把 merge body 里的 Phi 变量引用改到 copied vvar，就会被迫在后端猜。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:86`：
  `DephicationVVar` 新增 `SourceId`，记录 copied vvar 来源。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:233`：
  `StructuredCFG` 新增 `dephicationBlockContext(BlockId)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:207`：
  `addDephicationVVar()` 让原始 vvar 的 `SourceId` 指向自身。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:246`：
  实现 `dephicationBlockContext()`，按 merge block 返回 active vvar 和 copied vvar 映射。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:337`：
  `materializeBlockBodyImpl()` 在没有 edge incoming 时改用 block context。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:736`：
  `duplicateDephicationVVars()` 复制 vvar 时保留原始 `SourceId`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:876`：
  新增按 merge block 查询 vvar、按 vvar 来源生成 copy 映射的 helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2289`：
  新增 `testStructuredCFGMaterializeCopiedMergeReportsDephicationVVarCopy()`，
  覆盖 copied merge body 的 hook context 能看到 copied vvar 映射。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 smoke 耗时 `elapsed 2.60`，同口径未看到明显回退。

当前完成度：

- copied merge body materialize 可以消费 shared 层的 vvar copy 映射。
- 后端后续重写 copied merge body 变量引用时，不需要自己按名字猜变量来源。
- C / Solidity 还没有实际把 copied vvar 映射应用到 payload 变量引用上。

评分：

- 实现效果：6/10。补齐了 copied merge body 的 shared 消费信息。
- 复杂度：3/10。新增一个 block context，复用现有 dephication context 结构。
- 维护成本：3/10。多了 `SourceId` 字段，但减少了后端按名字匹配的风险。

## 2026-06-26 实现记录：C 后端消费 copied vvar rewrite

这轮把 C 后端从“只复制 Clang payload”推进到“按 shared dephication context
重写 DeclRef”。同时修了 copied switch region 下 assignment 和 merge body 不一致的问题：
copied merge body 已经读 `p_copy` 时，对应 dephication edge assignment 也会重新物化成
写 `p_copy`。

改动：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:29`：
  新增 `assignmentTargetVar()`，从 C dephication assignment 的 LHS 记录原始 vvar
  对应的 `VarDecl`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:40`：
  新增 `DeclRefRewriter`，只按 shared 给出的 vvar copy 映射替换 `DeclRefExpr`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:63`：
  新增 `DeclRefCollector`，最终只声明实际被使用的 copied vvar。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:86`：
  `StructuredGotoAdapter` 新增 `DephicationVarDecls` 和
  `CopiedDephicationDecls`，保存 shared vvar 到 C `VarDecl` 的映射。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:100`：
  C 路径的 `setPayloadMaterializeHook()` 改为调用 `materializePayload()`，并声明支持
  predecessor / grouped predecessor rewrite。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:133`：
  `materializePayload()` 从 `PayloadMaterializeContext` 读取
  `DephicationVVarCopies` 和 `CurrentDephicationIncoming`，再复制或重写 Clang
  payload。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:155`：
  `dephicationDeclReplacements()` 把 shared 的 `source vvar -> copied vvar`
  转成 C AST 的 `ValueDecl -> ValueDecl`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:200`：
  `copiedDephicationVarDecl()` 为 copied vvar 创建 `p_copyN` 这类局部变量。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:242`：
  `prependUsedCopiedDephicationDecls()` 在最终语句前补 copied vvar 声明。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:379`：
  `buildCFG()` 在读 C dephication edge assignment 时登记原始 vvar 的 `VarDecl`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:264`：
  暴露 `redirectDephicationIncomingTarget()`，供 SAILR deoptimization 的手写重定向路径调用。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:344`：
  `materializeBlockBodyImpl()` 允许 self-body 在 `BodyMaterialized=false` 且有 hook 时重新物化。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:619`：
  `redirectPredecessors()` 先更新 dephication incoming target，再替换 CFG 边，并保持失败回滚。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:794`：
  `redirectDephicationIncomingTarget()` 把原 edge incoming 指向 copied merge 的 copied vvar，
  并重新物化 edge assignment。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:911`：
  `dephicationVVarCopiesForIncomings()` 只按 `SourceTarget != Target` 判断 copied
  vvar，不再要求 edge block 必须也是 copy。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:501`：
  `redirectSwitchCases()` 在 switch case 重定向时同步 dephication incoming。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:596`：
  `replaceDefaultSwitchSuccessor()` 在 default successor 重定向时同步 dephication incoming。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:935`：
  `redirectNonSwitchCaseEdges()` 在非 case 边重定向时同步 dephication incoming。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2358`：
  新增 `testStructuredCFGRedirectDephicationEdgeRematerializesAssignment()`，覆盖
  dephication edge 指向 copied merge 后会重新物化 assignment。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:331`：
  新增 `sailr_angr_dephication_copied_switch_region`，覆盖 `p_copy1 = a;`
  和 `return p_copy1 + 1;` 必须一致。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。最后一次 structuring smoke 耗时 `elapsed 2.70`，同口径未看到明显回退。

额外手工检查的最小 IR 输出里，case1 已从错误的：

```c
p = a;
return p_copy1 + 1;
```

变为：

```c
p_copy1 = a;
return p_copy1 + 1;
```

当前完成度：

- C 后端已经消费 shared copied vvar 映射，不再自己判断 Phi 语义。
- copied merge body 和 dephication assignment 的 payload rewrite 已能保持一致。
- Solidity 路径仍是字符串 payload 复制 hook，本轮没有把 Solidity 也改成 copied vvar
  变量名重写。
- rollback 覆盖仍主要来自 shared materialize / duplicate 既有测试，还缺更复杂的
  angr dephication 失败样例。

评分：

- 实现效果：7/10。修掉 copied switch region 下最明显的 vvar 读写不一致问题。
- 复杂度：5/10。C AST rewrite 逻辑增加了一些代码，但语义判断仍放在 shared context。
- 维护成本：4/10。后续如果 Solidity 也要变量名级 rewrite，可以复用 shared context，
  但 C 侧 `VarDecl` 映射还需要继续保持窄边界。

## 2026-06-26 实现记录：Solidity 字符串 payload 消费 copied vvar 映射

这轮把 Solidity `BodyBuilder` 的 materialize hook 从纯字符串复制，推进到读取
shared dephication copied vvar 映射后再改写 payload 文本。它仍不判断 Phi 语义，只把
shared 给出的 `source vvar -> copied vvar` 转成 Solidity fallback 字符串里的完整标识符替换。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Solidity/BodyBuilder.h:31`：
  暴露 `BodyBuilder::rewriteCopiedDephicationVVars()`，方便测试字符串层完整标识符替换。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:39`：
  新增 `isIdentifierChar()`，把字母数字和 `_` 作为 Solidity fallback 标识符边界。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:43`：
  新增 `replaceIdentifier()`，只替换完整标识符，避免把 `panic` 或 `p_copy0`
  里的 `p` 误改掉。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:64`：
  新增 `copiedVVarName()`，按 shared copied vvar id 生成 `p_copyN` 这类名字。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:324`：
  `readBody()` 从 `StructuredCFG::dephicationVVars()` 收集 shared vvar 名字。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:331`：
  Solidity materialize hook 读取 `PayloadMaterializeContext::DephicationVVarCopies`，
  转成字符串 rewrite 规则，并声明支持 predecessor / grouped predecessor rewrite。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:366`：
  实现 `rewriteCopiedDephicationVVars()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1517`：
  新增 `testSolidityBodyBuilderRewritesCopiedDephicationVVars()`，覆盖 assignment、
  condition 以及完整标识符边界。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-solidity-vvar.ll --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 2.69`。
- EVM 25928 smoke 通过，耗时 `elapsed=17.79 rss_kb=958624`，同近期同口径记录接近。

当前完成度：

- Solidity fallback payload 现在也能消费 shared copied vvar 映射。
- C / Solidity 两个后端都不再需要自己判断 copied vvar 语义。
- 当前 Solidity 测试覆盖字符串 rewrite 和 shared context 两段；还缺一个自然触发
  Solidity readBody copied region 的 LLVM IR 集成样例。

评分：

- 实现效果：6/10。Solidity 字符串 payload 已可按 shared copied vvar 映射重写。
- 复杂度：3/10。只做完整标识符替换，没有引入 Solidity AST 级表达式恢复。
- 维护成本：3/10。接口窄，但后续需要补更完整的 Solidity copied region 集成测试。

## 2026-06-26 实现记录：shared dephication redirect 失败回滚覆盖

这轮补了一个 shared 层回滚测试，专门覆盖 dephication edge 重定向到 copied
merge 时，如果 assignment 重新物化失败，不能留下半改的 incoming target、
synthetic target 或 payload。

改动：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2435`：
  新增 `testStructuredCFGRedirectDephicationEdgeFailureRollsBack()`。测试先构造
  `1 -> edge 4 -> merge 3` 的 dephication edge，再复制 merge，最后让
  `PayloadMaterializeKind::DephicationAssignment` 返回失败。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2475`：
  验证 `redirectDephicationIncomingTarget(4, 3, CopyMerge)` 返回失败后，
  edge body 仍是原 assignment 40，`SyntheticTarget` 仍指向原 merge 3。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2486`：
  验证 shared incoming metadata 回滚到原 merge 3、原 vvar、原 assignment 40。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:8896`：
  把新测试加入 `structuring-analysis-test` 主测试列表。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-solidity-vvar.ll --tr-level=2
```

结果：

- 构建通过，目标已是最新。
- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 2.72`。
- EVM 25928 smoke 通过，耗时 `elapsed=17.51 rss_kb=956800`。

当前完成度：

- shared dephication redirect 的失败回滚有了直接覆盖。
- 这能防止 copied merge 重定向时只改了一半 metadata 或 payload。
- 还缺 pass pipeline 里自然触发的失败样例。

评分：

- 实现效果：5/10。覆盖了一个关键失败分支，但还是直接调用 shared helper。
- 复杂度：2/10。只新增测试，没有改生产逻辑。
- 维护成本：2/10。测试构造较直接，后续 helper 行为变化时容易定位。

## 2026-06-26 实现记录：switch case goto source 归因修正

这轮修了 shared `GotoManager` 的 switch case goto source 归因。之前 case body 里的
goto 会被记到 switch header 上；在 dephication edge 插到 case 和 merge 中间后，
后续 SAILR pass 就可能找不到真正需要复制的 case 侧前驱。

改动：

- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:49`：
  收集 `StructuredSwitchCase` 里的 goto 时，把当前 source 切到 case target，
  不再沿用 switch header。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:853`：
  调整 `testGotoManagerCollectsSwitchGotoEdgeKinds()`，验证 default goto 仍归
  switch header，case goto 归 case target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3239`：
  `testCrossJumpReverterUsesSwitchCaseGotoKind()` 改成直接喂 case-kind goto，
  保留 CrossJumpReverter 对 switch header case 边的旧覆盖。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3892`：
  新增 `testReturnDuplicatorLowUsesSwitchCaseGotoSource()`，覆盖 case target 作为
  goto source 时，`ReturnDuplicatorLow` 可以复制共享 return region。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-solidity-vvar.ll --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 2.72`。
- EVM 25928 smoke 通过，耗时 `elapsed=17.61 rss_kb=959320`。

当前完成度：

- switch case body 里的 goto source 更贴近真实 CFG 入口。
- dephication edge 插入 case 和 merge 之间后，后续复制 pass 更容易拿到正确前驱。
- 自然触发 Solidity `BodyBuilder::readBody()` copied-region 的 LLVM IR 集成样例仍没补上：
  当前 switch + dephi 形状在完整 SAILR pipeline 里会被 quality guard 拒掉，不能硬写成
  已覆盖。

评分：

- 实现效果：6/10。修正了 shared goto 事实，推进了 dephication edge 后的复制判断。
- 复杂度：3/10。只改 goto source 归因和相关测试。
- 维护成本：3/10。语义更明确，但会影响依赖旧 switch-header source 的测试预期。

## 2026-06-26 实现记录：Solidity readBody copied dephication 集成覆盖

这轮补上了一个自然走 Solidity `BodyBuilder::readBody()` 的 copied dephication 样例。
问题点是 `ReturnDuplicatorLow` 复制 return region 后，最终 goto target 指到 copied
block，quality guard 会把它当成新的 goto 目标，从而拒掉这次复制。

改动：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:44`：
  `ReturnDuplicatorLow` 覆盖 `getNewGotos()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1478`：
  新增 `hasInitialSourceGoto()`，只在初始结果里已有同 source、同原 target 的 goto
  时允许归一化。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1494`：
  `ReturnDuplicatorLow::getNewGotos()` 把指向 copied block 的 goto target 归一化到
  copied block 的 `SourceBlock`，避免 quality guard 把等价 copied target 当成变差。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:283`：
  Solidity fallback `readBody()` 收集最小 `return value;` payload，使 copied merge body
  有可重写文本。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:558`：
  新增 `makeSharedPhiSwitchFunction()`，构造 switch case 共享 Phi return 的 LLVM 样例。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1568`：
  新增 `testSolidityBodyBuilderReadsCopiedSharedPhiAssignments()`，通过
  `BodyBuilder::readBody()` 验证自然输出里出现 copied assignment 和 `return p_copy...;`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4009`：
  新增 `testReturnDuplicatorLowNormalizesCopiedGotoTargets()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4034`：
  新增 `testReturnDuplicatorLowAcceptsCopiedSwitchCaseReturnRegion()`，覆盖默认 quality
  检查能接受 copied return region。

验证：

```bash
cmake --build build --target structuring-analysis-test notdec-llvm2c -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e rss_kb=%M' ./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-25928-solidity-vvar.ll --tr-level=2
```

结果：

- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 2.74`。
- EVM 25928 smoke 通过，耗时 `elapsed=17.45 rss_kb=960276`。

当前完成度：

- Solidity `readBody()` 现在有自然 copied-region 集成覆盖。
- copied Phi assignment 和 copied merge return 都能消费 shared copied vvar 映射。
- 还缺更复杂的 pass pipeline 失败回滚样例，以及新 mode / legacy mode 更系统的对照。

评分：

- 实现效果：7/10。补上了之前缺的 Solidity 自然集成覆盖。
- 复杂度：4/10。quality 归一化只限定在 ReturnDuplicatorLow 的 copied target。
- 维护成本：4/10。新增最小 return fallback 后，Solidity readBody 输出更完整，但后续还要避免把它扩成表达式恢复。

## 2026-06-26 实现记录：sailr dephication legacy/angr 对照 smoke

这轮没有再改 shared CFG 主逻辑，只补了一条同一份 IR 的 legacy / angr 对照
smoke。目的很简单：把“旧 Phi demote 还是默认路径”和“新 dephication 模式确实
走 shared vvar / copied block 输出”这两件事放在同一个输入上一起验一遍。

改动：

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:540`：
  新增 `run_sailr_dephication_mode_contrast_case()`，用同一份 Phi 合流 IR 分别跑
  `--sailr-dephication-mode=legacy` 和 `--sailr-dephication-mode=angr`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:581`：
  smoke 主流程里把这条对照 case 接上。

验证：

```bash
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：

- 对照 smoke 通过。
- legacy 输出还是直接 `return a;` / `return b;`。
- angr 输出是 `int x;`、`x = a;`、`x = b;`、`return x;`。

当前完成度：

- 旧模式和新模式在同一份 IR 上已经能直接对照。
- 这条 smoke 只覆盖最小 Phi 合流，不代表 shared 层复制和 rollback 全部都补齐。

评分：

- 实现效果：5/10。补的是对照证据，不是新语义。
- 复杂度：2/10。只加了一条 smoke 分支。
- 维护成本：2/10。以后可以直接拿这条 case 看 mode 切换有没有回退。

## 2026-06-26 实现记录：copied switch dephication 模式对照

这轮把对照 smoke 再往前挪了一步，从普通 Phi 合流换成了 copied switch region。
原因很直接：当前目标不是证明“angr 模式能跑一个最小合流”，而是证明
shared vvar / copied block 这套结果在一个已经复制过的 region 里，和旧的
`reg2mem` 路径能明确区分开。

改动：

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:540`：
  新增 `run_sailr_dephication_copied_switch_contrast_case()`，对同一份 copied switch
  IR 分别跑 `legacy` 和 `angr`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:597`：
  smoke 主流程把 copied switch 对照接上。

验证：

```bash
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：

- 对照 smoke 通过，耗时 `elapsed 3.15`。
- legacy 输出里还是 `p_reg2mem`。
- angr 输出里能看到 copied vvar declaration 和 copied payload。

当前完成度：

- 旧模式和新模式在 copied switch region 上已经能直接分辨。
- 这条 case 还只是 smoke 级别，没把 shared 层复制回滚的所有边界都补完。

评分：

- 实现效果：6/10。比普通 Phi 对照更贴近当前目标。
- 复杂度：3/10。只是在 smoke 里多跑一组 mode。
- 维护成本：3/10。以后能直接用这个 case 看 copied 语义有没有回退。

## 2026-06-26 实现记录：shared dephication 回滚试探

这轮补的是结构恢复优化的回滚试探，不再往前加新语义。目标是确认
`StructuringOptimizationPass::analyze()` 在第一轮试探里如果把 copied dephication
状态和坏块一起塞进图里，第二轮重试时这些状态会跟着 rollback 一起消失，而不是
残留到后续 shared structuring。

改动：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:417`：
  新增 `RecoverCopiedDephicationPass`，第一轮复制带 shared dephication 的 region，
  同时插入一个坏块，第二轮只检查回滚后的图。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6599`：
  新增 `testStructuringOptimizationPassRollsBackCopiedDephicationMetadata()`，
  验证 `analyze()` 重试前已经把 copied edge / copied merge / 额外 vvar / incoming 都
  回滚掉。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：

- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 3.33`。

当前完成度：

- shared dephication 的 rollback 试探已经有直接测试。
- 还缺更贴近真实 pipeline 的失败回滚对照，但最小回滚边界已经被钉住。

评分：

- 实现效果：6/10。补的是回滚边界，不是新模式主逻辑。
- 复杂度：4/10。新增一个受控失败 pass 和一条单测。
- 维护成本：3/10。以后如果 rollback 行为变了，这条测试会先报出来。

## 2026-06-26 实现记录：copied dephication rollback 回归测试

这轮继续补 shared 层的回滚边界，不再扩核心逻辑。目标很明确：
验证 `StructuringOptimizationPass::analyze()` 在第一次 trial 失败后回滚时，
不仅把坏块删掉，也把 trial 里临时生成的 copied dephication 表一起收回去。

改动：

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:316`：
  新增 `RecoverCopiedDephicationPass`，第一次 trial 里会复制一个含 dephication 的 region，
  再注入坏块；第二次 trial 需要看到复制出来的 block 和 dephication 元数据已经不在。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6599`：
  新增 `testStructuringOptimizationPassRollsBackCopiedDephicationMetadata()`，直接跑
  `analyze()`，确认 rollback 后只保留原始 shared dephication。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:9283`：
  把新测试挂到主入口。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
/usr/bin/time -f 'elapsed %e' python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：

- `structuring-analysis-test` 通过。
- structuring smoke 通过，耗时 `elapsed 3.12`。
- rollback 后坏块 `99` 不在输出里。
- rollback 后 copied dephication 的临时 block 不在输出里。
- rollback 后只保留原始 `dephicationVVars()` / `dephicationIncomings()`。

当前完成度：

- 这条回滚边界现在已经被单测钉住。
- 还没补更大范围的 pipeline 对照，但这不影响这次回滚语义本身。

评分：

- 实现效果：6/10。把 rollback 语义补实了一点。
- 复杂度：4/10。新增了一个小的失败试探类和一条回滚测试。
- 维护成本：3/10。测试直接盯住 shared dephication 状态，后面好查回退。

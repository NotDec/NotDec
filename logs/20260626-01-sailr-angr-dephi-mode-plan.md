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

# 20260927-02 RegionSimplifier 第二批：EmptyNodeRemover + GotoSimplifier

## 0. 一句话

按 angr `region_simplifier.py::_simplify()` 的顺序往下补：`remove-empty` 与 `goto` 两条规则落地，
运行器加了"纯清理 pass 不要求 goto 下降"的提交策略。全语料 103 case：goto 1041 → **962**、
real jump 959 → **955**、fallthrough 76 → **1**、dangling 6 不变。
`RedundantLabelRemover`（angr 顺序里的下一条）**实测后不注册**：0 收益 + 大量注释翻动，理由见 §3。
定点 0870/0340/0637 不变（它们没有 fallthrough goto）。

## 1. 落地的两条规则

- **EmptyNodeRemover**：Sequence 的子节点里"渲染不出任何东西"的空 Sequence 直接摘掉（不动根，也不动
  If 的分支臂）。它本身不改任何指标，作用是把"相邻"暴露出来给 goto 规则。
- **GotoSimplifier**（angr `goto.py`）：goto 的目标标签就是它之后**紧接着渲染**的那个节点时，goto 是空操作。
  - 从 goto 出发按渲染顺序往后走：Sequence 层取下一个兄弟；兄弟取完就上升到上一层；
  - **跨 If 臂**：angr 把 ConditionNode 的 successor 传进两臂，所以一条臂末尾的 goto 之后，就是 If 之后的东西
    ——else 臂只在条件为假时执行，不在跳转这条路径上，等价处理；
  - 循环 / Switch 不跨：goto 在循环体末尾会回到循环头，在 switch case 末尾会 fallthrough 到下一个 case。
- **提交策略**：`RegionSimplifier::requiresFewerGotos()`。控制流规则必须严格减少 goto（angr 的
  `strictly_less_gotos`）；清理规则只要求不新增，否则空节点/注释类清理永远提交不了。

## 2. 效果

### 2.1 全语料（103 case，渲染后 `// goto block_` 统计）

| 指标 | R1 后 | 现在 |
| --- | --- | --- |
| goto | 1041 | **962** |
| real jump | 959 | **955** |
| fallthrough | 76 | **1** |
| dangling | 6 | 6 |

- 一共删了 79 条 goto。其中 **4 条在文本分类里算 real jump**：例如 0118 的
  `if (c) { …; // goto block_38 } else { … }` —— else 臂只在条件为假时走，所以 goto 38 之后就是
  `// block_38:`；文本分类器只跳 `}`/空行，看不出 else 不在路径上，树上的臂结构能看出来。这 4 条是真收益。
- 清理分布：第一批（不带跨 If）删 64 条（0029 的 32 条一次清完），第二批跨 If 臂再删 15 条。
- `empty-nodes` 计数是 0：语料里没有渲染为空的空 Sequence（规则留着兜底）。

### 2.2 定点 0870/0340/0637

不变（32 / 32 / 30，`redundant-gotos=0`）：这三例的 goto 都在 If 里、目标是远处的共享出口，
既不是 fallthrough 也不是"目标就在 If 后面"，属于 R1 诊断里的 missing-tail 12 / nonlinear-tail 16。

## 3. 为什么跳过 RedundantLabelRemover

angr 的 label 是**代码**（Jump 的目标，没用了就是死代码）；我们的 `// block_N:` 是**注释**。
落地前先量了一遍：1041 条 goto 里，**0 条**是因为中间夹了无用 label 才删不掉的（76 条 fallthrough 全直接相邻），
而每个函数有上百条 label 注释。上这条规则只会翻动全部输出、换不到任何指标，所以按 angr 顺序跳过它，
代码里也没留实现（注册处写了注释说明原因）。真要压输出体积再说。

## 4. 测试

- 单测 3 条仍绿（goto 规则在 If 臂里会让位给 R1：正例里 goto 之后先撞到的是尾部 label，不是目标 label，
  所以两条规则不打架）。
- 这条套件贴着 ctest 默认 1500 s 上限（正常 1427~1490 s，几乎全耗在 0029 的 `solc --via-ir`）；本轮有一次
  在同时跑别的任务时被 1500 s 掐掉。已给这条 test 显式设 `TIMEOUT 3600`（`test/CMakeLists.txt`），
  以后满负载下不会再假失败。
- `notdec.evm.solidity_pattern_compile`（103 case + solc）：§2.1 的数字与它一致，预算已下调到
  goto 962 / real jump 955。
- C 路径没接，`structuring-smoke` / `structuring-source-cases` 与上一轮一致（smoke 仍是那 30 行预先存在的漂移）。

## 5. 下一步

按 angr 顺序还剩 expr folding → switch* → ifelse → cascading ifs → loop：

- **expr folding** 是其中唯一要动接口的：structuring 层只拿得到 `PayloadRef`，要折叠表达式得让渲染器
  暴露 payload 的结构（或在渲染器侧做），不是纯树改写；
- **ifelse / cascading ifs / loop** 都是纯树改写，但收益要靠指标说话（goto 已基本只剩 R1 那两类 + 远处共享出口）；
- 更大的缺口不在这些清理规则里：0870/0340/0637 的 goto 目标是**远处共享出口**（missing-tail 12 / nonlinear-tail 16），
  要靠 R2（多前驱时复制尾部，angr ReturnDuplicator 的那一半职责）或 guard 形态的结构化改进；
  全语料这轮之后 goto=962 里 real=955，基本就是这批。

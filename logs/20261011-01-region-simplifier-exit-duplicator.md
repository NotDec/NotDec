# 20261011-01 RegionSimplifier：ExitDuplicator（守护跳到小终止出口）

## 0. 一句话

angr `region_simplifiers/` 剩下的几条（expr folding / cascading ifs / loop / switch*）都要 payload 级表达式访问，
而 structuring 层只有 `PayloadRef`，落不下去；真正还剩的 goto 缺口是"守护跳到远处的共享终止出口"。
于是补了树级 **ExitDuplicator**（angr `ReturnDuplicator` 的树对应物）：`if (c) { goto X }` 且 X 的区域是
小的、终止的直线代码时，把 X **复制**进分支、删掉 goto。全语料 103 case：goto 962 → **669**、
real jump 955 → **666**、dangling 6 → 2，solc 全过（compile failures 0）。

## 1. 规则与前提

- 分支形状与 R1 相同（一个分支只有 `Goto X`、另一个为空）；
- X 的 label 在树里存在；从 label 起按渲染顺序收集区域：只允许 Label / BasicBlock / Sequence，
  直到遇到**终止块**（CFG terminator Return/Unreachable）；
- 预算：语句 ≤ 8、调用 ≤ 16；
- **不许复制 recovery gap**：`StructuredCFG::setPayloadTodoHook` 让渲染器回答"这条 payload 是不是 TODO"，
  任一 payload 是则拒绝（避免把 TODO 文本复制一份、把棘轮顶上去）。渲染器侧复用已有的 recovery-gap 判定，
  并额外看 `EmitStatement.Comment`（事件签名未恢复的 TODO 存在这个字段里，共享判定看不到）；
- 改写：区域**深拷贝**（新 NodeId）→ `If.Then = Sequence(copies)`、清 Else/Children；条件不动（goto 在 Then），
  goto 在 Else 时翻转 `ConditionNegated` 归一化；
- 质量门沿用运行器：goto 必须严格下降才提交。

## 2. 数据（103 case，`notdec.evm.solidity_pattern_compile`）

| 指标 | R1+Goto | 现在 |
| --- | --- | --- |
| goto | 962 | **669** |
| real jump | 955 | **666** |
| dangling | 6 | **2** |
| unresolved | 97 | 93 |
| condition TODO | 2089 | 2087 |
| recover-body TODO | 95 | 81 |
| helper functions / call sites | 1943 / 5038 | 1968 / 5142 |
| solc compile failures | 0 | **0** |

预算按约定重锚（goto/real/dangling/unresolved/condition/recover-body 下调，helper floor 上调）。

## 3. 代价：event_signature 515 → 522（唯一上调的计数器，已写进预算 comment）

- 复制调用点会让 Solidity 后端的 **outlined helper 分区重排**：有的 helper 多了实例、有的换了名字；
  个别 helper body 里**本来就有的** `TODO: recover event signature` 因此被多印一次。
- 这是"复制既有 TODO 文本"，不是新的恢复缺口；同一次运行里 unresolved / condition TODO / recover-body
  合计下降 20。
- 如果不想动这个计数器：把 `MaxExitCalls` 收到 0 可以完全避免（实测 goto 回到 962，规则等于不生效）——
  语料里可复制的出口区域几乎都带调用。所以这里选择保留收益、重锚计数器。

## 4. 两个已知限制（debug 口径）

- `exit-not-straight`：目标块的入口是条件（树里该块是 `If` 节点）时不复制——需要"整棵子树所有路径都终止"的分析；
- `exit-too-large`：区域语句 > 8 或调用 > 16 时不复制。

这两个是下一批 goto 的主要来源（0870 里 not-straight 命中 57 次扫描）。

## 5. 为什么 angr 顺序里剩下的几条没落地

- **expr folding**：angr 在 AIL 上把单次使用的变量折进使用点；structuring 层看不到表达式，只有 `PayloadRef`；
- **cascading ifs**：要把两个条件合成 `a && b`，需要构造表达式；
- **loop**：for 的 iterator/initializer 识别要从循环体搬语句；
- **switch cluster/expr**：要读 switch 条件与 case 值。

这四条要么给 structuring 层加"渲染器构造/比较表达式"的接口，要么直接在渲染器侧做；它们跟 goto 无关，
收益主要可能落在 condition TODO 上（需要单独量）。

## 6. 测试与环境

- 单测：`structuring-analysis` 全绿，含本轮新增 4 条（复制出口 / 非终止拒绝 / 超预算拒绝 / TODO 拒绝）
  + 之前 3 条；
- 环境注意：顶层 HEAD 的工作区需要 `-DNOTDEC_ENABLE_BINARYSUB_FIELD_ORIGINS=ON -DBINARYSUB_ENABLE_BOUND_ORIGINS=ON`
  才能编译（并发会话的 MLsub 改动依赖这两个宏；`build-notdec-nothreads` 已按此重配）。
  `NOTDEC_SIMPLIFY=0` 仍是 A/B 开关；`NOTDEC_SIMPLIFY_DEBUG=1` 会打印 `exit-*` 计数
  （含 `[simplify-exit]` 的拒绝原因）。

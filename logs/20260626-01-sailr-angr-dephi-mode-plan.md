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


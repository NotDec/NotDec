# 2026-04-08 PNDiff Warning 与 MLsub Annotated IR 规划

## 背景

当前主链路已经完成工作文件夹迁移：

- 由 `--gen-work-dir` / `-g` 开启
- 默认路径为 `<input>.notdec`
- `--work-dir=<path>` 可覆盖路径

后续希望继续改进两件事：

1. 在类型推理结束时，如果 `PNDiff` 里仍残留未解的 `Add` / `Sub` 约束，输出专门的 warning 文件到 work dir
2. 为当前主用的 `MLsub` 类型恢复流程提供更直观的 annotated LLVM IR，直接展示每个 LLVM value 当前被判成 `ptr`、`num` 还是 `unknown`

本文件先记录当前会话里已经确认的现状和下一步实现规划，方便后续先单独处理中间的 `solve` 接线问题。

## 一、当前现状

### 1. PNDiff 当前实际覆盖范围

当前 `PNDiff` 里真正进入待求解队列的是：

- `Add`
- `Sub`

对应实现位于：

- `include/notdec/TypeRecovery/mlsub/PNDiff.h`
- `src/TypeRecovery/mlsub/PNDiff.cpp`

队列结构是：

- `PNIGraph::Constraints`
- `PNIGraph::Worklist`

而 `ICmp` 当前并不进入 `PNDiff` 队列。

在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp` 里：

- `addCmpConstraint()` 只是对两侧 operand 做 `PG.getPNIVar(...).unify(...)`
- 没有创建 compare constraint node，也没有放入 `PNDiff` 的待求解 worklist

因此这次 warning 文件的范围已经明确收窄为：

- 只处理 `Add` / `Sub`
- 不考虑 `ICmp`

### 2. PNDiff 求解器当前没有真正接上

目前发现两个关键问题：

1. `PNDiff::solve()` 本身开头仍然有：

```cpp
assert(false && "TODO");
```

2. 当前 `MLsub` 主流程中没有发现显式 `PG.solve()` 调用

也就是说，当前 `Add/Sub` 约束虽然会被收集进 `PNDiff`，但基本没有经过统一的 worklist 求解过程。

这意味着如果直接输出 `PNDiff.warn.txt`，很可能只是把“尚未尝试求解”的原始约束全都打印出来，并不能真实反映“求解后仍然残留”的问题。

所以后续应先优先解决：

- 把 `solve()` 真正接上
- 再把 warning 文件定义为“求解收尾后仍残留的未完全求解约束”

### 3. 旧 annotated writer 的位置与含义

仓库里已经有一套旧的 annotated writer，位于：

- `src/Passes/ConstraintGenerator.cpp`

主要入口是：

- `TypeRecovery::printAnnotatedModule(...)`
- `CGAnnotationWriter`

旧 writer 原先会产出两类文件：

- `02-AfterBottomUp.anno1.ll`
- `03-Final.anno2.ll`

这两个名字带有历史包袱。

实际语义并不是“第一版 / 第二版”，而是两套不同的数据视图：

- `anno1`
  - 对应旧 `TopDownGenerator`
- `anno2`
  - 对应旧 `SketchGenerator`

也就是说：

- `anno1` 更接近 top-down 后但还没进入最终 sketch/post-process 的视图
- `anno2` 更接近最终图

### 4. 旧 annotated writer 的可读性问题

当前旧 writer 最大的问题不是“没有信息”，而是“信息组织方式不适合用户直接读”：

1. 输出的是内部 graph/node 表示
   - 更偏实现细节
   - 不是用户最关心的 `ptr / num / unknown`
2. operand 注释使用 `covariant/contravariant` 混排
   - 格式类似 `a/b`
   - 没有显式字段名
3. 文件名历史含义不清
4. 数据源绑定在旧 `ConstraintGenerator` 图结构
   - 不适合当前主用的 `MLsub` 流程

因此这次不应原样复用旧格式，而应保留“Annotated IR”这个展示形式，换成更适合 `MLsub + PNDiff` 的注释内容。

## 二、本次已确认的设计决策

### 1. Warning 文件

后续将新增：

- `PNDiff.warn.txt`

输出位置：

- `workDir/PNDiff.warn.txt`

语义定义为：

- 在 `PNDiff.solve()` 被实际执行之后
- 仍然留在 `PNIGraph::Constraints` / `Worklist` 中
- 且未 fully solved 的 `Add` / `Sub` 约束

warning 文件至少应包含：

- 约束类型：`Add` 或 `Sub`
- 对应 LLVM 指令文本
- `Left` / `Right` / `Result` 的稳定 value 名
- 三者当前的 `ptr` / `num` / `unknown` 状态

### 2. Annotated IR

后续只保留一个 final annotated IR，不再延续旧的 `anno1/anno2` 双文件模型。

输出目标：

- 一个最终版 annotated IR 文件
- 放入 work dir

当前更推荐的命名风格是：

- `03-pndiff-final.ll`

具体文件名可以在实现时微调，但原则是：

- 明确
- 单文件
- 不再沿用 `anno1` / `anno2` 这种历史命名

### 3. Annotated 注释格式

最终注释应聚焦用户真正关心的信息：

- `ptr`
- `num`
- `unknown`

不再默认打印大段旧 graph node 字符串。

建议的注释风格是键值式、稳定、短小，例如：

```llvm
; pndiff: result=ptr, op0=ptr, op1=num
```

或者：

```llvm
; pndiff: result=unknown, op0=unknown, op1=num
```

重点目标：

1. 一眼能看出某个 LLVM value 当前是指针还是数字
2. operand 与 result 的含义明确
3. 对比不同 case 时 diff 更稳定

### 4. 旧 writer 的处理方式

旧 `ConstraintGenerator` 路径已不再是当前主用路径，因此：

- 不要求继续维护旧 writer 行为
- 可以把原有 annotated writer 逻辑迁移/重写到 `MLsub` 侧

核心思想是“迁移展示能力”，不是“保留旧格式”。

## 三、建议的实现顺序

后续建议按下面顺序推进，而不是一次性把所有事情混在一起改：

### 阶段 1：先接通 PNDiff solve

这是当前最关键的前置条件。

目标：

1. 去掉 `PNDiff::solve()` 开头的 `assert(false && "TODO")`
2. 在 `MLsub` 的合适收尾位置真正调用 `PG.solve()`
3. 先确认 `solve()` 接线后不会因为现存 `TODO` 分支直接崩掉

这里需要特别注意：

- `eraseConstraint()` 里目前仍有与 `ptradd` 相关的 `assert(false && "TODO")`
- 一旦 `solve()` 真被执行，这些分支可能首次真正被走到

因此阶段 1 的重点是：

- 先把求解流程接通
- 再看真实会暴露哪些残余分支和断言

### 阶段 2：增加 `PNDiff.warn.txt`

在阶段 1 基础上，再做：

1. 遍历仍残留的 `Constraints`
2. 只筛出未 fully solved 的 `Add` / `Sub`
3. 写入 `workDir/PNDiff.warn.txt`

这一步应建立在“求解已经跑过”的前提上。

### 阶段 3：迁移 annotated writer 到 MLsub

在 `MLsub` 侧新增自己的 annotated writer：

1. 数据源直接使用当前 SCC 对应的 `ConstraintsGenerator::PG`
2. 注释只显示 `ptr/num/unknown`
3. 只输出一个 final 文件

阶段 3 完成后，可以删除或至少不再继续扩展旧 `ConstraintGenerator` 路径上的 annotated writer。

## 四、后续实现时建议重点检查的点

1. `addSubConstraint()` 当前看起来把 `Sub` 也错误地接成了 `PG.addAddCons(...)`
   - 这很可能是现有 bug
   - 后续接通 `solve()` 时应优先确认
2. `solve()` 真跑起来以后，`eraseConstraint()` 中与 `ptradd` 相关的 TODO 很可能被触发
3. 若某些节点始终维持 `unknown`，要区分：
   - 是算法确实无法推出
   - 还是求解流程没有正确消费到相关约束
4. Annotated IR 的输出时机更适合放在：
   - `topDownPhase()` 之后且 `genASTTypes()` 之前/之后择一固定
   - 保证看到的是最终稳定状态

## 五、当前结论

这次会话已经完成的主要“设计收敛”是：

1. `PNDiff.warn.txt` 只关心 `Add/Sub`
2. `ICmp` 不纳入本轮范围
3. 旧的双阶段 `anno1/anno2` 不再保留
4. 新 annotated IR 只保留一个 final 文件
5. 注释内容改成更直观的 `ptr/num/unknown`
6. 下一步优先级最高的是：
   - 先把 `PNDiff.solve()` 接上
   - 再回来做 warning 和 annotated 输出

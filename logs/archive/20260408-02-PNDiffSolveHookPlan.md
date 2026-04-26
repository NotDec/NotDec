# 2026-04-08 PNDiff Solve Hook Plan

## 背景

当前 `MLsub` 路径下的 `PNDiff` 已经能收集 `Add` / `Sub` 约束，但还没有真正进入稳定可用的求解阶段。

本次规划聚焦三件事：

1. 如何把 `PNDiff::solve()` 真正接到当前 `MLsub` 主链路
2. `PNDiff` 里与 `solve` 直接相关的 TODO / 明显问题有哪些
3. `applyPNIPolicy()` 如何从旧路径迁移到新路径，并支持：
   - 内置默认策略
   - 外部 JSON 覆盖策略

本文件只做规划，不直接实现。

## 一、当前已确认的现状

### 1. `PNDiff::solve()` 目前还没有接通

当前文件：

- `src/TypeRecovery/mlsub/PNDiff.cpp`

现状：

- `PNIGraph::solve()` 开头仍然是 `assert(false && "TODO")`
- 原本应执行的 `applyPNIPolicy()` 仍停留在注释中：

```cpp
// CG.applyPNIPolicy();
```

### 2. `MLsub` 主流程里目前没有显式调用 `PG.solve()`

当前 `ConstraintsGenerator::run()` 位于：

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

其流程目前是：

1. 为函数、参数、返回值建点
2. 访问每个函数中的指令，收集约束与 PNI 信息
3. 结束

当前流程里没有：

- `PG.solve()`

因此即使 `Add` / `Sub` 约束被收集到了 `PNIGraph::Constraints` / `Worklist`，也基本不会被消费。

### 3. 当前 `MLsub` 版 `PNDiff` 与旧路径的关键差异

旧路径文件：

- `src/TypeRecovery/PointerNumberIdentification.cpp`

旧路径是基于：

- `ConstraintGraph`
- `CGNode *`
- `PNIToNode`

当前 `MLsub` 新路径是基于：

- `ExtValuePtr`
- `PNIGraph::PNIMap`
- `PNIMap.rev()`

也就是说，`solve()` 的核心规则可以直接迁移，但：

- `applyPNIPolicy()` 不能直接照搬旧 `ConstraintGraph::Nodes` 的写法
- `eraseConstraint()` 里对 `Result->setAsPtrAdd(...)` 的调用，也不能原样照搬旧路径

### 4. 旧路径 `applyPNIPolicy()` 的真实默认策略

旧代码位置：

- `src/TypeRecovery/ConstraintGraph.cpp`

旧实现不是 “1000 以下”，而是：

```cpp
if (C.offset != 0 && C.offset < 900 && C.offset > -900) {
  N.setPNINonPtr();
}
```

也就是说旧默认规则实际是：

- 仅对整数常量
- 非零
- 且绝对值严格小于 `900`
- 直接判为数字类型

这和“1000 以下的小数字”这个记忆接近，但并不完全一致。

## 二、solve 接线的推荐方案

### 方案总览

推荐按下面顺序接线：

1. 先把旧路径 `PointerNumberIdentification.cpp` 中的 `PNIGraph::solve()` 迁入当前 `mlsub/PNDiff.cpp`
2. 在当前 `MLsub` 版 `PNIGraph` 上补一个新的 `applyPNIPolicy()`
3. 在 `ConstraintsGenerator::run()` 的最后显式调用 `PG.solve()`
4. 修掉 `Sub` 被错误接成 `Add` 的明显 bug
5. 先保证 `solve` 能稳定跑完，再回来做 warning / annotated 输出

### 为什么优先接在 `ConstraintsGenerator::run()` 末尾

当前 `MLsub` 路径里，`PNDiff` 收集发生在 visitor 遍历指令期间：

- `visitAdd() -> addAddConstraint()`
- `visitSub() -> addSubConstraint()`
- 各种 `setPointer()` / `setNonPointer()` 也在 visitor 阶段进行

因此最自然的接法是：

- 先完整收集一个 SCC 内部的 PNI 信息
- 再在 `ConstraintsGenerator::run()` 末尾统一调用一次 `PG.solve()`

也就是：

1. 建点
2. visit 指令
3. `PG.applyPNIPolicy()`
4. `PG.solve()`
5. 返回

这能保证：

- `solve()` 看到的是当前 SCC 的完整 `Add/Sub` worklist
- 不需要把求解逻辑散到 visitor 中

### 当前阶段不建议做的事

当前不建议一开始就模仿旧路径做“多轮 interleaved solve”，例如：

- 在 binarysub 化简前后多次重跑
- 与 type simplifier 反复交织

原因：

- 当前 `MLsub` 路径没有旧 `ConstraintGraph::saturate()` 那样的 PNI <-> graph 联动框架
- 先把“一次收集，一次 solve”跑通，更容易定位剩余断言和规则缺口

## 三、当前和 `solve` 直接相关的问题清单

### 1. `addSubConstraint()` 当前把 `Sub` 错接成了 `Add`

文件：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

当前实现里：

```cpp
PG.addAddCons(LHS, RHS, I, I);
```

出现在 `addSubConstraint()` 内部。

这显然应该改成：

- `PG.addSubCons(...)`

这是接通 `solve()` 之前必须先修的明显 bug。

### 2. `PNDiff::solve()` 的 `assert(false && "TODO")`

文件：

- `src/TypeRecovery/mlsub/PNDiff.cpp`

这是最直接的阻塞点。

解决方式：

- 用旧路径 `src/TypeRecovery/PointerNumberIdentification.cpp` 中已经稳定工作的 `PNIGraph::solve()` 逻辑替换
- 再根据当前 `ExtValuePtr` 版本做适配

### 3. `eraseConstraint()` 里的两个 `ptradd` TODO

文件：

- `src/TypeRecovery/mlsub/PNDiff.cpp`

当前代码：

```cpp
assert(false && "TODO");
// Result->setAsPtrAdd(*Right, Off);
```

和：

```cpp
assert(false && "TODO");
// Result->setAsPtrAdd(*Left, Off);
```

这是旧路径里做“把 `ptr + num` 的求解结果回写成 pointer-add 语义”的位置。

但当前 `MLsub` 版没有旧的：

- `CGNode::setAsPtrAdd(...)`
- `ConstraintGraph` 层的 pointer-add 回写链路

因此这个 TODO 需要单独定策略。

### 4. `applyPNIPolicy()` 当前没有迁移到 `mlsub`

旧路径依赖：

- `ConstraintGraph::Nodes`
- `NodeKey`
- `CGNode::isPNIUnknown()`

当前 `MLsub` 版 `PNDiff` 没有 `ConstraintGraph`，因此必须改成基于：

- `PNIGraph::PNIMap`
- `ExtValuePtr`
- `PNINode`

去实现。

## 四、`eraseConstraint()` 中 ptradd TODO 的推荐处理

这里建议分两阶段处理。

### 阶段 A：先让 solve 稳定跑完

在首次接通 `solve()` 时，推荐先不要强行恢复旧路径的 `setAsPtrAdd()` 语义。

先做最小化策略：

- 移除当前 `assert(false && "TODO")`
- 对于已经 fully solved 的 `ptr + num -> ptr` 这种约束
  - 先仅做“约束移除”
  - 不额外回写复杂的 ptradd 结构信息

理由：

1. 当前目标首先是让 `solve()` 真正跑起来
2. `PNDiff.warn.txt` 和 final annotated IR 都主要依赖 `ptr/num/unknown` 状态
3. 是否保留“偏移语义”属于下一层增强，不是 solve 接通的先决条件

### 阶段 B：后续再补 ptradd 语义增强

等 solve 接通稳定后，如果发现：

- `ptr + num` 的偏移信息对 HType 质量有明显帮助

再考虑引入 `MLsub` 版的 `setAsPtrAdd()` 机制。

当前阶段不要把这个需求和 solve 接线绑在一起。

## 五、`applyPNIPolicy()` 的迁移方案

### 1. 推荐新增 `PNIGraph::applyPNIPolicy()`

文件建议：

- 头文件：`include/notdec/TypeRecovery/mlsub/PNDiff.h`
- 实现：`src/TypeRecovery/mlsub/PNDiff.cpp`

职责：

- 遍历当前 `PNIGraph` 中所有 PNI 相关 value
- 对符合策略的 unknown PNI 直接施加：
  - `setNonPtr()`
  - 或后续可能的 `setPtr()`

### 2. 默认内置策略

建议第一版直接保守复用旧路径默认策略：

- 整数常量
- 非零
- `abs(value) < 900`
- 设为 `num`

理由：

1. 这是仓库里已有历史行为
2. 改动最小
3. 先接上 solve 时更容易和旧路径结果对照

如果后续要调整成 `1000`，那属于显式策略变更，建议单独做，而不是和 solve 接线绑在一批里。

### 3. 外部 JSON 覆盖

推荐新增一个环境变量入口，例如：

- `NOTDEC_PNDIFF_POLICY_OVERRIDE`

值为 JSON 文件路径。

理由：

- 与现有：
  - `NOTDEC_SUMMARY_OVERRIDE`
  - `NOTDEC_SIGNATURE_OVERRIDE`
  - `NOTDEC_LEVEL_OVERRIDE`
  风格一致
- 不需要额外扩展 CLI
- 适合调试时快速替换策略

### 4. JSON 结构建议

第一版不要设计得太泛化，建议只覆盖当前真正需要的常量规则：

```json
{
  "version": 1,
  "int_constant_policy": {
    "non_pointer_abs_lt": 900,
    "exclude_zero": true
  }
}
```

这样内置默认行为可以是：

```json
{
  "version": 1,
  "int_constant_policy": {
    "non_pointer_abs_lt": 900,
    "exclude_zero": true
  }
}
```

覆盖文件只需要改其中的数值即可。

例如 WebAssembly 想试验更激进的版本，可以提供：

```json
{
  "version": 1,
  "int_constant_policy": {
    "non_pointer_abs_lt": 1000,
    "exclude_zero": true
  }
}
```

### 5. 为什么先不做更复杂的 JSON

当前不建议第一版就支持：

- 多条 range 规则
- 区分不同 target triple 的多层继承
- 对具体常量逐个 override

因为 solve 现在还没接通，优先级应是：

1. 先恢复旧能力
2. 再加最小 JSON 覆盖
3. 更复杂的策略系统后续再扩展

## 六、推荐的分阶段实施顺序

### 阶段 1：接通 solve 最小闭环

目标：

1. 将旧路径 `PNIGraph::solve()` 迁入当前 `mlsub/PNDiff.cpp`
2. 修复 `addSubConstraint()` 错接成 `addAddCons()` 的 bug
3. 在 `ConstraintsGenerator::run()` 末尾调用 `PG.solve()`
4. 将 `eraseConstraint()` 里的 `ptradd` TODO 改成“先不回写偏移语义，但不再断言”

完成标准：

- `solve()` 能被执行
- 不因为 `assert(false && "TODO")` 崩溃

### 阶段 2：迁移 `applyPNIPolicy()`

目标：

1. 在 `PNIGraph` 上新增 `applyPNIPolicy()`
2. 默认复用旧规则：`abs(constant) < 900 && constant != 0 -> num`
3. 支持从 `NOTDEC_PNDIFF_POLICY_OVERRIDE` 读取 JSON 覆盖

完成标准：

- 不提供 override 时，行为与旧路径尽量一致
- 提供 override 时，阈值可调整

### 阶段 3：接 warning 与 final annotated IR

这一步不在本文件主任务内，但依赖前两阶段。

完成标准：

- `PNDiff.warn.txt` 能反映“solve 后仍残留”的 `Add/Sub`
- final annotated IR 能直接展示 `ptr/num/unknown`

## 七、建议修改的文件范围

核心代码：

- `src/TypeRecovery/mlsub/PNDiff.cpp`
- `include/notdec/TypeRecovery/mlsub/PNDiff.h`
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

后续策略解析若需要单独工具函数，可考虑：

- `include/notdec/Utils/Utils.h`
- `src/Utils/Utils.cpp`

文档与记录：

- `logs/20260408-02-PNDiffSolveHookPlan.md`
- 后续实现完成后再补新的 `logs/` 记录

## 八、当前结论

目前最合理的下一步不是直接去做 `PNDiff.warn.txt` 或 annotated IR，而是：

1. 先把旧路径 `solve()` 正确迁入 `mlsub`
2. 先修 `Sub -> addAddCons` 这个明显 bug
3. 先把 `applyPNIPolicy()` 以“旧默认 + JSON 覆盖”的方式接回去
4. 先让 `solve()` 稳定跑完
5. 再回头做 warning 和 final annotated IR

这样改动风险最小，也最符合当前问题的真实依赖关系。

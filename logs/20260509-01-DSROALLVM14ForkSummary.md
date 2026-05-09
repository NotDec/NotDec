# 2026-05-09 DSROA 基于 LLVM 14 复制并本地化的梳理

## 背景

在处理 `src/Passes/DSROA.cpp` 的 LLVM 22 迁移前，先确认这份代码原本是怎么来的，
以及当时不是直接调用 LLVM 官方 `SROA`，而是复制一份自己维护，到底改了什么。

这份记录只做来源和本地化改法梳理，不改代码，不处理 LLVM 22 编译错误。

## 目标

1. 确认 `src/Passes/DSROA.cpp` 是不是从 LLVM 源码直接复制来的。
2. 确认它更接近哪个 LLVM 版本。
3. 总结当前代码里能直接看出来的本地化改法。
4. 给后面的 LLVM 22 迁移提供边界判断。

## 核对范围

主要看了这些地方：

- `git log --follow -- src/Passes/DSROA.cpp`
- `src/Passes/DSROA.cpp`
- `include/notdec/Passes/DSROA.h`
- `/sn640/llvm-toolchain-14-14.0.0-1ubuntu1.1/llvm/lib/Transforms/Scalar/SROA.cpp`

## 结论

### 1. 这份文件基本就是“从 LLVM 抄进来，再自己维护”

`src/Passes/DSROA.cpp` 的历史非常短：

1. `c3d0931` (`2025-07-01`, `Original SROA.`)
   - 一次性创建 `src/Passes/DSROA.cpp`。
2. `82cb97f` (`2025-07-30`)
   - 只补了 `using namespace llvm;`。

从历史上看，不存在长期小步演化的过程，起点就是一份完整拷贝。

### 2. 当前版本的底子明显是 LLVM 14，不是更高版本

对比当前 `src/Passes/DSROA.cpp` 和 LLVM 14 的
`llvm/lib/Transforms/Scalar/SROA.cpp`，能看出主体结构还是同一路：

- `AllocaSlices`
- `Partition`
- `AllocaSliceRewriter`
- `runOnAlloca()`
- `splitAlloca()`
- `rewritePartition()`
- `promoteAllocas()`

这些核心类型和流程都还在，说明它不是“参考 SROA 自己重写了一遍”，而是拿
LLVM 14 的实现做了本地化裁剪。

## 能直接确认的本地化改法

### 1. 从 LLVM 官方 pass 改成 NotDec 自己的 pass

最明显的变化，是不再依赖官方 `llvm/Transforms/Scalar/SROA.h`，而是改成项目自己的：

- `src/Passes/DSROA.cpp`
- `include/notdec/Passes/DSROA.h`

对应地，类型和命名空间也被改了：

- `llvm::sroa::SROAPass` -> `notdec::DSROAPass`
- `llvm::sroa` / 匿名命名空间 -> `notdec` / `notdec::dsroa`
- `DEBUG_TYPE` 从 `"sroa"` 改成 `"dsroa"`

这说明当时复制一份的第一个直接目的，就是把 SROA 接进 NotDec 自己的 pass
管线里，而不是把它当成 LLVM 内部黑盒 pass 用。

### 2. 沿用 LLVM 14 的 pass 成员状态，但改成项目自己的头文件和类型名

`include/notdec/Passes/DSROA.h` 里单独定义了 `DSROAPass`，里面放了这类成员：

- `Worklist`
- `DeadInsts`
- `PostPromotionWorklist`
- `PromotableAllocas`
- `SpeculatablePHIs`
- `SpeculatableSelects`

这里要注意，这几组成员本身并不是 NotDec 现设计的一套新状态。

LLVM 14 原版 `llvm::SROAPass` 头文件里本来就有几乎同样的一组成员，当前
`DSROAPass` 基本是沿着原版类定义搬过来的。更准确地说，本地化主要是：

- `SROAPass` -> `DSROAPass`
- `llvm` / `llvm::sroa` -> `notdec` / `notdec::dsroa`
- 官方头文件接口 -> `include/notdec/Passes/DSROA.h`

所以这里更像是“把 LLVM 14 的 pass 类整体搬到项目自己的命名空间和头文件里”，
不是重新设计了一套特殊的成员状态。

### 3. 官方 pass 壳和注册代码被删掉了

LLVM 14 原版底部那套东西，在当前文件里都没了：

- `runImpl()`
- legacy pass 包装
- `createSROAPass()`
- `INITIALIZE_PASS_*`

取而代之的是项目自己的 `DSROAPass::run()`。

这进一步说明，当时复制不是为了临时抄几段算法，而是为了把整段逻辑纳入 NotDec
自己的 pass 生命周期。

### 4. 统计和接线方式做过一层本地化

LLVM 原版大量用 `STATISTIC(...)`。

当前版本里：

- 一部分计数改成了 `DSROAPass` 的成员，例如
  `NumPromoted`、`NumDeleted`、`NumAllocasAnalyzed`、
  `NumAllocaPartitions`、`NumNewAllocas`。
- 还有少量仍是文件级变量，比如
  `NumLoadsSpeculated`、`NumVectorized`。

这些计数项本身大多也不是新发明的，基本能在 LLVM 14 原版
`SROA.cpp` 里的 `STATISTIC(...)` 找到对应项。差别主要是：

- LLVM 原版把它们放在文件级统计宏里。
- 当前 NotDec 版本把其中一部分改成了 `DSROAPass` 的成员变量。

另外，`DSROAPass::run()` 直接从 `FunctionAnalysisManager` 取：

- `DominatorTreeAnalysis`
- `AssumptionAnalysis`

并且返回时额外保了 `CallGraphAnalysis`。这都是本地 pass 接线的一部分。

### 5. 行为上已经和 LLVM 14 官方版有明确分叉

`runOnAlloca()` 里最显眼的一处，是 LLVM 14 原版会先跑：

```cpp
AggLoadStoreRewriter AggRewriter(DL, IRB);
Changed |= AggRewriter.rewrite(AI);
```

当前 NotDec 版本把这两行直接注释掉了。

这说明它不是“原样搬运再换个名字”，而是已经主动关掉过一段官方行为。

## 不能硬猜的地方

仓库里暂时没找到当时留下的设计说明，所以有些更细的原始动机现在不能写死。

比如：

- 为什么要关 `AggLoadStoreRewriter`
- 为什么这里要保 `CallGraphAnalysis`
- 为什么计数方式改成现在这样

这些现在最多只能记录成“代码事实”，不能直接写成“当时就是为了某个具体 decompiler
case”，除非后面再找到更直接的证据。

## 对 LLVM 22 迁移的意义

这个梳理说明了一件很关键的事：

- `src/Passes/DSROA.cpp` 不是项目里一段零散借用的 LLVM 代码。
- 它是一份已经本地化过、接进 NotDec pass pipeline 的私有 SROA。

所以后面迁 LLVM 22 时，至少从当前信息看，更稳的路线不是整份拿 LLVM 22
`SROA.cpp` 覆盖，而是保留 `DSROAPass` 这层本地壳，按块参考 LLVM 22
对应实现迁接口和必要逻辑。

## 本次修改

1. 新增 `logs/20260509-01-DSROALLVM14ForkSummary.md`
   - 记录 `src/Passes/DSROA.cpp` 的来源判断和本地化改法。

2. 回退 `logs/20260508-03-LLVM22MainProjectPort.md`
   - 把刚才误塞进去的 DSROA 来源梳理移回单独日志，避免主迁移日志混入太多旁支内容。

## 验证

这次没有改源码，也没有重新编译。

核对方式只有：

- 看 `git log`
- 看 LLVM 14 原版和当前 `DSROA` 的代码差异
- 看 `include/notdec/Passes/DSROA.h` 的本地 pass 接线

## 当前判断

这份日志已经足够支撑后面继续修 `DSROA.cpp` 的 LLVM 22 编译问题。

如果下一步继续做实现，建议直接以这份记录为背景，单独开“LLVM 22 下 DSROA
第一轮接口迁移”的实现记录。

# 2026-05-19 Gigahorse PHI 真实来源导出路线记录

## 用户原始 prompt

```text
写一个日志，新的日志专门针对处理，GIGAhorse当前如何不靠后验规则，而是直接导出真实来源
```

## 背景

这次不是在 NotDec 里继续补兜底，而是要把 PHI 这层语义往上移回 Gigahorse。

当前 evm2llvm 已经按“`PHIIncoming` 必须是真实 CFG 边上的真实来源”来做校验。
问题在于，Gigahorse 现在导出来的 `PHIIncoming.csv` 还混着两类东西：

1. 真正稳定的边来源。
2. 需要靠后验规则猜出来的 raw value 集合。

前一类可以直接给 LLVM native `phi` 用。
后一类一旦遇到多个候选、inline 叠加、或者同一 predecessor 上有多个 PHI，就会开始脆。

## 现在的情况

目前 Gigahorse 侧相关逻辑大致是：

- `logic/decompiler_output.dl`
  - 先从 `FunctionalBlockOutputContents` 推出 `PHIIncoming`。
  - 其中一部分情况会把前驱上的 PHI 结果直接当作 incoming。
  - 另一部分情况会从前驱 block 的 output / define 再去拼 `var_rep`。
- `clientlib/tac-transformers/abstract_function_inliner.dl`
  - inliner 里已经补了 `retStmt`，避免多返回 inline 时把 return exit 和 return value 搅成交叉积。
  - 但这里仍然有一层“用已有 PHI 结果和 use set 去筛候选”的后验规则。

从现在跑出来的样本看，问题已经不是“缺一个小修”，而是“事实表示层次不够”：

- `0146` 是缺 predecessor 对应的 incoming。
- `0010 / 0025 / 0032 / 0124 / 0143 / 0147` 这类是同一条边上挂了多个候选值。

这说明当前规则还没把“边上的真实来源”单独表达出来。

## 目标

把 `PHIIncoming` 从“边 + 值的猜测结果”改成“边 + 真正来源”的直接导出。

这里的“真实来源”不是泛泛的“某个变量”，而是下面这种更具体的东西：

- 这个 incoming 来自哪个直接 predecessor。
- 这个 predecessor 上真正应该输出哪个 SSA 值。
- 如果这个值是前一个 PHI 的结果，就直接导出那个 PHI def。
- 如果这个值来自 inline 后的 return 语义，就直接保留和那条 return 绑定的来源，不再靠 use set 反推。

## 这次不想走的路

不再继续往后验规则上加：

- 不再靠“某个 block 里哪个 PHI 的 use set 刚好匹配”来猜。
- 不再靠“如果多个候选，就挑一个看起来最像的”。
- 不再把 `PHIIncoming` 当成 raw vars 的集合再让下游自己合并。

原因很直接：

- 这类规则短期能补一部分样本。
- 但它们本质上是在补“没写进 facts 的语义”。
- 一旦出现多个相似候选，就还是会错。

## 更长远的路线

更稳的做法是让 Gigahorse 直接导出一条“边来源事实”。

思路上可以拆成两层：

1. 先把“哪条 CFG 边进来的”定死。
2. 再把“这条边上的值到底是谁”定死。

这样下游只需要消费，不需要猜。

对 inliner 来说，这意味着：

- `retStmt` 要保留。
- 但更关键的是，要让 `PHIIncoming` 直接绑定到那条 return edge 上的最终 SSA 来源。
- 不要再从 `actualRet`、`formalRet`、`use set` 反推唯一值。

对普通 block 来说，这意味着：

- 如果 predecessor 已经把某个 slot 合成了一个 PHI result，就直接导出这个 result。
- 不要把它重新展开成多个 raw vars。

## 可能的技术路线

这条路线更像是把事实层做干净，不是再写一层修补逻辑。

大致方向是：

- 把 `PHIIncoming` 里的来源从“拼出来的 `var_rep`”提升成“真实输出值”。
- 如果有必要，单独加一层中间关系，专门表示“边上的最终来源值”。
- inliner、普通 block output、函数参数这三类来源分开处理，但输出到同一个稳定的 edge-level 事实。

## 风险

1. 规则会比现在长。
   - 但这是把语义显式化，不是纯增复杂。

2. 可能还会碰到少数样本的歧义。
   - 这种歧义不能靠随便选一个解决。
   - 要么继续补 facts，要么把它单独归类，不要硬塞进一个默认分支。

3. 需要和 evm2llvm 的 validator 保持一致。
   - 这个 validator 现在是对的，不该为了方便去放宽。

4. 以后跑 Gigahorse 时默认关 inliner。
   - 不主动 inline，可以先把原始 CFG 和 PHI 来源看清楚。
   - 这样更容易区分到底是“block 自己的 PHI 语义有问题”，还是“inline 把来源搅乱了”。
   - 只有确实要测 inline 语义时，再单独开。

## 判断标准

这条路线算对了，至少要满足下面几条：

1. `PHIIncoming.csv` 里不再出现同一个 `(phiStmt, block, predBlock)` 下多个 `var` 的重复语义。
2. `PHIIncoming.csv` 不再缺 direct predecessor 对应的 incoming。
3. `evm2llvm` 不需要再靠 heuristics 兜底。
4. 对 inline 样本，return edge 上的来源和 exit block 能一一对上。
5. 对普通 block，前驱已经合成的 PHI result 能直接传下去，不再展开 raw vars。

## 运行规范

后续 Gigahorse 重跑默认使用关闭 inliner 的模式，避免把这两类问题混在一起：

- 原始 PHI 来源是否正确。
- inline 后的 PHI 来源是否正确。

这样跑出来的 `PHIIncoming.csv` 更适合先做语义归因，也更适合定位到底是导出问题还是 inline 问题。

## 备注

这份记录只定方向，不写实现清单。
等 Gigahorse 侧真改完，再把实际改动、影响范围和验证结果补进同一条日志里。

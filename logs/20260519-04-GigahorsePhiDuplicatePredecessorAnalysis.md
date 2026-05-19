# 2026-05-19 Gigahorse `duplicate PHIIncoming predecessor` 分析

## 用户原始 prompt

```text
那边gigahorse那边commit了吗，没有的话commit一下。然后，进一步调研duplicate PHIIncoming predecessor，写一个新的logs/下的文件分析问题
```

## 背景

`evm2llvm` 现在把 `PHIIncoming` 当成真实 CFG 语义来看。
也就是说，同一个 `(phiStmt, block, predBlock)` 只能有一条 incoming。

前一版修复已经把 Gigahorse 的 PHI 导出往“直接读块尾事实”方向收了，但现在跑 `evm2llvm_apehex_pilot/` 和 `apehex_evm_contracts/` 的一批样本时，还是会碰到：

```text
duplicate PHIIncoming predecessor for PHI ... from ...
```

这说明问题不在 `evm2llvm` 的校验器，而在 Gigahorse 还在把“一条 edge 上的多个候选值”一起导出来。

## 这次看到的现象

几个代表样本都很一致：

1. `0010`
   - `PHIIncoming.csv` 里，`0x116018_0x4` 这一组出现了两条记录。
   - 同一个 `phiStmt`、同一个 `block`、同一个 `predBlock=0x4678`，但值分别是 `0x4652arg0x2` 和 `0x4652arg0x3`。
   - 说明不是 predecessor 缺失，而是同一条 edge 上有两个候选来源。

2. `0032`
   - `0x386fS0x389c_0` 在多个 predecessor 上都能导出 incoming。
   - 其中报重复的那个 predecessor 对应了 6 个不同的 var。
   - 这不是单个样本的偶发现象，而是“一个 predecessor 对应多个值”这种模式反复出现。

3. pilot 里的 `07` / `08`
   - 也还是 `duplicate PHIIncoming predecessor`。
   - 说明这个问题在小样本和更大的训练样本里都存在。

## 现在更像什么问题

现在看下来，根因更像是：

1. Gigahorse 已经把 edge 定出来了。
2. 但 edge 上的“真实来源值”还没唯一化。
3. 导出规则仍然在把同一条边上能看到的多个值都写出去。

换句话说，问题不是 CFG 边错了，而是“边上到底该保留哪个 SSA 值”还没收紧。

这也解释了为什么只靠下面这类改法还不够：

- 先看 `IRBlockOutLocalStackContents`
- 再看 `FunctionArgument`
- 再用 fallback 补 `FunctionalBlockOutputContents`

这些事实能把来源范围缩小，但它们本身不是“唯一答案”。如果同一条 edge 上本来就有多个可见值，导出还是会炸。

## 目前判断

这类重复大概分两种：

1. **同一 predecessor 上确实有多个候选值**
   - 这是当前最主要的情况。
   - 例如 `0010` 里同一个 `predBlock=0x4678` 对应两个 argument 值。

2. **不同来源事实同时覆盖了同一条 edge**
   - 这类需要继续再拆，但它不是当前最显眼的主因。

所以，下一步更像是要在 Gigahorse 侧做“edge source 选型”，而不是在 `evm2llvm` 里放宽规则。

## 为什么会有多个候选

从代码看，`PHIIncoming` 不是一个“取唯一值”的关系，而是把几层多值事实直接并起来：

1. `PHILocation` 只负责找出哪些 `(block, stackIndex)` 需要 phi。
2. `PHIDirectIncoming` 对每条 CFG 边去扫 `IRBlockOutLocalStackContents`。
3. 这个上游关系本身就是集合，不保证一个 `(predBlock, stackIndex)` 只对应一个 `var`。
4. `PHIIncoming` 最后只是把 `PHIDirectIncoming` 原样输出，没有做去重选择。

这意味着：

- 只要 predecessor 的某个 stack slot 里本来就有多个可见来源，`PHIIncoming` 就会同时列出来。
- `!HasDirectPHIIncoming` 只能压掉 fallback 分支，压不掉 direct 分支内部的多值。

### 具体看见的两种模式

1. **同一个 predecessor 上挂了多个值**
   - `0010` 里，`0x116018_0x4` 从同一个 `predBlock=0x4678` 导出了两个值：
     - `0x4652arg0x2`
     - `0x4652arg0x3`
   - 这说明那条边在上游已经不是单值了。

2. **同一个 phi 的同一个 predecessor 下，候选数更多**
   - `0032` 里，`0x386fS0x389c_0` 对同一个 predecessor 一次能冒出 6 个值。
   - `pilot` 里的 `07` / `08` 也还是同类问题。

这更像是 `FunctionalBlockOutputContents` / `IRBlockOutLocalStackContents` 在表达“可能值集合”，而不是“唯一来源”。

## 为什么上游会变成多值

原因主要是两个：

1. **stack 传播本来就不是单值函数**
   - `FunctionalBlockOutputContents` 会把：
     - 当前 block 尾部的本地输出
     - 函数调用返回参数
     - 没有被 call 吃掉的旧 stack 内容
     都合到一起。
   - 这套规则天然更接近“保留所有可能性”，不是“选唯一值”。

2. **当前还没有一层专门的 edge-source 归一化**
   - 现在只知道“这个 predecessor 上能看到哪些值”。
   - 还没有一个事实明确说“这条 edge 最终只该保留哪个值”。
   - 所以只要同一个 slot 上能拼出多个候选，它们就都会被导出。

## 这件事说明什么

`duplicate PHIIncoming predecessor` 不是一个小的导出漏项。
它说明当前 facts 里缺的是“唯一化”这一步，而不是“更多候选来源”。

也就是说：

- 现在的导出已经能找对 edge。
- 但还不能稳定地把 edge 上的值收成一个。
- 所以越是复杂块、函数调用多、stack 传播多的地方，候选就越容易炸开。

## 结论

现在的状态不是“还差一个 fallback”。
而是 Gigahorse 的 PHI 导出还停在“列候选值”，没有真正收敛到“每条 edge 一个来源”。

这也是为什么：

- `PHIIncoming.csv` 不再是单纯缺事实的问题。
- 也不是简单的 inline 问题。
- 而是来源事实还没唯一化。

## 相关提交

- Gigahorse 提交：`2a4390d`，`Refine PHI incoming export`

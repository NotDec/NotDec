# 2026-05-19 Gigahorse 块头 Phi 收敛重构计划

## 背景

现在 Gigahorse 的栈传播更像“候选值传播”，不是“确定的语义转换”。
这在块头和 call/return 边界上最明显：

- 同一个 stack 槽可能保留多个来源。
- `PHIIncoming` 只是把这些候选继续往下传。
- 下游要自己猜哪个才是唯一来源。

这和现在想要的方向不一致。

我们现在要的不是再多补一些候选，而是把候选停在块头：

- 块外可以多值。
- 块头收敛成 phi。
- 块内和后续块只看单值 SSA，不再继续传播集合。

## 目标

把 Gigahorse 的栈语义拆成两层：

1. 候选收集层
   - 只负责从前驱和函数摘要里收集可能来源。
   - 允许多值。

2. 收敛层
   - 在每个 block head 上把同一 stack 位的候选收敛成一个 SSA 值。
   - 多个候选就建 phi。
   - 只有一个候选就直接绑定。

收敛完成后，后续逻辑只消费单值，不再传播候选集。

## 期望效果

1. `PHIIncoming` 不再在同一 `(phiStmt, block, predBlock)` 下堆出多个语义候选。
2. `BlockInputContents` 相关的多值只停留在块头收敛前。
3. `BlockOutLocalStackContents` / `FunctionalBlockOutputContents` 最终变成单值摘要的输入，不再承担候选传播职责。
4. call/return 边界只保留明确的栈变换，不再把“旧值平移”和“返回值”混成同一层候选集合。

## 技术路线

1. 先把原始候选和收敛结果拆开。
2. 在 block head 新增一个显式的 phi 收敛层。
3. 让后续读取口逐步切到收敛后的单值层。
4. 最后再考虑删掉旧的多值传播路径。

这次先做最小切口，不追求一步改完。

## 风险

1. 现在的传播链比较长，直接改成单值容易打断现有 call/return 识别。
2. 如果块头收敛点放错，可能把真实的合流语义提前截断。
3. 旧的分析和诊断规则很多，拆层后需要同步调整少量消费点。

## 判断标准

1. 块头能把多候选收敛住。
2. 后面的传播链不再继续放大候选集。
3. `duplicate PHIIncoming predecessor` 这类错误能明显减少。
4. 结构上能看出“候选层”和“单值层”的边界。

## 备注

这份计划只记录方向和风险，不写完整实现清单。
实现过程中的具体改动、命中样本和验证结果，后面继续补到同一条日志里。

## 2026-05-19 实现记录

这次没有继续叠在 `2a4390d` 那版 PHI 导出修复上，而是先切回修改前的基线 `0a19737`，
再开始做结构拆分。当前只做了第一步切口，把原始候选层和对外的输入/输出层分开，
方便后面把 block head 的 phi 收敛插进去。

改动文件：

- `/sn640/gigahorse-toolchain/logic/global_components.dl:131-158`
  - 新增 `BlockInputCandidate` 和 `BlockOutputCandidate`。
  - 让 `BlockInputContents`、`BlockOutputContents` 变成对候选层的薄包装。
  - 这样后面可以在候选层和对外层之间插入 block-head 收敛逻辑。
- `/sn640/gigahorse-toolchain/logic/functions.dl:1084-1154`
  - 新增 `FunctionalBlockInputCandidate` 和 `FunctionalBlockOutputCandidate`。
  - 让原来的函数级输入/输出传播先落到候选层，再由对外层包装出去。
  - `BeforeFunctionCallFunctionalBlockOutputContents` 也改为读候选层，给后续单值收敛留接口。
- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:20-78`
  - `PolymorphicStackIndex` 和 `PHIIncoming` 相关规则改成读候选层。
  - 这一步还没有真正收敛 `phi`，只是让 PHI 逻辑不再绑死在公共输入/输出层上。
- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:21-82`
  - 新增 `BlockHeadInputValue`。
  - 对非 phi 的块头输入，`TAC_Use` 现在读这个收敛后的单值接口。
  - 对 phi 块头，块头自己就是那个 SSA 值，不再让普通 use 口去读原始候选。

当前状态：

- 还没有把 `BlockInputContents` 整体改成最终单值接口。
- 但普通 use 口已经开始走块头收敛后的值。
- phi incoming 仍然保留原始候选，便于后面把收敛结果和 incoming 对齐。

## 2026-05-19 继续推进：前驱输出也用块头 phi 收口

这一步继续往前推，不只收 successor 的块头输入，也开始收 predecessor 的对外输出。

改动文件：

- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:21-102`
  - `PHIOutputForwarded` 现在直接看 `PHILocation(block, outputIndex, phiStmt)`。
  - 这表示只要 predecessor 块头在某个 slot 上已经有 phi，这个 phi 就是该 slot 的
    对外输出来源，不再要求 `IRBlockOutLocalStackContents` 还保留原始 stackIndex。
  - `PHIIncoming` 的 direct 分支仍然保留，但会先被 `PHIOutputForwarded` 挡掉。
  - `TAC_Use` 的块头读取也改成先拿 `BlockHeadInputValue`，再直接透传 `TACVariable`，
    避免再把块头值拆回 raw 候选。
- `/sn640/gigahorse-toolchain/logic/global_components.dl:356-359`
  - 去掉了 `OptimizedGlobalAnalysis` 里那条已经不再适用的 `.plan`。
  - 这是为了让新的非递归 `BlockInputContents` override 通过 Souffle 2.4 编译。

验证：

```bash
python3 /sn640/gigahorse-toolchain/gigahorse.py \
  -w /tmp/gigahorse-phi-refactor-test -j 1 -T 240 \
  --results_file /tmp/gigahorse-phi-refactor-test/results.json \
  --restart --disable_inline \
  /sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/05_medium_233cfe3212.hex
```

结果：

- 这次能完整跑完 Gigahorse 导出。
- `PHIIncoming.csv` 行数为 76。
- `(phiStmt, block, predBlock)` 重复键还有 15 组，说明还没有完全收干净。
- 但之前最显眼的 `0x116018_0x4` 重复已经不在了，说明把 predecessor 块头 phi 直接
  当成输出来源，这一步是有效的。

剩下的重复更像是 predecessor 自己的尾部输出还在保留多候选，而不是 successor 这一层
没接对。

判断：

- 实现效果：6/10。修掉了一批 edge 上的重复，但还没收完整。
- 复杂度：5/10。比上一步多了一层输出侧收口，但还是局部改动。
- 维护成本：5/10。后面如果继续推进，重点会转到块尾输出的单值化，而不是继续堆
  `PHIIncoming` 的后处理。

## 2026-05-19 重新从 `0a19737` 推进：edge 输出先收成单值

这次没有继续在 `830f820` 上堆。先把当时未提交的 `PHIOutputForwarded` 调整暂存为
stash：

```bash
git -C /sn640/gigahorse-toolchain stash push -m "wip phi output forwarded source slot"
```

然后从 `0a19737` 新开分支：

```bash
git -C /sn640/gigahorse-toolchain switch -c block-head-phi-refactor-clean 0a19737
```

当前分支只改导出层，不先改函数级递归传播。原因是 `Variable` 是 number，而导出的 PHI
是 symbol 级 `TACVariable`，不能直接把块头 PHI 塞回 `FunctionalBlockOutputContents`
继续递归传播。先在 PHI 导出边界做“edge 上的单值来源”，风险更小。

改动文件：

- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:45-65`
  - 新增 `BlockHeadInputValue`。
  - 如果块头 slot 是 polymorphic，就把 `PHILocation` 生成的 PHI stmt 当作该 slot 的
    唯一 TAC 值。
  - 如果不是 polymorphic，就保持原来的变量到 TACVariable 转换。
- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:67-90`
  - 新增 `BeforeCallBlockOutputValue`。
  - 对块尾直接产出的变量，按原定义块转成 TACVariable。
  - 对块尾转发的 stackIndex，不再展开 `FunctionalBlockInputContents` 候选，而是读
    `BlockHeadInputValue`。这一步是关键。
- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:92-111`
  - 新增 `BlockEdgeOutputValue`。
  - 分三类处理 edge 可见输出：普通 CFG 边、call return 新返回值、call 后未消费参数的
    stack 平移。
- `/sn640/gigahorse-toolchain/logic/decompiler_output.dl:113-118`
  - `PHIIncoming` 改成只读 `BlockEdgeOutputValue`。
  - 不再直接枚举 `FunctionalBlockOutputContents` 的候选集合。

验证：

```bash
python3 /sn640/gigahorse-toolchain/gigahorse.py \
  -w /tmp/gigahorse-phi-refactor-final-test -j 1 -T 240 \
  --results_file /tmp/gigahorse-phi-refactor-final-test/results.json \
  --restart --disable_inline \
  /sn640/NotDecChainExp/evm2llvm_apehex_pilot/inputs/05_medium_233cfe3212.hex
```

结果：

- Gigahorse 完整跑完，`errors=0`。
- 分析阶段输出时间：`0.01 + 1.57 + 0.00 + 0.00 secs`。
- `PHIIncoming.csv` 行数从 64 降到 61。
- `(phiStmt, block, predBlock)` 重复键从 3 组降到 0。
- 三条原来重复的 call 后继边，现在分别收成 predecessor 块头 PHI：
  - `0x378 -> 0x38d` 的 slot 1 收成 `0x378_0x3`
  - `0x3ea -> 0x3ff` 的 slot 1 收成 `0x3ea_0x3`
  - `0x456 -> 0x46b` 的 slot 1 收成 `0x456_0x3`

性能判断：

- 这次最终验证复用已编译 executable，没有重新编译 Datalog。
- 分析阶段时间和之前同用例基本一致，未看到运行期性能下降。
- 调试过程中有一次因逻辑变更触发重新编译，`main.dl` 编译约 347 秒；这是编译成本，
  不是样例分析运行成本。

判断：

- 实现效果：8/10。当前关注样例的 duplicate `PHIIncoming` 已清零。
- 复杂度：4/10。只在导出层增加 edge 输出单值接口，没有改递归核心。
- 维护成本：5/10。`PHIIncoming` 语义更清楚，但后续如果要彻底阻止候选继续传播，
  仍要考虑函数级 candidate / public contents 的更深拆分。

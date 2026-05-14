# evm2llvm SSA-only lowering 计划

## 总结

当前观察：

- Gigahorse TAC 已经把 EVM 栈语义提升成 `TAC_Def` / `TAC_Use`，evm2llvm 不应该再重新推栈。
- `PHIIncoming.csv` 已经补上 PHI 的 predecessor incoming 信息。
- 当前样本满足“一变量一 def”：`long_running` 是 1667 个 def、1667 个唯一变量；现有 fixtures 也没有重复 def。
- 现有 evm2llvm 还是 alloca/load/store slot 风格，PHI 是按边 store 到 phi def slot，不是 native LLVM phi。

计划直接做破坏式更新：

- 只保留 SSA-only lowering。
- 普通 TAC scalar 变量不再默认创建 slot。
- 普通 def 直接生成 LLVM `Value*`，use 直接读取这个 `Value*`。
- PHI 用 `PHIIncoming.csv` 生成 native LLVM `phi`。
- facts 不满足 SSA-only 要求时直接报错退出，不再静默退回旧 slot 模式。

自检单独做成函数。默认开启，发现问题直接失败。后续如果确认 Gigahorse 输出一直稳定，可以加选项关闭自检以减少开销。

## 背景

Gigahorse 的输入是 EVM 栈机器，但它输出的 TAC 已经不是原始栈操作。evm2llvm 应该消费这些 facts：

- `TAC_Def(stmt, var, n)`：语句定义变量。
- `TAC_Use(stmt, var, n)`：语句使用变量。
- `PHIIncoming(phiStmt, block, predBlock, var)`：某个 PHI 在某条 predecessor 边上取哪个变量。

`stackIndex` 是 Gigahorse 内部恢复 PHI 的依据，不应该成为 evm2llvm lowering 的主要模型。evm2llvm 只需要看 TAC 变量和 CFG facts。

## 当前实现

当前 lowering 是 slot-based：

```llvm
%x.slot = alloca i256
store i256 %v, ptr %x.slot
%u = load i256, ptr %x.slot
```

当前 PHI 处理已经按真实 CFG 边选择 incoming：

- 无条件边：在 predecessor terminator 前把 incoming value store 到 phi def slot。
- 条件边：必要时插入 `edge.<pred>.to.<succ>` block，只在实际走到这条边时 store。

这比旧 fallback 正确，但 IR 里还没有 native LLVM `phi`。

SSA-only 后，这套 slot PHI edge-store 逻辑应删除。PHI 应直接是：

```llvm
%x = phi i256 [ %a, %pred1 ], [ %b, %pred2 ]
```

## 关键观察

### 1. TAC var 当前可以当 SSA def

对 `/tmp/gigahorse-phiincoming-test/long_running/out/TAC_Def.csv` 的观察：

```text
TAC_Def 行数：1667
唯一 var 数：1667
```

当前 evm2llvm fixtures 也没有发现同一个 var 被不同 statement 定义多次。

这说明当前样本满足：

```text
var -> exactly one defining stmt
```

### 2. PHI 语义来自 Gigahorse

Gigahorse 内部关系本质上是：

```souffle
FunctionalBlockInputContents(next, index, var) :-
  FunctionalBlockOutputContents(from, index, var),
  LocalBlockEdge(from, next).
```

`PHIIncoming.csv` 把这层关系导出成：

```text
phiStmt    phiBlock    predBlock    incomingVar
```

因此 evm2llvm 不需要从变量名猜 predecessor，也不需要重新理解 stack index。

### 3. 可以放弃旧 slot 模式

现在目标是新的 evm2llvm 前端，不需要长期兼容 stock Gigahorse docker 的缺失 facts。wrapper 已经会在缺 `PHIIncoming.csv` 时提示。下一步 lowering 可以直接要求 patched Gigahorse facts。

如果 facts 不满足要求，应该报错并告诉用户缺什么，而不是生成看似通过 verifier 但语义不准的 IR。

## 目标

默认模式只有一个：SSA-only。

具体目标：

1. 普通 scalar TAC var 不建 slot。
2. `CONST/MOV/ADD/SUB/MUL/...` 直接生成 LLVM value。
3. `TAC_Use` 从 `ValueMap` 读取。
4. `PHIIncoming.csv` 生成 native LLVM `PHINode`。
5. `CALLPRIVATE` 返回值直接绑定到 SSA value。
6. side-effect 指令继续 emit runtime helper / terminator，但不能把 state effect 当普通 scalar value。
7. facts 自检失败时直接报错退出。

## facts 自检

新增一个独立函数，例如：

```cpp
llvm::Error validateSsaFacts(const TacProgram &program);
```

这个函数只检查 facts 是否满足 SSA-only lowering 的基本前提，不生成 LLVM IR。

检查项：

1. 每个 `TAC_Def` 的 `var` 最多被一个 statement 定义。
2. 每个 statement 的同一个 def index 不能重复。
3. 每个非 PHI use 必须能找到 def 或常量。
4. 每个 `PHIIncoming.phiStmt` 必须存在，对应 statement 必须是 `PHI`。
5. 每个有 `PHIIncoming` 的 PHI 必须有且只有一个 def。
6. 每条 `PHIIncoming(predBlock, block)` 必须是 `LocalBlockEdge(predBlock, block)`。
7. `predBlock` 和 `block` 必须属于同一个 function。
8. 每个有 `PHIIncoming` 的 PHI，incoming 数量至少覆盖当前函数内进入该 block 的 predecessor 边。
9. `CALLPRIVATE` 的 actual/formal 参数数量必须匹配。
10. `RETURNPRIVATE` 的返回数量必须匹配 function return vars。

处理策略：

- 默认启用自检。
- 自检失败直接返回 `Error`，CLI 打印错误并退出。
- 后续可以加 `--disable-ssa-fact-checks`，只在性能敏感或确认 facts 来源稳定时关闭。

不要在自检失败时自动退回 slot lowering。这样问题会被隐藏。

## 2026-05-14 评估调整

计划总体方向不需要变：终态仍然是 SSA-only，失败就报错，不保留完整 slot fallback。

需要改进的是前几步的落地边界。`ValueMap` 和普通表达式 SSA 化可以先做，但 native PHI 还没完成时，直接删除全部 slot 会让当前 `PHIIncoming` 用例失去 phi def 的可读值。因此前两步只能作为过渡：

- 普通 TAC scalar 的读写必须优先走 `Values`。
- `CONST/MOV/ADD/SUB/...` 这类已迁移指令写 def 时必须写入 `Values`。
- formals 进入函数时也要写入 `Values`。
- slot 只允许短暂服务于尚未迁移的 PHI edge-store 旧路径，不能作为普通变量的通用 fallback。
- 如果普通变量既不在 `Values`，也不是常量，应直接报错。
- native PHI 完成后，删除 PHI edge-store 和这段临时 slot 兼容逻辑。

这样前两步不会提前承诺“已经无 slot”，也不会让 slot fallback 继续掩盖 SSA facts 问题。

## 技术路线

### 阶段 1：建立 ValueMap

在 function lowering 内维护：

```cpp
std::map<FactId, llvm::Value *> Values;
```

读取变量：

1. 查 `Values[var]`。
2. 查 `TAC_Variable_Value` 常量。
3. 在 native PHI 完成前，只允许 PHI 旧路径读临时 slot。
4. 查不到就报错。

普通变量不再从 slot load。

写变量：

1. 确认 `var` 还没有写入 `Values`。
2. `Values[var] = value`。
3. 如果重复写，直接报错。

普通表达式不再依赖 slot。native PHI 尚未完成前，可以临时 mirror store 到 PHI 旧路径需要的 slot；这只是拆除旧 PHI lowering 前的中间态。

### 阶段 2：普通表达式 SSA lowering

先覆盖当前已经支持的纯 scalar 指令：

- `CONST`
- `MOV`
- `ADD/SUB/MUL`
- `AND/OR/XOR/NOT`
- `EQ/LT/GT/SLT/SGT/ISZERO`
- `DIV/SDIV/MOD/SMOD`
- `EXP/SIGNEXTEND/BYTE/SHL/SHR/SAR`
- `SHA3`
- `MLOAD/SLOAD/CALLDATALOAD/CALLDATASIZE/CALLVALUE/CALLER/TIMESTAMP/GAS/MSIZE`

规则：

- 有 def 的表达式必须产生一个 LLVM value。
- 单 def 写入 `Values[def]`。
- 多 def 暂时只允许已明确处理的 private call。
- 没有 def 的纯表达式可以直接忽略或报错，按当前 opcode 语义决定。

### 阶段 3：native PHI

PHI 不能等到普通语句顺序 lowering 时才创建。loop header 的 PHI 可能被 loop body 使用。

实现方式：

1. 创建所有 LLVM basic block。
2. 在普通语句 lowering 前，扫描每个 block 开头的 TAC PHI。
3. 对每个有 `PHIIncoming` 的 PHI 创建 placeholder `PHINode`。
4. 立即 `Values[phiDef] = phiNode`。
5. 普通语句 lowering 完成后，按 `PHIIncoming` 填 `addIncoming(value, predBB)`。

注意：

- 不再插 `edge.<pred>.to.<succ>` block。
- PHI incoming block 使用真实 LLVM predecessor。
- 如果后续其它 transform 拆边，必须同步更新 incoming block。第一版先不拆 PHI 边。

### 阶段 4：private call 多返回 SSA

`CALLPRIVATE` 当前已经能返回 `void`、`i256` 或 struct。SSA-only 下：

- 无返回：只 emit call。
- 单返回：`Values[retVar] = callResult`。
- 多返回：`Values[retVarI] = extractvalue callResult, i`。

`ActualReturnArgs.csv` 优先决定 caller 侧 return var。没有或数量不匹配就报错。

### 阶段 5：side-effect 指令

这些不是普通 scalar 表达式：

- `MSTORE`
- `MSTORE8`
- `SSTORE`
- `CALL`
- `LOG0..LOG4`
- `CALLDATACOPY`
- `RETURN`
- `REVERT`
- `STOP`
- `THROW`

处理规则：

- 有明确 scalar def 的 side-effect 指令才写 `Values`。
- 没有 scalar def 的只 emit side effect。
- terminator 仍由 CFG lowering 处理。

### 阶段 6：删除 slot 相关代码

SSA-only 稳定后，删除或大幅收缩：

- 为每个变量创建 `alloca` 的逻辑。
- `InstructionLowerer` 里的 slot-only `loadWord/storeWord`。
- PHI edge-store block 逻辑。

如果以后确实需要 slot，只为特定状态建，不再为普通 TAC var 建。

## 代码改动边界

### FactLoader

可以新增基础索引：

- `DefsByVar`
- `DefByStmtIndex`
- `StmtById`
- `BlockByStmt`

这些索引用于 `validateSsaFacts` 和 lowering。

### LlvmLowerer

负责 function-level SSA 状态：

1. 调用 `validateSsaFacts`。
2. 创建 LLVM function 和 basic block。
3. 创建 PHI placeholder。
4. 维护 `Values`。
5. lowering 普通 statements。
6. 填 PHI incoming。
7. lowering terminator。

### InstructionLowerer

终态下 `InstructionLowerer` 不再持有 `slots`。阶段 1/2 过渡时可以继续接收临时 slot map，但接口必须把普通 scalar 的读写收敛到 `ValueMap`，不要让 opcode lowering 直接依赖 slot。

它应该通过一个小接口读写 scalar：

```cpp
llvm::Expected<llvm::Value *> loadValue(const FactId &var);
llvm::Error defineValue(const FactId &var, llvm::Value *value);
```

这样 opcode lowering 不需要知道 facts 索引和 PHI 细节。

## 推进顺序

建议按这个顺序实现：

1. 新增 `validateSsaFacts`，先只检查 def 唯一性、PHIIncoming 边合法性。
2. 改 `InstructionLowerer` 接口，引入 `ValueMap`，formals 和普通 def 写入 `ValueMap`。
3. 迁移 `CONST/MOV/ADD/SUB`，只为尚未 native 化的 PHI 保留临时 slot 兼容，确保 fixtures 先过。
4. 迁移所有当前支持的纯 scalar opcode。
5. 实现 native PHI，删除 PHI edge-store 逻辑。
6. 迁移 `CALLPRIVATE` 返回值。
7. 删除默认变量 slot 创建。
8. 用 `long_running + PHIIncoming` 验证。
9. 需要时补 `--disable-ssa-fact-checks`。

## 风险

1. **dominance 问题**

LLVM SSA value 必须支配所有 uses。如果 Gigahorse facts 缺 PHI 或 CFG 不完整，SSA-only 会直接失败。这是预期行为。

2. **旧 Gigahorse facts**

旧 facts 没有 `PHIIncoming.csv`，不能可靠生成 native PHI。SSA-only 应直接报错，提示需要 patched Gigahorse。

3. **PHI 填充顺序**

loop PHI 需要先创建 placeholder，否则 loop body use 会找不到 value。

4. **side-effect 指令**

不能把 `MSTORE/SSTORE/CALL/LOG/RETURN/REVERT` 的 state effect 当普通 scalar value。只有明确 def 才写入 `ValueMap`。

5. **自检开销**

自检会多扫 facts。当前规模应可接受。后续如果确认 Gigahorse 输出稳定，可以加开关关闭。

## 判断标准

1. 自检能明确报错：

```text
duplicate TAC def for var ...
missing SSA value for use ...
PHIIncoming edge is not a LocalBlockEdge ...
```

2. 默认 fixtures 通过：

```bash
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
```

3. `long_running + PHIIncoming` 通过：

```bash
./build-evm2llvm/bin/evm2llvm --facts /tmp/gigahorse-phiincoming-test/long_running/out --output /tmp/out.ll
llvm-as /tmp/out.ll -o /tmp/out.bc
opt -passes=verify -disable-output /tmp/out.bc
```

4. IR 中能看到 native LLVM `phi`。

5. 普通 TAC scalar var 不再默认生成 `.slot` alloca。

6. 缺少 `PHIIncoming.csv` 时直接报错，提示需要支持 PHIIncoming 的 Gigahorse。

## 当前建议

下一步直接实现 SSA-only，不再做 mirror-slot 过渡模式。

先落地 `validateSsaFacts` 和 `ValueMap`，让错误尽早暴露。只要 Gigahorse facts 一直满足当前观察到的条件，SSA-only 路线比继续维护 slot fallback 更简单，也更符合 TAC 的设计。

## 2026-05-14 实现记录：阶段 1/2 已完成

本次实现了前两步的中间态：facts 自检 + `ValueMap` 普通表达式 lowering。native PHI 还没做，所以临时 slot 只保留给 PHI edge-store 旧路径。

改动文件和函数：

- `external/NotDec-evm2llvm/include/notdec-evm2llvm/SsaFactValidator.h:9`：新增 `validateSsaFacts` 声明。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:20`：新增 `validateSsaFacts`，检查重复 statement、重复 def、同 var 多 def、`PHIIncoming` 是否指向真实 PHI、边是否是 `LocalBlockEdge`、两端 block 是否同函数。
- `external/NotDec-evm2llvm/lib/CMakeLists.txt:1`：把 `SsaFactValidator.cpp` 加进 `notdec-evm2llvm-core`。
- `external/NotDec-evm2llvm/include/notdec-evm2llvm/InstructionLowerer.h:29`：`InstructionLowerer` 接收 `Values`，新增 `loadPhiEdgeWord` 和 `defineWord`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:45`：`loadWord` 改成先读 `Values`，再读常量，最后只读临时 PHI slot；普通变量缺值时报 `missing SSA value`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:65`：`loadPhiEdgeWord` 专门给 PHI edge-store 用，优先读 slot，避免 edge block 使用不支配该 block 的 SSA 指令。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:73`：`defineWord` 写入 `Values`，重复定义直接报错；如果变量属于临时 PHI slot，再 mirror store。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:401`：普通 scalar 指令、`CALL` 返回、无 incoming 的 `PHI` 都改用 `defineWord`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:34`：新增函数内 block lowering 顺序，按 entry 出发的 reverse postorder 处理，避免按 fact id 先处理 use block。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:107`：PHI edge-store 改用 `loadPhiEdgeWord`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:377`：只为 `PHIIncoming` 涉及的 phi def 和 incoming var 建临时 slot，不再为函数内所有变量建 slot。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:414`：函数 formals 直接放入 `Values`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:423`：普通 statement lowering 改用 reverse postorder。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:461`：`lowerToLlvm` 开始时调用 `validateSsaFacts`。

实现中调整：

- 原计划说前两步“不再从 slot load/store”，实际改成“普通变量不再依赖 slot；PHI edge-store 旧路径临时用 slot”。原因是 `long_running` 的 `PHIIncoming` 会在 edge-store 中读取 loop-carried incoming var，native PHI 之前不能直接用跨 block 的 SSA 指令，否则 LLVM verifier 会报 dominance 错。
- `PHIIncoming` 当前还会出现同一 PHI stmt、同一 edge 多个 incoming var 的情况。这次没有改变语义，只让旧 edge-store 路径继续按原顺序写 slot。native PHI 阶段需要重新确认这个 fact 的精确定义。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
/usr/bin/time -p ./build-evm2llvm/bin/evm2llvm --facts /tmp/gigahorse-phiincoming-test/long_running/out --output /tmp/notdec-evm2llvm-long-running.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-evm2llvm-long-running.ll -o /tmp/notdec-evm2llvm-long-running.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/notdec-evm2llvm-long-running.bc
```

结果：

- `evm2llvm.fixture`：12/12 passed。
- `long_running`：emit、assemble、verify 通过。
- `long_running` emit 时间：`real 0.12s, user 0.11s, sys 0.01s`。

性能说明：

- 本次只改 `external/NotDec-evm2llvm`，不触碰 NotDec 主 pass pipeline、类型恢复、结构体合并或 pointer analysis。
- 因此没有跑 fortune 当前关注用例；它和本次 evm2llvm fact lowering 不是同口径对比。

当前方案评分：

- 实现效果：7/10。前两步目标达成，fixtures 和 long_running 都过；但 native PHI 还没做，IR 里仍有 PHI 旧路径 slot。
- 复杂度：6/10。`ValueMap` 逻辑比较直接，但临时 PHI slot 让中间态还有一点理解成本。
- 维护成本：6/10。下一步 native PHI 完成后，应删除 `loadPhiEdgeWord`、PHI edge-store slot 和 edge block 插入逻辑，维护成本会下降。

下一步：

1. 处理 native PHI 前，先确认 `PHIIncoming` 同一 `phiStmt + edge` 多 incoming var 的真实语义。
2. 实现 native PHI placeholder 和 `addIncoming`。
3. 删除 PHI edge-store、`loadPhiEdgeWord`、临时 PHI slot。

## 2026-05-14 实现记录：阶段 3 输出 native PHI

阶段 3 做了调整：没有直接用 `PHIIncoming` 生成 `PHINode::addIncoming`。真实 `long_running` 里，`PHIIncoming` 的 incoming var 不一定支配 predecessor 末尾，直接 `addIncoming(value, pred)` 会被 LLVM verifier 拒绝。旧 edge-store 语义是正确的中间表示，所以本次保留 edge-store 作为构造步骤，再用 LLVM `PromoteMemToReg` 生成最终 native PHI。

改动文件和函数：

- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:12`：新增 `DominatorTree`、`BasicBlockUtils`、`PromoteMemToReg` 相关 include。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:110`：新增 `promotePhiSlotsToSsa`，收集可提升的 PHI 临时 alloca 并调用 `PromoteMemToReg`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:125`：新增 `mergePhiEdgeBlocks`，mem2reg 后合并 `edge.*` 空跳转块。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:445`：临时 alloca 名字改为变量名本身，不再带 `.slot` 后缀，避免 mem2reg 后残留 `.slot` 名字。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:472`：函数 lowering 结束后调用 `promotePhiSlotsToSsa` 和 `mergePhiEdgeBlocks`。

实现中调整：

- 直接 native PHI 方案试过，但 `long_running` 会报 dominance 错误，例如 incoming value 定义在并不支配 predecessor 的 block 中。
- 当前方案仍使用 `PHIIncoming` 决定 edge-store 的值，之后交给 mem2reg 构造合法 SSA PHI。输出效果满足 native PHI 目标，同时保留旧 edge-store 的边语义。
- 源码里暂时还有 `loadPhiEdgeWord` 和 PHI 临时 slot，因为它们是 mem2reg 前的构造步骤。最终输出 IR 不再保留这些 slot。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
/usr/bin/time -p ./build-evm2llvm/bin/evm2llvm --facts /tmp/gigahorse-phiincoming-test/long_running/out --output /tmp/notdec-evm2llvm-long-running.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-evm2llvm-long-running.ll -o /tmp/notdec-evm2llvm-long-running.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/notdec-evm2llvm-long-running.bc
```

结果：

- `evm2llvm.fixture`：12/12 passed。
- `long_running`：emit、assemble、verify 通过。
- `long_running` emit 时间：`real 0.12s, user 0.12s, sys 0.00s`。
- `/tmp/notdec-evm2llvm-long-running.ll` 中 `phi` 数量为 61。
- 输出中未发现 `.slot`、`alloca i256`、`edge.*`。

当前方案评分：

- 实现效果：8/10。输出达成 native PHI，fixtures 和 long_running 都过。
- 复杂度：7/10。比直接 `addIncoming` 多了一步 mem2reg，但这是当前 facts 语义下更稳的做法。
- 维护成本：6/10。后续如果 Gigahorse 导出的是真正 predecessor-local incoming value，可以再改成直接建 PHI；在当前 facts 下不建议强行直接化。

下一步：

1. 迁移 `CALLPRIVATE` 多返回的覆盖测试，确认 `ActualReturnArgs` 路径。
2. 收紧缺失 `PHIIncoming.csv` 的行为，按 SSA-only 要求报错或至少在含 PHI 时失败。

## 2026-05-14 实现记录：阶段 4 多返回覆盖

阶段 4 的实现逻辑已在前面 `ValueMap` 改造中完成：`lowerPrivateCall` 对多返回 call 使用 `extractvalue`，并把每个 caller 侧 return var 写入 `Values`。本次补测试覆盖，避免后续改动破坏这条路径。

改动文件和函数：

- `external/NotDec-evm2llvm/test/CMakeLists.txt:39`：新增 `private_call_multi_return` fixture。
- `external/NotDec-evm2llvm/test/fixtures/private_call_multi_return/ActualReturnArgs.csv:1`：声明 caller 侧两个返回变量 `0xret0`、`0xret1`。
- `external/NotDec-evm2llvm/test/fixtures/private_call_multi_return/TAC_Def.csv:2`：`CALLPRIVATE` 语句定义两个返回变量。
- `external/NotDec-evm2llvm/test/fixtures/private_call_multi_return/TAC_Op.csv:2`：caller 调用 private function，callee 通过 `RETURNPRIVATE` 返回 `{sum, diff}`。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
rg -n "private.call|extractvalue|ret.insert" build-evm2llvm/test/private_call_multi_return.ll
```

结果：

- `evm2llvm.fixture`：15/15 passed。
- `private_call_multi_return.ll` 中出现 `{ i256, i256 }` call 和两个 `extractvalue`。

性能说明：

- 本次只新增 evm2llvm fixture，不改变主链路性能。

## 2026-05-14 实现记录：缺 PHIIncoming 时失败

本次收紧了旧 Gigahorse facts 的处理：如果 facts 中有 TAC `PHI`，但没有 `PHIIncoming.csv`，自检直接失败。没有 PHI 的简单 fixture 不受影响。

改动文件和函数：

- `external/NotDec-evm2llvm/include/notdec-evm2llvm/TacProgram.h:67`：新增 `HasPhiIncomingFacts`。
- `external/NotDec-evm2llvm/lib/FactLoader.cpp:180`：加载 facts 时记录 `PHIIncoming.csv` 是否存在。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:40`：扫描 TAC `PHI`。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:62`：有 PHI 但缺 `PHIIncoming.csv` 时返回错误。
- `external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py:127`：wrapper warning 不再说会使用旧 slot fallback。

验证：

```bash
cmake --build build-evm2llvm --target evm2llvm -j4
ctest --test-dir build-evm2llvm -R evm2llvm.fixture --output-on-failure
rm -rf /tmp/evm2llvm-missing-phiincoming
cp -a external/NotDec-evm2llvm/test/fixtures/phi_branch /tmp/evm2llvm-missing-phiincoming
rm /tmp/evm2llvm-missing-phiincoming/PHIIncoming.csv
./build-evm2llvm/bin/evm2llvm --facts /tmp/evm2llvm-missing-phiincoming --output /tmp/evm2llvm-missing-phiincoming.ll
/usr/bin/time -p ./build-evm2llvm/bin/evm2llvm --facts /tmp/gigahorse-phiincoming-test/long_running/out --output /tmp/notdec-evm2llvm-long-running.ll
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-evm2llvm-long-running.ll -o /tmp/notdec-evm2llvm-long-running.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/notdec-evm2llvm-long-running.bc
```

结果：

- `evm2llvm.fixture`：15/15 passed。
- 缺 `PHIIncoming.csv` 的 `phi_branch` 负例按预期失败，错误为：`missing PHIIncoming.csv; SSA-only PHI lowering requires Gigahorse facts with predecessor-specific PHI inputs`。
- `long_running`：emit、assemble、verify 通过，emit 时间 `real 0.13s, user 0.11s, sys 0.01s`。

当前进度：

- 阶段 1/2 已完成。
- 阶段 3 输出 native PHI 已完成。
- 阶段 4 多返回 private call 已有覆盖。
- 阶段 5 side-effect 指令仍沿用当前 helper lowering，`CALL` 的 scalar def 已走 `Values`。
- 阶段 6 还未做源码层面的 PHI 临时 slot 删除；当前用它作为 mem2reg 输入，最终输出 IR 已无 slot。

## 2026-05-14 实现记录：源码层面原生 PHI

这次重新评估后，阶段 3 的判断更新：Gigahorse 侧已经补齐 `PHIIncoming` 的唯一缺口，`long_running` 的 PHI incoming 已能一条 predecessor 对一条 incoming。因此 evm2llvm 不再需要先建 PHI slot 再走 `PromoteMemToReg`，可以直接生成 LLVM `PHINode`。

改动文件和函数：

- `external/NotDec-evm2llvm/include/notdec-evm2llvm/InstructionLowerer.h:28`：`InstructionLowerer` 注释改为纯 SSA value 模型，构造函数不再接收 PHI slot。
- `external/NotDec-evm2llvm/include/notdec-evm2llvm/InstructionLowerer.h:39`：删除 `loadPhiEdgeWord` / `storeWord`。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:23`：构造函数删除 `Slots` 成员。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:43`：`loadWord` 只读 `Values` 或常量，缺失直接报错。
- `external/NotDec-evm2llvm/lib/InstructionLowerer.cpp:58`：`defineWord` 只写 `Values`，重复 def 报错。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:78`：新增 `terminalStatement`，跳过 PHI 找真实 terminator，修复 PHI id 排序导致的 `ret void` 错误。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:117`：新增 `PhiNodeMap`，用 PHI statement id 关联 LLVM `PHINode`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:128`：新增 `valueForPhiIncoming`，PHI incoming 可来自已降低 SSA value 或常量。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:144`：新增 `fillPhiIncoming`，按 `PHIIncoming.csv` 调 `PHINode::addIncoming`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:230`：普通 CFG 分支直接跳真实 successor，不再插 `edge.*` block。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:401`：普通语句 lowering 前预创建所有有 incoming 的 PHI placeholder，并写入 `Values[phiDef]`。
- `external/NotDec-evm2llvm/lib/LlvmLowerer.cpp:452`：所有 block lowering 完成后填 PHI incoming。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:26`：记录每个 block 的 predecessor 和每个 PHI 的 incoming predecessor。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:90`：拒绝同一个 PHI 从同一个 predecessor 重复 incoming。
- `external/NotDec-evm2llvm/lib/SsaFactValidator.cpp:123`：检查每个 PHI 的 incoming predecessor 必须和当前函数内 CFG predecessor 完全一致。

实现中调整：

- 之前 direct PHI 失败的核心原因不是 LLVM PHI 本身，而是 Gigahorse 少导出 `0x1078B0x1068B0xeee -> 0x107eB0x1078B0x1068B0xeee` 这一条 incoming。Gigahorse 修复后，这条边现在生成：

```llvm
%_0x107e_0x0V0x1078V0x1068V0xeee = phi i256
  [ %_0x1068_0x1V0xeee, %bb._0x1078B0x1068B0xeee ],
  [ %evm.add68, %bb._0x1087B0x1078B0x1068B0xeee ]
```

- `long_running` 里还有一个无关旧问题：block `0x784` 同时有 `THROW` 和同地址 PHI，旧代码用排序后的 `block.Statements.back()` 判断 terminator，会误把 PHI 当尾语句，导致返回 `i256` 的函数里生成 `ret void`。现在改成取最后一条非 PHI statement。
- 源码层面已删除 PHI 临时 slot、edge-store、`PromoteMemToReg`、`edge.*` block 合并逻辑。

验证：

```bash
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE
cmake --build build -j4
ctest --test-dir build --output-on-failure
build/bin/evm2llvm --facts /tmp/gigahorse-phi-postprocess-test/long_running/out --output /tmp/evm2llvm-long-running-native-phi.ll --module-name long_running
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/evm2llvm-long-running-native-phi.ll -o /tmp/evm2llvm-long-running-native-phi.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify -disable-output /tmp/evm2llvm-long-running-native-phi.bc
build/bin/evm2llvm --facts /tmp/gigahorse-souffle241-addon64-test/long_running/out --output /tmp/evm2llvm-long-running-native-phi-unrepaired.ll --module-name long_running
/usr/bin/time -p build/bin/evm2llvm --facts /tmp/gigahorse-phi-postprocess-test/long_running/out --output /tmp/evm2llvm-long-running-native-phi-time.ll --module-name long_running
```

结果：

- `ctest`：15/15 passed。
- 修复后的 `long_running`：emit、assemble、verify 通过。
- 未修复 Gigahorse facts 的 `long_running`：按预期失败，错误为 `PHIIncoming missing predecessor for PHI 0x107e_0x0S0x1078S0x1068S0xeee from 0x1078B0x1068B0xeee`。
- `long_running` emit 时间：`real 0.13s, user 0.11s, sys 0.01s`。
- 输出 IR 未发现 `alloca` 或 `edge.*`。

当前方案评分：

- 实现效果：9/10。源码和输出都是真正 native PHI，真实样例通过 verifier。
- 复杂度：8/10。删除 mem2reg 后路径更直接，但更依赖 Gigahorse `PHIIncoming` 完整性；validator 已提前兜住。
- 维护成本：8/10。后续主要维护 `PHIIncoming` fact 质量，不再维护 slot/mem2reg 双路径。

下一步：

1. 把 Gigahorse `PHIIncoming` 补边修复纳入固定测试或生成流程，避免回退到缺边 facts。
2. 再跑更多真实合约，确认 `terminalStatement` 对 PHI/terminator 同地址的处理没有其它边界问题。

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

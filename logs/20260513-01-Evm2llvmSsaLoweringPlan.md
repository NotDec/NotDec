# evm2llvm SSA lowering 计划

## 总结

当前观察很简单：

- Gigahorse TAC 已经把 EVM 栈语义提升成 `TAC_Def` / `TAC_Use`，evm2llvm 不应该再重新推栈。
- `PHIIncoming.csv` 已经补上了 PHI 的 predecessor incoming 信息。
- 当前样本里 `TAC_Def` 满足“一变量一 def”：`long_running` 是 1667 个 def、1667 个唯一变量；现有 fixtures 也没有重复 def。
- 现在 evm2llvm 仍主要用 alloca/load/store slot 表达变量，PHI 是按边 store 到 phi def slot，不是 native LLVM phi。

目标形态：

evm2llvm 应该逐步改成基于 TAC 变量的 SSA lowering。普通 TAC def 直接产生 LLVM `Value*`，use 直接读取这个 `Value*`；PHI 用 `PHIIncoming.csv` 生成 native LLVM `phi`。短期可以保留 slot fallback，避免一次性改太大。

## 背景

Gigahorse 的输入是 EVM 栈机器，但它输出的 TAC 已经不是原始栈操作。对 evm2llvm 来说，更应该相信这些 facts：

- `TAC_Def(stmt, var, n)`：语句定义变量。
- `TAC_Use(stmt, var, n)`：语句使用变量。
- `PHIIncoming(phiStmt, block, predBlock, var)`：某个 PHI 在某条 predecessor 边上取哪个变量。

这里的 `stackIndex` 是 Gigahorse 内部恢复 PHI 的依据，不应该成为 evm2llvm lowering 的主要模型。evm2llvm 应该消费 TAC 变量和 CFG facts。

## 当前实现

当前 evm2llvm 是 slot-based：

```llvm
%x.slot = alloca i256
store i256 %v, ptr %x.slot
%u = load i256, ptr %x.slot
```

当前 PHI 处理已经比旧版本正确：

- 对无条件边，在 predecessor terminator 前把 incoming value store 到 phi def slot。
- 对条件边，必要时插入 `edge.<pred>.to.<succ>` block，只在实际走到这条边时 store。

这保证了 PHI incoming 选择按 CFG 边发生，但 IR 里还没有 native LLVM `phi`。

## 关键观察

### 1. TAC var 当前看起来是 SSA def

对 `/tmp/gigahorse-phiincoming-test/long_running/out/TAC_Def.csv` 的观察：

```text
TAC_Def 行数：1667
唯一 var 数：1667
```

当前 evm2llvm fixtures 也没有发现同一个 var 被不同 statement 定义多次。

这说明至少当前样本可以按 SSA 风格处理：

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

`PHIIncoming.csv` 只是把这层关系导出成 client 可读 facts：

```text
phiStmt    phiBlock    predBlock    incomingVar
```

因此 evm2llvm 不需要从变量名猜 predecessor，也不需要重新理解 stack index。

### 3. slot fallback 仍有价值

即使 TAC 当前看起来是 SSA，短期仍保留 fallback 更稳：

- 旧 Gigahorse / stock docker 可能没有 `PHIIncoming.csv`。
- 后续真实合约可能暴露 imprecise facts。
- private call、多返回、side-effect 指令还需要逐步改。

## 目标

短期目标是混合 SSA：

- 新增 `ValueMap`：`FactId -> llvm::Value*`。
- 普通表达式 def 直接写入 `ValueMap`。
- use 优先从 `ValueMap` 读取。
- 找不到值时再 fallback 到 slot 或常量。
- PHI 生成 native LLVM `PHINode`，再视需要 store 回 slot 兼容旧逻辑。

中期目标是大部分 TAC 变量不再分配 slot：

- 纯表达式、常量、普通 def/use 全走 SSA。
- private call 返回值走 SSA value / extractvalue。
- PHI 直接成为 LLVM native phi。
- slot 只保留给确实需要 fallback 的变量。

长期目标是完全移除普通 TAC 变量 slot：

- EVM memory/storage/calldata/env 仍作为 runtime state。
- TAC scalar value 全部是 LLVM SSA value。

## 技术路线

### 阶段 1：加 SSA ValueMap

在 lowering function 内维护：

```cpp
std::map<FactId, llvm::Value *> Values;
```

`loadWord(var)` 改成：

1. 先查 `Values[var]`。
2. 再查常量。
3. 再 fallback slot。
4. 都没有则报错。

`storeWord(var, value)` 暂时可以同时：

1. `Values[var] = value`
2. 如 slot 存在，继续 store

这样可以先不破坏旧逻辑。

### 阶段 2：native PHI

对有 `PHIIncoming` 的 TAC PHI：

1. 在 phi block 开头创建 LLVM `PHINode`。
2. 对每条 incoming：
   - 取 `incomingVar` 对应的 LLVM value。
   - `phi->addIncoming(value, predBB)`。
3. `Values[phiDef] = phi`。
4. 短期可继续 store 到 `phiDef.slot`，方便还没 SSA 化的 use。

这一步完成后，IR 里应出现真实：

```llvm
%x = phi i256 [ %a, %pred1 ], [ %b, %pred2 ]
```

### 阶段 3：普通表达式改为 SSA def

对 `ADD/SUB/MUL/MLOAD/CALLVALUE/...` 这些当前返回一个 value 的指令：

- lower 后直接 `Values[def] = value`。
- 不再强制 store 到 slot。
- 如果后续 fallback 还依赖 slot，可以临时保留 store，等验证稳定后删除。

### 阶段 4：private call 多返回 SSA

`CALLPRIVATE` 当前已经能返回 `void`、`i256` 或 struct。SSA 形态应改成：

- 单返回：`Values[retVar] = callResult`
- 多返回：`Values[retVarI] = extractvalue callResult, i`

### 阶段 5：减少 slot

当 fixtures 和真实样本验证稳定后：

- 不再为所有 TAC vars 创建 alloca。
- 只为 fallback 需要的变量创建 slot。
- 或者先保留一个 debug 开关，方便对比 slot/SSA 两种 lowering。

## 风险

1. **dominance 问题**

LLVM SSA value 必须支配所有 uses。Gigahorse TAC 如果有 imprecise CFG 或缺 PHI，native SSA 会比 slot 更容易失败。

2. **旧 facts 兼容**

没有 `PHIIncoming.csv` 时，native PHI 不能可靠生成。需要继续支持旧 fallback，并保留 wrapper warning。

3. **插边逻辑和 native PHI 冲突**

当前 edge-store 会插 edge block。native PHI 阶段要明确 incoming block 是原 predecessor 还是拆边后的 block。第一版 native PHI 建议不要为 PHI 再插 edge block。

4. **side-effect 指令**

`MSTORE/SSTORE/CALL/LOG/RETURN/REVERT` 不是纯表达式。SSA 化时要只把 scalar def 放入 `ValueMap`，不要把 state effect 当普通值。

## 判断标准

1. `TAC_Def` 唯一性检查通过：

```text
每个 var 最多一个 defining stmt
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

5. 与当前 slot edge-store 版本相比，输出不应更容易 verifier 失败。

## 当前建议

下一步不要直接删除 slot。先做混合 SSA：

```text
ValueMap 优先，slot fallback 保底。
```

这样可以验证 Gigahorse TAC 的 SSA 质量，也能保留旧 Gigahorse / stock docker 的可用性。等 native PHI 和普通表达式 SSA 都稳定后，再逐步减少 slot。


# Payability guard CFG rewrite 计划

## 背景

当前 `PayabilityGuardPass` 已经能识别 Solidity nonpayable 入口保护：

```text
callvalue == 0 ? continue : revert(0, 0)
```

或者等价方向：

```text
callvalue != 0 ? revert(0, 0) : continue
```

当前测试覆盖：

- `test/evm/solidity-patterns/manifest.json` 共 58 个真实 evm2llvm case。
- `payability` 覆盖 50 个 case。
- `nonpayable_guard` 覆盖 50 个 case。
- `expected_nonpayable_functions` 总数为 1514。
- `notdec.solidity.payability_guard` metadata 总命中数为 7570。

现有 rewrite surface 已经会在所有 `addStringMetadata` 命中点同步插入：

- `notdec_solidity_rewrite_payability_guard(kind)`。
- `notdec_solidity_rewrite_hidden(kind)`。
- `!notdec.solidity.rewrite_hidden.payability_guard`。

但这还只是“可隐藏 / 可改写标记”，没有真正调整 CFG。payability guard 是 Solidity 编译器稳定生成的外层包装，不应该长期停在只标记不改 CFG 的状态。

## Solidity 生成形态依据

本地 `/sn640/solidity` 中的 Yul / cmdline 测试输出能看到稳定形态：

```yul
if callvalue() { revert(0, 0) }
```

部分路径会通过错误 helper 表达，但语义仍然是：

```text
nonpayable entry rejects non-zero msg.value before进入函数主体。
```

因此 NotDec 侧可以针对 `callvalue + zero check + empty revert` 这一编译器模式做结构化 CFG rewrite。

## 目标

`PayabilityGuardPass` 的主体目标调整为 CFG rewrite：

1. 识别 public entry 开头的 nonpayable guard。
2. 保留函数级 `notdec.solidity.nonpayable` 和 guard 组成部分 metadata，作为测试锚点。
3. 将 guard conditional branch 改成直接跳转 success block。
4. 删除或留给后续 CFG cleanup 处理 failure empty revert block。
5. 让后续输出不再面对外层 `callvalue -> revert(0,0)` 低层结构。

metadata / rewrite marker 仍然保留，但定位是内部识别结果和回归测试依据，不是最终主体流程。

## 匹配对象

第一版只支持当前测试集中最稳定的直接形态：

```text
GuardBlock:
  %callvalue = evm_callvalue(...)
  %cmp = icmp eq/ne %callvalue, 0
  br i1 %cmp, SuccessBlock, FailureBlock

FailureBlock:
  evm_revert(%mem, 0, 0)
  unreachable

SuccessBlock:
  ...
```

需要支持两个分支方向：

- `callvalue == 0 ? success : failure`
- `callvalue != 0 ? failure : success`

第一版暂不处理：

- failure 经过中间 block 才到 `revert(0,0)`。
- `callvalue` 被复杂 bool 链包裹。
- 多个 guard 共用同一个 failure block 的删除。
- fallback / receive payable 状态无法确认的入口。
- 非空 revert、Panic、Error 或 custom error failure。

这些后续再按测试样例扩展。

## 实现路线

### 1. 抽出 Guard match 结构

把当前 `PayabilityGuardPass::run` 里的即时匹配逻辑抽成结构化结果，例如：

```cpp
struct PayabilityGuardMatch {
  llvm::BasicBlock *GuardBlock;
  llvm::BasicBlock *SuccessBlock;
  llvm::BasicBlock *FailureBlock;
  llvm::CallBase *CallValue;
  llvm::ICmpInst *Condition;
  llvm::BranchInst *Branch;
};
```

匹配函数只负责判断模式和返回 match，不做 CFG 修改。

### 2. 复用 match 结果做标注

命中后继续保留现有标注：

- 函数挂 `notdec.solidity.nonpayable = true`。
- `evm_callvalue` 标 `notdec.solidity.payability_guard = callvalue`。
- `icmp` 标 `notdec.solidity.payability_guard = condition`。
- branch 标 `notdec.solidity.payability_guard = branch`。
- failure block 中的 `revert(0,0)` / `unreachable` 标 `notdec.solidity.payability_guard = revert`。

这一步仍会触发现有 rewrite marker / hidden marker / hidden metadata，作为回归测试锚点。

### 3. CFG rewrite

第一版选择最小 CFG 改动：

```text
conditional branch -> unconditional branch to SuccessBlock
```

即：

```llvm
br i1 %cond, label %success, label %failure
```

改为：

```llvm
br label %success
```

优点：

- `GuardBlock` 保留，不需要重接所有 predecessor。
- `SuccessBlock` 的 PHI incoming 大概率不需要调整。
- dominance 风险小。
- `callvalue` / `icmp` 变成死代码后可由后续 DCE / SimplifyCFG 清理。

failure block 处理：

- 如果 `FailureBlock` 只有这个 guard 一个 predecessor，可以删除，或先交给后续 cleanup。
- 如果 `FailureBlock` 有多个 predecessor，第一版不删除，只移除当前 edge 后保持合法 IR。

### 4. 标记 CFG rewrite 成功

为测试框架增加一个明确可计数的成功信号。推荐插入一个专门 marker：

```llvm
call void @notdec_solidity_cfg_rewrite_payability_guard(i256 kind)
```

插入点建议在 rewrite 后的 `GuardBlock` terminator 前。

这样 runner 不需要解析复杂 CFG，就能确认每个 nonpayable guard 都发生过结构化 rewrite。

后续如果需要更强验证，可以再给新 unconditional branch 挂：

```text
!notdec.solidity.payability_cfg_rewritten
```

第一版用 marker 即可。

## 测试框架改动

### pattern suite 扩展

继续使用现有：

```text
test/evm/solidity-patterns/
test/run_evm_solidity_patterns_suite.py
```

在 runner 中增加：

```python
def count_payability_cfg_rewrites(path: Path) -> int:
    text = path.read_text()
    return text.count("call void @notdec_solidity_cfg_rewrite_payability_guard(")
```

对包含 `payability` 或 `nonpayable_guard` pattern 的 case，默认期望：

```text
payability_cfg_rewrites == expected_nonpayable_functions
```

这样 manifest 可以不立刻给 50 个 case 全部增加字段；只有特殊 case 需要 override 时，再加入：

```json
"expected_payability_cfg_rewrites": 5
```

### rewrite suite 关系

`test/evm/solidity-rewrite/` 当前主要验证 selector outline 结构。payability CFG rewrite 第一阶段不需要新建 suite，先放进 pattern suite 更合适，因为：

- payability 的 50 个 case 已经在 pattern manifest 中有 oracle。
- rewrite 后仍要保证 metadata / rewrite marker / hidden marker 数量不退化。
- 全量 pattern suite 能同时捕获对 ABI / revert / checked-bounds 等后续 pass 的影响。

如果后续真的删除 failure block、清理 PHI 或做更强 CFG 规整，再考虑新增 `solidity-rewrite-payability` 专项 suite。

## 验收指标

第一版 CFG rewrite 的验收标准：

1. `notdec.evm.solidity_patterns` 全量通过。
2. `notdec.evm.solidity_rewrite` 不退化。
3. 50 个 payability case 全部完成 CFG rewrite。
4. `expected_nonpayable_functions` 总数仍为 1514，不因 CFG rewrite 下降。
5. `payability_cfg_rewrites` 总数为 1514。
6. `notdec.solidity.payability_guard` metadata 总量仍与 oracle 一致，除非后续明确决定删除已 rewrite 的低层 guard metadata。
7. 所有输出 `.ll` 都能通过项目 LLVM 22 `llvm-as`。
8. 如果 `revert` / `checked_bounds` oracle 变化，必须能解释为 nonpayable guard 已被 payability pass 消费，而不是误删用户业务分支。

推荐第一轮具体命令：

```bash
cmake --build ./build --target all -j4
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
```

额外抽查：

```bash
./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll \
  -o /tmp/0011.payability-rewrite.ll --tr-level=0
./llvm-22.1.0.obj/bin/llvm-as /tmp/0011.payability-rewrite.ll \
  -o /tmp/0011.payability-rewrite.bc
```

抽查点：

- `public_*` nonpayable 函数仍有 `!notdec.solidity.nonpayable`。
- 有 `notdec_solidity_cfg_rewrite_payability_guard` marker。
- 原 guard conditional branch 已改成直接跳 success block。
- `revert(0,0)` failure block 不再从该 guard 可达。

## 风险和处理

- PHI 风险：第一版只把 guard branch 改成 unconditional branch，保留 `GuardBlock`，不绕过它，因此 PHI 风险较低。
- 后续 pass 依赖：现有 `revert` / `checked_bounds` 可能会少看到 nonpayable 的 empty revert。这是预期结果，需要更新 oracle 或在 rewrite 前先完成相关 pass。
- 共享 failure block：第一版不强删共享 failure block，避免误删其它路径使用的 reject。
- optimizer 变形：第一版只覆盖直接形态，后续根据失败 case 扩展 `getCallValueFromPredicate`。

## 当前判断

Payability guard 是 Solidity 编译器稳定生成的外层保护，适合比其它模式更早进入 CFG rewrite 阶段。相比 ABI / storage / external call，payability 的数据流简单、边界清楚、测试覆盖充足。当前应把 metadata-only 定位为内部识别步骤，把 CFG rewrite 作为主流程目标。

## 实现记录

### 已完成：Payability guard CFG rewrite

修改文件：

- `src/Passes/evm/SolidityPatterns.cpp`
  - 在文件前部新增 `PayabilityGuardMatch`，记录 guard block、success/failure block、`evm_callvalue`、`icmp` 和 branch。
  - 新增 `matchPayabilityGuard()`，只匹配 `callvalue == 0 ? success : failure` 和 `callvalue != 0 ? failure : success`，并要求 failure 是 `revert(0,0)` + `unreachable` 的直接 block。
  - 新增 `insertPayabilityCfgRewriteMarker()`，在 rewrite 前插入 `notdec_solidity_cfg_rewrite_payability_guard(i256)`。
  - 修改 `PayabilityGuardPass::run()`：命中后保留原有函数级 `notdec.solidity.nonpayable`、guard metadata、rewrite marker 和 hidden marker；随后把原条件 branch 替换为直跳 success block，并把旧 branch metadata 复制到新 branch，避免 `notdec.solidity.payability_guard` 计数下降。
- `test/run_evm_solidity_patterns_suite.py`
  - 新增 `count_payability_cfg_rewrites()`。
  - 对包含 `payability` 或 `nonpayable_guard` pattern 的 case 增加 `payability_cfg_rewrites` 检查；默认期望等于 `expected_nonpayable_functions`，可用 `expected_payability_cfg_rewrites` 覆盖。

验证结果：

```bash
cmake --build ./build --target all -j4
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure
./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll \
  -o /tmp/0011.payability-rewrite.ll --tr-level=0
./llvm-22.1.0.obj/bin/llvm-as /tmp/0011.payability-rewrite.ll \
  -o /tmp/0011.payability-rewrite.bc
```

结果：

- 构建通过。
- `notdec.evm.solidity_patterns` 通过，耗时约 87s。
- `notdec.evm.solidity_rewrite` 通过，耗时约 87s。
- manifest 中 58 个 case，50 个 payability/nonpayable case。
- manifest 中 `expected_nonpayable_functions` 总数为 1514。
- 输出中 `notdec_solidity_cfg_rewrite_payability_guard` 总数为 1514。
- 输出中 `!notdec.solidity.nonpayable` 总数为 1514。
- 输出中 `!notdec.solidity.payability_guard` 总数为 7570，与 oracle 一致。
- 所有 pattern suite 输出 `.ll` 已由 runner 用 `llvm-22.1.0.obj/bin/llvm-as` 验证通过。
- 抽查 `0011_multi_public`：`public_Withdraw___0x65` 仍有 `!notdec.solidity.nonpayable`；guard block 中有 `notdec_solidity_cfg_rewrite_payability_guard`；原 guard 条件跳转已变成 `br label %bb._0x71`；failure block `bb._0x6d` 变成 `No predecessors!`。

验收结论：

1. `notdec.evm.solidity_patterns` 全量通过。
2. `notdec.evm.solidity_rewrite` 未退化。
3. 50 个 payability case 都完成 CFG rewrite。
4. `expected_nonpayable_functions` 总数仍为 1514。
5. `payability_cfg_rewrites` 总数为 1514。
6. `notdec.solidity.payability_guard` metadata 总量仍为 7570。
7. 所有输出 `.ll` 都通过项目 LLVM 22 `llvm-as`。
8. `revert` / `checked_bounds` oracle 未变化；这次没有误删用户业务分支的计数信号。

复杂度评分：

- 实现效果：9/10。已达到第一版目标，failure block 暂时不删除，交给后续 cleanup。
- 理解成本：8/10。新增一个小 match 结构和一个 marker 插入函数，改动集中在 payability pass。
- 维护成本：8/10。runner 默认规则减少 manifest 逐项维护，后续特殊 case 仍可覆盖。

暂不做：

- 不删除 failure block。
- 不处理中间 block、复杂 bool 链和共享 failure block 的更强规整。
- 不修改 `revert` / `checked_bounds` oracle。

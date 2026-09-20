# 交接：EVM calldata 逐函数记录（per-function calldata record）

日期：2026-09-20　　分支：NotDec `v2`（工作区干净）

## TL;DR

EVM 链路的 Solidity 语义恢复已经做到"private helper 输出 + 局部变量 + linkage 分类 + post-TR
参数识别测量"都能跑通、四套 suite 全绿。**唯一未解的核心问题**是：类型恢复把**所有入口的
`%calldata` 指针合并成同一个模块级记录**，导致"按记录数 ABI 参数"不可用。

根因已定位并复现：共享 outline 解码 helper 的调用约束（实参 ≤ 形参，参数位置逆变）把每个
调用方的常量偏移字段要求汇进同一个形参。仓库里已有的 **多态标记**（`KIND_MLSUB_POLYMORPHIC_FUNCTION`）
正是为这种"每个调用点独立实例化"设计的机制：

- **只标记 internal helper**（已落地、四套 suite 全绿）：收益小（record==annotated 177→288，
  入口之间仍通过 dispatcher 混合）；
- **helper + public entry 都标记**：收益很大（60 个 case 上 record==annotated 1527、偏多
  2106→104，0009 与 annotated 完全一致），但 **case 24259 不收敛**（25 分钟 / 3.0GB 仍未结束）。

用户判断："理论上还是会收敛的"，所以下一步要**深入类型推理/约束传播算法**，搞清 24259 为什么
在这张图上停不下来。本文就是为此准备的交接材料。

## 1. 目标链（为什么需要 per-function calldata record）

1. Solidity 源码 → EVM 字节码 → Gigahorse → evm2llvm LLVM IR → NotDec（pre-TR pass → 类型恢复
   → post-TR pass）→ Solidity 后端。链路细节见根目录 `EVM-Arch.md`。
2. 目标：把 ABI 参数恢复出来（`slot_5 = arg0` 这种）。两条路：
   - **pre-TR 形状匹配**（已存在）：`EvmCalldataAccessPass` + `AbiDecodeResultPass` 打
     `notdec.solidity.calldata.index` 标注（88 个 case / 1782 处，全部在 public entry 内）；
   - **post-TR 结构识别**（本轮新增）：用类型恢复出的 calldata 记录（`HTypeBufferView`）识别参数。
     规划里要求后者逐步取代前者（`EVM-Arch.md` 第 3 节有退役三步）。
3. 为什么后者卡住：记录是模块级的（见第 3 节），无法描述单个入口的参数表。

## 2. 现状快照

### 已交付（都已提交并 push）

| 提交 | 内容 |
| --- | --- |
| `b4f4739c`（顶层）+ llvm2c `f4bde15` | **Route B**：private helper 输出成 Solidity internal 函数 + SSA 折叠/局部变量（移植 C 后端 `addExprOrStmt` 判断流程） |
| `3b78d89b` + llvm2c `0eb74e9` | SSA 使用分析抽成共享头文件 `include/notdec-backends/Core/ValueUseAnalysis.h`（C/Solidity 两个后端共用） |
| `d19e0a97` | 根目录 `EVM-Arch.md`：整条 EVM 链路 + pre-TR/post-TR 职责 + IR 质量契约 |
| `5d11e87`（evm2llvm） | 前端按 `TacFunction::IsPublic` 生成 linkage：ABI 表面 external、outline helper internal |
| `93c1ea49`、`447e882b` | 消费端只按 internal linkage 判 helper；测试语料 86 个文件 / 3438 个 `private_*` 定义迁移为 internal |
| `d03a9a0d` | `calldata.index` 过渡态定位 + 退役路径（冻结扩展） |
| `fbdadd10` | `AbiParamRecoveryPass`（post-TR 识别 + pattern suite oracle），**只测量不改写** |
| `6b7754a8` | calldata 记录并集的根因 + 被否决的 one-way edge 修法 |
| `9398ac50` | **多态标记（helper 变体）落地**：所有 internal calldata helper 无条件标多态；4 个 case 的 `abi_return` oracle +2 |
| `fe7f3020`、`0d1e576d` | 多态实验数据 + 24259 profiling，entry 变体保持禁用 |

### 当前基线（103 个 pattern case）

- 四套 suite 全绿：`notdec.evm.solidity_patterns` / `_pattern_compile` / `_rewrite` / `_source`（source 89/89 含 solc）；
- compile 预算（`test/evm/solidity-patterns/compile-budget.json`）：
  unresolved **264**、condition TODO **729**、goto **424**、real jump **343**、dangling **69**、
  helper 函数 **1339**、helper 调用点 **1301**、编译失败 0；
- `AbiParamRecoveryPass` 的三个 metadata（见第 3 节）在 103 个 case 上：
  `record == annotated` **288**、record 偏多 **1911**、偏少 **117**、只有 record **1079**、
  单值 case **92/103**。

### 环境 / 仓库状态

- 构建：`cmake -S . -B build-notdec-nothreads -DNOTDEC_SOLC=/sn640/EthIR/source/solcv8.25` +
  `ninja -C build-notdec-nothreads notdec`；测试 `ctest --test-dir build-notdec-nothreads -R "notdec.evm.solidity"`。
  （`build-notdec-nothreads2` 已失效，缺 `llvm-22.1.0.nothreads.obj`，不要用。）
- **type-recovery 的 EVM golden suite 目前是红的**：`notdec.type_recovery.evm.tr_level_2` 失败，
  原因是另一个会话的提交 `53a9bdca`（MLsub allocator wrapper 检测）让 `.htypes` 多出
  `bytes: N` 标注而 expected 未更新。**与 calldata 无关**，但动类型恢复时要一并处理。
- 工作区干净；`external/NotDec-bin2llvm` 指针脏是历史遗留，不要动。

## 3. 核心问题：calldata 记录是模块级并集

### 机制（已用最小复现确认）

- 类型恢复对**共享 outline helper** 的调用约束：`MLsubVisitor::visitCallBase`（`src/TypeRecovery/mlsub/MLsubGenerator.cpp`）
  → `deferCallConstraint` → `applyDeferredCallConstraints` 里 `addSubtype(F, ActualFunc)`
  → 参数位置逆变 ⇒ **调用方实参 ≤ helper 形参**；
- 每个入口自己的常量偏移 calldata 读取（`EvmCalldataAccessPass` 重写成
  `inttoptr(add(ptrtoint(%calldata), C))` + `load`）会在这个形参上留下字段要求；
- 于是 helper 的 `%calldata` 形参 = 所有调用点字段的**并集**，而它同时是各调用方的上界，
  所以每个入口打印出来都是同一份记录；
- 另外：ABI entry 是被 dispatcher（`public___function_selector___0x0`）用**dispatcher 自己的
  `%calldata`** 调用的，所以 entry 之间也会通过 dispatcher 实参继续混合。

### 最小复现（可自包含，建议固化成 TR 回归用例）

```llvm
; helper 在 calldata+4 读一个字；入口 A 只经 helper 解码；入口 B 自己读 calldata+36 并也调 helper
target datalayout = "E-p:256:256-i256:256:256-S256-a:256:256"
target triple = "evm-unknown-unknown"
declare i256 @evm_calldatasize(ptr)
declare ptr @notdec_evm_calldata_min_size(ptr, i256)
declare void @evm_sstore(i256, i256)

define internal i256 @shared_decode(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %base, i256 %size, i256 %pc) {
bb:
  %base.int = ptrtoint ptr %calldata to i256
  %addr = add i256 %base.int, %base
  %p = inttoptr i256 %addr to ptr
  %v = load i256, ptr %p, align 1
  ret i256 %v
}
define void @public_one_word_uint256__0x10(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %r = call i256 @shared_decode(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 4, i256 %size, i256 0)
  call void @evm_sstore(i256 0, i256 %r)
  ret void
}
define void @public_two_words_uint256_uint256__0x20(ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
entry:
  %checked = call ptr @notdec_evm_calldata_min_size(ptr %calldata, i256 68)
  %base = ptrtoint ptr %checked to i256
  %addr = add i256 %base, 36
  %p = inttoptr i256 %addr to ptr
  %v = load i256, ptr %p, align 1
  call void @evm_sstore(i256 1, i256 %v)
  %size = call i256 @evm_calldatasize(ptr %calldata)
  %r = call i256 @shared_decode(ptr %mem, ptr %calldata, ptr %returndata, ptr %env, i256 %size, i256 0)
  call void @evm_sstore(i256 2, i256 %r)
  ret void
}
```

复现命令与判读：

```bash
build-notdec-nothreads/bin/notdec /tmp/repro3.ll -o /tmp/repro3.sol --tr-level=2 --dump-htypes=/tmp/repro3.htypes
grep -E "::arg1 =>" /tmp/repro3.htypes
```

- 现状（helper 不标多态 / 原始）：A、B、helper 三者都是 `struct {field@4, field@36}`（并集）；
- helper 标多态后：A=`{4}`、B=`{4,36}`、helper=`{4}`（逐调用点，正确）。

### 测量工具（已就位）

`AbiParamRecoveryPass`（`src/Passes/evm/solidity-patterns/AbiParamRecoveryPass.cpp`，post-TR，
注册在 `PassManager.cpp` 的 EVM post-TR 段）给每个 public entry 写三个函数级 metadata：

- `notdec.solidity.abi_param.record`：恢复出的 calldata 记录里 `4+32*i` 字段数（上界）；
- `notdec.solidity.abi_param.annotated`：pre-TR `calldata.index` 推出的逐函数参数个数；
- `notdec.solidity.abi_param.mismatch`：两者不一致时的 `record=N annotated=M`。

pattern suite 已为 `0011/0009/0031` 配置 `expected_metadata_string_values`；
`METADATA_ONLY_KINDS` 已包含这三个 kind。**用它的聚合数字当"逐函数记录是否收敛"的标尺**。

### 多态标记的现状

- 位置：`src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp`
  - `shouldMarkPolymorphicFunction(F)`：当前 = `detail::isEvmPrivateHelperFunction(F)`（只有 internal helper）；
  - `markPolymorphicHelpers(M)`：**已去掉**原来的 `hasPolymorphicCallsiteOffset` 门控，所有读
    calldata 的 internal helper 都标记；metadata 值区分 `calldata_offset`/`calldata_buffer`（诊断用）。
- 消费端：`MLsubRecovery` 的 SCC 划分里 `hasPolymorphic()`（`MLsubGenerator.cpp` 约 7482 行）
  命中后 "the SCC boundary instantiates each callsite independently"。
- 变体对比（103 case / 2316 入口）：

| 变体 | exact | 偏多 | 偏少 | record_only | 单值 case | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| 原始 | 177 | 2106 | 33 | 1179 | 102/103 | — |
| helper 标（已落地） | 288 | 1911 | 117 | 1079 | 92/103 | 四套 suite 全绿 |
| helper+entry 标（禁用） | **1527**（60 case 部分） | **104** | 282 | 104 | 10/60 | **24259 不收敛** |

entry 变体的实现（要复现时把 `shouldMarkPolymorphicFunction` 临时改成）：

```cpp
bool shouldMarkPolymorphicFunction(Function &F) {
  if (F.isDeclaration() || F.arg_size() < 2) return false;
  auto It = F.arg_begin(); ++It;
  return It->getType()->isPointerTy() && It->getName() == "calldata";
}
```

## 4. 未解问题：entry 多态下 24259 不收敛

现象（多次复现）：

- case：`test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll`；
- `timeout 1500` 仍不结束（25 分钟），`User time 419s+`、99% CPU、峰值 RSS **3.0GB**；
  helper-only 下同 case 约 1 秒、RSS 65MB；
- 前 50 秒 RSS 519MB→844MB 后**平台化**；
- 调用图**无环**（25 函数、18 个带 calldata、0 cycle），排除源级递归；
- `NOTDEC_CONSTRAINT_DIAG=1`：SCC 自底向上阶段正常完成（`bottom_up_done=42/42`，
  各子阶段 `wall_ms≈0`），日志停在 post-summary 三条 merge policy 打印之后；
- perf（`perf record -F 199 -g`）self 占比：
  `binarysub::skip_cached_or_materialized_variable_edge` **62.4%**、
  `binarysub::resolve_variable` 12.3%、
  `ConstraintsGenerator::hasStructPointerEvidenceInBound` 3.2%，
  其余在 `constrain_worklist_only` / `merge_variable_into_impl` / `register_nested_bound_uses_impl`。

判读：热点函数本身极小（pair 哈希查找 + `hasUpperBound`/`hasLowerBound` 两个
**unordered_set::count**，O(1)，见 `external/binarysub/include/binarysub/binarysub-core.h:190`），
所以问题是**同一条变量边被反复传播**，不是某处线性扫描。用户指出"理论上应该收敛"，
所以下一步要**进入算法内部**回答两个问题：

1. 图是否还在增长？——entry 多态会按调用点实例化整份函数摘要；如果每个合并轮次/证据变化都
   **重新实例化**（新 variable id），就永远收敛不了（RSS 3GB 也支持这一点）。
2. 若图已稳定，为什么 worklist 还在反复跑？——看 `constrain_worklist_only` 的
   `Cache` 生命周期（是跨轮共享还是每次 `constrain` 新建）、`mergePolicyRechecksOnEvidenceChange`
   与 merge policy 的收敛条件（`applyCallInterfaceMergePolicy` / `applyStructPtrLoadStoreMergePolicy`）。

### 建议的下一步（按顺序，每步都有可验证产出）

1. **给约束引擎加"周期性状态快照"**（binarysub + MLsub）：每 N 次 worklist 迭代或每 T 秒打印
   （a）`TypeNode`/变量总数、（b）worklist 长度、（c）`Context->stats` 里的
   `constraintTasksEnqueued` / `maxConstraintWorklistDepth` / `directVariableEdgeFastPathHits`、
   （d）merge policy 轮次与合并数、（e）`Cache` 大小。
   跑 entry 变体的 24259 2-3 分钟：**数字持续增长 = 实例化/建图不收敛；数字稳定 = 重复传播不收敛**。
   （现有 env：`NOTDEC_CONSTRAINT_DIAG`、`NOTDEC_CONSTRAINT_STATS`、`NOTDEC_SLOT_MERGE_DIAG`、
   `NOTDEC_SIMPLIFY_DIAG`、`NOTDEC_CONVERT_PROGRESS`，可先复用。）
2. **按第 1 步结论分支**：
   - 持续增长 → 找"每轮重新实例化/新建变量"的代码路径（SCC 实例化、merge policy 重建、
     `rebuildBoundSets`/transaction rollback 相关），加缓存/记忆化（按 (callee, call site, 轮次) 复用实例）；
   - 稳定但反复传播 → 找 worklist 的去重边界（`Cache` 是否跨轮、结构性边是否只按 pair 去重、
     merge policy 是否会来回震荡），必要时给合并加"单调性/上限"条件。
3. **收窄多态范围**（可与 1/2 并行）：只实例化 **calldata 实参这一条边**（而不是整份函数摘要），
   或只对"仅被 dispatcher 调用一次、无递归 SCC"的 entry 标记，目标是在不放大整图的前提下拿到
   逐函数记录。
4. 达到逐函数记录后：打开 entry 变体 → 重跑
   `python3 test/run_evm_solidity_patterns_suite.py ...`（或 ctest）更新
   `0011/0009/0031` 的 `abi_param.*` oracle → 四套 suite 全绿 → 按结果收紧预算。
5. 之后再谈"用记录写 ABI 形参签名"（现在的 `AbiParamRecoveryPass` 刻意不改签名）。

### 复现命令

```bash
# entry 变体（临时改 shouldMarkPolymorphicFunction 后）
ninja -C build-notdec-nothreads notdec
timeout 1500 build-notdec-nothreads/bin/notdec \
  test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll \
  -o /tmp/24259.sol --tr-level=2
# 状态快照 / 诊断
NOTDEC_CONSTRAINT_DIAG=1 ... > /tmp/diag.log 2>&1
perf record -F 199 -g -o /tmp/perf.data -p <pid> -- sleep 45 && perf report --stdio -i /tmp/perf.data --sort=symbol
```

## 5. 已否决 / 已排除的路径（不要重复）

1. **IR 层内联 private helper（Route A）**：pattern suite 103→32、`// goto` 423→4755，已回滚。
   结论在 `logs/20260917-04-evm-solidity-next-steps.md`。
2. **在 `visitCallBase` 给 calldata 实参加"单向边"（改约束框架）**：record==annotated 177→1463、
   偏多 2106→2，但偏少 33→851，且 0032 的 `abi_return` 46→42（真回归）。补丁形态见
   `logs/20260918-02-*` 的"根因调查"节；**用户明确要求不要改约束推理框架、不要给架构加额外条件**。
3. **无条件标记所有 calldata 函数（entry 变体）**：效果最好但不收敛，见第 4 节。
4. 名字前缀判据（`private__`）已彻底移除，语料已迁移为 linkage 判据，不要再引入名字判断。

## 6. 坑与注意事项

- **改类型恢复前先看** `logs/20260918-01-htype-frozen-chain-and-width.md` 与提交 `53a9bdca`
  （另一个会话的 MLsub allocator wrapper 改动）；`notdec.type_recovery.evm.tr_level_2` 目前因
  `.htypes` 的 `bytes: N` 标注与 expected 不一致而失败，动 TR 时要么一起更新 expected，要么
  先确认不是你引入的新差异。
- **测试语料已迁移**：`test/**/*.ll` 里 3438 个 `private_*` 都是 internal linkage；
  新生成的 IR 由 evm2llvm 直接产出 internal（`5d11e87`），不要再按名字分类。
- **预算棘轮**：`compile-budget.json` 的 8 项（含 2 个下限）只降不升；升高必须在日志说明。
- **`calldata.index` 是冻结的过渡态**：不要再扩大它的覆盖；退役三步写在 `EVM-Arch.md` 第 3 节。
- 提交约定：submodule 先提交再顶层指针；`external/NotDec-bin2llvm` 指针脏，不要动。

## 7. 关键文件 / 符号索引

| 位置 | 作用 |
| --- | --- |
| `EVM-Arch.md` | 整条 EVM 链路、pre-TR/post-TR 分工、IR 质量契约、linkage 规则 |
| `logs/20260918-02-calldata-index-abi-params-plan.md` | 本任务全部详细记录（机制、实验、数字、否决路径） |
| `src/Passes/evm/solidity-patterns/AbiParamRecoveryPass.cpp` | post-TR 识别 + oracle（测量） |
| `src/Passes/evm/solidity-patterns/EvmCalldataAccessPass.cpp` | calldata 访问重写、`calldata.index` 标注、多态标记 |
| `src/Passes/evm/solidity-patterns/AbiDecodeResultPass.cpp` | 解码 helper 结果 → `calldata.index` |
| `src/TypeRecovery/mlsub/MLsubGenerator.cpp` | `visitCallBase`/`deferCallConstraint`/`applyDeferredCallConstraints`、`hasPolymorphic`、`markBuiltinPolymorphicBufferFunctions`、`NOTDEC_*` 诊断 env |
| `external/binarysub/src/binarysub-core.cpp` | `skip_cached_or_materialized_variable_edge`（热点）、`constrain_worklist_only` |
| `external/NotDec-llvm2c/lib/Solidity/{Reader,BodyBuilder,Printer}.cpp` | Solidity 后端（helper 输出、`argN` 消费、`AbiParamRecoveryPass` 的消费者） |

## 8. 交接清单

- [x] 代码 / 语料 / 预算 / 文档全部提交并 push（NotDec `v2`、llvm2c `main`、evm2llvm `main`）；
- [x] 工作区干净，entry 变体禁用且注释写明原因；
- [x] 复现材料与测量工具就位（`AbiParamRecoveryPass` + 三个 case 的 oracle + 本文的最小复现）；
- [ ] **待接手**：第 4 节的 5 步（加状态快照 → 定位增长源 → 收窄多态范围 → 打开 entry 变体并更新
      oracle → 之后再做形参签名）。

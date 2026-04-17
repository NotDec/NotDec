# MLsub 剩余阶段 B 与回归接线分步计划

日期：2026-04-17

## 1. 当前状态

围绕这两份主计划：

1. [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
2. [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)

当前已经完成的部分基本包括：

1. `NOTDEC_EXTRA_CONSTRAINTS` 入口已经接通
2. `ir_anchor.sha256` 校验已经落地
3. `target.kind = arg | ret | named_value | inst | operand | binding`
   已支持
4. workdir 中的 `SelectableValues.txt` 已导出
5. `--emit-tr-input-ir=<path>` 已作为阶段 A 正式 CLI 落地

当前仍然没有正式收口的核心剩余项，是 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
里明确保留的：

1. “阶段 B 只接受冻结 IR”
2. “启用 JSON 约束注入时，不再重新跑阶段 A 的结构改写 pass”
3. “把两阶段工作流真正接成可回归、可文档化的正式入口”

补充现状：

1. 当前 `fortune.o3.wasm.1.ll` 在 `--tr-level=2` 下已经可以跑通并产出
   `.htypes`
2. 但 `test/type-recovery/howard-o3-split/manifest.json` 中它仍然是 `skip`
3. 也就是说，当前更像是“能力已具备，但工作流和回归还没有正式收口”

## 2. 剩余任务的推荐拆分

我建议把剩余工作拆成下面 5 步，按顺序推进。

### 2.1 第一步：把“阶段 B”单独收紧成明确模式

目标：

1. 当启用 `NOTDEC_EXTRA_CONSTRAINTS` 时，不再把当前运行理解为“原始输入 +
   pre-TR 规范化 + MLsub”
2. 而是明确理解为“冻结 IR + MLsub”

推荐动作：

1. 新增一个显式的运行语义开关，表示“输入已经是冻结后的 TR input IR”
2. `NOTDEC_EXTRA_CONSTRAINTS` 启用时强制要求该模式打开
3. 没有打开时直接报错，而不是继续隐式复用当前 `tr-level>=2` 的旧路径

这样做的原因：

1. 现在只有 `ir_anchor.sha256` 校验，还没有把“阶段 B 不再重跑阶段 A”
   这层语义真正落实到 pipeline
2. 如果还继续让阶段 B 隐式复用现有 `build_passes(tr-level>=2)`，那么
   selector 绑定的虽然是冻结 IR，但执行时却仍然可能重新经历一遍结构改写

预计修改点：

1. [include/notdec/DecompilerContext.h:14](/sn640/NotDec/include/notdec/DecompilerContext.h#L14)
   到 [include/notdec/DecompilerContext.h:21](/sn640/NotDec/include/notdec/DecompilerContext.h#L21)
   - `notdec::Options`
2. [src/NotDec.cpp:112](/sn640/NotDec/src/NotDec.cpp#L112)
   到 [src/NotDec.cpp:170](/sn640/NotDec/src/NotDec.cpp#L170)
   - 命令行参数与组合校验
   - `main()`
3. [include/notdec/Passes/PassManager.h:103](/sn640/NotDec/include/notdec/Passes/PassManager.h#L103)
   到 [include/notdec/Passes/PassManager.h:123](/sn640/NotDec/include/notdec/Passes/PassManager.h#L123)
   - `DecompileConfig::build_passes()`

### 2.2 第二步：把 pass pipeline 拆成“阶段 A 规范化”和“阶段 B MLsub”

目标：

1. 让 `LinearAllocationRecovery`、`MemsetMatcher`、`MemcpyMatcher`、
   `UndoInstCombine`、`BDCE`、`ReorderBlocksPass` 明确只属于阶段 A
2. 当输入已经是冻结 IR 时，阶段 B 直接进入 `MLsubRecoveryMain`
3. 避免对同一份 IR 再次重复跑阶段 A

推荐动作：

1. 在 `PassEnv::build_passes()` 里把 pre-TR 规范化段抽成可复用的单独分支
2. 新增“只跑 MLsub / top-down / opt 后处理”的入口
3. 对冻结 IR 输入，不再依赖 `level >= 2` 触发那一整段结构改写

这样做的原因：

1. 当前 `PassEnv::build_passes()` 在
   [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
   到 [src/Passes/PassManager.cpp:390](/sn640/NotDec/src/Passes/PassManager.cpp#L390)
   里仍把“阶段 A + 阶段 B”绑在同一段 `level >= 2` 逻辑里
2. `--emit-tr-input-ir` 只是把阶段 A 提前导出了，但运行阶段 B 时并没有真正跳过
   这段 pass

预计修改点：

1. [include/notdec/Passes/PassManager.h:76](/sn640/NotDec/include/notdec/Passes/PassManager.h#L76)
   到 [include/notdec/Passes/PassManager.h:82](/sn640/NotDec/include/notdec/Passes/PassManager.h#L82)
   - `PassEnv::build_passes()`
2. [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
   到 [src/Passes/PassManager.cpp:390](/sn640/NotDec/src/Passes/PassManager.cpp#L390)
   - `PassEnv::build_passes()`
3. 如果需要保留现有 `tr-level=2/3` 语义，则同步补
   [src/Passes/PassManager.cpp:395](/sn640/NotDec/src/Passes/PassManager.cpp#L395)
   到 [src/Passes/PassManager.cpp:424](/sn640/NotDec/src/Passes/PassManager.cpp#L395)
   - `PassEnv::add_llvm2c()`
   - `PassEnv::emit_tr_input_ir()`

### 2.3 第三步：把阶段 B 的输入校验补完整

目标：

1. 启用 `NOTDEC_EXTRA_CONSTRAINTS` 时，只接受 `.ll/.bc`
2. 错误信息直接说明：
   - 当前输入类型
   - 为什么不允许
   - 推荐先运行的阶段 A 命令
3. 如果输入是 `.ll/.bc` 但明显不是冻结 IR，也尽量在更前面暴露问题

推荐动作：

1. 在 `main()` 的输入后缀校验阶段先收紧文件类型
2. 在进入 `MLsubRecovery::run()` 前补阶段 B 专用诊断
3. 保留当前 `validateExtraConstraintsFile()` 的 anchor 校验，作为最终兜底

这样做的原因：

1. 现在 `.wasm/.wat` 仍然可以在 `tr-level>=2` 下进入恢复链路
2. 只有到了
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1468](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1468)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1492](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1492)
   的 `validateExtraConstraintsFile()` 才会做 anchor 严格校验
3. 这会让“阶段 B 是冻结 IR 专用模式”显得太隐式

预计修改点：

1. [src/NotDec.cpp:145](/sn640/NotDec/src/NotDec.cpp#L145)
   到 [src/NotDec.cpp:200](/sn640/NotDec/src/NotDec.cpp#L200)
   - `main()`
2. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1399](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1399)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1406](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1399)
   - `MLsubRecovery::run()`
3. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1468](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1468)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1492](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1468)
   - `MLsubRecovery::validateExtraConstraintsFile()`

### 2.4 第四步：补一个正式的两阶段端到端回归

目标：

1. 把“阶段 A 导出冻结 IR”
2. “阶段 B 在冻结 IR 上应用 extra constraints”
3. 这整条链路变成仓库内可重复执行的正式验证项

推荐动作：

1. 先选一个小 LLVM IR case，而不是直接拿 `fortune`
2. 新增一份最小 extra-constraints JSON 固件
3. 覆盖：
   - `--emit-tr-input-ir`
   - workdir 中 `02-mlsub-input.anchor.json`
   - `SelectableValues.txt`
   - `NOTDEC_EXTRA_CONSTRAINTS`
   - 最终 `.htypes` 或日志变化

这样做的原因：

1. 当前 selector / anchor / CLI 虽然都各自验证过，但还没有被收成一条正式回归
2. 如果先做这一步，后面再把 `fortune` 提升为 `xfail/pass`，风险会小很多

预计修改点：

1. [test/run_type_recovery_suite.py:266](/sn640/NotDec/test/run_type_recovery_suite.py#L266)
   到 [test/run_type_recovery_suite.py:430](/sn640/NotDec/test/run_type_recovery_suite.py#L266)
   - 如需扩 runner，则修改 `main()`
2. [test/type-recovery/llvm-ir/manifest.json](/sn640/NotDec/test/type-recovery/llvm-ir/manifest.json)
   或新增一个更合适的小型 suite
3. 新增固定输入、固定 JSON、固定 expected snapshot

### 2.5 第五步：把 `fortune.o3.wasm.1.ll` 正式推进到回归状态

目标：

1. 先把当前“人工可运行”的状态升级成“有 manifest 状态定义”
2. 再决定是先记成 `xfail` 还是直接提成 `pass`

推荐动作：

1. 先基于当前运行结果给 `fortune.o3.wasm.1` 补一份基线记录
2. 如果输出稳定，就在
   [test/type-recovery/howard-o3-split/manifest.json](/sn640/NotDec/test/type-recovery/howard-o3-split/manifest.json)
   中把它从 `skip` 提升到 `pass`
3. 如果仍有输出抖动或已知语义缺口，就先记成 `xfail` 并写清原因

这样做的原因：

1. 当前关注 case 已经能跑通，但 suite 中仍完全不可见
2. 不把它推进到 manifest 状态，后续关于 builtin summary、extra constraints、
   冻结 IR 工作流的改动都很难稳定追踪回归

预计修改点：

1. [test/type-recovery/howard-o3-split/manifest.json:1](/sn640/NotDec/test/type-recovery/howard-o3-split/manifest.json#L1)
   到 [test/type-recovery/howard-o3-split/manifest.json:20](/sn640/NotDec/test/type-recovery/howard-o3-split/manifest.json#L1)
2. `test/type-recovery/howard-o3-split/expected/tr-level-2/`
   下的 `fortune.o3.wasm.1.htypes`
3. 必要时补一份解释该 case 当前状态的日志

## 3. 推荐执行顺序

建议实际实现时按下面顺序推进：

1. 先做“阶段 B 模式 + pipeline 拆分”
2. 再做“输入校验与用户诊断”
3. 然后补“一条小型两阶段端到端回归”
4. 最后再把 `fortune.o3.wasm.1` 提升到 `xfail/pass`

原因：

1. 如果先碰 `fortune`，很容易把当前工作流缺口掩盖成单 case 特判
2. 先把阶段职责切干净，后面的 case 回归才有稳定语义

## 4. 每一步完成后的日志建议

为了和当前仓库日志习惯保持一致，建议后续每完成一小步就单独补一篇日志：

1. 阶段 B 模式收紧
2. pipeline 拆分
3. 输入校验与报错
4. 两阶段端到端回归
5. `fortune.o3.wasm.1` manifest 提升

每篇日志里都应明确写：

1. 具体修改文件
2. 具体行号
3. 涉及函数
4. 验证命令
5. 当前仍未解决的边界

## 5. 当前判断

如果只看收益和风险比，接下来最应该做的不是继续扩 selector 语法，也不是立刻
继续给 `fortune` 手补约束，而是先把“阶段 A / 阶段 B”彻底切开。

这一步做完后：

1. `--emit-tr-input-ir` 才算真正成为阶段 A
2. `NOTDEC_EXTRA_CONSTRAINTS` 才算真正绑定到阶段 B
3. 后续无论是小 case 回归，还是 `fortune` 这类 howard case 的推进，都会更稳

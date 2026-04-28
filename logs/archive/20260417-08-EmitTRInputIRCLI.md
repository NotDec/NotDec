# `--emit-tr-input-ir` 阶段 A 导出 CLI

日期：2026-04-17

## 1. 本次范围

沿着 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
继续把“阶段 A：冻结类型恢复输入 IR”这一步真正做成可用 CLI。

本次新增：

1. `--emit-tr-input-ir=<path>`
2. 运行 pre-type-recovery 标准化 pass 后导出 `.ll/.bc`
3. 导出后直接退出，不进入 `MLsubRecoveryMain`
4. 如果配置了 workdir，则继续导出：
   - `02-mlsub-input.ll`
   - `02-mlsub-input.anchor.json`
   - `SelectableValues.txt`

## 2. 具体修改

### 2.1 CLI 与配置

1. [src/NotDec.cpp:112](/sn640/NotDec/src/NotDec.cpp#L112)
   到 [src/NotDec.cpp:119](/sn640/NotDec/src/NotDec.cpp#L112)
   - 新增命令行参数：
     - `--emit-tr-input-ir=<path>`

2. [src/NotDec.cpp:149](/sn640/NotDec/src/NotDec.cpp#L149)
   到 [src/NotDec.cpp:159](/sn640/NotDec/src/NotDec.cpp#L149)
   - 增加 CLI 组合约束：
     - 不能和 `-o` 混用
     - 不能和 `--dump-htypes` 混用

3. [include/notdec/DecompilerContext.h:20](/sn640/NotDec/include/notdec/DecompilerContext.h#L20)
   - `notdec::Options` 新增：
     - `emitTRInputIR`

### 2.2 pass pipeline

1. [include/notdec/Passes/PassManager.h:76](/sn640/NotDec/include/notdec/Passes/PassManager.h#L76)
   到 [include/notdec/Passes/PassManager.h:82](/sn640/NotDec/include/notdec/Passes/PassManager.h#L76)
   - `PassEnv::build_passes()` 新增 `stopBeforeTypeRecovery` 参数
   - `PassEnv` 新增：
     - `emit_tr_input_ir()`

2. [include/notdec/Passes/PassManager.h:103](/sn640/NotDec/include/notdec/Passes/PassManager.h#L103)
   到 [include/notdec/Passes/PassManager.h:123](/sn640/NotDec/include/notdec/Passes/PassManager.h#L103)
   - `DecompileConfig::build_passes()` 中：
     - 如果配置了 `emitTRInputIR`
     - 则把有效 `tr-level` 至少提升到 `2`
     - 并在 pre-TR 标准化 pass 后停止
   - `DecompileConfig` 新增：
     - `emit_tr_input_ir()`

3. [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
   到 [src/Passes/PassManager.cpp:365](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
   - `PassEnv::build_passes()` 在加入：
     - `LinearAllocationRecovery`
     - `MemsetMatcher`
     - `MemcpyMatcher`
     - `UndoInstCombine`
     - `BDCE`
     - `ReorderBlocksPass`
   - 之后，如果 `stopBeforeTypeRecovery=true`，则直接返回
   - 涉及函数：
     - `PassEnv::build_passes()`

4. [src/Passes/PassManager.cpp:417](/sn640/NotDec/src/Passes/PassManager.cpp#L417)
   到 [src/Passes/PassManager.cpp:424](/sn640/NotDec/src/Passes/PassManager.cpp#L417)
   - 新增 `PassEnv::emit_tr_input_ir()`
   - 统一转调 `MLsubRecovery::emitTRInputArtifacts()`

### 2.3 `MLsub` 输入产物导出复用

1. [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:387](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L387)
   - `MLsubRecovery` 新增：
     - `emitTRInputArtifacts()`

2. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:284](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L284)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:313](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L284)
   - 新增 `writeTRInputModule()`
   - 支持导出：
     - `.ll`
     - `.bc`

3. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1375](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1375)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1388](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1375)
   - `MLsubRecovery::run()` 不再自己重复拼装这批导出逻辑
   - 改为复用：
     - `emitTRInputArtifacts()`

4. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1495](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1495)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1511](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1495)
   - 实现 `MLsubRecovery::emitTRInputArtifacts()`
   - 统一负责：
     - workdir 中的 `02-mlsub-input.ll`
     - workdir 中的 `02-mlsub-input.anchor.json`
     - workdir 中的 `SelectableValues.txt`
     - 用户指定路径上的 `.ll/.bc` 导出

### 2.4 文档

1. [DEBUG.md:11](/sn640/NotDec/DEBUG.md#L11)
   到 [DEBUG.md:22](/sn640/NotDec/DEBUG.md#L11)
   - 在基本用法里加入 `--emit-tr-input-ir`

2. [DEBUG.md:69](/sn640/NotDec/DEBUG.md#L69)
   到 [DEBUG.md:74](/sn640/NotDec/DEBUG.md#L69)
   - 说明 `02-mlsub-input.ll` 既可能来自 `MLsubRecovery::run()`
   - 也可能来自 `--emit-tr-input-ir`

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 直接导出

执行：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --emit-tr-input-ir=/tmp/notdec-emit-tr-input/out.ll \
  --tr-level=1
```

结果：

1. 成功写出 `/tmp/notdec-emit-tr-input/out.ll`
2. 日志打印 `TR input IR emitted to ...`
3. 进程在 pre-TR 标准化阶段后退出，没有进入 `MLsubRecoveryMain`

### 3.3 配合 workdir

执行：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  --emit-tr-input-ir=/tmp/notdec-emit-tr-input-work/out.ll \
  --tr-level=1 -g --work-dir=/tmp/notdec-emit-tr-input-work/work
```

结果：workdir 中出现：

1. `00-lifted.ll`
2. `01-1-BeforeStackAlloca.ll`
3. `02-mlsub-input.ll`
4. `02-mlsub-input.anchor.json`
5. `SelectableValues.txt`

## 4. 当前判断

这一步做完后，“阶段 A 导出冻结 IR”终于变成了正式接口，而不再只是依赖
完整类型恢复运行过程里的副产物。

后续更自然的下一步就变成：

1. 让启用 `NOTDEC_EXTRA_CONSTRAINTS` 的阶段 B 只接受冻结后的 `.ll/.bc`
2. 明确把 `--emit-tr-input-ir` 产物作为推荐输入

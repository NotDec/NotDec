# MLsub 阶段 B 跳过 pre-TR 标准化 pass

日期：2026-04-17

## 1. 本次范围

沿着 [logs/20260417-09-MLsubStageBRemainingPlan.md](/sn640/NotDec/logs/20260417-09-MLsubStageBRemainingPlan.md)
里的第二步，开始把 pass pipeline 从“阶段 A 规范化 + 阶段 B `MLsub`”
的硬耦合里拆开。

本次完成的点：

1. 把 pre-type-recovery 标准化 pass 抽成单独 helper
2. 把 `MLsubRecoveryMain` 和 level-3 后处理抽成单独 helper
3. `--frozen-tr-input-ir` 模式下不再重复跑：
   - `buildFunctionOptimizations()`
   - `LinearAllocationRecovery`
   - `MemsetMatcher`
   - `MemcpyMatcher`
   - `UndoInstCombine`
   - `BDCE`
   - `ReorderBlocksPass`
4. `--emit-tr-input-ir` 仍然继续走“阶段 A 后退出”

本次没有做的点：

1. 还没有新增更细的阶段 B 输入 provenance 诊断
2. 也还没有把 howard suite 的 `fortune` 正式提到 `xfail/pass`

## 2. 代码修改

### 2.1 `PassEnv` 拆出两个阶段 helper

文件：

- [include/notdec/Passes/PassManager.h:76](/sn640/NotDec/include/notdec/Passes/PassManager.h#L76)
  到 [include/notdec/Passes/PassManager.h:79](/sn640/NotDec/include/notdec/Passes/PassManager.h#L76)
- [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
  到 [src/Passes/PassManager.cpp:407](/sn640/NotDec/src/Passes/PassManager.cpp#L334)

涉及函数：

1. `PassEnv::add_pre_type_recovery_passes()`
2. `PassEnv::add_type_recovery_passes(int level)`
3. `PassEnv::build_passes(int level, bool stopBeforeTypeRecovery, bool frozenTRInputIR)`

修改内容：

1. 新增 `add_pre_type_recovery_passes()`，集中承载阶段 A 的 pass：
   - `buildFunctionOptimizations()`
   - `LinearAllocationRecovery`
   - `MemsetMatcher`
   - `MemcpyMatcher`
   - `UndoInstCombine`
   - `BDCE`
   - `ReorderBlocksPass`
2. 新增 `add_type_recovery_passes(int level)`，集中承载阶段 B：
   - `MLsubRecoveryMain`
   - `MLsubRecoveryOpt`
   - 以及原先 `tr-level>=3` 的后处理
3. `build_passes()` 现在新增 `frozenTRInputIR` 分支：
   - 非 frozen：先跑阶段 A，再进入阶段 B
   - frozen：直接跳过阶段 A，进入阶段 B

### 2.2 `DecompileConfig` 把 frozen 模式真正接进 pipeline 调度

文件：

- [include/notdec/Passes/PassManager.h:107](/sn640/NotDec/include/notdec/Passes/PassManager.h#L107)
  到 [include/notdec/Passes/PassManager.h:119](/sn640/NotDec/include/notdec/Passes/PassManager.h#L107)

涉及函数：

- `DecompileConfig::build_passes(int level)`

修改内容：

1. `Opts.frozenTRInputIR` 现在开始参与有效 `tr-level` 计算
2. 当前策略：
   - `emitTRInputIR` 或 `frozenTRInputIR` 任一启用时
   - `EffectiveLevel = max(level, 2)`
3. 调用：
   - `PE.build_passes(EffectiveLevel, EmitTRInputIR, FrozenTRInputIR)`
4. `.c` 输出时 `disableTypeRecovery` 也改为基于 `EffectiveLevel`
   而不是原始 `Opts.trLevel`

这样做的目的：

1. `--frozen-tr-input-ir` 现在不只是入口语义
2. 它已经真正改变了 pass pipeline 的物理执行路径

### 2.3 调试文档补充 frozen 模式的行为说明

文件：

- [DEBUG.md:30](/sn640/NotDec/DEBUG.md#L30)
  到 [DEBUG.md:34](/sn640/NotDec/DEBUG.md#L30)

修改内容：

1. 明确写出：
   - `--frozen-tr-input-ir` 会跳过 pre-type-recovery 标准化 pass
   - 直接进入 `MLsub`

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 阶段 A 导出仍然可用

执行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  --emit-tr-input-ir=/tmp/fortune-stagea.ll \
  --tr-level=2
```

结果：

1. 成功导出 `/tmp/fortune-stagea.ll`
2. 日志中仍然出现：
   - `LinearAllocationRecovery`
   - `Merging ... into memset`
3. 说明阶段 A 路径没有被这次重构破坏

### 3.3 原始输入路径仍然会跑阶段 A

执行：

```bash
./build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  --tr-level=2 \
  -o /tmp/fortune-normal.ll > /tmp/fortune-normal.log 2>&1
```

再检查：

```bash
rg -n "LinearAllocationRecovery|Merging .*memset|Constraint generation done" \
  /tmp/fortune-normal.log
```

结果：

1. 仍然能看到：
   - `LinearAllocationRecovery`
   - 多条 `Merging ... into memset`
   - `Constraint generation done! SCC count:25`

### 3.4 冻结 IR 路径现在会跳过阶段 A

执行：

```bash
./build/bin/notdec /tmp/fortune-stagea.ll \
  --tr-level=2 \
  --frozen-tr-input-ir \
  -o /tmp/fortune-stageb.ll > /tmp/fortune-stageb.log 2>&1
```

再检查：

```bash
rg -n "LinearAllocationRecovery|Merging .*memset|Constraint generation done" \
  /tmp/fortune-stageb.log
```

结果：

1. 只看到：
   - `Constraint generation done! SCC count:25`
2. 不再出现：
   - `LinearAllocationRecovery`
   - `Merging ... into memset`
3. 同时 `/tmp/fortune-stageb.ll` 成功生成

这说明：

1. frozen 模式现在已经真正绕过 pre-TR 标准化段
2. 而不是只在 CLI 层面做了语义声明

## 4. 当前判断

这一步做完后，“阶段 A / 阶段 B”已经不只是文档概念，而是开始体现在
pipeline 结构里了。

后续更自然的下一步就是：

1. 继续补阶段 B 输入校验与更清晰的报错
2. 再把一条正式的两阶段端到端回归加进测试

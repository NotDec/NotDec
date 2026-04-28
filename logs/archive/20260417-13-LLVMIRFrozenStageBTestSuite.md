# LLVM IR 类型恢复 suite 切到 frozen 阶段 B

日期：2026-04-17

## 1. 本次目标

把 `test/type-recovery/llvm-ir/` 这套 LLVM IR regression 从原先默认会经过
`tr-level=2` 前置规范化 pass 的路径，改成：

1. 所有 case 默认以 frozen 阶段 B 运行
2. 不再重跑优化和变换 pass
3. 只测 `MLsub` 类型推理本身

## 2. 代码与测试数据修改

### 2.1 suite manifest 改成 frozen 阶段 B 默认参数

文件：

- [test/type-recovery/llvm-ir/manifest.json:1](/sn640/NotDec/test/type-recovery/llvm-ir/manifest.json#L1)
  到 [test/type-recovery/llvm-ir/manifest.json:6](/sn640/NotDec/test/type-recovery/llvm-ir/manifest.json#L1)

修改内容：

1. suite 名称改为：
   - `notdec type recovery llvm ir frozen stage-b tr-level=2`
2. `default_args` 从：
   - `--tr-level=2`
   改为：
   - `--tr-level=2`
   - `--frozen-tr-input-ir`

涉及入口：

1. `test/run_type_recovery_suite.py` 仍未改代码
2. 本次只通过 `manifest.json` 的 `default_args` 改变 suite 语义

### 2.2 `xfail` 提升为 `pass`

文件：

- [test/type-recovery/llvm-ir/manifest.json:86](/sn640/NotDec/test/type-recovery/llvm-ir/manifest.json#L86)
  到 [test/type-recovery/llvm-ir/manifest.json:114](/sn640/NotDec/test/type-recovery/llvm-ir/manifest.json#L86)

修改内容：

1. `14_Equality1`
2. `17_StackArray`
3. `18_offset1`

都从原来的 `xfail` 改成了 `pass`，并补上了 `expected` snapshot 路径。

### 2.3 README 改成“只测类型推理”的当前语义

文件：

- [test/type-recovery/llvm-ir/README.md:5](/sn640/NotDec/test/type-recovery/llvm-ir/README.md#L5)
  到 [test/type-recovery/llvm-ir/README.md:25](/sn640/NotDec/test/type-recovery/llvm-ir/README.md#L5)

修改内容：

1. `expected/tr-level-2/` 的 oracle 描述改成：
   - `notdec --tr-level=2 --frozen-tr-input-ir --dump-htypes`
2. 新增 `Current Execution Model` 小节，明确写出：
   - `manifest.json` 默认传 `--frozen-tr-input-ir`
   - `cases/*.ll` 被视为已经冻结好的 LLVM IR
   - suite 的目的就是隔离类型推理本身，而不是重跑 pre-TR 规范化

### 2.4 golden snapshot 全量更新

文件：

- [test/type-recovery/llvm-ir/expected/tr-level-2/01_Simple1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/01_Simple1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/02_ConstantAddr1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/02_ConstantAddr1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/03_LoadUpdateGood.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/03_LoadUpdateGood.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/04_LoadUpdate.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/04_LoadUpdate.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/05_MultiOffset.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/05_MultiOffset.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/06_SimpleRecursive2.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/06_SimpleRecursive2.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/07_PassStack1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/07_PassStack1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/08_Memory1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/08_Memory1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/09_OffsetLoop.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/10_BottomUp1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/10_BottomUp1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/11_SimpleRecursive1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/11_SimpleRecursive1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/12_Stack1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/14_Equality1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/14_Equality1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/16_Poly1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/16_Poly1.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes)
- [test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes](/sn640/NotDec/test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes)

修改内容：

1. 用 frozen 阶段 B 路径重新生成了这 16 个成功 case 的 HType golden
2. 这批 snapshot 的主要变化来自：
   - 不再运行 pre-type-recovery 规范化
   - memory / `<null-value>` 更常保留为 `top:32`
   - 少数 pointer 约束从旧 snapshot 的 `|` 形式变成新的 `&` 形式

涉及函数：

1. 这些 snapshot 都来自 `notdec` 在：
   - `PassEnv::build_passes()`
   - `PassEnv::add_type_recovery_passes()`
   - `MLsubRecovery::run()`
   当前 frozen 阶段 B 路径下的实际输出

## 3. 验证

### 3.1 先跑一次 frozen suite，确认状态变化

执行：

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-llvm-ir-frozen-suite
```

结果：

1. 原 13 个 `pass` case 全部变成 snapshot mismatch
2. `14_Equality1`、`17_StackArray`、`18_offset1` 从 `xfail` 变成 `xpass`
3. `13_stack_variable_alloc`、`15_signed1`、`19_PtrPtr` 仍然 `xfail`

### 3.2 更新 golden 与 manifest 后再次重跑

执行：

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root . \
  --workdir /tmp/notdec-llvm-ir-frozen-suite-2
```

结果：

```text
Summary: 16 passed, 3 xfailed, 0 skipped, 0 xpassed, 0 failed
```

## 4. 当前判断

现在 `test/type-recovery/llvm-ir/` 的语义已经更接近“手写 LLVM IR 上的纯类型推理回归”了：

1. 输入是 checked-in `.ll`
2. 默认直接以 frozen 阶段 B 执行
3. 不再混入阶段 A 规范化和结构改写的行为

后续如果要继续推进：

1. 可以考虑把其他 suite 也区分成“阶段 A 行为回归”和“阶段 B 纯类型推理回归”
2. 也可以继续观察这 3 个剩余 `xfail` 是否因为 frozen 语义改变而出现新的修复机会

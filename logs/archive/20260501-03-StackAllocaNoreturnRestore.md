# 2026-05-01 StackAlloca noreturn restore 放宽

## 背景

`fortune` 的 `work_dir/01-recovery-passes.log` 里一直有两条：

- `ERROR: Cannot find sp restore? func: main`
- `ERROR: Cannot find sp restore? func: usage`

先看 `01-1-BeforeStackAlloca.ll` 后，问题不是没识别到栈分配，而是：

- `main` 和 `usage` 都有标准 prologue
- 但这两个函数最后都走 `exit(...)` + `unreachable`
- 当前 `LinearAllocationRecovery::run()` 只要没在 exit block 里找到
  `store LoadSP -> @__stack_pointer`，就直接放弃整函数转换

这会把“正常 noreturn 函数没有 restore”的情况误判成失败。

## 目标

只做最小放宽：

- 保持现有 prologue 匹配不变
- 保持普通返回函数必须有 restore 不变
- 仅当函数所有终点都是 `unreachable` 时，允许缺少 restore，继续做
  `alloca` 改写

## 实现

### `src/Passes/StackAlloca.cpp`

- `32-48`，新增 `hasOnlyUnreachableExitBlocks(Function &F)`
  - 扫描所有无后继 basic block
  - 只有当所有 exit block 的 terminator 都是 `unreachable` 时才返回 true
  - 这一步的目的，是把“真的 noreturn” 和 “普通函数但 restore 漏匹配” 分开
- `336-350`，`LinearAllocationRecovery::run()`
  - 原来没找到 restore 就直接报错并 `continue`
  - 现在先判断 `hasOnlyUnreachableExitBlocks(*F)`
  - 命中时改记
    `NOTE: missing sp restore is allowed for noreturn func: ...`
  - 否则仍保留原来的 error 路径

涉及函数：

- `hasOnlyUnreachableExitBlocks`
- `LinearAllocationRecovery::run`

## 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. `fortune` 非 frozen 定点验证

```bash
rm -rf /tmp/notdec-fortune-stackalloca-noreturn
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.stackalloca-noreturn.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-stackalloca-noreturn
```

结果：

- 通过，`real 15.73s`
- `/tmp/notdec-fortune-stackalloca-noreturn/01-recovery-passes.log`
  不再出现这两条 error
- 改为：
  - `NOTE: missing sp restore is allowed for noreturn func: main`
  - `NOTE: missing sp restore is allowed for noreturn func: usage`
- 输出 IR 中：
  - `main` 已生成 `alloca [2960 x i8]`
  - `usage` 已生成 `alloca [144 x i8]`

3. `fortune` 当前关注 frozen 口径

```bash
rm -rf /tmp/notdec-fortune-stackalloca-frozen-noextra
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.stackalloca-frozen-noextra.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-stackalloca-frozen-noextra
```

结果：

- 通过，`real 17.07s`
- 这个口径会跳过 pre-TR recovery passes，所以本次改动本身不会走到
  `StackAlloca`
- 当前参考是 `16.33s`，这次比参考慢 `0.74s`，但这条差异不能归因到本次代码修改

4. 额外观察

如果同时带：

```bash
NOTDEC_EXTRA_CONSTRAINTS=test/type-recovery/realworld/support/fortune.o3.wasm.extra.json
```

当前会先在 frozen 校验阶段报 `ir_anchor.sha256 mismatch`。这是现有约束文件和当前
输入不一致的问题，不是这次 `StackAlloca` 改动引入的。

## 效果判断

这次改动达到了预期：

- 修掉了 `main` / `usage` 这类 noreturn 函数的假阳性报错
- 没放宽普通返回函数的 restore 要求
- 改动范围只在 `StackAlloca.cpp`，复杂度很低

## 评分

- 实现效果：8/10
- 理解成本：8/10
- 维护成本：8/10

原因：

- 规则很窄，只处理“所有终点都是 unreachable”的函数
- 不引入新的 CFG/支配分析
- 后面如果再遇到“restore 被合并到更复杂形态”那是另一类问题，应该单独处理

## 追加实现：刷新 frozen `fortune` realworld case

这一步不是为了让 suite 一定通过，而是先把当前调试和 frozen 工作流里真正使用的
`fortune` IR 更新到包含这次 `StackAlloca` 改动的新版本。

### 修改点

#### `test/type-recovery/realworld/cases/fortune.o3.wasm.ll`

- 整体替换为重新生成的 frozen IR
- 当前文件头变为：
  - `ModuleID = 'test/type-recovery/realworld/cases/fortune.o3.wasm.ll'`
  - `source_filename = "/sn640/NotDec/test/lifting/wasm/cases/fortune.o3.wasm"`
- 关键变化是 `main` / `usage` 已经不再保留
  `load/add/store @__stack_pointer` 形式，而是直接落成 `alloca`

#### `test/type-recovery/realworld/support/fortune.o3.wasm.extra.json`

- `ir_anchor.sha256`
  - 从旧值更新为 `6620beabc3151befe0fe881ebbd307d42787ffe7a0b738c3ef8f8fe16b527358`
- 这是当前这份 checked-in frozen IR 在
  `/sn640/NotDec/test/type-recovery/realworld/cases/fortune.o3.wasm.ll`
  路径下运行 `--frozen-tr-input-ir` 时实际导出的 anchor

### 生成与核对命令

1. 基于当前 checked-in frozen case 再过一遍非 frozen 恢复链路，拿到更新后的
   stage-B IR

```bash
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.stackalloca-noreturn.out.ll \
  --tr-level=2 -g \
  --work-dir=/tmp/notdec-fortune-stackalloca-noreturn
```

结果：

- 通过，`real 15.73s`
- `/tmp/notdec-fortune-stackalloca-noreturn/02-mlsub-input.ll`
  作为这次刷新后的 case 内容来源

2. 用更新后的 case 做一次 frozen 运行，读取实际 anchor

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  /sn640/NotDec/test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.realworld.altcase.verify.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-realworld-altcase-verify
```

结果：

- 通过，`real 15.37s`
- 导出的
  `/tmp/notdec-fortune-realworld-altcase-verify/02-mlsub-input.anchor.json`
  中 `sha256` 为
  `6620beabc3151befe0fe881ebbd307d42787ffe7a0b738c3ef8f8fe16b527358`

### 当前判断

- 这次先把 frozen case 和 extra anchor 对齐，保证后续 `fortune` frozen 调试路径
  使用的是包含新 `StackAlloca` 结果的 IR
- `realworld` suite oracle 目前是否完全一致，和这次“先更新 IR 再提交”是两件事；
  若后面要继续维持 suite 通过，需要单独判断是否更新 oracle 还是继续收敛恢复结果

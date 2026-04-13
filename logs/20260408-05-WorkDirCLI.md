# 2026-04-08 Work Dir CLI Migration

## 背景

这次调整的目标是把原先分散在两个环境变量上的中间产物目录逻辑，统一收敛成一个由命令行显式开启的工作文件夹机制。

原先仓库里的中间产物目录入口有两套：

- `NOTDEC_DEBUG_DIR`
- `NOTDEC_TYPE_RECOVERY_DEBUG_DIR`

虽然多数场景里二者都会被配置成同一个目录，但语义和入口仍然分裂，导致：

1. CLI 行为不够直观，调试时必须记忆环境变量
2. `launch.json` / `run.sh` / 文档都需要重复维护两套名字
3. `llvm2c`、主 pass pipeline、类型恢复阶段虽然最终都在写同一个目录，但代码入口不统一

这次改动直接废弃这两个环境变量，不保留兼容层。

## 方案

最终采用的接口如下：

- `--gen-work-dir`
  - 开启工作文件夹生成
- `-g`
  - `--gen-work-dir` 的短参数
- `--work-dir=<path>`
  - 覆盖默认工作文件夹路径，仅在开启 `--gen-work-dir` 时有效

默认工作文件夹路径规则：

- `<input>.notdec`

例如：

- `test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll`
- 默认生成目录：
  `test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll.notdec/`

## 实现范围

### 1. 主程序 CLI

在 `src/NotDec.cpp` 中新增：

- `--gen-work-dir`
- `-g`
- `--work-dir`

并增加参数约束：

- 单独传 `--work-dir` 会直接报错退出

### 2. 统一工作文件夹配置

在 `notdec::Options` 中加入：

- `workDir`

并在 `Utils` 中新增统一 helper：

- `getDefaultWorkDir()`
- `setWorkDir()`
- `getWorkDir()`
- `getWorkDirOpt()`

这样主 pass pipeline、类型恢复、stack recovery 都不再直接读环境变量，而是统一走进程内配置。

### 3. 类型恢复与主流程落盘统一

把以下中间产物都切换到工作文件夹：

- `00-lifted.ll`
- `01-1-BeforeStackAlloca.ll`
- `01-Optimized.ll`
- `CallGraph.txt`
- `CallGraph.dot`
- `SCCs.txt`
- `ValueTypes.txt`

### 4. llvm2c 同步收敛

`external/NotDec-llvm2c` 的 `Options` 中新增：

- `workDir`

并让 `decompileModule()` 使用该路径写：

- `llvm2c-before-demotessa.ll`
- `llvm2c-after-demotessa.ll`

这样 `.c` 输出路径下的 backend dump 也和前面的中端产物放到同一个工作文件夹里。

### 5. 调试入口与文档更新

同步更新了：

- `run.sh`
- `.vscode/launch.json`
- `README.md`
- `DEBUG.md`

并把 `launch.json` 里的 `dec` 配置样例输入改到当前仓库真实存在的
`test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll`。

## 验证

### 构建

执行：

```bash
cmake --build ./build --target all
```

构建通过。

### CLI 行为

执行：

```bash
./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll \
  -o /tmp/notdec-cli-check.ll \
  --tr-level=1 \
  --work-dir=/tmp/should-fail
```

结果：

- 报错 `Error: --work-dir requires --gen-work-dir.`
- 退出码为 `1`

说明参数约束生效。

### 默认工作文件夹

执行：

```bash
NOTDEC_DISABLE_INTERPROC=1 \
NOTDEC_SAT_DISABLE=1 \
./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll \
  -o /tmp/notdec-cli-check.ll \
  --tr-level=1 \
  -g
```

结果生成：

- `test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll.notdec/00-lifted.ll`

说明默认 `<input>.notdec` 路径生效。

### 自定义工作文件夹

执行：

```bash
NOTDEC_DISABLE_INTERPROC=1 \
NOTDEC_SAT_DISABLE=1 \
./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll \
  -o /tmp/notdec-cli-check.c \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-work
```

结果生成：

- `/tmp/notdec-work/00-lifted.ll`
- `/tmp/notdec-work/01-1-BeforeStackAlloca.ll`
- `/tmp/notdec-work/01-Optimized.ll`
- `/tmp/notdec-work/CallGraph.txt`
- `/tmp/notdec-work/CallGraph.dot`
- `/tmp/notdec-work/SCCs.txt`
- `/tmp/notdec-work/ValueTypes.txt`
- `/tmp/notdec-work/llvm2c-before-demotessa.ll`
- `/tmp/notdec-work/llvm2c-after-demotessa.ll`

说明中端与 llvm2c backend 的中间产物已经统一落到同一个工作文件夹。

## 结论

本次改动完成了“debug 目录”到“工作文件夹”的入口迁移：

1. 废弃旧环境变量入口
2. 统一成显式 CLI 开关
3. 允许路径 override
4. 默认路径直接跟随输入文件
5. 主流程、类型恢复、llvm2c backend 的中间产物目录逻辑收敛为一套

# DEBUG.md

本文档集中说明 `NotDec` 当前调试流程里的工作文件夹和中间产物，方便在使用 `debugmcp`、`launch.json` 或手工命令行调试时快速定位问题。

## 1. 工作文件夹的基本用法

工作文件夹由命令行选项开启，不再由环境变量控制。

常见做法：

```bash
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 --gen-work-dir
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --work-dir=work_dir
./build/bin/notdec input.bc --emit-tr-input-ir=/tmp/tr-input.ll -g --work-dir=work_dir
./build/bin/notdec /tmp/tr-input.ll -o /tmp/out.ll --tr-level=2 --frozen-tr-input-ir
```

其中：

- `--gen-work-dir` / `-g`
  - 开启工作文件夹输出
- 默认路径
  - 使用输入文件同名并追加 `.notdec`，例如 `cases/foo.ll.notdec/`
- `--work-dir=<path>`
  - 用于覆盖默认工作文件夹路径
- `--emit-tr-input-ir=<path>`
  - 跑完 pre-type-recovery 标准化 pass，导出类型恢复真正消费的输入 IR
  - 支持输出到 `.ll` 或 `.bc`
  - 导出后直接退出，不进入 `MLsubRecoveryMain`
- `--frozen-tr-input-ir`
  - 声明当前输入已经是 `--emit-tr-input-ir` 导出的冻结 IR
  - 用于显式进入阶段 B 语义
  - 当前会跳过 pre-type-recovery 标准化 pass，直接进入 `MLsub`
  - 只接受 `.ll` / `.bc` 输入
  - 当前启用 `NOTDEC_EXTRA_CONSTRAINTS` 时必须同时传入

仓库内的 `run.sh` 和 `.vscode/launch.json` 里的常用配置现在也统一走这套命令行参数。

## 2. 工作文件夹里的主要内容

工作文件夹统一承载原先两类中间产物，主要覆盖：

- 主 pass pipeline 初始 IR dump
- stack / memory recovery 前的 IR dump
- 类型恢复阶段的优化后 IR
- CallGraph 文本与 dot
- SCC 划分信息
- binarysub trace
- 最终 `IR Value -> binarysub UType` 对照表
- `llvm2c` 后端在 demote SSA 前后的 IR dump

如果问题表现为：

- 类型恢复异常
- 某个值被推成了奇怪的类型
- binarysub 约束传播/简化结果不对

那么优先看这个目录。

## 3. 工作文件夹里常见文件及作用

下面按当前仓库里实际会出现的文件说明。不是每次运行都会生成全部文件；是否出现取决于输入、`tr-level`、是否走到 `llvm2c`、以及具体 pass 是否执行到对应阶段。

### `00-lifted.ll`

- 来源：主 pass pipeline 早期 dump
- 作用：看“输入被载入/提升后”的 LLVM IR 初始状态
- 典型用途：确认前端读入 `.ll` / `.bc` / `.wasm` 后，IR 是否已经在最开始就有问题

### `01-1-BeforeStackAlloca.ll`

- 来源：`LinearAllocationRecovery`
- 作用：看 stack alloca 恢复前的模块状态
- 典型用途：排查 stack pointer 识别失败、栈增长方向判断异常、stack rewrite 前后差异

### `02-mlsub-input.ll`

- 来源：
  - `MLsubRecovery::run()` 入口处的模块 dump
  - 或 `--emit-tr-input-ir=<path>` 对应的导出内容
- 作用：看 `mlsub` 真正吃到的 LLVM IR
- 典型用途：判断问题是在更早的优化/恢复阶段就产生了，还是在类型恢复阶段才出现
- 备注：如果后续 JSON 约束注入要绑定某个固定 IR，这份文件比泛化的 “Optimized” 更接近实际锚点

### `02-mlsub-input.anchor.json`

- 来源：`MLsubRecovery::run()` 在导出 `02-mlsub-input.ll` 后同步生成
- 作用：记录当前 `mlsub` 输入 IR 的阶段名、数据布局、target triple 和内容摘要
- 典型用途：给后续约束文件或调试脚本提供机器可读的 IR 锚点，快速确认 selector 是否仍对着同一份冻结 IR
- 当前摘要字段：只保留 `ir_anchor.sha256`
- 当前校验规则：`NOTDEC_EXTRA_CONSTRAINTS` 要求 `ir_anchor.sha256` 与当前
  `02-mlsub-input.ll` 的 SHA-256 一致
- 当前阶段约束：`NOTDEC_EXTRA_CONSTRAINTS` 还要求命令行显式传入
  `--frozen-tr-input-ir`，表示当前运行是“冻结 IR + MLsub”的阶段 B 模式；
  该环境变量不能与 `--emit-tr-input-ir` 同时使用
- 相关环境变量：`NOTDEC_EXTRA_CONSTRAINTS` 当前已经支持读取一个 JSON 文件，并在 `MLsub` 开始前校验其中的 `ir_anchor` 是否匹配当前冻结 IR；函数级 `actions` 已支持 `kind = "pndiff" | "subtype" | "equal"`，`target` 当前支持 `arg` / `ret` / `named_value` / `inst` / `operand` / `binding`；其中 `named_value` 表示当前函数里第一个同名非 `void` instruction result，`inst` 的 `id` 和 `operand.inst` 都使用 `toStableString()` 风格的指令 id，`operand` 现在支持两种 instruction 定位方式：
  - `operand(inst="main::%foo", index=N)`：按 stable id 定位
  - `operand(name="foo", index=N)`：按当前函数里第一个同名非 `void` instruction result 定位
  `binding` 表示引用当前函数 `bindings` 段里的局部 selector 别名

### `CallGraph.txt`

- 来源：类型恢复阶段构造 CallGraph 后导出
- 作用：文本形式查看调用图
- 典型用途：排查函数是否被识别为直接调用、某条调用边是否存在、某个函数是否被纳入分析

### `CallGraph.dot`

- 来源：类型恢复阶段构造 CallGraph 后导出
- 作用：Graphviz dot 格式的调用图
- 典型用途：把调用关系可视化，快速看 SCC、递归、调用边结构

### `SCCs.txt`

- 来源：类型恢复阶段的 SCC 划分逻辑
- 作用：记录类型恢复按什么 SCC 顺序/分组进行分析
- 典型用途：排查多态函数复制、bottom-up / top-down 分析边界、某个函数为什么被分到当前 SCC

### `ValueTypes.txt`

- 来源：类型恢复阶段 `bulkSimplify` 结束后导出
- 作用：记录最终的 `IR Value -> binarysub UType` 映射
- 典型用途：直接看某个 LLVM Value 最后对应的 `UType` 是什么，适合调试“为什么这个值被推成这个类型”

当前格式特点：

- 以 `## SCC: ...` 分块，便于按 SCC 查看
- `"[+]"` / `"[-]"` 表示当前条目在 binarysub 简化时采用的极性
- `[memory] <memory> => ...` 表示 memory 总类型

### `SelectableValues.txt`

- 来源：`MLsubRecovery::run()` 在导出 `02-mlsub-input.ll` 后同步生成
- 作用：列出当前冻结 IR 上可直接写进 `NOTDEC_EXTRA_CONSTRAINTS` 的 selector 提示
- 典型用途：手写 `inst` / `operand` / `named_value` / `arg` / `ret` selector 时直接抄 workdir 里的示例
- 当前格式特点：
  - 每个函数以 `## Function: ...` 分块
  - 直接列出 `arg` / `ret` / instruction stable id
  - 对有名非 `void` instruction 额外列出 `named_value(...) -> ...`
  - 对每条 instruction 列出 `operand(inst="...", index=N)` 示例
  - 对有名非 `void` instruction 额外列出 `operand(name="...", index=N)` 示例

### `binarysub-trace.log`

- 来源：`external/binarysub`
- 生成条件：仅当 `NOTDEC_BINARYSUB_TRACE=1` 且启用了 `--gen-work-dir` 时生成
- 作用：记录 binarysub 与 `PNDiff` 的内部事件流，包含约束、extrude、simplify、
  `PNDiff` 加减法求解、unify 等 trace
- 典型用途：当 `ValueTypes.txt` 只能看到“结果不对”，但还不知道“约束传播过程中哪一步歪了”时，继续往这个文件追

### `llvm2c-before-demotessa.ll`

- 来源：`llvm2c` 后端
- 作用：看 `llvm2c` 在 demote SSA 之前收到的模块
- 典型用途：排查“类型恢复输出正常，但 C 后端行为异常”的问题

### `llvm2c-after-demotessa.ll`

- 来源：`llvm2c` 后端
- 作用：看 `llvm2c` 执行 reg2mem / demote SSA 之后的模块
- 典型用途：排查 phi、局部变量重建、SSA 降级后导致的结构恢复问题

## 4. 推荐阅读顺序

如果遇到反编译问题，通常可以按下面顺序看：

1. `00-lifted.ll`
   - 确认输入 IR 是否一开始就异常
2. `01-1-BeforeStackAlloca.ll`
   - 如果问题与栈/内存恢复相关，先看这里
3. `02-mlsub-input.ll`
   - 确认类型恢复实际输入
4. `02-mlsub-input.anchor.json`
   - 确认当前 workdir 里的 `mlsub` 输入 IR 身份信息
5. `CallGraph.txt` / `SCCs.txt`
   - 确认分析范围、递归分组、多态边界
6. `ValueTypes.txt`
   - 直接看某个值最后被推成什么 `UType`
7. `binarysub-trace.log`
   - 继续追 binarysub 内部传播细节
8. `llvm2c-before-demotessa.ll` / `llvm2c-after-demotessa.ll`
   - 如果 `.ll` 看起来正常，但 `.c` 输出异常，再看这里

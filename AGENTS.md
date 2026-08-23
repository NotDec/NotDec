# Work guidelines

# 项目规范

1. 代码一定要多写注释，特别是新引入的数据结构前，说明背后的设计理念。
1. 工作流程：收到需求 -> 思考后告诉用户打算怎么更改 -> 讨论一致后再开始实现。复杂代码修改实现完后再写文档到 `logs/`；简单文档修改、注释修改、错别字修正不需要写日志。
   实现完计划并完成验证后，默认必须直接提交，不要停在未提交状态；涉及 submodule 时，先在 submodule 内提交，再提交顶层指针和日志。
   如果只是文档修改，并且前一个 commit 也是同一模块的文档-only 修改，优先用 amend 合并到前一个 commit，必要时同步修改 commit message，减少零碎提交。
   如果是已经确定的commit，例如发现存在不是当前会话修改的，但是没有push到上游的commit，需要直接push到上游。但出现冲突时禁止未经同意force push。
1. 写修改日志时，必须明确指出修改了哪个文件的哪一行，涉及哪些函数。
1. 尽量复用并改进之前的日志，最好每个功能都单独一个日志。
1. plan日志重点写问题背景、目标、期望效果、大致技术路线、风险和判断标准，要让没有上下文的人也能看懂；不要过早写成具体实现清单、命令清单或行号清单。实现记录才需要明确写修改了哪个文件的哪一行、涉及哪些函数、验证命令和结果。
   `logs/` 下的 plan 文档顶部必须先保留本次用户的原始 prompt，然后再写背景、目标、路线、风险和判断标准。
1. 如果当前的任务是对之前的plan日志的实现，则不需要单独创建日志，而是将实现情况写入之前的计划日志，比如将计划的步骤在标题中标记为已完成，记录实现细节，以及调整计划时考虑不全而实现时有所改变的部分。同时也不要使得日志文件过于冗长，简洁一些，包括语言风格上，以及没有真正实现，或者试错的思路都尽量简写。
1. Goal编写规范：Goal按照一下的三段格式编写：第一段 “基于 logs/xxx.md的规划进行”（启动goal前必须先生成对应规划文档）。第二段：使用200字左右描述当前用什么技术路线处理什么问题。第三段是固定内容，必须放这个原话：遇到了不确定的技术路线选择或计划里明显没有考虑到的事情时暂停goal。样例如下：
```
基于logs/20260623-01-sailr-angr-completion-plan.md基于完善SAILR的复刻，继续补齐 shared structuring 语义。

优先按旧链路在结构恢复前 demote Phi，并维护 HType 到 demoted LLVM Value 的映射，使 structuring 算法不直接处理 Phi；随后完善 copied/virtual block 的 payload materialize、ReturnDuplicatorLow、switch deoptimization 和 Angr pass 对齐。最后将Angr那边的SAILR相关的测试迁移过来。

遇到了不确定的技术路线选择或计划里明显没有考虑到的事情时暂停goal。
```

## 1. Git 与 external/ 子模块

`external/NotDec-bin2llvm`、`external/NotDec-wasm2llvm`、
`external/NotDec-llvm2c`、`external/binarysub`
都是 git submodule。检查工作树时注意区分：

- 顶层 `git status --short`
- 子模块内部真实状态，例如
  `git -C external/binarysub status --short`

不要误回滚 `external/` 下已有本地开发态改动。

### 多 worktree 同步流程

如果当前开发 worktree 的提交需要合到主 worktree 所在分支，例如 `/sn640/NotDec`
的 `v2`，按这个顺序做：

1. 先确认当前 worktree 和相关子模块状态：
   `git status --short`，以及 `git -C external/<submodule> status --short`。
2. 当前分支先 rebase 到目标分支最新提交，例如：
   `git fetch . v2:v2` 后执行 `git rebase v2`。
   如果目标分支来自远端，就先 fetch 对应 remote，再 rebase 到那个分支。
3. 涉及子模块时，先在子模块内部完成 commit；顶层再提交 submodule 指针和日志。
4. rebase 后把当前分支的新提交合入目标分支。优先用非交互命令，例如：
   `git checkout v2 && git merge --ff-only <current-branch>`。
   如果当前分支不能快进合入，先停下来确认原因，不要强行改历史。
5. 更新主 worktree：在 `/sn640/NotDec` 里切到目标分支并拉到最新提交，然后执行
   `git submodule update --init --recursive`，确保 `external/` 指针也到位。
6. 最后分别检查开发 worktree、主 worktree、相关子模块的 `git status --short`。

原则：

- 不用 cherry-pick 代替 rebase/fast-forward 流程，除非用户明确要求只挑某几个提交。
- 不要回滚主 worktree 或子模块里不属于本次任务的本地改动。
- 如果目标分支、当前分支、主 worktree 分支关系不清楚，先用 `git branch --show-current`、
  `git log --oneline --decorate -5` 和 `git status --short` 查清楚再动。

## 2. 项目主链路

`NotDec` 是一个以 LLVM IR 为中间表示的 WebAssembly 反编译与静态分析框架。
主链路可以概括为：

1. 前端读取输入
   - `.ll` / `.bc`：直接加载 LLVM IR
   - `.wasm` / `.wat`：经 `external/NotDec-wasm2llvm` 提升为 LLVM IR
2. 中端执行恢复与优化
   - 优化
   - 栈/内存恢复
   - 类型恢复
3. 后端按输出后缀决定产物
   - `.ll`：输出 LLVM IR
   - `.bc`：输出 bitcode
   - `.c`：经 `external/NotDec-llvm2c` 生成 C
   - `.sol`：经 `external/NotDec-llvm2c` 里的 Solidity backend 生成 Solidity-like 源码

## 3. 关键入口

主入口在 `src/NotDec.cpp`，通常先从这里确认：

1. CLI 参数解析
2. 输入格式分流
3. `DecompilerContext` / `DecompileConfig` 构造
4. `build_passes(trLevel)` / `run_passes()` 调用
5. `.ll` / `.bc` / `.c` 输出路径

相关上下文定义在 `include/notdec/DecompilerContext.h`。

如果问题属于 pass pipeline，再继续看：

- `include/notdec/Passes/PassManager.h`
- `src/Passes/PassManager.cpp`

## 4. Pass Pipeline 概览

`PassEnv::build_passes(int level)` 按 `tr-level` 分层：

### `tr-level >= 1`

基础优化，主要来自 `buildFunctionOptimizations()`，包括：

- `InstCombine`
- `SimplifyCFG`
- `PromotePass`
- `GVN`
- `SCCP`
- `BDCE`
- `ADCE`

### `tr-level >= 2`

恢复流程主入口：

- `LinearAllocationRecovery`
- `MemsetMatcher`
- `MemcpyMatcher`
- `UndoInstCombine`
- `ReorderBlocksPass`
- `mlsub::MLsubRecoveryMain`

### `tr-level >= 3`

更完整的类型恢复后处理：

- `mlsub::MLsubRecoveryOpt`
- `InstCombine`
- `PromotePass`
- `GVN`
- `BDCE`
- `SimplifyCFG`
- `AllocAnnotator`
- 再次 `ReorderBlocksPass`

如果输出后缀是 `.c`，还会额外接 `add_llvm2c()`，最终运行
`MLsubNotdecLLVM2C`。

## 5. 高频目录

- `src/`
  - 主程序与核心 pass 实现
- `include/notdec/`
  - 对应头文件
- `src/TypeRecovery/`
  - 类型恢复实现
- `src/Passes/`
  - decompilation / recovery pass
- `external/NotDec-wasm2llvm/`
  - wasm/wat 前端
- `external/NotDec-bin2llvm/`
  - ELF / shared object 到 LLVM IR 的二进制前端
- `external/NotDec-llvm2c/`
  - 当前所有源码后端都先放在这里，包括 C backend 和 Solidity backend。后续可以考虑把这个子模块仓库改名为 `notdec-codegen`，但现在先不改名。
- `external/binarysub/`
  - 类型推理核心算法
- `test/`
  - 集成测试、回归样例、suite runner
- `unittests/`
  - GoogleTest
- `docs/`
  - 文档站与设计说明
- `scripts/`
  - 调试、LLVM、可视化辅助脚本

## 6. 近期目标

**bin2llvm项目**

bin2LLVM 子项目近期目标：围绕 Bench2 这些真实项目生成 LLVM IR，并且语义要对。
“能被 `llvm-as` 接受”只是底线，不能代替语义正确。

Bench2 真实项目集合在 `/sn640/NotDec-Exp/Bench2`：

- `rootfs/`：已收集的真实项目二进制和依赖。
- `manifest/benchmark-targets.tsv`：当前选中的 ELF / shared object 目标。
- `manifest/benchmark-needed.tsv`：目标的动态依赖。
- `bin2llvm-ir/`：bin2llvm 相关 JSON、`.ll`、`.bc`、日志和 Ghidra project。

bin2llvm native 链路写计划时，优先按这个结构写，范围只限
`external/NotDec-bin2llvm` 的 native 路线：

1. 先说明当前目标和已有 native 状态。
2. 再介绍 Ghidra 相关实现，明确写出源码文件和关键函数。
3. 然后说明 native 侧要复刻哪些策略，哪些地方要保守处理或暂时不做。
4. 最后写阶段计划、判断标准、风险和不做什么。

这个写法只用于 bin2llvm native 链路；主 NotDec pass、evm2llvm、wasm2llvm、
llvm2c 等其他任务仍按普通项目规范写计划。

**evm2llvm项目**

apehex EVM 数据集和跑批目录在：

- 数据集：`/sn640/NotDecChainExp/apehex_evm_contracts`
- 运行目录：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot`
- 历史结果：`/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs`

当前这条链路的跑法是：

- 入口脚本：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/scripts/notdec-evm2llvm-apehex-batch.py`
- screen 会话：`apehex-longrun`
- 目前的循环方式：在 `screen` 里反复跑批次，批次名形如 `YYYYMMDD-evm2llvm-train-batchNNN`
- 常用参数：`--gigahorse-dir /sn640/gigahorse-toolchain --run-root /sn640/NotDecChainExp/evm2llvm_apehex_pilot --batch-size 40 --jobs 8`
- 输出内容：每个批次会生成 `manifest.csv`、`summary.csv`、`inputs/`、`logs/`、`outputs/`、`work/`
- 结果目录：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260521-evm2llvm-train-batch001`

CheckedBoundsPass 迭代时，apehex 侧默认抽样最近批次里约 100 个 IR 做 smoke/audit 即可；
只有发现新的 checked-bounds skip、runner 失败或明确需要扩大覆盖时，再扫更多批次。

Solidity 编译器源码在 `/sn640/solidity`。做 EVM Solidity 语义恢复时，优先对照 Solidity 自己的 codegen 形状，不要只从样例里猜。


## 7. 构建

当前仓库依赖本地 LLVM 22，默认布局是：

- `llvm-22.1.0.obj`

不要用系统 `/usr/bin/llvm-as`、`/usr/bin/opt` 验证当前 IR；它们可能仍是旧 LLVM。
需要直接使用：

- `llvm-22.1.0.obj/bin/llvm-as`
- `llvm-22.1.0.obj/bin/opt`

常用构建命令：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -S . -B ./build -G Ninja

cmake --build ./build --target all
```

产物通常在：

- `build/bin/notdec`

## 8. 运行与调试

调试前先读仓库根目录的 `DEBUG.md`。本文件只保留最小结论，workdir 产物和
调试文件清单统一以 `DEBUG.md` 为准。

当前推荐入口：

- `--gen-work-dir` / `-g`
- `--work-dir=<path>`
- `--fast-work-dir`
  - 必须和 `--gen-work-dir` / `-g` 一起使用
  - `ValueTypes.txt`、`VarOrigins.txt` 只写 stable label，完整项目跑批时可避免 verbose `Value::print()` 开销
- `--emit-tr-input-ir=<path>`
  - 注意：输出的是 `00-lifted.ll` 版本（无向量拆标量等 MLsub 输入预处理）；
    stage-B 的 checkpoint 锚点是 workdir 里重发射的 `02-mlsub-input.ll`（带向量
    拆标量，内容不同）。需要可复现的 stage-A frozen IR 时，用
    `-g --work-dir=<dir>` 生成后取 `<dir>/02-mlsub-input.ll`，不要用
    `--emit-tr-input-ir` 的输出直接当 stage-B 输入
- `--frozen-tr-input-ir`

当前仍常用的环境变量：

- `NOTDEC_SUMMARY_OVERRIDE`
- `NOTDEC_SIGNATURE_OVERRIDE`
- `NOTDEC_EXTRA_CONSTRAINTS`
- `NOTDEC_BINARYSUB_THREADS`
- `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL`

说明：

- `NOTDEC_SUMMARY_OVERRIDE`
  - summary / lower-bound override，方向是 `OverrideTy <: F`
- `NOTDEC_SIGNATURE_OVERRIDE`
  - 函数自身签名 upper-bound override，方向是 `F <: OverrideTy`
- `NOTDEC_EXTRA_CONSTRAINTS`
  - 额外 MLsub / PNDiff 约束，当前与 frozen stage-B IR 工作流配套使用
- `NOTDEC_BINARYSUB_THREADS`
  - 控制 binarysub bulk simplify 的线程数；未设置时使用硬件线程数
  - 需要整个 bulk simplify 单线程时设为 `1`
- `NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL`
  - oneTBB 构建下默认开启 canonicalize 并行；设为 `0` 时只关闭 canonicalize 并行
  - 这个开关不控制更早的 bottom-up 约束生成；该阶段当前仍是串行
  - 需要编译期完全关闭 binarysub oneTBB 时，用
    `-DNOTDEC_ENABLE_BINARYSUB_PARALLEL=OFF` 重新配置 CMake

典型命令：

```bash
./build/bin/notdec input.wat -o /tmp/out.c --tr-level=2 --gen-work-dir
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --work-dir=/tmp/notdec-work
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --fast-work-dir
```

### 源码级 IR 的 DebugInfo 合并策略评估

`--merge-eval-dir=<dir>` 是专门给带 DebugInfo 的源码级 LLVM IR 用的评估链路。
它不会改变普通类型恢复输出，主要用于评估节点合并策略：根据 LLVM DebugInfo 收集
`struct*` 这类有 ground truth 的值，类型推理结束后检查同一 DebugInfo 类型的节点是否
合得够多，以及有没有把不同 DebugInfo 类型错误合并。

源码级 IR 统一存放在 `/sn640/NotDec-Exp/Bench2/source-ir/ir/<project>/{<project>.ll,
<project>.bc}`，项目清单、构建方式和已知问题见该目录的
`/sn640/NotDec-Exp/Bench2/source-ir/README.md`（不在本仓库内）。

典型 fortune 命令：

```bash
NOTDEC_BINARYSUB_THREADS=8 /usr/bin/time -v ./build/bin/notdec \
  /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  --tr-level=2 \
  --merge-struct-ptr-load-store \
  -g --work-dir=/tmp/notdec-source-fortune-work \
  --merge-eval-dir=/tmp/notdec-source-fortune-eval \
  -o /tmp/notdec-source-fortune-out.ll
```

源码级 IR 评估显式固定为 8 线程，避免机器硬件线程数不同导致性能数据不可比。
canonicalize 默认已经并行，不需要再设置
`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1`。

主要输出：

- `merge-eval-summary.json`：汇总 coverage、wrong merge、fragmentation、performance。
- `DebugInfoValueTypes.txt`：DebugInfo oracle，类似 HType txt，列出每个有类型值的
  DebugInfo 类型和 strict 类型。
- `fragmented_types.jsonl`：同一 DebugInfo strict 类型分散在多个 SimpleType root 的情况。
- `bad_unions.jsonl`：不同 DebugInfo strict 类型被合到同一个 component 的错例。

常看的指标：

- `wrong_merge.bad_unions` / `polluted_components`：是否有错误合并，优先级最高。
- `fragmentation.fragmented_nodes` / `fragmented_types`：同类型节点是否还分散。
- `performance.wall_ms` / `peak_rss_mb`：类型恢复加评估链路的耗时和峰值内存。

**类型推理结果的对比依据是 HType 文件，不是输出 IR**：workdir 的
`ValueTypes.txt`（Final Value -> binarysub UType mapping）、`ValueHTypes.txt`、
`ImportantHTypes.txt`、`VarOrigins.txt`，以及 eval 的 `DebugInfoValueTypes.txt`
是类型推理的直接产物。输出 IR 不体现类型推理结果（类型恢复不重写 IR 指令，
IR 基本不动是正常现象），**"输出 IR cmp SAME"不能作为推理等价性或回归通过
的证据**。做 A/B 对照或回归验证时，必须对比 workdir/eval 的 HType 文件内容
（或 `merge-eval-summary.json` 的指标），IR 只用来做 `llvm-as` 合法性检查。

### 源码级 IR 合并评估的固定流程

对新的源码级项目做类型合并评估时，按下面顺序处理：

1. 先确认输入确实带有可用 DebugInfo，并记录源码版本、IR 路径、编译方式和 IR 哈希。运行结果还要用
   LLVM 22 verifier 检查，不能只看 merge-eval 成功退出。
2. 跑评估前先读源码和 IR，检查多态边界。重点包括 libc 的通用 allocator、deallocator、裸 buffer
   读写/复制函数、回调的 `void *` context，以及项目内对这些函数的 wrapper。`malloc/calloc/realloc`
   的纯返回转发 wrapper、`free` 的纯参数转发 wrapper 都应按调用点隔离；分配后立即初始化固定结构体的
   factory 不是通用 malloc wrapper，不要标成多态。
   **必须优先查项目自己的 allocator 名字**（如 redis 的 `zmalloc` 系列、tmux 的 `xrealloc`）：这类
   函数常带 `malloc_usable_size`/内存计数/OOM 分支，`getGenericMallocWrapperAllocator` 的自动检测
   只认 libc 6 个名字且拒绝这种形状，检测不到也不该依赖它。确认语义通用后直接加进
   `isBuiltinPolymorphicBufferFunctionName` 的内置名单（`src/TypeRecovery/mlsub/MLsubGenerator.cpp`，
   该名单现在对有定义的函数同样生效，效果同 av_calloc）。不带分配的裸 allocator（如 zmalloc）标记后
   其所有调用点各自实例化，是消除巨型互递归类型的关键，漏掉会导致类型图爆炸或全程序统一。
3. 网络程序额外检查 `read/write/recv/send` 及 wrapper，也检查 `recvfrom/recvmsg/accept/getsockname`、
   `getpeername/getsockopt/setsockopt/ioctl` 这类布局由地址族、option 或 request 决定的内存参数。只有裸
   buffer 或运行时决定布局的参数需要多态；固定读写某个明确结构体的业务函数不要因为调用了 socket API
   就整体标成多态。
4. 用 workdir 的 `MallocWrappers.txt`、`PolymorphicBufferFunctions.txt` 和源码互相核对。自动检测只能作为
   提示：还要检查 wrapper 链、函数指针/间接调用、allocator/free 是否被改名，以及是否存在未覆盖的 raw
   memory 输入。新增内置名单前要确认函数语义确实通用，避免丢掉固定类型 API 的跨调用约束。
5. 完整评估固定使用 8 线程、`--tr-level=2`、`--merge-struct-ptr-load-store`、`--fast-work-dir` 和
   `--merge-eval-dir`，并用 `/usr/bin/time -v` 记录 wall time 与峰值内存。每次使用新的 work/eval 目录，
   避免追加旧文件。canonicalize 默认并行，不额外设置 parallel 环境变量。
6. 结果先看 DebugInfo oracle coverage 是否足够，再看 `bad_unions`、`polluted_components` 和
   `bad_unions.jsonl`。出现错误合并时结合 first witness、`CallSlotMergeDecisions.txt` 和源码定位第一条错误
   边；wrong merge 解决后再看 fragmentation，不能用减少 fragmentation 换取错误合并。
7. 如果完整运行太慢，或错误合并难以分析，就按调用图逐步增加函数，保留相关全局变量、声明和 DebugInfo，
   找到首次出现耗时突增或同一条 bad union 的最小函数集合。小 IR 必须复现相同 strict 类型和 first
   witness；在小 IR 上修复后，最后仍要回到完整项目复跑，确认错误消失且输出 IR、coverage 和性能没有
   退化。

典型命令：

```bash
NOTDEC_BINARYSUB_THREADS=8 /usr/bin/time -v ./build/bin/notdec \
  input.ll --tr-level=2 --merge-struct-ptr-load-store \
  -g --fast-work-dir --work-dir=/tmp/notdec-source-project-work \
  --merge-eval-dir=/tmp/notdec-source-project-eval \
  -o /tmp/notdec-source-project-out.ll
```

### 大项目内存与时间开销分析

Bench2 大项目（tmux 39MB、redis 55MB 等）的恢复链路常见小时级耗时或几十 GB
内存爆炸。内存用 jemalloc profile 归因，时间用 perf attach 采样，两者结合
`/usr/bin/time -v` 的 wall/峰值 RSS 划分阶段。

**内存：jemalloc profile**

- 现成脚本 `scripts/profile-memcached-memory.sh`：默认 memcached，`--input` 换
  目标、`--threshold-gib 0` 不限内存、`--threads 8` 固定线程数；输出
  `rss-pss.csv`（RSS/PSS 时间序列）和 jemalloc heap dump。脚本里用的是系统
  `/usr/lib/x86_64-linux-gnu/libjemalloc.so.2`。
- 手动跑法：

  ```bash
  LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2 \
  MALLOC_CONF='prof:true,prof_active:true,lg_prof_sample:19,lg_prof_interval:30,prof_final:true,prof_prefix:/tmp/jeprof.tmux' \
  NOTDEC_BINARYSUB_THREADS=8 ./build-relwithdebinfo-20260731/bin/notdec \
    input.ll -o /tmp/out.ll --tr-level=2 --merge-struct-ptr-load-store \
    -g --fast-work-dir --work-dir=/tmp/work --merge-eval-dir=/tmp/eval
  ```

  进程正常退出时 jemalloc 在 `prof_prefix` 写 heap dump（文件名带序号和
  `.heap` 后缀；`prof_final` 保证被 kill 也有最终 dump）。
- 分析：

  ```bash
  jeprof --show_bytes --text ./build-relwithdebinfo-20260731/bin/notdec \
    /tmp/jeprof.tmux.*.heap
  jeprof --show_bytes --pdf ./build-relwithdebinfo-20260731/bin/notdec \
    /tmp/jeprof.tmux.*.heap > /tmp/jeprof-tmux.pdf
  ```

  `--alloc_space` 看累计分配总量，默认看 live；`--lines` 落到源码行。注意
  jemalloc 会改变分配行为，时间 A/B 对比必须同一 allocator，内存归因可以接受
  profile 自身开销。

**时间：perf attach**

- 本机 perf 是自编译的 `/home/ubuntu/.local/bin/perf`（5.15.160），
  `perf_event_paranoid=0`，不需要 sudo。先 `pgrep -x notdec` 拿真实 notdec
  进程 pid（`-f` 会同时匹配 bash 启动器），跑批起来后按阶段 attach 采样：

  ```bash
  perf record -F 99 -g -p <notdec-pid> -- sleep 30
  perf report
  perf annotate --symbol=<函数名>
  ```

- 阶段线程模型：约束生成是单核（进程只有 1 个线程，99% CPU）；analyze/simplify
  是 8 线程。attach 前先 `ps -L -p <pid>` 看线程数，不要把多线程阶段误判为单核
  热点。阶段边界看 workdir 文件时间：`02-mlsub-input.ll` 是约束生成输入，
  `[simplify-start]`（`NOTDEC_SIMPLIFY_DIAG=1`）是 simplify 开始。
- attach 偶发只采到极少量样本（进程长时间泡在深迭代或内核态段），重复采样或加
  `--call-graph dwarf`；与 `/usr/bin/time -v` 的 wall/峰值 RSS 交叉核对阶段划分。
- 跑批期间不要重链 `notdec`：运行中的进程还映射旧 inode，重建后 perf 里
  `notdec` 会显示成 `(deleted)`、函数名解析丢失。需要可解析符号的采样就在
  启动跑批前完成构建。
- run-to-run 复现性：指针键容器（`std::set<llvm::Function*>`、
  `std::map<llvm::CallBase*,...>` 等）的迭代顺序随 ASLR 变化，会让
  ValueTypes 文本、merge 决策和大 group 分组跨 run 不同（tmux 全量因此
  16-70 分钟随机）。要可复现/无离群尾巴，用 `setarch x86_64 -R env ...`
  关 ASLR 跑；`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=0` 可进一步去掉
  canonicalize 并行的分组影响（速度代价另测）。

## 9. 测试

测试布局和 oracle 细节以 `test/README.md` 为准；这里仅保留当前最常用入口。

当前 CTest 已接入：

- `notdec.lifting.wasm`
- `notdec.type_recovery.llvm_ir.tr_level_2`
- `notdec.type_recovery.sysy.tr_level_2`
- `notdec.type_recovery.realworld.tr_level_2`

当前常用 suite：

- `test/lifting/wasm/`
- `test/type-recovery/llvm-ir/`
- `test/type-recovery/sysy/`
- `test/type-recovery/realworld/`

典型命令：

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy|realworld).tr_level_2' --output-on-failure
cmake --build ./build --target TypeBuilderTest binarysub -j4
./build/bin/TypeBuilderTest
./build/binarysub
```

注意：

- `test.sh` 里还有 `--only-opt` 旧调用；当前不要假设该 CLI 选项仍有效
- 跑全量前先确认外部依赖、数据路径、子模块状态
- `test/legacy/` 和各 suite 下的 `legacy/` 默认不是当前 golden

## 10. 修改代码时的建议

建议的阅读顺序：

1. `src/NotDec.cpp`
2. `include/notdec/DecompilerContext.h`
3. `include/notdec/Passes/PassManager.h`
4. `src/Passes/PassManager.cpp`
5. 目标 pass / 类型恢复实现

常见排查方向：

1. `.c` 异常但 `.ll` 正常
   - 先看 `external/NotDec-llvm2c`
2. 变量/内存恢复异常
   - 先看 `src/Passes/`
   - 再看 `src/TypeRecovery/`

## 11. 本文件维护原则

当本文件涉及的内容变化时，应同步更新本文件：

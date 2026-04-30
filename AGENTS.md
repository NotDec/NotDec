# Work guidelines

## 0. 语言风格

即使是在说话和思考的时候，也要保持简洁，不造抽象层次的风格。**这一点非常重要，必须从头到尾始终贯彻，即使是在自己思考的过程中**
1. 说白话，使用更简洁务实的说法，不要过度抽象，不要引入自己造的名词，不要用新术语把问题重新命名。
2. 不要在特别简单的，比如命名，比如用户已经意识到的，或者肯定知道的问题上大费笔墨，而是思考那些真正关键的地方，真正和当前事情相关，更重要的地方。

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

# 项目规范

1. 调试可以使用 `debugmcp`，也可以直接用 `gdb`。建议从 `launch.json` 里的
   `dec` 配置入手；启动时必须显式选择具体配置，并先把参数改到当前要反编译的
   IR。
2. 代码一定要多写注释，特别是新引入的数据结构前，说明背后的设计理念。
3. 工作流程：收到需求 -> 思考后告诉用户打算怎么更改 -> 讨论一致后再开始实现 -> 实现完后再写文档到logs/
4. 写修改日志时，必须明确指出修改了哪个文件的哪一行，涉及哪些函数。
5. 尽量复用并改进之前的日志，最好每个功能都单独一个日志。
6. plan日志重点写问题背景、目标、期望效果、大致技术路线、风险和判断标准，要让没有上下文的人也能看懂；不要过早写成具体实现清单、命令清单或行号清单。实现记录才需要明确写修改了哪个文件的哪一行、涉及哪些函数、验证命令和性能结果。最后，需要从实现效果，复杂度（增加其他人对项目的理解成本），后期维护成本等角度对当前的方案打分，同时思考有没有更好的方案。
7. 如果当前的任务是对之前的plan日志的实现，则不需要单独创建日志，而是将实现情况写入之前的计划日志，比如将计划的步骤在标题中标记为已完成，记录实现细节，以及调整计划时考虑不全而实现时有所改变的部分。同时也不要使得日志文件过于冗长，简洁一些，包括语言风格上，以及没有真正实现，或者试错的思路都尽量简写。
8. 每次改动后都要关注是否造成性能下降。涉及类型恢复、结构体合并、pointer analysis、pass pipeline 时，至少对比 fortune 当前关注用例的同口径运行时间。

当前关注的测试用例： test/type-recovery/realworld/cases/fortune.o3.wasm.ll
源码在：/sn640/NotDec-Exp/ICSE-HOWARD/fortune-mod
当前参考运行时间：2026-04-29，`NOTDEC_POINTER_ANALYSIS_MODE=original`，
`--tr-level=2 --frozen-tr-input-ir -g`，
workdir `/tmp/notdec-fortune-structmerge-hlayout-final`，
`/usr/bin/time -p` 结果 `real 16.33s`。
最新的计划需要看 `logs/` 下最新计划和日志。

## 1. Git 与 external/ 子模块

`external/NotDec-wasm2llvm`、`external/NotDec-llvm2c`、`external/binarysub`
都是 git submodule。检查工作树时注意区分：

- 顶层 `git status --short`
- 子模块内部真实状态，例如
  `git -C external/binarysub status --short`

不要误回滚 `external/` 下已有本地开发态改动。

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
- `external/NotDec-llvm2c/`
  - C backend
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

## 6. 构建

当前仓库依赖本地 LLVM 14，默认布局仍是：

- `llvm-14.0.6.obj`

常用构建命令：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -S . -B ./build -G Ninja

cmake --build ./build --target all
```

产物通常在：

- `build/bin/notdec`

## 7. 运行与调试

调试前先读仓库根目录的 `DEBUG.md`。本文件只保留最小结论，workdir 产物和
调试文件清单统一以 `DEBUG.md` 为准。

当前推荐入口：

- `--gen-work-dir` / `-g`
- `--work-dir=<path>`
- `--emit-tr-input-ir=<path>`
- `--frozen-tr-input-ir`

当前仍常用的环境变量：

- `NOTDEC_SUMMARY_OVERRIDE`
- `NOTDEC_SIGNATURE_OVERRIDE`
- `NOTDEC_EXTRA_CONSTRAINTS`

说明：

- `NOTDEC_SUMMARY_OVERRIDE`
  - summary / lower-bound override，方向是 `OverrideTy <: F`
- `NOTDEC_SIGNATURE_OVERRIDE`
  - 函数自身签名 upper-bound override，方向是 `F <: OverrideTy`
- `NOTDEC_EXTRA_CONSTRAINTS`
  - 额外 MLsub / PNDiff 约束，当前与 frozen stage-B IR 工作流配套使用

典型命令：

```bash
./build/bin/notdec input.wat -o /tmp/out.c --tr-level=2 --gen-work-dir
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3 -g --work-dir=/tmp/notdec-work
```

## 8. 测试

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

## 9. 修改代码时的建议

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

## 10. 本文件维护原则

当本文件涉及的内容变化时，应同步更新本文件：

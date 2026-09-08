# 项目规范

## 开发与记录

1. 代码要有必要的注释；新数据结构前必须说明设计原因，不能只复述字段。
2. 收到需求后先说明打算如何修改，与用户讨论一致后再实现。简单文档、注释和错别字修改不写日志；复杂代码修改完成后必须在 `logs/` 写记录。
3. 修改记录必须指出文件、行、函数、验证命令和结果。优先复用相关旧日志，每个功能的记录保持聚焦。
4. plan 日志写背景、目标、预期效果、大致技术路线、风险和判断标准，顶部必须先保留本次用户原始 prompt。不要过早写成命令或行号清单；实现记录再写具体行号和命令。
5. 实现已有 plan 时直接更新该 plan：标记完成步骤，简要记录实现、验证和计划调整；同一功能不另建日志。
6. 启动 Goal 前必须先生成对应 plan 日志。Goal 分三段：第一段以“基于 logs/xxx.md的规划进行”开始；第二段约 200 字说明技术路线和问题；第三段必须原样写：`遇到了不确定的技术路线选择或计划里明显没有考虑到的事情时暂停goal。`
7. 已讨论一致的实现通过验证后默认直接提交。涉及 submodule 时先在 submodule 内提交，再提交顶层指针和日志；同模块连续文档-only 修改优先 amend。已经确定且尚未 push 到上游的 commit 必须直接 push；出现冲突时不得未经同意 force push。

## Git 与子模块

`external/NotDec-bin2llvm`、`external/NotDec-wasm2llvm`、
`external/NotDec-llvm2c`、`external/binarysub` 是 submodule。检查状态时同时看顶层
`git status --short` 和相关子模块的 `git -C <path> status --short`，不要回滚
`external/` 下已有本地开发态改动。

开发 worktree 的提交要合入 `/sn640/NotDec` 所在目标分支时：

1. 先检查分支、最近提交和各工作树状态。
2. fetch 后将开发分支 rebase 到目标分支。
3. 先完成相关 submodule 提交，再提交顶层指针和日志。
4. 用 `git merge --ff-only <development-branch>` 合入；不能 fast-forward 时停下查明原因。除非用户明确要求挑选提交，否则不用 cherry-pick 代替该流程。
5. 更新主 worktree，执行 `git submodule update --init --recursive`。
6. 最后检查开发 worktree、主 worktree 和相关子模块状态。

## 项目导航

NotDec 以 LLVM IR 连接前后端：`.ll`/`.bc` 直接读取，`.wasm`/`.wat` 经
`external/NotDec-wasm2llvm`，`.c`/`.sol` 经 `external/NotDec-llvm2c`。当前所有源码
backend 都在 `external/NotDec-llvm2c`，包括 C 和 Solidity；后续可以考虑改名为
`notdec-codegen`，现在不改。

主入口是 `src/NotDec.cpp`，负责 CLI、输入分流、context、pass 构造和输出。
`include/notdec/DecompilerContext.h` 定义上下文。pipeline 改动先看
`include/notdec/Passes/PassManager.h` 和 `src/Passes/PassManager.cpp`；类型恢复在
`src/TypeRecovery/`。

## 构建与验证

仓库使用本地 LLVM 22：`llvm-22.1.0.obj`。验证 IR 只能用
`llvm-22.1.0.obj/bin/llvm-as` 和 `llvm-22.1.0.obj/bin/opt`，不能用系统 LLVM。

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -S . -B build -G Ninja
cmake --build build --target all
```

类型推理等价性以 workdir 的 `ValueTypes.txt`、`ValueHTypes.txt`、
`ImportantHTypes.txt`、`VarOrigins.txt`，以及适用时的 `DebugInfoValueTypes.txt` 或
`merge-eval-summary.json` 为准。输出 IR 不编码恢复类型，IR 相同不能证明推理等价；LLVM 22
只用于检查 IR 合法性。

## 按任务必读

- workdir、冻结 stage-A/stage-B IR、调试选项和类型恢复环境变量：先读 `DEBUG.md`。
- 新增或修改类型恢复 merge policy：先读 `docs/src/docs/type-recovery-merge-policies.md`。
- 带 DebugInfo 的源码级 IR merge 评估：先读
  `docs/src/docs/source-ir-merge-evaluation.md`。
- Bench2 时间或内存分析：先读 `docs/src/docs/performance-profiling.md`。
- 测试布局、oracle 和命令：先读 `test/README.md`。
- EVM Solidity 功能：先读 `docs/evm/README.md` 和
  `docs/evm/feature-development-guideline.md`；跑批路径、历史 screen 约定和抽样口径见
  `docs/evm/apehex-batch-loop.md`。该文档不能据此断言 screen 循环正在运行。恢复 Solidity
  语义时对照 `/sn640/solidity` 的 codegen，不能只从样例猜。
- native bin2llvm：先读 `external/NotDec-bin2llvm/AGENTS.md` 及其 `DEBUG.md`。

本文件涉及的规则或阅读入口变化时同步更新。

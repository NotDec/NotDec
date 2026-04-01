# AGNENTS.md

本文件面向在 `NotDec` 仓库内工作的自动化 agent / 开发者，目标是帮助快速定位入口、理解主流程，并在不破坏现有实验环境的前提下进行修改。

## 0. Git 与 external/ 子模块

仓库的 `external/NotDec-wasm2llvm`、`external/NotDec-llvm2c`、`external/binarysub` 是 git submodule。检查工作树时需要注意：如果要看子模块内部的真实状态，使用`git -C external/binarysub status --short`

## 1. 项目概览

`NotDec` 是一个以 LLVM IR 为中间表示的 WebAssembly 反编译与静态分析框架。当前仓库的主链路可以概括为：

1. 前端读取输入文件
   - `.ll` / `.bc` 直接由 LLVM IR parser 加载
   - `.wasm` / `.wat` 由 `external/NotDec-wasm2llvm` 提升为 LLVM IR
2. 中端执行 decompilation / recovery pass
   - 优化
   - 栈/内存恢复
   - 类型恢复
3. 后端按输出后缀决定产物
   - `.ll` 输出 LLVM IR
   - `.bc` 输出 bitcode
   - `.c` 走 `external/NotDec-llvm2c` 生成 C

## 2. 关键入口

主要入口在 `src/NotDec.cpp`。

这个文件负责：

1. 解析命令行参数
   - 关键参数：`-o`、`--tr-level`、`--stack-recovery-algo`、`--log-level`
2. 根据输入后缀装载模块
3. 构造 `notdec::DecompilerContext`
4. 创建 `notdec::passes::DecompileConfig`
5. 调用 `build_passes(trLevel)` 和 `run_passes()`
6. 根据输出后缀写回 `.ll` / `.bc`，或者把 `.c` 输出交给 llvm2c pass

辅助上下文定义在 `include/notdec/DecompilerContext.h`：

- `notdec::Options` 保存 CLI 级别配置
- `notdec::DecompilerContext` 持有 `LLVMContext`、`IRBuilder`、`Module`

## 3. Pass Pipeline

pass 管线的核心实现位于：

- `include/notdec/Passes/PassManager.h`
- `src/Passes/PassManager.cpp`

`PassEnv::build_passes(int level)` 按 `tr-level` 分层构建流程：

### `tr-level >= 1`

执行基础优化，来自 `buildFunctionOptimizations()`，包括：

- `InstCombine`
- `SimplifyCFG`
- `PromotePass`
- `GVN`
- `SCCP`
- `BDCE`
- `ADCE`

### `tr-level >= 2`

进入恢复流程但不做完整 stack breaking：

- `LinearAllocationRecovery`
- `MemsetMatcher`
- `MemcpyMatcher`
- `UndoInstCombine`
- `ReorderBlocksPass`
- `mlsub::MLsubRecoveryMain`

### `tr-level >= 3`

启用更完整的类型恢复与后处理：

- `mlsub::MLsubRecoveryOpt`
- `InstCombine`
- `PromotePass`
- `GVN`
- `BDCE`
- `SimplifyCFG`
- `AllocAnnotator`
- 再次 `ReorderBlocksPass`

如果输出后缀是 `.c`，`DecompileConfig::build_passes()` 会额外调用 `PassEnv::add_llvm2c()`，最终在 pass pipeline 末尾运行 `MLsubNotdecLLVM2C` 写出 C 代码。

## 4. 源码组织

高频目录如下：

- `src/`
  - 主程序与核心 pass 实现
- `include/notdec/`
  - 对应头文件
- `src/TypeRecovery/`
  - 类型恢复实现
- `src/Passes/`
  - decompilation / recovery pass
- `src/Passes/notdec_retypd_provider/`
  - Python 类型推导相关辅助代码
- `external/NotDec-wasm2llvm/`
  - wasm/wat 前端
- `external/NotDec-llvm2c/`
  - C backend
- `external/binarysub/`
  - 外部类型/分析依赖
- `test/`
  - wasm 样例、功能测试数据
- `unittests/`
  - 当前已接入的 CMake 单测
- `docs/`
  - 文档站与设计说明
- `scripts/`
  - LLVM、调试、可视化辅助脚本

## 5. 构建方式

顶层 `CMakeLists.txt` 表明当前仓库依赖本地 LLVM 14 安装，默认路径是：

- `llvm-14.0.6.obj`

构建特征：

- C++17
- 默认 `Debug`
- 可选开启
  - `NOTDEC_ENABLE_WASM`
  - `NOTDEC_ENABLE_LLVM2C`
- 可执行文件目标名：
  - `notdec`（target 名为 `notdec-decompile`，输出名重写为 `notdec`）

常见构建命令以仓库 README 为准，当前推荐模式是：

```bash
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
  -S . -B ./build -G Ninja

cmake --build ./build --target all
```

产物通常在：

- `build/bin/notdec`

## 6. 运行与调试约定

仓库内现有 `run.sh` 体现了一组常用调试环境变量：

- `NOTDEC_DEBUG_DIR`
- `NOTDEC_TYPE_RECOVERY_DEBUG_DIR`
- `NOTDEC_SUMMARY_OVERRIDE`
- `NOTDEC_SIGNATURE_OVERRIDE`
- `NOTDEC_SAT_DISABLE`
- `NOTDEC_DISABLE_INTERPROC`
- `NOTDEC_SAT_NOPTRRULE`

其中：

- 设置 `NOTDEC_DEBUG_DIR` / `NOTDEC_TYPE_RECOVERY_DEBUG_DIR` 后，pass 运行前会把 `00-lifted.ll` 等中间结果落到对应目录
- README 明确建议在类型恢复较重时启用
  - `NOTDEC_DISABLE_INTERPROC=1`
  - `NOTDEC_SAT_DISABLE=1`

典型执行方式：

```bash
./build/bin/notdec input.wat -o /tmp/out.c --tr-level=2
./build/bin/notdec input.bc -o /tmp/out.ll --tr-level=3
```

## 7. 测试现状

当前测试入口不算统一，主要有三类：

1. CMake 单测
   - `unittests/Retypd/GraphTest.cpp`
2. shell 脚本
   - `test.sh`
3. 大量样例/实验脚本
   - `test/`
   - `experiment/`

注意：

- `test.sh` 中存在 `--only-opt` 调用，但当前 `src/NotDec.cpp` 里没有对应命令行参数定义；修改测试或文档时不要假设这个选项仍然有效
- 仓库中有大量实验数据和外部子模块，跑全量测试前先确认依赖和数据路径是否可用

## 8. 修改代码时的建议

### 优先阅读顺序

如果任务从主流程开始，建议按下面顺序读：

1. `src/NotDec.cpp`
2. `include/notdec/DecompilerContext.h`
3. `include/notdec/Passes/PassManager.h`
4. `src/Passes/PassManager.cpp`
5. 目标 pass / 类型恢复实现文件

### 修改 pass 的基本原则

1. 先确认 pass 挂载层级
   - function pass 还是 module pass
   - `tr-level` 哪一层生效
2. 如果改动会影响 `.c` 输出，检查是否连带影响 `llvm2c` 的输入前提
3. 对内存、类型恢复相关改动，优先保留中间 IR dump 能力，方便回归比较
4. 若新增命令行选项，需要同步：
   - `src/NotDec.cpp`
   - 默认值与 `notdec::Options`
   - 必要文档

### 修改构建系统时

1. 优先改顶层 `CMakeLists.txt` 和 `src/CMakeLists.txt`
2. 不要假设系统 LLVM 可直接替代仓库指定的 LLVM 14 布局
3. 外部模块位于 `external/`，修改前先确认是否应在主仓库包装，还是应提交到子模块

## 9. 仓库现状与风险提示

当前工作树不是全干净状态，且包含子模块改动。进行任何代码修改前应先：

1. 用 `git status --short` 确认本次任务涉及的文件是否已被他人改动
2. 避免误回滚 `external/` 下现有变更
3. 避免把实验性脚本状态误写成正式接口文档

特别是：

- `external/NotDec-llvm2c`
- `external/binarysub`

都可能处于本地开发态

## 10. 给后续 Agent 的执行建议

如果你是后续进入该仓库的 agent，默认策略建议如下：

1. 先从 `src/NotDec.cpp` 定位任务影响的输入格式、参数和输出路径
2. 再进入 `PassManager` 确认该问题属于
   - 前端提升
   - 中端优化/恢复
   - 后端 C 生成
3. 如果问题表现为生成的 C 异常，但 `.ll` 正常，优先排查 `external/NotDec-llvm2c`
4. 如果问题表现为变量/内存恢复错误，优先排查
   - `src/Passes/`
   - `src/TypeRecovery/`

## 11. 本文件维护原则

当以下内容发生变化时，应更新本文件：

- 主入口或 CLI 参数发生变化
- pass pipeline 分层发生变化
- 构建依赖版本发生变化
- 推荐运行方式或测试入口发生变化

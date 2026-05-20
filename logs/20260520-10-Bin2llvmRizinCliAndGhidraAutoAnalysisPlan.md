# 2026-05-20 Bin2LLVM rizin CLI 与 Ghidra AutoAnalysis 架构计划

## 用户原始 prompt

```text
分析一下Ghidra和 /sn640/rizin 代码。目前希望bin2llvm在反汇编方面，能够向rizin看齐，即有一个方便的命令行接口可以做反汇编，从而不用做GUI接口。同时，需要参考Ghidra的数据结构，和autoanalysis架构。统计一下rizin的命令行到底提供了哪些功能（不太关注反编译，而是关注反汇编，交叉引用相关功能），同时它的代码架构是什么样的，有哪些数据结构，怎么支撑了背后的功能。其次，尝试基于Ghidra的数据结构架构，实现rizin的这些功能，大致和Ghidra的代码怎么对应得上。最后，规划一下主要有哪几块内容需要实现，如果基于Ghidra的二进制分析的架构的同时，像rizin一样有个命令行接口的功能。大致分哪几大块的功能，以及各个大块的功能的内部大致要分哪几块实现。
```

后续补充：

```text
将当前的这些详细写到一个logs/的新的文档里。
```

## 背景

`external/NotDec-bin2llvm` 的 native 路线现在已经有 LIEF、Sleigh/libsla、PLT relocation、entry、symbol、eh_frame 等基础。
但它仍更像一组工具和 analyzer 雏形，还没有形成一个适合命令行查询的程序分析状态。

这次目标不是改 lowering，也不是做 GUI，而是规划一条路线：

1. 底层程序状态和分析调度参考 Ghidra。
2. 命令行能力参考 rizin。
3. 指令语义继续使用 Sleigh/P-Code。
4. LLVM lowering 消费分析结果，不再自己承担函数发现和 CFG 恢复。

## rizin 命令行能力

rizin 的相关命令定义主要在：

1. `/sn640/rizin/librz/core/cmd_descs/cmd_print.yaml`
2. `/sn640/rizin/librz/core/cmd_descs/cmd_analysis.yaml`

命令 handler 主要在：

1. `/sn640/rizin/librz/core/cmd/cmd_print.c`
2. `/sn640/rizin/librz/core/cmd/cmd_analysis.c`
3. `/sn640/rizin/librz/core/disasm.c`
4. `/sn640/rizin/librz/core/canalysis.c`

### 反汇编输出

重点命令：

1. `pD`：按字节数反汇编。
2. `pd`：按指令数反汇编。
3. `pda`：逐字节尝试所有可能 opcode。
4. `pdb`：反汇编当前 basic block。
5. `pdf`：反汇编当前函数。
6. `pde`：按执行流从当前 PC 继续反汇编。
7. `pdr` / `pdr.`：递归反汇编函数图或当前 block。
8. `pdR`：不依赖已有函数，递归反汇编一段 block size。
9. `pdJ` / `pdbJ`：JSON 输出。
10. `pi` / `pif` / `pib` / `pir`：更轻量的指令列表。
11. `pa` / `pad` / `pix`：汇编和 hex 互转。
12. `pxr`：按 word 扫描并展示引用。

这里值得学的是功能形态，不是命令名字本身。bin2llvm 第一版不需要完整 shell，只要有等价 CLI 子命令即可。

### 分析命令

重点命令：

1. `aa`：分析 `sym.*` 和 entry。
2. `aaa`：分析 calls、refs、emulation、signatures。
3. `aaaa`：实验性更激进分析。
4. `aas`：只分析 symbols / entries。
5. `aaf`：分析所有函数。
6. `af` / `afr`：从当前位置递归分析函数。
7. `afb`：列 basic blocks。
8. `afb+` / `afb-`：手工加删 basic block。
9. `afl`：列函数。
10. `afi`：函数信息。
11. `afx`：函数引用。
12. `afj`：jump table。
13. `aar`：分析当前 section 或指定字节范围内 xrefs。
14. `aad`：分析 data references to code。
15. `aae` / `aaef`：用 ESIL 做引用分析。
16. `aap`：搜索 prelude 并创建函数。
17. `aav`：分析指向 section/map 的值。

bin2llvm 里可以先只对齐这些核心能力：

1. 分析入口和 symbols。
2. 递归函数分析。
3. 列函数、函数信息、basic blocks。
4. 分析和查询 xrefs。
5. 输出 CFG / callgraph。

ESIL、交互编辑、复杂变量命令不属于第一阶段。

### XRef 命令

重点命令：

1. `ax`：从当前地址手工加 xref。
2. `axc`：加 code xref。
3. `axC`：加 call xref。
4. `axd`：加 data xref。
5. `axs`：加 string xref。
6. `axt`：列到当前地址的 xrefs。
7. `axf`：列从当前地址发出的 xrefs。
8. `axl`：列全部 xrefs。
9. `ax-` / `ax-*`：删除 xrefs。
10. `axg`：画到当前函数的 xref 图。
11. `axtg`：输出生成 xref 图的命令。

对 bin2llvm 最重要的是 `to/from/list` 三类查询。手工增删 xref 可以后置。

### 图命令

重点命令：

1. `agf`：函数 basic block CFG。
2. `agc`：函数 callgraph。
3. `agC`：全局 callgraph。
4. `agx`：xref graph。
5. `agr` / `agR`：局部/全局 references graph。
6. `agF`：不含 calls 的 CFG。
7. `agCi`：inter-procedural CFG。

bin2llvm 第一版建议输出 DOT 和 JSON，不做图片渲染。

## rizin 代码架构

rizin 大体分四层。

### 1. 命令层

命令定义在 `librz/core/cmd_descs/*.yaml`，生成注册代码。
具体 handler 在 `librz/core/cmd/*.c`。

这层负责：

1. 参数解析。
2. 输出模式选择，比如 standard、quiet、table、json。
3. 调用 core 层 API。

它不直接做复杂分析。

### 2. Core 层

`RzCore` 串起 IO、bin、analysis、asm、flags、config。

相关文件：

1. `/sn640/rizin/librz/core/disasm.c`
2. `/sn640/rizin/librz/core/canalysis.c`
3. `/sn640/rizin/librz/core/cgraph.c`

这层负责把命令转成分析操作或展示操作。

### 3. Analysis 层

核心头文件：

`/sn640/rizin/librz/include/rz_analysis.h`

关键数据结构：

1. `RzAnalysisFunction`
   - 函数名、入口、bits、calling convention、返回类型、变量、basic blocks、noreturn、imports。
2. `RzAnalysisBlock`
   - block 地址、大小、jump、fail、switch、指令 offset、SP delta、所属函数。
3. `RzAnalysisOp`
   - 单条指令摘要，包含 mnemonic、type、size、jump、fail、ptr、val、ESIL、IL、source/dest。
4. `RzAnalysisXRef`
   - `from`、`to`、`type`，type 包括 code、call、data、string。
5. `RzAnalysisTaskItem`
   - 递归函数分析的任务项，包含当前函数、block、SP、起始地址。

关键点是：CLI 不是每次重新分析。分析结果存在 `RzAnalysis`，后续 `pd/pdf/afl/axt/agf` 都是不同视图。

### 4. Arch / Asm 插件层

不同架构的指令 decode 和语义摘要在：

1. `/sn640/rizin/librz/arch/p/analysis/*`
2. `/sn640/rizin/librz/arch/p/asm/*`

`RzAnalysisPlugin::op` 是核心接口。它给出 `RzAnalysisOp`，上层再据此建函数、block 和 xref。

## Ghidra 架构对应

Ghidra 的重点不是命令系统，而是 Program database 和 AutoAnalysis。

### Program 数据结构

入口接口：

`/sn640/ghidra/Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Program.java`

它把程序分成几块：

1. `Listing`
2. `FunctionManager`
3. `SymbolTable`
4. `Memory`
5. `ReferenceManager`
6. `DataTypeManager`

这和 rizin 的 `RzCore + RzAnalysis + RzBin + RzAsm` 类似，但 Ghidra 的边界更清楚。

### Function

接口：

`/sn640/ghidra/Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/listing/Function.java`

重点字段和能力：

1. 单一 entry point。
2. function body。
3. 参数、返回类型、calling convention。
4. thunk、inline、noreturn。
5. 注释和 repeatable comment。

### AutoAnalysis

关键文件：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/services/AbstractAnalyzer.java`
3. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/services/Analyzer.java`

它做的事：

1. 管理 analyzer 列表。
2. 按 analyzer type 和 priority 调度。
3. 监听 Program 变化。
4. analyzer 把结果写回 Program。
5. 新增 code、function、reference 后继续触发后续 analyzer。

bin2llvm 应该学这个模式，不要把所有 native discovery 写进一个大函数。

### Ghidra 入口和引用分析

入口分析：

`/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`

主要策略：

1. 只处理 executable address set。
2. code symbols 进入 `doNowSet`。
3. 可疑入口进入 `doLaterSet`。
4. 用 `PseudoDisassembler.isValidSubroutine(...)` 过滤可疑入口。
5. 先 disassemble，再创建或修正 function body。

引用分析：

`/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java`

主要策略：

1. 遍历已有 instruction reference。
2. 识别 string、pointer、address table、subroutine。
3. relocation 可以辅助 pointer 分析。
4. 发现新 code 后调度 disassembly。
5. switch table 和 address table 单独处理。

## bin2llvm 当前对应关系

当前 native 侧已有这些基础：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h`
2. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp`
3. `external/NotDec-bin2llvm/include/notdec-bin2llvm/LiefElfLoadImage.h`
4. `external/NotDec-bin2llvm/lib/LiefElfLoadImage.cpp`
5. `external/NotDec-bin2llvm/include/notdec-bin2llvm/SleighLift.h`
6. `external/NotDec-bin2llvm/lib/SleighLift.cpp`

已存在的概念：

1. `NativeProgramState`
2. `NativeMemoryRange`
3. `NativeFunctionSeed`
4. `NativeRelocationInfo`
5. `NativePltEntry`
6. `NativeEhFrameStats`
7. `NativeAnalyzer`
8. `NativeAnalysisManager`

这些和 Ghidra / rizin 的对应关系：

1. Ghidra `Program` -> `NativeProgramState`
2. Ghidra `Memory` -> `NativeMemoryRange`
3. Ghidra `FunctionManager` -> 当前 `NativeFunctionSeed`，后续需要扩成 `NativeFunctionStore`
4. Ghidra `ReferenceManager` -> 后续 `NativeXRefStore`
5. Ghidra `Listing` -> 后续 `NativeInstructionStore` 和 `NativeBlockStore`
6. Ghidra analyzer -> `NativeAnalyzer`
7. rizin `RzAnalysisOp` -> libsla decode 后的 native op summary
8. rizin `RzAnalysisBlock` -> 后续 `NativeBasicBlock`
9. rizin `RzAnalysisXRef` -> 后续 `NativeXRef`

当前最大缺口：

1. 只有 function seed，还没有确认后的 function/block/instruction/xref 存储。
2. 分析调度仍偏线性，还没有真正 worklist。
3. 反汇编和 xref 查询没有统一 CLI。
4. recursive disassembly 还没有形成稳定 CFG。
5. P-Code lowering 和函数发现仍没有清晰分层。

## 建议总体路线

不要直接复刻 rizin 的整体架构。

更合适的路线是：

1. Program state 和 AutoAnalysis 学 Ghidra。
2. CLI 功能面学 rizin。
3. 指令语义学 Ghidra/Sleigh。
4. 输出查询方式做成简单 CLI，不做 GUI。

## 主要实现分块

### 1. Program State 层

目标：先把共享状态补齐。

建议拆成：

1. `NativeMemoryMap`
   - segment / section。
   - read bytes。
   - read pointer。
   - executable check。
2. `NativeSymbolStore`
   - ELF symbols。
   - dynamic symbols。
   - PLT externals。
   - entry labels。
3. `NativeFunctionStore`
   - function seed。
   - confirmed function。
   - function range。
   - aliases。
   - source / confidence。
4. `NativeInstructionStore`
   - address 到 decoded instruction。
   - instruction bytes。
   - mnemonic。
   - P-Code ops。
   - op summary。
5. `NativeBlockStore`
   - basic block。
   - jump / fail。
   - successors。
   - 所属函数。
6. `NativeXRefStore`
   - `from`。
   - `to`。
   - type。
   - source。
   - to/from/list 查询。
7. `NativeMetaStore`
   - comment。
   - string。
   - data/code 标记。
   - 第一版可以很薄。

判断标准：

1. CLI 查询不需要重新 decode 全程序。
2. analyzer 之间可以通过 state 交换结果。
3. lowering 只读 state，不自己重新发现函数。

### 2. AutoAnalysis 调度层

目标：把当前固定顺序 analyzer 推进到轻量 worklist。

建议拆成：

1. analyzer 注册。
2. priority。
3. enable / disable。
4. worklist item。
5. event 类型。
6. 每个 analyzer 的统计。

第一批 worklist 类型：

1. `FunctionSeed`
2. `InstructionRange`
3. `XRefTarget`
4. `DataPointer`
5. `Report`

第一版仍可以单线程。先把边界做清楚，不急着并行。

### 3. Loader / Entry Discovery 层

目标：复刻 Ghidra loader 和 EntryPointAnalyzer 里对当前 Bench2 有用的部分。

建议拆成：

1. ELF segment / section load。
2. `STT_FUNC` symbol。
3. export / dynamic symbol。
4. `DT_INIT` / `DT_FINI`。
5. init/fini array。
6. relocation。
7. PLT stub 到 external symbol。
8. `.eh_frame_hdr` / `.eh_frame` 的 FDE start/range。
9. prelude scan，低优先级。

第一版策略：

1. 高可信入口直接入队。
2. 可疑入口先保留 confidence，不直接 lower。
3. 地址必须落在 executable range。
4. shared object 的 `e_entry = 0` 不当作有效函数。

### 4. Recursive Disassembly / CFG 层

目标：从“手动地址 + 长度线性 decode”改成“函数入口递归 decode CFG”。

建议拆成：

1. libsla instruction decode。
2. native op summary。
3. block builder。
4. function builder。
5. overlap / conflict 处理。
6. unresolved indirect flow 记录。

op summary 至少要有：

1. address。
2. size。
3. mnemonic。
4. op type。
5. jump。
6. fail。
7. call target。
8. data pointer。
9. return / stop 标记。

控制流规则：

1. direct jump 进入当前函数 CFG。
2. conditional jump 同时加入 jump/fail。
3. direct call 生成 call xref，并把 target 作为函数 seed。
4. ret 停止当前 block。
5. invalid / undecodable 停止当前 block，并记录 note。
6. indirect jump/call 先记录 unresolved，不猜目标。

### 5. Reference Analysis 层

目标：支撑 `axt/axf/axl/agx`，同时帮助发现更多代码。

建议拆成：

1. direct code refs。
2. direct call refs。
3. data refs。
4. string refs。
5. pointer table。
6. jump table。
7. relocation guided refs。

第一版只做：

1. CFG decode 产生 code/call refs。
2. op summary 里的直接 pointer 产生 data refs。
3. 指向可打印字符串的 data ref 标成 string。
4. relocation 结果参与 pointer read。

jump table 可以后置。不要用过度猜测污染函数边界。

### 6. CLI / 输出层

目标：像 rizin 一样不用 GUI 就能做反汇编和查询。

建议先做一个工具，或者几个小工具。命名可以后面定。

第一组命令：

1. `analyze`
   - 类似 `aa/aaa/aas/aaf/aar`。
   - 参数选择 analyzer 集合。
2. `disasm`
   - 类似 `pd/pD/pi`。
   - 支持 address、instruction count、byte count。
3. `function`
   - 类似 `afl/afi/afb/pdf`。
   - 列函数、看函数信息、看 blocks、看函数反汇编。
4. `xref`
   - 类似 `axt/axf/axl`。
   - 支持 to/from/list。
5. `graph`
   - 类似 `agf/agc/agx`。
   - 输出 DOT / JSON。

输出格式：

1. human。
2. table。
3. JSON。

第一版不做：

1. 交互式 shell。
2. TUI。
3. 手工编辑函数和 xref。
4. 反编译。

## 阶段计划

### 阶段 1：补 state

目标：

1. 增加 function/block/instruction/xref 存储。
2. 现有 analyzer 结果写入这些 store。
3. report analyzer 从 store 输出统计。

判断标准：

1. 能列出 function seeds 和 confirmed functions。
2. 能列出 xrefs。
3. 能解释每个结果的来源。

### 阶段 2：递归反汇编

目标：

1. 从 function seed 出发 decode。
2. 建 block。
3. 建 CFG。
4. 生成 call/code xrefs。

判断标准：

1. 不再要求用户手动传函数长度。
2. 能输出 `pdf` 等价的函数反汇编。
3. 能输出 `agf` 等价 CFG。

### 阶段 3：CLI 第一版

目标：

1. 能运行分析。
2. 能列函数。
3. 能反汇编地址、block、函数。
4. 能查 xref。
5. 能输出 JSON。

判断标准：

1. 常用排查不需要 Ghidra GUI。
2. 同一个二进制可以重复跑，输出稳定。
3. JSON 足够给脚本对比。

### 阶段 4：引用增强

目标：

1. data refs。
2. string refs。
3. pointer table。
4. relocation guided refs。
5. 简单 jump table。

判断标准：

1. 函数覆盖率接近 GhidraScript heritage 路线。
2. xref 查询能解释更多函数发现来源。
3. 错误 target 明显减少，不能靠猜测扩大覆盖率。

### 阶段 5：接 lowering

目标：

1. LLVM lowering 使用 NativeProgramState 里的函数和 CFG。
2. lowering 不再自行做函数发现。
3. native 路线和 Ghidra heritage 路线保留对比能力。

判断标准：

1. native mode 能对 Bench2 目标输出模块级 LLVM IR。
2. 函数数量、call refs、失败原因可以和 GhidraScript 路线对比。
3. regression 输出可脚本化。

## 风险

### 1. 过早做完整 rizin shell

风险：CLI 变复杂，但 state 仍不完整。

处理：第一版只做子命令和 JSON，不做交互 shell。

### 2. 入口发现靠猜测

风险：函数数变多，但错误函数也变多，后续 lowering 更难排错。

处理：所有 seed 带 source 和 confidence。低可信入口先记录，验证后再确认。

### 3. Ghidra Program 复刻过重

风险：把数据库、类型、UI 相关能力都搬进来，范围失控。

处理：只做 native lifter 需要的最小 Program state。

### 4. 指令语义重复实现

风险：绕开 Sleigh 手写太多架构规则。

处理：decode 和 P-Code 仍以 libsla 为准；native op summary 只做控制流和引用摘要。

### 5. jump table 污染 CFG

风险：早期 jump table 误识别会大量污染函数边界。

处理：jump table 后置，先记录 unresolved indirect jump。

## 不做什么

第一阶段不做：

1. rizin 交互 shell。
2. TUI。
3. 反编译。
4. 完整 ESIL 等价层。
5. 完整类型恢复。
6. 完整 Ghidra ProgramDB。
7. 复杂 jump table 和异常 landing pad 的完整恢复。

## 结论

bin2llvm native 路线要向 rizin 看齐，重点不是复制 rizin 的内部实现，而是提供同类命令行能力。

底层应采用 Ghidra 风格：

1. 统一 Program state。
2. analyzer 分层。
3. worklist 调度。
4. 结果写回 state。
5. CLI 只做查询和触发分析。

上层应采用 rizin 风格：

1. 反汇编地址、block、函数。
2. 列函数和 basic blocks。
3. 查询 xrefs。
4. 输出 CFG、callgraph、xref graph。
5. 默认支持 JSON，方便脚本和回归测试。


# 2026-05-20 Bin2LLVM native AutoAnalysis 入口发现计划

## 用户原始 prompt

```text
按照这个方式重写计划吧
```

## 前置讨论

```text
当前bin2llvm项目的native路线（不使用GhidraScript调用Java层的路线），目前还需要手动输入一个函数入口点，目前函数入口点识别目前做的还不是特别好，没有从 ELF symbol、.eh_frame、relocation、PLT、entry、export table 出发做递归下降。也没有“发现新函数 -> 加入 worklist -> 继续 decode”的流程。目前，需要将它进一步向完整的完整 binary lifter的方向推进，具体的算法直接复刻Ghidra的策略。参考Ghidra的源码（/sn640/ghidra）规划一下，并且写到一个logs文件 
目前使用Bench2里面的libuv进行测试
```

```text
重新写一下，按照这样的思路：本次主要关注实现算法发现更多的入口信息，从而识别出更多的函数。首先是介绍Ghidra相关的部分是怎么实现的，代码具体是哪些文件。其次就是按照复刻Ghidra的策略的思路去规划接下来怎么做。
```

```text
思路是这个思路，但是如果真的有G策略做的不好的地方，也可以在计划里写
```

```text
现在考虑，要不还是直接做完整的AutoAnalysis调度，复刻为C++版本，但是为了敏捷开发，先做测试用例用到的部分？
```

## 背景

这次目标还是“发现更多入口，从而识别更多函数”，但实现方式改一下。

之前的思路是先做一条比较直的 discovery pipeline。现在调整为：直接按 Ghidra AutoAnalysis 的思想做 C++ 版调度框架，但第一版只实现 libuv 当前会用到的 analyzer。

这样做的原因：

1. 函数入口发现本来就是多轮过程。ELF symbol、dynamic entry、eh_frame、relocation、PLT、direct call 之间会互相补信息。
2. 如果只做一次性 pipeline，后面加 jump table、函数指针、noreturn、thunk、eh_frame landing pad 时会越来越乱。
3. 直接做完整 Ghidra Program database 又太大，不适合当前阶段。

所以第一版要做的是“轻量 AutoAnalysis”，不是完整 Ghidra 复刻。

当前 native 路线已有基础：

1. LIEF 能读 ELF。
2. `LiefElfLoadImage` 能把 executable `PT_LOAD` 映射给 libsla。
3. `collectSleighPcode(...)` 能从手工给的地址和长度收集 P-Code。
4. `PcodeToLLVM` 能把一段 P-Code lower 到 LLVM IR。

当前缺口：

1. 仍要手动传 `-a/-l`。
2. 没有统一的 program state，analyzer 之间不能共享发现结果。
3. 没有调度队列，也没有“发现新地址 -> 安排后续分析”的机制。
4. libuv 旧 GhidraScript 路线能 lower 478 个函数，native 路线目前还远不到这个覆盖面。

## libuv 当前基线

测试目标：

`/sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0`

当前已知信息：

1. ELF 是 x86-64 shared object，`e_entry = 0x0`。
2. defined `FUNC` symbol 有 307 个。
3. `DT_INIT = 0x8000`，`DT_FINI = 0x29240`。
4. `DT_INIT_ARRAY` 里有 `0x9e30`。
5. `DT_FINI_ARRAY` 里有 `0x9df0`、`0x9c70`。
6. `.plt`、`.plt.got`、`.plt.sec` 存在。
7. `.eh_frame_hdr`、`.eh_frame` 存在。
8. GhidraScript 旧链路能导出并 lower 478/478 个函数。

判断：

1. `e_entry` 对 shared object 帮助不大。
2. libuv 第一批入口主要来自 symbol、init/fini、eh_frame、PLT 和后续 direct flow。
3. 它适合验证 C++ AutoAnalysis 调度：入口来源多，但规模还可控。

## Ghidra 相关实现

Ghidra 的关键点不是某一个 analyzer，而是 loader 和 analyzer 之间共享同一个 Program state，并通过 AutoAnalysis 反复调度。

### 1. AutoAnalysis 调度

源码重点：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/services/AbstractAnalyzer.java`
3. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/services/Analyzer.java`

它做的事：

1. 管理 analyzer 列表和优先级。
2. 每个 analyzer 只处理自己关心的 address set。
3. 一个 analyzer 发现新 code 或新 reference 后，可以调度后续 analyzer。
4. 分析结果写回 Program，后面的 analyzer 继续读。

native 侧要学的是这个模式：

1. 不把所有逻辑写在一个大函数里。
2. 每个 analyzer 只做一件事。
3. analyzer 通过统一 state 交换结果。
4. 新发现能重新入队。

### 2. ELF loader 创建初始入口

源码：

`/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/ElfProgramBuilder.java`

关键函数：

1. `processEntryPoints(...)`
2. `createDynamicEntryPoints(...)`
3. `createEntryFunction(...)`
4. `processRelocations(...)`

策略：

1. 读 `e_entry`。
2. 读 `DT_INIT`、`DT_FINI`。
3. 读 `DT_INIT_ARRAY`、`DT_PREINIT_ARRAY`、`DT_FINI_ARRAY`。
4. 入口必须落在 executable memory block。
5. relocation 会先尽量应用到 Program memory，让后续 analyzer 读到真实指针。

native 第一版要复刻：

1. dynamic entry 和 init/fini array。
2. executable 地址检查。
3. 和入口发现直接相关的 relocation。

### 3. EntryPointAnalyzer 处理符号和入口

源码：

`/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/EntryPointAnalyzer.java`

关键函数：

1. `added(...)`
2. `addCodeSymbolsToSet(...)`
3. `addExternalSymbolsToSet(...)`
4. `moveSuspectSymbolsToDoLaterSet(...)`
5. `checkDoLaterSet(...)`
6. `doDisassembly(...)`

策略：

1. 只处理 executable address set。
2. code symbol 进入 `doNowSet`。
3. 可疑入口进入 `doLaterSet`。
4. `doLaterSet` 用 `PseudoDisassembler.isValidSubroutine(...)` 过滤。
5. 反汇编成功后再创建函数。

native 第一版要复刻：

1. `STT_FUNC` symbol 作为高可信入口。
2. 入口可信度分层。
3. 可疑入口先记录或验证，不直接 lower。

### 4. OperandReferenceAnalyzer 从引用里发现新代码

源码：

`/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java`

关键函数：

1. `added(...)`
2. `getAddressTable(...)`
3. `createFlowTable(...)`
4. `createDisassemblyCommandsForAddress(...)`
5. `checkForExternalJump(...)`
6. `checkForPointer(...)`

策略：

1. 遍历已有 instruction reference。
2. flow reference 按 call/jump 处理。
3. 目标像代码时，先检查 executable、read/write 引用、普通计算使用、subroutine 合法性。
4. address table / jump table 单独处理。
5. 发现新代码后调度后续 disassembly。

native 第一版要复刻：

1. decode 后收集 direct call/jump。
2. direct call 到 executable 地址时生成新函数入口。
3. indirect target 和 address table 先记录为 unresolved。

### 5. GCC exception analyzer 处理 eh_frame

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/GccExceptionAnalyzer.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/`

关键函数：

1. `canAnalyze(...)`
2. `handleStandardSections(...)`
3. `analyzeEhFrameHeaderSection(...)`
4. `handleDebugFrameSection(...)`
5. `processCallSiteRecord(...)`

策略：

1. 检查 `.eh_frame_hdr`、`.eh_frame`、`.debug_frame`。
2. 解析 FDE，得到函数范围。
3. 处理 LSDA call site 和 landing pad。

native 第一版要复刻：

1. 先只取 FDE start 和 range。
2. FDE start 作为函数入口。
3. FDE range 作为 decode 边界。

### 6. Disassembler 递归跟随 flow

源码：

`/sn640/ghidra/Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/disassemble/Disassembler.java`

关键函数：

1. `disassemble(...)`
2. `disassembleNextInstructionSet(...)`
3. `disassembleInstructionBlock(...)`
4. `processInstructionFlows(...)`

策略：

1. 从 seed 地址开始。
2. 按指令 flow 继续排队。
3. direct branch/call 形成新 flow。
4. 不能解析、越界、冲突、return 时停止当前 block。

native 第一版要复刻：

1. 不再按固定长度线性 decode。
2. 以函数入口为 seed，递归 decode CFG。
3. direct flow 一边建函数内 CFG，一边给模块分析提供新入口。

## 总体方案：C++ 轻量 AutoAnalysis

不要一上来复制 Ghidra 的完整 Program database。先做一个小的 C++ 调度框架，接口向完整 AutoAnalysis 靠，但只实现当前用到的能力。

建议新增模块：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h`
2. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp`
3. `external/NotDec-bin2llvm/tools/notdec-native-discover.cpp`

后面如果代码变大，再按 analyzer 拆文件。

### 核心状态

第一版 `NativeProgramState` 只需要这些：

```text
MemoryMap:
  executable segments
  readable/writable segments
  readBytes/readPointer/isExecutable

SymbolTable:
  symbols by address
  external symbols
  names and sizes

RelocationTable:
  relocation entries needed by discovery
  relocated pointer lookup
  PLT/GOT mapping

FunctionTable:
  function seeds
  decoded functions
  ranges
  source/confidence
  decode/lower status

ReferenceTable:
  direct calls
  direct jumps
  external calls
  unresolved indirect targets
```

这里需要多写注释。重点说明：这不是 Ghidra Program 的完整替代，只是 native lifter 当前需要的共享状态。

### 调度器

第一版 `NativeAnalysisManager` 做这些：

1. 注册 analyzer。
2. 按优先级执行 analyzer。
3. 维护 address/function worklist。
4. analyzer 改变 state 后，可以重新安排别的 analyzer。
5. 最后输出 discovery report。

接口可以很小：

```text
NativeAnalyzer:
  name()
  priority()
  run(NativeProgramState &, NativeAnalysisManager &)

NativeAnalysisManager:
  addFunctionSeed(...)
  enqueueFunction(...)
  enqueueAnalyzer(...)
  run()
```

第一版不要做复杂并发，也不要做持久化数据库。

## 第一批 analyzer

按 libuv 当前需求，先实现这些。

### 1. ElfLoadAnalyzer

作用：

1. 用 LIEF 读取 ELF 基础信息。
2. 建立 `MemoryMap`。
3. 建立 section / segment 索引。
4. 记录 architecture 和 pointer size。

输入：

1. ELF path。
2. LIEF `Binary`。

输出：

1. executable segment 列表。
2. readable segment 列表。
3. section address map。

验收：

1. libuv 能识别 `.text`、`.plt`、`.plt.sec`、`.init`、`.fini`。
2. `isExecutableAddress(0x9c70)` 为 true。

### 2. ElfEntryAnalyzer

作用：

复刻 `ElfProgramBuilder.processEntryPoints(...)` 里和入口相关的部分。

处理：

1. `e_entry`。
2. `DT_INIT`。
3. `DT_FINI`。
4. `DT_INIT_ARRAY`。
5. `DT_PREINIT_ARRAY`。
6. `DT_FINI_ARRAY`。

规则：

1. 地址为 0 不加入。
2. 必须落在 executable segment。
3. array 元素用 relocation-aware pointer 读取。

libuv 验收：

1. 不加入 `e_entry = 0x0`。
2. 加入 `0x8000`、`0x29240`、`0x9e30`、`0x9df0`、`0x9c70`。

### 3. ElfSymbolAnalyzer

作用：

复刻 `EntryPointAnalyzer.addCodeSymbolsToSet(...)` 的 native 简化版。

处理：

1. `.symtab`。
2. `.dynsym`。
3. defined `STT_FUNC`。

规则：

1. 地址必须 executable。
2. symbol size 非 0 时记录 range。
3. 同地址多名字时保留主名和 alias。

libuv 验收：

1. 发现 307 个 defined FUNC symbol。
2. 每个 seed 来源为 `elf-symbol`。

### 4. RelocationPltAnalyzer

作用：

只处理入口发现和 call target 解析需要的 relocation / PLT。

处理：

1. `R_X86_64_RELATIVE`。
2. `R_X86_64_JUMP_SLOT`。
3. `R_X86_64_GLOB_DAT`。
4. `DT_JMPREL` / `.rela.plt`。
5. `.plt` / `.plt.sec` stub 地址和外部 symbol 的对应关系。

不做：

1. 不先实现完整 relocation engine。
2. 不处理所有架构。
3. 不把所有 relocated executable pointer 都直接升函数。

libuv 验收：

1. 能列出 PLT 外部符号。
2. direct call 到 PLT stub 时能解析到外部函数名。

### 5. EhFrameAnalyzer

作用：

复刻 `GccExceptionAnalyzer` 的最小入口发现部分。

处理：

1. `.eh_frame_hdr`。
2. `.eh_frame`。
3. FDE initial location。
4. FDE range。

不做：

1. 不先处理完整 LSDA。
2. 不先恢复异常语义。
3. landing pad 先记录，不自动当函数。

libuv 验收：

1. discovery report 出现 `eh-frame` source。
2. 有 symbol 之外的 FDE start 被补成 seed。
3. FDE range 能被 `FunctionDecodeAnalyzer` 用作边界。

### 6. FunctionDecodeAnalyzer

作用：

把 `collectSleighPcode(address, length)` 改成以函数 seed 为单位递归 decode。

处理：

1. 从 `FunctionTable` 取未 decode seed。
2. 用 libsla decode 指令。
3. 建 basic block worklist。
4. direct branch 留在当前函数 CFG。
5. direct call 到 executable 地址时加入新 function seed。
6. direct call 到 PLT 时记录 external call。
7. indirect jump/call 记录 unresolved。

边界优先级：

1. eh_frame range。
2. symbol size。
3. 下一个已知函数入口。
4. 当前 executable segment 末尾。

停止条件：

1. return。
2. unconditional branch 无 fallthrough。
3. invalid decode。
4. 跳出可信 range。
5. 遇到另一个已知函数入口。

libuv 验收：

1. 不需要 `-a/-l`，能从 seed 队列 decode。
2. direct-flow seed 数量可见。
3. 串函数情况有报告。

### 7. ReportAnalyzer

作用：

输出调度结果，方便先验证算法，不急着全部 lower。

报告内容：

1. seeds 总数。
2. 按 source 统计。
3. 按 confidence 统计。
4. decoded / failed / skipped 数量。
5. external calls 数量。
6. unresolved indirect target 数量。
7. 每个失败函数的原因。

libuv 验收：

1. 能看出初始入口、eh_frame 补入口、direct-flow 补入口分别贡献多少。
2. 能和 GhidraScript 旧链路的 478 函数做数量对比。

## 调度顺序

第一版顺序可以固定，不需要复杂优先级系统：

```text
ElfLoadAnalyzer
RelocationPltAnalyzer
ElfEntryAnalyzer
ElfSymbolAnalyzer
EhFrameAnalyzer
FunctionDecodeAnalyzer until worklist empty
ReportAnalyzer
```

后续如果需要更像 Ghidra，再把 fixed order 改成 priority queue。

这里的关键是 `FunctionDecodeAnalyzer` 能产生新 seed，并重新入队自己继续处理。

## 和 Ghidra 不完全一致的地方

### 1. 只做测试用例用到的 analyzer

完整 Ghidra AutoAnalysis 很大。当前只做 libuv 需要的部分，不先做：

1. byte pattern function start analyzer。
2. full data type propagation。
3. full reference analyzer。
4. full relocation engine。
5. full exception semantics。

### 2. 低可信入口不自动 lower

Ghidra 有些“看起来像代码”的判断比较激进。native 第一版要保守。

分层：

1. `high`：dynamic entry、FUNC symbol、eh_frame FDE start。
2. `medium`：direct call target、PLT stub、relocated function pointer。
3. `low`：普通数据引用、address table、间接目标推测。

默认只 decode high/medium。low 只写报告。

### 3. 不默认线性扫描 `.text`

线性扫描可能发现更多函数，但误判也多。libuv 当前有 symbol 和 eh_frame，不需要第一步就扫 `.text`。

可以保留 `--aggressive-scan`，默认关闭。

### 4. relocation 先服务入口发现

Ghidra 会尽量应用 relocation。native 第一版只实现入口发现和 PLT 解析必须的 relocation。

未支持 relocation 要出现在 report 里，不要静默忽略。

## 实施顺序

### 第一步：只做框架和报告

实现：

1. `NativeProgramState`。
2. `NativeAnalysisManager`。
3. `ElfLoadAnalyzer`。
4. `ElfEntryAnalyzer`。
5. `ElfSymbolAnalyzer`。
6. `ReportAnalyzer`。
7. `notdec-native-discover`。

验收：

1. libuv 输出 307 个 symbol seed。
2. libuv 输出 init/fini seed。
3. 不改现有 lowering。

### 第二步：补 relocation / PLT

实现：

1. `RelocationPltAnalyzer`。
2. relocation-aware pointer read。
3. PLT stub -> external symbol mapping。

验收：

1. libuv 能列出 PLT 外部符号。
2. init/fini array 读取不依赖未修正字节。

### 第三步：补 eh_frame

实现：

1. `EhFrameAnalyzer`。
2. FDE start/range。
3. range 写入 `FunctionTable`。

验收：

1. libuv seed 数量超过 307。
2. report 能显示哪些 seed 来自 eh_frame。

### 第四步：函数递归 decode

实现：

1. `FunctionDecodeAnalyzer`。
2. 函数内 block worklist。
3. direct call 生成新 function seed。
4. PLT call 生成 external call。

验收：

1. 不传 `-a/-l` 能跑 libuv native discover/decode。
2. report 里 direct-flow seed 可见。
3. decode 失败有原因分类。

### 第五步：接 LLVM module 输出

实现：

1. 把 decoded function 集合接到现有 P-Code lowering。
2. 生成多函数 LLVM module。
3. 失败函数保留 declaration 或报告，不让整个模块崩。

验收：

1. libuv native 路线能输出 `.ll`。
2. `llvm-22.1.0.obj/bin/llvm-as` 能接受输出。

## libuv 判断标准

短期：

1. 不手动输入入口，也能发现 symbol 和 init/fini 入口。
2. report 能按 analyzer/source 解释入口来源。
3. 不默认线性扫描 `.text`。

中期：

1. eh_frame 后函数 seed 数超过 307。
2. direct-flow 后 seed 继续增长。
3. PLT 调用能解析到外部函数名。

长期：

1. native 发现函数数逐步接近 GhidraScript 旧链路的 478。
2. 每个失败入口都有原因。
3. discovery、decode、lower 时间分开统计。
4. 这套调度能复用到 Bench2 其他 x86-64 shared object。

## 风险

1. 轻量 AutoAnalysis 如果接口太像 Ghidra，容易过早膨胀。第一版必须只保留 libuv 当前需要的状态。
2. analyzer 之间共享 state 后，错误状态会传播。report 里要记录每个 seed 的来源和状态。
3. eh_frame PIE 地址修正容易错，要先做小样例和 libuv 对照。
4. direct-flow decode 可能串函数，必须用 symbol size、eh_frame range、下一个入口限制。
5. 函数发现变多后，lowering 失败也会变多。计划里要区分“发现成功”“decode 成功”“lower 成功”。

## 2026-05-20 实现记录：第一步框架和报告

本次完成“第一步：只做框架和报告”。还没有做 relocation / PLT、eh_frame、递归 decode。

改动：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h:17`
   - 新增 `NativeFunctionConfidence`。
   - `NativeProgramState` 在 `:56` 定义共享状态，保存内存段、section、function seed、source 统计和 note。
   - `NativeAnalyzer` / `NativeAnalysisManager` 在 `:94`、`:108` 定义轻量 analyzer 接口和固定顺序调度器。
2. `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:71`
   - 新增 `ElfLoadAnalyzer`，检查 executable `PT_LOAD`。
   - `:89` 新增 `ElfEntryAnalyzer`，处理 `e_entry`、`DT_INIT`、`DT_FINI`、`DT_INIT_ARRAY`、`DT_PREINIT_ARRAY`、`DT_FINI_ARRAY`。
   - `:143` 新增 `ElfSymbolAnalyzer`，收集 defined `STT_FUNC` 且落在 executable segment 的符号。
   - `:170` 新增 `ReportAnalyzer`，输出 seed 总数、source 统计、confidence 统计、executable sections 和 seed 列表。
   - `:261` 构造 `NativeProgramState`，从 LIEF segment/section 建内存和 section 索引。
3. `external/NotDec-bin2llvm/tools/notdec-native-discover.cpp:34`
   - 新增独立 CLI：`notdec-native-discover <elf-file>`。
4. `external/NotDec-bin2llvm/lib/CMakeLists.txt:40`
   - `notdec-bin2llvm-native` 编进 `NativeAnalysis.cpp`。
5. `external/NotDec-bin2llvm/tools/CMakeLists.txt:91`
   - 新增 `notdec-native-discover` target。
   - `:101` 新增 `/bin/ls` smoke test：`notdec.native_discover.x86_64_smoke`。

验证：

```bash
cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-native-ghidra -DNOTDEC_BIN2LLVM_ENABLE_LIEF=ON -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON -DNOTDEC_BIN2LLVM_GHIDRA_SOURCE_DIR=/sn640/ghidra
cmake --build /tmp/notdec-bin2llvm-native-ghidra --target notdec-native-discover -j4
ctest --test-dir /tmp/notdec-bin2llvm-native-ghidra -R notdec.native_discover.x86_64_smoke --output-on-failure
/tmp/notdec-bin2llvm-native-ghidra/bin/notdec-native-discover /sn640/NotDec-Exp/Bench2/rootfs/usr/lib/x86_64-linux-gnu/libuv.so.1.0.0
```

结果：

1. `/bin/ls` smoke test 通过。
2. libuv report：
   - `function seeds: 311`
   - `elf-symbol: 307`
   - `dt-init: 1`
   - `dt-fini: 1`
   - `dt-init-array: 1`
   - `dt-fini-array: 2`
3. libuv executable sections 能看到 `.init`、`.plt`、`.plt.got`、`.plt.sec`、`.text`、`.fini`。
4. `e_entry = 0` 没有被加入 seed。

性能影响：

这次只改 `external/NotDec-bin2llvm` 的 native 实验工具，没有接入 NotDec 主 pass pipeline，不影响 fortune 当前关注用例。libuv discovery 本身在当前机器上是瞬时完成，输出规模主要来自 311 行 seed 报告。

评分：

1. 实现效果：8/10。第一步验收达成，能解释 libuv 当前入口来源；但还没覆盖 eh_frame 和 direct-flow。
2. 复杂度：4/10。新增状态和 analyzer 接口很小，主要复杂度来自 LIEF ELF 信息整理。
3. 维护成本：4/10。后续 relocation、eh_frame、decode 可以接在同一 state 上，但如果 analyzer 继续增多，应该拆文件。

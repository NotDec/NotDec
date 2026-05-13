# 2026-05-10 Ghidra/C++ 二进制转 LLVM IR 项目计划

## 原始需求

现在需要在external/目录下创建一个，基于Ghidra插件和C++的，二进制代码转LLVM IR项目。它支持两条链路：
- 首先，基于Ghidra插件的架构，通过GhidraScript的方式，增加一个导出IR的命令。它在Java层获取解析的PCode结果，然后调用外部native CLI，把PCode转成LLVM IR。数据优先通过临时文件或stdin/stdout传递，方便调试和分层。即使没有图形界面，也可以使用 -PostScript参数直接调用Ghidra导入某个二进制然后执行插件导出IR。
- 其次，native模块也要支持基于LIEF解析二进制模块，然后直接调用那边转PCode的libsla库，得到PCode并转IR。全程没有任何Ghidra的Java层参与，仅复用libsla库。也可以考虑同时引入libdecomp库，如果里面的代码用的上。
实现要求：
- 对于Ghidra插件部分，读取PCode的方式可以参考：/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/ghidra_pcode_transfer_plugin_notes.md。Java层优先调用外部native CLI，不先做JNI/JNA。native侧通过C/C++接口，复用C++那边的代码，比如解析PCode的部分。
- PCode转LLVM IR的代码部分直接用C++编写，直接使用libSLA/Ghidra的PCode结构，并动态链接LLVM库生成IR。
总的来说，大致会分成这几步，首先是搭建基本的ghidra插件的代码框架，以及C++ native代码框架。其次是完成pcode的Java侧到C++侧的接口，使得C++侧能够访问到PCode。然后是直接使用LLVM C++ API完成PCode转IR的具体逻辑部分。具体的转换逻辑可以参考：https://github.com/toor-de-force/Ghidra-to-LLVM/blob/master/src/xmltollvm.py
- 调用LLVM时，使用动态链接库。使用/sn640/NotDec/llvm-22.1.0.obj这里编译好的版本。

## 当前理解

这个新项目放在 `external/` 下，暂定可以命名为 `NotDec-ghidra2llvm` 或
`NotDec-bin2llvm`。命名本身不重要，关键是不要把它和当前 wasm 前端、llvm2c 后端混在一起。

它不是先做完整反编译器，而是先做“二进制指令/函数到 LLVM IR”的前端。两条链路共享同一个 C++
lifting 核心：

1. GhidraScript 链路负责从 Ghidra Java 层拿 P-Code。
2. LIEF + libsla 链路负责绕过 Java，直接从二进制得到 P-Code。

C++ 侧直接使用libSLA/Ghidra的P-Code结构、地址空间、寄存器、内存、函数边界等信息，然后生成 LLVM IR。
当前不单独定义统一P-Code IR，也不优先考虑复用性。

## 目标

第一目标是做出一个可跑通的最小闭环：

1. 对一个简单 x86-64 或 wasm 外的本地二进制样例，先能导出单指令或短基本块 LLVM IR。
2. GhidraScript 模式可以在无 GUI 下用 `-PostScript` 执行。
3. C++ lifting 核心用 LLVM 22 动态库生成 module/function/basic block/instruction。
4. 两条链路最终走同一个 C++ P-Code lowering 入口。

第二目标是让接口边界稳定：

1. Java 侧只负责拿 Ghidra 的 Program、Instruction、P-Code 和必要元数据。
2. C/C++ 侧只负责复用 Ghidra 已有 packed P-Code 解码、libsla/libdecomp 接口，避免重写复杂解析。
3. C++ 侧直接使用 libSLA/Ghidra P-Code 结构和 LLVM C++ API 生成 LLVM IR。

## 不做的事

第一阶段不追求：

1. 完整还原高级类型。
2. 完整 C 输出。
3. 完整 ABI 参数和返回值恢复。
4. 完整异常处理、间接跳转恢复、switch 恢复。
5. 所有 P-Code opcode 一次性全支持。

这些会把范围拉得太大。先保证整数、寄存器、内存、直接控制流的闭环。

## 总体架构

建议拆成四层。

### Ghidra 插件层

这一层放 Java/GhidraScript 代码，职责是：

1. 打开当前 Program 或由 headless Ghidra 导入的 Program。
2. 选定函数或地址范围。
3. 遍历 Instruction。
4. 用 Ghidra 自己的接口导出 P-Code。
5. 导出语言、地址空间、寄存器、函数入口、基本块边界等元数据。
6. 要求调用方补全必要上下文，比如入口地址、地址空间、架构语言、compiler spec、函数/基本块边界。
7. 调用外部 native CLI，把数据通过临时文件或stdin/stdout传给 native helper。

根据已有笔记，P-Code 热路径优先复用 Ghidra 的 packed 编码，不要一开始就自定义 JSON/XML。
初始化元数据可以走 XML 或项目自己的小 schema。

### Native C++ lifting 层

这一层是主代码，直接用 C++ 实现：

1. 接收 Java 侧的 packed P-Code。
2. 复用 Ghidra C++ 侧 `PackedDecode`、`PcodeOpRaw::decode()` 或 `PcodeEmit::decodeOp()`。
3. 在 libsla 链路里负责调用 Sleigh translator，拿到同样形态的 P-Code。
4. 不再单独定义统一 P-Code IR，直接使用 libSLA/Ghidra 的 P-Code 数据结构。
5. 直接调用 LLVM C++ API 构造 module/function/basic block/instruction。

这里的边界是：Java 只负责导出 P-Code 和上下文；native C++ 负责 P-Code 获取、P-Code 到 LLVM IR、
LLVM module verify 和 IR 输出。

### CLI 与构建层

需要有一个 native CLI 用来独立测试：

1. 读取已导出的 P-Code 文件，输出 LLVM IR。
2. 调 libsla 链路直接处理二进制，输出 LLVM IR。
3. 作为 GhidraScript 的 native helper 被调用。

这能让核心逻辑脱离 Ghidra GUI 调试。

## 两条链路细化

### 链路 A：GhidraScript 到 C++

推荐路线：

1. Java/GhidraScript 负责枚举函数和指令。
2. 指令级 P-Code 用 Ghidra packed 格式导出。
3. native helper 复用 Ghidra C++ 解码代码。
4. 解码后的 P-Code 直接进入 C++ lowering。
5. C++ 生成 LLVM IR。

这个路线的好处是兼容 Ghidra 自己的 P-Code 语义，少造协议。

外层通信可以先简单一些：

1. 如果用进程通信，可以用长度前缀二进制包。
2. 如果用 JNI/JNA，接口会更直接，但调试和构建更麻烦。
3. 第一阶段更建议进程通信，方便 headless Ghidra 和 CLI 共用。

需要保留的元数据：

1. 语言 ID、compiler spec。
2. 地址空间信息，特别是 ram/register/unique/const。
3. 指令地址和 p-code op 顺序。
4. 函数入口、基本块切分、fallthrough 和分支目标。
5. 寄存器名、offset、size。
6. 调用方需要提供的上下文缺失时，工具应明确报错，不靠猜测补齐。

### 链路 B：LIEF + libsla 到 C++

推荐路线：

1. LIEF 只负责解析二进制容器、section、symbol、入口、架构信息。
2. 优先通过 `lifting-bits/sleigh` 引入 Ghidra Sleigh C++ 代码，而不是自己手动拆 Ghidra 源码。
3. `sleigh::sla` 负责按架构规格把机器指令翻译成 P-Code。
4. 如果 `sleigh::decomp` 里的 libdecomp 代码有现成的地址空间、寄存器、p-code decode 代码，再局部复用。
5. 输出 libSLA/Ghidra 原生 P-Code 结构，直接进入 C++ lowering。

`lifting-bits/sleigh` 已经 clone 到 `/sn640/sleigh`。它是 Ghidra Sleigh/decompiler C++ 代码的
CMake 包装项目，当前能看到两个关键 target：

1. `sleigh_sla` / `sleigh::sla`，输出库名是 `sla`，对应计划里的 libsla。
2. `sleigh_decomp` / `sleigh::decomp`，输出库名是 `decomp`，包含 Ghidra 的 `libdecomp.cc/libdecomp.hh`。

后续新项目优先通过 `find_package(sleigh REQUIRED COMPONENTS Support)` 或 `add_subdirectory` 引用它，
链接 `sleigh::sla`、`sleigh::decomp`、`sleigh::support`。它自带的 `sleigh-lift` 示例已经演示了
从 byte string 到 disassembly / p-code 的最小用法，适合作为链路 B 的起点。

### Ghidra 源码版本选择

`lifting-bits/sleigh` 默认会在 CMake 配置时下载并管理 Ghidra 源码。这里改为优先使用它自己管理的
源码版本，不再优先指定 `/sn640/ghidra` 本地 checkout。原因是它的 CMake、patch、source list 和
Ghidra 版本绑定更紧，使用它管理的版本能减少隐藏的不兼容问题。

建议配置方式：

```bash
cmake -S /sn640/sleigh -B /sn640/sleigh/build-notdec \
  -G Ninja \
  -Dsleigh_BUILD_TOOLS=ON \
  -Dsleigh_BUILD_SUPPORT=ON \
  -Dsleigh_BUILD_EXTRATOOLS=ON
```

注意：

1. 先用它默认的 stable 源码版本验证 `sleigh::sla`、`sleigh::decomp`、`sleigh-lift`。
2. 只有确实需要和本地 Java/GhidraScript 源码完全一致时，才临时尝试
   `-Dsleigh_RELEASE_TYPE=HEAD -DFETCHCONTENT_SOURCE_DIR_GHIDRASOURCE=/sn640/ghidra`。
3. 如果临时使用 `/sn640/ghidra` 构建失败，不把它作为主路线，也不要为了构建直接回滚本地 Ghidra 改动。
4. Java/GhidraScript 链路和 `sleigh::sla` 链路的语义对齐，靠导出的上下文和 P-Code 对比测试来保证，
   不靠强行共用同一个 Ghidra checkout。

这里的风险比 GhidraScript 链路大，因为 Ghidra Java 层平时替我们处理了不少 Program
上下文。直接用 libsla 时，要自己处理语言规格、内存映射、指令 bytes、地址空间和寄存器库。
因此这条链路要求用户或调用方补全必要上下文；缺少上下文时宁可失败，也不要静默猜测。

所以顺序上应先做链路 A，再做链路 B。链路 A 跑通后，C++ lowering 的输入和上下文要求才有参照物。

## LLVM IR 生成计划

LLVM 调用必须用 `/sn640/NotDec/llvm-22.1.0.obj` 的动态库。当前方案直接使用 LLVM C++ API，
不再额外做单独的 LLVM 封装库。

第一批只需要覆盖：

1. `LLVMContext`、`Module`、`IRBuilder` 创建和销毁。
2. integer、void、pointer、array、function type。
3. function、basic block、global 创建。
4. 常量整数、undef/poison 需要按 LLVM 22 语义确认。
5. load/store、binary op、icmp、cast、gep、br、condbr、ret、call。
6. module verify 和 IR 打印。

第二批再考虑：

1. attribute。
2. metadata。
3. debug info。
4. diagnostic。
5. pass pipeline。
6. object emission。

先不要把 pass pipeline、优化、LTO、诊断系统一起搬进第一阶段。

## 构建系统计划

这个新项目同时涉及 Java/GhidraScript、C++、LLVM 22、`lifting-bits/sleigh` 和后续 LIEF。
构建系统要先保守，不要一开始把所有东西绑进 NotDec 顶层。

建议分三层：

### 第一层：独立 external 项目

新项目先作为 `external/NotDec-bin2llvm` 之类的独立目录存在。它有自己的 CMake 配置，
先不接入 NotDec 顶层 `all` target。

第一阶段只要求：

1. 能单独配置和构建 native helper。
2. 能单独构建 native CLI。
3. 能找到 `/sn640/NotDec/llvm-22.1.0.obj` 的 LLVM 动态库。
4. 能找到或构建 `/sn640/sleigh`。

这样做的好处是失败面小。等 CLI 和 helper 跑通后，再考虑接入 NotDec 顶层。

### 第二层：sleigh 作为预构建依赖

`/sn640/sleigh` 先单独构建和安装到固定目录，例如 `/sn640/sleigh/install-notdec`。新项目通过
`find_package(sleigh REQUIRED COMPONENTS Support)` 引用安装结果。

不建议第一版在新项目里 `add_subdirectory(/sn640/sleigh)`，原因是：

1. `lifting-bits/sleigh` 配置时可能下载 Ghidra 源码，耗时且变量多。
2. 它的 CMake 选项和 NotDec 顶层选项容易互相影响。
3. 预构建安装后，依赖边界更清楚，问题更容易定位。

建议先验证这些 target：

1. `sleigh::sla`
2. `sleigh::decomp`
3. `sleigh::support`
4. `sleigh-lift`

如果后续需要 CI 或完全可复现构建，再把 `/sn640/sleigh` 固定到 submodule 或明确 commit。

### 第三层：Java 与 native helper 的连接

Java/GhidraScript 和 native helper 先走进程通信，不做 JNI/JNA：

1. Java/GhidraScript 负责导出 packed P-Code 和必要上下文。
2. Java/GhidraScript 调用外部 native CLI，数据通过临时文件或stdin/stdout传递。
3. native helper 接收输入，复用 Ghidra C++ packed decode 或 libSLA 直接得到 P-Code。
4. native helper 直接生成 LLVM IR 文件。

这样可以避免把 Ghidra Gradle/Java 环境和 C++/LLVM/sleigh 构建绑在一起。等脚本和 native helper
稳定后，再考虑是否需要 JNI/JNA。

### GhidraScript 构建

GhidraScript 先不放进 CMake 编译链。它按脚本文件管理：

1. Java/GhidraScript 放在项目 `ghidra_scripts/`。
2. native helper 路径通过脚本参数或环境变量传入。
3. headless Ghidra 测试命令单独写到文档或测试脚本。

等脚本稳定后，再考虑 Gradle/Ghidra extension 打包。

### LIEF 接入顺序

LIEF 不是阶段 1 的构建依赖。先用 `sleigh-lift` 风格的 byte string 输入跑通 libsla 到 P-Code，
再引入 LIEF 解析真实二进制。这样可以把“容器解析问题”和“Sleigh lifting 问题”分开。

## P-Code 到 LLVM IR 的转换计划

可以参考 `Ghidra-to-LLVM` 的 opcode 覆盖方式，但不要照搬它的 XML 和 llvmlite 结构。它更适合作为
“需要支持哪些 P-Code opcode”的清单。

第一阶段优先支持：

1. `COPY`
2. `LOAD`
3. `STORE`
4. `INT_ADD`、`INT_SUB`、`INT_MULT`
5. `INT_AND`、`INT_OR`、`INT_XOR`
6. `INT_LEFT`、`INT_RIGHT`、`INT_SRIGHT`
7. `INT_EQUAL`、`INT_NOTEQUAL`、`INT_LESS`、`INT_SLESS`
8. `INT_ZEXT`、`INT_SEXT`
9. `SUBPIECE`、`PIECE` 的简单情况
10. `BRANCH`、`CBRANCH`、`CALL`、`RETURN`

暂缓支持：

1. floating point opcode。
2. `MULTIEQUAL`、`INDIRECT` 这类需要 SSA/数据流语义的 opcode。
3. `CALLIND`、`BRANCHIND` 的精确目标恢复。
4. `USERDEFINED`、`CALLOTHER`。

内存模型先用简单模型：

1. ram 统一映射到一个 byte pointer 或全局 byte array。
2. register 映射到 per-function alloca 或 module global，先以简单正确为主。
3. unique 映射成 SSA 临时值表。
4. const 直接生成 LLVM constant。

后续如果要接 NotDec 的类型恢复，再考虑把 load/store 和 pointer type 做得更细。

## 阶段计划

### 阶段 0：确认边界

目标：

1. 确认新项目目录名。
2. 确认 `lifting-bits/sleigh` 使用它管理的 Ghidra 源码版本能构建。
3. 确认 LLVM 22 动态库、头文件、链接方式。
4. 确认第一批样例二进制。
5. 确认 `/sn640/sleigh` 的安装目录和 `find_package(sleigh)` 引用方式。

判断标准：

1. 能说明每一层输入输出是什么。
2. 不依赖 GUI 的运行路径清楚。
3. `sleigh::sla`、`sleigh::decomp`、`sleigh::support` 的引用方式明确。

### 阶段 1：项目骨架

目标：

1. 在 `external/` 下建独立项目。
2. 有 C++ native helper、GhidraScript 目录。
3. 能链接 LLVM 22 动态库并输出一个空 LLVM module。
4. 能通过 `find_package(sleigh)` 链接预构建的 `/sn640/sleigh` 安装结果。

判断标准：

1. native CLI 能生成最小 IR。
2. 不影响 NotDec 顶层现有构建。
3. `sleigh-lift` 风格的 byte string 到 P-Code 测试可运行。

### 阶段 2：GhidraScript P-Code 导出闭环

目标：

1. headless Ghidra 能导入二进制并执行脚本。
2. 脚本能导出一条指令或一个函数的 packed P-Code。
3. native helper 能解码并打印 opcode、varnode、地址空间。

判断标准：

1. Java 侧看到的 P-Code 和 native 解码结果一致。
2. 同一个样例可重复运行。
3. 缺少必要上下文时有明确报错。

### 阶段 3：C++ LLVM IR 生成

目标：

1. C++ native helper 可以创建 module/function/basic block。
2. 可以生成整数运算、load/store、branch、return。
3. 可以 verify module 并打印 IR。

判断标准：

1. 不靠拼字符串生成 IR。
2. 关键 LLVM 对象生命周期清楚。
3. 动态链接只走 LLVM 22。

### 阶段 4：P-Code lowering 最小闭环

目标：

1. 把一小段无复杂控制流的 P-Code 转成 LLVM IR。
2. 支持寄存器、unique、constant、ram 的基本访问。
3. 支持直接分支和条件分支。

判断标准：

1. 输出 IR 通过 LLVM verifier。
2. 简单函数的基本块结构和 Ghidra P-Code 控制流一致。

### 阶段 5：LIEF + libsla 链路

目标：

1. LIEF 解析同一个样例二进制。
2. 通过 `lifting-bits/sleigh` 的 `sleigh::sla` 生成 P-Code。
3. 复用阶段 4 的 C++ lowering 输出 IR。

判断标准：

1. 对同一指令，libsla 链路和 GhidraScript 链路的 P-Code 语义能对齐。
2. 不需要 Ghidra Java 层也能跑通。
3. `lifting-bits/sleigh` 自己管理的 Ghidra 源码版本可重复构建。

### 阶段 6：扩大 opcode 和样例

目标：

1. 增加常见整数 opcode。
2. 增加 call/return 的基本表示。
3. 增加更多架构或编译器样例。

判断标准：

1. 每次扩展都有小样例。
2. unsupported opcode 有明确报错和统计。

## 主要风险

1. LLVM 22 opaque pointer 下，P-Code 的 byte/word 语义需要自己保留显式宽度。
2. P-Code 的 `MULTIEQUAL`、`INDIRECT`、`CALLIND` 不能简单翻译成单条 LLVM 指令。

## 当前取舍

1. 先做 GhidraScript 链路，因为它最容易拿到可信 P-Code。
2. 不单独定义统一 P-Code IR，直接使用 libSLA/Ghidra 的 P-Code 结构。
3. C++ 是主链路，负责 P-Code 获取、P-Code lowering 和 LLVM IR 生成。
4. 直接使用 LLVM C++ API，不再做单独的 LLVM 封装库。
5. P-Code opcode 按样例驱动补，不一次性铺满。
6. 链路 B 优先引用 `lifting-bits/sleigh`，并优先使用它自己管理的 Ghidra 源码版本。
7. LIEF 后置，先用 byte string 验证 `sleigh::sla` 到 LLVM IR 的链路。

## 验收方式

计划完成后，可以这样验证：

1. native CLI 输出空 module。
2. native CLI 输出一个简单 P-Code 样例对应的 LLVM IR。
3. Ghidra headless `-PostScript` 跑一个二进制并导出 P-Code。
4. Java 导出的 P-Code 能被 native helper 解码。
5. 解码结果能进入 C++ lowering 并输出 verifier 通过的 IR。
6. LIEF + libsla 对同一二进制跑通一条指令或一个函数。
7. `/sn640/sleigh` 使用它自己管理的 Ghidra 源码版本时，能构建出 `sleigh::sla` 和 `sleigh::decomp` 对应库。
8. 两条链路都能直接使用 libSLA/Ghidra P-Code 结构进入 lowering。

## 实现记录（2026-05-13）

这次先完成阶段 1 的最小骨架，目标只到“独立子项目能链接 LLVM 22，并输出 verifier 通过的最小 IR”，还没开始接 GhidraScript、packed p-code，也还没验证 `find_package(sleigh)` 的真实安装链路。

### 已完成

1. 新建独立子项目 `external/NotDec-bin2llvm/`，没有接进顶层默认构建。
2. 新建本地 LLVM 查找逻辑，默认指向仓库内 `llvm-22.1.0.obj`，并强制走 shared `libLLVM.so`。
3. 新建最小库目标 `notdec-bin2llvm-core` 和 CLI `notdec-bin2llvm`。
4. CLI 当前能生成一个最小 module：
   `define void @notdec_stub() { ret void }`
5. 预留 `ghidra_scripts/` 目录，但故意不放进构建链。

### 本次修改

1. `external/NotDec-bin2llvm/CMakeLists.txt:1-21`
   - 新建独立项目入口。
   - 增加 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH` 开关，默认不启用。
2. `external/NotDec-bin2llvm/cmake/FindLLVM.cmake:1-90`
   - 新建本地 LLVM 22 查找逻辑。
   - 新建接口库 `notdec_bin2llvm_llvm_deps`，统一挂 shared LLVM 依赖。
3. `external/NotDec-bin2llvm/include/notdec-bin2llvm/ModuleBuilder.h:1-23`
   - 新建 `notdec::bin2llvm::BuildConfig`。
   - 新建 `notdec::bin2llvm::buildDemoModule(...)` 声明。
4. `external/NotDec-bin2llvm/lib/ModuleBuilder.cpp:1-29`
   - 实现 `notdec::bin2llvm::buildDemoModule(...)`。
   - 只创建 `module/function/basic block/ret void`，不提前引入 p-code 抽象。
5. `external/NotDec-bin2llvm/lib/CMakeLists.txt:1-13`
   - 新建 `notdec-bin2llvm-core` 静态库目标。
6. `external/NotDec-bin2llvm/tools/notdec-bin2llvm.cpp:1-45`
   - 实现 CLI `main(...)`。
   - 实现局部辅助函数 `writeModule(...)`。
   - 先做参数检查、`verifyModule` 和 `.ll` 输出。
7. `external/NotDec-bin2llvm/tools/CMakeLists.txt:1-8`
   - 新建 CLI target `notdec-bin2llvm`。
8. `external/NotDec-bin2llvm/ghidra_scripts/README.md:1-9`
   - 只保留脚本目录约定和后续待补项。

### 验证

1. 配置：
   `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-build -G Ninja`
2. 构建：
   `cmake --build /tmp/notdec-bin2llvm-build --target notdec-bin2llvm -j4`
3. 运行：
   `/tmp/notdec-bin2llvm-build/bin/notdec-bin2llvm /tmp/notdec-bin2llvm-demo.ll`
4. IR 装配验证：
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-demo.ll -o /tmp/notdec-bin2llvm-demo.bc`

结果：

1. 四条命令都已跑通。
2. 产物 `/tmp/notdec-bin2llvm-demo.ll` 可被 `llvm-as` 接受。
3. 当前输出 IR 为最小 stub，说明 LLVM 22 动态链接、库目标、CLI 入口都已经通了。

### 这一步没做

1. 没接 `sleigh::sla`、`sleigh::decomp`。原因很直接：`/sn640/sleigh` 当前还没看到现成安装结果，这一步应该单独验证。
2. 没接 GhidraScript/headless `-PostScript`。
3. 没开始做 packed p-code 解码。
4. 没做 LIEF。

### 下一步

1. 先把 `/sn640/sleigh` 单独 build + install，确认 `sleighConfig.cmake`、`sleigh::sla`、`sleigh::decomp`、`sleigh::support` 的真实引用方式。
2. 在 `external/NotDec-bin2llvm` 里加一个可选的 `sleigh-lift` 风格最小实验，把 byte string 转成 p-code 并打印。
3. 等 native 侧 p-code 输入通了，再开始补 `ghidra_scripts/` 的 headless 导出闭环。

### 仓库结构更新（2026-05-13）

后续决定把 `external/NotDec-bin2llvm` 单独做成 git 仓库，并注册为主项目子模块，这样更符合现有 `external/` 目录的组织方式，也避免主仓库直接跟踪子项目内部文件。

1. `external/NotDec-bin2llvm`
   - 执行 `git init -b main`
   - 提交首个骨架 commit `3d2da89`，提交信息 `Initial project skeleton`
   - 配置远端 `git@github.com:am009/NotDec-bin2llvm.git`
   - 执行 `git push -u origin main`
2. `.gitmodules:10-13`
   - 新增子模块 `external/NotDec-bin2llvm`
   - URL 为 `git@github.com:am009/NotDec-bin2llvm.git`
   - 跟踪分支设为 `main`
3. `external/NotDec-bin2llvm/.git`
   - 执行 `git submodule absorbgitdirs external/NotDec-bin2llvm`
   - 现在已经转成 `gitdir: ../../.git/modules/external/NotDec-bin2llvm`

### 评价

1. 实现效果：8/10。最小骨架已经能独立配置、构建、出 IR，达到了阶段 1 的起点目标。
2. 理解成本：8/10。目录和入口很少，没有先引入假的中间层。
3. 维护成本：8/10。现在主要风险不在代码，而在后续 `sleigh` 安装和 Ghidra Java/native 边界。
4. 更好的方案：如果 `/sn640/sleigh` 很快能稳定安装，下一版可以直接把 `sleigh` 的最小 p-code 打印器一起落进这个骨架，比继续空转 GhidraScript 更值。

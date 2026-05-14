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

2026-05-14 更新：这里的“再做链路 B”不再指马上接 C++ 原生 libdecomp
`Funcdata`。当前先继续路线 A，也就是 Ghidra headless / Java decompiler 导出
`HighFunction` 后 P-Code，再由 native CLI 校验和 lowering。`sleigh::decomp`
仍可继续作为 raw P-Code、opcode 名称和后续 native 收敛的参考，但原生 libdecomp
环境暂缓，等导出 schema 和 LLVM lowering 覆盖面稳定后再评估。

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

1. 还没把 `sleigh::sla`、`sleigh::decomp` 真正连到 `notdec-bin2llvm` 的库或 CLI 上；这次只先把依赖获取和 target 注入方式定下来。
2. 没接 GhidraScript/headless `-PostScript`。
3. 没开始做 packed p-code 解码。
4. 没做 LIEF。

### 下一步

1. 在 `external/NotDec-bin2llvm` 里加一个最小 `bytes -> p-code 打印` CLI，真正链接 `sleigh::sla`、`sleigh::decomp`、`sleigh::support`。
2. 先复用固定 commit 的 `FetchContent` 路线；本地调试时用 `NOTDEC_BIN2LLVM_SLEIGH_SOURCE_DIR=/sn640/sleigh` 覆盖，避免每次都走网络。
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

### 依赖接入更新（2026-05-13）

这一步把 `sleigh` 的接入策略从“将来单独 install 后再 `find_package`”改成了“默认固定 commit 的 `FetchContent_MakeAvailable`，本地源码覆盖可选”。原因很简单：当前更需要的是把 `sleigh` targets 直接注入当前构建，先跑通 native 侧 p-code 输入，而不是先搭一个 superbuild。

1. `external/NotDec-bin2llvm/CMakeLists.txt:1-56`
   - `cmake_minimum_required` 从 `3.13.4` 提到 `3.14`，因为 `FetchContent_MakeAvailable` 需要这个下限。
   - 保留 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH` 开关，默认仍然关闭，避免最小骨架构建时无条件拉大依赖。
   - 新增 `NOTDEC_BIN2LLVM_SLEIGH_GIT_REPOSITORY`，当前固定为 `https://github.com/lifting-bits/sleigh.git`。
   - 新增 `NOTDEC_BIN2LLVM_SLEIGH_GIT_TAG`，当前固定 commit 为 `c1aec71e4090a57daea1544379c63537e5e1add7`。
   - 新增 `NOTDEC_BIN2LLVM_SLEIGH_SOURCE_DIR`，用于本地源码覆盖。
   - `NOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON` 时，改为 `include(FetchContent)` + `FetchContent_Declare(...)` + `FetchContent_MakeAvailable(sleigh)`。
   - 这一步先显式关掉 `sleigh_BUILD_TOOLS`、`sleigh_BUILD_SLEIGHSPECS`、`sleigh_BUILD_EXTRATOOLS`，只保留 `sleigh_BUILD_SUPPORT=ON`，避免把不需要的工具链一并打开。

2. 验证命令
   - 关闭 sleigh：
     `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-build-off -G Ninja`
   - 开启 sleigh，并用本地源码覆盖：
     `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-build-sleigh -G Ninja -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON -DNOTDEC_BIN2LLVM_SLEIGH_SOURCE_DIR=/sn640/sleigh`
   - 两个配置分别构建当前 CLI：
     `cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-bin2llvm -j4`
     `cmake --build /tmp/notdec-bin2llvm-build-sleigh --target notdec-bin2llvm -j4`

3. 验证结果
   - `OFF` 路径保持原样，最小骨架继续能独立配置和构建。
   - `ON + 本地源码覆盖` 路径可以成功配置，并把 `sleigh` 及其 Ghidra 源码依赖拉进 build tree。
   - 这里也顺手确认了一个事实：即使本地覆盖 `/sn640/sleigh` 源码，`sleigh` 自己仍会继续下载并 patch 它依赖的 Ghidra 源码；这属于 `sleigh` 项目自身的构建行为，不是 `NotDec-bin2llvm` 这边接法的问题。

### Native p-code CLI 更新（2026-05-13）

这一步开始真正消费 `sleigh` target，不再只停在“依赖能接进来”。目标很窄：先做一个最小 native CLI，输入 `.sla + hex bytes`，直接打印 p-code。

1. `external/NotDec-bin2llvm/tools/CMakeLists.txt`
   - 新增 `notdec-sleigh-pcode` target。
   - 只在 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON` 时构建。
   - 直接链接 `sleigh::sla`、`sleigh::decomp`、`sleigh::support`。

2. `external/NotDec-bin2llvm/tools/notdec-sleigh-pcode.cpp`
   - 新增最小 CLI。
   - 复用了 `sleigh-lift` 的核心思路，但只保留当前需要的部分：
     - `InMemoryLoadImage`
     - hex bytes 解析
     - Sleigh/pspec 初始化
     - `PcodeEmit` 打印
   - 当前支持参数：
     - 位置参数：`<sla-file> <hex-bytes>`
     - 可选参数：`-a address`、`-p root-sla-dir`、`-s pspec-file`

3. `external/NotDec-bin2llvm/CMakeLists.txt`
   - 为 `sleigh_sla`、`sleigh_decomp`、`sleigh_support` 显式加回 `-frtti`。
   - 原因是 `FindLLVM.cmake` 当前会全局加 `-fno-rtti`，而 `sleigh` 上游内部有 `dynamic_cast`，不加回 RTTI 会在 `slghpattern.cc`、`slghpatexpress.cc` 等文件直接编译失败。
   - 这一步只修正 `sleigh` 子目标，不改当前主项目的 LLVM 风格选择。

4. 验证命令
   - 构建：
     `cmake --build /tmp/notdec-bin2llvm-build-sleigh --target notdec-sleigh-pcode -j4`
   - 运行：
     `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-pcode /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 4881ecc00f0000 -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`

5. 验证结果
   - `notdec-sleigh-pcode` 已成功构建。
   - 对 x86-64 字节串 `4881ecc00f0000`，已经能直接打印出 p-code。
   - 输出里包含：
     - `INT_SUB`
     - `INT_LESS`
     - `INT_SBORROW`
     - `INT_EQUAL`
   - 说明当前链路已经达到：
     `hex bytes -> sleigh/libsla -> p-code print`

6. 当前判断
   - 先不切 shared `libsla/libdecomp`。上游 `BUILD_SHARED_LIBS` 注释已经明确写了 `Untested and not supported`，现在更值的是先把链路跑通。
   - 下一步应当把这个 CLI 产出的 p-code 输入收敛成我们自己的数据访问接口，再开始做 `p-code -> LLVM IR` 的最小 lowering。

### 评价

1. 实现效果：8/10。最小骨架已经能独立配置、构建、出 IR，达到了阶段 1 的起点目标。
2. 理解成本：8/10。目录和入口很少，没有先引入假的中间层。
3. 维护成本：8/10。现在主要风险不在代码，而在后续 `sleigh` 安装和 Ghidra Java/native 边界。
4. 更好的方案：如果 `/sn640/sleigh` 很快能稳定安装，下一版可以直接把 `sleigh` 的最小 p-code 打印器一起落进这个骨架，比继续空转 GhidraScript 更值。

## 实现记录（2026-05-13，native P-Code 到 LLVM IR 最小闭环）

这次继续推进上一节的“先做 native 侧 lowering”判断，没有先接 GhidraScript。当前目标是让
`hex bytes -> sleigh/libsla -> P-Code -> LLVM IR` 跑通，范围只覆盖整数和临时/寄存器值，不碰内存、
真实函数边界和控制流。

### 已完成

1. 新增项目自己的轻量 P-Code 数据视图：
   - `external/NotDec-bin2llvm/include/notdec-bin2llvm/Pcode.h:10-55`
     - 新增 `PcodeOpcode`、`VarnodeView`、`PcodeOpView`、`PcodeProgram`。
   - `external/NotDec-bin2llvm/lib/Pcode.cpp:5-51`
     - 新增 `pcodeOpcodeName(...)`。
2. 新增最小 P-Code lowering：
   - `external/NotDec-bin2llvm/include/notdec-bin2llvm/PcodeToLLVM.h:15-22`
     - 新增 `PcodeLoweringConfig` 和 `buildPcodeModule(...)`。
   - `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:22-329`
     - 新增 `PcodeLowerer`。
     - 当前支持 `COPY`、整数算术/位运算、比较、移位、`INT_ZEXT`、`INT_SEXT`、`PIECE`、
       `SUBPIECE`、`POPCOUNT`、`INT_SBORROW`。
     - unknown register/unique 读先用 `freeze poison`，避免把运算直接常量折叠没。
     - 写 register/unique 只维护本函数内 SSA 值表，还不是 ABI/状态建模。
3. 把 `notdec-sleigh-pcode` 的 Sleigh 输入逻辑抽成共享代码：
   - `external/NotDec-bin2llvm/tools/SleighBytes.h:13-27`
     - 新增 `SleighBytesOptions`、`collectSleighPcode(...)`、`printPcodeProgram(...)`。
   - `external/NotDec-bin2llvm/tools/SleighBytes.cpp:16-348`
     - 迁移原来的内存镜像、`.sla/.pspec` 查找、hex bytes 解析、Sleigh 初始化。
     - 新增 `PcodeCollector`，把 `ghidra::PcodeEmit` 回调转成 `PcodeProgram`。
   - `external/NotDec-bin2llvm/tools/notdec-sleigh-pcode.cpp:15-30`
     - 改成只解析参数、收集 P-Code、打印 `PcodeProgram`。
4. 新增 native bytes 到 LLVM IR CLI：
   - `external/NotDec-bin2llvm/tools/notdec-sleigh-llvm.cpp:16-117`
     - 参数格式：`<sla-file> <hex-bytes> -o <output.ll> [-a address] [-p root-sla-dir] [-s pspec-file]`。
     - 调用 `collectSleighPcode(...)`、`buildPcodeModule(...)`、`verifyModule(...)`，最后写 `.ll`。
5. 更新 CMake：
   - `external/NotDec-bin2llvm/lib/CMakeLists.txt:1-4`
     - `notdec-bin2llvm-core` 加入 `Pcode.cpp`、`PcodeToLLVM.cpp`。
   - `external/NotDec-bin2llvm/tools/CMakeLists.txt:10-44`
     - 新增 `notdec-bin2llvm-sleigh-bytes` 静态库。
     - 新增 `notdec-sleigh-llvm` target。
     - `notdec-sleigh-pcode` 改为复用 `notdec-bin2llvm-sleigh-bytes`。

### 验证

1. 构建 native/sleigh 两个 CLI：
   `cmake --build /tmp/notdec-bin2llvm-build-sleigh --target notdec-sleigh-pcode notdec-sleigh-llvm -j4`
2. 确认无 sleigh 的旧最小 CLI 仍可构建：
   `cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-bin2llvm -j4`
3. 旧 P-Code 打印路径：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-pcode /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 4881ecc00f0000 -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
4. 新 lowering 路径：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 4881ecc00f0000 -o /tmp/notdec-sleigh-sub.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
5. LLVM IR 装配验证：
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-sub.ll -o /tmp/notdec-sleigh-sub.bc`
6. 旧空 module 路径：
   `/tmp/notdec-bin2llvm-build-off/bin/notdec-bin2llvm /tmp/notdec-bin2llvm-demo-check.ll`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-demo-check.ll -o /tmp/notdec-bin2llvm-demo-check.bc`

结果：

1. 上面命令都已通过。
2. `4881ecc00f0000` 仍能打印原来的 9 条 P-Code。
3. 新 CLI 输出的 `/tmp/notdec-sleigh-sub.ll` 可被 LLVM 22 `llvm-as` 接受。
4. 输出 IR 里能看到 `icmp ult`、`llvm.ssub.with.overflow.i64`、`sub`、`icmp slt`、`and`、
   `llvm.ctpop.i64` 等，对应当前样例里的 flag 计算和 `rsp -= 0xfc0`。

### 当前限制

1. `LOAD`、`STORE`、`BRANCH`、`CBRANCH`、`CALL`、`RETURN` 还没降。
2. register 还不是显式状态对象，只是按 varnode key 维护 SSA 值。
3. unknown 输入用 `freeze poison`，只适合现在的结构验证，不代表真实入口寄存器建模。
4. 这次只改 `external/NotDec-bin2llvm`，没有改 NotDec 主 pass pipeline；因此没有跑 fortune 当前关注用例计时。

### 评价

1. 实现效果：8/10。已经有第一条 `bytes -> P-Code -> LLVM IR` 闭环，且 IR 通过 verifier/assembler。
2. 理解成本：7/10。新增了必要的数据视图和 lowering，但没有引入更大的抽象层。
3. 维护成本：7/10。后续主要成本是逐步补 opcode 和把 register/memory 状态做实。
4. 更好的方案：下一步优先补 `LOAD/STORE` 的简单 byte-array 内存模型，随后再做基本块和直接分支；现在不应急着接 LIEF。

## 实现记录（2026-05-13，LOAD/STORE 最小内存模型）

这次继续沿着 native lowering 走，先补 `LOAD/STORE`，不接 GhidraScript，不接 LIEF。当前只做结构正确的
IR：把 P-Code 的 memory 访问降到一个外部 byte array，不试图恢复真实段、栈、ABI 或初始内存内容。

### 已完成

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/Pcode.h:10-13`
   - `PcodeOpcode` 新增 `Load`、`Store`。
   - 更新 `PcodeOpView` 注释，说明 output 为空现在主要对应 `STORE` 和控制流类 op。
2. `external/NotDec-bin2llvm/lib/Pcode.cpp:5-13`
   - `pcodeOpcodeName(...)` 新增 `LOAD`、`STORE` 名称。
3. `external/NotDec-bin2llvm/tools/SleighBytes.cpp:98-105`
   - `convertOpcode(...)` 新增 `ghidra::CPUI_LOAD`、`ghidra::CPUI_STORE` 到项目 opcode 的映射。
4. `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:262-319`
   - 新增 `memoryGlobal()`，第一次遇到内存访问时创建
     `@notdec_ram = external global [1048576 x i8]`。
   - 新增 `memoryPointer(...)`，把 P-Code 地址值转成 `getelementptr`。
   - 新增 `requireConstSpaceSelector(...)`，先要求 `LOAD/STORE` 第一个输入是 `const`。
   - 新增 `lowerLoad(...)`，生成 unaligned `load`。
   - 新增 `lowerStore(...)`，生成 unaligned `store`。
   - `lowerOp(...)` 接入 `Load`、`Store`。

### 验证

1. 构建：
   `cmake --build /tmp/notdec-bin2llvm-build-sleigh --target notdec-sleigh-pcode notdec-sleigh-llvm -j4`
2. 原整数样例仍可降：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 4881ecc00f0000 -o /tmp/notdec-sleigh-sub.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-sub.ll -o /tmp/notdec-sleigh-sub.bc`
3. LOAD 样例：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 488b0424 -o /tmp/notdec-sleigh-load.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-load.ll -o /tmp/notdec-sleigh-load.bc`
4. STORE 样例：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 48890424 -o /tmp/notdec-sleigh-store.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-store.ll -o /tmp/notdec-sleigh-store.bc`
5. 旧 P-Code 打印路径仍可用：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-pcode /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 488b0424 -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
6. 无 sleigh 的旧最小 CLI 仍可构建和装配：
   `cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-bin2llvm -j4`
   `/tmp/notdec-bin2llvm-build-off/bin/notdec-bin2llvm /tmp/notdec-bin2llvm-demo-check.ll`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-demo-check.ll -o /tmp/notdec-bin2llvm-demo-check.bc`

结果：

1. 上面命令都已通过。
2. `488b0424` 输出包含 `@notdec_ram`、`getelementptr`、`load i64`。
3. `48890424` 输出包含 `@notdec_ram`、`getelementptr`、`store i64`。

### 当前限制

1. P-Code 的 address-space selector 只检查是否为 `const`，还没映射到具体 Ghidra address space。
2. `@notdec_ram` 是固定 1MiB 外部数组，地址越界、真实 section、栈和全局变量都还没建模。
3. load/store 直接按 LLVM 整数 load/store 表示，没有显式处理端序。
4. 这次仍只改 `external/NotDec-bin2llvm`，没有改 NotDec 主 pass pipeline；因此没有跑 fortune 当前关注用例计时。

### 评价

1. 实现效果：7/10。内存访问已经能进 IR，并通过 LLVM verifier/assembler。
2. 理解成本：7/10。只加了一个外部 byte array 模型，容易替换。
3. 维护成本：7/10。后续要把 address-space、section、栈和初始内存补实。
4. 更好的方案：下一步应补基本块和直接分支，否则无法表达多指令控制流；真实内存布局等 LIEF 接入时再细化。

## 实现记录（2026-05-13，基本块和直接分支）

这次继续补控制流，只做 raw P-Code 里最直接的一层：`BRANCH`、`CBRANCH`、`RETURN`。目标是让短字节串里的
直接跳转能生成 LLVM basic block 和 terminator。间接跳转、call、跨块 SSA/PHI 先不做。

### 已完成

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/Pcode.h:10-56`
   - `PcodeOpcode` 新增 `Branch`、`CBranch`、`Return`、`BoolNegate`。
   - `PcodeOpView` 新增 `Address` 字段，用来记录每条 P-Code 所属指令地址。
2. `external/NotDec-bin2llvm/lib/Pcode.cpp:5-63`
   - `pcodeOpcodeName(...)` 新增 `BRANCH`、`CBRANCH`、`RETURN`、`BOOL_NEGATE`。
3. `external/NotDec-bin2llvm/tools/SleighBytes.cpp:98-175`
   - `convertOpcode(...)` 新增 `ghidra::CPUI_BRANCH`、`ghidra::CPUI_CBRANCH`、
     `ghidra::CPUI_RETURN`、`ghidra::CPUI_BOOL_NEGATE`。
   - `PcodeCollector::dump(...)` 保存 `address.getOffset()` 到 `PcodeOpView::Address`。
4. `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:47-249`
   - `PcodeLowerer` 新增 basic block 构建流程。
   - 按入口、直接分支目标、terminator 后继 op 切块。
   - `BRANCH` 降成 `br label`。
   - `CBRANCH` 降成 `condbr`，true target 是 P-Code 的 direct ram target，false target 是 fallthrough。
   - `RETURN` 暂时降成 `ret void`，不使用返回地址 varnode。
   - 目标地址不在输入 bytes 内时，创建外部 target block，并暂时 `ret void`。
5. `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:199-205,455-463`
   - 新增 `asCondition(...)`。
   - 新增 `lowerBoolNegate(...)`，支持 x86 `jne` 这类先生成 `BOOL_NEGATE` 再 `CBRANCH` 的样例。
6. `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:66-68`
   - 每个 basic block 开始时清空当前 SSA 值表。
   - 原因是当前还没有 PHI 和显式 register state，跨块直接复用 LLVM instruction 会违反 dominance。

### 验证

1. 构建：
   `cmake --build /tmp/notdec-bin2llvm-build-sleigh --target notdec-sleigh-pcode notdec-sleigh-llvm -j4`
2. 条件分支 `je + ret + ret`：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 7401c3c3 -o /tmp/notdec-sleigh-cbranch.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-cbranch.ll -o /tmp/notdec-sleigh-cbranch.bc`
3. 直接分支 `jmp + ret + ret`：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla eb01c3c3 -o /tmp/notdec-sleigh-branch.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-branch.ll -o /tmp/notdec-sleigh-branch.bc`
4. `jne + ret + ret`，覆盖 `BOOL_NEGATE`：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-llvm /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 7501c3c3 -o /tmp/notdec-sleigh-jne.ll -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-jne.ll -o /tmp/notdec-sleigh-jne.bc`
5. 回归之前样例：
   - `4881ecc00f0000` 整数样例
   - `488b0424` LOAD 样例
   - `48890424` STORE 样例
   - `c3` RETURN 样例
   都已用 `notdec-sleigh-llvm` 输出 `.ll` 并通过 `llvm-as`。
6. 旧 P-Code 打印路径仍可用：
   `/tmp/notdec-bin2llvm-build-sleigh/bin/notdec-sleigh-pcode /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.sla 7401c3c3 -s /sn640/ghidra/build/dist/ghidra_11.3.2_DEV/Ghidra/Processors/x86/data/languages/x86-64.pspec`
7. 无 sleigh 的旧最小 CLI 仍可构建和装配：
   `cmake --build /tmp/notdec-bin2llvm-build-off --target notdec-bin2llvm -j4`
   `/tmp/notdec-bin2llvm-build-off/bin/notdec-bin2llvm /tmp/notdec-bin2llvm-demo-check.ll`
   `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-demo-check.ll -o /tmp/notdec-bin2llvm-demo-check.bc`

结果：

1. 上面命令都已通过。
2. `7401c3c3` 输出包含 `br i1 ... label %bb_3, label %bb_2`。
3. `eb01c3c3` 输出包含直接 `br label %bb_3`。
4. `7501c3c3` 输出包含 `xor i1 ..., true` 和 `condbr`。

### 当前限制

1. 跨 basic block 的 register/unique 值没有 PHI，也没有显式状态对象；每个 block 开始都会重新从 unknown 输入读值。
2. `RETURN` 现在只生成 `ret void`，还没表达返回地址、返回值或调用约定。
3. 只支持 direct `ram` target；`BRANCHIND`、`CALL`、`CALLIND` 都还没做。
4. unreachable block 仍会保留在 IR 里，比如直接跳过的 fallthrough block。
5. 这次仍只改 `external/NotDec-bin2llvm`，没有改 NotDec 主 pass pipeline；因此没有跑 fortune 当前关注用例计时。

### 评价

1. 实现效果：7/10。直接控制流已经能进 LLVM IR，并通过 verifier/assembler。
2. 理解成本：7/10。basic block 切分逻辑还小，但已经到了需要认真处理状态和 PHI 的前夜。
3. 维护成本：6/10。继续扩展前最好先决定 register state 是 SSA map、函数参数结构体，还是显式内存状态。
4. 更好的方案：下一步不急着接更多 opcode，先把 register state 做成显式结构，否则跨块和 call 很快会卡住。

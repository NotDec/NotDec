# 2026-05-19 Bin2LLVM 本地二进制模式计划

## 用户原始 prompt

现在需要对bin2llvm项目增加新的链路。支持基于LIEF解析二进制模块，然后直接调用那边转PCode的libsla库，得到PCode并转IR。全程没有任何Ghidra的Java层参与，仅复用libsla库。也可以考虑同时引入libdecomp库，如果里面的代码用的上。
实现要求：
- PCode转LLVM IR的代码部分直接用C++编写，直接使用libSLA/Ghidra的PCode结构，并动态链接LLVM库生成IR。
- 使用LLVM C++ API完成PCode转IR的具体逻辑部分。具体的转换逻辑可以参考：https://github.com/toor-de-force/Ghidra-to-LLVM/blob/master/src/xmltollvm.py
- 调用LLVM时，使用动态链接库。使用/sn640/NotDec/llvm-22.1.0.obj这里编译好的版本。


## 背景

`external/NotDec-bin2llvm` 现在已经有一条基于 Ghidra Java 导出的主链路，也有一条本地 native lowering 的雏形。
但这两条路现在还是混在同一份计划里，后面会把边界弄乱。

这次要单独把“本地二进制模式”拆出来，单独规划：

1. Java/Ghidra 模式保留。
2. 本地 native 模式单独推进。
3. 两条链路最后共享同一套 C++ lowering，但入口不同。

这里的本地模式不是替代 Ghidra，而是给 `bin2llvm` 再加一个工作模式：

- 一条模式继续走 GhidraScript / headless 导出。
- 另一条模式直接在 C++ 里解析二进制，再拿到 P-Code 或等价输入，继续 lower 到 LLVM IR。

## 目标

先把本地二进制模式做成一个能单独跑的闭环。

第一层目标：

1. 能直接读本地二进制文件。
2. 能拿到最基本的程序信息、入口、段、符号、架构信息。
3. 能把这些信息送进 native 侧 lowering。
4. 能输出结构正确的 LLVM IR。

第二层目标：

1. 本地模式和 Java 模式共存。
2. 两条模式共享同一套 C++ lowering 核心。
3. 上层能按参数切换模式，不把两条路绑死。

## 范围

这份计划只管本地二进制模式，不管 Java 侧导出流程的重构。

只做这些：

1. 二进制容器解析。
2. 架构和地址空间信息获取。
3. 函数 / block / 指令入口组织。
4. 本地输入到现有 lowering 的接线。

不先做这些：

1. 不删 Ghidra Java 路径。
2. 不做 JNI / JNA。
3. 不先做全程序精确恢复。
4. 不先补复杂类型恢复。

## 技术路线

### 1. 先把模式边界分开

当前代码里，Ghidra 导出和 native lowering 还是同一条线上的不同阶段。
本地模式要单独成入口，至少要做到：

1. 命令行能显式选模式。
2. Java 模式走现有导出逻辑。
3. 本地模式走二进制解析逻辑。

### 2. 本地模式先只做最小容器解析

本地模式第一步不要急着解析全语义。
先只拿这些：

1. 文件格式。
2. 架构。
3. entry point。
4. sections / segments。
5. symbols。
6. 需要时的动态链接信息。

这一步的重点是把“二进制是什么”搞清楚，不是立刻做完整反编译。

### 3. 解析库优先考虑 LIEF

如果本地模式要直接读 ELF / 共享库 / 相关二进制容器，优先考虑用 LIEF。

原因很直接：

1. 容器解析这件事没必要自己重写。
2. LIEF 可以先把二进制结构、符号和入口信息接出来。
3. 解析层和 lifting 层分开，后面更好排错。

### 4. lifting 层继续复用现有 C++ lowering

本地模式真正要共享的是 C++ lowering，不是共享导出方式。

所以本地模式的中间结果要尽量贴近现有 lowering 入口，避免额外发明一套只服务本地模式的 IR。

如果必要，也可以先用一个很薄的本地中间表示：

1. 先收二进制和基础元信息。
2. 再转换成现有 lowering 能消费的结构。
3. 最后才考虑是否需要更完整的原生 P-Code 表达。

### 5. libsla / libdecomp 对接规划

先把 `libsla` 作为主线接进来，`libdecomp` 暂时只保留为依赖和实验方向。

判断：

1. `libsla` 的边界清楚，负责从机器码和架构 spec 生成 raw P-Code。
2. `libdecomp` 涉及 Ghidra decompiler 侧的 `Architecture`、函数模型、符号表、调用约定和 action pipeline，第一步直接接主链路容易把问题混在一起。
3. 本地模式当前最缺的是真实二进制内存映射、函数范围和 P-Code 收集入口，不是马上复刻 Java decompiler 的 HighFunction。

实施顺序：

1. 先把现有 `SleighBytes` 里的 `.sla/.pspec` 加载、寄存器收集、`PcodeEmit` 收集逻辑拆成公共 C++ 库。
2. 公共库入口接受 `ghidra::LoadImage`、起始地址和长度，不再绑定 hex 字符串。
3. 旧的 hex 工具继续保留，用新的公共库实现，作为回归验证。
4. 下一步用 LIEF 生成真实 ELF `LoadImage`，把 executable segment 映射给 libsla。
5. 函数入口第一版来自 LIEF 符号表和 entry point，不先做递归反汇编和 CFG 恢复。
6. `.sla/.pspec` 第一版由命令行显式指定，等 x86_64 跑通后再做 LIEF machine 到 Sleigh spec 的自动映射。

`libdecomp` 的处理：

1. CMake 继续链接 `sleigh::decomp`，因为当前 sleigh target 已经暴露出来。
2. 主线不先调用 `libdecomp`。
3. 后续单独做一个 probe 工具，验证能否在 native 侧构造 decompiler 环境并拿到接近 Java `HighFunction` 的结果。
4. 如果 probe 稳定，再决定是否把 `libdecomp` 用作 raw P-Code 之后的增强层。

## 实现记录

### 2026-05-19

先把 LIEF 作为可选依赖接进 `external/NotDec-bin2llvm`，不动现有默认链路。

修改点：

1. `external/NotDec-bin2llvm/CMakeLists.txt:11-102`
2. 新增 `NOTDEC_BIN2LLVM_ENABLE_LIEF` 开关，默认关闭。
3. 新增 `NOTDEC_BIN2LLVM_LIEF_GIT_REPOSITORY`、`NOTDEC_BIN2LLVM_LIEF_GIT_TAG`、`NOTDEC_BIN2LLVM_LIEF_SOURCE_DIR` 三个缓存参数。
4. 在开关打开时，用 `FetchContent_Declare(lief ...)` + `FetchContent_MakeAvailable(lief)` 拉取 `https://github.com/lief-project/LIEF.git` 的 `0.17.6` release。
5. 先关掉 `LIEF_TESTS`、`LIEF_PYTHON_API`、`LIEF_EXAMPLES`，避免第一次接入把构建面铺太大。

验证：

1. `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-cmake-test`
2. 默认关闭 LIEF 时配置正常。

注意：

1. LIEF 0.17.6 本身要求更高的 CMake 版本，后续如果把开关打开，要再确认构建环境是否满足。

### 2026-05-19 补充

把 LIEF 的 ELF 解析先跑通到一个独立小工具里，确认 native 侧可以直接调用它读真实 ELF。

修改点：

1. `external/NotDec-bin2llvm/tools/CMakeLists.txt:1-67`
2. `external/NotDec-bin2llvm/tools/notdec-lief-elf.cpp:1-46`
3. 新增 `notdec-lief-elf` 可执行文件，仅在 `NOTDEC_BIN2LLVM_ENABLE_LIEF=ON` 时构建。
4. `notdec-lief-elf` 直接调用 `LIEF::ELF::Parser::parse()`，打印文件类型、架构、入口、section、segment、symbol、dynamic entry 统计。

验证：

1. `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-lief -DNOTDEC_BIN2LLVM_ENABLE_LIEF=ON`
2. `cmake --build /tmp/notdec-bin2llvm-lief --target notdec-lief-elf -j4`
3. `/tmp/notdec-bin2llvm-lief/bin/notdec-lief-elf /bin/ls`
4. 输出正常，`/bin/ls` 被识别为 ELF `DYN`，并打印了入口和各类计数。

### 2026-05-19 libsla 公共层拆分

按上面的 libsla 规划，先把旧 Sleigh hex 工具里的核心逻辑拆出来，给后面的 LIEF ELF loader 复用。

修改点：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/SleighLift.h:17-33`
   - 新增 `SleighSpecOptions`。
   - 新增 `findSleighSpecPath(...)`。
   - 新增面向 `ghidra::LoadImage` 的 `collectSleighPcode(...)`。
2. `external/NotDec-bin2llvm/lib/SleighLift.cpp:12-255`
   - 从旧工具迁出 varnode 转换、opcode 转换、`PcodeCollector`、寄存器收集、processor spec context 加载。
   - `collectSleighPcode(...)` 现在负责初始化 `ghidra::Sleigh`，加载 `.sla/.pspec`，按地址范围调用 `oneInstruction()` 收集 P-Code。
3. `external/NotDec-bin2llvm/tools/SleighBytes.cpp:15-140`
   - 保留旧的 hex bytes 输入和 `InMemoryLoadImage`。
   - 删除重复的 Sleigh 初始化和 P-Code collector 代码，改为调用新的公共 `collectSleighPcode(...)`。
4. `external/NotDec-bin2llvm/lib/CMakeLists.txt:20-37`
   - 新增 `notdec-bin2llvm-sleigh` 静态库，仅在 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON` 时构建。
   - 该库链接 `notdec-bin2llvm-core`、`sleigh::sla`、`sleigh::decomp`、`sleigh::support`。
5. `external/NotDec-bin2llvm/tools/CMakeLists.txt:83-115`
   - `notdec-bin2llvm-sleigh-bytes` 改为依赖 `notdec-bin2llvm-sleigh`。

验证：

1. `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-cmake-test`
   - 默认配置通过，默认链路没有引入 Sleigh 依赖。
2. `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-sleigh -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON`
   - Sleigh 配置通过。
3. `cmake --build /tmp/notdec-bin2llvm-sleigh --target notdec-sleigh-pcode notdec-sleigh-llvm -j4`
   - 两个旧 Sleigh 工具构建通过。
   - 上游 Ghidra / libdecomp 编译有大量 `-Wsign-compare` warning，当前不是本次改动引入的错误。

性能影响：

1. 默认 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH=OFF` 时没有新增编译目标，也没有影响现有 bin2llvm 主链路。
2. 运行时还没接入 NotDec 主 pipeline，不需要对 fortune 用例做运行时间对比。

复杂度评分：

1. 实现效果：7/10。先把 libsla 公共入口拆出来了，后面 LIEF loader 可以直接传 `LoadImage`。
2. 理解成本：6/10。新增一个 `SleighLift` 层，但它只承载原来已经存在的初始化和 collector 逻辑。
3. 维护成本：6/10。Sleigh 依赖仍然只在开关打开时构建，默认链路没有新增负担。

后续：

1. 新增 LIEF ELF `LoadImage`，支持按 VA 从 executable segment 读字节。
2. 新增 native pcode 工具，输入 ELF、函数地址/长度、`.sla/.pspec`，输出 P-Code。
3. 再把 native pcode 工具接到现有 `PcodeToLLVM`。

### 2026-05-19 LIEF ELF LoadImage 和 native pcode 工具

继续按计划推进第一步本地闭环，先不接主 `bin2llvm` CLI。

修改点：

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/LiefElfLoadImage.h:1-32`
   - 新增 `LiefElfLoadImage`，继承 `ghidra::LoadImage`。
   - 只负责把 ELF 可执行 `PT_LOAD` 段暴露成 VA 到字节的读取视图。
2. `external/NotDec-bin2llvm/lib/LiefElfLoadImage.cpp:1-59`
   - 从 LIEF `Binary::segments()` 收集 `LOAD + X` 段。
   - `loadFill(...)` 按 VA 查段，未映射地址和 bss 范围先读 0。
3. `external/NotDec-bin2llvm/lib/CMakeLists.txt:38-54`
   - 新增 `notdec-bin2llvm-native` 静态库。
   - 只在 `NOTDEC_BIN2LLVM_ENABLE_LIEF=ON` 且 `NOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON` 时构建。
4. `external/NotDec-bin2llvm/tools/notdec-native-pcode.cpp:1-127`
   - 新增 `notdec-native-pcode` 工具。
   - 参数为 `<elf-file> <sla-file> -a <address> -l <length> [-p root-sla-dir] [-s pspec-file]`。
   - 工具流程是 LIEF 解析 ELF，构造 `LiefElfLoadImage`，再调用 `collectSleighPcode(...)` 输出 P-Code。
5. `external/NotDec-bin2llvm/tools/CMakeLists.txt:90-101`
   - 新增 `notdec-native-pcode` 构建目标。
6. `external/NotDec-bin2llvm/lib/SleighLift.cpp:5-213`
   - 增加 XML `.sla` 检查。
   - 当前 libsla 入口需要压缩 `.sla`，遇到 Ghidra release 里的 XML debug `.sla` 时明确报错，避免直接抛 `LowlevelError`。

验证：

1. `cmake -S external/NotDec-bin2llvm -B /tmp/notdec-bin2llvm-native -DNOTDEC_BIN2LLVM_ENABLE_LIEF=ON -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON`
   - 配置通过。
2. `cmake --build /tmp/notdec-bin2llvm-native --target notdec-native-pcode -j4`
   - 构建通过。
   - 首次构建会编译 LIEF 和 Sleigh/Ghidra 依赖，耗时较长。
3. `cmake --build /tmp/notdec-bin2llvm-native --target notdec-lief-elf -j4`
   - 构建通过。
4. `/tmp/notdec-bin2llvm-native/bin/notdec-lief-elf /bin/ls`
   - 输出正常，`/bin/ls` 被识别为 ELF `DYN`，entry point 为 `0x6aa0`。
5. `/tmp/notdec-bin2llvm-native/bin/notdec-native-pcode /bin/ls /sn640/myprograms/ghidra_11.0.1_PUBLIC/Ghidra/Processors/x86/data/languages/x86-64.sla -a 0x6aa0 -l 16 -s /sn640/myprograms/ghidra_11.0.1_PUBLIC/Ghidra/Processors/x86/data/languages/x86-64.pspec`
   - 工具能启动并进入 native ELF + libsla 路径。
   - 这里没有得到 P-Code，因为该 `.sla` 是 XML debug 格式，当前 libsla 报错：`libsla expects a compressed .sla file`。

性能影响：

1. 默认开关仍然关闭，默认构建和运行不引入 LIEF / Sleigh。
2. 还没有接入 NotDec 主 pipeline，不需要对 fortune 用例做运行时间对比。

复杂度评分：

1. 实现效果：7/10。ELF 到 `LoadImage` 的接线已经完成，native pcode 工具也能走到 libsla；剩下阻塞是压缩 `.sla` 输入。
2. 理解成本：6/10。新增 `LiefElfLoadImage` 一层，但职责很小，只是 VA 读字节。
3. 维护成本：6/10。新库和工具都在双开关下，默认链路不受影响。

后续：

1. 解决压缩 `.sla` 来源。可以增加一个可选的 Sleigh spec compiler 工具，或者在构建时允许生成 compressed `.sla`。
2. 用 compressed x86-64 `.sla` 跑通 `/bin/ls` entry 附近 P-Code 输出。
3. 再新增 native LLVM 输出工具，复用现有 `PcodeToLLVM`。

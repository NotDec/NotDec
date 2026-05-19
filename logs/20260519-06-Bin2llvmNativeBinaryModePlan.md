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

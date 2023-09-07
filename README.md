# NotDec: WebAssembly Decompiler and Static Analysis Framework

[Website](https://notdec.github.io/NotDec/)

[中文](#中文)

NotDec is
1. A project that aims to demystify the internal of decompiler.
1. A webassembly decompiler that can experiment with new decompiler techniques.
   - Variable Recovery
   - Structual Analysis

# 中文

[文档站](https://notdec.github.io/NotDec/)

## NotDec: 反编译器原理分析

本项目旨在：
1. 学习现有反编译器的原理，系统地总结现有反编译器的工作，算法
1. 选择合适的算法，尝试实现自己的反编译器


### 资料收集

学习阶段：

1. LLVM IR基础：只要达到能手写LLVM IR的程度就行。即主要理解各种语言特性对应的是什么样的LLVM IR代码。同时理解带alloca的半SSA形式，即alloca里的变量是非SSA，外面的是SSA。
   - llvm-tutor 
   - ollvm源码
2. SSA与编译优化基础
   - [《Engineering a compiler》](https://book.douban.com/subject/20436488/) 上来先看9.3章，深入研读。其他的章节没那么重要
   - 再找找其他讲过SSA的中文书？

   关于SSA的实验
   - [mem2reg 实验指导 · GitBook (buaa-se-compiling.github.io)](https://buaa-se-compiling.github.io/miniSysY-tutorial/challenge/mem2reg/help.html)
   - [Lv9+.4. SSA 形式 - 北京大学编译实践课程在线文档 | 北大编译实践在线文档 (pku-minic.github.io)](https://pku-minic.github.io/online-doc/#/lv9p-reincarnation/ssa-form)

   其他不错的资料：
   - [《SSA book》](https://pfalcon.github.io/ssabook/latest/book-v1.pdf)
   - [《simple and efficient ssa construction》](https://pp.info.uni-karlsruhe.de/uploads/publikationen/braun13cc.pdf)

3. 反编译
   - [代码优化](docs/optimizers.md) 和 [变量恢复](docs/variable_recovery.md)
   - [控制流结构恢复](docs/backend.md)

#### 资料

[现有的反编译器和资料](papers.md)

### 规划：基于LLVM IR的反编译

为什么要使用LLVM IR？

1. 之后可以直接对接SVF，得到较好的指针分析结果。

优先实现wasm的反编译。

1. Wasm转LLVM IR
   1. WAVM是一个基于LLVM的wasm的JIT，有部分逻辑是WASM转 LLVM IR
      1. 生成的IR不够简洁，有很多为了编译到汇编的冗余的内容
2. TODO

### 规划：反编译阶段

![反编译的各个阶段](docs/src/imgs/decompiler-architecture.png)

图片来自[Static Single Assignment for Decompilation](https://yurichev.com/mirrors/vanEmmerik_ssa.pdf)

反编译中的关键算法： Type Recovery（通过指令约束推导类型） Structual Analysis(恢复控制流)

1. 前端：将字节码转为LLVM IR
2. 中端：优化与分析
   1. 分析函数参数、分析callee saved register (wasm可以跳过这个阶段)
   2. SSA构建：使得前端可以有些冗余的alloca，由SSA构建来将相关alloca消除。 （编译原理相关）
   3. GVNGCM：Global Value Numbering and Global Code Motion 优化算法，有强大的优化能力，有助于反混淆等。（编译原理相关）
   4. 内存分析：将各种通过内存访问的变量显式地恢复出来。可能要用到指针分析算法，类型恢复等。关键词：Memory SSA。
3. 后端：高层控制流恢复，将字节码转为AST，打印为高级语言的形式。

### 项目架构与工具

由于基于LLVM IR，因此语言采用C++。

开发环境：VSCode + CMake。将Wabt，LLVM等作为CMake的外部依赖。

#### 开发环境搭建 - DevContainer

2023年9月7日注：由于对LLVM的调试需求越来越多，项目转为使用本地源码编译的LLVM（RelWithDebInfo或Debug build），且编译得到的二进制文件过大，因此不再推荐使用DevContainer。

VSCode DevContainer。出于[性能考虑](https://code.visualstudio.com/remote/advancedcontainers/improve-performance)，在clone时可以直接clone到wsl的ext4文件系统里。

1. 安装Docker Desktop on Windows： https://docs.docker.com/desktop/install/windows-install/ （无论是家庭版还是专业版均可）
   1. 其他系统直接安装docker
1. 用vscode打开代码，安装Dev Containers插件，按Ctrl-Shift-P 然后输入查找 `Remote-Containers: Rebuild and Reopen in container`.
1. 等待构建，构建完成后会直接进入开发环境中。
1. 安装CMake相关插件，toolkit选clang。

如果出现了无法使用windows侧的ssh-agent提供的ssh key的forward功能：
https://stackoverflow.com/questions/72293035/error-communication-with-agent-failed-when-ssh-auth-sock-is-set-but-ssh-agent 

#### 开发环境搭建 - Linux

基于Ubuntu系统。

1. 软件安装
   - apt安装
      ```
      sudo apt install wabt python-is-python3 clang-14 cmake zlib1g-dev g++
      ```
   - 安装wasi-sdk到/opt
      ```
      wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-20/wasi-sdk-20.0-linux.tar.gz -P /tmp
      sudo tar xf /tmp/wasi-sdk-20.0-linux.tar.gz -C /opt
      ```
1. clone 本仓库
1. 安装LLVM 14
   - 方式1：下载提前构建好的LLVM，解压得到`llvm-14.0.6.obj`文件夹，放到项目根目录。
   - 方式2：执行`scripts/build-debug-llvm.sh`脚本，下载并构建LLVM源码。中途可能遇到内存不足的情况，需要手动降低并行数量到1。
      成功构建后可以将`llvm-14.0.6.obj`文件夹打包发送给其他人。
1. cmake build本仓库


#### 代码调试

直接使用自带的C/C++调试，不知道为什么会非常慢，gdb执行backtrace要卡3秒，各种step命令要卡5-6秒。因此安装使用vscode的CodeLLDB插件。

代码补全使用clangd插件。根据提示禁用Intellisense，然后根据插件提示确认下载clangd。

### 提交代码前

1. 写好commit message，简要概况所有的修改。
2. 检查添加的代码的注释和文档是否充足。


## TODO

1. 反编译器自身：能够对“内存”中的变量也构建SSA进行优化。
2. 最终的结果能够很好地重编译。
3. 反编译器实现过程尽量记录完善的文档，未来考虑整理扩写为系列教程。

1. 将wasm lift到LLVM IR
   - 支持将wasm内存直接映射到某个基地址，从而直接支持运行，以及memory grow相关指令。
   - 支持DWARF调试信息，从而映射回原wat，wasm
1. 设计一个映射，将lift之后的IR反向转回wasm

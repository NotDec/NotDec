# RetDec

https://zhuanlan.zhihu.com/p/509763117 有一些源码解读的内容。

- retdec的4.0是最后一个包含单独的ir转c工具的版本。相关代码`src/llvmir2hlltool/CMakeLists.txt` (要先checkout到v4.0。)
- 能否直接把ir传给

### 开发环境

在cmake配置里增加"-DCMAKE_INSTALL_PREFIX=/home/xx/retdec/build/retdec-install"，然后直接使用cmake插件，目标选择install。（因为retdec-decompile工具需要找到decompiler-config.json文件，因此需要安装）

#### 入口函数

- retdec-decompiler.cpp 主要是解压，脱壳什么的，然后调用retdec::decompile函数。这里的retdec是namespace，不是class，所以就在src/retdec/retdec.cpp。
- 其中非常重要的是读取share/retdec/decompiler-config.json，其中有llvmPasses这一项，指定了一系列pass名字。然后在那边retdec::decompile函数，他会根据config.parameters.llvmPasses里的值找到pass，然后依次加入passmanager里。然后pm.run(*module);运行结束之后，一切反编译过程都完成了。

#### Pass功能

vscode代码搜索方法：基于`src/retdec-decompiler/decompiler-config.json`，对每个pass的名字带双引号搜索。比如搜`"retdec-decoder"`

- retdec-provider-init： 这个pass负责给很多Provider类设置信息（到静态变量里）。FileImageProvider，DebugFormatProvider，DemanglerProvider等等。如果我们要设置一下简单的，可以直接在Pass运行前加。
- retdec-decoder：`src/bin2llvmir/optimizations/decoder/decoder.cpp` 负责把capstone的结果转成IR。
- "retdec-x86-addr-spaces"：把一些对FS，GS寄存器寻址的访问转成intrinsic call，如`getReadFsByte`
- retdec-x87-fpu: replace fpu stack operations with FPU registers
- retdec-main-detection: 识别出main函数之后，也就是（在retdec自己的config里）重命名了一下。
- retdec-idioms-libgcc：把一些libgcc的算数运算替换成LLVM里的运算。
- retdec-idioms：把常见的指令组合替换成别的指令？
- retdec-inst-opt：好像是简单的窥孔优化。
- retdec-inst-opt-rda
- retdec-cond-branch-opt 这些优化应该不用
- retdec-syscalls：好像是把系统调用转成对应的call？有一个map
- **retdec-stack：**关键Pass，识别栈指针相关的操作。
- retdec-constants：好像是识别常量的类型的。看不懂。里面好像有根据debuginfo，获取data段变量类型。
- retdec-param-return：识别call指令参数的存放什么的
- **retdec-simple-types**：关键类型识别Pass，eqSet和equation
- retdec-write-dsm：Generate the current disassembly？
- retdec-remove-asm-instrs：Remove all special instructions used to map LLVM instructions to ASM instructions
- retdec-class-hierarchy：好像是根据RTTI和vtable搞class的继承关系
- retdec-select-fncs：如果config里面选择了部分函数，就把其他函数删了。
- retdec-unreachable-funcs：删除不可达函数？
- retdec-register-localization让所有寄存器变成局部变量
- retdec-value-protect：Protect values from LLVM optimization passes
- **retdec-stack-ptr-op-remove**：重要的
- retdec-remove-phi：
- retdec-write-ll：
- retdec-write-bc：
- retdec-llvmir2hll：


### retdec 栈恢复源码解读

源码在`retdec\src\bin2llvmir\optimizations\stack\stack.cpp`

大体要做的，是把栈指针的偏移访问，都改成alloca。
1. 函数开头sub了栈指针，改成对应大小的alloca
1. 处理load/store
    - 如果是load：改为对对应alloca的对应偏移的load
    - 如果是store：改为对对应alloca的对应偏移的store

在上面算法的基础上，增加一个map，从变量映射到栈偏移。然后在load和store的时候尝试将ptr解析为栈指针的偏移。然后在对应大小的地方创建变量。

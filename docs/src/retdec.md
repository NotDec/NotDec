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


### retdec 栈恢复算法

源码在`retdec\src\bin2llvmir\optimizations\stack\stack.cpp`。总体思路非常简单，分析每个load和store用到的东西，提取成一个表达式树（SymbolicTree类）。首先判断表达式树里面有没有栈指针，没有就不处理。然后尝试把整个表达式树化简，把栈指针看作0，化简成一个常量，然后把这个常量当作栈偏移，创建一个alloca去替换它。

这个方法还是有很大问题，有许多处理不了的情况。从这个角度看，retdec确实是比ghidra差的。现在现有的开源反编译器里面也就ghidra最好了。比如如果存在memcpy这种函数的调用，由于直接传地址，所以不是load/store的形式，而是计算完地址直接传给函数了，导致没有将里面的值替换为新创建的alloca。

**代码解读**

- abi.cpp 主要负责提供两个函数，isStackPointer和（我们自己新增的）isMemory，判断一个值是否是栈指针。
- reaching-definition.cpp 计算load和store之间的到达定值关系？
- symbolic-tree.cpp 符号树。
    - expand操作，当初次构建符号树的时候，会从感兴趣的值开始反向遍历Use关系，生成符号树。
        - 我们适配wasm时，让栈指针的子节点为常量0。方便后续化简栈指针偏移的访问为常量（之后的化简操作会将栈指针视为常量0）。（retdec在分析非wasm程序时，会为寄存器创建对应的全局变量，初始值为0，但是我们wasm的栈指针初始值不为0，所以需要修改。）
    - simplifyNode操作，尝试将符号树化简。比如如果有算数操作，且两边都是常量，则会化简为运算后的结果。
- stack.cpp 栈分析的主体代码。遍历处理load/store指令。有三种情况：1 处理Load指令的指针，2 处理Store指令打算存进内存的值，3 处理store指令的指针
    - 对要分析的Value构建SymbolicTree（expand操作）。
    - 使用val2val这个map进行缓存，缓存已经化简过的结果，从被分析的value映射到化简后的ConstantInt。如果不在缓存里，继续后面的分析。
    - 首先判断当前的SymbolicTree里面有没有栈指针，如果没有就直接返回，放弃处理。
    - 化简当前的SymbolicTree，如果化简为ConstantInt常量，则继续处理，否则直接返回放弃处理。
    - （我们新增）使用off2alloca这个map从偏移映射到alloca指令，防止重复创建栈变量。
    - 把化简后的常量当作栈偏移，为每个不同的栈偏移创建变量。变量类型从load/store中找的好像。
    - 把当前被分析的Value替换为对应的Alloca指令。
- stack-pointer-op-remove.cpp 独立的pass，移除栈变量识别后无用的代码。

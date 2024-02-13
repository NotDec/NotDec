# RetDec

- [retdec/publications at master · avast/retdec (github.com)](https://github.com/avast/retdec/tree/master/publications) 
   - 综述：[Retargetable Analysis of Machine Code](http://www.fit.vutbr.cz/study/DP/PD.php?id=482&file=t) 

## 代码

https://zhuanlan.zhihu.com/p/509763117 有一些源码解读的内容。

- retdec的4.0是最后一个包含单独的ir转c工具的版本。相关代码`src/llvmir2hlltool/CMakeLists.txt` (要先checkout到v4.0。)

### 开发环境

在cmake配置里增加"-DCMAKE_INSTALL_PREFIX=/home/xx/retdec/build/retdec-install"，然后直接使用cmake插件，目标选择install。（因为retdec-decompile工具需要找到decompiler-config.json文件，因此需要安装）

在`deps/llvm/CMakeLists.txt`里面将LLVM的构建设置为`RelWithDebInfo`，后续调试的时候能够看到更多的内容，比如LLVM的结构体成员的内容。

可以使用Vscode + cmake 插件，在设置里为cmake配置额外的参数，设置安装的前缀路径即可。
```
"cmake.configureArgs": [
    "-DCMAKE_INSTALL_PREFIX=/sn640/retdec/build/retdec-install"
],
```

#### 入口

- retdec-decompiler.cpp 主要是解压，脱壳什么的，然后调用retdec::decompile函数。这里的retdec是namespace，不是class，所以就在src/retdec/retdec.cpp。
- 其中非常重要的是读取share/retdec/decompiler-config.json，其中有llvmPasses这一项，指定了一系列pass名字。然后在那边retdec::decompile函数，他会根据config.parameters.llvmPasses里的值找到pass，然后依次加入passmanager里。然后pm.run(*module);运行结束之后，一切反编译过程都完成了。

#### Pass

基于`src/retdec-decompiler/decompiler-config.json`列出来的每个pass名字，对每个pass的名字带双引号（比如`"retdec-decoder"`）在vscode里搜索，可以直接定位到对应的Pass代码位置。

- retdec-provider-init： 这个pass负责给很多Provider类设置信息（到静态变量里）。FileImageProvider，DebugFormatProvider，DemanglerProvider等等。如果我们要设置一下简单的，可以直接在Pass运行前加。
- retdec-decoder：`src/bin2llvmir/optimizations/decoder/decoder.cpp` 负责把capstone的结果转成IR。
- "retdec-x86-addr-spaces"：把一些对FS，GS寄存器寻址的访问转成intrinsic call，如`getReadFsByte`
- retdec-x87-fpu: replace fpu stack operations with FPU registers
- retdec-main-detection: 识别出main函数之后，也就是（在retdec自己的config里）重命名了一下。
- retdec-idioms-libgcc：把一些libgcc的算数运算替换成LLVM里的运算。
- retdec-idioms：把常见的指令组合替换成别的指令？
- retdec-inst-opt：好像是简单的窥孔优化。
- retdec-inst-opt-rda：
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

源码在[`retdec\src\bin2llvmir\optimizations\stack\stack.cpp`](https://github.com/avast/retdec/blob/master/src/bin2llvmir/optimizations/stack/stack.cpp)。总体思路非常简单，分析每个load和store用到的东西，提取成一个表达式树（SymbolicTree类）。首先判断表达式树里面有没有栈指针，没有就不处理。然后尝试把整个表达式树化简，把栈指针看作0，化简成一个常量，然后把这个常量当作栈偏移，创建一个alloca去替换它。

例如，对于下面的语法树：
```
src/optimizers/retdec-stack/retdec-stack.cpp:142: 
-----------------------------------------------
>|   %254 = getelementptr [16777216 x i8], [16777216 x i8]* @__notdec_mem0, i64 0, i64 %253
        >| @__notdec_mem0 = global [16777216 x i8] zeroinitializer
        >| i64 0
        >|   %calcOffset249 = add i32 %0, -16
                >|   %0 = load i32, i32* @__stack_pointer, align 4
                        >| @__stack_pointer = internal global i32 5247184
                                >| i32 0
                >| i32 -16
-----------------------------------------------
```

被化简为：
```
src/optimizers/retdec-stack/retdec-stack.cpp:171: 
-----------------------------------------------
>| i32 -16
-----------------------------------------------
```

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


### retdec 关键类型识别

源码在`retdec/src/bin2llvmir/optimizations/simple_types/simple_types.cpp`。感觉就是一个启发式的算法，利用库函数和DEBUG信息来恢复类型，涉及到一些指针分析的内容。比较迷惑的一点是这个Pass会运行两次，第一次应该是比较重要的，第二次涉及到一些前端的函数，主要做的好像就是针对宽字符类型参数的修复。

**数据结构**
- eSourcePriority：一个枚举类型，作用是定义优先级，每个`ValueEntry`、`TypeEntry`都有一个优先级，优先级越高，说明其类型信息越准确。
    - 0 | PRIORITY_NONE：默认优先级
    - 1 | PRIORITY_LIT：一般就是非用户定义的函数，如动态链接/静态链接/系统调用/IDIOM（不太明白是什么）
    - 2 | PRIORITY_DEBUG :表示该变量/函数是从debug信息中引入的。

- ValueEntry；对原生Value的封装，成员函数有value比较、hash以及`getTypeForPropagation`：就是获取value的类型，如果是普通指针的话就返回指向的元素类型，数组指针则返回数组内元素类型（函数名中的Propagation可能就是来自这里），如果是函数那么返回函数的返回值类型。

- TypeEntry： 同样也是对原生Type的封装，没有比较特殊的成员函数。

- EquationEntry；用于描述两个等价集的关系，就两个关系：
    - otherIsPtrToThis ： 是另一个集合的指向
    - thisIsPtrToOther ： 是另一个集合的指针

- EqSet (Equivalence set)：等价集，一个类型对应一个等价集，比较重要的成员变量有：
    ```C++
    public:
		TypeEntry masterType; //主类型，会在propagate时不断更新，
		ValueEntrySet valSet; //储存与指针有关的Value集合，propagate时会遍历
                              //这个集合，寻找优先级高的Value的类型作为主类型
		TypeEntrySet typeSet; //储存与指针有关的Type集合，propagate时会遍历
                              //这个集合，寻找优先级高的类型作为主类型
		EquationEntrySet equationSet; //储存有可能指向该Value的指针，实际并没有用到
    ```

- EqSetContainer ：储存Module中的全部等价集。

**代码解读**

第一次调用Pass的流程：

1. buildEqSets: 对所有全局变量，函数参数、Alloca指令调用`processRoot`，实际上就是为指针建立一个`EqSet`，并把跟它有关的Value、Type放进去。
    - processRoot(Value *v)：创建一个新的`EqSet`，将v加入待处理队列`toProcess`，并调用`processValue`。
    - processValue：处理待处理队列`toProcess`中的v，将v放入`EqSet`，并遍历v的所有`Use`，调用`processUse`。
    - processUse：真正的处理函数，因为use一般是各种指令，需要对不同的指令做不同的处理：
        - 通常情况就是把指令的操作数都加入待处理队列。
        - 如果是是`Store`指令，那指针操作数也要放入`toProcess`
        - 如果是常量表达式`ConstantExpr`，那就继续往下寻找真正Use的地方。
        - `PtrToInt`/`BitCast`：如果操作数是全局变量，那就放入`val2PtrVal`集合，因为ptrtoint和bitcast实际上就是把指针存在一个临时寄存器里，不知道为什么不处理alloc的指针。
        - ...

1. buildEquations：遍历`val2PtrVal`集合，更新`equationSet`，实际上就是维护指针和存指针的变量的关系。
1. propagate：在module的范围内进行的类型信息的传播与合并，优先级的作用体现在这里，优先级越高的类型会替代`masterType`。
1. apply：更新`valSet`中各个Value的类型，调用`IrModifier`修改类型。
1. eraseObsoleteInstructions：删除被替换掉的指令。
1. setGlobalConstants： 将没有Store指令的全局变量设置const属性，这个属性是retdec自己定义的。

第二次有不同的流程，遍历每个全局变量的Users，只处理两种指令：
- 如果是`CallInst`，如果全局变量为宽字符类型并且是函数的参数，则使用 IrModifier 对象将其类型更改为宽字符类型。
- 如果是`ConstantExpr`,那么继续往下寻找真正Use的地方，如果找到的是`CallInst`，就跟上面的流程基本一致。


### 结构分析-llvmir2hll

#### 移植

- 首先将`src/llvmir2hll`目录（以及include目录）复制了过来，然后使用替换把对应路径的include替换为新路径的include。
    ```
    cp -r /sn640/retdec/src/llvmir2hll ./retdec-llvmir2hll
    cp -r /sn640/retdec/include/retdec/llvmir2hll/* ./retdec-llvmir2hll/
    cp -r /sn640/retdec/src/common ./retdec-llvmir2hll/
    cp -r /sn640/retdec/include/retdec/common ./retdec-llvmir2hll/
    cp -r /sn640/retdec/include/retdec/utils ./retdec-llvmir2hll/
    cp -r /sn640/retdec/src/utils ./retdec-llvmir2hll/
    cp -r /sn640/retdec/include/retdec/config ./retdec-llvmir2hll/retdec-config
    cp -r /sn640/retdec/src/config/* ./retdec-llvmir2hll/retdec-config
    cp -r /sn640/retdec/src/serdes ./retdec-llvmir2hll/
    cp -r /sn640/retdec/include/retdec/serdes ./retdec-llvmir2hll/
    ```
    在vscode里面把`#include "retdec/llvmir2hll` -> `#include "backend/retdec-llvmir2hll`
    把`#include "retdec/common` -> `#include "backend/retdec-llvmir2hll/common`
    把`#include "retdec/utils` -> `#include "backend/retdec-llvmir2hll/utils`

    增加cmake里面的源码
    ` find common/ utils/ retdec-config/ serdes/ retdec-utils/ -name "*.cpp" `
- 发现使用了common里面的东西，把common目录也复制过来。同理复制config目录
- 修复大量编译报错，以及新版本LLVM的变化

#### 调用

分析`src/llvmir2hll/llvmir2hll.cpp`中对llvmir2hll的[调用](https://github.com/avast/retdec/blob/eba8d78c3432b6ad7c3aa9ef913b42b5ce7f9baf/src/llvmir2hll/llvmir2hll.cpp#L150)。

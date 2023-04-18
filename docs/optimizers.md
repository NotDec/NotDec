# Docs of optimizers

项目使用LLVM Pass框架来进行反编译中的中间代码优化。

## 相关资料

入门教程[banach-space/llvm-tutor: A collection of out-of-tree LLVM passes for teaching and learning (github.com)](https://github.com/banach-space/llvm-tutor)

中文文章[[翻译\]现代化地编写LLVM Pass -- part I-外文翻译-看雪论坛-安全社区|安全招聘|bbs.pediy.com (kanxue.com)](https://bbs.kanxue.com/thread-257665.htm)







## 变量识别

清除IR中的全局栈指针，并对其使用处的指令操作数(局部变量地址)进行转换，转换为对alloca里的东西的使用，把对mem0的使用，其中的使用mem0的栈部分，改为使用我们的alloca。
参考[retdec/stack.cpp](https://github.com/avast/retdec/blob/master/src/bin2llvmir/optimizations/stack/stack.cpp)

- 识别并消除函数开头和结尾的栈指针操作
- 把对mem0的取下标+load/store操作，根据全局变量还是局部变量，转变为（新创建的）局部变量或者全局变量的操作。

变量分为全局变量，局部变量，堆变量，堆变量一般直接调函数分配，暂时不考虑。
- 全局变量的访问：对mem0取下标的这个值是常量（范围大致在1024到一个很大的值之间）【可能存在偏移运算】
- 局部变量的访问：对mem0取下标的值，是栈指针（取global 0）【可能存在偏移运算】
- 也可能直接把地址存入变量里，即取地址。

其中偏移运算可能是常量也可能是变量，是变量时甚至可能存在乘法运算。
（如果把栈指针存到了结构体里怎么办？假装它没有定义结构体，定义了很多零散的变量？）结构体的问题在于，成员地址可能基于结构体自身的指针计算得到

- 如何判断global0是不是栈指针
- 如何匹配函数开头的栈指针的sub操作
- 如何判断哪些值是栈内存的指针



TODO：

能否保证语义安全？因为我们现在转出来的IR是能跑的，如果变量恢复后，是不是就不能跑了？比如部分变量识别失败，还是存到mem里去了。

### 实现

wasm解析为IR后先优化一下。


#### 如何判断哪些值是栈内存的"抽象解释"算法

和传统的数据流分析不同的地方在于，LLVM是SSA形式，每个值只有一个赋值点。因此，一个值要么是栈指针，要么不是。因此只需要直接循环迭代。但是LLVM里还是有变量，即alloca出来的值，可能因为控制流的跳转而来自不同的取值，从而导致基于变量计算出来的值，依赖于这个变量是不是和栈指针有关的东西。

要分析清楚对mem0取下标的这个值，是不是来自stackpointer的运算。
1. 一定来自stack pointer
1. 可能来自stack pointer
1. 一定不来自stack pointer

为每个llvm的Value维护一个bool类型变量表示是否是栈指针。
遍历所有基本块（可能拓扑排序会高效一点），直到某次完全遍历也没有任何变化
初始化：算法开始前已经判断了函数开头的栈指针值，对应的bool设置为true
如果遇到了运算，任意一个输入值对应true的话，结果也设置为true。

把变量也标为是栈指针类型？所有对这个变量的load都是栈指针？


union怎么办？先不考虑。


目前的解决方法：
遇到内存访问指令沿着use-def向上回溯，构造一条chain 看是否可达sp,如果可达，那么就是栈地址，并把它放在栈相关集合中。
如果存进去的值也是栈地址，那么就把对应偏移的放在另一个栈相关集合中。


TODO:

1.需要实现过程间分析，如果call指令的参数和返回值都是栈地址，需要把它们放到栈相关集合中。


## retype

类型恢复

参考[GrammaTech/retypd-ghidra-plugin: Retypd plugin for Ghidra reverse engineering framework from NSA (github.com)](https://github.com/GrammaTech/retypd-ghidra-plugin)
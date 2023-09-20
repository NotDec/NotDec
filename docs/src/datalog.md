# Datalog

基于Datalog描述程序分析的优点是开发迅速。

- Souffie使用了Futamura projections技术将datalog转C++代码

## 现有工具/资料

- 综述，介绍datalog：[Datalog and Recursive Query Processing](http://blogs.evergreen.edu/sosw/files/2014/04/Green-Vol5-DBS-017.pdf)

**基于Datalog的LLVM分析**

- [Vandal](https://github.com/vandaltool/vandalir) 

**基于Datalog的反编译**（相关的工作）

- webassembly decompilation based on datalog
- elipmoc/gigahorse
- ddisasm

### 将LLVM IR导出为Datalog规则

Datalog挺像数据库的。Souffle的语法也拓展了很多特性，使得导出的写法不止一种。
- VandalIR，基于python的llvmlite模块，直接解析IR字符串。
    - 导出IR时给每个函数和基本块有一个ID。
    - 指令还有一个virtual register，即指令`%xx = ...`最左边的部分。LLVM中负责打印IR的相关的源码在[llvm\lib\IR\AsmWriter.cpp](https://github.com/llvm/llvm-project/blob/llvmorg-16.0.6/llvm/lib/IR/AsmWriter.cpp#L2414)，可以看到，有Name的会打印Name，没有Name的打印编号Slot是由WriterCtx.Machine负责的。因此总体上还是按照LLVM的规范，依次给没有Name的Value按顺序赋序号。
    - 指令没有新的方式。

几个需要考虑的设计选择：
- 如何给每个函数、基本块、指令确定一个ID？
    - （最后选了）使用valueID，给每个Value一个ID，包括指令，函数参数，基本块，函数。类似的，给每个类型一个ID，用类型的字符串表示做去重。
        - 使用AssemblyAnnotationWriter，给IR里面的指令用注释标出来编号。
    - 函数名-基本块编号-指令编号：因为根据[这里](https://stackoverflow.com/questions/66528940/unique-identifierinteger-id-for-llvminstruction-across-different-run#comment117667940_66528940)，基本块在函数里的顺序是不会边的
    - 使用函数名-基本块名-指令名
        - 导出前用一个Pass给每个没有名字的基本块，指令命名。
        - 没有名字的使用`printAsOperand`获取。但是这样还是有问题，对于没有返回值的指令，比如store，会打印`void <badref>`。这种情况可以另外单独给个ID。
    - 使用基本块地址，指令的地址：LLVM不会移动指令。但是要求得在单次运行中完成，即直接导入souffle依赖，在内存里传递facts。
- 指令的表示
    - 每个指令单独一个decl
    - 指令复用同一个decl：用id/enum表示不同的指令opcode，引用其他指令就用Pos/ID表示
        - 遇到了常量怎么办？给常量也有一个ID？

目标：a webassembly and ethereum VM bytecode decompiler. 

侧重于前中端（转IR与IR的优化。）

### 从“零”实现反编译器

为什么要从零开始？为了更好地学习反编译器的原理。即使最后改为对接现有的反编译器。

怎么样的从零？可以使用现有的disassembler，IR，compiler等，但是不能直接去对接现有的反编译器。前期可以使用一些LLVM的Pass，后期最好都替换为自己写的Pass。

计划产出：

1. 反编译器自身：能够对“内存”中的变量也构建SSA进行优化。
2. 最终的结果能够很好地重编译。
3. 反编译器实现过程尽量记录完善的文档，未来考虑整理扩写为系列教程。

### 不知道接下来怎么办？（资料收集）

多看看现有的资料：

1. Static Single Assignment for Decompilation vanEmmerik_ssa https://yurichev.com/mirrors/vanEmmerik_ssa.pdf
2. [retdec/publications at master · avast/retdec (github.com)](https://github.com/avast/retdec/tree/master/publications) 
   1. 综述是[Retargetable Analysis of Machine Code](http://www.fit.vutbr.cz/study/DP/PD.php?id=482&file=t) 

参考现有的反编译器：

1. Ghidra [ghidra/docmain.hh at master · NationalSecurityAgency/ghidra (github.com)](https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh) 看代码前构建doxygen看文档。
2. [avast/retdec: RetDec is a retargetable machine-code decompiler based on LLVM. (github.com)](https://github.com/avast/retdec)
3. [Boomerang Decompiler (sourceforge.net)](https://boomerang.sourceforge.net/)     [BoomerangDecompiler/boomerang: Boomerang Decompiler - Fighting the code-rot :) (github.com)](https://github.com/BoomerangDecompiler/boomerang) 
4. [yegord/snowman: Snowman decompiler (github.com)](https://github.com/yegord/snowman) 

也要学习程序分析的内容。

[SVF-tools/SVF: Static Value-Flow Analysis Framework for Source Code (github.com)](https://github.com/SVF-tools/SVF) 

控制流恢复算法：

1. [No More Gotos: Decompilation Using Pattern-Independent Control-Flow Structuring and Semantics-Preserving Transformations – NDSS Symposium (ndss-symposium.org)](https://www.ndss-symposium.org/ndss2015/ndss-2015-programme/no-more-gotos-decompilation-using-pattern-independent-control-flow-structuring-and-semantics/) 
2. Phoenix: [Schwartz et al._2013_Native x86 Decompilation using Semantics-Preserving Structural Analysis and Iterative Control-Flow Structuring.pdf (cmu.edu)](https://users.ece.cmu.edu/~dbrumley/pdf/Schwartz et al._2013_Native x86 Decompilation using Semantics-Preserving Structural Analysis and Iterative Control-Flow Structuring.pdf) 

### 规划：基于LLVM IR的反编译

为什么要使用LLVM IR？

1. 之后可以直接对接SVF，得到较好的指针分析结果。

优先实现wasm的反编译。

1. Wasm转LLVM IR
   1. WAVM是一个基于LLVM的wasm的JIT，有部分逻辑是WASM转 LLVM IR
      1. 生成的IR不够简洁，有很多为了编译到汇编的冗余的内容
2. 

### 规划：反编译阶段

反编译中的关键算法： Type Recovery（通过指令约束推导类型） Structual Analysis(恢复控制流)

1. 前端：将字节码转为LLVM IR
2. 中端：优化与分析
   1. 分析函数参数、分析callee saved register (wasm可以跳过这个阶段)
   2. SSA构建：使得前端可以有些冗余的alloca，由SSA构建来将相关alloca消除。 （编译原理相关）
   3. GVNGCM：Global Value Numbering and Global Code Motion 优化算法，有强大的优化能力，有助于反混淆等。（编译原理相关）
   4. 内存分析：将各种通过内存访问的变量显式地恢复出来。可能要用到指针分析算法，类型恢复等。关键词：Memory SSA。
3. 后端：高层控制流恢复，将字节码转为AST，打印为高级语言的形式。

### 项目架构与工具

Markdown编辑器（建议）使用Typora，或VSCode

由于基于LLVM IR，因此语言采用C++。

开发环境：VSCode + CMake。将Wabt，LLVM等作为CMake的外部依赖。



### 提交代码前

1. 写好commit message，简要概况所有的修改。
2. 检查添加的代码的注释和文档是否充足。



### 其他
各种杂七杂八的事情随意地列在这里

1. 反编译优化后重新编译，和原有代码对比测试。效果好甚至可以作为字节码的优化器。
2. 未来实现得够好之后，可以重写README，该文档可以改名为`plan.md`放到docs文件夹里。


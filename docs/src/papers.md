# 现有论文与资源


## 资源集

[**Working Conference on Reverse Engineering (WCRE)：**](https://ieeexplore.ieee.org/xpl/conhome/1000635/all-proceedings )

[**PPREW-5: Proceedings of the 5th Program Protection and Reverse Engineering Workshop**](https://dl.acm.org/conference/pprew) 这个期刊好啊。

[**SSPREW: Software Security, Protection, and Reverse Engineering Workshop**](https://dl.acm.org/conference/ssprew)


其他：

- [RetDec的publication](https://github.com/avast/retdec/blob/05c9b11351d3e82012d823fa3709f940033768cf/publications/README.md)
- [Decompiler Design - Type Analysis](https://www.backerstreet.com/decompiler/type_analysis.php) 居然有介绍反编译器架构的网站。


Github的两个list：

- [Awesome-Info-Inferring-Binary](https://github.com/yasong/Awesome-Info-Inferring-Binary)
- [Awesome-Binary-Rewriting](https://github.com/SystemSecurityStorm/Awesome-Binary-Rewriting)

### 课程

- [CMU Lecture Notes on Decompilation (15411: Compiler Design)](https://www.cs.cmu.edu/~fp/courses/15411-f13/lectures/20-decompilation.pdf) 反编译内部各个阶段都有介绍。

## 现有的反编译器

1. Ghidra [ghidra/docmain.hh at master · NationalSecurityAgency/ghidra (github.com)](https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh) 看代码前构建doxygen看文档。
2. [avast/retdec: RetDec is a retargetable machine-code decompiler based on LLVM. (github.com)](https://github.com/avast/retdec)
3. [Boomerang Decompiler (sourceforge.net)](https://boomerang.sourceforge.net/)     [BoomerangDecompiler/boomerang: Boomerang Decompiler - Fighting the code-rot :) (github.com)](https://github.com/BoomerangDecompiler/boomerang) 
4. [yegord/snowman: Snowman decompiler (github.com)](https://github.com/yegord/snowman) 
5. angr好像也有了

## 读论文

一篇很好的综述：[Static Single Assignment for Decompilation (Boomerang)](https://yurichev.com/mirrors/vanEmmerik_ssa.pdf) 感觉可以抓住优化方面的脉络。

### 控制流结构恢复-structural analysis

很多都是借用现有的type recovery，重点去讲structure recovery。

- 【Phoenix】Native x86 Decompilation Using Semantics-Preserving Structural Analysis and Iterative Control-Flow Structuring [paper link](https://kapravelos.com/teaching/csc591-s20/readings/decompilation.pdf)
    
    [Edward Schwartz's PhD thesis](https://users.ece.cmu.edu/~ejschwar/papers/arthesis14.pdf) 里面进一步介绍了Phoenix反编译器
    
    这篇论文关注控制结构的恢复。控制结构的恢复最早是基于interval analysis的（？这是什么得学一学）。后面才被细化为structural analysis

- [【Dream】No More Gotos: Decompilation Using Pattern-Independent Control-Flow Structuring and Semantics-Preserving Transformations](https://www.ndss-symposium.org/wp-content/uploads/2017/09/11_4_2.pdf) [slides](https://www.ndss-symposium.org/wp-content/uploads/2017/09/11NoMoreGotos.slide_.pdf) [code](https://github.com/USECAP/dream)

### 类型恢复 - Type Recovery

- [【TIE】Principled Reverse Engineering of Types in Binary Programs.](http://users.ece.cmu.edu/~aavgerin/papers/tie-ndss-2011.pdf) 这篇基于VSA搞了自己的DVSA，主要区别是StridedInterval里可以放除esp外的变量符号？。重点主要在后面的约束求解部分。后面的类型系统和求解部分也非常复杂TODO。

- 【DIVINE】: DIscovering Variables IN Executables 这篇还是VSA系列的那些人写的。讲先用最简单的semi naive方法鉴别变量，跑VSA，然后拿VSA结果去生成约束跑ASI。迭代几次得到最好的结果。
里面说如果变量是8字节大小，那VSA直接无法处理，值总是Top（32位程序）。那就不能直接把内存最大切分粒度搞成4字节？？

- [【REWARDS】Automatic Reverse Engineering of Data Structures from Binary Execution](https://www.cs.purdue.edu/homes/xyzhang/Comp/ndss10.pdf) TODO

- [【retypd】](https://arxiv.org/pdf/1603.05495.pdf) 需要进一步学习类型系统的高级知识，比如subtyping。它不仅开源，而且不需要VSA的指针信息。可以与之前需要VSA的结合？~~但是似乎没有说怎么从一整块栈内存中识别出变量。~~

- [【SecondWrite】Scalable Variable and Data Type Detection in a Binary Rewriter](https://user.eng.umd.edu/~barua/elwazeer-PLDI-2013.pdf) 

### 变量恢复

变量恢复和类型恢复关联较大。如果把函数开头分配的栈空间看作一个巨大的结构体，变量恢复就转换成了类型恢复。

[OSPREY: Recovery of Variable and Data Structure via Probabilistic Analysis for Stripped Binary](https://www.cs.virginia.edu/~yk2bb/data/osprey_sp21.pdf) [slides](https://www.cs.purdue.edu/homes/zhan3299/res/SP21a_slides.pdf) 把变量的访问看作变量存在的暗示，同时存在很多这样的暗示，使用概率性的推导。好像是基于后面那篇BDA的工作。

### C++ 反编译

C++的类给反编译带来了额外的困难，涉及到（复杂的）约束求解等。

- [OOAnalyzer: Using Logic Programming to Recover C++ Classes](https://edmcman.github.io/papers/ccs18.pdf) C++反编译 
and Methods from Compiled Executables 

- SmartDec: Approaching C++ Decompilation.

- Reconstruction of Class Hierarchies for Decompilation of C++ Programs.

### VSA相关

- [WYSINWYX: WHAT YOU SEE IS NOT WHAT YOU EXECUTE](http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.76.637&rep=rep1&type=pdf) 第三章讲了VSA的事情。它也讲了很多二进制分析的事情。

    ■ 文中提到的一些其他的没有用到VSA的方法：
    
    [33] C. Cifuentes and A. Fraboulet. Interprocedural data flow recovery of high-level language code from assembly. Technical Report 421, Univ. Queensland, 1997.

    [34] C. Cifuentes and A. Fraboulet. Intraprocedural static slicing of binary executables. In Proc. Int. Conf. on Software Maintenance (ICSM), pages 188–195, 1997.

    [35] C. Cifuentes, D. Simon, and A. Fraboulet. Assembly to high-level language translation. In Proc. Int. Conf. on Software Maintenance (ICSM), pages 228–237, 1998.
    
    [45] S.K. Debray, R. Muth, and M. Weippert. Alias analysis of executable code. In Proc. Principles of Programming Languages (POPL), pages 12–24, January 1998.

- [基础的VSA](https://research.cs.wisc.edu/wpis/papers/cc04.pdf) Analyzing Memory Accesses in x86 Executables

- [Improved Memory-Access Analysis for x86 Executables](http://research.cs.wisc.edu/wpis/papers/etaps08.invited.pdf)  GMOD-Based Merge Function
     
     这篇论文开头有很多引用：Research carried out during the last decade by our research group [64, 65, 6, 56, 55, 7, 8, 36, 4, 49, 9] as well as by others [48, 22, 33, 14, 2, 31, 13, 44, 32, 3, 54, 37, 21, 46, 28, 19, 16, 34, 66] has developed the foundations for performing static analysis at the machine-code level.  能找到非常多的其他paper了？

     没想到相比于源码级的分析，二进制级的分析还有好处。有很多源码层没有指明的实现细节（比如C++的表达式求值顺序），源码级分析想要sound需要考虑所有可能的实现，而二进制级的分析只需要考虑编译器选择的实现。

- 《DIVINE: DIscovering Variables IN Executables》 （VSA with ASI(Automated Struct Identification)）（栈变量恢复）VSA可以用于一定程度的变量恢复。

- [这人](https://www.zybuluo.com/SmashStack/note/847219)想要实现Value Set Analysis到RadecoIL上。

- [这里](https://github.com/radareorg/radeco/tree/master/radeco-lib/src/analysis/valueset) 也有人实现，还有WYSINWYNX那个论文相关的东西？

- [这个课件](http://www-verimag.imag.fr/~mounier/Enseignement/Software_Security/slides_lecture_7.pdf)讲到了一点点。这门课是和安全相关的。TODO，不太看得懂。

### 框架与综述

- [C Decompilation : Is It Possible?](http://web.archive.org/web/20180517094139/http://decompilation.info/sites/all/files/articles/C%20decompilation.pdf) 2009的一篇

    第二章相关工作里面有不少引用：

    structural analysis：[4–6]，这个也用在了编译器：[8]。

    unification-based algorithm for recovery of types：Mycroft [9]

    现有反编译器：DCC decompiler [7]. Boomerang [11], REC [12] and Hex-Rays plug-in [13]

- [【rev.ng】rev.ng: A Multi-Architecture Framework for Reverse Engineering and Vulnerability Discovery.](https://www.rev.ng/downloads/iccst-18-paper.pdf)

    这个反编译器开源了lifter：先转到Qemu IR然后转到LLVM IR。这个好像也不太和反编译相关，也只是搞插桩、fuzzing的。

## 下游研究

- [Augmenting Decompiler Output with Learned Variable Names and Types](https://www.usenix.org/system/files/sec22summer_chen-qibin.pdf) 基于AI恢复反编译的变量名。

**反编译器测试**
- [【DecFuzzer】How far we have come: testing decompilation correctness of C decompilers](https://dl.acm.org/doi/abs/10.1145/3395363.3397370 ) [代码](https://github.com/monkbai/DecFuzzer)

    functionality-preserving disassembling and C style control structure recovery [17, 31, 47, 64, 65, 67]
	
    变量恢复static analysis and inference techniques [10, 12, 13, 30, 54]. 
	
    fool-proof techniques for binary disassembling and decompilation [17, 31, 64-67].

    EMI编译器测试看了下是插入了不影响语义的代码之后去开编译优化，发现优化器做出的错误决定而导致的crash。比如把一个不该执行的循环内操作提到外面。错误判断一些分支恒为真或假。是设置Csmith的输出使得只生成一个函数？？
    
    本来Csmith生成的代码很多全局变量的使用。如果全局变量改变了，很难手动找到是哪个函数？它是生成了局部变量，然后把对全局变量的使用全替换成了局部变量，函数结束的时候把局部变量的值update到全局变量，这样如果全局变量变了，就肯定是在最后update的时候改变的。那手动看的时候不要继续找内部怎么使用？这样做有什么好处吗。。。可能是方便找到这个函数到底涉及到了哪些全局变量，然后方便只提取这些到反编译结果的全局变量？？

- [Semantic Fidelity of Decompilers](https://apps.dtic.mil/sti/trecms/pdf/AD1173815.pdf) CMU的技术报告。这篇基于上面的进一步做了。基于副作用（全局变量，内存访问，函数调用）分析单个函数函数的反编译正确性。


### SecondWrite系列

- [Decompilation to Compiler High IR in a binary rewriter](https://user.eng.umd.edu/~barua/high-IR-technical-report10.pdf) 提升到编译器IR，然后优化。对写反编译有较大启发

    - 有一段提到了栈编译的坏处。


### 基于搜索的反编译

- [Evolving Exact Decompilation](https://www.cs.unm.edu/~eschulte/data/bed.pdf) 好像和主流的反编译技术不同。
- [Decompilation as search](https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-844.pdf) 

## Java 字节码反编译

Java反编译的几篇

- Proebsting and Watterson [24] 《Krakatoa: Decompilation in java (does bytecode reveal source?).》
- Dava Miecznikowski and Hendren [22] 《Decompiling java bytecode: Problems, traps and pitfalls》
- Naeem and Hendren [25] 《Programmer-friendly decompiled java,》
- Harrand et al. [27] present Arlecchino  《Java decompiler diversity and its application to meta-decompilation》
- [《Decompiler Implementation》](https://link.springer.com/chapter/10.1007/978-1-4302-0739-9_6) 一本书，讲Java字节码反编译的，暂未找到免费的电子版。

## 其他相关的静态分析技术

[Aggregate structure identification and its application to program analysis](https://dl.acm.org/doi/10.1145/292540.292553) 

## 其他

- [Decomperson: How Humans Decompile and What We Can Learn From It](https://www.usenix.org/system/files/sec22-burk.pdf) 人经常会看生成的汇编，做些很小的修改。意味着可能可以强化学习。另外反编译可能代码片段可以拆分成子任务？，因为人经常关注一个小片段。

- [A Survey of Software Reverse Engineering Applications | SpringerLink](https://link.springer.com/chapter/10.1007/978-3-030-24268-8_22) 讲了逆向软件分析的合理性，用处等等。以后写intro的时候很有用。

- [An evaluation of output from current Java bytecode decompilers: Is it Android which is responsible for such quality boost?](https://ieeexplore.ieee.org/abstract/document/8109451/) 

- [Retrofitting Security in COTS Software with Binary Rewriting](https://angelosk.github.io/Papers/2011/secondwrite.pdf) 给二进制增加保护。也提到了优化

- [DECOMPERSON: How Humans Decompile and What We Can Learn From It](https://www.usenix.org/system/files/sec22-burk.pdf) 调研人工反编译的过程

- [研究VSA对人工分析的帮助](https://www.ndss-symposium.org/wp-content/uploads/bar2021_23002_paper.pdf)。（对学习VSA没啥用）


**最近的新论文**

- [DEEPVSA](https://www.usenix.org/system/files/sec19-guo.pdf): Facilitating Value-set Analysis with Deep Learning for Postmortem Program Analysis 这篇参考意义不大，是ML结合的。

- BDA: Practical Dependence Analysis for Binary Executables by Unbiased Whole-Program Path Sampling and Per-Path Abstract Interpretation

- BinPointer: Towards Precise, Sound, and Scalable Binary-Level Pointer Analysis

    提及：[BPA: Refining Indirect Call Targets at the Binary Level](https://www.cse.psu.edu/~gxt29/papers/cfgByDatalog_NDSS21.pdf)这篇也值得读。用了块内存的抽象解释。


### 其他资料（网页等）：

- https://github.com/cmu-sei/pharos 涉及到很多反编译技术

- https://news.ycombinator.com/item?id=11218138 两个人的讨论。里面推荐对两篇文章的逆向引用搜索：https://scholar.google.com/scholar?as_ylo=2018&hl=en&as_sdt=2005&sciodt=0,5&cites=1148004013363547510&scipsc=   https://scholar.google.com/scholar?cites=7322807636381891759&as_sdt=2005&sciodt=0,5&hl=en 
- https://github.com/toor-de-force/Ghidra-to-LLVM https://uwspace.uwaterloo.ca/bitstream/handle/10012/17976/Toor_Tejvinder.pdf?sequence=3&isAllowed=y Ghidra Pcode编译到IR。代码太简单了。。栈内存好像是alloca出来的，可能还是想保持语义想运行。
- https://github.com/decomp/decomp 这人也想基于LLVM IR然后去优化。https://github.com/decomp/doc 相关文档 
- [The Decompilation Wiki.](http://www.program-transformation.org/Transform/DeCompilation)
- [github.com/repzret/dagger](https://github.com/repzret/dagger) 反编译到LLVM IR。aarch64还在开发过程中。[介绍的slides](https://llvm.org/devmtg/2013-04/bougacha-slides.pdf)

    dagger主要讲的是反编译到IR上，找到语义等价的LLVM IR的指令的过程。感觉有点像编译器后端的Instruction Selection，可能能用上利用DAG（有向无环图）选择指令的技术。它是作为llvm的fork编写的，2017后就没有维护了。和llvm耦合好严重啊，都不知道哪里是它的代码。好像好复杂。

- https://github.com/JuliaComputingOSS/llvm-cbe 曾经IR到C有一个backend，2016年被移除了。现在有人接手

- https://corescholar.libraries.wright.edu/cgi/viewcontent.cgi?article=3277&context=etd_all LLVM IR based decompilation。

- https://github.com/lifting-bits/sleigh sleigh作为Ghidra的反编译器，是用C++写的，而且汇编到pcode的lift部分也是它负责的。所以用Ghidra可能也只要用这个就可以了。

- [Ghidra上的ASI https://blog.grimm-co.com/2020/11/automated-struct-identification-with.html](https://blog.grimm-co.com/2020/11/automated-struct-identification-with.html)

## 领域的研究者


[Shuai Wang](https://www.cse.ust.hk/~shuaiw/)

[Gang Tan](https://www.cse.psu.edu/~gxt29/publications/)


**WebAssembly**

University of Stuttgart 的 [Michael Pradel](https://software-lab.org/people/Michael_Pradel.html) 以及他的学生 [Daniel Lehmann](https://dlehmann.eu/) 在webassembly方面发了很多paper。


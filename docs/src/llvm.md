# LLVM Basics

### 资料

书籍：
[Learning LLVM 12（中文版）](https://github.com/xiaoweiChen/Learn-LLVM-12)

[LLVM-Techniques-Tips-and-Best-Practies（中文版）](https://github.com/xiaoweiChen/LLVM-Techniques-Tips-and-Best-Practies)

如何操作LLVM IR：
1. https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/
1. 可以使用`llc -march=cpp` ： https://stackoverflow.com/questions/7787308/how-can-i-declare-a-global-variable-in-llvm 
1. https://llvm.org/docs/LangRef.html 查询IR

中端优化：
- 实现自己的mem2reg(SSA construction)：https://www.zzzconsulting.se/2018/07/16/llvm-exercise.html
- 中端优化为什么这么神奇：https://blog.matthieud.me/2020/exploring-clang-llvm-optimization-on-programming-horror/

YouTube的LLVM频道里有很多不错的视频。
- 2019 LLVM Developers’ Meeting: E. Christopher & J. Doerfert “Introduction to LLVM” 
    
    果然后端的水很深啊。
- 2019 EuroLLVM Developers’ Meeting: V. Bridgers & F. Piovezan “LLVM IR Tutorial - Phis, GEPs and other things, oh my! - Vince Bridgers (Intel Corporation)”》
    
    讲getelementptr指令讲得特别好，要是当时理解debug info metadata的时候看了这个就好了。
- 2019 LLVM Developers’ Meeting: J. Paquette & F. Hahn “Getting Started With LLVM: Basics”
    
    前半 讲了LLVM IR Pass需要考虑到的一些东西，users的概念，讲了移除基本块和指令时需要注意的。 后半部分讲Backend Pass也非常不错。
- 2019 LLVM Developers’ Meeting: S. Haastregt & A. Stulova “An overview of Clang ”
    
    讲了clang前端的架构。和之前想象中有些不一样。先是driver，然后是前端：词法分析语法分析，生成AST后codegen生成IR。
- 2017 LLVM Developers’ Meeting: D. Michael “XRay in LLVM: Function Call Tracing and Analysis ” 
    
    非常不错的Hook介入的框架
- 2018 EuroLLVM Developers’ Meeting: C. Hubain & C. Tessier “Implementing an LLVM based Dynamic Binary Instrumentation framework  - Charles Hubain 
    
    是Quarkslab的那个QBDI，有时间真得好好学学。
- 2017 LLVM Developers’ Meeting: “Challenges when building an LLVM bitcode Obfuscator ” 
    
    汇编混淆确实需要考虑很多情况。。
- 2014 LLVM Developers’ Meeting: “Debug Info Tutorial ”
- 2019 LLVM Developers’ Meeting: S. Haastregt & A. Stulova “An overview of Clang ”
    
    讲了clang前端的架构。和之前想象中有些不一样。先是driver，然后是前端：词法分析语法分析，生成AST后codegen生成IR。
- 2017 LLVM Developers’ Meeting: D. Michael “XRay in LLVM: Function Call Tracing and Analysis
    
    非常不错的Hook介入的框架
- 2018 EuroLLVM Developers’ Meeting: C. Hubain & C. Tessier “Implementing an LLVM based Dynamic Binary Instrumentation framework  - Charles Hubain 
    
    是Quarkslab的那个QBDI，有时间真得好好学学。
- 2017 LLVM Developers’ Meeting: “Challenges when building an LLVM bitcode Obfuscator ” 
    
    汇编混淆确实需要考虑很多情况。

其他：
- language server
  - [CppCon 2018: Ilya Biryukov “Clangd: architecture of a scalable C++ language server” - YouTube](https://www.youtube.com/watch?v=5HIyAXj1YNQ)
  - [Language Server Protocol and Implementation](https://www.apress.com/br/book/9781484277911) 
  - [Integrating with the Language Server Protocol](https://www.oreilly.com/library/view/modern-vim/9781680506006/f_0057.xhtml) 
- 《Getting Started with LLVM Core Libraries》前端代码转换的部分
  - [Getting Started with LLVM Core Libraries（中文版）](https://getting-started-with-llvm-core-libraries-zh-cn.readthedocs.io/zh_CN/latest/)


### debug

打印使用`llvm::outs() << xxx;`直接打印llvm Value


### 声明全局变量

Global variable definitions must be initialized.

Global variables in other translation units can also be declared, in which case they don’t have an initializer.

- `dso_local` 看作是C语言的static，在同一个编译单元内

LinkageTypes
- `external` 是默认的，如果没有initializer就会带一个external，如果有就没有修饰符，正常的符号。

### 嵌套的指令 nested instrucitons

https://lists.llvm.org/pipermail/llvm-dev/2015-October/091467.html

使用IRBuilder创建的这个其实不是嵌套的指令，LLVM也不支持嵌套的指令，而是创建了GEP constant expression。这种表达式比指令更好，同时蕴含着没有副作用的语义。



### PassManger

LLVM PASS的管理有两种实现

- LegacyPass Manger
- NewPass Manger

传统的LegacyPM有一些不足，例如Analysis Pass不能缓存分析的信息导致重复分析等。NewPM将Analysis和Pass做了区分。在LegacyPM中存在过多的全局变量与registries，每个Pass都需要通过宏来注册，NewPM改进了这一点，当然还有内联函数分析等其他优化。LLVM12使用的是LegacyPassManager，13之后默认使用的是NewPassManager，**本项目也使采用NewPassManager**。

### 运行与管理

LLVM API允许在应用程序中嵌入LLVM Pass，并将其作为库调用。

### 调试

我们基于 vscode开发，使用codelldb插件。发现无法下条件断点。
- 首先下普通断点
- 使用`breakpoint list` 查看断点编号
- 使用`breakpoint modify 1 -c "((int64_t) (ci->getSExtValue()) < -100)"`这样的命令给断点增加条件

更多使用方式见[lldb使用教程](https://lldb.llvm.org/use/tutorial.html)

一些非常实用的调试技巧

- 打印指令，值，函数等： `expr llvm::errs() << I`
- 打印模块：使用定义的工具函数： `expr notdec::printModule(M, "out.ll")`

### InstVisitor

```cpp
  class MyVisitor
      : public llvm::InstVisitor<MyVisitor> {
```

访问逻辑：

- 名字仅有visit的为入口函数。访问模块或者函数 (`Visitor.visit(M)`) 会帮你访问里面的指令
- 默认继承的父类的行为：
  - 指令的入口函数 (`void visit(Instruction &I)`): （一般不会override）根据指令类型调用细分的visit方法
  - 各种其他类型的细分visit方法调用对应的大类visit方法 例如 `visitLoadInst -> visitUnaryInstruction`。
  - 最后一个大类visit方法 `visitInstruction`： 为空。

总结：细分visit方法是我们要override的，如果没有override实现，则会因为继承的默认实现继续分流到大类方法，最终到最通用的`visitInstruction`。反过来说如果override了，（如果没有主动去调用）则会截断这类指令的访问，使得这类指令不会主动调用 `visitInstruction`。

需要注意，如果想要visitor有个返回值类型，需要在父类模板的第二个参数指定类型，同时必须实现visitInstruction方法，因为至少要给你的返回类型指定一个默认返回值。

### 修改LLVM值的类型

LLVM值的类型，基于def-use关系，其实利用类型转换还是可以灵活变动。比如一个整数，虽然可能有一些加法运算，但是你还是可以强制把它设成指针类型，然后在每个使用点插入ptrtoint指令转回去。

有些LLVM值的类型，修改起来非常麻烦。尤其是函数的参数和返回值类型。函数也是GlobalValue，甚至是Constant。

### 打印Pass前后IR

LLVM提供了方便的 `--print-after-all` 等选项。首先在创建PassBuilder的时候，创建插桩的类并注册CallBack。注册所有的标准插桩后很多选项都可以生效，包括计算Pass的运行时间，是否在每个前后打印Pass，展示IR的变化diff，甚至生成HTML的报告。

```cpp
// add instrumentations.
PassInstrumentationCallbacks PIC;
StandardInstrumentations SI(::llvm::DebugFlag, false, PrintPassOptions());
SI.registerCallbacks(PIC, &FAM);
```

现在新的Pass都继承自 `PassInfoMixin` 类，这个类会增加一个name函数，在运行时可以动态获取类的名称（包含namespace）。如果不确定可以在`--print-after-all` 后在log里查看。这个名称称为 PassID 。

命令行参数里指定的是通过PassID查找得到的缩略名称。需要提前在 `PassInstrumentationCallbacks` 中注册：

```
  PIC.addClassToPassName("notdec::LinearAllocationRecovery",
                         "linear-allocation-recovery");
  PIC.addClassToPassName("notdec::PointerTypeRecovery",
                         "pointer-type-recovery");
```

此时就可以在命令行中指定相关的pass打印了。比如 `--print-before=pointer-type-recovery` 。

### Clang 

**AST handling**

- 创建AST时有两种方式
  1. 使用Create方法，如`clang::FunctionDecl::Create(...)`
  2. 没有Create方法时，使用ASTContext的new操作符。如：`new (ASTCtx) clang::GotoStmt(...)`

**CFG handling**

CFGBlock包含：
- LabelStatement，用于生成Goto语句。
- 一系列语句
- CFGTerminator
  - 对于return/unreachable：保存对应的返回语句或函数调用语句。
  - 对于br/switch，保存对应的条件表达式。同时保证successor的顺序：本来CFG保证的是If-Then-Else顺序，但是如果是条件跳转，我们使用True-False的顺序，和IR里的successor()顺序一致。


- CFGBuilder在LLVM中是逆序创建的，即逆向遍历AST，逆向创建语句。在打印或者遍历CFG块的时候，其中`CFGBlock::ElementList`把正常的iterator改用reverse_iterator实现。存储的时候是逆向存储，但是后续读取每个CFGBlock的时候也被偷偷改成逆向读取。但是我们算法如果不是逆向的话，想要在末尾继续插入语句，反而需要插入到开头，内存开销增加了。
  - 如果将结构恢复算法弄成逆向的，首先找到所有的没有后继节点的结束块，每个结束块分治。
- 对于`if (A && B && C)`，`CFGBlock->getTerminatorStmt()`会在C处返回整个`A && B && C`表达式。调用`getLastCondition()`，会把CFGBlock最后一个statement转换成Expr返回。

**BumpVector Memory Allocation**

Clang的内存分配都是通过ASTContext进行的，内部使用了一个BumpAllocator，Bump是一种简单的线性内存分配，内存不会真正被释放，放弃内存复用。或者等一整块分配结束之后，再整块全部释放。根据[这里](https://github.com/llvm/llvm-project/blob/e6de9ed37308e46560243229dd78e84542f37ead/clang/include/clang/AST/ASTContext.h#L617)，ASTContext解构时释放所有存储AST节点的内存。

- 根据[这里](https://discourse.llvm.org/t/checker-for-destruction-needing-classes-allocated-in-bumpptrallocators/15386)，从这里分配的相关的数据结构内部不能有std::vector或者SmallVector这种会在堆上分配内存的数据结构。因为不会调用destructor，导致堆上的内存不会被free，导致内存泄露。

BumpVectorContext存储了一个`llvm::PointerIntPair<llvm::BumpPtrAllocator*, 1> Alloc;`，这是一个指针低位复用的数据结构，可以理解为一个指针加一个bool值。用户传入外部的allocator，比如ASTContext的，完全不释放内存。也可以让它新建一个allocator，并在解构的时候释放所有内存。

Clang CFG内置了一个BumpVectorContext，解构时会释放所有内存。因此在结构分析过程中，CFG相关的基本块虽然出现创建后又删除的情况，但不会导致内存泄露。 TODO: 一些临时的AST节点，如一些临时的Goto语句，它们的内存释放怎么办？

这次在BumpVector里增加的erase函数，仅把元素移动到末尾然后解构，和内存泄露应该无关。

**clang::QualType**

QualType 本质上就是一个指针，同时复用了低位的bit位，存储一些qualifier信息，如volatile等修饰符。
- QualType大部分情况下可以看作是clang::Type*
- QualType有可能为空指针，同时也可以直接创建空的QualType

**clang::IdentifierInfo**

IdentifierInfo除了名字字符串之外，仅仅存储了一些名字的种类信息，例如是不是define的，是不是keyword，是不是variable或者function name。

- **变量重名**：它似乎并不对应作用域，并不用于检测变量重名。
  - [`Sema::LookupName`](https://github.com/llvm/llvm-project/blob/f15014ff549a8686671a599f7b49ce9963769eaf/clang/lib/Sema/SemaLookup.cpp#L1914) 内部手动遍历检测了重名情况。


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

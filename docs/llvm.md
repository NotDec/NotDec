
### 资料

如何操作LLVM IR：
1. https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/
1. 可以使用`llc -march=cpp` ： https://stackoverflow.com/questions/7787308/how-can-i-declare-a-global-variable-in-llvm 
1. https://llvm.org/docs/LangRef.html 查询IR

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

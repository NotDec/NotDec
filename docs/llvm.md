
### 资料

如何操作LLVM IR：
1. https://mukulrathi.com/create-your-own-programming-language/llvm-ir-cpp-api-tutorial/
1. 可以使用`llc -march=cpp` ： https://stackoverflow.com/questions/7787308/how-can-i-declare-a-global-variable-in-llvm 
1. https://llvm.org/docs/LangRef.html 查询IR

### 声明全局变量

Global variable definitions must be initialized.

Global variables in other translation units can also be declared, in which case they don’t have an initializer.

- `dso_local` 看作是C语言的static，在同一个编译单元内
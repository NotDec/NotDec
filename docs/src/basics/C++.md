# C++ Development

### stackoverflow

1. 代码规范遵守LLVM的，https://llvm.org/docs/CodingStandards.html#do-not-use-rtti-or-exceptions 注意不要使用异常处理，用abort替代。可以使用[特殊的宏](https://stackoverflow.com/questions/2849832/c-c-line-number)辅助打印错误信息。
    ```cpp
    if(this->globs.size() != 0) {
        std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: Cannot add module when globals is not empty" << std::endl;
        std::abort();
    }
    ```
1. 最好不要在头文件里用using namespace。但是可以在函数体开头使用。 https://stackoverflow.com/questions/223021/whats-the-scope-of-the-using-declaration-in-c
1. 返回值选择object还是pointer： https://stackoverflow.com/questions/13213912/returning-an-object-or-a-pointer-in-c
1. 是否使用智能指针：https://stackoverflow.com/questions/106508/what-is-a-smart-pointer-and-when-should-i-use-one
1. 用引用还是指针：https://stackoverflow.com/questions/7058339/when-to-use-references-vs-pointers
1. 引用和指针的区别：https://stackoverflow.com/questions/57483/what-are-the-differences-between-a-pointer-variable-and-a-reference-variable

### 注意事项

1. 注意相关STL函数使用要带上check。
    1. vector使用back一定要检查是否是空！！！ `assert(stack.size() > 0);`

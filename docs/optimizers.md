# Docs of optimizers

项目使用LLVM Pass框架来进行反编译中的中间代码优化。

## 相关资料

入门教程[banach-space/llvm-tutor: A collection of out-of-tree LLVM passes for teaching and learning (github.com)](https://github.com/banach-space/llvm-tutor)

中文文章[[翻译\]现代化地编写LLVM Pass -- part I-外文翻译-看雪论坛-安全社区|安全招聘|bbs.pediy.com (kanxue.com)](https://bbs.kanxue.com/thread-257665.htm)



## 规范

### Manger

LLVM PASS的管理有两种实现

- LegacyPass Manger
- NewPass Manger

传统的LegacyPM有一些不足，例如Analysis Pass不能缓存分析的信息导致重复分析等。NewPM将Analysis和Pass做了区分。在LegacyPM中存在过多的全局变量与registries，每个Pass都需要通过宏来注册，NewPM改进了这一点，当然还有内联函数分析等其他优化。LLVM12使用的是LegacyPassManager，13之后默认使用的是NewPassManager，**本项目也使采用NewPassManager**。

### 运行与管理

LLVM API允许在应用程序中嵌入LLVM Pass，并将其作为库调用。





## stack

清除IR中的全局栈指针，并对其使用处的指令操作数(局部变量地址)进行转换。
参考[retdec/stack.cpp](https://github.com/avast/retdec/blob/master/src/bin2llvmir/optimizations/stack/stack.cpp)



## retype

类型恢复

参考[GrammaTech/retypd-ghidra-plugin: Retypd plugin for Ghidra reverse engineering framework from NSA (github.com)](https://github.com/GrammaTech/retypd-ghidra-plugin)
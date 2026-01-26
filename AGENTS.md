### 重构类型推理Pass

- external/binarysub 这个目录下有核心类型推理算法文件。但是里面使用的是自定义的函数式语言。需要模仿里面语言的处理，但是使用类型推理的核心接口，比如constraint函数等，基于LLVM IR Visitor去进行类型推理。
- src/TypeRecovery/mlsub 这个目录下是核心的pass相关文件。头文件在 include/notdec/TypeRecovery/mlsub
  - 其中主要的Pass入口在src/TypeRecovery/mlsub/MLsubGenerator.cpp。里面有Pass的定义，类型推理算法的主体类的定义。
  - 其中src/TypeRecovery/mlsub/MLsubGraph.cpp文件里面的类只需要作为external/binarysub里面的类型推理算法的封装。
  - src/Passes/PassManager.cpp 里面是运行Pass的入口。已经改好了，会自动运行Pass。
  - 其他地方的无关代码太多，最好不要阅读。而且最好不要使用搜索功能，而是阅读整个文件。

MVP阶段：当前需要解决代码报错，先在输入上一个最简单的只有空的main函数的情况下跑起来。


代码构建使用的命令： 

cmake --build /sn640/NotDec/build --config Debug --target all --

代码运行使用的命令：

./run.sh test/backend/Simple1.ll

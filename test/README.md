
### wasm

基于sysY语言的测试用例。

流程：

1. 首先使用编译器编译为wasm
    - 使用`add_include.sh`预处理sy文件，增加include头文件的代码。
    - 使用`test/wasm/sysy/compile.sh`脚本，编译每个文件为wasm，生成辅助的wat文件。
        - （同时编译辅助的`sylib.ll`文件）
2. 然后使用我们的反编译器编译为LLVM IR
    - 使用test_functional.py脚本。
3. 最后使用lli执行，对比输出是否是期望的结果

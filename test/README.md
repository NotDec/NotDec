
## wasm

### 基于sysY语言的测试用例。

流程：

1. 首先使用编译器编译为wasm
    - 使用`add_include.sh`预处理sy文件，增加include头文件的代码。
    - 使用`test/wasm/sysy/compile.sh`脚本，编译每个文件为wasm，生成辅助的wat文件。
        - （同时编译辅助的`sylib.ll`文件）
2. 然后使用我们的反编译器编译为LLVM IR
    - 使用test_functional.py脚本。
3. 最后使用lli执行，对比输出是否是期望的结果

2023年3月1日 成功通过“所有”用例。除了`83_long_array.c~`是因为栈分配得太大，栈爆了，直接改后缀名排除了。

### 编译器测试用例

基于：https://github.com/c-testsuite/c-testsuite 

### 手动编写测试用例

### 编译选项的选择

- `/opt/wasi-sdk-20.0/bin/wasm-ld --help`
- 如何

链接libc的好处是，可以利用memcpy等函数的实现。默认函数不导出且没有被使用就会被优化删除。

1. `-I. -g -O0` 
    - 让编译时能找到当前文件夹下的头文件，开启调试信息
    - 关闭优化：开启后，栈指针相关操作，栈上变量访问将没那么明显
1. `-Wl,--export-all` 导出所有函数。当链接libc的时候，可能会导出大量的`wasi_snapshot_preview1`相关函数，即使没有人调用。
1. `-fno-builtin`
1. `-fno-lto`
1. `--no-standard-libraries`
1. `-Wl,--no-entry` 不添加会报错`wasm-ld: error: entry symbol not defined (pass --no-entry to suppress): _start`
1. `-Wl,--allow-undefined`
1. `-lc` 链接libc

### 常见问题

#### 2023年3月21日 编译命令 出现对memcpy等函数的导入

原本的编译命令是：`-I. -g -O0 -fno-builtin --no-standard-libraries -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -o`

发现增加`-lc`有效（-l是一个命令行选项，表示链接什么库，参数不用加空格，-lc表示链接libc），但是会增加很多wasi_snapshot_preview1函数的导入，去掉`-Wl,--export-all`就好了，但是此时没有main函数了，只有__original_main

有时会出现`undefined symbol: mem0`，因为我sylib里面的函数实现

不能直接把编译的sylib链进来，而是应该在反编译后链进来。直接链进来，外部函数直接是wasi_snapshot_preview1的函数，相当于反编译静态链接的模块。如果不直接链接进来，则反编译之后会出现mem0，而那边sylib不能直接访问mem0了，所以必须要显式声明mem0，即相关的外部操作接口都需要自己过一遍。

#### 2023年5月20日 编译命令 出现memory.copy等指令不支持的情况。

似乎是正常的，支持一下memory相关指令转为llvm的原语。

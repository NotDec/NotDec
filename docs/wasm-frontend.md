wasm frontend 负责将WASM字节码转为LLVM IR。

LLVM的好处就在于可以先生成比较差的IR，然后通过优化Pass不断修补。


## WASM 现有工具

1. WAVM也是一个基于LLVM的带JIT功能的runtime。C++编写
   1. `WAVM\Lib\LLVMJIT\LLVMCompile.cpp` LLVMJIT::compileModule这个函数应该是编译入口点，很多可以参考。
   1. `WAVM\Lib\LLVMJIT\EmitFunction.cpp` EmitFunctionContext::emit 编译每个函数。关键是`decoder.decodeOp(*this);`这句，会根据不同的指令访问对应的同名函数，比如看`WAVM\Lib\LLVMJIT\EmitCore.cpp`，遇到block指令会调用EmitFunctionContext::block函数。
1. [aWsm](https://github.com/gwsystems/aWsm) 也是一个基于LLVM的带JIT功能的runtime。虽然是rust写的，但是还是用的LLVM C++ API，转换相关的逻辑也都是可以抄的。
1. WAMR wasm-micro-runtime 基于LLVM的，但是是C语言，使用LLVM-C-API，我们打算用的是C++的API。
    1. 真的是自己写的字节码解析器好像。。。[wasm_loader.c](https://github.com/bytecodealliance/wasm-micro-runtime/blob/3220ff6941b64de684a5a60a5e3f8adad4a18fb0/core/iwasm/interpreter/wasm_loader.c) [wasm.h](https://github.com/bytecodealliance/wasm-micro-runtime/blob/3220ff6941b64de684a5a60a5e3f8adad4a18fb0/core/iwasm/interpreter/wasm.h)
    1. 有相关wasm到LLVM IR的转换可以参考：[aot_llvm_extra.cpp](https://github.com/bytecodealliance/wasm-micro-runtime/blob/c07584400134bb5f1be80b4f5df96eb1d8c94324/core/iwasm/compilation/aot_llvm_extra.cpp)


## 代码架构

- wasm模块解析器：基于wabt。[wasm-c-api](https://github.com/WebAssembly/wasm-c-api)不太行因为是用来embed一个WASM VM的。
   - 目前直接通过
   - 未来考虑通过find_package直接使用： https://github.com/WebAssembly/wabt/pull/1980


## Specification

需要了解LLVM IR的语义：

1. [LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html) 
2. [2019 EuroLLVM Developers’ Meeting: V. Bridgers & F. Piovezan “LLVM IR Tutorial - Phis, GEPs ...” - YouTube](https://www.youtube.com/watch?v=m8G_S5LwlTo) 

和WASM的语义：[Modules — WebAssembly 2.0 (Draft 2022-09-27)](https://webassembly.github.io/spec/core/binary/modules.html) 注意现在直接翻标准是新release的2.0标准了。我们暂时先支持1.0标准，wabt现在也仅支持1.0，如果文件头里写version为2会报错。1.0的标准可以看[这里](https://www.w3.org/TR/wasm-core-1/#syntax-importdesc)

1. 名字比较难处理，wasm的[name section](https://github.com/WebAssembly/extended-name-section/blob/main/proposals/extended-name-section/Overview.md)允许重名，而且wasm中因为是二进制格式，理论上名字可以取任意utf-8。那边wat格式的定义也有类似的问题。但是wabt似乎已经处理了相关的问题？
   - 在src\binary-reader-ir.cc里的BinaryReaderIR::GetUniqueName函数，如果重名了会加数字后缀。
1. 类型：i32 i64 对应LLVM中的i32 i64, f32 f64对应LLVM中的float double。
1. 每个wasm的Global值转为llvm中一个的global值。相关访问只有Load和Store指令。
   1. 名字直接使用wabt那边传过来的名字 ~~名字更改为`global_<ind>_<original_name>`这种格式，即在原来名字前加上前缀标识。~~
   1. [Linkage Types](https://llvm.org/docs/LangRef.html#linkage-types) 选择internal。被导出的更改为external。
   1. 根据mutable，设置llvm那边的const属性
   1. 处理init_expr
3. 内存：转为一个global数组，u8 array。
   1. 内存访问：计算关于u8的偏移（get element ptr），然后再转为对应的类型指针load出来。即LLVM中`[大数字 x i8]`类型。因为只是分析，所有不用考虑内存增长的事情。
4. 函数
   1. 每个Local转化为函数开头的一个alloca。
   2. [非直接跳转 callind](https://webassembly.github.io/spec/core/exec/instructions.html#xref-syntax-instructions-syntax-instr-control-mathsf-call-indirect-x-y) 

### 难以处理的情况

1. import了一个table。


## 指令、栈、控制流的处理

参考WAVM，见顶部现有工具一节。参考栈验证逻辑。能保留的最好直接解码为SSA。这里的block直接考虑[Multi Value Extension](https://github.com/WebAssembly/multi-value)，防止以后架构需要重构，但是函数返回多个值的先不支持。

- 每个栈上元素对应一个SSA的Value。某种形式上可以维护一个Value栈（作为局部变量，不需要作为Context）。
- 控制流跳转维护一个block的嵌套栈，保存br时跳转的目标。
- 处理Block的时候，这里用递归和用栈都可以。选择用实现起来更简单的递归。

### 控制流指令的处理，与SSA生成

1. block，loop分别对应在结尾，开头，增加一个label。注意到block只需要为return的值创建Phi，loop需要对参数创建Phi。
1. if对应一些label和br_if，br代表直接跳转，br_if同理，根据语义找到对应的跳转目标，生成条件跳转即可。
1. br_table看似比较麻烦，看了下和LLVM的switch语句对应得非常好啊。也是根据不同的值跳转到不同的边。

### Wasm中的非直接跳转（复习）

在二进制模块中有id为4的table section。里面有一系列的table类型，初始化则由element section负责。table类型有两部分，reftype和limit。[limit](https://webassembly.github.io/spec/core/binary/types.html#limits)应该是类似数组大小的东西，但是同时包含了上限和下限。

reftype其实就是个enum，表示是不透明的external类型还是function类型。即，光是table section里，有用的信息只有定义了index，给每个index处的table标明了上下限。

接下来看elem section。~~它可以是passive的，即等着被table.init指令使用，用来初始化某个table。或者是active的，直接初始化某个table。~~ 现在直接翻标准是新release的2.0标准了。我们暂时先支持1.0标准，wabt现在也仅支持1.0，如果文件头里写version为2会报错。（这个version是在整个二进制模块的header处定义的。）elem section由三部分组成：

1. table index, 初始化哪个table。目前因为只有一个table，所以必须是0。
2. offset, 常量表达式，即一些指令。例如：`41 01 0b`解码为`i32.const 1; end;`。
3. vec(func ind) 一系列函数下标，表示要初始化成这些。

[Features to add after the MVP - WebAssembly 中文网|Wasm 中文文档](https://www.wasm.com.cn/docs/future-features/) https://www.w3.org/TR/wasm-core-1/#element-segments%E2%91%A0 （可以在这个页面搜索`at most one`） 这里提到了，MVP标准中wasm最多有一块内存，最多有一个table。

对应到LLVM IR的关键是，相同的语言特性会怎么在LLVM IR上实现/怎样的LLVM IR会编译到这样的wasm。LLVM里只有Call指令，但是参数是一个函数地址的value。目前看来可以搞一个函数指针数组，对应初始化后的table。然后将callind翻译为从函数指针中取，然后再call。



## TODO

最好能实现单个函数的反编译与混淆，即转换回Wasm时最好能保证其他部分不变。。。如果使用LLVM自己的wasm后端好像有点复杂

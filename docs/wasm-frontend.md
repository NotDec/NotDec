wasm frontend 负责将WASM字节码转为LLVM IR。

LLVM的好处就在于可以先生成比较差的IR，然后通过优化Pass不断修补。



## Specification

需要了解LLVM IR的语义：

1. [LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html) 
2. [2019 EuroLLVM Developers’ Meeting: V. Bridgers & F. Piovezan “LLVM IR Tutorial - Phis, GEPs ...” - YouTube](https://www.youtube.com/watch?v=m8G_S5LwlTo) 

和WASM的语义：[Modules — WebAssembly 2.0 (Draft 2022-09-27)](https://webassembly.github.io/spec/core/binary/modules.html) 

1. 类型：i32 i64 对应LLVM中的i32 i64, f32 f64对应LLVM中的float double。
2. 每个wasm的Global值转为llvm中一个的global值。相关访问只有Load和Store指令。
3. 内存：转为一个global数组，u8 array。
   1. 内存访问：计算关于u8的偏移（get element ptr），然后再转为对应的类型指针load出来。即LLVM中`[大数字 x i8]`类型。因为只是分析，所有不用考虑内存增长的事情。
4. 函数
   1. 每个Local转化为函数开头的一个alloca。
   2. [非直接跳转 callind](https://webassembly.github.io/spec/core/exec/instructions.html#xref-syntax-instructions-syntax-instr-control-mathsf-call-indirect-x-y) 



## 栈的处理

参考栈验证逻辑。能保留的最好直接解码为SSA。





## 控制流处理

目前block，loop分别对应在结尾，开头，增加一个label。if对应一些label和br_if，br代表直接跳转，br_if同理。比较麻烦的是br_table。但是看了下和LLVM的switch语句对应得非常好啊。



## Wasm中的非直接跳转（复习）

在二进制模块中有id为4的table section。里面有一系列的table类型，初始化则由element section负责。table类型有两部分，reftype和limit。[limit](https://webassembly.github.io/spec/core/binary/types.html#limits)应该是类似数组大小的东西，但是同时包含了上限和下限。

reftype其实就是个enum，表示是不透明的external类型还是function类型。即，光是table section里，有用的信息只有定义了index，给每个index处的table标明了上下限。

接下来看elem section。~~它可以是passive的，即等着被table.init指令使用，用来初始化某个table。或者是active的，直接初始化某个table。~~ 现在直接翻标准是新release的2.0标准了。我们暂时先支持1.0标准。（这个version是在整个二进制模块的header处定义的。）elem section由三部分组成：

1. table index, 初始化哪个table。目前因为只有一个table，所以必须是0。
2. offset, 常量表达式，即一些指令。例如：`41 01 0b`解码为`i32.const 1; end;`。
3. vec(func ind) 一系列函数下标，表示要初始化成这些。

[Features to add after the MVP - WebAssembly 中文网|Wasm 中文文档](https://www.wasm.com.cn/docs/future-features/) https://www.w3.org/TR/wasm-core-1/#element-segments%E2%91%A0 （可以在这个页面搜索`at most one`） 这里提到了，MVP标准中wasm最多有一块内存，最多有一个table。

对应到LLVM IR的关键是，相同的语言特性会怎么在LLVM IR上实现/怎样的LLVM IR会编译到这样的wasm。LLVM里只有Call指令，但是参数是一个函数地址的value。目前看来可以搞一个函数指针数组，对应初始化后的table。然后将callind翻译为从函数指针中取，然后再call。



## TODO

最好能实现单个函数的反编译与混淆，即转换回Wasm时最好能保证其他部分不变。。。如果使用LLVM自己的wasm后端好像有点复杂

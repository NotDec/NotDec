wasm frontend 负责将WASM字节码转为LLVM IR



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



## TODO

最好能实现单个函数的反编译与混淆，即转换回Wasm时最好能保证其他部分不变。。。如果使用LLVM自己的wasm后端好像有点复杂
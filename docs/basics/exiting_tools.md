

1. WAMR wasm-micro-runtime
    1. 真的是自己写的字节码解析器好像。。。[wasm_loader.c](https://github.com/bytecodealliance/wasm-micro-runtime/blob/3220ff6941b64de684a5a60a5e3f8adad4a18fb0/core/iwasm/interpreter/wasm_loader.c) [wasm.h](https://github.com/bytecodealliance/wasm-micro-runtime/blob/3220ff6941b64de684a5a60a5e3f8adad4a18fb0/core/iwasm/interpreter/wasm.h)
    1. 有相关wasm到LLVM IR的转换可以参考：[aot_llvm_extra.cpp](https://github.com/bytecodealliance/wasm-micro-runtime/blob/c07584400134bb5f1be80b4f5df96eb1d8c94324/core/iwasm/compilation/aot_llvm_extra.cpp)

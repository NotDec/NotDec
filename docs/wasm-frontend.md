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

- 首先由于最后都是转IR，所以BaseContext保存LLVM相关的Context。其实可以作为全局变量，为了以后可能的并行，把这类全局变量都搞到一个类里。
- wasm::Context是相关生成的代码依附的数据结构，保存比如wabt::Module这种Context。为了方便应用，增加了对BaseContext的引用，对llvmCtx的引用等等。

## Specification

需要了解LLVM IR的语义：

1. [LLVM Language Reference Manual](https://llvm.org/docs/LangRef.html) 
2. [2019 EuroLLVM Developers’ Meeting: V. Bridgers & F. Piovezan “LLVM IR Tutorial - Phis, GEPs ...” - YouTube](https://www.youtube.com/watch?v=m8G_S5LwlTo) 

和WASM的语义：[Modules — WebAssembly 2.0 (Draft 2022-09-27)](https://webassembly.github.io/spec/core/binary/modules.html) 注意现在直接翻标准是新release的2.0标准了。**我们暂时先支持1.0标准**，wabt现在也仅支持1.0，如果文件头里写version为2会报错。1.0的标准可以看[这里](https://www.w3.org/TR/wasm-core-1/#syntax-importdesc)。
不确定每个指令的语义，看[这种地方](https://developer.mozilla.org/en-US/docs/WebAssembly/Reference/Numeric/Negate)。

1. 名字比较难处理，wasm的[name section](https://github.com/WebAssembly/extended-name-section/blob/main/proposals/extended-name-section/Overview.md)允许重名，而且wasm中因为是二进制格式，理论上名字可以取任意utf-8。那边wat格式的定义也有类似的问题。但是wabt似乎已经处理了相关的问题？
   - 在src\binary-reader-ir.cc里的BinaryReaderIR::GetUniqueName函数，如果重名了会加数字后缀。
1. 类型：i32 i64 对应LLVM中的i32 i64, f32 f64对应LLVM中的float double。
1. 每个wasm的Global值转为llvm中一个的global值。相关访问只有Load和Store指令。
   1. 名字直接使用wabt那边传过来的名字 ~~名字更改为`global_<ind>_<original_name>`这种格式，即在原来名字前加上前缀标识。~~
   1. [Linkage Types](https://llvm.org/docs/LangRef.html#linkage-types) 选择internal。被导出的更改为external。
   1. 根据mutable，设置llvm那边的const属性
   1. 处理init_expr
3. 内存：转为一个global数组，u8 array。
   1. 内存初始化：似乎LLVM IR里一个数组不能部分初始化。很多0也没办法。就这样吧
   1. 内存访问：计算关于u8的偏移（get element ptr），然后再转为对应的类型指针load出来。即LLVM中`[大数字 x i8]`类型。因为只是分析，所有不用考虑内存增长的事情。
4. 函数
   1. 每个Local转化为函数开头的一个alloca。
   2. [非直接跳转 callind](https://webassembly.github.io/spec/core/exec/instructions.html#xref-syntax-instructions-syntax-instr-control-mathsf-call-indirect-x-y) 

## 指令、栈、控制流的处理

参考WAVM，见顶部现有工具一节。参考栈验证逻辑。能保留的最好直接解码为SSA。这里的block直接考虑[Multi Value Extension](https://github.com/WebAssembly/multi-value)，防止以后架构需要重构，但是函数返回多个值的先不支持。

- 每个栈上元素对应一个SSA的Value。某种形式上可以维护一个Value栈（作为局部变量，不需要作为Context）。
- 控制流跳转维护一个block的嵌套栈，保存br时跳转的目标。关键是如何在找到跳转目标的同时，把栈弹到对应的值。
- 处理Block的时候，这里用递归和用栈都可以。选择用实现起来更简单的递归。aWsm好像是递归的写法，WAVM好像是用栈，复杂一点。
- loop和block的区别在于，给phi赋值，然后用Phi替换栈上值的地方不同。一个是基本块开头，一个是基本块结尾
- 函数体大致也算一个Block块，但是labelType写Func。

### 控制流指令的处理，与SSA生成

visitFunction：
1. 创建allocaBlock，分配参数和local空间

1. 创建alloca -> entry边，创建return块备用
1. 调用visitBlock函数（visitBlock函数必须把所有的结束跳转都引导到exit块）
1. 创建结尾的return指令。（visitBlock内部处理的时候只有br，return也看作特殊的br，函数只允许在结尾返回）

递归的基本块生成算法visitBlock：
**要求与保证**：
1. 要求算法的整体表现类似于给定类型的单个指令
1. 要求调用者提供的entry和exit中，需要创建Phi的那个为空基本块（但是可以有Phi指令），便于创建Phi节点。
1. 保证结束的时候跳转到exit块。不会有其他控制流。

注：
1. 没必要再用一个额外的stack防止访问更深元素，因为调用过了wabt的validate

调用visitBlock前，根据block类型
1. Block类型：创建新的exit块，替换原来exit，处理完毕后新的exit作为entry继续生成指令，旧的exit还是exit
1. Loop类型：创建新的entry块，替换原来entry。loop结束也一样。

visitBlock：
1. 先创建好跳转目标，Phi节点，用这些设置好BreakoutTarget结构体，压入栈中：
   1. 根据是loop还是block类型的块创建Phi。如果是Loop，直接把当前栈上的值弹出来，作为phi的operand，然后把phi push回去，替换。如果是block，先创建空的Phi。（等后面结束的时候再从栈上加operand。
   1. 保存当前value栈的情况。
1. 依次遍历每个指令生成。
   1. 普通的指令根据指令语义，从value stack中取值，
   1. 如果遇到了跳转指令：
      1. 如果跳转的目标是普通基本块，则从栈上取值加入到对应的Phi中，
1. 块结束的时候，不需要主动跳转到exit，因为exit不一定是当前block的exit，因为在Loop的情况下，结尾没有额外创建基本块，所以不需要特殊处理。Block结尾的跳转交给外面处理。
1. Block结束时，在end前，处理隐含跳转到结束块。由于类型检查，不会有多余的值，不需要unwinding弹出栈。

控制流指令的处理：

1. br指令，其后是stack可以是任何类型。为了处理这种情况，我们直接增加unreachable标识，无视这些指令。
   - 对于Block
   - 对于Loop，由于结尾是从Loop离开的唯一方式。如果有br指令封锁了结尾，则不可能从这个loop结尾离开了。此时直接保留UnReachableState
1. block，loop分别对应在结尾，开头，增加一个label。注意到block只需要为return的值创建Phi，loop需要对参数创建Phi。
1. if对应一些label和br_if，br代表直接跳转，br_if同理，根据语义找到对应的跳转目标，生成条件跳转即可。
1. br_table看似比较麻烦，看了下和LLVM的switch语句对应得非常好啊。也是根据不同的值跳转到不同的边。

最开始的时候先写一个类型检查，打印出每个指令后当前栈上的类型情况的代码，然后再加生成相关的东西。

wabt那边代表Block的结构体看wabt的`src\ir.h` 383行`struct Block`这边。
- `std::string label` 直接放到BasicBlock的名字里面
- `BlockDeclaration decl` 和`FuncDeclaration`是一个类型
- `ExprList exprs`
- `Location end_loc` 代表输入文件里的位置，暂时不管，除非后面想加debug信息

多个参数和返回值的时候，顺序：
- 函数参数逆序遍历（pop），同时从栈上pop出来。
- 函数返回值顺序遍历，同时push到栈上。

查OpCode看`wabt/src/opcode.def`。Opcode和ExprType之间的关系看`src\lexer-keywords.txt`，或者看`src\binary-reader.cc`里面对应的Opcode创建了什么Expr，~~或者看`src\binary-reader-ir.cc`里找对应的指令到底创建了哪种Expr类。~~

这里面的类继承关系看 `src\ir.h`。其实就是搞了一个ExprType，然后在onXXX指令的函数处直接创建了这个类型的Expr，导致opcode和expr之间没有明确的对应关系。

## 运算指令的处理

- 简单的可以对着这个找指令https://llvm.org/docs/LangRef.html。
- 可以找llvm intrinsic，例如fabs指令使用了对应的`Intrinsic::fabs`
- 更复杂的可以自己手写llvm函数，然后直接调自己写的函数，之后看看要不要内联什么的

资料：
- WAMR里，intrinsic的实现 https://github.com/bytecodealliance/wasm-micro-runtime/blob/d309091706f2fbfc3ccca2907226f57db4d612f3/core/iwasm/aot/aot_intrinsic.c
- WAVM里，intrinsic的实现（使用irBuilder） https://github.com/WAVM/WAVM/blob/79c3aa29366615d9b1593cd527e5b4b94cc6072a/Lib/LLVMJIT/EmitNumeric.cpp

### 比较 - 浮点数

参照https://www.w3.org/TR/wasm-core-1/#-hrefop-feqmathrmfeq_n-z_1-z_2 和https://llvm.org/docs/LangRef.html#id309 对比语义

1. feq在wasm中，如果有nan就返回0，反过来只有无nan才返回true，所以采用`fcmp oeq`。
1. 而fne，有nan就返回1，所以要用`fcmp une`




### 链接

-  WebAssembly Object File Linking： https://github.com/WebAssembly/tool-conventions/blob/main/Linking.md
-  Adventures in WebAssembly object files and linking： https://mike.zwobble.org/2021/04/adventures-in-webassembly-object-files-and-linking/

相关section的解析可以看`src\binary-reader.cc`里的`BinaryReader::ReadCustomSection`函数。


## 测试

1. 基于sysY语言的测试用例，自动编译为wasm和wat格式，反编译到IR后和sylib.c得到sylib.ll一起输入lli执行。验证输出的正确性。
   - 使用-c编译为未链接的object ？
      - 缺点1：内存是导入的，大小不确定
      - 缺点2：需要处理额外的。
   - 编译为完整模块，加上`-g -O0 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined`等选项。全部导出可以不用特殊处理main函数的导出，allow undefined好像会让没定义的都变成导入。
      - 目前暂时的方案。


## 其他

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

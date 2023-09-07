# Ghidra

## 资源

其他资源列表：
- [Ghidra - low level](https://low-level.readthedocs.io/en/latest/reversing/ghidra/) 这里有很多其他资源，dump local variables的脚本。和搭建调试环境的issue链接。
- [Awesome Ghidra](https://github.com/AllsafeCyberSecurity/awesome-ghidra)

书籍：

- 《THE GHIDRA BOOK The Definitive Guide》 TheGhidraBook.pdf
- 《Ghidra Software Reverse Engineering for Beginners》 有pdf

使用教程：

- Ghidra API的tutorial：https://github.com/HackOvert/GhidraSnippets
- Dump出high-pcode的教程 https://github.com/HackOvert/GhidraSnippets#dumping-refined-pcode 
- 基于Ghidra实现的VSA：https://github.com/penhoi/ghidra-decompiler/wiki/Symbolic-Value-Set-Analysis 
- 如何开发调试Ghidra：https://spinsel.dev/2021/04/02/ghidra-decompiler-debugging.html 这人也

代码分析：

- [《Ghidra To The Next Level》](https://conference.hitb.org/hitbsecconf2021sin/materials/D1T2%20-%20Taking%20Ghidra%20to%20The%20Next%20Level%20-%20Zhanzhao%20Ding.pdf) 丁湛钊的介绍 这个slides讲了不少ghidra的反编译器的设计。
- [《记一次对Ghidra反编译的修复》](https://www.freebuf.com/articles/network/278014.html)解释了反编译原理，里面解决的问题其实就是栈变量识别相关的 
- [DevGuide.md](https://github.com/NationalSecurityAgency/ghidra/blob/master/DevGuide.md) 官方的eclipse环境搭建
- [《Exploring Ghidra’s decompiler internals to make automatic P-Code analysis scripts》](https://research.nccgroup.com/2022/05/20/earlyremoval-in-the-conservatory-with-the-wrench/)解释了一些decomp_dbg的代码。

### Ghidra-sleigh 调试环境搭建

相关的C/C++代码主要在 Ghidra/Features/Decompiler/src/decompile.cpp下。docmain.hh和 doccore.hh两个文件可以先看，或者先build doc出来，生成的doc在`../doc`目录（Ghidra/Features/Decompiler/src/decompile/doc/html/index.html）

Ghidra-sleigh的[ghidra/docmain.hh](https://github.com/NationalSecurityAgency/ghidra/blob/master/Ghidra/Features/Decompiler/src/decompile/cpp/docmain.hh) 里有很多文档，看代码前构建doxygen看文档。

根据这里https://daniao.ws/notes/quick-tips/build-ghidra 下载并编译Ghidra。生成各种中间文件

```Bash
sudo apt install openjdk-17-jdk-headless unzip --no-install-recommends
wget -c https://services.gradle.org/distributions/gradle-8.1.1-bin.zip -P /tmp
sudo unzip -d /opt/gradle /tmp/gradle-8.1.1-bin.zip
export PATH=$PATH:/opt/gradle/gradle-8.1.1/bin
echo 'export PATH=$PATH:/opt/gradle/gradle-8.1.1/bin' >> ~/.bashrc

git clone https://github.com/NationalSecurityAgency/ghidra.git
git checkout Ghidra_10.3.1_build
git checkout -b mydev
gradle --init-script gradle/support/fetchDependencies.gradle init
gradle buildGhidra
```

然后vscode打开文件夹`/home/ubuntu/ghidra/Ghidra/Features/Decompiler/src/decompile`

使用bear生成compile_commands.json文件

```Bash
bear -- make decomp_dbg
```

增加debug配置。注意里面的环境变量指向ghidra源码文件夹

```JSON
{
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lldb",
            "request": "launch",
            "name": "Debug",
            "program": "${workspaceFolder}/cpp/decomp_dbg",
            "args": [],
            "cwd": "${workspaceFolder}/datatests",
            "env": {"SLEIGHHOME": "/home/ubuntu/ghidra"}
        }
    ]
}
```

从ghidra图形界面，反编译的右上角的debug decompiler里面可以导出xml文件，用于调试。

### 调试与decomp_dbg常用命令

https://github.com/NationalSecurityAgency/ghidra/issues/720

```JSON
load file test 加载二进制文件
load addr 0x1149
decompile
print C

restore Reset.xml    加载xml文件
trace address
trace list
```

搜索相关的命令：`registerCommands` `registerCom` 

部分命令需要`OPACTION_DEBUG`开启

## 反编译器代码解读（sleigh）

**总体分析流程**

Ghidra的Java部分代码启动sleigh子进程，然后通过stdin输入xml文件，stdout读取xml文件作为反编译结果。比如Ghidra\Features\Decompiler\src\main\java\ghidra\app\decompiler\DecompileResults.java parseRawString函数这里开始解析反编译器的输出的。Ghidra\Framework\SoftwareModeling\src\main\java\ghidra\program\model\pcode\HighFunction.java readXML函数里可以看到high pcode是基于这个ast的标签解析的。

反编译输出展示上面的选项里，选Debug Function Decompilation导出的xml文件是反编译器的输入。根据https://github.com/NationalSecurityAgency/ghidra/issues/720 这里可以使用decomp_dbg命令行输入xml文件进行反编译。

应该反编译过程中，Pcode都是一套Pcode，只不过https://spinsel.dev/assets/2020-06-17-ghidra-brainfuck-processor-1/ghidra_docs/language_spec/html/additionalpcode.html 有一些额外的东西只有在反编译之后才会产生。

**反编译阶段**

首先反编译过程被分为了几个大的 `simplification styles` `Root Action Groups`。它们由 `base groups` （例如 “stackvars” or “typerecovery”） 组成。

- decompile – The main decompiler action
- normalize – Decompilation tuned for normalization
- jumptable – Simplify just enough to recover a jump-table
- paramid – Simplify enough to recover function parameters
- register – Perform one analysis pass on registers, without stack variables
- firstpass – Construct the initial raw syntax tree, with no simplification

在[universalAction](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_10.1.2_build/Ghidra/Features/Decompiler/src/decompile/cpp/coreaction.cc#L5021-L5275)函数里构建了所有可能的Action和Rule列表，即定义了他们的执行顺序。每个Action或Rule是否运行再由标签确定。

[Rule](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_10.3.1_build/Ghidra/Features/Decompiler/src/decompile/cpp/action.hh#L194): 代表着某个特定的点位可能采取的操作。它输入一个Pcode位置，首先判断能不能在该处应用，然后再应用更改。Rule可以通过`getOpList()`函数先给出一个点位可能的Opcode列表，方便外部提前过滤。多个Rule可以组合为`ActionPool`

[Action](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_10.3.1_build/Ghidra/Features/Decompiler/src/decompile/cpp/action.hh#L52) 类似Pass，对一个函数做变换。每次变换递增一下count变量。

和栈分析相关的标签：`localrecovery` `typerecovery` `stackvars`

和栈分析相关的标签，和内部的Action和Rule：

`base`
- ActionHeritage SSA构建算法。

`localrecovery`
    - ActionRestrictLocal: 限制局部变量在栈上的可能范围，排除一些参数之类的栈范围，使得这些范围不会创建局部变量。
        - 首先对每个FunctionCallSpecs，的每一个参数，把它们的地址排除在局部变量之外。
        - 对保存的caller的寄存器，相关的栈内存排除在局部变量之外。
        - 注：可见wasm里不用管这些。
    - ActionRestructureVarnode
        - gatherVarnodes：从varnode中收集栈上值的类型信息。它遍历funcdata在stack这个space上的Varnode，然后在对应的栈偏移处增加当前varnode的类型信息，表示为增加一个RangeHint
        - gatherOpen：先调用 AliasChecker::gather，收集一个AddBase的list，包含所有的栈相关指针，然后对每个指针调用gatherOffset, 收集偏移。最后会设置一个aliasBoundary为最小的offset，可能比它大就可能alias。
            - AliasChecker::gather：对于给定的函数和address space（栈），收集所有，指向当前地址空间的，指针（varnode）。
                - AliasChecker::gatherAdditiveBase：首先从栈指针开始，收集所有和它有关的加法操作（INT_ADD, INT_SUB, PTRADD, PTRSUB, and SEGMENTOP）。广度优先地遍历，vnqueue作为worklist。最后生成了AddBase的一个list，表示每个有关的root varnode和可能存在的index varnode。
            - gatherOffset：对每个收集的AddBase的base调用，负责计算出varnode的常量offset
        - gatherSymbols：
        - ScopeLocal::restructure：

`stackvars`
    - RuleLoadVarnode
    - RuleStoreVarnode

`stackptrflow`
    - ActionStackPtrFlow
        - `checkClog`: Clog是指，栈指针加栈上的值？
        - `analyzeExtraPop`: sub-functions TODO，难道是说子函数里面用父函数栈指针？


decomp_dbg 代码位于consolemain.cc（main 函数，也定义了 load/save/restore几条命令）、ifacedecomp.hh/cc （反编译相关命令）。

- Heritage::discoverIndexedStackPointers

通过一通搜索，我找到了restructure varnode这个action，发现了一些问题。这个action的主要工作就是调用 ScopeLocal::restructureVarnode函数（varmap.hh/cc)，这个函数的工作分配给了 MapState::gatherVarnodes 、MapState::gatherOpen 和 MapState::gatherSymbols ，最后调用了ScopeLocal::restructure。

其中有关的部分在gatherOpen和gatherVarnodes。gatherOpen在正常情况下，应该分析出好几段栈空间对应的 open RangeHint ，其中的处理逻辑在AliasChecker::gather -> AliasChecker::gatherInternal -> AliasChecker::gatherAdditiveBase 。

MapState::gatherVarnodes

所以，我将范围扩大到在heritage 、restructureVarnodes action之后，然后找到了相关的action ：RuleLoadVarnode和 RuleStoreVarnode。

这两个action（其实是Rule）应该就是分析的主要了，他们都会经过RuleLoadVarnode::checkSpacebase -> RuleLoadVarnode::vnSpacebase -> RuleLoadVarnode::correctSpacebase (ruleaction.cc) 的检查。调试发现，没能分析出来的变量所对应的 store/load在correctSpacebase的检查中失败了 。

这两个action（其实是Rule）应该就是分析的主要了，他们都会经过RuleLoadVarnode::checkSpacebase -> RuleLoadVarnode::vnSpacebase -> RuleLoadVarnode::correctSpacebase (ruleaction.cc) 的检查。调试发现，没能分析出来的变量所对应的 store/load在correctSpacebase的检查中失败了。

 ActionDatabase::buildDefaultGroups 分析好像是分组的，里面很多rule。

 “simplification styles” are also referred to as “root actions” or “groups” in the decompiler source code. They consist of groups of “base groups” such as “stackvars” or “typerecovery”, which are more fine-grained groups of specific analysis operations.

### Ghidra 中的 value set analysis

- [`ValueSetSolver`](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_10.3.1_build/Ghidra/Features/Decompiler/src/decompile/cpp/rangeutil.hh#L274) VSA分析。好像会先执行`vsSolver.establishValueSets`给要分析的值赋初始值，然后调用`vsSolver.solve`运算。
    - `CircleRange`: 底层抽象域，针对不同opcode写了运算，例如`CircleRange::pushForwardBinary`负责binary op
    - 使用方面，有两个使用点：一个`IfcAnalyzeRange`好像是执行`analyze range full|partial <varnode>`命令的时候触发。另外一个是`ValueSetSolver.analyzeNewLoadGuards`函数，为LoadGuard确认访问范围。
- `LoadGuard`: 对一个，访问了栈，但是是在动态访问栈的load指令的描述。描述其可能访问的栈范围。包括min, max, step等。和VSA分析有很大关系。




## 编写插件

### Ghidra scripting

- 命令行直接加载二进制文件，可以设置分析后是否保存
    https://static.grumpycoder.net/pixel/support/analyzeHeadlessREADME.html#scripting_headlessScripts_controlProgramDisposition 
- 如何遍历定义的结构体成员
    https://reverseengineering.stackexchange.com/questions/21320/automate-looking-for-calls-to-an-offset-of-a-structure
    
    `structure.getDefinedComponents` `Ghidra\Framework\SoftwareModeling\src\main\java\ghidra\program\model\data\CompositeInternal.java` 的dumpComponents方法是结构体转string的时候用到的。

- 获取创建符号
    getSymbols(name, namespace)

- 获取和创建函数
    FlatAPI的getFunction系列。按照namespace获取函数是在listing里：getCurrentProgram().getListing()
    
    Flatapi的createFunction。更多创建函数在FunctionManager：getCurrentProgram().getFunctionManager().createFunction
    
    getCurrentProgram().getExternalManager().addExtFunction好像同时会创建External Location和对应的函数？


- 指定地址寻找函数
    看了下flat program api里面对getFirstFunction和getFunctionAfter的实现，发现就是currentProgram.getListing().getFunctions函数的简单调用。而且getFunctionAfter还有传入地址和函数的版本。


- 设置函数签名
    FunctionSignatureParser负责的就是修改函数的时候，上面那个输入框的解析。
    
    FunctionEditorModel这个类负责这个输入框背后的逻辑：调用上述parser，得到FunctionDefinitionDataType，然后调用自身的setFunctionData函数。函数内部设置函数的参数和返回值。

- 偏移找寄存器
    `\ghidra_10.1.2_PUBLIC\Ghidra\Processors\ARM\data\languages\ARM.sinc`看这个文件

- 设置/读取寄存器值
    https://ghidra.re/ghidra_docs/api/ghidra/program/model/listing/ProgramContext.html 
    
    program.getProgramContext().setValue(register, start, end, value); 一般可以start=end=想要的地址

    Ghidra\Features\Base\src\main\java\ghidra\app\util\viewer\field\RegisterFieldFactory.java getRegisterStrings好像是负责写assume xxx = xxx的。
    
    Ghidra\Features\Base\src\main\java\ghidra\app\util\viewer\field\RegisterFieldFactory.java getSetRegisters负责获取设置的寄存器

- 加载头文件报错的常见解决方法
    在parse C source界面之所以会标红，应该是要自己在下面的-I选项给出这些头文件存在的位置。比如我的-IC:\Program Files\LLVM\lib\clang\13.0.0\include
    
    -IC:\Program Files (x86)\Dev-Cpp\MinGW64\lib\gcc\x86_64-w64-mingw32\4.9.2\include

    目前用到size_t的还真的不知道怎么办。上面两个头文件处理后都不太行，变成typedef __SIZE_TYPE__ size_t;这种，而且完全看不到int32_t的定义。所以windows下的头文件不太行。还是得看看。最后从wsl里复制header出来，在x86_64-linux-gnu\bits\types.h这里定义了。但是我include好像没用，不知道为什么

    TODO：暂时放弃，直接使用types里面复制处理的。

    ```c
    #define __stdcall
    struct va_list; // shitty hack
    typedef struct va_list *va_list;

    typedef long int ptrdiff_t;
    typedef long unsigned int size_t;
    ```

    有些可以直接用-D参数的方式解决。-D甚至可以定义函数式宏：https://stackoverflow.com/questions/31857559/gcc-define-function-like-macros-using-d-argument 
    
    -D__attribute__(x)= 好像没有用，但是ghidra好像能直接识别，不会报这个的错

    ```c
    -Dsize_t="unsigned long"

    /* Fixed-size types, underlying types depend on word size and compiler.  */
    typedef signed char __int8_t;
    typedef unsigned char __uint8_t;
    typedef signed short int __int16_t;
    typedef unsigned short int __uint16_t;
    typedef signed int __int32_t;
    typedef unsigned int __uint32_t;
    #if __WORDSIZE == 64
    typedef signed long int __int64_t;
    typedef unsigned long int __uint64_t;
    #else
    __extension__ typedef signed long long int __int64_t;
    __extension__ typedef unsigned long long int __uint64_t;
    #endif
    ```

#### MemoryBlock

原来没有直接的方法，是通过创建新Block然后调用join方法。

`Ghidra\Features\Base\src\main\java\ghidra\app\plugin\core\memory\ExpandBlockModel.java` applyTo函数

- 读写内存区域
    Uninitialized的内存默认是不能写的，需要把要写的部分分割开，然后设置成initialized。
    
    https://github.com/NationalSecurityAgency/ghidra/issues/3585 


- 动态链接与Thunk Function
    
    ThunkFunction corresponds to a fragment of code which simply passes control to a destination function. All Function behaviors are mapped through to the current destination function. 另外thunk不需要设置参数和返回值类型，直接复制的目标对象的类型。
    
    主要的逻辑在Ghidra\Features\Base\src\main\java\ghidra\app\util\opinion\ElfProgramBuilder.java这里。当加载程序的时候，它会处理各种符号。单是创建单独的external符号是在“<EXTERNAL>”（Library.UNKNOWN）这个库下的，默认没有map到内存中，所以getExternalSpaceAddress这里是一种Fake的地址。而内存中的“EXTERNAL”（MemoryBlock.EXTERNAL_BLOCK_NAME）是ElfProgramBuilder创建的。（通过搜索NOTE: This block is artificial and is used to make relocations work correctly找到的）

    为什么他创建的thunk方法可以不在listring的Function里显示，秘诀大概是：getCurrentProgram().getSymbolTable().removeSymbolSpecial。参照下面文件里搜索removeSymbolSpecial的代码。

    分配单独的一块内存创建EXTERNAL块相关的逻辑在Ghidra\Features\Base\src\main\java\ghidra\app\util\opinion\ElfProgramBuilder.java allocateLinkageBlock函数。


### 查看high p-code

https://reverseengineering.stackexchange.com/questions/29646/dump-pcode-in-ghidra-for-a-specific-decompiled-function 直接在界面点就可以，但是好像只是为了给你看形状的。打开python然后currentLocation.token.pcodeOp也可以稍微看看当前指针位置的。

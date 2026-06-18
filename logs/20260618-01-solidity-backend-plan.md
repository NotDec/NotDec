# Solidity 后端技术路线规划

## 原始 prompt

现在经过了logs/20260614-03-evm-high-level-type-pass-plan.md，还有之前的一些plan，总之就是已经完成了类型推理，高层类型的识别暂时也还可以了，下一步最好是直接对着数据集，apehex数据集那边有源码的部分去改进。但是目前链路还是没那么完整，下一步要规划solidity代码生成。参考那边solidity compiler solc源码，或者网上的其他的类似的库，或者LLVM2c仓库（那边是抄的LLVM的，C 的AST打印的代码）。规划一下solidity后端的技术路线，以及代码放在什么位置合适

## 背景

当前 EVM 链路已经有三块基础：

- evm2llvm 把字节码提到 LLVM IR。
- 类型恢复已经能给出 HType，并且 EVM runtime helper 也有 address、storage_key、integer 这类语义类型。
- Solidity pattern pass 已经能重写一部分 ABI return、revert、event、storage 高层访问。

下一步不应该继续只在 IR 里加 helper。数据集里有源码的 apehex 样本更适合用来做输出侧对齐：拿源码作参考，先让生成的 Solidity 更像真实源码，再反过来发现前面哪些语义还缺。

这里的目标先定成“可读 Solidity-like 输出”，不是“重新编译后字节码等价”。如果一开始追求可编译，会被 modifier、inheritance、library dispatch、optimizer 形状、source map 这些问题拖住。

## 可参考的代码

### solc

本地源码在 `/sn640/solidity`。能参考的是规则和类型形状，不建议直接复用或复制实现。

重点看：

- `/sn640/solidity/libsolidity/ast/AST.h`
  - `SourceUnit`、`ContractDefinition`、`FunctionDefinition`、`VariableDeclaration`、statement/expression 节点。
- `/sn640/solidity/libsolidity/ast/Types.h`
  - `IntegerType`、`AddressType`、`ArrayType`、`StructType`、`MappingType`、`FunctionType`、`DataLocation`。
- `/sn640/solidity/libsolidity/interface/ABI.*`
  - ABI selector、参数、返回值规则。
- `/sn640/solidity/libsolidity/interface/StorageLayout.*`
  - storage layout 输出规则。
- `/sn640/solidity/libsolidity/codegen/ABIFunctions.*`、`ArrayUtils.*`、`YulUtilFunctions.*`
  - 编译器怎么处理 ABI、array、bytes/string。

solc 不是一个合适的直接后端依赖：

- 它是编译器前端和 codegen，不是从任意 IR 反建源码的库。
- Solidity AST 构造需要很多语义分析 annotation，不适合我们手工拼。
- 许可证也要谨慎，先只参考行为，不抄代码。

Yul 有 `libyul/AsmPrinter.*`，但它打印的是 Yul，不解决 Solidity 源码输出。

### LLVM2C

`external/NotDec-llvm2c` 值得借架构，不建议复用 Clang AST：

- `StructuralAnalysis.cpp`：LLVM CFG 到结构化语句。
- `TypeManager.cpp`：HType 到 C type/field 的映射。
- `ASTPrinter/*`：AST 打印和括号、优先级、声明打印。

原先判断是 Solidity 输出先不塞进 llvm2c 子模块。现在路线调整：先把这个子模块整理成通用 backend 仓库，再在里面实现 Solidity 后端。

这个想法可行，但前提是不能只把 Solidity 代码直接放进当前 `notdec-llvm2c` 目录。当前子模块还是 C 后端形状：

- CMake project、library、tool 都叫 `notdec-llvm2c`。
- public namespace 是 `notdec::llvm2c`。
- `Interface.h` 直接 include Clang AST 头文件。
- `StructuralAnalysis`、`TypeManager`、`ASTPrinter` 都直接构造 C / Clang AST。
- 但 `Interface/ExtValuePtr.*`、`Interface/HType.*`、`Interface/StructManager.*` 这类东西已经接近通用 backend core。

所以整理目标应该是：把 `external/NotDec-llvm2c` 逐步变成 `NotDec-backends` 语义的仓库，内部包含 shared core、C backend、Solidity backend。仓库物理名字可以晚点改，先把目录、target、namespace 边界整理清楚。

## Backend 仓库整理路线

补充 prompt：

> 考虑要不后端都放在llvm2c那边，让那个仓库后续作为所有backend的代码存放的位置，不局限于c backend。这个想法怎么样？
>
> 直接先整理backend仓库，然后在里面进一步实现solidity吧，先规划一下backend仓库怎么整理合适，方便接入多语言backend

建议目标目录：

```text
external/NotDec-llvm2c/
  include/notdec-backends/
    Core/
      BackendOptions.h
      BackendResult.h
      ExtValuePtr.h
      HType.h
      HTypeResult.h
      ValueNamer.h
      StructManager.h
    Structuring/
      StructuredCFG.h
      Structurer.h
      GotoStructurer.h
      PhoenixStructurer.h
    C/
      CBackend.h
      CASTManager.h
      CTypeManager.h
      CStructuralAnalysis.h
      CPrinter.h
    Solidity/
      SolidityBackend.h
      SolidityAst.h
      SolidityTypePrinter.h
      SolidityPrinter.h
  lib/
    Core/
    Structuring/
    C/
    Solidity/
  tools/
    notdec-llvm2c/
    notdec-llvm2sol/
```

CMake target 建议分层：

```text
notdec-backend-core
notdec-backend-structuring
notdec-backend-c
notdec-backend-solidity
notdec-llvm2c-exe
notdec-llvm2sol-exe
```

依赖方向要固定：

```text
Core
  -> LLVM support/IR，尽量不依赖 Clang

Structuring
  -> Core + LLVM analysis
  -> 不依赖 Clang
  -> 不依赖 Solidity

C backend
  -> Core + Structuring + Clang

Solidity backend
  -> Core + Structuring
  -> 不依赖 Clang
```

短期为了少动，可以保留兼容 target：

```text
notdec-llvm2c -> alias 或 wrapper 到 notdec-backend-c
include/notdec-llvm2c/Interface.h -> 兼容转发头
namespace notdec::llvm2c -> 先保留，内部逐步转到 notdec::backend::c
```

第一阶段不要强行改仓库名。先把 CMake target 和目录分开，主仓库仍然可以继续引用 `external/NotDec-llvm2c`。等引用都切干净后，再考虑把子模块仓库改名成 `NotDec-backends`。

### 第一阶段：拆 Core

先移动或复制再清理这些比较通用的文件：

```text
include/notdec-llvm2c/Interface/ExtValuePtr.h
include/notdec-llvm2c/Interface/HType.h
include/notdec-llvm2c/Interface/Range.h
include/notdec-llvm2c/Interface/StructManager.h
include/notdec-llvm2c/Interface/ValueNamer.h

lib/notdec-llvm2c/Interface/ExtValuePtr.cpp
lib/notdec-llvm2c/Interface/HType.cpp
lib/notdec-llvm2c/Interface/StructManager.cpp
lib/notdec-llvm2c/Interface/ValueNamer.cpp
lib/notdec-llvm2c/Range.cpp
```

目标是生成 `notdec-backend-core`。

这里有一个坑：`HType.h` 当前还有 `clang::Decl *ASTDecl` 之类字段。第一阶段不要大改 HType 数据模型，可以先把这些字段保留但隔离成“C backend annotation”，或者先把 Clang include 从公共 core 里移到实现/adapter 层。真正目标是 core 不要求使用者链接 Clang。

`HTypeResult` 也应该从现在的 `Interface.h` 里拆出来，放到：

```text
include/notdec-backends/Core/HTypeResult.h
```

主项目的类型恢复、EVM pattern pass、C/Solidity 后端都应该只依赖这个 core 头。

### 第二阶段：拆 C backend

把当前 C 专属内容收到 `C/` 下面：

```text
ASTManager
ASTPrinter/*
TypeManager
StructuralAnalysis
Utils 中 Clang AST 构造相关部分
tools/notdec-llvm2c
```

`decompileModule()` 这一类 C 后端入口改成：

```cpp
namespace notdec::backend::c {
void decompileModule(...);
}
```

同时保留旧接口：

```cpp
namespace notdec::llvm2c {
inline void decompileModule(...) {
  notdec::backend::c::decompileModule(...);
}
}
```

这样主仓库可以分步迁移，不需要一次改完所有 include。

### 第三阶段：拆 Structuring

当前 llvm2c 的结构恢复和 Clang AST 绑得太紧，不能直接给 Solidity 用。要抽出语言无关结构：

```text
Structuring/StructuredCFG
Structuring/StructuredNode
Structuring/Structurer
Structuring/GotoStructurer
Structuring/PhoenixStructurer
```

第一版先实现 `GotoStructurer`，保证任意 CFG 都有输出。Phoenix 可以晚点迁。

C 后端短期可以继续用旧 `StructuralAnalysis`，Solidity 后端先用新 `Structuring`。等新结构恢复稳定后，再把 C 后端切到新接口。

这样做比一上来把 Phoenix 拆干净更稳。

### 第四阶段：加 Solidity backend

Solidity 后端放：

```text
include/notdec-backends/Solidity/
lib/Solidity/
```

入口：

```cpp
namespace notdec::backend::solidity {
struct Options {
  std::string workDir;
  bool emitCommentsForUnknown = true;
};

void decompileModule(llvm::Module &M,
                     llvm::ModuleAnalysisManager &MAM,
                     llvm::raw_ostream &OS,
                     const Options &Opts,
                     std::shared_ptr<notdec::backend::core::HTypeResult> HT);
}
```

它只依赖：

- LLVM IR
- backend core 的 HTypeResult / ExtValuePtr
- backend structuring 的结构树
- EVM pattern pass 在 IR 里留下的 helper / metadata

不要依赖 Clang，也不要复用 C ASTPrinter。

### 主仓库接入方式

主仓库里 `PassManager` 后续应该同时支持：

```text
.c   -> notdec::backend::c::decompileModule
.sol -> notdec::backend::solidity::decompileModule
```

`src/CMakeLists.txt` / 顶层 CMake 需要从只链接 `notdec-llvm2c` 改成按需链接：

```text
notdec-backend-core
notdec-backend-c
notdec-backend-solidity
```

为了减少破坏，第一步可以让 `notdec-llvm2c` target 继续存在，并让它 link 到 `notdec-backend-c`。等主仓库改完再删旧名。

## 放置位置

路线调整后，Solidity 后端不放主仓库，而是放 backend 子模块。主仓库只保留 pass pipeline 接入和 EVM 语义 rewrite。

```text
external/NotDec-llvm2c/include/notdec-backends/Solidity/
external/NotDec-llvm2c/lib/Solidity/
```

第一版文件大概是：

```text
external/NotDec-llvm2c/include/notdec-backends/Solidity/SolidityBackend.h
external/NotDec-llvm2c/include/notdec-backends/Solidity/SolidityAst.h
external/NotDec-llvm2c/include/notdec-backends/Solidity/SolidityTypePrinter.h
external/NotDec-llvm2c/include/notdec-backends/Solidity/SolidityPrinter.h

external/NotDec-llvm2c/lib/Solidity/SolidityBackend.cpp
external/NotDec-llvm2c/lib/Solidity/SolidityAst.cpp
external/NotDec-llvm2c/lib/Solidity/SolidityTypePrinter.cpp
external/NotDec-llvm2c/lib/Solidity/SolidityPrinter.cpp
```

不要放在 `src/Passes/evm/solidity-patterns/`。那里是 IR 规范化和高层语义 rewrite，不是最终源码后端。

Pass 接入可以像 llvm2c 一样放在 `PassManager` 外壳里：

- `src/NotDec.cpp` 增加 `.sol` 输出后缀。
- `include/notdec/Passes/PassManager.h` 增加 `add_solidity()`。
- `src/Passes/PassManager.cpp` 增加一个很薄的 `MLsubNotdecSolidity` pass。
- 真正逻辑调用 `notdec::backend::solidity::decompileModule(...)`，不要把后端代码写在 `PassManager.cpp`。

这样后端仍然能拿到 `Module`、`ModuleAnalysisManager` 和 `MLsubRecovery::Result`，和 llvm2c 当前链路一致。

## 后端内部结构

第一版自己定义很小的 Solidity AST，不接 solc AST，也不接 Clang AST。

只需要这些节点：

- `SourceUnit`
- `Contract`
- `StateVariable`
- `Function`
- `Parameter`
- `Block`
- statement：`VariableDeclStmt`、`Assign`、`If`、`While`、`Return`、`Emit`、`Revert`、`ExprStmt`
- expression：`Name`、`Literal`、`Call`、`Member`、`Index`、`Unary`、`Binary`、`Conditional`
- type：`uintN/intN`、`bool`、`address`、`bytesN`、`bytes/string`、array、mapping、struct、unknown

这里不要做完整 Solidity 语义模型。够打印当前能识别的样本即可。

设计上分三层：

1. `SolidityIRReader`
   - 从 LLVM IR、HType、pattern helper 里读信息。
   - 识别 public/external entry、selector、ABI 参数/返回、state variable、event、revert。
2. `SolidityStructurer`
   - 借鉴 llvm2c 的 CFG 结构化思路，把 LLVM block 转成 if/while/return。
   - 第一版可以保守：结构不出来的块打印成内部 helper 函数或注释块，不强行造复杂控制流。
3. `SolidityPrinter`
   - 只负责格式化源码。
   - 类型打印单独放 `SolidityTypePrinter`，避免 printer 里到处判断 HType。

## 结构恢复算法路线

补充 prompt：

> 那还是不考虑这一块了。计划里面得考虑一下结构分析算法。llvm2c那边当前好像实现的是phoenix算法，是不是有一个抽象接口，但是可能得再怎么抽象出来，更抽象一点，使得可以跨语言复用，也方便实现多个不同的结构恢复算法
>
> 另外，调研完这个之后，再调研一下Ghidra的结构恢复算法是在哪里，具体是什么流程，方便后续复刻过来

Solidity 后端不应该自己在后端里临时拼 if/while。结构恢复应该单独抽出来，C 后端和 Solidity 后端都能用。

当前 llvm2c 已经有一个接口：

- `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h`
  - `IStructuralAnalysis`
- `external/NotDec-llvm2c/include/notdec-llvm2c/Phoenix.h`
  - `Phoenix`
- `external/NotDec-llvm2c/include/notdec-llvm2c/Goto.h`
  - `Goto`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp`
  - `SAFuncContext::run()` 里先跑 `CFGCleaner`，再跑 `CompoundConditionBuilder`，然后按 `opts.algo` 选择 `Goto` 或 `Phoenix`。

但这个接口还不够通用。`IStructuralAnalysis` 直接依赖：

- `SAFuncContext`
- llvm2c 自己的 `CFG`
- Clang `Stmt` / `Expr`
- C 后端的 label/goto 构造

所以它只是 llvm2c 内部算法接口，不是跨语言结构恢复接口。Solidity 后端如果直接复用它，会被 Clang AST 和 C 语法绑住。

更合适的抽象是新增一层语言无关结构恢复模块，例如：

```text
external/NotDec-llvm2c/include/notdec-backends/Structuring/
external/NotDec-llvm2c/lib/Structuring/
```

第一版可以先放这些东西：

```text
external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h
external/NotDec-llvm2c/include/notdec-backends/Structuring/Structurer.h
external/NotDec-llvm2c/include/notdec-backends/Structuring/PhoenixStructurer.h
external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoStructurer.h

external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp
external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp
external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp
```

接口边界应该是：

- 输入：语言无关 CFG。
  - block id
  - successor / predecessor
  - block 内 statement 句柄
  - terminator condition 句柄
  - switch 信息
  - return / revert / unreachable 这类出口标记
- 输出：语言无关结构树。
  - sequence
  - if / if-else
  - while
  - do-while
  - infinite loop
  - switch
  - break / continue
  - goto / label fallback
  - basic block leaf

结构恢复层不要知道 C 或 Solidity，也不要构造 Clang AST。表达式和 statement 只作为 opaque handle 保留，后端再解释：

- C 后端把结构树转成 Clang `Stmt`。
- Solidity 后端把结构树转成自己的 Solidity AST。

这样后续可以同时接多个算法：

- `goto`：最保守，几乎只线性化 block，加 label/goto。
- `phoenix`：沿用 llvm2c 当前 Phoenix 路线。
- `ghidra-collapse`：后续复刻 Ghidra 的 collapse 规则。

第一步不要急着把 llvm2c 全部迁走。可以先：

1. 在 backend 仓库新增结构恢复数据结构。
2. 先实现一个很小的 `GotoStructurer`，让 Solidity 后端能输出完整控制流。
3. 再把 llvm2c Phoenix 里不依赖 Clang 的部分逐步搬到 `PhoenixStructurer`。
4. C 后端晚一点再切过去，避免一次性改坏现有 `.c` 输出。

## Ghidra 结构恢复调研

本地 Ghidra 源码位置：

```text
/sn640/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/
```

核心文件：

- `block.hh` / `block.cc`
  - `FlowBlock`
  - `BlockBasic`
  - `BlockGraph`
  - `BlockIf`
  - `BlockWhileDo`
  - `BlockDoWhile`
  - `BlockInfLoop`
  - `BlockSwitch`
  - `BlockGoto`
  - `BlockMultiGoto`
- `blockaction.hh` / `blockaction.cc`
  - `CollapseStructure`
  - `ActionBlockStructure`
  - `ActionPreferComplement`
  - `ActionStructureTransform`
  - `ActionFinalStructure`
- `coreaction.cc`
  - decompile action pipeline，把 block recovery 插到整体流程里。

Ghidra 的主结构恢复类是 `CollapseStructure`。它的注释已经把算法说得很清楚：

1. 从基本块 CFG 开始。
2. 反复找能匹配高层控制结构的子图。
3. 匹配到了就把这几个节点 collapse 成一个结构节点。
4. 如果卡住，就选一些边标成 unstructured goto，然后继续 collapse。

具体入口：

- `ActionBlockStructure::apply(Funcdata &data)`
  - `data.installSwitchDefaults()`
  - `graph.buildCopy(data.getBasicBlocks())`
  - `CollapseStructure collapse(graph)`
  - `collapse.collapseAll()`

`collapseAll()` 的流程：

- `orderLoopBodies()`
  - 先识别 loop body 和内外层关系。
- `collapseConditions()`
  - 先简化 AND/OR 条件结构。
- `collapseInternal(nullptr)`
  - 反复应用 collapse 规则。
- 如果还有没 collapse 完的 block：
  - `selectGoto()` 选一条最合适变成 unstructured goto 的边。
  - 再从这个目标继续 `collapseInternal(targetbl)`。

`collapseInternal()` 里按顺序尝试这些规则：

- `ruleBlockGoto`
- `ruleBlockCat`
- `ruleBlockProperIf`
- `ruleBlockIfElse`
- `ruleBlockWhileDo`
- `ruleBlockDoWhile`
- `ruleBlockInfLoop`
- `ruleBlockSwitch`
- 其他规则都不能推进时，再尝试：
  - `ruleBlockIfNoExit`
  - `ruleCaseFallthru`

`BlockGraph` 负责真正创建结构节点，例如：

- `newBlockList`
- `newBlockCondition`
- `newBlockIf`
- `newBlockIfElse`
- `newBlockWhileDo`
- `newBlockDoWhile`
- `newBlockInfLoop`
- `newBlockSwitch`
- `newBlockGoto`
- `newBlockMultiGoto`

结构恢复之后还有几个后处理：

- `ActionPreferComplement`
  - 对称结构里选更自然的条件方向。
- `ActionStructureTransform`
  - 给结构节点最后一次转换机会，比如 while 转 for。
- `ActionFinalStructure`
  - `orderBlocks()`
  - `finalizePrinting(data)`
  - `scopeBreak(-1,-1)` 生成 break。
  - `markUnstructured()` 生成 goto。
  - `markLabelBumpUp(false)` 修 label。

在 `coreaction.cc` 的默认 decompile pipeline 里，结构恢复不是最后才做一次。关键位置是：

- main loop 里先跑 `ActionRedundBranch`，再跑 `ActionBlockStructure("blockrecovery")`。
- 后面 cleanup / merge / cast / naming 后，再跑：
  - `ActionPreferComplement("blockrecovery")`
  - `ActionStructureTransform("blockrecovery")`
  - `ActionFinalStructure("blockrecovery")`

这说明 Ghidra 的结构恢复和数据流优化是互相影响的，不是纯打印阶段。对 NotDec 第一版不必完全复刻这点。更稳的路线是：

- 先在后端输出前做一次结构恢复。
- 如果发现结构恢复需要改变 IR 或 SSA 形状，再考虑把部分结构恢复前置到 pass pipeline。
- 复刻 Ghidra 时优先复刻 `BlockGraph + CollapseStructure` 这层，不先复刻完整 action database。

后续如果要复刻 Ghidra，建议按这个顺序：

1. 先实现 `BlockGraph` / `StructuredCFG` 这种中间结构。
2. 实现 `newBlockIf`、`newBlockIfElse`、`newBlockList`、`newBlockGoto` 这些最小 collapse。
3. 再加 while/do-while/inf-loop。
4. 再处理 switch。
5. 最后才做 unstructured edge 评分和 `selectGoto()`。

这样比直接搬 `CollapseStructure` 风险小，也能和 Phoenix/Goto 共用同一个输出结构树。

## 第一阶段目标

先围绕 apehex 有源码样本做最小闭环：

- 输出一个合约壳子：`contract Decompiled { ... }`。
- 根据 selector entry 输出 public/external 函数名。没有源码名时用 `public_<selector>`。
- 参数和返回值来自 ABI decode/return helper、HType、semantic primitive。
- state variable 来自 storage rewrite 结果。名字先用 `storage_<slot>` 或 `field_<n>`。
- event 来自 `EventLogPass` 的结果，先能打印 `emit Event_<topic0>(...)`。
- revert 先打印 `revert(...)` 或 `require(...)`，能区分 panic/error/string 时优先打印更高层形式。
- 普通表达式先覆盖算术、比较、逻辑、storage load/store、memory/calldata load 后的变量使用。

判断标准：

- 对 5 到 10 个 apehex 有源码样本，函数边界、参数、返回、主要 storage/event/revert 形状能和源码对上。
- 输出可以读，不要求 solc 编译通过。
- 不引入新的 IR rewrite 规则。发现缺语义时回到前面的 pattern pass 或类型恢复补。

## 第二阶段目标

在第一阶段稳定后，再提高 Solidity 感：

- 用源码样本反推更好的变量命名规则。
- 把常见 guard 提升成 `require`。
- 把 storage field 聚合成 `struct`、`mapping`、array。
- 处理 dynamic bytes/string、dynamic/static array。
- 把 fallback/receive/constructor 分出来。
- 对 internal/private helper 做函数签名恢复，减少一大坨内联代码。

这阶段仍然先看源码对齐效果，不把 solc 解析或 LSP 诊断放进主目标。

## 第三阶段目标

最后再考虑更完整的源码工程形态：

- 多 contract / library / interface。
- inheritance 和 override。
- modifier。
- error/event/struct/enum 命名恢复。
- NatSpec 或注释里输出无法确定的信息。
- 更稳定的结构恢复算法和更少的 goto fallback。

## 不做什么

- 不把 Solidity 后端直接塞进旧的 `external/NotDec-llvm2c` C/Clang AST 层。
- 不用 Clang AST 打印 Solidity。
- 不直接依赖 solc AST 构造源码。
- 不接 Solidity LSP 或 parser 做动态语法校验；这块暂时不考虑。
- 不一开始追求重新编译等价。
- 不在后端里重新猜 calldata/memory/storage 访问模式。
- 不为了输出好看而删除现有 guard 或改变 IR 语义。

## 风险

- HType 里的 array/bytes 还不够硬，第一版不要强行打印成 Solidity array/bytes。
- storage rewrite 如果只保留 helper 名，没有足够 path 信息，后端会缺 state variable 结构。
- CFG 结构化是大头。llvm2c 可以借思路，但 Solidity 后端需要先保守输出。
- 源码样本可能和链上字节码不是同一个 solc 设置，不能把差异都当成 NotDec 错。

## 验证路线

第一轮不跑全 apehex，先挑有源码的 5 到 10 个样本：

- 保存原始 Solidity 源码、NotDec `.sol` 输出、HType dump、workdir IR。
- 人工对比函数数、selector、参数、返回、storage/event/revert。
- 增加一个轻量 runner，只检查输出文件存在、非空、包含 contract/function/event/revert/storage 这些 oracle 计数。
- 如果引入 `.sol` 后端代码，再补 EVM 性能 smoke：
  `test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll`。

## 2026-06-18 实现记录：backend core target 拆分

本轮执行整理计划的第一步：先在 `external/NotDec-llvm2c` 里建立 backend core 骨架，不改 C 后端行为，不开始 Solidity 输出实现。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Core/ExtValuePtr.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface/ExtValuePtr.h`。
- `external/NotDec-llvm2c/include/notdec-backends/Core/HType.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface/HType.h`。
- `external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface.h` 里的 `HTypeResult`。
- `external/NotDec-llvm2c/include/notdec-backends/Core/Range.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface/Range.h`。
- `external/NotDec-llvm2c/include/notdec-backends/Core/StructManager.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface/StructManager.h`。
- `external/NotDec-llvm2c/include/notdec-backends/Core/ValueNamer.h:1`
  新增 core 转发头，暂时转发到旧 `notdec-llvm2c/Interface/ValueNamer.h`。
- `external/NotDec-llvm2c/lib/Core/CMakeLists.txt:1`
  新增 `notdec-backend-core` target，编译 `StructManager`、`ExtValuePtr`、`ValueNamer`、`HType`、`Range` 这些共享代码。
- `external/NotDec-llvm2c/lib/CMakeLists.txt:1`
  先添加 `Core` 子目录，再添加旧 `notdec-llvm2c` 子目录。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/CMakeLists.txt:1`
  旧 `notdec-llvm2c` target 删除 core 源文件重复编译，改为链接 `notdec-backend-core`。

当前保留的限制：

- `notdec-backend-core` 现在仍然需要 Clang 依赖，因为旧 `HType.h` / `StructManager.h` 里还有 `clang::Decl *`。这轮只拆 target 和 include 边界，不清理 HType 的 C 后端 annotation。
- 旧 include 路径和 `notdec::llvm2c` namespace 继续保留，主项目不用同步大改。
- 还没有新增 `Structuring`、`C/`、`Solidity/` 目录；这轮先让 core target 站住。

验证：

- `cmake --build ./build --target notdec-backend-core notdec-llvm2c -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- 只出现既有 warning，包括 `StructManager.cpp` signedness warning、`CFG.cpp` switch warning、`Utils.cpp` LLVM deprecation warning、`ASTPrinter` switch warning。
- 本轮只拆 CMake target 和转发头，不改 pass pipeline，不新增运行时分析逻辑；性能上不预期影响 decompile 路径，未单独跑 EVM runtime smoke。

评分：

- 实现效果：7/10。已经把共享 core target 拆出来，并保持旧 C 后端继续构建。
- 复杂度：3/10。新增一层 CMake target 和转发头，理解成本低。
- 维护成本：4/10。短期有旧路径和新路径并存；后续需要继续清掉 core 对 Clang 的依赖。

更好的后续方案：

- 下一步不要急着加 Solidity AST。先把 `HTypeResult` 从旧 `Interface.h` 里真正拆到 `notdec-backends/Core/HTypeResult.h`。
- 然后处理 `HType.h` / `StructManager.h` 里的 `clang::Decl *`，把它们移到 C 后端 adapter，避免 Solidity 后端被迫链接 Clang。

## 2026-06-18 实现记录：HTypeResult 移入 core

本轮继续第一阶段 core 拆分，把 `HTypeResult` 从旧 C 后端入口头里挪到 backend core 头里，并去掉 core target 对 Clang 库的链接。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h:1`
  从转发头改成真实定义，包含 `HTypeResult` 的 value type、memory/storage type 和 snapshot 打印逻辑。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:1`
  删除 `HTypeResult` 定义和不再需要的 LLVM/Clang include，只保留 C 后端 `Options`、`StructuralAlgorithms`、`decompileModule()`、`demoteSSA()`，并 include 新的 core 头。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:24`
  用 `clang::Decl` 前置声明替代 public Clang include，降低 core 头的依赖。
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/StructManager.h:4`
  删除未使用的 Clang include 和旧的注释字段。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/StructManager.cpp:1`
  删除未使用的 Clang include。
- `external/NotDec-llvm2c/lib/Core/CMakeLists.txt:9`
  `notdec-backend-core` 不再链接 `notdec_clang_deps` / `clangAST`。

当前保留的限制：

- `HTypeResult` 仍在 `notdec::llvm2c` namespace 下，避免主项目和 C 后端同步大改。
- `HType.h` 里还保留 `clang::Decl *ASTDecl` 这类 C 后端 annotation 字段，只是 public include 不再拉 Clang 头。后续如果 Solidity 后端确实需要完全无 Clang 类型污染，再单独迁移这部分字段。

验证：

- `cmake --build ./build --target notdec-backend-core notdec -j4` 通过。
- 只出现既有 warning，包括 `StructManager.cpp` signedness warning 和 `ASTPrinter` switch warning。
- 本轮只改头文件归属和 CMake 链接，不改 pass pipeline，也不改运行时分析逻辑；性能上不预期影响 decompile 路径，未单独跑 EVM runtime smoke。

评分：

- 实现效果：8/10。`HTypeResult` 已经可以从 backend core 引入，core target 也不再链接 Clang。
- 复杂度：3/10。主要是头文件搬迁和依赖清理，兼容旧 namespace。
- 维护成本：4/10。短期仍有旧路径和新路径并存，后续要继续拆 C 专属接口。

## 2026-06-18 实现记录：新增 Structuring 骨架

本轮开始第三阶段的前置工作：先新增语言无关结构恢复 target，不迁移旧 Phoenix，也不接入 C 后端。目标是给 Solidity 后端准备一个不依赖 Clang AST 的结构树接口。

修改内容：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:1`
  新增语言无关 CFG 和结构树数据结构。block、statement、condition 都用 id / payload id 表示，结构恢复层不保存 Clang 或 Solidity AST 指针。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/Structurer.h:1`
  新增结构恢复算法公共接口 `Structurer::structure()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoStructurer.h:1`
  新增保守 fallback 算法声明。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:1`
  实现 `StructuredCFG::addBlock()`、`StructuredCFG::getBlock()`、`StructuredTree::addNode()`、`StructuredTree::getNode()`。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:1`
  实现最小 `GotoStructurer`：每个 block 输出 label、basic block leaf，再按 terminator 输出 goto / if / switch / return / unreachable。
- `external/NotDec-llvm2c/lib/Structuring/CMakeLists.txt:1`
  新增 `notdec-backend-structuring` static target。
- `external/NotDec-llvm2c/lib/CMakeLists.txt:1`
  添加 `Structuring` 子目录。

当前保留的限制：

- 新结构树还没有接主仓库，也没有替换旧 `notdec-llvm2c` 的 `StructuralAnalysis`。
- `GotoStructurer` 只保证完整表达控制流，不负责输出漂亮的 if/while。
- payload id 的实际含义由后端维护。Solidity 后端后续需要自己保存 payload id 到表达式/语句节点的映射。

验证：

- `cmake --build ./build --target notdec-backend-structuring notdec -j4` 通过。
- 本轮只新增未接入 target，不改 pass pipeline 和现有 C 后端运行逻辑；性能上不预期影响 decompile 路径，未单独跑 EVM runtime smoke。

评分：

- 实现效果：7/10。结构恢复层的目录、target、最小接口已经站住，Solidity 后端可以先依赖它做完整 fallback。
- 复杂度：4/10。新增数据结构较少，但引入了新 namespace 和 payload id 约定。
- 维护成本：4/10。后续需要补结构树 printer/adapter，并决定旧 Phoenix 怎么迁移到这套接口。

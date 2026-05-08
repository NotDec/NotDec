# 2026-05-08 LLVM 22 llvm2c 第一轮错误与验证入口

## 背景

`logs/20260508-01-LLVM22Wasm2llvmPort.md` 已经把顶层 CMake 和
`external/NotDec-wasm2llvm` 迁到可编译、可运行的状态。继续全量构建后，新的阻塞点
落在 `external/NotDec-llvm2c`。

这份日志只负责 `llvm2c`。后续如果继续修 LLVM 22 兼容问题，也继续写这里。

## 目标

先回答两个问题：

1. `llvm2c` 在 LLVM 22 下是否必须修编译错误。
2. 它有没有独立二进制，修完后能不能直接单独验证。

## 当前结论

结论是：需要修，而且不是单点错误。

直接执行：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
```

构建在 `external/NotDec-llvm2c` 早期就失败，说明当前还不能靠顶层 `notdec` 间接绕过
这个子模块。

## 第一批错误

目前看到的错误主要分四类：

1. `Optional` 迁移
   - `lib/notdec-llvm2c/Phoenix.cpp:15` 直接包含 `llvm/ADT/Optional.h`，LLVM 22 下已不存在。
   - `include/notdec-llvm2c/CFG.h:85`、`include/notdec-llvm2c/StructuralAnalysis.h:950` 等处仍在用
     `clang::Optional` / `llvm::Optional`。

2. LLVM / Clang 常规 API 变化
   - `lib/notdec-llvm2c/ASTManager.cpp:204`：`StringRef::equals` 不再可用。
   - `include/notdec-llvm2c/Dominators.h:176`：`llvm::erase_value` 不再可用。
   - `lib/notdec-llvm2c/StructuralAnalysis.cpp:264`、`268`：`TypeSize::getFixedSize`
     变成 `getFixed`。

3. Clang AST 构造接口变化
   - `include/notdec-llvm2c/CCodeTransform.h:403` 起多处：`BinaryOperator::getFPFeatures`
     现在不再接收 `LangOptions`。
   - `include/notdec-llvm2c/StructuralAnalysis.h:700`：`CompoundStmt::Create` 现在需要
     `FPOptionsOverride`。
   - `lib/notdec-llvm2c/StructuralAnalysis.cpp:271`：`CallExpr::Create` 的参数类型更严格，
     不能再直接传花括号初始化列表。

4. opaque pointer 相关
   - `lib/notdec-llvm2c/StructuralAnalysis.cpp:480`：不能再从 pointer type 上调
     `getPointerElementType()`。

另外还有一类更麻烦的问题：

- 这不是“项目真的在处理 SYCL/OpenACC 语义时出错”，而是 `llvm2c` 自己实现的
  AST transform / visitor 分发模板和 Clang 22 的节点集合不再对齐。
- 具体点说，`include/notdec-llvm2c/CCodeTransform.h` 里有
  `StmtTransformBase::TransformStmt` / `TransformExpr`。这两套分发会通过
  `clang/AST/StmtNodes.inc` 枚举所有 `Stmt` / `Expr` 节点，然后对每个节点生成
  `cast<Node>(...)` 调用。
- Clang 22 的 `StmtNodes.inc` 已经包含了新的 `SYCL` / `OpenACC` 节点，比如
  `SYCLKernelCallStmt`、`OpenACCWaitConstruct`、`OpenACCUpdateConstruct`、
  `OpenACCLoopConstruct` 等。
- 但我们自己的 `CCodeTransform.h` 只显式包含了 `Stmt.h`、`StmtCXX.h`、
  `StmtObjC.h`、`StmtOpenMP.h`，没有像 LLVM 22 里的
  `clang/AST/StmtVisitor.h` / `RecursiveASTVisitor.h` 那样把
  `StmtSYCL.h`、`StmtOpenACC.h` 一并带进来。
- 结果就是：模板实例化到这些新节点时，编译器只能看到前向声明或不完整类型，看不到
  `Node::classof` 的完整定义，于是报错会落在 `llvm::isa` / `llvm::cast`
  这层，看起来像 `Goto.cpp`、`StructuralAnalysis.cpp` 在炸，但真正的根因是
  `CCodeTransform.h` 的分发模板和头文件集合不匹配。
- 当前最典型的现象是：
  - 编 `Goto.cpp` 也会炸，因为 `Goto.cpp -> Goto.h -> StructuralAnalysis.h ->
    CCodeTransform.h`，即使 `Goto` 自己没直接处理 `SYCL/OpenACC`，也会被这套模板连带实例化。
  - 错误文本通常是 `incomplete type 'clang::SYCLKernelCallStmt' used...` 或
    `incomplete type 'clang::OpenACC*' used...`。
- 这一类问题不能再简单归到“普通 API 替换”。至少需要明确选择下面一种方向：
  1. 补齐 `StmtSYCL.h`、`StmtOpenACC.h` 等头文件，让现有模板继续覆盖全部节点。
  2. 改写 `CCodeTransform.h` 的分发方式，避免对所有 `StmtNodes.inc` 节点都做硬编码式
     `cast<Node>()` 实例化。
  3. 如果项目根本不需要这些节点，想办法把覆盖面裁掉，别让不相关节点进入模板实例化。

## 独立二进制与验证方式

`llvm2c` 有独立工具，不必通过顶层 `notdec` 才能跑。

入口：

- CMake target：`notdec-llvm2c-exe`
- 输出名：`notdec-llvm2c`
- 当前顶层构建目录下的预期路径：
  `build/external/NotDec-llvm2c/bin/notdec-llvm2c`

CLI 用法从 `tools/notdec-llvm2c/notdec-llvm2c.cpp` 可以直接看出来：

- 输入只接受 `.ll` / `.bc`
- `-o xxx.c` 时执行 decompile
- `-o xxx.ll` 时只是把 IR 再 dump 一遍

现有独立验证入口：

1. 最直接的手工验证
   ```bash
   build/external/NotDec-llvm2c/bin/notdec-llvm2c input.ll -o /tmp/out.c
   ```

2. 子模块自带 sysy 回归入口
   - `external/NotDec-llvm2c/test/sysy/CMakeLists.txt`
   - 实际执行的是：
     ```bash
     make clean test
     ```
   - `external/NotDec-llvm2c/test/sysy/Makefile` 里会先用 `clang-14` 把
     `functional/*.c` 编成 `.ll`，再调用 `notdec-llvm2c` 生成 `.dec.c`

注意：这个 `sysy` 测试入口现在默认还写死 `clang-14`，就算 `llvm2c` 本体修到能编，
这套验证脚本是否也要跟着升级，要另看。

## 当前判断

方案评分：6/10。

优点：

- 迁移点已经比较清楚，确实有独立工具可单独验证，不需要每次都跑完整个 `NotDec` 主流程。

问题：

- 报错分散在 LLVM API、Clang AST API、opaque pointer、以及 Clang 22 新节点适配。
- 这比前面的 `wasm2llvm` 迁移更重，不能只按“搜一个旧接口全量替换”的思路做。

建议路线：

1. 先把 `Optional`、`StringRef::equals`、`getFixedSize` 这种低风险机械错误清掉。
2. 再集中处理 Clang AST 构造接口变化。
3. 最后处理 opaque pointer 和 `OpenACC*` 这类更容易牵出设计问题的点。

## 本次未做

这次还没有修改 `external/NotDec-llvm2c` 源码，也没有性能数据，因为项目还没恢复到
可完整构建、可跑 fortune 的状态。

## 已完成：第一轮低风险接口迁移

这轮只清“机械替换”级别的问题，没有继续碰 opaque pointer，也没有处理
`OpenACC` / `SYCL` 那组 visitor 不完整类型。

修改：

1. `external/NotDec-llvm2c/include/notdec-llvm2c/CFG.h:19,86`
   - `CFGElement::getAs` 从 `clang::Optional` 改成 `std::optional`。
   - 涉及函数：`CFGElement::getAs`。

2. `external/NotDec-llvm2c/lib/notdec-llvm2c/CFG.cpp:78`
   - `getAs<CFGStmt>()` 的调用侧改成 `auto` + `std::optional` 风格。
   - 涉及函数：`StmtPrinterHelper::StmtPrinterHelper`。

3. `external/NotDec-llvm2c/include/notdec-llvm2c/Phoenix.h:7,83`
   `external/NotDec-llvm2c/lib/notdec-llvm2c/Phoenix.cpp:739-765,960-969,1012-1036`
   - `llvm::Optional` / `llvm::None` 改成 `std::optional` / `std::nullopt`。
   - `hasValue()` 改成 `has_value()`。
   - 涉及函数：`findLastResortEdge`、`virtualizeReturn`、`lastResort`。

4. `external/NotDec-llvm2c/include/notdec-llvm2c/PostOrderCFGView.h:8-9,38-49`
   - 去掉 `llvm/ADT/None.h`。
   - `po_iterator` 辅助返回值从 `llvm::NoneType` 改成 `std::monostate`。
   - 涉及函数：`PostOrderCFGView::CFGBlockSet::insert`。

5. `external/NotDec-llvm2c/include/notdec-llvm2c/Dominators.h:177`
   - `llvm::erase_value` 改成 `erase(remove(...))`。
   - 涉及函数：`ChildrenGetterTy<clang::CFGBlock, IsPostDom>::get`。

6. `external/NotDec-llvm2c/lib/notdec-llvm2c/ASTManager.cpp:201-205`
   - `StringRef::equals` 改成 `==`。
   - 涉及函数：`ASTManager::getFuncDeclaration`。

7. `external/NotDec-llvm2c/include/notdec-llvm2c/StructuralAnalysis.h:700-703,951-956`
   `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:256-275,691-692,781-878,883-888,1004-1008,1274,1268-1275,1969-1970,2159-2161`
   - `convertOp` 的返回类型改成 `std::optional`。
   - `hasValue()` / `getValue()` 改成 `has_value()` / `*op`。
   - `TypeSize::getFixedSize()` 改成 `getFixedValue()`。
   - 多处 `startswith` 改成 `starts_with`。
   - `CompoundStmt::Create` 补 `clang::FPOptionsOverride()`。
   - `CallExpr::Create` 的参数列表改成显式 `llvm::ArrayRef<clang::Expr *>`。
   - 涉及函数：`CFGBuilder::visitAllocaInst`、`handleCmp`、`convertOp`、
     `CFGBuilder::visitCallInst`、`CFGBuilder::visitExtractValueInst`、
     `CFGBuilder::visitCastInst`、`SAContext::createDecls`、
     `decompileFunction`、`IStructuralAnalysis::makeCompoundStmt`。

8. `external/NotDec-llvm2c/include/notdec-llvm2c/CCodeTransform.h:215-218,400-498`
   - `BinaryOperator::getFPFeatures(this->Context.getLangOpts())` 改成
     `getFPFeatures()`。
   - 涉及函数：`StmtTransform::TransformBinaryOperator`、
     `DivideTransform::TransformBinaryOperator`。

9. `external/NotDec-llvm2c/lib/notdec-llvm2c/Goto.cpp:112-114`
   `external/NotDec-llvm2c/lib/notdec-llvm2c/Phoenix.cpp:324-326,1241-1243`
   - `CompoundStmt::Create` 补 `clang::FPOptionsOverride()`。
   - 涉及函数：`Goto::execute`、`Phoenix::ReduceCyclic`、
     `Phoenix::reduceIncSwitch`。

10. `external/NotDec-llvm2c/lib/notdec-llvm2c/ASTPrinter/StmtPrinter.cpp:2050`
    - `Optional<Expr *>` 改成 `auto`，适配新返回类型。
    - 涉及函数：`StmtPrinter::VisitCXXNewExpr`。

11. `external/NotDec-llvm2c/lib/notdec-llvm2c/Utils.cpp:60-62,92-93`
    - `getBoolTy` 直接返回 `Ctx.BoolTy`，不再走旧 `getTypedefType` 接口。
    - `endswith` 改成 `ends_with`。
    - 涉及函数：`getBoolTy`、`buildAST`。

12. `external/NotDec-llvm2c/tools/notdec-llvm2c/notdec-llvm2c.cpp:2,111`
    - 引入 `<optional>`。
    - `PassBuilder` 第三个参数从 `llvm::None` 改成 `std::nullopt`。
    - 涉及函数：`main`。

13. `external/NotDec-llvm2c/include/notdec-llvm2c/CCodeTransform.h:15,19`
    - 补 `clang/AST/StmtOpenACC.h` 和 `clang/AST/StmtSYCL.h`。
    - 目的不是增加新功能，而是让 `StmtTransformBase::TransformStmt` /
      `TransformExpr` 通过 `StmtNodes.inc` 生成 `cast<Node>()` 分发时，
      能拿到 Clang 22 新增 `SYCL` / `OpenACC` 节点的完整定义。
    - 涉及模板：`StmtTransformBase::TransformStmt`、
      `StmtTransformBase::TransformExpr`。

验证：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
```

结果：

- `Optional` / `None` / `equals` / `startswith` / `endswith` /
  `getFixedSize` 这批低风险接口错误基本已经前推掉。
- `Utils.cpp` 里的 `getTypedefType` 旧接口也不再是当前首要失败点。
- 给 `CCodeTransform.h` 补 `StmtOpenACC.h` / `StmtSYCL.h` 之后，之前那批
  `SYCLKernelCallStmt`、`OpenACC*` 的 `incomplete type` 报错已经不再是最前排，
  说明这部分判断是对的。
- 当前新的主阻塞已经更清楚地集中到三类：
  1. `include/notdec-llvm2c/Dominators.h:167-178` 这组
     `IDFCalculatorDetail::ChildrenGetterTy` 适配现在成了新的第一失败点。
     代码当前特化的是 `clang::CFGBlock`，但实际 `ControlDependencyCalculator`
     里跑的是 `IDFCalculatorBase<notdec::llvm2c::CFGBlock, true>`。LLVM 22 下
     模板推导更严格，已经不再接受这里的节点类型/迭代器不匹配：
     `children()` 返回的是 `AdjacentBlock` 范围，不是 `auto *Succ` 期待的裸指针范围。
  2. 还没处理的 opaque pointer 问题，例如
     `lib/notdec-llvm2c/StructuralAnalysis.cpp:482` 和 `2354` 的
     `getPointerElementType()`。
  3. 仍有少量 LLVM / Clang 22 接口变动还没清干净，但它们已经不是最前面的阻塞了。
     现在更像是第二梯队问题，不再是主线第一个卡点。

性能：

- 仍未形成可运行的完整 `llvm2c` 二进制，不能跑 fortune，也没有新增性能数据。

当前评分：

- 6.5/10。
- 收益是把真正简单的 LLVM 22 / Clang 22 接口迁移先剥干净了。
- 风险是后面的阻塞已经不再是“搜索替换”级别，尤其是 visitor 和新 AST 节点那块，
  需要单独决定是补全头文件、裁掉 visitor 覆盖面，还是换一层实现方式。

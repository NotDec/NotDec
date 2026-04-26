# Parallel PointerAnalysis Implementation

## 修改目标

按照 `logs/20260424-08-ParallelPointerTypeAnalysisDesign.md` 落地第一版并行
PointerAnalysis 接口。默认仍是旧 `original` 行为；通过
`NOTDEC_POINTER_ANALYSIS_MODE=shadow|replace` 启用新路径。

## 修改内容

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:55`：新增
  `PointerAnalysisMode`，支持 `Original`、`Shadow`、`Replace` 三种模式。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:61`：新增
  `MemoryLocKey`，用 `Root + Path + BitSize` 表示显式对象字段身份。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:82`：新增
  `PointerSlotKey`，区分 SSA pointer value slot 和 memory pointer cell。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:94`：新增
  `PointerAnalysis`，维护 `pts(slot) = {MemoryLocKey}`，支持 addr-of、copy、
  field、load-ptr、store-ptr 和 solve。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:149`：新增
  `RecordedLoad`、`RecordedStore` 和 `MemoryAccessRecords`，记录 IR load/store
  事件，并给对象内容分配 `SimpleType`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:184`：让
  `ConstraintsGenerator` 持有 `PointerAnalysis PA`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:190`：让
  `ConstraintsGenerator` 持有 `MemoryAccessRecords MemoryAccesses`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:217`：从环境变量
  `NOTDEC_POINTER_ANALYSIS_MODE` 读取 PA 模式，默认 `original`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:256`：在 `PG.solve()` 后，
  shadow/replace 模式下运行 `PA.solve()` 和
  `flushPointerDerivedTypeConstraints()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:299`：扩展
  `addSubtype()` 的 observer，shadow 模式下记录旧 PtrStore/PtrLoad 推出的
  store-load 关系。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:346`：新增
  PA 模式判断、对象 root 创建、addr-of/copy 入口、load/store 记录入口和
  flush 入口。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:380`：扩展
  `setAsPtrAdd()`，保留原 record 类型约束，同时通知 `PA.addField()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:71`：新增 `MemoryLocKey` 和
  `PointerSlotKey` 的 trace 格式化。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2084`：实现
  `PointerAnalysis::addPointsTo()`，内部去重并支持 delta callback。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2099`：实现 root object 和 field
  object 创建。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2112`：实现 addr-of、copy、field、
  load-ptr、store-ptr 规则注册。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2139`：实现 inclusion-style
  worklist 求解，传播 copy、field、pointer load 和 pointer store。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2203`：实现
  `MemoryAccessRecords::getOrCreateLocType()`，为显式对象内容创建
  `SimpleType`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2217`：新增 PA trace 输出。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2226`：实现旧 PtrStore/PtrLoad
  relation 的 shadow 观测。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2246`：实现 `recordLoad()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2259`：实现 `recordStore()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2272`：实现
  `onPointsToDelta()`，把 `pts(addr)` 翻译成 `ObjTy(loc) <: loadTy` 和
  `storeTy <: ObjTy(loc)`；replace 模式才真正调用 `addSubtype()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2310`：实现
  `flushPointerDerivedTypeConstraints()`，输出 pts、PA 派生 load/store 边和
  store-load relation。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2389`：在创建 `ConstantAddr` 和
  `GlobalVariable` 节点时给 PA 添加 root object。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2400`：扩展 `addRemapType()`，
  同步添加 PA copy。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3780`：在 malloc 分支添加 heap
  object addr-of。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3829`：在 PHI 处理时添加 PA copy。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3846`：扩展 load 处理，记录内存
  load；指针 load 时添加 `PA.addLoadPtr()`；replace 模式下停止添加旧
  `make_ptr_load()` 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3876`：扩展 store 处理，记录内存
  store；指针 store 时添加 `PA.addStorePtr()`；replace 模式下停止添加旧
  `make_ptr_store()` 约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3908`：在 alloca 分支添加 stack
  object addr-of。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3982`：在 select 处理时添加两条
  PA copy。

## 涉及函数

- 新增/实现：`PointerAnalysis::addPointsTo()`、
  `PointerAnalysis::getRootObject()`、`PointerAnalysis::getFieldObject()`、
  `PointerAnalysis::addAddrOf()`、`PointerAnalysis::addCopy()`、
  `PointerAnalysis::addField()`、`PointerAnalysis::addLoadPtr()`、
  `PointerAnalysis::addStorePtr()`、
  `PointerAnalysis::addPointsToDeltaCallback()`、`PointerAnalysis::solve()`。
- 新增/实现：`MemoryAccessRecords::getOrCreateLocType()`。
- 新增/实现：`ConstraintsGenerator::emitPointerAnalysisTrace()`、
  `ConstraintsGenerator::observeOldMemoryTypeEdge()`、
  `ConstraintsGenerator::recordLoad()`、`ConstraintsGenerator::recordStore()`、
  `ConstraintsGenerator::onPointsToDelta()`、
  `ConstraintsGenerator::flushPointerDerivedTypeConstraints()`。
- 修改：`ConstraintsGenerator::run()`、`ConstraintsGenerator::addSubtype()`、
  `ConstraintsGenerator::setAsPtrAdd()`、`ConstraintsGenerator::createNode()`、
  `ConstraintsGenerator::addRemapType()`。
- 修改：`ConstraintsGenerator::MLsubVisitor::visitCallBase()`、
  `ConstraintsGenerator::MLsubVisitor::handlePHINodes()`、
  `ConstraintsGenerator::MLsubVisitor::visitLoadInst()`、
  `ConstraintsGenerator::MLsubVisitor::visitStoreInst()`、
  `ConstraintsGenerator::MLsubVisitor::visitAllocaInst()`、
  `ConstraintsGenerator::MLsubVisitor::visitSelectInst()`。

## 验证

- 通过：`cmake --build ./build --target all -j4`。
- 通过：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`。
- 通过 shadow 冒烟：
  `NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune-pa-shadow.ll --tr-level=2 --frozen-tr-input-ir -g --work-dir=/tmp/notdec-pa-shadow`。
  结果：`binarysub-trace.log` 中有 `pts=65`、`new-store-load=146`。
- 通过 replace 冒烟：
  `NOTDEC_POINTER_ANALYSIS_MODE=replace NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune-pa-replace.ll --tr-level=2 --frozen-tr-input-ir -g --work-dir=/tmp/notdec-pa-replace-fortune`。
  结果：`binarysub-trace.log` 中有 `pts=65`、`new-load=91`、
  `new-store=54`、`new-store-load=146`。
- 未通过：`ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 --output-on-failure`。
  失败用例仍是 `fortune.o3.wasm`，compare 显示 `@File_list`、`free_desc::arg0`、
  `get_tbl::arg0`、`matches_in_list::arg0`、`maxlen_in_list::arg0` 缺字段或
  `void*`，属于当前 fortune oracle 差异；默认 `original` 模式仍保留旧
  PtrLoad/PtrStore 约束。


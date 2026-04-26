# PointerAnalysis File Split

## 修改目标

把 `PointerAnalysis` 从 `MLsubGenerator` 中拆出来，减少
`MLsubGenerator.h/.cpp` 的职责。只做文件拆分，不改变 PA 行为和默认
`original` 模式。

## 修改内容

- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:1`：新增独立头文件。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:19`：移动
  `PointerAnalysisMode`。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:25`：移动
  `MemoryLocKey`，继续用 `Root + Path + BitSize` 表示对象字段身份。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:46`：移动
  `PointerSlotKey`，继续区分 SSA value slot 和 memory cell slot。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:52`：移动
  `PointsToDelta`。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:58`：移动
  `PointerAnalysis` 类声明。
- `include/notdec/TypeRecovery/mlsub/PointerAnalysis.h:113`：声明
  `formatMemoryLocKey()` 和 `formatPointerSlotKey()`，供 trace 使用。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:1`：新增独立实现文件。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:8`：移动
  `formatMemoryLocKey()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:20`：移动
  `formatPointerSlotKey()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:27`：移动
  `PointerAnalysis::addPointsTo()`。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:42`：移动 root/field object
  创建函数。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:55`：移动 addr-of、copy、field、
  load-ptr、store-ptr 规则注册函数。
- `src/TypeRecovery/mlsub/PointerAnalysis.cpp:82`：移动
  `PointerAnalysis::solve()`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:41`：改为 include
  `PointerAnalysis.h`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:56`：删除原来内联在
  `MLsubGenerator.h` 里的 PA 数据结构，只保留 `RecordedLoad`、
  `RecordedStore` 和 `MemoryAccessRecords`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2065`：保留
  `MemoryAccessRecords::getOrCreateLocType()` 在 `MLsubGenerator.cpp`，因为它
  需要 `ConstraintsGenerator` 和 BinarySub 类型变量。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2079`：保留
  `ConstraintsGenerator` 的 PA trace 和类型约束 bridge。
- `src/CMakeLists.txt:20`：把 `TypeRecovery/mlsub/PointerAnalysis.cpp` 加入
  `notdec` 静态库。

## 涉及函数

- 移动到新文件：`PointerAnalysis::addPointsTo()`、
  `PointerAnalysis::getRootObject()`、`PointerAnalysis::getFieldObject()`、
  `PointerAnalysis::addAddrOf()`、`PointerAnalysis::addCopy()`、
  `PointerAnalysis::addField()`、`PointerAnalysis::addLoadPtr()`、
  `PointerAnalysis::addStorePtr()`、
  `PointerAnalysis::addPointsToDeltaCallback()`、`PointerAnalysis::solve()`。
- 移动到新文件：`formatMemoryLocKey()`、`formatPointerSlotKey()`。
- 保持在 `MLsubGenerator.cpp`：`MemoryAccessRecords::getOrCreateLocType()`、
  `ConstraintsGenerator::emitPointerAnalysisTrace()`、
  `ConstraintsGenerator::observeOldMemoryTypeEdge()`、
  `ConstraintsGenerator::recordLoad()`、`ConstraintsGenerator::recordStore()`、
  `ConstraintsGenerator::onPointsToDelta()`、
  `ConstraintsGenerator::flushPointerDerivedTypeConstraints()`。

## 验证

- 通过：`cmake --build ./build --target all -j4`。
- 通过：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`。
- 通过 shadow 冒烟：
  `NOTDEC_POINTER_ANALYSIS_MODE=shadow NOTDEC_BINARYSUB_TRACE=1 ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune-pa-shadow-split.ll --tr-level=2 --frozen-tr-input-ir -g --work-dir=/tmp/notdec-pa-shadow-split`。
  结果：`binarysub-trace.log` 中 `pts=65`、`new-store-load=146`。


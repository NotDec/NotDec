# Replace Mode Pointer Access View Implementation

## 修改目标

根据 `logs/20260426-01-ReplaceModePointerLoadStoreView.md`，在 replace 模式下
恢复最终 HType 输出里的 `ptr<load=..., store=...>` 视图。

目标不是让 PointerAnalysis 重新依赖 BinarySub，而是在 PA 求解完成后，把已记录的
IR load/store 事件投影成 pointer access view，供最终类型输出使用。

## 修改内容

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:79`：在
  `MemoryAccessRecords` 中新增 `EmittedLoadViews`，用于去重 replace 模式派生的
  load view。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:80`：在
  `MemoryAccessRecords` 中新增 `EmittedStoreViews`，用于去重 replace 模式派生的
  store view。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:286`：新增
  `ConstraintsGenerator::addPointerAccessViews()` 声明。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2172`：实现
  `ConstraintsGenerator::addPointerAccessViews()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2176`：遍历
  `MemoryAccesses.LoadsByAddr`，对每个已记录的 load 给地址节点补
  `make_ptr_load(Load.ResultTy, Load.BitSize)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2193`：遍历
  `MemoryAccesses.StoresByAddr`，对每个已记录的 store 给地址节点补
  `make_ptr_store(Store.ValueTy, Store.BitSize)`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2263`：在
  `flushPointerDerivedTypeConstraints()` 末尾调用 `addPointerAccessViews()`。

## 设计说明

replace 模式下仍然保留 PA 的对象内容流：

```text
store v, p  =>  type(v) <: ObjTy(O)
load p -> r =>  ObjTy(O) <: type(r)
```

新增的 access view 只恢复指针值自己的输出形状：

```text
p : ptr<load=type(r), store=type(v)>
```

这样 `PointerAnalysis` 仍负责对象身份，`ObjTy(MemoryLocKey)` 仍负责对象内容类型，
最终 HType 输出也能和旧模式一样显示 load/store 区分。

这里没有使用 `ObjTy(O)` 作为 view 的 load/store 侧，因为实际对比发现这会输出比旧
golden 更具体的对象内容变量，例如 `ptr<load='i:32, store='i:32>`，而旧模式期望
的是原始 load result / store value 形成的视图。

## 涉及函数

- 新增：`ConstraintsGenerator::addPointerAccessViews()`。
- 修改：`ConstraintsGenerator::flushPointerDerivedTypeConstraints()`。
- 相关但未修改行为：`ConstraintsGenerator::recordLoad()`、
  `ConstraintsGenerator::recordStore()`、`ConstraintsGenerator::onPointsToDelta()`。

## 验证

- 通过：`cmake --build ./build --target all -j4`。
- 通过 original 回归：
  `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`。
- 通过 replace suite：
  `NOTDEC_POINTER_ANALYSIS_MODE=replace python3 test/run_type_recovery_suite.py --binary build/bin/notdec --manifest test/type-recovery/llvm-ir/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-llvm-ir-replace-event-view`。
  结果：`16 passed, 3 xfailed, 0 failed`。
- 与 original 输出完全一致：
  `/tmp/notdec-llvm-ir-original` 和 `/tmp/notdec-llvm-ir-replace-event-view`
  中 16 个可输出 `*.out.htypes` 全部一致，`diff=0`。


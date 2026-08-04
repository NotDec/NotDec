# 删除已弃用的 PointerAnalysis

## 原始 prompt

当前指针分析应该被弃用了，分析一下指针分析的代码，看看怎么直接移除掉

## 背景与判断

`ConstraintsGenerator` 默认使用 `PointerAnalysisMode::Original`，实际只执行
BinarySub/PNDiff 的普通内存约束；`shadow` 和 `replace` 只是历史环境变量路径，已不再
用于当前测试和 profiling。PA 自身还维护 `MemoryLocKey`、路径归一化、points-to 固定点
求解和派生 load/store 约束，增加了代码和潜在的内存/时间开销。

## 实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:77-82`：用只保存 EVM
  store evidence 的 `EVMStoreRecords` 替代原来依赖 `MemoryLocKey` 的
  `MemoryAccessRecords`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:320-398`：删除 PA 成员、模式
  读取、`PA.solve()` 和所有 PA 桥接接口；`run()` 只保留原来的 BinarySub 求解和
  后续合并策略。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2921-2965`：删除 PA trace、旧
  PtrLoad/PtrStore shadow 观测和对象类型创建。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:4928-4938`：`recordStore()` 只记录
  EVM store，供后续语义 pass 使用。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:5064-5125`、`8518-8691`：删除
  addr-of/copy/field/load-ptr/store-ptr 注册，恢复普通 `ptr_load`、`ptr_store` 和
  BinarySub 字段约束。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6016-6050`：从新的 EVM store 记录
  生成 `EVMStoreEvidence`。
- `src/CMakeLists.txt:26-35`：移除 `PAPath.cpp` 和 `PointerAnalysis.cpp`。
- 删除 `include/notdec/TypeRecovery/mlsub/{PAPath.h,PointerAnalysis.h}` 和
  `src/TypeRecovery/mlsub/{PAPath.cpp,PointerAnalysis.cpp}`。
- 更新 20/21 两个递归字段测试的注释，使其描述 BinarySub 字段恢复，不再声称测试
  replace-mode PA；测试文件名暂时保留，避免无关 golden 路径变更。

`HeapObject`、`StackObject` 没有删除。它们仍是 llvm2c 的公共 `ExtValuePtr` 变体，
由后端和测试使用；本次只移除了主类型恢复中把它们当作 PA 根对象的用法。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`：通过。
- 全量 `cmake --build ./build --target all -j4` 仍被已有的
  `external/NotDec-llvm2c` Solidity 测试接口不匹配阻断，与本次改动无关。
- SysY suite 仍是已有 golden 漂移；realworld suite 仍因 extra-constraints 的 IR
  hash 不匹配失败，均不是本次删除引入的错误。

## 影响评估

- 运行时：默认类型恢复路径的语义保持不变，删除了默认本来不会执行的 PA 状态和
  分支；复杂度降低，高度低。
- 理解成本：减少一个独立固定点分析和一套 `MemoryLocKey/PAPath` 数据结构，降低
  理解成本；复杂度降低，高度低。
- 维护成本：删除 shadow/replace 功能后，不能再通过
  `NOTDEC_POINTER_ANALYSIS_MODE` 请求旧 PA 行为；这是有意的兼容性变化。若未来需要
  别名分析，应作为独立模块重新设计，而不是恢复这套桥接。

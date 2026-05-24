# 2026-05-09 删除未接线的 DSROA

## 背景

前面已经确认：

- `src/Passes/DSROA.cpp`
- `include/notdec/Passes/DSROA.h`

当前都没有接进主 pass pipeline。

仓库内也没有搜到 `DSROAPass` 的实际使用点。它现在唯一的实际影响，是还在
`src/CMakeLists.txt` 里参与构建，导致 LLVM 22 迁移时会额外卡在这份没接线的旧代码上。

## 目标

1. 从主项目源码和构建里删除未使用的 `DSROA`。
2. 收敛 LLVM 22 当前的构建阻塞面。

## 修改

1. `src/CMakeLists.txt`
   - `2-21` 的 `notdec` 静态库源文件列表里，删除了原来的
     `Passes/DSROA.cpp` 条目。

2. `src/Passes/DSROA.cpp`
   - 整个删除。
   - 涉及函数：`DSROAPass::run`、`runOnAlloca`、`deleteDeadInstructions`、
     `splitAlloca`、`presplitLoadsAndStores`、`rewritePartition`、
     `clobberUse`、`promoteAllocas` 等全部移除。

3. `include/notdec/Passes/DSROA.h`
   - 整个删除。
   - 涉及类型：`notdec::DSROAPass`、`dsroa::Partition`、
     `dsroa::AllocaSlices`、`dsroa::AllocaSliceRewriter` 的声明一起移除。

## 验证

验证命令：

```bash
ninja -C build-main-no-llvm2c -k 20 notdec > /tmp/notdec-build-no-dsroa.log 2>&1
```

判断标准：

- 构建日志里不再出现 `src/Passes/DSROA.cpp.o`
- 后续如果还有失败，应该落到其他剩余 LLVM 22 迁移点

结果：

- 日志里已经不再出现 `src/Passes/DSROA.cpp.o`。
- 当前新的第一失败点已经变成
  `src/TypeRecovery/mlsub/MLsubGenerator.cpp.o`。
- 具体剩余报错是 LLVM 22 opaque pointer 相关的
  `getPointerElementType()`：
  - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3436-3443`
  - `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3720`

## 性能

这次只是删除一个未接线、未运行的 pass，没有新的 fortune 性能数据。

## 当前判断

方案评分：8.5/10。

优点：

- 删除的是当前仓库内未接线代码，收益直接，风险相对低。
- 能把 LLVM 22 构建面继续收敛，避免花时间迁一份当前没跑到的旧 pass。

代价：

- 如果后面想重新启用 DSROA，需要从历史里恢复，或者重新基于新 LLVM 版本接回。

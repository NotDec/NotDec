# 2026-04-18 TypeBuilder Root Trace 去重

日期：2026-04-18

## 目的

调整 `work_dir/04-typebuilder-convertstruct.log` 的输出格式：

1. 顶层 `convertStruct` 入口继续打印 `root=... [stable=...]`
2. 递归缩进层不再重复打印 `root=...`
3. 递归层只保留 `path=...`

这样可以避免深层递归日志里不断重复同一个 root 标签，更容易聚焦当前递归位置。

## 具体修改

1. 在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:143](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L143)
   到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:155](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L155)
   新增辅助函数 `appendCurrentDebugPathOnly`
   - 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
   - 函数：`appendCurrentDebugPathOnly`
   - 作用：只输出当前递归 `path=`，不重复输出 root label

2. 在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:775](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L775)
   到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:785](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L785)
   调整 `TypeBuilder::convertStruct`
   的 `begin` trace：
   - 顶层 `TraceDepth == 0` 时继续输出
     `root=<label> [stable=...] path=...`
   - 非顶层时改为只输出 `path=...`

3. 在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:1120](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L1120)
   到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:1131](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L1131)
   同步调整 `TypeBuilder::convertStruct`
   的 `pointee/layout mismatch` trace：
   - 顶层保留 root
   - 非顶层只保留 path

## 验证

本次仅完成代码与日志整理，尚未重新运行 `notdec` 生成新的
`work_dir/04-typebuilder-convertstruct.log`。

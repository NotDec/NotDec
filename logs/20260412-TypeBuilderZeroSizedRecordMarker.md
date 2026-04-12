# 2026-04-12 TypeBuilder 零大小 Record Marker 收尾

## 背景

当前暂存修改没有在 `logs/` 下找到现成对应条目，但从代码内容看，明显属于
`TypeBuilder` 近期 HType / set-theoretic type 收敛过程中的一个后续修补。

这批改动针对的问题是：

- `binarysub` 某些路径会产出空 `URecordType`，它更像“零大小占位 marker”，而不是真正应当落到最终 `HType` 里的结构体
- 这些 marker 一旦混入 `UUnion` / `UInter` 或 struct field 构造流程，会导致：
  - union / intersection 两侧被无意义空 record 污染
  - `convertStruct()` 里留下 size 为 0 的 field
  - 空 panel / 空 struct 最终退化路径不稳定

## 本次改动

改动文件：

- `.gitignore`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp`

核心处理：

1. 新增 `isZeroSizedRecordMarker()`
   - 识别空 `URecordType`
   - 也递归识别“字段全是零大小 marker”的 record

2. 在以下转换路径里跳过 marker 分支：
   - `convertFieldType()`
   - `convert()`
   - `convertPointer()`

   如果 `UUnion` / `UInter` 一侧是 marker，就直接返回另一侧，而不是继续构造
   `SetUnionType` / `SetInterType`。

3. 在 `convertStruct()` 里过滤零大小 field
   - 跳过 `RemainingEntries` 中的 marker
   - 在正式构造前删除 `Size == 0` 的 field entry

4. 调整空字段收尾语义
   - 不再对空 struct 打 `Error: Empty Struct?`
   - 若 pointee size 已知且大于 0，则保留一个空 layout 的 struct shell
   - 否则退回 `void*`

5. 补充代码注释
   - 说明 zero-sized record marker 属于 `binarysub` 内部占位，而非用户可见聚合
   - 解释 union / intersection 折叠 marker 分支，以及空 field 收尾为何保留
     sized struct shell

6. `.gitignore` 增加 `Prompts.md`
   - 避免本地辅助记录误入版本库

## 预期效果

- 最终 `HType` 不再被零大小 record marker 污染
- union / intersection 更接近真实语义，而不是把内部占位节点也暴露到输出
- struct 构造在空 field 边界上更稳定，不再打印误导性错误

## 验证

执行：

```bash
cmake --build build --target notdec-decompile
```

结果：

- 通过

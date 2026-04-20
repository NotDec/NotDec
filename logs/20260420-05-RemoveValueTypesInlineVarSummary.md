# 2026-04-20 Remove ValueTypes inline var summary

## 目标

上一轮把 `UTypeVariable::{id, originIds}` 的摘要直接拼到了
`ValueTypes.txt` 每一行的末尾：

- `; vars='a=ut#...<-vs#...`

实践下来，这对真实大 case 的直接扫读仍然太吵，尤其是函数类型和大型 record
类型本身已经很长时，会把“主信息是 `UType`”这件事冲淡。

这次改动的目标是：

1. `ValueTypes.txt` 回到只看 `value => UType`
2. 所有变量追踪信息统一放到 `VarOrigins.txt`

## 修改

### 1. 去掉 `ValueTypes.txt` 的行尾变量摘要

- 文件：
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1312](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1312)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1546](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1546)
- 涉及：
  - `appendDebugValueTypes`
  - `formatInlineUTypeVariableSummary`

具体处理：

- `appendDebugValueTypes()` 不再给每条记录追加 `VarSummary`
- `[memory] <memory>` 行也不再补 `; vars=...`
- 删除 `formatInlineUTypeVariableSummary()` 辅助函数

现在 `ValueTypes.txt` 再次只保留：

```text
[+] <value-label> => <binarysub UType>
```

### 2. 文档改为显式引导去 `VarOrigins.txt`

- 文件：
  - [DEBUG.md:125](/sn640/NotDec/DEBUG.md#L125)

更新后明确说明：

- `ValueTypes.txt` 只保留 `value => UType`
- `ut#id / vs#originId / ExtValuePtr` 的展开统一看 `VarOrigins.txt`

## 验证

本次是 workdir 文本导出逻辑收缩，没有改类型恢复算法本身。

建议验证方式：

```bash
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-valuetypes-clean/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-valuetypes-clean
```

预期结果：

1. `ValueTypes.txt` 不再出现 `; vars=...`
2. `VarOrigins.txt` 仍完整保留 `appears-in` 与 `vs#originId => ExtValuePtr`

## 结论

`ValueTypes.txt` 更适合作为“主结果视图”，不要继续承载解释性元数据。

如果后续还要增强 explainability，优先继续扩充单独文件：

- `VarOrigins.txt`
- 未来可能的 machine-readable JSON dump

而不是再往 `ValueTypes.txt` 单行里塞结构化摘要。

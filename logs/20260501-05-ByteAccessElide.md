# 2026-05-01 Byte Access Elide

## 背景

`fortune.o3.wasm.ll` 的类型恢复里，同一偏移同时出现 `read(..., 4)` 的 4-byte
访问和后续 `i8` / byte 视图时，最终会在 `ValueHTypes.txt` 里落成：

- `ptr<load=u32 & u8, store=u32 | u8, psize=32>*`

这会把本来已经比较清楚的 `u32` 字段又混回 byte 访问。

## 目标

- 同名 primitive 在最终 `UType` / `HType` 输出阶段，如果同时有 `1-byte` 和更大位宽，
  忽略 `1-byte` 分量。
- 不影响 solver 主体，只收紧最终展示和 lowering 结果。
- 顺手修掉一个明显的 size 忽略点：`remove-primitive-sandwich` 不能再把
  `uint:8` 和 `uint:32` 当成同一个 primitive。

## 修改

文件：`external/binarysub/src/binarysub.cpp`

1. `coalesce_primitive_components()`，`113-145`
   - 新增 `hasWiderSiblingWithSameName()`
   - 当某个 primitive 是 `size=8`，并且同名存在更大位宽 sibling 时，直接跳过这个
     `8-bit` primitive，不再进入最终 `UType` 组件列表。

2. `TypeSimplifier::computeSimplificationPlan()`，`1522-1542`
   - `remove-primitive-sandwich` 从只比 `name` 改成同时比较 `name + size`
   - 避免把 `uint:8` / `uint:32` 误当成同一种 primitive 提前抹掉中间变量。

## 验证

构建：

```bash
cmake --build ./build --target bin/notdec -j1
```

运行：

```bash
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.byte-access-elide-v8.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-byte-access-elide-v8
```

结果：

- 时间：`real 15.01s`
- 对比基线：`16.33s`
- `[/tmp/notdec-fortune-byte-access-elide-v8/ValueHTypes.txt](/tmp/notdec-fortune-byte-access-elide-v8/ValueHTypes.txt:7)`
  里 `40..56` 偏移已经变成 `ptr<load=u32, store=u32, psize=32>*`
- `[/tmp/notdec-fortune-byte-access-elide-v8/ValueHTypes.txt](/tmp/notdec-fortune-byte-access-elide-v8/ValueHTypes.txt:12)`
  的 `offset 60` 仍然保留成 `ptr<load=u8, store=u8, psize=8>*`
- `[/tmp/notdec-fortune-byte-access-elide-v8/ValueTypes.txt](/tmp/notdec-fortune-byte-access-elide-v8/ValueTypes.txt:23)`
  里 `@get_tbl` 对应字段也不再出现 `Ptr<uint & uint, uint | uint>`

## 方案评价

- 效果：8/10。当前关注用例已经回到想要的 `u32` / `u8` 分离。
- 复杂度：8/10。只改 `binarysub` 最终 primitive coalesce 和一个 size 判定点。
- 维护成本：7/10。规则现在只特判“同名 + 1-byte 被更大位宽覆盖”，边界清楚，但如果以后
  想推广到 `u16` / `u32` 这类“更小但不只 1-byte”的情况，还要再扩一层策略。

## 更好的方案

如果后面要把“忽略更小访问”做成更通用的规则，最好不要继续堆在 `print/coalesce` 末端，
而是把 `CompactType` 里同一 primitive family 的 size 归并提早到 canonical/simplify
阶段。这样 `type-struct-merge.md` 这类 sidecar 里也不会再看到 `{uint, uint}` 这种
没位宽的中间形态。

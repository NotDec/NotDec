# 2026-05-01 ValueType Primitive Width Print

## 背景

前一个改动已经把 `ValueHTypes` 里的 `u32 & u8` 消掉了，但 `ValueTypes.txt`
原来的 primitive pretty print 只打名字，不打位宽，仍然会把：

- `uint:32`
- `uint:8`

都显示成 `uint`，排查时很费劲。

## 目标

- 只改 `binarysub::printTypeImpl()` 的 primitive 输出。
- `ValueTypes.txt` / 其他 `printType()` 使用方里，把 primitive 明确打印成
  `name:size`，例如 `uint:32`、`char:8`、`sint:64`。
- 不改 solver，不改最终 `ValueHTypes` lowering 逻辑。

## 修改

文件：`external/binarysub/src/binarysub.cpp`

1. `printTypeImpl()`，`424-432`
   - `UPrimitiveType` 从只打印 `n.name`
   - 改成打印 `n.name << ":" << n.size`

## 验证

构建：

```bash
cmake --build ./build --target bin/notdec -j1
```

运行：

```bash
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.byte-access-elide-v9.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-byte-access-elide-v9
```

结果：

- 时间：`real 15.13s`
- `[/tmp/notdec-fortune-byte-access-elide-v9/ValueTypes.txt](/tmp/notdec-fortune-byte-access-elide-v9/ValueTypes.txt:23)`
  里 `@get_tbl` 已经能直接看到
  `@40..@56 -> Ptr<uint:32, uint:32>`，`@60 -> Ptr<uint:8, uint:8>`
- `[/tmp/notdec-fortune-byte-access-elide-v9/ValueTypes.txt](/tmp/notdec-fortune-byte-access-elide-v9/ValueTypes.txt:1340)`
  里 `%7 = add i32 %_arg_0, 40` 也明确显示为 `Ptr<uint:32, uint:32>`
- `[/tmp/notdec-fortune-byte-access-elide-v9/ValueHTypes.txt](/tmp/notdec-fortune-byte-access-elide-v9/ValueHTypes.txt:7)`
  仍然保持 `ptr<load=u32, store=u32, psize=32>*`

## 方案评价

- 效果：9/10。对调试 `ValueTypes` 非常直接，尤其是同名不同位宽 primitive。
- 复杂度：9/10。只改一行输出格式。
- 维护成本：9/10。格式简单稳定，下游如果有人依赖 `printType()` 的旧文本，需要同步更新。

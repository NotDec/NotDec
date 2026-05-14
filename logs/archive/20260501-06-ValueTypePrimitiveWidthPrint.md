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

## 2026-05-03 收尾

### 测试期望补齐

修改文件：`external/binarysub/src/binarysub-test.cpp`

- 第 235-243 行，函数 `test_parse`
- 第 269-275 行，函数 `test_mlsub`
- 第 288-291 行，函数 `test_top_level_polymorphism`
- 第 317-327 行，函数 `test_rec_producer_consumer`
- 第 358-364 行，函数 `test_misc`
- 第 383-400 行，函数 `test_pointer_paper` / `test_pointer_record_wrap`
- 第 690、699、894、900、908 行附近，函数 `test_utype_pretty_printing` /
  `test_primitive_semantic_lattice`

本次只改期望字符串，把旧的 `int` / `bool` / `i8` / primitive semantic 名字，
同步成现在的 `name:size` 打印结果。

### 新发现的卡死点

重跑 `./build/binarysub` 时，前半段字符串比对问题已经消失，但程序会卡在
`test_compact_recursive_size()` 这条新测试链上。

定位结果：

- 调用点：`external/binarysub/src/binarysub-test.cpp:420`，
  `TypeSimplifier::coalesceCompactType(root, false, false)`
- 卡死点：`external/binarysub/src/binarysub.cpp:1743`，
  `inProcess.find(key)`
- 根因链：
  - `PolarCompactTypeMap` 当前 key 是 `std::pair<CompactTypePtr, bool>`
  - `CompactTypePtr` 是 `value_ptr<CompactType>`
  - `value_ptr::operator<` 在
    `external/binarysub/include/binarysub/binarysub-utils.h:170-177`
    走的是“解引用后按值比较”
  - `CompactType::operator<` 在
    `external/binarysub/include/binarysub/binarysub.h:261-266`
    又会递归比较 `record/function/ptrLoad/ptrStore`
  - 新测试构造的是 `root->record["4"] = root` 的自环，所以一旦拿这个
    `CompactType` 去做 `map` 查找，比较器就会沿着自环无限递归

当前判断：后面要修的不是 `printType()`，而是 `coalesceCompactType()`
这条递归检测表的 key 比较方式。最小方向应当是把这里改成按节点身份比较，
不要再按 `CompactType` 结构值比较。

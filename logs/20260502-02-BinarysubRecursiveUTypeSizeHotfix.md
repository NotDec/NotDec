# 2026-05-02 binarysub recursive UType size 热修

## 背景

fortune 里 `add_file::%_param_1.0` 的原始 `ValueTypes.txt` 还是递归：

- `({@4: μ31} & Ptr<⊤, ⊥>) as μ31`

但到了 HType 侧，`μ31` 被当成 `size=0`，最后字段直接被跳过，退化成：

- `rec_xxx { bottom:32 & struct_11 }`
- `struct_11 { i8[4] padding_0; }`

前面已经确认 `SimpleType` 变量本身不是 0 宽；问题落在 binarysub 最后
`CompactType -> UType` 这一步。

## 目标

先做最小修补，不改整体 `CompactType.size` 设计：

1. `coalesceCompactType()` 构造递归回边占位变量时，直接继承当前
   `CompactType.size`
2. 这里先加硬断言，要求 size 存在且非 0
3. 先把 fortune 这条链打通，再决定后面要不要把 `CompactType.size`
   从 `optional` 收紧

## 实现记录

修改文件：`external/binarysub/src/binarysub.cpp`

- 第 1673 行附近，函数 `TypeSimplifier::coalesceCompactType`
- 新增局部 helper `requireCompactTypeSize`
  - 断言 `CompactType` 非空
  - 断言 `size.has_value()`
  - 断言 `*size != 0`
- 第 1760 行附近，递归回边占位 `recVarGetter`
  - 原逻辑：只有 `getOnlyVariable(*ty)` 时才从变量取 size，否则默认 0
  - 新逻辑：统一改成 `requireCompactTypeSize(ty)`

这样 synthetic `μN` 回边也会继承当前递归节点的逻辑大小，不再偷偷掉成 0。

## 测试

修改文件：

- `external/binarysub/src/binarysub-test.cpp`
  - 第 412 行附近，新增 `test_compact_recursive_size`
  - 直接构造一个 `size=32` 的自递归 `CompactType`
  - 断言 coalesce 后：
    - 顶层得到 `URecursiveType`
    - body 里回边仍是 `UTypeVariable`
    - 回边 `size == 32`
    - 回边名字和 binder 名字一致

- `external/binarysub/include/binarysub/binarysub-test.h`
  - 第 11 行附近，新增声明 `test_compact_recursive_size`

- `external/binarysub/src/binarysub-test-main.cpp`
  - 第 35 行附近，把 `test_compact_recursive_size()` 挂到现有测试主程序

## 验证

已运行：

```bash
cmake --build build --target binarysub -j4
cmake --build build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-recsize-fix-2/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-recsize-fix-2
```

结果：

- `binarysub` 编译通过
- `notdec-decompile` 已重链
- fortune 当前口径运行通过，`real 15.01s`、`user 14.67s`、`sys 0.33s`
- `ValueHTypes.txt` 中该点不再是 `rec_xxx -> struct_11` fallback 壳
  - 现在 `add_file::%_param_1.0` / `%bb.loop_entry122.i2` 的 upper 直接是
    `struct_11`
  - `struct_11` 内能看到自递归字段：`struct_11 field_0; /* at offset: 4 */`

补充说明：

- 直接运行 `./build/binarysub` 时，最前面的 parsing/pretty-print 预期本身就有旧失败
  - 例如 `int` 现在打印成 `int:32`、`bool` 打印成 `bool:1`
  - 这批失败发生在本次新增测试之前，不适合拿来判断这次热修是否正确

## 当前判断

这次热修已经把当前最关键的 0 宽回边问题压住了。

但它只是局部修补，不代表 `CompactType.size` 设计已经理顺：

1. 现在仍有不少中间 `CompactType` 片段把 `size` 当 `optional` 用
2. `coalesceCompactType()` 里还有别处在用 `value_or(0)`
3. 如果后面继续沿这条线收紧，最好把“完整类型节点必须有 size”和
   “中间空累加器可以没 size” 明确拆开

### 简评

- 实现效果：8/10
- 复杂度成本：8/10
- 维护成本：7/10

当前方案足够小，也把当前 fortune 问题直接修掉了。更彻底的下一步应该是梳理
`CompactType.size` 的语义边界，而不是继续在各个出口点补 `value_or(0)`。

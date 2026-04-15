# TypeBuilder `UTop` 字段布局崩溃修复

日期：2026-04-15

## 背景

调试 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll` 时，`notdec`
在类型恢复阶段崩溃：

```text
src/TypeRecovery/mlsub/TypeBuilder.cpp:411:
TypeBuilder::accessedPointeeSizeInBits(...)
Assertion `false && "Impossible UType variant"' failed.
```

复现命令：

```bash
build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/out.ll --tr-level=2 --dump-htypes /tmp/out.htypes \
  --gen-work-dir --work-dir=/tmp/notdec-fortune-work
```

## 定位结果

- 触发函数：
  - `src/TypeRecovery/mlsub/TypeBuilder.cpp:409-447`
  - `notdec::mlsub::TypeBuilder::accessedPointeeSizeInBits`
- 触发路径：
  - `src/TypeRecovery/mlsub/TypeBuilder.cpp:661-804`
  - `notdec::mlsub::TypeBuilder::convertStruct`
- 具体根：
  - `Value:   %21 = call i32 @malloc(i32 84) (In Func: add_file)`
  - 在该 malloc 对应 record 的字段布局里，offset `@20` 和 `@32` 出现了
    `UTop(32)`，`convertStruct()` 在计算字段大小时调用
    `accessedPointeeSizeInBits()`，而后者仍把 `UTop` 当成“不可能出现”直接断言

这说明当前问题不是 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md`
对应的 primitive semantic lattice 设计本身导致的：

- 本次复现命令没有传 `--primitive-semantic-lattice`
- gdb/trace 抓到的实际类型文本是 `⊤`，不是 `prim.*` semantic primitive

更接近的回归来源是近期 `TypeBuilder` 对 `top/bottom` 保留策略扩展后，
旧的字段尺寸推导分支没有同步覆盖 `UTop`。

## 修改

### 1. 让 `accessedPointeeSizeInBits()` 接受带已知位宽的叶子类型

文件：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:409-420`

涉及函数：

- `notdec::mlsub::TypeBuilder::accessedPointeeSizeInBits`

修改内容：

- `UTop` 改为返回 `V->size`
- `UPrimitiveType` 改为返回 `V->size`
- `UFunctionType` 改为返回 `binarysub::get_size(Ty)`

这样 `convertStruct()` 在遇到“字段类型未知但位宽已知”的 record entry 时，
可以继续完成 layout，而不是因为旧断言直接中止。

### 2. 新增一个最小回归测试覆盖 `UTop` record field

文件：

- `unittests/Retypd/GraphTest.cpp:8`
- `unittests/Retypd/GraphTest.cpp:99-125`

涉及测试：

- `TEST(Retypd, TypeBuilderTopFieldRecordLayoutTest)`

修改内容：

- 新增 `binarysub/binarysub.h` include，直接构造：
  - `{@0: UTop(32), @4: uint32}`
- 调用 `TypeBuilder::convert()`
- 断言结果是 record pointer，且 `@0` 字段被保留为 `TopType`

## 验证

执行：

```bash
cmake --build build --target notdec-decompile GraphTest -j4
build/bin/GraphTest --gtest_filter='Retypd.TypeBuilderSemanticPrimitiveAliasTest:Retypd.TypeBuilderTopFieldRecordLayoutTest'
build/bin/notdec test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.1.ll \
  -o /tmp/out.ll --tr-level=2 --dump-htypes /tmp/out.htypes \
  --gen-work-dir --work-dir=/tmp/notdec-fortune-verify
```

结果：

- 两个 `GraphTest` 通过
- `fortune.o3.wasm.1.ll` 不再在 `TypeBuilder::accessedPointeeSizeInBits()` 断言
- `notdec` 能完成运行并产出 `.ll` / `.htypes`

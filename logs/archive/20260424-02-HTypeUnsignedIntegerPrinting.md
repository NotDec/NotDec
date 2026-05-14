# 2026-04-24 HType unsigned 整数打印区分 signed / unsigned

## 背景

这次先处理的是输出里的“假重复”：

- `fortune.o3.wasm.ll` 的 `ValueHTypes.txt` / `ImportantHTypes.txt`
  里有很多
  - `i8 & i8`
  - `i8 | i8`

先核对后发现，其中一部分不是集合里真有两个完全相同的分量，而是：

- `UType` 里还是 `char & uint`
- 到 `HType` 打印时，`signed` / `unsigned` 都统一打印成 `iN`

这样 `char(8)` 和 `uint(8)` 在文本上撞成了同一个 `i8`，看起来像重复项。

本次只修这个打印层问题，不改 `UType` / `HType` 内部结构。

## 代码修改

1. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:15-21`
   - 涉及函数：
     - 新增 helper `formatIntegerTypeName`
   - 修改内容：
     - 统一封装整数类型文本格式
     - `1-bit` 继续打印为 `bool`
     - 其他整数改为：
       - signed: `iN`
       - unsigned: `uN`

2. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:188-196`
   - 涉及函数：
     - `HType::getAsString(int)`
   - 修改内容：
     - `TK_Integer` 不再固定打印 `i<bits>`
     - 改为走 `formatIntegerTypeName`

3. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:470-479`
   - 涉及函数：
     - `HTypeSnapshotFormatter::formatType(const HType *, int)`
   - 修改内容：
     - snapshot / golden 路径同步使用 `formatIntegerTypeName`
     - 保证常规打印和 snapshot 打印一致

4. `unittests/Retypd/TypeBuilderTest.cpp:104-123`
   - 涉及函数：
     - 新增测试 `Retypd.TypeBuilderSignedUnsignedByteSetPrinting`
   - 修改内容：
     - 构造 `char(8) & uint(8)` 的最小 `UType`
     - 断言 `TypeBuilder` 转出来的 `HType` 文本是：
       - `i8 & u8`
     - 同时断言 snapshot formatter 输出也一致

## 验证

1. 构建
   - 命令：
     - `cmake --build /sn640/NotDec/build --target notdec-decompile -j2`
     - `cmake --build /sn640/NotDec/build --target TypeBuilderTest -j2`
   - 结果：通过

2. 单测
   - 命令：
     - `./build/bin/TypeBuilderTest --gtest_filter='Retypd.TypeBuilderSignedUnsignedByteSetPrinting:Retypd.HTypeSetPrettyPrintingFlattensChains:Retypd.TypeBuilderSemanticPrimitiveAliasTest'`
   - 结果：3 个测试全部通过

3. `fortune` 定点验证
   - 命令：
     - `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune-unsigned-fix2.out.ll --tr-level=2 -g --work-dir=/tmp/notdec-fortune-unsigned-fix2`
   - 结果：
     - `@2288` 对应项已从
       - `ptr<load=ptr<load=i8 & i8, store=i8, psize=8> & struct_4*, ...>`
       变成
       - `ptr<load=ptr<load=i8 & u8, store=i8, psize=8> & struct_4*, ...>`
     - 同类项也同步变成：
       - `i8 & u8`
       - `i8 | u8`
       - `u32 & u8`
       - `u32 | u8`

## 备注

这次修掉的是“打印碰撞导致的假重复”。

如果后面还看到像 `i8 & i8` 这种输出，需要继续区分：

- 是真的两个等价 signed 分量都保留下来了
- 还是别的打印碰撞

这两个问题不能混在一起处理。

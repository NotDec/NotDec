# 2026-05-02 TypeBuilder recursive anchor 与 `URecordType` 缓存修正

## 背景

原计划标题偏向“指针语义修正”，但这次实际没有重写整体指针语义。
真正修的是 recursive anchor 选择，以及 `URecordType` 在对象视角和地址视角之间的缓存复用问题。

之前把 `URecordType` 当成结构体对象来收口，这是错的。

重新看 `binarysub` 语义后，当前应按下面理解：

- `URecordType` 表示“结构体地址/结构体指针”
- `HType::RecordType` 表示 `struct X` 对象
- `HType::PointerType(RecordType)` 表示 `struct X *`
- `URecordType` 的字段类型也是地址语义，字段视角要投影成字段值类型

所以 `convert(URecordType)` 返回 `struct*` 是合理的；真正的问题不在这里。

## 问题

这次主要发现三处不一致：

1. `getStructOrNull()` 只认缓存里的 `RecordType`
   - 但 `URecordType` 普通转换后缓存的是 `PointerType(RecordType)`
   - 同一个 `URecordType` 后续从字段视角再转换时，可能找不到已有 record decl，重复造结构体

2. `convertRecursive()` 没处理 `struct*` body 的 anchor
   - `convert(T.body)` 如果得到 `struct*`，旧代码不会把 pointee record 当 anchor
   - 它会进入 fallback，额外造一个单字段 `rec_xxx` 结构体
   - `work_dir/ValueHTypes.txt` 里的 `rec_3378` 就是这种形态

3. `convertRecursive()` 也没处理 set 里的 `struct*`
   - fortune 里常见形态不是裸 `struct*`
   - 而是 `bottom/top/typevar & struct*`
   - anchor 查找要能穿过 `SetUnionType` / `SetInterType`

## 目标

保留 `URecordType` 的指针语义，只修不一致的地方：

1. `convert(URecordType)` 继续产出 `struct*`
2. 字段视角继续把 `URecordType` 转成结构体对象字段
3. 结构体 decl 复用要能透过缓存里的 `struct*` 找到 pointee record
4. 递归 body 是 `struct*` 或 set 里包含 `struct*` 时，anchor 挂到 pointee record，不再制造多余 fallback 壳

## 实现记录

### 已完成：缓存里的 `struct*` 可复用 pointee record

修改文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`

- 第 491 行附近，函数 `TypeBuilder::getStructOrNull`
- 原逻辑只在缓存值是 `RecordType` 时返回 decl
- 新逻辑增加 `PointerType` 分支：
  - 如果缓存值是 `struct*`
  - 且 pointee 是 `RecordType`
  - 返回 pointee 的 `RecordDecl`

这样 `TypeCache[URecordType]` 即使保存的是 `struct*`，布局复用仍能找到原 record decl。

### 已完成：普通 `convert(URecordType)` 不泄漏内部 record 缓存

修改文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`

- 第 864 行附近，函数 `TypeBuilder::convert`
- `convertStruct()` / struct-merge 可能为了复用 decl，把 `URecordType` 暂时缓存成 `RecordType`
- 新逻辑在 cache hit 时检查：
  - 如果当前 `Ty` 是 `URecordType`
  - 且缓存值是 `RecordType`
  - 对外返回 `PointerType(RecordType)`

这样字段视角留下的内部对象缓存，不会破坏普通 `URecordType -> struct*` 语义。

### 已完成：递归 body 从 set 中寻找 pointee anchor

修改文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`

- 第 166 行附近，新增 `findRecursiveAnchorDecl`
  - 直接 record/union：返回自身 decl
  - pointer：返回 pointee record/union decl
  - set union/intersection：递归扫描 set terms

- 第 818 行附近，函数 `TypeBuilder::convertRecursive`
  - 原逻辑只认 body 本身是 record/union
  - 新逻辑改为调用 `findRecursiveAnchorDecl(Body)`

这样 `URecursiveType` body 是 `bottom & struct*` 这类形态时，也能使用 `struct*` 指向的结构体作为 anchor。

### 已完成：更新 TypeBuilder 单测

修改文件：`unittests/Retypd/TypeBuilderTest.cpp`

- 第 54 行附近，更新 `TypeBuilderTopFieldRecordLayoutTest`
  - 现在明确断言 `TB.convert(URecordType)` 是 `PointerType`
  - 再检查 pointee 是 `RecordType`
  - 原有字段布局检查改为检查 pointee record

- 第 86 行附近，新增 `TypeBuilderRecursiveRecordSetAnchorsPointeeRecord`
  - 构造 `μr.(bottom & {@0: r})`
  - 断言 recursive body 是 set intersection
  - 断言 set 中的 `struct*` pointee 被选为 recursive anchor
  - 防止以后又退回单字段 fallback 壳

## 验证

已运行：

```bash
cmake --build ./build --target TypeBuilderTest -j4
./build/bin/TypeBuilderTest
cmake --build ./build --target notdec-decompile -j4
NOTDEC_POINTER_ANALYSIS_MODE=original \
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-structmerge-hlayout-final/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-structmerge-hlayout-final
NOTDEC_POINTER_ANALYSIS_MODE=original \
ctest --test-dir build -R notdec.type_recovery.realworld.tr_level_2 \
  --output-on-failure
```

结果：

- 5 个 TypeBuilder 相关测试全部通过
- fortune 当前口径通过，`real 15.19s`、`user 14.74s`、`sys 0.45s`
- `ValueHTypes.txt` 中第一处原 `rec_3378`/`rec_3329` fallback 壳消失，变成 `struct_3` 内直接回指 `struct_3`
- realworld CTest 仍失败
  - 失败点是 oracle compare，主要是 `@File_list` / `free_desc::arg0` / `matches_in_list::arg0` 等
  - 报告位置：`build/test/artifacts/notdec.type_recovery.realworld.tr_level_2/fortune.o3.wasm.compare.md`
  - 这条 CTest 使用 `--dump-htypes` 和 suite extra constraints，检查口径不同于上面的手动性能口径
  - 本次没有继续扩大到 suite oracle 或 compare 逻辑

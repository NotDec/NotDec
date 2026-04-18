# 2026-04-18 TypeBuilder PointeeSize Crop

日期：2026-04-18

## 目的

针对 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll`
里 `main` 的 `@0+4i` case，做一个保守修复：

1. 当 `TypeBuilder::convertStruct()` 已知 `PointeeSize` 时，
   优先满足这个边界
2. 超出 `PointeeSize` 的 field 直接裁掉或跳过
3. 避免再把 4-byte 元素强行合成为 8-byte layout

对应之前的 trace 问题：

- [work_dir/04-typebuilder-convertstruct.log:60](/sn640/NotDec/work_dir/04-typebuilder-convertstruct.log#L60)
  到 [work_dir/04-typebuilder-convertstruct.log:71](/sn640/NotDec/work_dir/04-typebuilder-convertstruct.log#L71)
  中，`pointer-record(fields=1) -> stride(4, 0..4)` 这层
  会把 `@0+4i` / `@4+4i` 一起收进来，最终触发
  `pointee/layout mismatch`

## 具体修改

### 1. 新增裁剪 helper

在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:204](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L204)
到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:241](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L241)
新增匿名 namespace 辅助函数 `cropFieldTypeToRange`

- 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
- 函数：`cropFieldTypeToRange`
- 作用：
  - 对已生成的 `HType` 做保守裁剪
  - 优先保留可精确裁剪的数组
  - 不能保持原 shape 时，回退为 `char[n]` byte blob

### 2. 让 `craftStruct()` 在 `ValidRange` 下真正裁 field

在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:638](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L638)
到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:736](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L736)
调整 `TypeBuilder::craftStruct`

- 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
- 函数：`TypeBuilder::craftStruct`
- 具体行为：
  - 完全落在 `ValidRange` 外的 field，直接跳过，不再断言
  - 部分重叠 `ValidRange` 的 field，通过 `cropFieldTypeToRange()`
    做保守裁剪
  - 数组扩张逻辑改为基于裁剪后的 `CurrentDecl.R`，避免把已经裁小的
    field 又按旧范围扩回去

### 3. 在 `convertStruct()` 的 stride field 阶段优先按 `PointeeSize` 剪枝

在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:888](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L888)
到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:956](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L956)
调整 `TypeBuilder::convertStruct`

- 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
- 函数：`TypeBuilder::convertStruct`
- 具体行为：
  - `stride` 分组若 `RangeStart >= PointeeSize`，直接不生成该 field
  - 若 stride field 尾部越界，则先把 `FieldSize` 截到
    `PointeeSize - RangeStart`
  - 截断后的 partial stride field 不再强制包成 unsized array，
    直接落为裁剪后的 member type

### 4. 在 `convertStruct()` 的非数组 field 阶段提前截断 payload

在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:960](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L960)
到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:986](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L986)
调整 `TypeBuilder::convertStruct`

- 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
- 函数：`TypeBuilder::convertStruct`
- 具体行为：
  - `RemainingEntries` 转换为 `Fields` 时，
    若 `offset + payload_size > PointeeSize`，则先截断 field size
  - 若 field 已完全超界，则直接跳过

### 5. mismatch 时不再断言，直接收缩到 `PointeeSize`

在 [src/TypeRecovery/mlsub/TypeBuilder.cpp:1188](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L1188)
到 [src/TypeRecovery/mlsub/TypeBuilder.cpp:1206](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp#L1206)
调整 `TypeBuilder::convertStruct`

- 文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`
- 函数：`TypeBuilder::convertStruct`
- 具体行为：
  - 保留 mismatch trace
  - 但不再 `assert(PointeeSize < Size)`
  - 而是直接令最终 layout size 收缩到 `PointeeSize`

## 验证

### 1. 构建

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：

- 编译通过

### 2. 复跑聚焦 case

执行：

```bash
env NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
  NOTDEC_DISABLE_INTERPROC=1 \
  NOTDEC_SAT_DISABLE=1 \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/fortune.o3.wasm.4.out.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune.o3.wasm.4.out.htypes \
  -g \
  --work-dir=/sn640/NotDec/work_dir
```

结果：

- 进程成功结束，输出
  `/tmp/fortune.o3.wasm.4.out.ll`
  和 `/tmp/fortune.o3.wasm.4.out.htypes`
- `main` 的关键 trace 已变为：
  - [work_dir/04-typebuilder-convertstruct.log:64](/sn640/NotDec/work_dir/04-typebuilder-convertstruct.log#L64)
    `field(@0, bytes=4) -> record(fields=2, field_bytes=4)`
  - [work_dir/04-typebuilder-convertstruct.log:67](/sn640/NotDec/work_dir/04-typebuilder-convertstruct.log#L67)
    只剩下 `stride(4, 0..4)` 这一个 in-range 子字段
- 原先那条
  `... record(fields=2, field_bytes=4) ... pointee/layout mismatch`
  已不再出现
- 最终 `@main` 对应的聚焦结构保持为只保留第一个 4-byte 槽位：
  - [/tmp/fortune.o3.wasm.4.out.htypes:127](/tmp/fortune.o3.wasm.4.out.htypes#L127)
    到 [/tmp/fortune.o3.wasm.4.out.htypes:140](/tmp/fortune.o3.wasm.4.out.htypes#L140)
  - 其中 [/tmp/fortune.o3.wasm.4.out.htypes:139](/tmp/fortune.o3.wasm.4.out.htypes#L139)
    到 [/tmp/fortune.o3.wasm.4.out.htypes:141](/tmp/fortune.o3.wasm.4.out.htypes#L141)
    的 `struct_14` 只剩：

```text
struct struct_14 {
  'p92:32 field_0; /* at offset: 0 */
};
```

## 结论

这次修复是一个“优先满足 `PointeeSize`”的保守裁剪版本：

1. 越界 field 不再硬塞进最终 layout
2. 当前 `fortune.o3.wasm.4.ll` 的 `main` case 已能稳定跑通
3. 对无法精确保型的部分裁剪，当前策略会退化成 byte blob，而不是继续生成
   超界 aggregate

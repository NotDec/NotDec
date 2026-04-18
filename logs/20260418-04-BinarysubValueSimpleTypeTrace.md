# 2026-04-18 Binarysub Value SimpleType Trace

## 背景

`work_dir/binarysub-trace.log` 之前只有 `binarysub` / `PNDiff` 的约束与求解事件，
缺少 `LLVM Value -> SimpleType` 的映射建立日志。调试
`test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll` 时，不容易从
trace 里直接看出某个 `ExtValuePtr` 最初被绑定到了哪个 `SimpleType`，以及后续
有没有发生 remap / merge。

## 本次修改

1. 在 `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:71`、
   `:123-128`、`:132`、`:166` 为 `notdec::mlsub::ConstraintsGenerator`
   增加并声明了 mapping trace 相关接口：
   - `addMergeNode`
   - `emitMappingTrace`
   - `emitRemapTrace`
   - `emitMergeTrace`
   - `createNode`
   - `addRemapType`

2. 在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1350-1369` 新增
   `sanitizeTraceText` 与 `formatExtValueMappingLabel`，用于把
   `toStableString()` / `toString(..., true)` 组合成单行 trace 标签。

3. 在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1608-1708` 实现
   `ConstraintsGenerator` 的 mapping trace：
   - `emitMappingTrace`
   - `emitRemapTrace`
   - `emitMergeTrace`
   - `addMergeNode`
   - `createNode`
   - `addRemapType`

4. 在 `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1757-1760` 为
   `MLsubRecovery::run` 写入 trace 文件头注释，明确说明
   `[mapping:*]` 用于记录 value/simple-type binding。

## 验证

编译：

```bash
cmake --build build --target notdec-decompile -j2
```

运行：

```bash
NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
NOTDEC_TYPEBUILDER_TRACE_CONVERTSTRUCT=1 \
NOTDEC_BINARYSUB_TRACE=1 \
./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/fortune.o3.wasm.4.out.ll \
  --tr-level=2 -g --work-dir=/sn640/NotDec/work_dir
```

结果：

- `work_dir/binarysub-trace.log` 开头已出现
  `[mapping:create] value=@malloc ... simple=vs#1[...]`
  这类记录
- 中段已出现
  `[mapping:remap] value=main::%bb.brif_next241.i0 ... target=...`
  这类记录
- 本次运行后续仍在
  `src/TypeRecovery/mlsub/TypeBuilder.cpp:1125`
  的 `TypeBuilder::convertStruct` 断言处终止；这是已有 case 问题，不是这次
  trace 改动引入的新编译错误

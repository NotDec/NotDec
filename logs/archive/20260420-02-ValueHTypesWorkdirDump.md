# 2026-04-20 ValueHTypes workdir dump

## 背景

当前 `work_dir` 已有 `ValueTypes.txt`，但它记录的是 `IR Value -> binarysub UType`。
这次补一个最终态的 `ValueHTypes.txt`，从 `HTypeResult` 角度直接看
`IR Value -> HType`，便于在类型恢复结束后继续排查：

1. `ValueTypes.txt` 的 `UType` 是否已经合理
2. `UType -> HType` 转换后是否发生了意料外的收窄/扩张
3. 最终 record / memory canonical 名在 `HTypeResult` 里是什么

## 修改

1. [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L53)
新增 `kValueHTypesFile = "ValueHTypes.txt"` 常量。

2. [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1340)
新增 `writeDebugValueHTypes()` 函数。
这个函数直接把最终 `llvm2c::HTypeResult` 调 `print()` 落到
`work_dir/ValueHTypes.txt`，避免再写一套独立的 HType snapshot 格式。

3. [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1851)
在 `MLsubRecovery::run()` 里新增：
当开启 `work_dir` 且 `ResultVal` 还没 materialize 时，主动调用
`genASTTypes(M)`。
这是为了覆盖 `.ll` 输出链路；否则之前只有 `--dump-htypes` 或 `.c` 后端
真正取 `HTypeResult` 时，`genASTTypes()` 才会执行，`ValueHTypes.txt`
不会生成。

4. [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2357)
在 `MLsubRecovery::genASTTypes()` 汇总 `ResultVal->ValueTypes`、
`ResultVal->ContraVariantValues`、`ResultVal->MemoryType` / `MemoryDecl`
后，如果存在 `work_dir`，调用 `writeDebugValueHTypes()` 导出最终快照。

涉及函数：

- `writeDebugValueHTypes()`
- `MLsubRecovery::run()`
- `MLsubRecovery::genASTTypes()`

5. [DEBUG.md](/sn640/NotDec/DEBUG.md#L131)
新增 `ValueHTypes.txt` 说明，明确来源、用途，以及它复用
`HTypeResult::print()` 的 snapshot 格式。

## 验证

使用当前关注 case：

```bash
/sn640/NotDec/build/bin/notdec \
  /sn640/NotDec/test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-valuehtypes-check/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-valuehtypes-check
```

验证结果：

1. `ValueHTypes.txt` 已生成于 `/tmp/notdec-valuehtypes-check/ValueHTypes.txt`
2. 文件包含 `# HTypeResult`、`[decls]`、`[types]`、`[memory]` section
3. 与现有 `ValueTypes.txt` 并存，可用于对比最终 `UType` 与 `HType` 结果

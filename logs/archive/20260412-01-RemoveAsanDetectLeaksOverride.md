# Remove `ASAN_OPTIONS=detect_leaks=0` Defaults

## Background

仓库里曾在类型恢复 suite、调试配置和 `external/binarysub` 测试入口中默认关闭 LeakSanitizer 检查，用来避免“功能正确但因泄漏失败”的噪声影响回归。

当前判断这些泄漏问题已修复，因此本次把这些默认覆盖移除，让 ASan/LSan 恢复为正常失败语义。

## Changes

- 删除 `test/type-recovery/llvm-ir/manifest.json` 中的 `ASAN_OPTIONS`
- 删除 `test/type-recovery/sysy/manifest.json` 中的 `ASAN_OPTIONS`
- 删除 `test/type-recovery/howard-o3-split/manifest.json` 中的 `ASAN_OPTIONS`
- 删除 `.vscode/launch.json` 里 `dec` 配置的 `ASAN_OPTIONS`
- 删除 `external/binarysub/src/binarysub-test-main.cpp` 中的 `__asan_default_options()`
- 更新 `AGENTS.md`，移除过时的 ASAN 过渡说明，并补充“只保留当前有效信息、历史放到 logs”这一维护原则
- 更新历史日志，避免后续复制旧命令时继续带上该环境变量

## Verification Plan

- 重新构建受影响目标
- 运行 `binarysub` 测试，确认没有依赖默认 `detect_leaks=0`
- 运行类型恢复 CTest，确认 suite 在未设置该环境变量时仍可通过

## Verification Result

- `cmake --build build --target binarysub` 成功
- `./build/binarysub` 成功，当前在默认 LeakSanitizer 下全部通过
- 剩余真实问题确认出在 `binarysub` 测试/推导清理路径，而不是 `notdec` 类型恢复 suite 本身必须依赖 `ASAN_OPTIONS`
- `ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy|howard_o3_split).tr_level_2' --output-on-failure`
  仍然失败，但失败原因为既有 golden mismatch / XPASS 基线漂移：
  - `llvm_ir`: `05_MultiOffset`、`09_OffsetLoop`、`11_SimpleRecursive1`、`16_Poly1`
  - `sysy`: `26_while_test1`、`34_arr_expr_len`、`73_int_io` 目前为 `XPASS`
  - `howard_o3_split`: 通过

## Root Cause

`external/binarysub` 之前虽然已经引入 `release_type_graph()`，但没有把它接到实际生命周期里：

- 测试成功路径在 `TypeSimplifier`/打印后没有释放推导根
- 递归 `typeLetRhs` 的失败分支没有清理局部图
- `TypeScheme::instantiate()` freshen 出来的临时实例图、内层 `let` 绑定图，以及应用表达式失败分支创建的临时图，没有统一在 `Typer` 生命周期结束时回收

本次修复把这些路径接上后，`binarysub` 已经可以在不设置 `ASAN_OPTIONS=detect_leaks=0` 的情况下通过。

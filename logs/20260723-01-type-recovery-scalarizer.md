# 原始 prompt

是的，插一下吧，然后再看看因为字段冲突导致没有合并的情况

# 实现记录

## 背景

fortune 的源 IR 里有 `load <4 x i32>` / `llvm.bswap.v4i32` / `store <4 x i32>`。MLsub 直接按 load/store 的 LLVM 类型生成 `load[128]` / `store[128]`，导致 `FILEDESC::tbl` 在 offset 64 被看成一个 16 字节字段片段，和其它 4 字节字段证据冲突。

## 修改

- `src/Passes/PassManager.cpp`
  - 第 39 行引入 `llvm/Transforms/Scalar/Scalarizer.h`。
  - 第 316-324 行在 `PassEnv::add_type_recovery_passes()` 的 `MLsubRecoveryMain` 前插入 `ScalarizerPass`。
  - 打开 `ScalarizerPassOptions::ScalarizeLoadStore`，让 fixed vector load/store 在 MLsub 约束生成前拆成标量访问。

## 期望效果

`llvm.bswap.v4i32` 这种 SLP 形状会在类型恢复前变成多个 `llvm.bswap.i32` 和多个 `i32` load/store。MLsub 不再生成 `@64:16` 的字段证据，字段合并策略可以看到 `@64:4`、`@68:4`、`@72:4`、`@76:4` 这类真实标量访问。

## 验证

- 构建：`cmake --build ./build --target notdec -j4` 通过。
- 回归：`ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure` 通过。
- fortune 运行目录：`/tmp/notdec-fortune-scalarizer-20260723-2`。
- `work/02-mlsub-input.ll` 里 `get_tbl` 的 `<4 x i32>` load/store 已变成 4 个 `i32` load/store；`00-lifted.ll` 保留原始 vector IR，用来对照。
- `work/binarysub-trace.log` 中没有 `record{load[128]` / `record{store[128]`，也没有 `overlap-conflict`。
- `eval/merge-eval-summary.json`：`bad_unions=0`，`fragmented_nodes=14`，`wall_ms=30494`，`peak_rss_mb=1932`。
- 旧对照运行 `/tmp/notdec-fortune-callarg-trace-20260723-1`：`bad_unions=0`，`fragmented_nodes=16`，`wall_ms=63721`，`peak_rss_mb=5683`，trace 中有 `@64:16` 与 `@68:4` 的 overlap conflict。

## 评估

- 实现效果：8/10。直接消除了 fortune 当前由 vector load/store 引起的字段重叠冲突，并让 `get_tbl` 相关 struct pointer 参数合并成功。
- 复杂度：2/10。复用 LLVM 标准 pass，只增加一个 pipeline 步骤。
- 维护成本：3/10。后续如果某些 vector 形状不能被 Scalarizer 拆开，需要单独确认，但当前没有自定义 IR rewrite 负担。

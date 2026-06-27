# 原始 prompt

> NotDec2那边有一些未commit的修复，好像是修了一个小问题，看看是不是有用的话单独合并，在主worktree这边作为一个commit，然后NotDec2那边就直接切到和主worktree完全一致

# 背景

`/sn640/NotDec2` 有两处未提交改动：

- 顶层 `src/Passes/PassManager.cpp`：`--tr-level=0` / `disableTypeRecovery` 输出 C 时，不再强制解引用 `TR`。
- 子模块 `external/binarysub`：给 canonicalize 阶段加一个默认关闭的超时预算环境变量。

这两处都还没有进入主 worktree。顶层改动是明确的小 bug 修复；binarysub 改动是防止 canonicalize 长时间卡住的保守开关，默认不改变现有路径。

# 改动

子模块 `external/binarysub`：

- `external/binarysub/include/binarysub/binarysub.h:559`：
  `TypeSimplifier` 新增 canonicalize deadline / timeout 状态。
- `external/binarysub/include/binarysub/binarysub.h:581`：
  新增 `setCanonicalizeTimeoutMs()` 和 `canonicalizeBudgetExpired()`。
- `external/binarysub/include/binarysub/binarysub.h:631`：
  `isClear()` 和 `clear()` 纳入 timeout 状态，避免复用 simplifier 时残留预算。
- `external/binarysub/src/binarysub.cpp:1113`：
  实现 timeout 设置和预算检查，超时时只打一次 trace。
- `external/binarysub/src/binarysub.cpp:1332`：
  `canonicalizeType()` 的 closure、`go0`、`go1` 递归路径定期检查预算。
- `external/binarysub/src/binarysub.cpp:2690`：
  新增 `NOTDEC_BINARYSUB_CANONICALIZE_TIMEOUT_MS` 解析。
- `external/binarysub/src/binarysub.cpp:2723`：
  `TypeSimplifier::bulkSimplifyDetailed()` 在 canonicalize 前设置预算；超时后跳过剩余 root。

顶层：

- `src/Passes/PassManager.cpp:131`：
  `MLsubNotdecLLVM2C` 持有 `mlsub::MLsubRecovery *`，允许关闭类型恢复时为空。
- `src/Passes/PassManager.cpp:148`：
  只在未关闭类型恢复时断言并读取 `TR->getResult()`。
- `src/Passes/PassManager.cpp:412`：
  `PassEnv::add_llvm2c()` 在 `disableTypeRecovery` 时传 `nullptr`，避免无意义解引用。

# 验证

```bash
cmake --build build --target notdec binarysub -j4
./build/binarysub
./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-no-tr.c --tr-level=0
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-notdec2-merge.c --tr-level=2
```

结果：

- 构建通过。
- `binarysub` 全部测试通过。
- `--tr-level=0` fortune 输出 C 成功，确认关闭类型恢复路径不再因为 `TR` 解引用崩溃。
- fortune `tr-level=2` smoke 通过，`elapsed=199.54 user=227.23 sys=1.64 maxrss=1279608`。

# 影响判断

顶层修复只影响 C 输出且关闭类型恢复的路径。

binarysub timeout 默认关闭，正常类型恢复路径不应该改变；只有显式设置
`NOTDEC_BINARYSUB_CANONICALIZE_TIMEOUT_MS` 时才会用不完整 canonicalize 结果继续后续流程。

性能风险低。fortune 同口径时间没有明显退化。

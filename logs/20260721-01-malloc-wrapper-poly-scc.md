原始 prompt：

> 对，按照这种方式仅识别通用的malloc wrapper，得在划分SCC之前，把识别出来的malloc wrapper标记为多态。识别到任何这种malloc wrapper的话，还是得打印或者运行日志里说明一下

# malloc wrapper 识别并作为 SCC 多态边界

## 背景

fortune 里的 `do_malloc` 是通用 allocator wrapper：调用 `malloc`，判空失败后 `exit`，正常路径直接返回 `malloc` 结果。之前它没有 summary / 多态标记，会留在主 SCC 里。

本次只处理这种通用 wrapper，不识别 `new_fp` 这类会初始化结构体字段的 factory。

## 实现

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:499` 给 `DetectedMallocWrappers` 加注释，说明它用于 `prepareSCC()` 前建立 summary 边界。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:554` 声明 `MLsubRecovery::detectMallocWrappers()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2311` 新增 allocator 名称判断，仅接受 `malloc` / `calloc`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2356` 新增 `blockCanReachReturn()`，用于判定显式 null/error 分支是否还能回到正常 `ret`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2392` 新增 `MallocWrapperUseChecker`，只允许 allocator 结果被本地转发、判空、返回；遇到 GEP、store-through、普通 call 会拒绝。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2593` 新增 `getGenericMallocWrapperAllocator()`，要求函数返回 pointer、只有一个 allocator call、所有正常 `ret` 都由该 call 支配并返回该结果。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3366` 在 CallGraph 构建和 `prepareSCC()` 之前调用 `detectMallocWrappers()`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3488` 实现 `detectMallocWrappers()`：命中后写 `KIND_MLSUB_POLYMORPHIC_FUNCTION` metadata，stderr 打印，并在 workdir 写 `MallocWrappers.txt`。

## 验证

构建：

```bash
cmake --build ./build --target notdec -j4
```

结果：通过。

fortune：

```bash
./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll \
  -o /tmp/notdec-fortune-20260721-malloc-wrapper-120619.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-fortune-work-20260721-malloc-wrapper-120619 \
  --merge-eval-dir=/tmp/notdec-merge-eval-fortune-20260721-malloc-wrapper-120619
```

结果：

- stderr 有 `Info: detected generic malloc wrapper: do_malloc via malloc`。
- `/tmp/notdec-fortune-work-20260721-malloc-wrapper-120619/MallocWrappers.txt` 记录 `do_malloc	malloc`。
- `/tmp/notdec-fortune-work-20260721-malloc-wrapper-120619/SCCs.txt` 中 `do_malloc,exit,__fprintf_chk` 为 level 1，不再在主 SCC0；`malloc` 为 level 2。
- `Constraint generation done! SCC count:11`。

## 评分

- 实现效果：8/10。已解决 `do_malloc` 这类通用 wrapper 的 SCC 边界问题，且没有识别 `new_fp`。
- 复杂度：6/10。新增了一段保守 use 检查，理解成本可控，但比单纯名字匹配高。
- 维护成本：6/10。规则集中在一个 cpp helper 里，后续扩展 wrapper 形态比较直接；风险主要是 LLVM IR 形状更多时需要补 local forwarding。

## 追加：allocator 白名单扩展到 realloc/strdup 家族（2026-08-13）

tmux 自带 `xrealloc`/`xreallocarray`/`xrecallocarray`/`xstrdup`，分别调
`reallocarray`/`recallocarray`/`strdup`；原白名单只有 `malloc`/`calloc`，这四
个 wrapper 不会被识别。`isMallocWrapperAllocator()`
（`src/TypeRecovery/mlsub/MLsubGenerator.cpp:2547`）的白名单加入
`realloc`/`reallocarray`/`recallocarray`/`strdup`。use-checker 不变，仍会拒绝
任何初始化/使用返回内存的 factory，因此只有和 xmalloc 相同形状的纯转发
wrapper 才会被标记多态。

验证：

- tmux 全量检测：6 个 wrapper 全部命中（xmalloc/xcalloc/xrealloc/xreallocarray/
  xrecallocarray/xstrdup），写进 workdir `MallocWrappers.txt`。
- fortune / memcached merge-eval 回归：指标与历史一致（fortune bad_unions=0
  frag=8 cov 2.85%；memcached bad_unions=1 frag=133），白名单扩展没有改变
  这两个项目的结果。

注意：tmux 里这些 wrapper 都在巨型 SCC0 内，多态标记不会拆 SCC，只影响
per-callsite 实例化；tmux 的时间/内存瓶颈另有其因（见
[20260812-02](./20260812-02-tmux-analyze-constraint-gen-bisection.md)）。

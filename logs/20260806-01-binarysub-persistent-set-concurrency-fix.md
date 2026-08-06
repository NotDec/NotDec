# binarysub PersistentSet 并发竞争修复

## 背景

notdec 侧（另一个调用 binarysub 的类型推理实现）在 canonicalize 并行（TBB、`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL`）下复现三类问题：

1. `PersistentSet.h:203` `assert(!isRecycled(released))`：节点被标记 RecycledMask 后又再次 release。
   两线程失败、单线程成功、两线程但关 canonicalize 并行成功。
2. `binarysub.cpp:1452` `assert(base->size() <= vars.size())`：CompactTypeBuilder 变量集不变量。
3. `CompactVarSet::Storage::cachedHash` / `CompactType::cachedHash` 无锁读写 `std::optional`，UB。

根因（已确认）：canonicalize 并行时所有 worker 共享同一 `TypeSimplifier::compactTypes` 的
`varState`（`CompactVarSet::ArenaState`）。`PersistentSet::Factory` 的 refCount / freeNodes /
createdNodes / 节点 arena 全部无锁，而 `ArenaState::mutex` 只覆盖集合构造路径；`PersistentSet`
的拷贝与析构经 `retainRoot`/`releaseRoot`（PersistentSet.h:435/442）直接访问 Factory，不受锁
保护。锁外操作实际不止 releaseRoot/retainRoot：builder 的 `rememberReusableVars` 拷贝
`rhs->vars`（retainRoot）、`freeze` 的 `makeVarSetWithBase` 拷贝 `base->storage->set`、
builder 析构 `reusableVars`（releaseRoot）、`vars.insert(rhs->vars.begin(), ...)` 遍历共享树。
`35c39e9`（hash-cons）使 `makeImpl` 命中时按值传入的局部 CompactVarSet 在锁外析构 release
共享 root，把竞争变成高频，但它是放大因素不是根因。

## 修改内容

### external/binarysub/include/binarysub/detail/PersistentSet.h

- `Node::refCount`（53 行）：`std::uint32_t` → `std::atomic<std::uint32_t>{0}`。
- `Factory::add`（145-157 行）：整体持 `mutex`，内部改调 `recoverCreatedNodesLocked()`。
- `Factory::retain`（~195 行）：`++refCount` → `refCount.fetch_add(1, relaxed)`（无锁）。
- `Factory::release`（~208 行）：`--refCount` → `fetch_sub(1)`；归零线程在 `mutex` 内回收。
- `Factory::recycleNodeLocked`（239-267 行）：原递归 `recycleNode` 改为迭代版，假设锁已持有，
  children 归零用 `fetch_sub` 收集到 work 队列，避免回收路径重入锁。
- `Factory::recoverCreatedNodesLocked`（268-276 行）：原 `recoverCreatedNodes` 改名，锁内调用。
- `Factory::mutex`（347 行）：新增 `mutable std::mutex`，串行化 free-list / createdNodes /
  allocatedNodeSlots 的全部变更；refCount 原子不取锁。

设计要点：retain 只原子加计数，拷贝（高频）不碰锁；节点只有在"最后一个引用释放"时才会被
回收进 free-list，且回收与复用（add 内 pop）共用同一把锁，因此持有 storage 的遍历（迭代
`rhs->vars`）期间树必然存活且不被覆写。

### external/binarysub/include/binarysub/binarysub.h

- `ArenaState`（308 行附近）：删除 `std::mutex mutex` 成员，锁统一下沉到 Factory 内。
- `Storage::cachedHash`（382 行）：`std::optional<std::size_t>` → `mutable std::atomic<std::size_t>{0}`，
  0 表示未缓存，缓存值为 hash+1。
- `CompactType::cachedHash`（412 行）：同上。
- `CompactType::operator<` / `operator==`（~433-455 行）与 `structuralHash`（~465-513 行）：
  缓存读写改为 `std::atomic` relaxed load/store；比较时用 hash+1 单调等价。

### external/binarysub/src/binarysub.cpp

- `CompactVarSet::hash`（1372-1397 行）：缓存读写改 `std::atomic`（存 hash+1）。
- `CompactVarSet(const SimpleVarSet&)`（~1407）、`makeVarSet`（~1425）、`makeVarSetWithBase`
  （~1457）、`persistentVarSetNodeSlotsForTesting`（~1472）、`mergeVarSets`（~1485）：
  删除 `lock_guard(owner->mutex)`，构造/拷贝路径统一由 Factory 内部锁保护。

### external/binarysub/src/binarysub-test.cpp + binarysub-test.h + binarysub-test-main.cpp

- 新增 `test_persistent_set_concurrent_factory`（binarysub-test.cpp:1276）：4 个 std::thread
  共享一个 `CompactTypeArena`，并发 `merge`（共享 root 的 retain/release）与重复 `make`
  （命中 hash-cons、参数集锁外析构），各 30000 轮；结束后断言 hash-cons 与变量集仍完整。
  binarysub-test.h:22 声明，binarysub-test-main.cpp:51 注册。

## 验证

- binarysub 单测（g++-14 配置，`BINARYSUB_ENABLE_TBB_PARALLEL=1`）：全量 `./build/binarysub`
  连续 3 次 "All tests passed!"。
- 回归有效性：把 retain/release 临时改回非原子读改写 + 回收不取锁后，新并发测试稳定复现
  原 bug（`PersistentSet.h:203` 断言），恢复修复后通过。
- TSAN（`build-tsan`，`-fsanitize=thread`）：修复后纯 std::thread 并发测试阶段 0 报告；
  TBB 并行测试阶段报告从修复前 78 个降到 63 个（消失的 15 个为本次消除的真实竞争），
  剩余的 63 个栈全部为 TBB 调度器内部 + parallel_for 收尾（main 读结果 vs worker 写）形态，
  TSAN 不识别 TBB 的 join 同步，属既有噪声。
- notdec 主项目（`/sn640/NotDec`）重建后，fortune 复现场景
  `NOTDEC_BINARYSUB_THREADS=2 ./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll --tr-level=2 --merge-struct-ptr-load-store -g --fast-work-dir --work-dir=/tmp/fortune-work-N -o /tmp/fortune-out-N.ll`：
  连续 5 次 exit=0，无断言，输出 IR 通过 LLVM 22 `llvm-as`。
- 问题 2（`base->size() <= vars.size()`）未单独修复：按分析视为问题 1 的连带损坏（树节点被
  回收覆写后迭代错乱），修复根因后 fortune/单测均不再触发该断言。

## 性能 A/B：memcached 166 切片无退化

同一机器、同一 RelWithDebInfo 构建目录（`build-relwithdebinfo-20260731`）、同一命令
（`/tmp/slice-cum-assoc_init.bc` + `memcached.opaque-slabs.summary.json` + `--tr-level=2
--merge-struct-ptr-load-store --frozen-tr-input-ir -g --fast-work-dir`、1 线程），仅 binarysub
版本不同，各跑一次 `perf stat`：

| 指标 | 修复前 8b65571 | 修复后 3e5f0b4（两次） | 结论 |
|---|---|---|---|
| instructions | 1947 亿 | 1951 / 1955 亿 | +0.2~+0.4%，可忽略 |
| cycles | 1741 亿 | 1733 / 1787 亿 | 两次运行自身波动范围内 |
| wall | 43.4s | 43.4 / 44.8s | 波动范围内 |

`out.ll` 与 `ValueTypes.txt` md5 修复前后逐字节一致，且 `out.ll` 与 8-05 基线
`568216bdec2f56c376007a5b5e8831cd` 相同。8-05 日志基线（1397 亿 cycles / 37.8s）偏低来自
当时机器负载/频率差异：今天同环境下修复前也测得 1741 亿 cycles / 43.4s。

## 评分与取舍

- 实现效果：根因消除（refCount 原子 + 回收/复用统一锁 + hash 缓存原子化），行为不变。
- 复杂度：中等。锁只落在 add / 回收路径，高频 retain 保持无锁；迭代回收避免递归重入锁。
- 维护成本：低。`PersistentSet` 的并发不变量集中在 Factory 内，注释说明了锁的边界。
- 备选方案：refCount 原子化 + 无锁 free-list（性能更好但回收/复用协议复杂，容易再引入
  节点提前复用类 bug）；"去掉 hash 缓存"（正确性最简单，但 canonicalize hot path 会重复
  遍历，fortune 这类大输入损失明显）。当前方案作为正确性基线，后续若 canonicalize 并行
  吞吐不足再评估无锁化，不改动正确性设计。

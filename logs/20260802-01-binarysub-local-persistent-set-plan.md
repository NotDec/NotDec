> 原始 prompt：
>
> 考虑怎么让binarysub还是脱离LLVM依赖，比如去llvm源码把ImmutableSet / BumpPtrAllocator的实现复制一份过来吧，比如专门src里面搞个文件夹叫llvm文件夹放，但是namespace可以不一样？，防止链接的时候和llvm产生冲突
>
> 那按这个弄吧
>
> 后续原始 prompt：
>
> 那引用计数好复刻吗？对着抄一下试试吧

# binarysub 本地持久集合计划（已完成）

## 背景

`CompactType` 为避免复制 `SimpleVarSet`，目前使用 LLVM 的
`ImmutableSet` 和两种 bump allocator。它虽然改善了类型恢复的内存路径，
但让 standalone binarysub 配置也必须查找、包含和链接 LLVM Support。

当前集合 factory 关闭 canonicalization，只需要持久 AVL 集合的插入、查找、
有序遍历和比较；不需要 LLVM 的 FoldingSet、DenseMap 或全局集合去重。

## 目标

让 `external/binarysub` 独立构建时不再依赖 LLVM，同时保持 `CompactType`
变量集合的排序、共享子树、跨 arena 生命周期和清理行为不变。NotDec 顶层
仍可继续按自己的需要链接 LLVM，这不属于本次范围。

## 路线（已完成）

在 `include/binarysub/detail/` 放入小型本地实现，使用
`binarysub::detail` 命名空间，避免模仿 `llvm::` 名称或把不相关的 LLVM ADT
带进仓库。持久集合按 AVL 树实现：插入只复制根到修改点的路径，未修改子树
继续由新旧 root 共同引用。`CompactVarSet` 的 owner 继续持有 `ArenaState`，
保证跨 arena 携带集合时原 factory 不会过早释放。

本地 typed bump arena 会继续在清理时析构全部 `CompactType`，随后释放 slab。
这样保留 `std::map`、`std::vector` 和集合 root 的析构顺序。CMake 删除 binarysub
自己的 LLVM 查找和链接分支。

实际实现补上了 LLVM `ImmutableSet` 同样的节点引用计数和本轮构造节点回收：
`add()` 先让最终 root 获得外部引用，再扫描本轮 `createdNodes`。不属于最终树的
旋转临时节点递归释放子树，并进入 free-list 等待复用。节点所在 slab 仍由
`ArenaState` 统一释放，不做每节点 `free()`。

## 风险与判断标准

主要风险是变量 `(id, level)` 顺序变化、持久 root 被修改，或 source arena 清理
后 destination arena 的集合悬空。新增集合语义与跨 arena 回归，并保留现有
`CompactTypeArena` 生命周期测试。验证时 standalone binarysub 配置中不得出现
LLVM 查找或 include，binarysub 自测、顶层类型恢复测试和实际 profile 必须通过。

本地实现保留关闭 canonicalization 的原有选择，但不能省略引用计数：没有它时，
已丢弃 root 和 AVL 旋转临时节点会一直占用 bump arena 槽位。profile 将其确认为
真实内存回退，下面的实现记录包含对比数据。

## 实现记录（2026-08-02）

### 代码

- `external/binarysub/include/binarysub/detail/BumpArena.h:19` 新增
  `TypedBumpArena<T>`；`create()`（35）按类型构造，`clear()`（47）逆序析构并释放
  slab。slab 从 4 KiB 开始，每 128 个块才增长一档，最大 1 MiB，避免过快翻倍。
- `external/binarysub/include/binarysub/detail/PersistentSet.h:28` 新增本地 AVL
  持久集合，保留 `(id, level)` 比较顺序。`Node`（38）保存 child 引用计数；
  `Factory::add()`（138）在结果 root retain 后调用 `recoverCreatedNodes()`（233）。
  `makeNode()`（166）记录本轮节点，`release()` / `recycleNode()`（197、220）递归
  回收失去所有 root/parent 引用的节点，并放入 free-list。最高位作为回收标记，
  不额外扩大 `Node`。
- `external/binarysub/include/binarysub/binarysub.h:282` 将 `CompactVarSet` 切换到
  `detail::PersistentSet`；`CompactTypeArena`（499）切换到本地 typed arena。
  `src/binarysub.cpp:1220` 的 `makeImpl()` 使用 `create()`，`clear()`（1371）保持
  先析构 `CompactType`、再销毁变量集合 owner 的顺序。
- `external/binarysub/CMakeLists.txt:36` 删除 binarysub 自己的 LLVM 查找、include
  与 `LLVMSupport` 链接。顶层 NotDec 是否链接 LLVM 不受本次影响。
- `external/binarysub/src/binarysub-test.cpp:512` 新增集合回归：乱序插入、重复插入、
  旧 root 快照、顺序无关比较、root 释放后全部槽位归还 free-list、第二轮无新增槽位，
  以及 typed arena 析构测试；入口在 `src/binarysub-test-main.cpp:42`。

### 为什么需要引用计数

初版本地集合只把所有 AVL 节点放进 `TypedBumpArena`。结果正确，但释放一个
`PersistentSet` root 不会让其路径复制节点可复用；AVL 旋转还会在一次 `add()` 中
生成不属于最终 root 的内层节点。LLVM 的 `ImutAVLFactory` 用
`createdNodes/recoverNodes` 处理后一类，本地实现按同一生命周期复刻。

引用计数只维护 root 和 parent-child 的共享关系。节点计数归零时先 release child，
再把当前槽位放入 free-list。free-list 槽位会被后续 `makeNode()` placement-new
覆盖；不会立刻缩小 slab，也不会改变 `CompactTypeArena::clear()` 的整体释放语义。
直接逐节点释放会破坏 bump allocator 的简单性，并增加 allocator 调用，不采用。

### profile 对比

同一份冻结的 vsftpd stage-B 输入、单线程、jemalloc `lg_prof_interval:30`。
`t*` 是 jemalloc heap 文件头的采样 live bytes，三次命令相同，因此可比较。

| 实现 | 最大采样 live heap | `/usr/bin/time` 最大 RSS | `smaps_rollup` 最大 RSS | wall |
| --- | ---: | ---: | ---: | ---: |
| 原 LLVM `ImmutableSet` | 157,122,144 B | 588,748 KiB | 571,976 KiB | 12.74 s |
| 本地集合，无引用计数 | 579,876,840 B | 1,071,412 KiB | 1,070,296 KiB | 12.50 s |
| 本地集合，引用计数 | 78,938,432 B | 497,604 KiB | 481,248 KiB | 12.18 s |

相对无引用计数版本，最大采样 live heap 降 86.4%，最大 RSS 降 53.6%。相对旧 LLVM
实现，本地引用计数版本的采样 heap 低 49.8%，最大 RSS 低 15.5%。这证明回收的是
实际保留的集合节点，而非仅改变 profiler 结果。

### 验证

- `cmake -S external/binarysub -B /tmp/notdec-binarysub-refcount-standalone-20260802 -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/g++-14 -DBINARYSUB_ENABLE_TBB_PARALLEL=ON`，随后 build 和 `binarysub`：通过，配置没有 LLVM 输入。
- TBB on/off standalone self-test、ASan self-test：通过。
- `./build/bin/TypeBuilderTest` 与
  `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- 同一 vsftpd profile 完成；新 `out.ll` SHA-256 为
  `0fe289fe5b135ecd5003940e2a45601f72482d2c8ff24b2d66a14ba5df66063c`，与旧 LLVM
  和无引用计数版本逐字节相同。LLVM 22 `opt -passes=verify` 通过。
- merge-eval 正确性未变：`bad_unions=0`、`polluted_components=0`、
  `fragmented_nodes=141`、`fragmented_types=8`；只有 profile 中的 RSS/wall 值变化。

### 评估

成本分数越低越好。实现效果 9/10：内存回退被消除且 profile 优于原 LLVM 路径；
理解成本 4/10：需要理解 root、child 和构造临时节点三种引用；维护成本 3/10：
逻辑集中在一个小 header，测试覆盖快照和槽位复用。比继续依赖 LLVM 少了构建耦合，
也比复制 LLVM 的 canonicalization/cache 更小；代价是需要长期保留这段明确注释的
引用计数实现。

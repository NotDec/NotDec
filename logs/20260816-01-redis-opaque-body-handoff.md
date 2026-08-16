# redis 分配器 opaque_body 实验与交接（2026-08-16）

## 用户原始 prompt

> 当前redis源码级IR里，是否正确找出了内存分配等函数并标记为了多态？
> z系列那个分配器相关的函数，能不能都标记为不分析其函数体？看看能不能根据call graph scc抓到所有的分配器相关的函数
> 试一下吧，确认一下名单，但是不要搞什么内置名单，就针对这个单独的case，用那个json，或者怎么拓展json功能禁用分析函数体即可
> 先这样吧，把当前的任务以及做了什么写到一个log，收尾

## 背景（本会话完整成果链）

沿 tmux 优化日志路线验证 redis-server 源码级 IR（57MB，3870 函数）的时间/内存爆炸与崩溃。已完成的修复（全部提交，日志在 `logs/20260814-01-redis-allocator-opt-ufa-analysis.md` + binarysub `doc/效率提升.md`）：

| commit | 内容 |
| --- | --- |
| `ea8663f3` | zmalloc 多态名单 + AGENTS.md 强调自定义 allocator |
| `f406641` | hasBound O(1) + Ptr 索引 + go1FullCache UAF 修复 |
| `da3a08a` | OccurrenceInterner（49GB→9GB）+ iterator 动态栈 + Node 树级 hash |
| `ded4aba` | **TBB worker 4MB 栈溢出根因修复**（canonicalize 改显式 pthread 128MB 大栈线程池）+ retain/recycle CAS |
| `bf47c6f` | CompactVarSet 内容级 hash-cons（结构共享）+ copy UAF 修复（cloneVarSet） |

## 本任务：分配器函数跳过函数体（opaque_body）

### 背景分析

1. **z\* 系列已正确标记多态**（调用点隔离），但**函数体仍照常分析**——zmalloc 内部 malloc/malloc_usable_size/原子计数的约束照常生成。
2. **sds 系列未标记**（sds_malloc/sdsnewlen 等 30 个函数）：分析结论——sds 是自洽的统一字符串系统（所有字符串经 sdscat 等普通函数统一，标记创建点也会在使用点重新连接），且 zmalloc 多态已把 sds/hiredis/裸 buffer 三条线隔离。
3. 用户提议：**分配器函数跳过函数体分析** + **按 call graph 自动抓取所有分配器相关函数**，不用内置名单，用 JSON 配置。

### 实现（复用现有机制，零代码改动）

**`opaque_body` JSON 字段已存在**（summary override 机制）：
- JSON：`NOTDEC_SUMMARY_OVERRIDE` 指向文件，`"functions": {"zmalloc": {"opaque_body": true}, ...}`
- 机制：`isOpaqueBody`（MLsubGenerator.cpp:5703）→ `OpaqueBodies` 集合 → ConstraintsGenerator 跳过函数体（MLsubGenerator.h:394，签名级约束保留）
- 名单文件：`/tmp/redis-allocator-summary.json`（30 个函数：z\* 系列 13 个 + sds 系列 9 个 + hi_sds 系列 7 个 + zlibc_free）

### 当前 run 状态

- 命令：`NOTDEC_SUMMARY_OVERRIDE=/tmp/redis-allocator-summary.json NOTDEC_BINARYSUB_THREADS=8 ... redis-server.ll --tr-level=2 --merge-struct-ptr-load-store -g --fast-work-dir`
- **进程 pid 3707891 仍在后台运行**（setsid 隔离，日志 `/tmp/redis-opaque.log`，workdir `/tmp/redis-cons-work`）
- 58 分钟：RSS 8.5GB 稳定（无 opaque 版同期约 10.5GB，内存略降）；30 个函数确认跳过（"Info: opaque body skipped for ..."）
- 关键验证点：之前结构共享版在 114 分钟进入内存爬升（11→48GB）——**当前 run 是否避开/缓解**待观察

### 待办（交接给下一任务）

1. **看 opaque run 能否跑完**（`/tmp/redis-opaque.log` + pid 3707891），对比无 opaque 版的 114 分钟爆炸点；跑完拿 eval（`/tmp/redis-cons-eval`）与历史对比
2. **正确性验证**：opaque_body 改变约束（跳过函数体）——需要对照 ValueTypes/HType（`/tmp/redis-cons-work/ValueTypes.txt`）与无 opaque 版（历史 run 的 workdir 已删，可重新跑对照）
3. **自动抓取**（用户提议但未实现）：从 opaque 名单出发，沿 call graph 用 `getGenericMallocWrapperAllocator` 形状检测（扩展 `isMallocWrapperAllocator` 认 opaque 名单）自动识别 sds_malloc 等转发 wrapper——目前是手动列 30 个
4. **sds 系列标记收益待确认**：如果 opaque run 结果与无 opaque 一致（ValueTypes 相同），说明 sds 跳过函数体无副作用，可放心推广

## 注意事项

- 磁盘：/ 分区已清理（tmux-run-\* 418GB 删除），当前 172GB 可用；`/tmp/tmux-slices`（62GB）保留
- TBB worker 固定 4MB 栈（canonicalize 已走大栈线程池，analyze 阶段 4MB 够用）
- 后台 run 用 `setsid bash -c` 启动（nohup 会被 shell 工具超时误杀）
- 正确性对比以 HType 文件为准（ValueTypes.txt），不是输出 IR

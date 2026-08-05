# 危险类型模式扫描 pass（类型推理前 warning）

## 用户原始 prompt

> 到底是哪一块函数加进来之后时间爆炸了啊，要不还是继续不断地基于166继续增加切片？还能这样继续切片分析，具体是什么样的代码模式导致爆炸吗
>
> 思考一下，有没有什么高效的办法，比如说根据什么特殊的线索找到这种破坏性高的函数，加入分析之后就内存爆炸的函数？
>
> 当前发现总结了吗？没有的话总结到一个log文件。然后，考虑，怎么把访问次数 × struct 大小 × 递归权重这个递归逻辑，怎么通过不使用debug info的方式也扫描出来，不使用debug info也可以使用类似的方式弄出一个分数吗？需要写成一个pass，在类型推理前，对于这种危险的函数报warning

## 背景

memcached 全量 401 函数类型推理 3 小时 RSS 线性涨到 41GB 未收敛（之前一轮增量切片定位到的结论全部被机器 OOM/swap 污染）。需要精确定位"加入哪个函数后爆炸"，并找到可以提前预警的静态特征，做成类型推理前的 pass。

## 关键发现（2026-08-05 实测）

干净机器、1 线程、串行重跑（`NOTDEC_BINARYSUB_THREADS=1`，tr-level=2，frozen stage-B IR，opaque-slabs summary override）：

| 切片 | 内容 | wall | 峰值 RSS | 结果 |
|---|---|---|---|---|
| 256 | 基线 | 100s | 17.6GB | 完成 |
| 257 | +resp_reset | 92s | 17.5GB | 完成 |
| 258 | +resp_reset +resp_add_chunked_iov | 92s | 17.5GB | 完成 |
| 259 | +resp_start | 97s | 17.9GB | 完成 |
| 260 | +resp_allocate | >6.5min | 62GB 未收敛 | 爆炸 |
| 260b | 256+resp_allocate | 92s | 17.4GB | 完成 |
| 260c | 256+resp_start+resp_allocate（最小复现） | >6.5min | 62GB 未收敛 | 爆炸 |
| 260c | 同上去掉 --merge-struct-ptr-load-store | >6.5min | 62GB 未收敛 | 爆炸 |

结论：

1. 爆炸窗口是 259→260，最小复现只需 `resp_start + resp_allocate` 两个函数。
2. 与 `--merge-struct-ptr-load-store` 无关。
3. 爆炸机制：`resp_allocate` 对 `open_bundle->r[13]`（13 个 `mc_resp` 柔性数组）做循环展开后的 13 次动态 GEP 访问；`resp_start` 把返回的 resp 挂到 `c->resp` 链表（写 `resp->next`），使 `mc_resp` 成为递归类型（next 自引用）。递归类型 × 数组展开的组合，被 256 个既有函数共享的 conn/thread/item 类型反复含折叠展开，导致内存/时间爆炸。
4. 之前"resp_reset+resp_add_chunked_iov 组合爆炸、merge 是诱因"的结论是 OOM 污染下的误判，已推翻。

静态启发式验证（Python 一次性脚本，扫 memcached.ll 0.3s）：

```
score = Σ 动态GEP次数 × struct大小(字节) × (递归 ? 4 : 1)

  61568  geps=13  resp_allocate    %struct._mc_resp ×13 [recursive]
    984  geps=3   extstore_maint_thread ...
    960  geps=5   extstore_init ...
     16  geps=1   resp_add_chunked_iov    %struct.iovec ×1
```

`resp_allocate` 61568 分断崖第一（第二名不足 2%），与实测完全吻合。不爆的函数只碰非递归小结构（iovec 16B），分数极低。

## 目标

写一个 ModulePass，在类型推理（`MLsubRecoveryMain`）之前扫描 IR，对"破坏力分数"超阈值的函数输出 warning 到 stderr，帮助提前发现会拖垮类型推理的函数，不需要先跑一遍推理。

## 技术路线（不使用 DebugInfo）

DebugInfo 只在有源码编译的 IR 里可用（bin2llvm 等输入没有），所以 pass 全部用 IR 自身信息：

1. struct 大小：收集模块内所有 `StructType`（遍历 GEP source element type、load/store/参数/返回值里的 struct），用 `DataLayout::getStructLayout()->getSizeInBytes()` 计算。DebugInfo 里 9472 bits 和 DataLayout 结果一致。
2. 动态数组 GEP 统计：匹配 `GEP %struct.S, ptr %base, i64 %idx`（idx 为变量）以及 `GEP [N x %struct.S], ptr %base, i64 0, i64 %idx`，按 (函数, struct) 计数。unroll 后同模式出现多次会自然放大计数。
3. 递归检测（无 DebugInfo 的值流近似）：
   - 构建 S* 值集合：seed 为所有 `GEP(S, ...)` 结果；通过 phi/bitcast/select 传播；`load` 的指针操作数是 S* 且该字段属于 S 时结果也是 S*；函数返回值是 S* 时调用点返回值也标记为 S*（跨函数迭代两轮，覆盖 resp_allocate 返回 `&b->r[x]` → resp_start 使用）。
   - 若存在 `GEP(S, off)` 被 store 一个 S* 值，或 `GEP(S, off)` 被 load 出 S* 值 → S 递归。
4. 分数与 warning：`score = Σ count(S) × size(S) × (recursive ? 4 : 1)`；阈值默认用环境变量 `NOTDEC_DANGEROUS_SCAN_THRESHOLD` 控制（默认比如 10000），超过则 `llvm::errs()` 输出 warning，包含函数名、分数、struct 名、GEP 次数、是否递归。

## 风险与判断标准

- 递归检测是近似值流分析，可能漏报（深层 def-use 链）或误报（字段存了同名结构指针但实际不构成递归展开）。warning 定位是提示性质，误报可接受。
- 阈值需要调：先用 memcached 260c 验证 resp_allocate 一定被报出，再扫全量 401 函数看 warning 数量是否合理（期望只有几个高分函数）。
- 判断标准：pass 在 memcached 切片/全量上运行，resp_allocate 出现在 warning 里；对 vsftpd/libuv 等之前不爆的项目，warning 数量少或不影响正常输出。

## 实现记录（已完成，最终为无类型方案）

第一版依赖 IR 里的 `%struct.X` 类型（源码 IR 有效，memcached 全量只报 resp_allocate）。
但用户指出 wasm/二进制转出的 IR 没有高层类型：wasm32 是 i32 指针 + inttoptr
（`GEP [N x i8] @__notdec_mem0, 0, %addr`），bin2llvm 早期是寄存器全局 + 字节偏移
GEP。实测确认后，重写为完全不用类型/DebugInfo 的方案。

新增 ModulePass `DangerousTypePatternScan`，在 `MLsubRecoveryMain` 之前运行：

- `include/notdec/Passes/DangerousTypePatternScan.h`：pass 声明与无类型设计说明。
- `src/Passes/DangerousTypePatternScan.cpp`：
  - `normalizePointer`：把指针值拆成 (base, 常量偏移)，支持 GEP i8/ptr 单索引常量、
    add、inttoptr（wasm32）、bitcast，递归最多 6 层。
  - `longestArithmeticProgression`：同一 (函数, base) 的偏移集合找最长等差子序列，
    返回 (项数, 间距)。unroll 的大对象数组展开留下等差偏移（resp_allocate 13×1184）。
  - `collectPoolLinkCallees`：无类型"池分配挂链表"形状（把 call 返回值 store 到
    load 出来的对象的偏移处），收集被挂链的函数集合；只有这些函数命中等差数组时
    score ×3（按调用图传播，避免 crc32 表初始化这种无关大数组被全局误判）。
  - `collectAccessOffsets`：收集所有 load/store/memset/memcpy 指针操作数的
    (base, 常量偏移)，按 (函数, base) 分组。
  - `run`：score = Σ max(项数 × 间距)；挂链目标 ×3；递归命中且 ≥ 阈值
    （默认 10000，`NOTDEC_DANGEROUS_SCAN_THRESHOLD` 可调）报 HIGH warning；
    无递归证据且 ≥ 10 倍阈值报 Note。
- `src/Passes/PassManager.cpp`：`add_type_recovery_passes` 里 `MLsubRecoveryMain`
  前插入 pass（约 468 行）。
- `src/CMakeLists.txt`：源文件列表加入新 cpp。

验证（build-relwithdebinfo-20260731/bin/notdec）：

- memcached 源码 IR 全量：只报 `resp_allocate`（score=46176，
  base=%call36 ap=13x1184，挂链 ×3），与实测爆炸函数一致。
- crc32c_init_sw_big/little（8×2048 / 8×1024 表初始化）经挂链按调用图传播修正后不误报。
- 单函数 resp_allocate：13×1184=15392 无挂链，不报，与"单函数不爆"一致。
- 260c（resp_start+resp_allocate）：报 HIGH。
- vsftpd / libuv / wasm fortune（stage-B，无类型 IR）：零 warning，pass 无副作用。

已知限制：只覆盖"unroll 成常量等差偏移"的大数组展开；未 unroll 的循环（动态索引）
暂不计分。检测是启发式，warning 提示性质，阈值可按项目调。

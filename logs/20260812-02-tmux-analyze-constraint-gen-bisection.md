# tmux 约束生成/analyze 瓶颈二分与 A/B/C 优化验证

## 用户原始 prompt

> 背景：项目 NotDec……任务起点：解决 lighttpd 内存爆炸；后扩展为 Bench2 大项目
> （tmux 39MB/redis 55MB）的 scalability。评估链路
> `--tr-level=2 --merge-struct-ptr-load-store --gen-work-dir --fast-work-dir
> --merge-eval-dir --frozen-tr-input-ir`，8 线程；类型推理结果看 HType 文件。
> 下一步建议第 1 条：PNDiff 性能/死循环排查（当前焦点）：perf 采样 PNDiff 热点……
> 确认是 O(n²) 还是死循环。继续看看 PNDiff 为什么耗时这么久

## 背景

续 [20260812-01](./20260812-01-tmux-simplify-shared-memo-optimization.md)。
上一日志停在 tmux-final 跑批中；本轮先把“PNDiff 单核 12+ 分钟”查清，再继续把
tmux 全量从小时级往下压。结论先用 IR 函数切片二分定位爆发函数，再对 analyze 做
A/B/C 三方案对照。

## 关键结论

1. **PNDiff 不是瓶颈**。perf 实测单核 13+ 分钟的热点都在约束生成：
   `resolve_variable` 19%、`addUpperBound` 16%、`constrain_impl` /
   `constrain_worklist_only` 16%、`collectMaxDirectFieldAccessSizeBytes` 9.5%，
   PNDiff 相关合计约 4%。`PNDiff.warn.txt` 是 apply override 时写的时间戳，不能当
   solve 完成标记；阶段边界要看 `02-mlsub-input.ll` 与 `[simplify-start]` 打印。

2. **IR 切片二分**（按 fan-in 排序增量加函数，`/tmp/tmux-slices`）：

   | defs | 总时间 | 备注 |
   | --- | --- | --- |
   | 14 | 0.82s | |
   | 407 | 32.7s | |
   | 748 | 157s | |
   | 969 | 397s | 约束生成 372s |
   | 987（969+18 个 tty_*） | 525s | simplify 从 ~5s 变 ~6min |
   | 1180 | 4561s | 约束生成 ~75min |
   | 1848（全量） | 3-7h | simplify 为主 |

   爆发区间 969→1180（1.22x defs → 11.5x 时间），新增函数是
   `server_acl_*`/`session_*`/`status_*`/`tty_*`：共享全局 sessions/status 树和
   tty 巨型结构（tty 结构上百字段 + 函数指针表），高扇出、相互经共享全局状态连接。
   SCC 结构无质变（切片和全量都是巨型 SCC0：969 切片 SCC0=1287 个函数、
   1180 切片 SCC0=1457），瓶颈是约束传播与 simplify 的展开超线性，不是 SCC 碎化。
   1064 defs 复跑显示该切片约束生成仅 1:55、simplify 21+ 分钟，说明爆发主要落在
   simplify 阶段；两阶段占比在切片间不完全一致，不要死记单一百分比。

3. **analyze 是 simplify 阶段的主体**。diag1275 切片合计 `total_analyze_ms`
   ≈13.09M ms；20.5 亿次共现列表交集 × 平均列表长 244 ≈ 5000 亿次元素比较，
   这是 per-group 共现分析（`binarysub.cpp` 的 occurrence/analyze 系列函数）的
   固有成本。

4. **A/B/C 三方案**（diag1275 系列，8 线程）：
   - **B+C ✅**（合并 var/prim 循环 + visited 用 unordered set）：
     37:48 → 18:02/22:56，约 2 倍。commit `0a8b5a8`。
   - **A ❌**（共现交集 bitset 化）：两轮均 28:35，AND 收益被每 var 的 idToIndex
     hash 查找（20.5 亿次）和 collectVars 第一遍遍历抵消；变量 id 全局稀疏，
     稠密映射必须每组建表，hash 常数吃掉了 35 倍 AND 的账面上收益。
   - **空列表短路 + RecVarIds hash ❌**：27:06，且 intersect 次数 20.56 亿 vs
     20.54 亿基本不变，证明共现列表在巨型 SCC 里几乎从不为空，短路率≈0，已回退。
   - `a328303`：OccurrenceMap 从 std::map 改 unordered_map，配 identity 一致的
     PolarVarHash。

## 提交状态（收尾时遗留）

- binarysub 已有 `a328303`、`0a8b5a8` 两个提交，但顶层
  `external/binarysub` 指针仍指 `77c229a`；失败的 bitset/短路实验已回退，
  binarysub worktree 干净。顶层指针更新和本日志的提交由后续会话收尾。
- 本轮改动位置：`external/binarysub/src/binarysub.cpp`（analyze 的 var/prim
  合并循环、visited set、OccurrenceMap 与 PolarVarHash）；诊断打印沿用
  `NOTDEC_SIMPLIFY_DIAG`。

## 验证

- fortune/memcached eval 指标与历史一致（见 20260812-01 的验证节）。
- B+C 版两次 22:56/18:02 本身波动 5 分钟，机器负载噪声明显；27:06 的“优化版”
  结论要与完整数据一起看（intersect 次数不变才是决定性证据）。

## 未解决 / 下一步

- 5000 亿次比较的固有成本：bitset 路线在“全局 id 稀疏”下没有好解；跨组 analyze
  缓存未试（10 万组里大量共享相同 recVars，如 255/415）。
- 约束生成 ~13 分钟单核，热点已明确（resolve/addUpperBound/constrain 系列），
  尚未动手优化。
- 全量 tmux 端到端确认与顶层指针提交。

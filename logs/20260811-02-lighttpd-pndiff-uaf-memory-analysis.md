# lighttpd 内存爆炸归因：PNDiff UAF 修复与 per-group simplify 内存分析

## 用户原始 prompt

> 当前继续基于源码IR已经合并评估数据的链路，目前需要解决lighthttpd的内存爆炸问题，看看最近的日志就知道了。尝试分析出是什么原因。当前，通过之前用的jemalloc profile的方式，找出具体为什么内存爆炸，分析原因并讨论，思考可能的解决办法

## 背景

lighttpd（1.4.82 源码级 IR，14MB）在 tr-level=2 + merge-struct-ptr-load-store 链路上内存爆炸：
canonicalize 完成后仅 1.2GB，simplify/coalesce 阶段涨到 55-57GB（8 线程，机器 62GB OOM 边缘）。
计划用 jemalloc profile（`scripts/profile-memcached-memory.sh`，8 线程，32GiB 阈值）归因。

## 过程与发现

### 1. jemalloc profile 被 PNDiff UAF 挡住

jemalloc 跑在约束生成后期信号 11 崩溃（34s，740MB）。gdb 取证：崩溃在
`PNINode::unify`（external/binarysub/src/PNDiff.cpp:490），`this->Id` 读出垃圾值、
`Node` 悬垂。ASan 构建复现并给出铁证：

- READ of size 8 at `this` 地址（读 `this->Parent`，offset 0）
- free 栈：`mergePNINodes:568 → Absorbed->eraseFromParent()`（std::list erase 销毁对象）
- use 栈：`unify:490`，同一 `unifyPNDiffValueGroups:6959` 调用链内两次 unify 之间

**根因**：11001f2（PNDiff union by size）让 `mergePNINodes` 可能销毁 `this`
（Absorbed），但 `unify` 的 490 行在 merge **之后**仍读 `this->Parent` —— UAF。
487 行（merge 前）不崩，所以崩溃稳定发生在 490 行。glibc 下概率性触发
（tmux 全量 22 分钟无崩溃只是运气）；jemalloc/ASan 布局下必崩。

**修复**（binarysub 提交 `8b83082`）：490 行改用返回值
`Node->Parent.trace(...)`（Node 是 mergePNINodes 返回的代表，保证存活）。
ASan 复验：完整通过约束生成 + PNDiff 阶段。

### 2. jemalloc profile 跑通后：内存爆炸已不存在

修复后 jemalloc profile（8 线程、32GiB 阈值）3:26 就完整跑通、峰值仅 6.7GB。
用原生分配器对照（7:35、16.3GB），输出 IR 与 jemalloc 版 `cmp` SAME。

**结论：本次的 55-57GB 不是当前代码的行为**。对照实验（回退 11001f2 + 490 修复，
即固定方向 merge）同输入同参数重跑：simplify 完成后 RSS 仅 4.4GB、
峰值 16.4GB、完整跑通，IR 与当前版 SAME（验证 union-by-size 语义等价）。

### 3. 内存爆炸真正的根因：per-root simplify 流程

时间线对账：

| 跑 | 代码版本 | 结果 |
|---|---|---|
| 20260810 perroot3 | per-root 流程（12 万 root 各自简化） | 53.7GB 跑通 |
| 20260811 memstats（10:00） | 同上（8d29998 10:26 才提交） | 57GB 卡 simplify |
| 20260811 晚 lighttpd | 同上（per-root 二进制） | 55.7GB 被停 |
| 20260811 tmux（union-by-size 版，UAF 污染） | per-group + UAF | 62.3GB OOM |
| **本次 oldpndiff 对照** | **per-group + 固定方向** | **16.4GB 跑通，IR SAME** |
| **本次 jemalloc2/native** | **per-group + union-by-size + UAF 修复** | **6.7GB / 16.3GB 跑通，IR SAME** |

**模式**：per-root 流程里每个 root 持有独立的 simplifier/arena/递归展开结果，
12 万 root 线性累积，类型结构复杂（递归 record 链）时每份都大 → 内存爆炸。
**8d29998（08-11 10:26，per-group 归组 + CoalesceMemo + ReconstructMemo +
mergeInsertVars）是内存修复的关键**：结构相同的 root 归组（compactMap 里
CompactType hash-consed，指针相同的结构共享一次展开），跨组递归展开由
CoalesceMemo 去重（oldpndiff 显示 shared_hits=492793）。per-group 版
simplify 后 4.4GB vs per-root 版 57GB，约 13 倍差距。

memstats（08-11 10:00）的 57GB 用的还是 per-root 版二进制（8d29998 当时未提交），
与 20260810 的 53.7GB 同源；当时日志把"lighttpd 内存爆"归因到 simplify 阶段，
实际是 per-root 流程固有的线性内存累积。

## 遗留

- tmux（43MB IR）在 union-by-size + UAF 污染下 62.3GB OOM；修复后需重测，
  确认 tmux 是否还有独立的内存问题（UAF 污染 vs 固有内存）。
- jemalloc profile 归因已无必要（内存爆炸不复现）；后续大项目先看
  `NOTDEC_BULK_MEM_STATS` 的 after-canonicalize/after-simplify RSS 即可定位阶段。

## 验证

- ASan（build-asan，单线程）：修复前立即 UAF 崩溃；修复后通过约束生成 + PNDiff。
- 三组完整跑（8 线程，`--frozen-tr-input-ir`，work/eval 目录在
  `/sn640/NotDec-Exp/Bench2/type-recovery-runs/20260811-lighttpd-*`）：
  jemalloc2（6.7GB）、native-fixed（16.3GB）、oldpndiff（16.4GB），
  输出 IR 全部 `cmp` SAME，`llvm-22.1.0.obj/bin/llvm-as` 通过。
- eval：coverage 3.9%、bad_unions 18、fragmented 33 类型 / 329 节点。

## 评分

- 实现效果：UAF 修复 1 行，消除 jemalloc/ASan 下的必崩 + glibc 下的概率性
  UAF/污染；内存问题确认已由 8d29998 解决（55GB→16GB），本次修复解锁了验证。
- 复杂度：无（一行 + 注释）。
- 维护成本：无。
- 更优方案：无（根因修复）。后续可将"每组展开共享"推广到 tmux 等更大项目
  前先重测确认。

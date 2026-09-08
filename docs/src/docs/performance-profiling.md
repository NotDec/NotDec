# 性能分析

本流程用于 Bench2 类型恢复的时间或内存分析。结合 `/usr/bin/time -v`、jemalloc 分配 profile
和分阶段 perf 采样。分析大源码 IR 项目前必须先读本文档。

## 内存 profile

`scripts/profile-memcached-memory.sh` 支持 `--input`、`--threshold-gib`、`--threads`，
默认目标是 memcached；`--threshold-gib 0` 表示不设内存阈值，`--threads 8` 固定线程数。
脚本输出 `rss-pss.csv` 和 jemalloc heap dump。手动运行时 preload 系统 jemalloc，并使用
独立 profile prefix：

```bash
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so.2 \
MALLOC_CONF='prof:true,prof_active:true,lg_prof_sample:19,lg_prof_interval:30,prof_final:true,prof_prefix:/tmp/jeprof.project' \
NOTDEC_BINARYSUB_THREADS=8 ./build/bin/notdec input.ll -o /tmp/out.ll \
  --tr-level=2 --merge-struct-ptr-load-store -g --fast-work-dir \
  --work-dir=/tmp/notdec-work --merge-eval-dir=/tmp/notdec-eval
```

进程正常退出时，`prof_final` 请求 jemalloc 在 prefix 写最终 heap dump。它不能保证信号终止
后仍有 dump，尤其是 `SIGKILL`。`jeprof --show_bytes --text` 看 live allocation，
`--alloc_space` 看累计 allocation，`--lines` 落到源码。jemalloc 会改变分配行为，时间对比
必须在相同 allocator 配置下进行。

## CPU profile

使用 `/home/ubuntu/.local/bin/perf`。用 `pgrep -x notdec` 找真实进程，用
`ps -L -p <pid>` 查看线程，再采样一个阶段：

```bash
perf record -F 99 -g -p <notdec-pid> -- sleep 30
perf report
perf annotate --symbol=<function>
```

约束生成通常单线程，analyze/simplify 通常使用 8 线程。`02-mlsub-input.ll` 标记类型恢复输入，
`NOTDEC_SIMPLIFY_DIAG=1` 下的 `[simplify-start]` 标记 simplify。采样过少时重试或加
`--call-graph dwarf`，并与 time/RSS 交叉确认。

跑批期间不要重链 `notdec`：运行进程保留旧 inode，perf 会显示 `(deleted)` 且无法解析符号；
必须在启动前构建。指针键容器顺序受 ASLR 影响，会改变 HType 文本、merge decision、分组和总时间。
复现 profile 用 `setarch x86_64 -R env ...`；设置
`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=0` 也能去掉 canonicalize 调度变化，但性能代价要单独测。

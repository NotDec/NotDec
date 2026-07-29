# vsftpd 源码级 IR 合并评估

> 那接下来跑vsftpd试试，看看耗时多久。先把跑的流程固定总结到AGENTS.md的一个章节中：即首先要关注是否存在多态函数，特别是libc中的函数以及wrapper，以及malloc wrapper，free wrapper等，找到了要标记为多态。还要注意什么来着，然后如果是用带有调试信息作为ground truth的话，就看看日志有没有错误合并吧，然后如果跑一次时间太长，就尝试分割出一个较小的能复现错误合并问题的IR，再去分析和解决问题。

## 输入与流程

- 输入是 vsftpd 3.0.5 的 `/sn640/NotDec-Exp/Bench2/source-ir/ir/vsftpd/vsftpd.ll`，
  SHA-256 为 `0c139d992649aaaaa7b75c1b17ce1afba1d4177944b70eab861a731b92565692`。
- IR 有 550 个函数定义、200 个声明和 39 个 `DICompileUnit`，LLVM 22 assembler 和 verifier 均通过。
- `AGENTS.md:351` 的“源码级 IR 合并评估的固定流程”补充了多态边界预检、8 线程固定参数、
  DebugInfo 结果顺序和小 IR 裁剪要求，已由 `33de7e94` 提交。

## 多态边界

`test/type-recovery/realworld/support/vsftpd.source.summary.json:3` 新增 42 个项目专用标记：

- allocator/deallocator：libc 的 `calloc/mmap/munmap`，以及 malloc、mmap、secure-buffer wrapper。
- raw memory：read/write、memcpy/memcmp、SSL/FTP/private-socket buffer wrapper 和
  `ptrace_sandbox_get_buf`。
- 参数化容器：`hash_*` 的 key/value 接口。
- socket：`accept`、name 和 sockopt 这类布局由运行时参数决定的 libc 接口。

固定 `stat`、sockaddr factory、固定 `msghdr` wrapper 和普通 destructor 没有标成多态。

## 完整 IR

命令固定使用 `NOTDEC_BINARYSUB_THREADS=8`、`--fast-work-dir`、
`--merge-struct-ptr-load-store` 和 `--merge-eval-dir`。

最终 42 项 override 下的完整重跑在 11 分 00.04 秒后超时：CPU 99%，
峰值 RSS 843104 KiB。此时进程仍只有一个线程，`SCCs.txt` 仍是一个 614 函数的 level-0 大组，
`ValueTypes.txt` 只有文件头，评估目录只有 DebugInfo oracle，没有
`merge-eval-summary.json`。因此当前只能确认完整评估 10 分钟内不能完成，不能判断完整项目是否有
wrong merge。产物在 `/tmp/notdec-source-vsftpd-final-20260729-MWInAh`。

## 小 IR

用 LLVM 22 `llvm-extract` 从完整 bitcode 抽取函数体，同时用 `--rglob='.*'` 保留全局 initializer
和 DebugInfo。最终 42 项 override 下的结果如下：

| 组合 | 定义数 | 外层耗时 | 峰值 RSS | typed/target | bad unions | fragmented types/nodes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| hash 两种 key/value 实例 | 16 | 1.50 s | 326 MiB | 7/371 | 0 | 1/5 |
| socket option/address | 20 | 1.02 s | 282 MiB | 3/223 | 0 | 1/1 |
| string core | 11 | 0.68 s | 244 MiB | 6/84 | 0 | 1/5 |
| string list | 10 | 0.73 s | 260 MiB | 7/99 | 0 | 2/3 |

四个输出均通过 LLVM 22 verifier，结果在
`/tmp/notdec-vsftpd-final-small-runs-20260729-PWLq7z`。这些组合只说明已覆盖接口没有发现错误合并；
coverage 很低，不能代替完整项目结论。

另一个 8 函数 secure-buffer 组合在 2.41 秒时稳定触发
`external/binarysub/src/TypeBuilder.cpp:682` 的
`Unexpected primitive UType as a direct field` 断言，峰值 RSS 约 768 MiB。多态标记后仍可复现，
位置在 HType 转换，发生在 merge-eval 最终报告之前，因此它不是 bad union 证据。复现目录是
`/tmp/notdec-vsftpd-final-small-runs-20260729-PWLq7z/secbuf`。

## 判断

vsftpd 的首要问题是完整 level-0 大组在类型求解前半段长期单线程运行。当前小组合没有暴露 wrong
merge；下一步若继续做，应先用 secure-buffer 小 IR 处理 HType 断言，再按函数数量扩大 IR，直到能生成
第一份有代表性的 `merge-eval-summary.json`，然后才分析 bad union。

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

另一个 8 函数 secure-buffer 组合最初在 2.41 秒时稳定触发
`Unexpected primitive UType as a direct field`。原因是旧 buffer 从 `char **` load 后经
`ptrtoint` 参与取模，新 buffer 又写回同一 slot；load/store closure 使新旧指针共享类型，最终形成
`uint:64 & record`。其中 primitive 是地址数值用法，不是 dereference 后的字段布局。

## 2026-07-29 字段混合交集修复

修改文件和函数：

- `external/binarysub/include/binarysub/TypeBuilder.h:48-52,122-124`
  - `TypeBuilder` 增加字段 fallback warning 去重集合和输出函数声明。
- `external/binarysub/src/TypeBuilder.cpp:487-508`
  - `emitFieldFallbackWarning()` 按原因、UType 文本形状和字段宽度去重，warning 带 root 和递归路径。
- `external/binarysub/src/TypeBuilder.cpp:676-776`
  - `convertFieldType()` 处理 `UInter` 时，如果同时有 record/pointer/recursive/function 布局项和
    primitive，只在字段转换中跳过 primitive。原始 SimpleType/UType 约束不变。
  - 裸 primitive 或 bottom 不再断言，也不返回空指针或伪造 `void *`；改为 warning 后返回按
    `FieldSizeBytes` 定宽的 `TopType`，保留字段范围。
- `unittests/Retypd/TypeBuilderTest.cpp:9-17,146-188`
  - 增加裸 primitive、裸 bottom 和 `primitive & record` 三个回归测试。

验证结果：

- `TypeBuilderTest`：9/9 通过。
- `binarysub` 自测通过。
- `llvm_ir` tr-level=2 suite 通过。
- secure-buffer IR 完整跑通，耗时 0.97 秒，峰值 RSS 约 269 MiB；两个 SCC 各输出一次去重
  warning，LLVM 22 verifier 通过。
- secure-buffer merge-eval：`bad_unions=0`、`polluted_components=0`，但 typed coverage 是 0/85，
  所以只能证明评估链路完成，不能作为类型合并质量样本。结果目录是
  `/tmp/notdec-vsftpd-secbuf-fieldfix-final-20260729-KlPxkh`。
- `sysy` suite 仍有 9 个匿名值编号 golden 差异，失败日志没有本次 TypeBuilder warning；单线程
  canonicalize 结果相同。fortune suite 在类型推理前因 extra-constraints SHA 与当前 IR 不匹配退出。

简评：

- 实现效果：8/10。消除断言，同时只在字段视角丢弃地址的数字用法。
- 复杂度成本：8/10。改动局限在 `convertFieldType()`，没有引入 optional 返回链。
- 维护成本：8/10。warning 保留异常输入证据，规则和 value/member 视角边界一致。

更好的长期方案是让 `ptrtoint` 的 PNDiff alias 和 MLsub 类型 alias 分开，但这会影响 EVM 的地址整数
模型，不适合作为本次局部修复。

## 判断

secure-buffer 的 HType 断言已经解决。vsftpd 当前首要问题仍是完整 level-0 大组在跨 SCC 调用约束传播
阶段长期单线程运行；下一步应给 deferred call constraint 增加分段计时和传播计数，再按函数数量扩大 IR，
直到能生成第一份有代表性的 `merge-eval-summary.json`。

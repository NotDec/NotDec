# 源码级 IR 合并评估

本流程用于在带 DebugInfo 的源码级 LLVM IR 上评估类型恢复 merge policy，不改变普通
恢复输出。修改策略并运行 `--merge-eval-dir` 前必须先读本文档。

## 前提与输入审查

源码 IR 位于
`/sn640/NotDec-Exp/Bench2/source-ir/ir/<project>/<project>.{ll,bc}`；其 README
记录源码版本、构建方式和已知问题。运行前确认 DebugInfo 可用，并记录源码版本、IR 路径、
构建命令和输入哈希。

先读源码和 IR，再分类多态边界。检查 libc 和项目自己的 allocator、deallocator、裸 buffer
读写/复制 API、`void *` callback context 及 wrapper。`malloc`/`calloc`/`realloc` 的纯返回
转发 wrapper，以及 `free` 的纯参数转发 wrapper，应按调用点隔离；立即初始化固定结构体的
factory 仍应保留类型。

项目自定义 allocator 要特别审查。`zmalloc`、`xrealloc` 这类函数常有 accounting、OOM 或
`malloc_usable_size` 逻辑，自动 libc wrapper 检测可能漏掉。确认 API 确实通用后，将其加入
`src/TypeRecovery/mlsub/MLsubGenerator.cpp` 的
`isBuiltinPolymorphicBufferFunctionName`；内置名单也对有定义函数生效，能避免不同调用点变成
一个巨型递归类型。不能仅因函数分配内存就把固定类型 API 标成多态。

网络程序还要检查 `read`、`write`、`recv`、`send` 及 wrapper，以及 `recvfrom`、`recvmsg`、
`accept`、`getsockname`、`getpeername`、`getsockopt`、`setsockopt`、`ioctl`。只有裸 buffer
或布局由地址族、option、request 决定的参数才是多态。

`MallocWrappers.txt` 和 `PolymorphicBufferFunctions.txt` 是证据而非完整检测结果。仍要核对
wrapper 链、间接调用、改名的 allocator/deallocator 和剩余 raw memory 输入。

## 标准运行与结果解释

每次运行使用新的 work/eval 目录。固定 8 线程以保持性能口径可比，并用仓库 LLVM 22 验证输出：

```bash
NOTDEC_BINARYSUB_THREADS=8 /usr/bin/time -v ./build/bin/notdec \
  input.ll --tr-level=2 --merge-struct-ptr-load-store \
  -g --fast-work-dir --work-dir=/tmp/notdec-source-project-work \
  --merge-eval-dir=/tmp/notdec-source-project-eval \
  -o /tmp/notdec-source-project-out.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-source-project-out.ll -o /tmp/notdec-source-project-out.bc
```

`merge-eval-summary.json` 汇总 coverage、wrong merge、fragmentation 和 performance。
`DebugInfoValueTypes.txt` 是 DebugInfo oracle；`bad_unions.jsonl` 记录错误 component；
`fragmented_types.jsonl` 记录同类类型分散到多个 root 的情况。

先确认 DebugInfo coverage 足够，再要求 `wrong_merge.bad_unions` 和
`polluted_components` 为零，之后才看 fragmentation。结合 `bad_unions.jsonl`、first witness
和 `CallSlotMergeDecisions.txt` 定位第一条错误边；不能以降低 fragmentation 换取错误合并。

直接的推理产物是 `ValueTypes.txt`、`ValueHTypes.txt`、`ImportantHTypes.txt`、
`VarOrigins.txt` 和 `DebugInfoValueTypes.txt`。输出 IR 相同不能证明推理等价，IR 只检查 LLVM
合法性。将 `/usr/bin/time -v` 的 wall time、峰值 RSS 和语义指标一起记录。

## 全量运行太慢或错误时

全量运行太慢、或 bad union 难解释时，按调用图逐步构造缩小模块，保留相关 global、声明和
DebugInfo。小 IR 必须复现相同 strict type 和 first witness；先在小 IR 修复，再回到全量运行，
确认 coverage、wrong merge、fragmentation、性能和 LLVM 合法性。

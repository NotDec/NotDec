# 用户原始 prompt

> 那试一下这个memchached

# 背景

libuv 是通用跨平台运行时库，whole-library IR 中包含大量文件、网络、线程和系统接口包装，
不适合作为下一轮判断多态 SCC 边界的主要样本。Bench2 按实际 ELF 大小在 libuv 之后的目标是
`/usr/bin/memcached`（262,344 B），对应 Ubuntu Noble `memcached 1.6.24-1build3`。Bench2
已保存匹配 build-id 的 dbgsym，DebugInfo 指向同版本源码，但 `source-ir` 尚没有该目标的源码和
whole-program LLVM IR。

# 目标

用现有 LLVM 22、wllvm、`-g -O2` 口径构建 memcached 自身的 whole-program bitcode，并确认
DebugInfo、链接范围和 LLVM verifier 有效。随后以固定 8 线程运行类型恢复与 DebugInfo merge-eval，
检查多态函数是否保留独立边界，记录耗时、峰值内存、coverage、wrong merge 和 fragmentation。

# 路线

获取与 Bench2 包一致的 Ubuntu Noble source package 和 Debian 补丁，先确认构建系统及依赖。构建时
只链接 memcached 自己的源文件；libevent、SASL、OpenSSL 和 libc 仍保留为外部声明，避免把动态依赖的
实现混入评估模块。产物写入 Bench2 `source-ir/src`、`ir/memcached` 和 `logs`，并更新 source-ir
manifest 与 README。

运行前从源码和 IR 核对 allocator/free、item/value buffer、网络读写、协议解析、libevent callback
arg 和 `void *` context。仅将确实跨调用点承载不同布局的接口标为多态；固定的 memcached item、连接和
线程结构体路径保留跨调用约束。评估沿用 `--tr-level=2`、结构体指针 load/store 合并、fast workdir 和
独立 merge-eval 目录。

# 风险和判断标准

- 源码版本、Debian 补丁和构建配置必须与 `1.6.24-1build3` 对齐；本地 rebuilt ELF 不要求复现原 build-id。
- memcached 有 libevent callback、线程和网络 buffer；需要区分协议 buffer 的通用性与固定 `conn`、`item`
  等业务结构，不能因为存在 `void *` 就整体放宽。
- 当前高 level SCC 修正若在 memcached 上触发 binarysub 断言，先定位并修复该运行时问题，再解释性能或
  merge-eval 结果。
- 最终 IR 必须有可用 `DICompileUnit` 和 struct DebugInfo，并通过 LLVM 22 assembler/verifier；wrong
  merge 必须优先清零或逐条解释，不能用降低 fragmentation 掩盖错误合并。

# 实现记录（SCC 探针已完成）

- Bench2 `source-ir` 已加入 memcached 的源码、构建记录、`ir/memcached/memcached.bc` 和
  `ir/memcached/memcached.ll`，并更新外部目录的 manifest/README。BC SHA-256 为
  `b660fb973f7389755aea7bb2f6981f83d2ef3f4da056778b7da111861d9008fb`，LL SHA-256 为
  `987fc46ee295b4b984bee4f460c59b17c38d4293ee112e9480f1f9027494e0d0`。IR 有 401 个定义、28 个
  `DICompileUnit`、538 个 struct DebugInfo。
- `test/type-recovery/realworld/support/memcached.source.summary.json:1-22` 标记 `calloc`、cache/slab
  allocator/free、`pthread_create`、`event_set` 和运行时布局由参数决定的 socket/buffer API。`tcp_*`、
  `ssl_*`、`restart_*` 仍保持单态：它们在 memcached 内部使用固定 `conn` 或固定数据结构。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6317-6374` 的 function-only Tarjan 避开 synthetic
  CallGraph 节点；`src/TypeRecovery/mlsub/MLsubGenerator.cpp:6552-6658` 保证多态 SCC 不会被 same-level
  union 吞进单态邻居组；`src/TypeRecovery/mlsub/MLsubGenerator.cpp:6701-6705` 在分组完成后 flush
  `SCCs.txt`，使被停止的大样本也能留下分区证据。
- 带 memcached override 的 SCC 探针写入
  `/tmp/notdec-source-memcached-poly-probe-work/SCCs.txt`：level 0 只有一个 533-node 组；level 1 有
  34 组、35 个节点，最大组仅 `do_slabs_newslab,do_slabs_free` 两个节点；level 2 的
  `malloc`、`free`、`realloc` 都是单节点组。`event_set`、`pthread_create`、socket API、cache/slab API
  也各自独立于 level 1。故多态函数确实形成了更高 level 的独立 SCC，而不是留在 level 0 大组里。

## 性能结论

未带项目 override 的完整基线在 11:50.26、峰值 62,956,388 KiB 时被 SIGKILL，未生成
`merge-eval-summary.json`。该次运行发生在 function-only Tarjan 修正前，不能作为修正后的性能数字。

修正后虽然去除了 synthetic 节点造成的假 raw SCC，但
`MLsubRecovery::prepareSCC()` 的 level-0 合并仍会把所有 level-0 raw SCC 放到一个 generator，当前就是
533 个节点。这是为了保持全局变量和共享 memory 的跨函数约束；直接按调用图拆开会改变语义。因此本轮没有
再用约 60 GiB 的完整运行去伪造性能结论，也还没有 coverage、wrong merge 或 fragmentation 指标。

## 验证

- LLVM 22 `llvm-as` 与 `opt -passes=verify` 验证 memcached IR 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `./build/bin/MLsubGeneratorTest`：18/18 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。

## 复杂度和维护成本评估

- 实现效果：8/10。消除了 CallGraph synthetic 节点导致的假 SCC，并直接在 memcached 上确认高 level
  多态边界。
- 理解成本：5/10。分区现在有 raw SCC、same-level group 和收缩图三个阶段，代码注释明确了每一步的
  调用方向和原因。
- 后期维护成本：5/10。Tarjan 只遍历有 `Function` 的节点；新增 CallGraph 节点类型时需要保持该过滤，
  但不依赖 LLVM 内部 synthetic 节点的具体连边。

# 用户原始 prompt

> 接下来继续拿bench2跑下一个更大一些的binary。同样，如果没有带 debuginfo 的源码 jar，就编一个出来

# 背景

Bench2 中已有 DebugInfo 源码级 IR 的最大一档是 vsftpd：目标 ELF 为 160 KB，源码 IR 为
4.0 MB。按目标 ELF 大小继续向上，下一项是 211 KB 的 `libuv.so.1.0.0`。Bench2 保存了 Ubuntu
Noble `libuv1t64 1.48.0-1.1build1` 的二进制和 dbgsym，但 `source-ir` 尚无对应源码和 whole-library
LLVM IR。

# 目标

用与现有 Bench2 source-ir 相同的 LLVM 22、wllvm、`-g -O2` 口径构建 libuv 1.48.0 的完整库
bitcode，确认 DebugInfo、链接范围和 LLVM verifier 均有效。随后检查 libuv 的 allocator/free、裸
buffer、socket 地址、线程回调和 `void *` context 等多态边界，运行完整 HType 推理与 DebugInfo
merge-eval，记录时间、内存、coverage、wrong merge 和 fragmentation。

# 路线

从 Ubuntu Noble source package 获取与 Bench2 包一致的源码和补丁，优先复用 CMake 的库目标并让
wllvm 捕获全部组成对象；若共享库提取不稳定，则直接从相同对象链接 whole-module bitcode。产物放入
Bench2 `source-ir/src`、`ir/libuv` 和 `logs`，并更新 `source-ir/manifest.tsv` 与 README。

运行前从源码和 IR 核对通用分配器、free wrapper、raw read/write、socket 地址参数和 callback data。
只把运行时布局通用的接口标记为多态，固定 `uv_*` handle/request 类型的 API 不因含有 `void *data`
字段就整体放宽。评估使用 8 线程、fast workdir、结构体指针 load/store 合并和独立 merge-eval 目录。

# 风险和判断标准

- Ubuntu source 版本、编译优化级别和目标组成必须记录清楚，不能拿系统 Jammy 1.43 源码替代 Bench2
  Noble 1.48。
- libuv 含平台条件编译、线程入口和大量回调；间接调用可能降低约束覆盖，需要在结果中明确说明。
- whole-library IR 必须保留有效 `DICompileUnit`/结构体 DebugInfo，并通过 LLVM 22 assembler 和 verifier。
- 先确认 `ValueHTypes.txt`、`ImportantHTypes.txt` 完整生成，再解释 merge-eval coverage；wrong merge 优先于
  fragmentation。运行过慢时再裁剪能复现同一问题的小 IR，不能直接用小样本代替完整结论。

# 实现记录（已完成）

- Bench2 `source-ir` 已加入 Ubuntu Noble `libuv1t64 1.48.0-1.1build1` 的源码、`libuv.bc` 和
  `libuv.ll`；LLVM 22 assembler/verifier 均通过。多态配置写在
  `test/type-recovery/realworld/support/libuv.source.summary.json:1-69`，只标通用 allocator、裸
  buffer/socket API、通用 context API 和线程回调入口，没有把固定 `uv_*` handle/request 实现整体放宽。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6317-6374` 的 `MLsubRecovery::prepareSCC()` 不再把 LLVM
  CallGraph 的 synthetic external-caller/external-callee 节点放进 Tarjan 图。它们会把 address-taken
  函数和外部声明绕回一块，造成不真实的大 raw SCC。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6552-6658` 保留多态 raw SCC 的边界：same-level union 只合并
  两端都不是多态 SCC 的调用边；随后按收缩后的调用图重新拓扑排序，保证 `bottomUpPhase()` 反向遍历时
  总能先拿到 callee generator。此前只按组中第一个 raw SCC 排序，libuv 会触发 `Ind2 > Ind` 断言。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:6921-6933` 将 global ConstantExpr 的 `inttoptr`/`ptrtoint`
  当作自身 LLVM 类型的新变量，避免 libuv 的地址常量在 `convertSimpleTypeVal()` 中断言。

## 验证与结论

- 第一轮 libuv 在 ConstantExpr 路径于约 7.20 s 中止；修正后旧的单大组运行在 1:48.84、峰值
  61,781,644 KiB 时被系统 SIGKILL。随后分组修正先暴露了调用组排序断言，收缩图拓扑排序后该断言消失。
- 新分区继续运行时在 binarysub 的 `level_of()` 遇到已有子模块问题；用户随后决定换更适合判断 SCC
  边界的 memcached。本计划没有得到可比较的完整 libuv merge-eval 指标，不能把上述 OOM 当作修正后的
  性能结果。
- 这轮发现的 synthetic CallGraph 问题与具体项目无关，因此修正保留在主链路，并由 memcached 继续验证
  高 level 多态 SCC 是否真的独立。

# 用户原始 prompt

> 感觉不对，还是不要这样了。先把NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL改成默认开启吧，需要的时候再通过环境变量弄成单线程。同时把这里并行相关的配置记录到AGNENTS.md里面合适位置。我记得好像有一个什么ffplay，这个项目好像更小一些，如果是的话，看看能不能编译出这个项目的IR。
>
> 按这个做一下试试吧。另外ffplay缺依赖的话，能不能直接apt安装？

# 背景

binarysub 的 bulk simplify 已默认允许 oneTBB 并行，但 canonicalize 仍要求显式设置
`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=1`。这个 gate 最初用于隔离递归类型输出不稳定风险；
当前代码显式开启 8 线程后，两次 fortune 运行的 LLVM IR 和 HType 文本已经一致，可以重新评估默认值。

现有 Bench2 源码级 IR 只有 fortune、wrk 和 vsftpd。wrk 会因 `x86_fp80` 尚未支持而退出，
vsftpd 的 bottom-up 约束生成耗时较长。Bench2 rootfs 里还有 ffplay：它属于 ffmpeg 6.1.1，
DebugInfo 中约 111 个已定义函数，规模介于 fortune 和 wrk 之间，适合作为下一档输入。

# 目标

canonicalize 在 oneTBB 可用且调用方允许并行时默认并行，同时保留清楚、可验证的串行退路。
源码级 IR 评估固定使用 8 个线程，避免不同机器的硬件线程数影响性能数据。把相关环境变量和
阶段边界写入 `AGENTS.md`，避免再次把串行 bottom-up 误认为 canonicalize 没有并行。

从与 Bench2 二进制一致的 Ubuntu ffmpeg 源码版本构建 ffplay，生成带 DebugInfo 的 whole-module
LLVM 22 bitcode 和文本 IR。只收集 ffplay executable 自身的源码对象，动态链接的 libav 库不并入
该 IR。

# 技术路线

binarysub 只翻转 canonicalize 环境变量未设置时的默认值，不改变线程数优先级和并行算法。
`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=0` 只关闭 canonicalize 并行；
`NOTDEC_BINARYSUB_THREADS=1` 让 bulk simplify 整体使用单线程。评估命令显式设置线程数为 8。

验证同时覆盖默认并行、显式关闭 canonicalize、整体单线程和不带 TBB 的构建路径。对 fortune
重复运行并比较 LLVM IR、HType 和评估结果，确认旧的递归类型文本不稳定没有重新出现。

ffplay 使用 Ubuntu Noble 对应的 ffmpeg 6.1.1 源码，以 LLVM 22/wllvm、`-g -O2` 和 shared
libraries 构建。缺失的通用构建依赖和 SDL2 开发包可直接通过 apt 安装；不使用 Jammy 的旧版
libav 开发头替代源码树内的 6.1.1 头文件。构建成功后用 LLVM 22 工具验证、反汇编并记录规模。

# 风险

- canonicalize 并行历史上出现过等价递归类型的文本表示不稳定；重复输出不一致时不能默认开启。
- 并行只覆盖 top-down bulk simplify 的 canonicalize 和后续部分，bottom-up 约束生成仍是串行。
- 当前机器有 16 个硬件线程；未设置线程数时会使用硬件线程数，因此性能对比必须显式固定 8。
- ffmpeg 构建系统和依赖较多。若 shared build 仍把 libav 实现带入 ffplay bitcode，需要改为按
  executable manifest 只链接三个 fftools 编译单元，不能接受一个包含整个 ffmpeg 的巨大 IR。
- Ubuntu 源码仓库可能只保留更新后的修订版；拿不到 6.1.1-3ubuntu5 时应记录实际版本差异。

# 判断标准

- 默认路径 trace 显示 canonicalize parallel 已启用，8 线程运行时能看到多线程 CPU 使用。
- 两种串行环境变量行为与文档一致，binarysub 自测和主要类型恢复测试通过。
- 两次 8 线程 fortune 输出一致；若不一致，停止默认翻转。
- ffplay 的 `.bc` 能被 LLVM 22 读取，`.ll` 能重新汇编，保留有效 DebugInfo，且函数规模位于
  fortune 和 wrk 附近，没有静态带入整套 libav 实现。

# 实现记录

## canonicalize 默认并行

- `external/binarysub/src/binarysub.cpp:2510` 的 `canonicalize_parallel_enabled()` 改为环境变量
  未设置或为空时返回 true，`NOTDEC_BINARYSUB_CANONICALIZE_PARALLEL=0` 仍关闭并行。
- `external/binarysub/src/binarysub-test.cpp:640` 的
  `test_bulk_simplify_parallel_options()` 增加默认开启和显式关闭的 trace 检查。
- `AGENTS.md:278` 记录两个并行环境变量、bottom-up 串行边界和源码级评估固定 8 线程的命令。
- binarysub 子模块提交：`317b95a Enable canonicalize parallel by default`。

验证通过：

- `cmake --build ./build --target binarysub notdec -j4`
- `./build/binarysub`
- oneTBB OFF 独立构建和自测
- `notdec.type_recovery.llvm_ir.tr_level_2`
- 默认 8 线程 trace 为 `threads=8 tbb=1 canonicalize_parallel=1`
- 显式关闭时为 `threads=8 canonicalize_parallel=0`，整体单线程时为
  `threads=1 canonicalize_parallel=0`
- 两次 fortune 默认 8 线程运行的 `.ll` 和 `.htypes` SHA-256 分别一致，耗时为
  11.89 秒和 11.85 秒

SysY 现有 9 个 golden 失败在显式关闭 canonicalize 后仍可复现；realworld fortune 的 extra
constraints anchor 也已过期。这两项不是本次并行默认值导致，没有更新无关 golden。

## ffplay IR 与评估入口

- 通过 apt 安装 `libsdl2-dev` 和 `nasm`，取得与 Bench2 一致的 Ubuntu ffmpeg
  `7:6.1.1-3ubuntu5` 源码。
- 使用 LLVM 22/wllvm、`-g -O2` 编译 `fftools/cmdutils.c`、`opt_common.c` 和 `ffplay.c`，
  生成 `/sn640/NotDec-Exp/Bench2/source-ir/ir/ffplay/ffplay.bc`、`.ll` 和 manifest。
- IR 有 108 个已定义函数、3 个 `DICompileUnit`、184 个 struct DebugInfo 类型；LLVM 22
  `llvm-dis`、`llvm-as` 和 `opt -passes=verify` 均通过。
- 首次评估暴露合法 block 顺序下 PHI 被重复创建的问题。`src/TypeRecovery/mlsub/MLsubGenerator.cpp:7494`
  的 `visitPHINode()` 改为复用已有节点；`unittests/Retypd/MLsubGeneratorTest.cpp:88` 增加
  latch 先于 header 的回归测试，单测通过。
- 修复后全量 ffplay 已越过该断言，但在串行 bottom-up 阶段运行数分钟仍未完成。按后续要求
  停止全量运行，改用只保留部分函数的 IR 测试后续合并策略。

## 评分

- 实现效果：8/10。并行默认值、串行退路和 ffplay IR 已完成；全量 ffplay 尚未得到评估汇总。
- 复杂度：9/10。binarysub 只翻转一个默认值；PHI 修复复用已有 API，没有新增状态。
- 维护成本：9/10。环境变量和阶段边界已写入 `AGENTS.md`，默认值与回归测试绑定。

当前更合适的后续方案是从 ffplay 提取少量相关函数形成带 DebugInfo 的裁剪 IR，先用它验证
调用参数/返回值合并策略；全量性能数据等串行 bottom-up 再优化后补测。

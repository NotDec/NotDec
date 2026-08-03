# binarysub canonicalize 单次冻结优化实现记录

> 用户原始 prompt：
>
> 按这个改进一下试试

## 背景

`TypeSimplifier::canonicalizeType()` 原来先把所有 bound 和 `ty` 合成 arena
节点 `res`，递归替换它的 record、函数和 pointer 子项后，再新建 `adapted`。`CompactTypeArena`
是 bump arena，`res` 即使马上不用也会一直留到 simplify 结束。

本次只去掉这层外部 `res`。bound 与 `ty` 的合并顺序保持为“全部 bound 在前，`ty` 在后”，
避免改变 positive record intersection 的语义。

## 实现

- `external/binarysub/src/binarysub.cpp:668-884`：`CompactTypeSlot` 增加
  `materialize()` / `replace()`；`CompactTypeBuilder::transformChildren()` 在
  builder 仍持有父节点组件时，按原顺序改写 record field、函数参数/返回值和 pointer
  load/store。普通路径直接替换；显式 timeout 模式先暂存所有替换，只有所有子项成功才提交。
  这样 timeout 会返回原始合并树，不会返回半改写树。
- `external/binarysub/src/binarysub.cpp:1197-1211`：materialize 内部 child builder
  时只冻结该 child，并把结果留在 slot 中；最终 `freeze()` 复用这个值。
- `external/binarysub/src/binarysub.cpp:2432-2493`：`canonicalizeType()` 直接向
  `CompactTypeBuilder` 追加每个 bound 和最后的 `ty`，插入 active-path key 后调用
  `transformChildren()`，最后只 `freeze()` 一次。child polarity、递归 key 和
  `finalizeRecursiveVar()` 保持原逻辑。
- `external/binarysub/include/binarysub/binarysub-test.h:19`、
  `src/binarysub-test-main.cpp:48`、`src/binarysub-test.cpp:1054-1094`：增加
  `test_canonicalize_builder_single_freeze()`。它通过带一个 record field 的 root
  覆盖事务式 child 改写，并固定 arena 节点数为 5；旧的父 `res` 路径会多出第 6 个节点。

## 测量

输入固定为 ngIRCd：
`/sn640/NotDec-Exp/Bench2/source-ir/ir/ngircd/ngircd.bc`，参数为
`--tr-level=2 --merge-struct-ptr-load-store --fast-work-dir`，8 个 binarysub 线程。
原生 allocator 的输出 SHA-256 都是
`cc9adf69bca00bca6e168911ce690f135f5d6f9b408452400d7199fe21b15733`，LLVM 22 verifier
通过。

| 指标 | active-path 优化后 | 本次两次运行 | 变化 |
| --- | ---: | ---: | ---: |
| wall time | 17.10 s | 17.24 s, 16.38 s | 无可见退化 |
| peak RSS | 3,383,052 KiB | 2,405,452 KiB, 2,399,248 KiB | 约 -29.0% |

第一次新 run 的 merge-eval 指标和旧 run 完全相同：`typed_target_nodes=490`、
`bad_unions=1`、`fragmented_nodes=303`、`fragmented_types=12`。jemalloc run 的最终 IR
也相同，但并行调度让其 merge-eval 中 `bad_unions` 从 1 变成 2；这是已有的递归 binder
选择不稳定，不用它判断本次语义回归。

jemalloc 使用 `prof:true,prof_accum:true,lg_prof_sample:19`。旧 profile 的累计估计为
9,948,832,252 B；新 profile 为 8,319,138,182 B，下降 16.4%。新版 `jeprof` 对 threaded
final heap 在所有 current-inuse 样本为零时会输出空表，因此直接按每个栈的
`[累计对象数:累计采样字节]` 用 Poisson 校正求和。该计算在旧 heap 上得到
9,948,832,253 B，与已保存的 `jeprof` 结果只差 1 B，故可用于新旧比较。

旧累计 profile 中 `CompactTypeArena::mergeAll()` 是 4.839 GB inclusive。现在
`canonicalizeType()` 的 root merge 不再调用它；本次不消除 record/function/pointer
子槽位中仍需要 materialize 的中间 CompactType，所以累计分配和峰值不会按整整一半下降。

## 验证

- `cmake --build build-relwithdebinfo-20260731 --target binarysub -j4`
- `./build-relwithdebinfo-20260731/binarysub`
- `cmake --build build-relwithdebinfo-20260731 --target notdec -j4`
- `NOTDEC_BINARYSUB_THREADS=1 ctest --test-dir build-relwithdebinfo-20260731 -R '^notdec\\.type_recovery\\.llvm_ir\\.tr_level_2$' --output-on-failure`
- ngIRCd 两次完整原生运行、SHA-256 比对、LLVM 22 `opt -passes=verify`
- `git diff --check` 和 `git -C external/binarysub diff --check`

## 评估

实现效果 9/10：完整真实 workload 的峰值 RSS 约减少 29%，累计分配减少 16.4%，输出不变。

理解成本 4/10：新增的 builder child 改写集中在一个函数；timeout 的暂存逻辑比直接原地替换多一些代码，
但避免了显式启用 timeout 后混合新旧子树。

维护成本 3/10：新的节点数回归锁住“只冻结一次”的目标，现有 full binarysub 和 IR suite 覆盖正常路径。
后续若要继续降低峰值，应针对重复 field/function/pointer 子槽位的 materialize，而不是再复制这层父节点逻辑。

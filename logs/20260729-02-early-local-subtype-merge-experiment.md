# 用户原始 prompt

> 为什么完整 vsftpd 很慢，这一块：当前增加函数间的子类型关系的时候，会有这样的问题：当前增加的约束在后面的跨函数约束合并时，可能该约束会直接作为节点合并策略去合并。这就导致前面增加的很多边，可能后面会因为节点合并而直接被合并消除掉。如果能够尽量提前合并，而不是先增加子类型关系再合并，应该能有效提升性能。针对这个思路思考一下，但是有一个潜在的问题就是，如果子类型关系没有增加多少的话，判断节点结构体布局是否冲突的策略可能无法提前发现冲突。比如在当前参数位置，可能目前的节点合并不会带来冲突，但是后面更多的子类型关系被连接上之后，如果再回头看的话，之前的合并策略可能就不应该做出。
>
> 有没有可能，很多C函数里面其实不需要用到子类型特性，而多态用得更多，即主要是识别多态函数？有点好奇，如果把子类型关系（或者只关注指针/结构体指针的子类型关系）都弄成允许合并（比如放宽那个函数内的局部值之间的合并策略到直接允许？），但是依然正确标注多态函数，那么，是否会出现很多类型冲突？会在哪里出现类型冲突？用更小的比如ffplay 和fortune验证一下

# 背景

当前调用约束先统一加入 subtype，求解后才事务化合并函数参数和返回槽。普通的同函数 var-var
subtype 又可能被 `shouldMergeSameFunctionStructPtrSubtype()` 直接改成节点 merge。因此部分调用边先传播，
随后又因节点合并消失，完整项目可能为这些临时边付出较大代价。

但 subtype 表示兼容方向，merge 表示两端是同一类型。提前 merge 只能检查当时已经存在的 primitive 和
结构体布局；后续才接入的调用边可能带来新冲突，而已提交 merge 不能自动拆开。多态标记可以隔离
`void *` buffer/context API 的不同调用点，但不能覆盖函数内派生值、不同参数槽和真实的宽窄类型转换。

# 目标

用默认不生效的实验模式比较三种同函数 subtype 处理：现有结构体指针规则、所有 pointer-like 规则、
所有同层同位宽局部值规则。保持当前内置和显式多态标记不变，在 fortune 和裁剪 ffplay 上比较耗时、
节点合并、subtype 数、solver 冲突、DebugInfo wrong merge 和 fragmentation。

# 技术路线

给 same-function subtype hook 增加环境变量选择的实验模式。默认模式仍要求 pointer-like 且至少一端有
结构体证据；pointer 模式去掉结构体证据要求；all-local 模式只保留同函数、同 level、同位宽条件。
workdir 增加汇总文件，记录各 SCC 的候选、接受次数和触发模式，避免只从最终节点数猜测。

先用单测确认三档边界和默认行为，再在相同 8 线程配置下运行 fortune 与 ffplay 小 IR。重点检查
`bad_unions.jsonl` 中首次新增的类型组合及 merge reason；solver 没有报错但出现 bad union，正是“冲突证据
后到、早期 merge 无法撤销”的直接反例。

# 风险和判断标准

- 环境变量未设置时，输出、合并数量和现有测试必须不变。
- pointer/all-local 模式只用于实验，不能因小样例更快就改成默认。
- DebugInfo coverage 很低时，零 bad union 只能说明已覆盖节点未发现问题。
- 若放宽模式减少时间且不增加 wrong merge，再用完整 ffplay 或 vsftpd 子集扩大覆盖；若已在小 IR 出现
  wrong merge，则直接定位来源，不再据此改默认。

# 实现记录（已完成）

## 修改

- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:229-250,356-368,481-510`：
  `ConstraintsGenerator` 增加两个默认关闭的实验开关、hook/merge/subtype rejected 计数和环境变量解析；
  `addSubtype()` 保持原有忽略 solver rejected 的行为，只记录原因。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2065-2175,2982-2990,3137-3181,4517-4549,5337-5341`：
  `writeLocalSubtypeMergeStats()` 输出统计；`configureConstraintContext()` 区分首次边和后到 bound 触发的
  policy merge event；`shouldMergeSameFunctionStructPtrSubtype()` 支持 `struct-pointer`、`pointer`、
  `all-local` 三档；`applyDeferredCallConstraints()` 支持先事务合并调用槽、后加原 subtype 的实验顺序。
  默认仍先加入所有 call subtype，再按完整证据做事务合并。
- `unittests/Retypd/MLsubGeneratorTest.cpp:314-405`：增加三项测试，覆盖 pointer 模式、all-local 的同函数
  边界和 early-call 的实际处理顺序。
- `DEBUG.md:235-259`：记录 `LocalSubtypeMergeStats.txt`、环境变量、默认行为和后到证据风险。

## 结果

fortune 三档结果如下。`performance.wall_ms` 有正常波动，输出 IR SHA-256 均为
`2d8e07d57e2357c0998d91e67cf18f98270ac32c395f3bf83b5177e9be564237`。

| 模式 | merged / representative | wall | peak RSS | bad unions |
| --- | ---: | ---: | ---: | ---: |
| struct-pointer | 167 / 2257 | 3711 ms | 720 MiB | 0 |
| pointer | 249 / 2175 | 4009 ms | 747 MiB | 0 |
| all-local | 503 / 1921 | 3734 ms | 701 MiB | 0 |

fortune 只有 `33/1159` 个目标节点有 strict DebugInfo 类型。ffplay 的 codec/filter 两个历史错合并小组合
在三档下也都是零 bad union，但分别只有 `15/455`、`8/375` 覆盖；all-local 已把 codec 的 merged
nodes 从 33 增到 82，把 filter 从 26 增到 57，不能据此认为安全。

完整 ffplay 结果更明确。三档最终 IR SHA-256 均为
`d3f53f7a4276366410c98d6b7008a41fcb5d52b8db7a9f3786e1527e331d257f`，LLVM 22 verifier 通过。

| 模式 | merged / representative | eval wall | peak RSS | fragmented nodes/types | bad unions |
| --- | ---: | ---: | ---: | ---: | ---: |
| struct-pointer | 692 / 9706 | 36522 ms | 2505 MiB | 31 / 19 | 0 |
| pointer | 909 / 9486 | 57134 ms | 4568 MiB | 29 / 19 | 0 |
| all-local | 2073 / 8322 | 47063 ms | 4561 MiB | 29 / 19 | 0 |

仅放宽 pointer-like 也明显变慢、增内存；all-local 代表节点更少，仍更慢。原因是 merge 会递归改写两端
已有 upper/lower bounds，产生更大的交集、record 和 canonicalize 中间结构。节点数减少不能抵消这部分
成本。完整 ffplay strict coverage 也只有 `107/5539`，零 bad union 仍不能证明未覆盖节点安全。
同版本 `/usr/bin/time` 外层结果是默认 42.71 秒、pointer 65.81 秒；峰值分别为 2565576、4678616 KiB。

默认完整 ffplay 共执行 233 次同函数 subtype policy merge event：161 次在首次处理 var-var 边时直接发生，
72 次由后到的结构体或 pointer bound 触发。后者正是“边先传播，之后又被 merge 消除”的实际规模。
这些 event 包含事务后来回退的尝试；最终图仍以 `merged_nodes=692` 为准。它证明该现象存在，但在 ffplay
中远小于 9796 次顶层 `addSubtype()`，也没有形成提前调用槽合并的明显收益。

提前调用槽事务 A/B 结果：

| 输入 | 提前合并对数 | 默认 wall | early wall | 结果 |
| --- | ---: | ---: | ---: | --- |
| fortune | 52 | 3642 ms | 3299 ms | 节点数、fragmentation、输出相同 |
| ffplay codec/filter 小组合 | 20 / 9 | 567 / 440 ms | 566 / 440 ms | 无可测收益 |
| 完整 ffplay | 337 | 32576 ms | 32258 ms | merged nodes 692 -> 693 |

完整 ffplay 只改善约 1%。更重要的是 fragmentation 从 `31 nodes / 19 types` 变成
`36 nodes / 17 types`：`AVFormatContext`、`AVFrame`、`AVStream` 更集中，但 `AVClass` 从 3 个 component
变为 8 个，并新增 `AVCodec` fragmentation。early 阶段缺少结构体证据的槽会被跳过，加入 call subtype
后证据虽已补齐，候选却不会重试；已经提交的 merge 也不能因后到冲突自动拆开。因此不改默认顺序。

默认和实验顺序的 subtype rejected 次数相同：fortune 81 次，ffplay codec/filter 为 9/3 次，完整
ffplay 为 232 次。实际原因包括 32/64 位宽不一致、`bool/char/uint/sint` 不兼容、函数参数数量不一致、
function/record 和 primitive/record 不兼容。它们说明 subtype 仍承载整数语义、函数参数逆变/返回值协变、
cast/PHI/select 和结构体方向性数据流；正确标注 allocator、raw buffer、`void *` context 等多态边界，只能
隔离通用 API 的不同调用点，不能把这些关系普遍替换成 equality。

完整 vsftpd 的现有运行仍停在 614 函数 level-0 大图的串行 bottom-up 约束和 summary 接入阶段，
10 到 14 分钟内没有进入最终 canonicalize/DebugInfo 评估。已有日志显示大组末尾一次事务合并了 582 对，
但提前调用槽实验在完整 ffplay 上只带来约 1% 收益，所以没有再花一轮完整 vsftpd 的成本验证同一方案。
后续更合适的是给 deferred call subtype 分段计时和统计传播/bound 增长，按 target/slot 去重等价边，并
优化递归 bound 重写和 canonicalize，而不是放宽 subtype 语义。

## 验证

- `cmake --build build --target MLsubGeneratorTest notdec -j4`：通过。
- `./build/bin/MLsubGeneratorTest`：17/17 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- fortune、ffplay 小组合和完整 ffplay 输出均通过 LLVM 22 verifier。
- 完整 ffplay 三档和 early A/B 均为 `bad_unions=0`；coverage 风险见上文。

## 评分

- 实现效果：8/10。明确否定了“更多提前 merge 就会更快”，也量化了较保守 early-call 的收益上限。
- 理解成本：7/10。新增代码只在现有 hook 和 deferred call flush 周围，但多了两个实验配置和一份统计。
- 维护成本：7/10。默认行为不变且有单测；实验开关长期只应用于诊断，不应进入常规跑批。

更好的方案是保留 subtype 语义，先找出重复 call edge 和传播量最大的 target/slot，再做去重、cache 或
局部增量求解。若以后仍要提前 merge，需要保存缺证据的候选并在 bound 变化后重试；若还要处理已提交
merge 的后到冲突，则必须引入更长生命周期的事务或可撤销 equality，复杂度明显更高。

## 后续实验：`std::set` constraint worklist（未采用）

临时将 `external/binarysub/src/binarysub-core.cpp` 中 `constrain_worklist_only()` 和 `constrain()`
的 vector worklist 换成按 `TypeNode *` 排序的 `std::set`，用容器本身去掉尚未 pop 的重复约束对。
没有使用 `TypeRef::operator<`，避免结构比较开销和不同节点被当成同一 key。

ffplay 在 Debug + ASAN、8 线程下交错各跑两次：

| worklist | wall | user CPU | 平均 RSS |
| --- | --- | --- | ---: |
| vector | 33.84s / 32.90s | 71.07s / 66.74s | 2566908 KiB |
| `std::set` | 33.20s / 33.52s | 70.39s / 68.22s | 2585260 KiB |

两者平均 wall 分别为 33.37s 和 33.36s，无可测改善；`std::set` 平均 user CPU 高约 0.6%，
RSS 多约 18 MiB。两种输出的 SHA-256 均为
`d3f53f7a4276366410c98d6b7008a41fcb5d52b8db7a9f3786e1527e331d257f`，`binarysub` 自测通过。
这说明 ffplay 中 pending 重复很少，或者去重收益被红黑树分配和 `O(log n)` 操作抵消。实验代码已撤回，
保留 vector 默认实现。

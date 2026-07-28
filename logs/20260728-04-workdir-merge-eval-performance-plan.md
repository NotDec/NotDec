# 用户原始 prompt

> 增加一个按 ExtValuePtr 缓存最终 label机制试试吧。这个observer 只保存 strict oracle 值的改进也试试

后续 prompt：

> 对，那增加fast-work-dir试试吧

# 背景

完整 ffplay 在普通类型恢复模式下需要约 34 秒。仅开启 workdir 后增至 179 秒，仅开启
DebugInfo merge-eval 后增至 122 秒，两者同时开启需要 267 秒。这两部分额外开销基本相加，
可以分别处理。

workdir 的主要热点是 `formatExtValueMappingLabel()`。`ValueTypes.txt` 和 `VarOrigins.txt` 会为
同一个 `ExtValuePtr` 重复生成 stable 和 verbose label，其中 verbose label 会调用 LLVM
`Value::print()` 并重新构造 `SlotTracker`。merge-eval 的主要热点是在线 merge observer：每次
SimpleType 或 V2N merge 都扫描所有已见值，虽然错合并判断实际只需要带 strict struct-pointer
DebugInfo oracle 的少量值。

# 目标

保持类型推理、节点合并、事务回退和 DebugInfo 判断语义不变，减少 workdir label 重复生成和
merge-eval 对无关值的扫描。ffplay 输出 IR、workdir 调试文件及除性能字段外的评估结果应保持
不变。

# 技术路线

在一次 top-down 类型生成期间维护共享的 `ExtValuePtr -> final label` 缓存，让全部 SCC 的
`ValueTypes.txt` 和 `VarOrigins.txt` 复用完全相同的 label。缓存只在开启 workdir 时创建，普通
路径不增加状态和查询。

merge-eval 收到 value-node 映射时先查 oracle，只跟踪 strict struct-pointer oracle 值，并同时
保存 oracle 指针和 stable label。merge observer 直接扫描这组值，不再对每个普通值反复查询
oracle。最终 coverage、wrong merge 和 fragmentation 仍从完整 AG/V2N 计算，不缩小评估范围。

# 风险和判断标准

- label 缓存必须以完整 `ExtValuePtr` 为 key，不能用可能碰撞的显示字符串代替。
- 缓存只复用字符串，不能改变 label 格式、排序或调试文件内容。
- oracle 指针只可引用初始化后不再修改的 oracle map；需要在数据结构注释中写清生命周期。
- transactional rollback 的 bad-union checkpoint 行为必须保持不变。
- ffplay 四种模式重新计时；输出 IR 哈希一致，workdir 文件一致，`bad_unions=0` 且
  fragmentation 保持 `31 nodes / 19 types`。

# 实现记录（已完成）

## 修改

`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:53-57,388-389` 增加
`ExtValueLabelCache`，并让 `genTypes()` 接收本轮 top-down 共享缓存。
`src/TypeRecovery/mlsub/MLsubGenerator.cpp:1869-2159,5793-5898,5965-5984` 修改
`formatExtValueMappingLabel()`、`appendDebugValueTypes()`、`appendDebugVarOrigins()`、
`ConstraintsGenerator::genTypes()` 和 `MLsubRecovery::topDownPhase()`：同一个完整
`ExtValuePtr` 的 final label 在全部 SCC 中只生成一次。未开启 workdir 时传入空指针，不走缓存查询。

`src/TypeRecovery/mlsub/MergePolicyEval.cpp:282-299,464-550` 增加
`TrackedOracleValue` 和 `StrictOracleValues`。`observeValueNode()` 只记录 strict struct-pointer
oracle，并缓存 oracle 指针及 stable label；两个 collect-truth 路径直接扫描这些记录，V2N merge 只更新
已经跟踪的值。`finish()` 仍扫描完整 AG/V2N，coverage 和 fragmentation 的统计范围没有变化。

## ffplay 结果

环境均为 8 线程、Debug + ASan，前三种模式使用同样的 `perf record` 命令：

- plain：`34.11s -> 34.11s`，普通路径无变化。
- workdir-only：`179.24s -> 115.22s`，减少 35.7%。
- merge-eval-only：`122.23s -> 36.13s`，减少 70.4%。
- workdir + merge-eval：`267.07s -> 114.31s`，减少 57.2%；summary wall
  `261013 -> 108260 ms`。

workdir 新 profile 中 `appendDebugVarOrigins()` 已退出 1% 以上热点；剩余主要开销是每个唯一值第一次
生成 verbose label，`Value::print()` inclusive 约 49.8%。merge-eval 新 profile 中 `getOracle()`、
`observeVariableMerged()` 和 `observeValueMapMerge()` 均退出 1% 以上热点，主要开销回到
`canonicalizeType()`。

## 正确性

- 四种模式输出 IR SHA256 均为 `d3f53f7a...d257f`，组合输出通过 LLVM 22 verifier。
- `bad_unions=0`、`polluted_components=0`，fragmentation 保持 `31 nodes / 19 types`；
  `DebugInfoValueTypes.txt`、`bad_unions.jsonl`、`fragmented_types.jsonl` 与基线 SHA256 相同。
- `CallSlotMergeDecisions.txt` 与基线 SHA256 相同。
- `ValueTypes.txt` 的全部 value label 哈希相同；`VarOrigins.txt` 的 label 多重集哈希相同。整文件中的
  UType 临时编号仍会因 8 线程 canonicalize 的既有非确定性发生变化，不是本次缓存引入。

## 回归

- `./build/bin/MLsubGeneratorTest`：14/14 通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- SysY 仍是已有的 9 个 Clang 14 输入与旧 golden 差异；realworld fortune 仍由 frozen IR 的 extra
  constraint SHA256 锚点阻断，均发生在本次逻辑前。

## 评分

- 实现效果：9/10。merge-eval 额外耗时基本消失，组合模式减少 57.2%；workdir 仍有继续优化空间。
- 理解成本：9/10。缓存只在 top-down 调试输出期间存在；observer 的过滤条件直接对应评估 ground
  truth。
- 维护成本：9/10。没有新增环境变量或改变文件格式；新 oracle 类型若进入 wrong-union 判断，需要同步
  调整 strict 过滤条件。

workdir 的下一步可以复用 LLVM `ModuleSlotTracker`，避免每个唯一值第一次打印时重新扫描 module
metadata。这个改动涉及 llvm2c 的 `ExtValuePtr::toString()` 接口，本次不继续扩大范围。

# Fast workdir 后续实现（已完成）

`include/notdec/DecompilerContext.h:14-24`、`include/notdec/Utils/Utils.h:33-41` 和
`src/Utils/Utils.cpp:25-51` 增加 fast workdir 运行状态。`src/NotDec.cpp:106-125,243-250,300-327`
增加 `--fast-work-dir`，未同时开启 `--gen-work-dir` 时直接报错。

`src/TypeRecovery/mlsub/MLsubGenerator.cpp:2077-2105` 修改
`formatExtValueMappingLabel()`：fast 模式仍按完整 `ExtValuePtr` 缓存 label，但只生成
`toStableString()`，不再生成 verbose `toString(Value, true)`。默认 workdir 格式不变。现有
`toStableString()` 对 `UConstant` 仍可能打印常量，这是少量剩余开销，不在本次扩大范围。

完整 ffplay、Debug + ASan、8 线程下，使用同一二进制且都不带 perf 时，详细 workdir
`114.26s`，fast workdir `36.59s`，减少 68.0%，已经接近 plain 的 `34.11s`；fast 模式峰值内存
为 `2521.9 MiB`。输出 IR SHA256 仍为
`d3f53f7a...d257f`，通过 LLVM 22 verifier；`CallSlotMergeDecisions.txt` 与详细模式 SHA256
一致。`ValueTypes.txt` 和 `VarOrigins.txt` 合计从 `3,861,665` 字节降至 `2,445,200` 字节。

验证：`MLsubGeneratorTest` 14/14 通过；LLVM IR tr-level 2 suite 通过；CLI 已验证单独传入
`--fast-work-dir` 会返回 1 并说明依赖 `--gen-work-dir`。`cmake --build build --target all -j4`
仍被 `external/NotDec-llvm2c` 现有 structuring test 的字符串 payload 与 `BodyBuilder::Payload`
接口不匹配阻断；本次目标 `notdec` 和 `MLsubGeneratorTest` 均已单独构建成功。

- 实现效果：10/10。workdir 额外 wall time 从约 80 秒降至约 2.5 秒。
- 理解成本：9/10。只增加一个显式 CLI 开关，fast 和详细格式的差别直接对应 label 内容。
- 维护成本：9/10。类型推理和文件结构未变；新增 ExtValuePtr 类型时仍由 `toStableString()` 统一处理。

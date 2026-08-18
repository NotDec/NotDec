# 用户原始 prompt

> 详细规划 checkpoint。先讨论吧，不过为什么叫bottom-up checkpoint？simplify的通过CompactType求解HType类型的步骤可以放到整个bottom-up 和top-down之后吧？当前有做top-down吗？可能还没做？

> 对，先按这个拆分一下，topDownConstraintPhase先留空，单独commit，然后再针对checkpoints形成一个规划文件

## 背景

Redis 的类型恢复已经把约束生成和递归传播定位为当前开发反馈的主要等待点。主 SCC 的
`bottomUpPhase()` 中，deferred-call 传播、call-interface transaction、struct-slot merge
和 field-followup merge 合计约 10 分钟，而且主要是单线程。当前流程每次修改 binarysub
simplify 或 CompactType 逻辑时，都要重新支付这段成本，导致后续性能实验的反馈周期过长。

本轮已先把流程边界显式拆开：`MLsubRecovery::run()` 依次执行
`bottomUpPhase()`、空的 `topDownConstraintPhase()` 和 `solveAndLowerTypes()`。
当前名为 `topDownPhase()` 的逻辑实际上只是调用 `genTypes()`，完成
`SimpleType -> CompactType/UType -> HType` 的求解和 lowering，并没有 caller-to-callee
的约束传播。真正的 top-down 约束阶段暂时保留为空。

因此 checkpoint 不命名为 bottom-up checkpoint，而定义为：

> `post-constraint/pre-simplify`：所有会修改 SimpleType 约束图的阶段结束后，
> CompactType 求解开始前。

当前该边界位于空的 `topDownConstraintPhase()` 之后。将来如果实现真正的 top-down
约束传播，checkpoint 仍然放在它之后，不改变 checkpoint 的语义。

## 目标

1. 第一次运行完成完整 bottom-up 约束生成、SCC summary 实例化和递归传播后，把可以继续
   CompactType 求解的完整状态保存到磁盘。
2. 后续运行使用同一份 frozen IR 和配置加载 checkpoint，跳过约束生成阶段，直接进入
   CompactType simplify 和 HType lowering。
3. checkpoint 必须包含 IR 锚点以及 LLVM Value 到 SimpleType 的映射，不能依赖当前进程的
   裸指针地址。
4. 加载失败时必须明确报错并停止，不能使用部分恢复的图继续输出结果。
5. 当前只实现 `post-constraint/pre-simplify` checkpoint，不设计或实现
   `post-simplify` checkpoint。CompactType/UType 完成后到 HType lowering 之间的缓存边界
   暂不作为开发目标。

## 阶段语义

### 当前真实流程

```text
prepareSCC
  -> bottomUpPhase
       函数 visitor
       同 SCC deferred call
       PNDiff solve
       call-interface / struct-slot / field-followup merge
       跨 SCC summary instantiate
       post-summary constraint 和 merge
  -> topDownConstraintPhase
       当前为空操作
  -> solveAndLowerTypes
       每个 SCC 的 CompactType simplify
       CompactType/UType -> HType
       workdir 类型输出
```

`bottomUpPhase()` 逆 SCC 顺序处理，因此 callee 的 summary 在 caller 实例化之前已经
存在。它结束后，各个 `ConstraintsGenerator` 中的 `SimpleType` 图已经包含当前实现的
全部约束传播和 merge 结果。

`solveAndLowerTypes()` 正向遍历 SCC，只读取并求解已经完成的图；它会在
`genTypes()` 开始时归一化 `V2N` 中指向已合并变量的条目，然后构造 polar roots，调用
`TypeSimplifier::bulkSimplifyDetailed()`，最后由 `TypeBuilder` 转换为 HType。

### 将来的 top-down

如果将来 `topDownConstraintPhase()` 开始实现 caller-to-callee 类型需求传播，它可以
修改 `SimpleType` 图。因此 checkpoint 的保存点必须位于该函数之后，不能在
`bottomUpPhase()` 内部提前保存并假设图已经最终稳定。

当前空阶段仍然保留在调用链中，目的是让 checkpoint 和后续真正的 top-down 实现拥有
稳定的生命周期边界。

## 用户接口

采用显式的目录参数，不自动发现或自动加载 checkpoint：

```text
--emit-post-constraint-state=<directory>
--load-post-constraint-state=<directory>
```

约束：

- 两个选项不能同时使用。
- 只允许 `.ll` / `.bc` frozen stage-B 输入。
- 必须使用 `--frozen-tr-input-ir`。
- 必须满足 `--tr-level >= 2`。
- 加载 checkpoint 时仍需提供当前输入 IR，让 NotDec 重建 LLVM Module、CallGraph 和
  Value selector；checkpoint 内嵌的 IR 仅作为内容锚点和复现材料，不通过裸指针替代当前
  Module。
- checkpoint 保存成功后默认继续执行 simplify；后续如需只生成 checkpoint，可增加独立
  的 diagnostic stop，但不把保存动作和当前 group stop 语义混合。

配置字段需要从 `src/NotDec.cpp` 经 `DecompilerContext` / `DecompileConfig` 传递到
`MLsubRecovery`，最终由 `run()` 在阶段边界决定 save 或 load。加载路径不能再次调用
`bottomUpPhase()`。

## 磁盘布局

```text
<checkpoint-dir>/
  manifest.json
  input.ll
  input.bc
  simple-type-graph.bin
  value-mapping.bin
  pndiff-state.bin
  diagnostics.bin
```

第一版可以先使用临时目录写出所有文件，完成校验和 flush 后再原子 rename 到目标目录。
目标目录已经存在时拒绝覆盖，避免半写入 checkpoint 被误认为有效状态。

### manifest.json

manifest 是加载前的快速校验层，至少包含：

- `format_version`
- `stage = "post-constraint/pre-simplify"`
- frozen IR 的 canonical text SHA-256
- data layout 和 pointer size
- tr-level
- struct-pointer load/store merge 等影响约束图的开关
- summary override、signature override、extra constraints 的文件摘要
- polymorphic/allocator 配置摘要
- binarysub state ABI 版本
- 每个 SCC 的稳定名称、level、函数名称列表和 graph 统计

`format_version` 处理文件布局变化；`binarysub state ABI` 处理
`TypeNode` / `VariableState` 内部语义变化。任何不匹配都必须在加载前报错。

## SimpleType 图 snapshot

### 保存内容

在 `external/NotDec-bin2llvm` 不涉及本功能；图 snapshot 属于
`external/binarysub` 的 `binarysub-core` 层。新增一个不暴露裸容器内部细节的 snapshot
接口，保存所有从 checkpoint roots 可达的节点：

- `VariableState.id / level / size`
- `VariableState.mergedInto`
- lower bounds 和 upper bounds
- primitive 节点
- function 节点的参数和返回值
- memory/record 节点的字段
- ptr-load / ptr-store 节点的目标和 access size
- `globalTypeContext().varSupply.next`

节点使用 checkpoint-local 整数 ID，而不使用进程地址。结构节点允许共享；同一节点在
多个 field、bound 或 root 中只能保存一次。

### 恢复顺序

恢复不能边读边递归创建，否则互相引用的变量和结构节点会出现前向引用问题。采用两遍
恢复：

1. 创建所有变量节点并建立 checkpoint ID -> `SimpleType` 表。
2. 创建 primitive/function/record/memory/ptr 节点，按 ID 表解析引用。
3. 恢复每个变量的 bounds 和 `mergedInto`。
4. 恢复所有 SCC roots。
5. 重建派生索引：`lowerBoundSet`、`upperBoundSet`、ptr-load/ptr-store 索引和
   `nestedBoundUsers`。
6. 校验每个变量的代表、bound 数量和图统计与 manifest 一致。

`SimpleTypePool` 的 intern 表和节点生命周期必须由 binarysub 提供正式 restore 入口；
NotDec 不直接修改 `TypeNode::variant` 或 pool 私有成员。所有恢复 API 都需要明确
说明 graph ownership、hash-consing 和 derived-index invariant。

### 变量 ID 和后续 fresh variable

变量 ID 会影响稳定输出和后续类型实例化。除了保存每个变量的原始 ID，还必须保存并恢复
全局 fresh-variable supply 的下一个值。加载后新建变量不能与 checkpoint 中的 ID 冲突。

## LLVM Value 映射

每个 `ConstraintsGenerator` 至少保存：

- `V2N` 的 key -> graph node ID
- `ContraVariantValues`
- `OriginalVariableSources`
- memory/storage root
- storage field name -> graph node ID
- EVM store evidence 中的 address、stored value、source instruction selector

进程内的 `llvm::Value *` 只能作为运行时查找键，地址在重新解析 IR 后会变化，不能进入
checkpoint。现有 `toStableString()` 和 `SelectableValues.txt` 已经提供跨进程的可读定位方式：
函数/全局使用符号名，参数使用函数名和参数序号，有名指令使用函数名和 SSA 名，无名指令
使用函数、基本块及块内指令序号，常量则补充 user 和 operand index。它们可以复用为诊断、
配置和加载校验接口，但不表示跨 IR 版本的语义身份。只要指令、基本块或 module 顺序变化，
无名 value 的定位也会变化；checkpoint 因此只支持 SHA-256 完全一致的 frozen IR，不尝试把
旧状态迁移到优化或重排后的 IR。

磁盘格式为所有映射涉及的值建立统一的 checkpoint-local `ValueId` 表。普通 mapping 只保存
`ValueId -> graph node ID`，避免在 Redis 规模的数据中反复写长字符串；`ValueId` 对应的表项
则使用 typed/tagged selector，至少区分：

- 普通 LLVM Value
- ReturnValue 和 return index
- UConstant 及其 use site
- ConstantAddr
- StackObject
- HeapObject

其中普通 value 的结构化 selector 至少记录 function/global、argument index，或
function/block/instruction 的定位字段；UConstant 必须保留 user selector 和 operand index，
因为文本相同的常量在不同 use site 可以对应不同类型节点。ReturnValue、StackObject 和
HeapObject 是类型恢复构造的语义对象，也不能退化为普通 `Value *`。

加载时按当前 Module 的稳定遍历顺序重建 `ValueId -> ExtValuePtr`，并用 typed selector 和
stable string 交叉校验。出现 IR hash 不匹配、selector 缺失、重复、kind/type 不一致或
use-site operand 越界时，整个加载失败，不能回退到地址、仅按常量文本匹配或选择第一个
候选。这样保存和加载进程中的地址可以完全不同，恢复后的 V2N 仍绑定到当前 Module 的新
指针。

## SCC 和 Generator 恢复

加载时仍然先执行 `prepareSCC()`，因为 LLVM CallGraph 和 `SCCData` 需要当前 Module
中的对象地址。随后不执行 `bottomUpPhase()`，而是按 `AG.AllSCCs` 创建空的
`ConstraintsGenerator` shell，再把 checkpoint 中的数据装回对应 generator。

恢复时校验：

- SCC 数量一致。
- SCC 稳定名称和 function name 集合一致。
- SCC level 一致。
- opaque body 集合一致。
- memory/storage root 的共享关系一致。
- polymorphic summary 边界和 checkpoint manifest 一致。

`unhandledCalls` 在完整 bottom-up 后原则上应为空；如果 checkpoint 发现仍有未处理调用，
应拒绝保存或拒绝加载，而不是尝试在 solve 阶段补实例化。

## PNDiff、EVM 和评估状态

### PNDiff

为了让加载 checkpoint 后仍能生成 residual PNDiff warning 和 annotated IR，计划保存：

- PNI value selector -> PNI node
- PNI node lattice type、ID 和 DSU representative
- add/sub constraints 及 instruction selector
- residual constraint/worklist 状态
- `NextConsSeq` 等影响确定性的计数器

PNDiff 的回调函数不序列化，加载后根据当前 Module 和恢复后的 selector 重新绑定。

### EVM evidence

EVM store evidence 只引用 LLVM instruction 和 ExtValuePtr，因此通过同一套 selector
恢复。普通 Redis 输入不使用该部分，但不能让通用 checkpoint 格式丢失这些字段。

### MergeEval

MergeEval 的最终 coverage 可以从恢复后的 V2N 和 graph 重新计算，但历史 bad-union
witness 发生在 merge 当时，不能仅凭最终图恢复。第一版有两个可接受行为：

1. 在 checkpoint 中保存 MergeEval 的历史 records 和计数；或
2. 加载 checkpoint 且请求 `--merge-eval-dir` 时显式报错，避免生成不完整评估。

推荐先实现第二种安全行为，再根据 Redis checkpoint 的实际使用需要增加可选的 eval event
snapshot。不能静默丢失历史 witness。

## 运行时生命周期

### 保存路径

```text
prepareSCC
bottomUpPhase
topDownConstraintPhase   // 当前为空；未来可能修改 SimpleType 图
savePostConstraintState  // emit 选项启用时
solveAndLowerTypes
```

保存动作必须位于 top-down constraint boundary 之后、`solveAndLowerTypes()` 之前。保存
过程不能调用 `bulkSimplifyDetailed()`，也不能清理 binarysub graph。

### 加载路径

```text
prepareSCC
validate manifest and IR anchor
restore SimpleType graph and mappings
solveAndLowerTypes
```

checkpoint 的 stage 已经位于 `topDownConstraintPhase()` 之后，因此加载时必须同时跳过
bottom-up 和 top-down constraint phase。将来加入真实 top-down 传播后，生成 checkpoint
的 fresh run 负责执行它；load run 不能再次传播，否则恢复状态不再等价于保存点。

加载成功后日志需要明确报告：

- checkpoint 目录
- IR hash
- SCC 数量和 graph node/edge 数
- restore wall time、CPU time、RSS
- bottom-up skipped=true

加载失败必须不进入 HType 输出，也不能生成看似成功的部分 workdir 类型文件。

## 实现阶段

### 阶段 1：binarysub graph snapshot

- 定义版本化 snapshot record 和 restore API。
- 实现变量、结构节点、bounds、merge representative 的两遍恢复。
- 重建所有派生索引。
- 增加 synthetic graph round-trip 测试，覆盖 recursive bounds、record/function、
  ptr-load/ptr-store、merge 和 nested-bound users。

### 阶段 2：NotDec semantic checkpoint

- 扩展 CLI、`Options`、`DecompileConfig` 和 `MLsubRecovery` 配置。
- 实现 manifest、IR copy、hash 校验和 atomic directory write。
- 恢复 SCC generator、V2N、contra/origin mapping、memory/storage roots。
- 先完成 Redis 类型求解所需的 semantic state。

### 阶段 3：辅助状态和严格失败路径

- 接入 PNDiff snapshot/restore。
- 接入 EVM evidence。
- 对 MergeEval 先实现 load-time rejection，随后评估 event snapshot 是否值得增加。
- 增加 manifest mismatch、selector collision、truncated file、未知 schema 的负向测试。

### 阶段 4：真实输入验证

- `10_BottomUp1.ll` fresh/save/load 对照。
- 递归和 polymorphic 小样例对照。
- Redis stage-B IR 生成一次 checkpoint。
- 多次从 checkpoint 加载并运行 simplify，比较 HType、ValueTypes、LLVM verifier、
  group progress、PathMemo 统计和 RSS。

## 风险和控制

- **图语义变化**：binarysub 修改 `VariableState` 或 bound invariant 时必须提升
  `binarysub_state_abi`，旧 checkpoint 拒绝加载。
- **LLVM selector 不稳定**：selector 必须带 function/instruction/use-site 上下文，并在
  加载时检查唯一性；它只在 frozen IR hash 完全一致时有效，不承诺跨 IR 版本迁移。
- **hash-consing 顺序变化**：checkpoint 保存稳定 node ID 和变量 ID；需要额外固定结构
  节点恢复顺序，避免仅因指针顺序改变 group 分组。
- **全局类型池污染**：restore 前必须确认当前进程没有遗留另一份 MLsub 图；必要时增加
  dedicated restore context 或显式清空未使用的 global pool。
- **文件过大**：graph 使用流式二进制格式，不用 JSON 保存节点；首版不引入压缩，先测
  写入/加载时间和峰值 RSS，再决定是否加 zstd。
- **诊断状态缺失**：任何无法恢复的诊断历史都必须在 manifest 中标记，并在相关选项下
  显式拒绝，而不是输出不完整的评估结果。

## 判断标准

1. 首次运行生成 checkpoint 后，manifest、IR hash、图统计和 mapping 统计完整。
2. 小样例 save/load 的 `ValueTypes.txt`、`ValueHTypes.txt`、`ImportantHTypes.txt`、
   `VarOrigins.txt` 与 fresh run 一致，输出 IR 通过 LLVM 22 verifier。
3. Redis load run 不进入 bottom-up；checkpoint 加载时间单独报告，并明显短于重新生成
   约束图的时间。
4. checkpoint 加载后的 simplify group 数、最大 group 行为、HType 结果和错误合并指标
   与 fresh run 可解释且无新增错误。
5. IR/config/schema 不匹配时，程序在 solve 前失败，不生成部分成功结果。
6. 当前空的 `topDownConstraintPhase()` 保持独立调用点；未来加入真实传播时，不需要改变
   checkpoint 文件的阶段语义，只需把保存点放在该阶段完成之后。

## 当前不做

- 不实现真正的 top-down constraint propagation。
- 不设计 post-simplify checkpoint。
- 不把约束事件日志作为主要恢复机制。
- 不自动发现或自动加载旧 checkpoint。
- 不为了 checkpoint 顺便重构 CompactType/UType 或 HType lowering。
- 不在本计划中决定 Redis 后续并行化算法；并行化仍依赖无 group 上限 perf 和
  checkpoint 加速后的实验反馈。

## 实现记录（2026-08-18）

### 已完成

- **阶段边界**：`src/TypeRecovery/mlsub/MLsubGenerator.cpp:5855` 的
  `MLsubRecovery::run()` 保持 `bottomUpPhase()` -> 空的
  `topDownConstraintPhase()` -> `solveAndLowerTypes()` 顺序；save 位于空阶段之后，load
  直接跳过两个约束阶段。没有实现真正的 top-down propagation，也没有加入 post-simplify
  checkpoint。
- **SimpleType 图**：`external/binarysub/include/binarysub/SimpleTypeSnapshot.h:72-86` 和
  `src/SimpleTypeSnapshot.cpp:181,307,546,609` 新增版本化二进制 snapshot/restore API。节点、边、变量 ID、
  merge representative、fresh-variable supply 都使用 checkpoint-local ID；恢复时重建
  bound set、ptr-load/store index 和 `nestedBoundUsers`。bound vector 的重复槽位会保留，
  因为 merge rewrite 可能只去重存在性索引而不重排遍历向量。
- **CLI 与原子目录**：`src/NotDec.cpp:144-150,308-337`、
  `include/notdec/DecompilerContext.h`、`include/notdec/Passes/PassManager.h`、
  `src/Passes/PassManager.cpp:496` 接入
  `--emit-post-constraint-state=<dir>` 和 `--load-post-constraint-state=<dir>`，校验
  frozen `.ll/.bc`、`tr-level >= 2`、互斥选项和已有目标目录。新增的
  `src/TypeRecovery/mlsub/PostConstraintCheckpoint.cpp:1368,1452` 的
  `MLsubRecovery::savePostConstraintState()` / `loadPostConstraintState()` 写入临时目录后
  rename，manifest 校验 stage、IR SHA-256、target/data layout、配置摘要、ABI 和图统计。
- **Value 与 SCC 状态**：`src/TypeRecovery/mlsub/PostConstraintCheckpoint.cpp:264,632,726,801,982` 的
  `ValueCatalog`、`resolveValueSelectors()` 和 semantic state serializer 不保存
  `llvm::Value *`。typed selector 覆盖普通 Value、ReturnValue、UConstant、ConstantAddr、
  StackObject、HeapObject，并遍历指令及按需遍历常量 DAG/全局 initializer。SCC 名称、函数
  集合、V2N、contra/origin、memory/storage roots、storage fields 和 EVM store evidence
  均可恢复。
- **PNDiff**：`external/binarysub/include/binarysub/PNDiff.h:266`、`src/PNDiff.cpp:623` 增加
  `restorePNINode()`，checkpoint 保存等价类、lattice/conflict、add/sub constraints、
  worklist、instruction selector 和 `NextConsSeq`，恢复后可继续生成 annotated IR 与
  warning。Merge-decision history 尚未保存；load 若请求 `--merge-eval-dir` 会明确失败，
  manifest 也标记 `merge_eval_history_saved=false`。
- **诊断与失败路径**：save/load 日志增加 wall、CPU 和 peak RSS；二进制 reader 会报告
  实际文件名。selector 缺失/碰撞、hash/config/schema 不匹配、图/mapping/PNDiff 截断、
  save/load 同时指定和 merge-eval 冲突均在 solve 前失败。

### 验证结果

- `10_BottomUp1.ll`、ConstantAddr、Stack、EVM heap、EVM aggregate return、EVM storage
  bytes 等 save/load 对照中，`ValueTypes.txt`、`ValueHTypes.txt`、`ImportantHTypes.txt`、
  `VarOrigins.txt`、`PNDiff.warn.txt`、`03-pndiff-final.ll` 和输出 IR 均逐字节一致；非
  `--fast-work-dir` 模式也通过。输出 IR 用 LLVM 22 `llvm-as` 检查。
- binarysub 新增 `test_simple_type_snapshot_round_trip()` 覆盖 recursive bounds、重复
  bound vector、record/function（含 void null result）、ptr-load/store、merge、nested
  users、fresh ID 和截断输入。完整 `./build/binarysub` 仍在既有
  `test_local_persistent_set()` assertion 处提前失败，未进入该测试。
- lighttpd frozen stage-B（约 6.3 万图节点、5.3 万 selector）保存约 9 秒、checkpoint
  约 23.6 MB；修复重复 bound vector 后 load 成功，restore 日志约 18 秒、peak RSS 约
  300 MiB。该结果表明 checkpoint 复用成本明显低于重新生成约束，Value selector 扫描和
  PNDiff/图恢复仍是后续可优化的对象。
- 手工负向测试覆盖互斥参数、缺少 frozen、已有目标目录、IR hash mismatch、三类截断
  文件和 merge-eval load rejection，均按预期非零退出且没有 HType 输出。

### Redis 验证

Redis stage-B 使用固定 frozen IR、allocator summary、ASLR disabled、8 threads 和
`NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS=1` 验证，checkpoint 位于
`/tmp/notdec-checkpoint-redis.g9ke4Q/state`：

- bottom-up 到保存边界 `9:53.06`；save 函数自身 `wall=52.814 s`、`cpu=52.809 s`、
  `peak_rss=1,797,652 KiB`。图 `314,398` nodes / `201,150` roots，Value selector
  `252,955`，文件合计约 `88 MiB`（graph 60 MB、mapping 22 MB、PNDiff 5.4 MB）。
- load 恢复函数 `wall=13.444 s`、`cpu=13.443 s`、`peak_rss=1,226,120 KiB`，日志明确
  输出 `bottom_up_skipped=true top_down_skipped=true pndiff_state_restored=true`。
  这是约束生成近十分钟与 checkpoint 恢复十几秒的直接对照。
- load 后第一个 simplify group 仍需约四分钟，整次被诊断停止时 peak RSS 约 3.7 GiB；
  因此 checkpoint 只缩短前置约束反馈，不代表 CompactType simplify 已解决。保存和加载
  run 都在 checkpoint 完成后主动终止，未把部分 simplify 结果当成完整回归依据。

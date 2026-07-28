# 用户原始 prompt

> 目前考虑维护一个修改日志，启动记录后，可以后续根据日志反向操作，回退到启动记录时的状态。当前的SimpleType可以看作一个有向图，每个节点是一个SimpleType，节点间的边带有标记，边的关系包括所有的upperBounds关系，以及lowerBounds关系，且如果bound里面一般是一层比如结构体类型，然后里面某个label是指向其他节点，则这个结构体label也可以看作是边上的标记。从这个角度，从图的底层操作上维护一个修改日志，可能就可以实现回退机制？然后基于回退机制，可以在决策合并某个函数的参数/返回值时，如果在过程中通过约束的递归传递发现了冲突，则可以回退到参数合并前，然后放弃合并这个函数。规划一下这个怎么实现，形成一个规划日志文件

# 背景

当前调用槽策略会先检查一个 formal 槽的已知布局，再依次调用
`merge_variable_into()`。底层合并不只移动当前两个节点的边：它会递归添加约束、排队新的变量合并、
重写结构 bound 里的变量引用，并维护反向索引。`shouldMergeSameFunctionStructPtrSubtype()` 也会在
bound 重写过程中再次触发合并。因此外层预检查通过，不代表实际执行过程中不会碰到原来没包含的节点。

ffplay 的 `configure_filtergraph::arg0/arg3` 就是这个情况：合并 arg0 actual 时重写出
`arg0 <: arg3`，随后 same-function hook 在内部把两者合并。等外层处理 arg3 时已经无法阻止。

# 目标和边界

目标是在 binarysub 的 SimpleType 图上提供一次性事务：创建 checkpoint 后执行若干约束和合并；成功时
提交，发现冲突时恢复 checkpoint 前的可观察图状态。NotDec 用一个事务处理同一目标函数在当前 generator
中的全部参数槽和返回槽；任一递归合并失败时，本轮不保留该函数的任何调用槽合并。

第一版只保证 checkpoint 前已经存在的可达 SimpleType 图完全恢复。事务中临时创建、回退后不再可达的
immutable bound 和变量节点允许留在 TypeContext pool 中，因此全局变量 id、`nodes_created` 和 trace
文本不回退。它们不影响类型语义，但需要在性能评估里单独统计。

事务范围只覆盖当前 `ConstraintsGenerator` 的一次调用接口合并，不回退之前已经提交的其他函数、其他 SCC
或普通约束生成。先不支持嵌套事务，也不把 DebugInfo ground truth 用作生产推理依据。

# 当前需要覆盖的状态

`VariableState` 的可变状态不只有两组 bound：

- `lowerBounds` / `upperBounds`：直接变量边和 function、record、memobject、load/store 等结构 bound。
- `nestedBoundUsers`：结构 bound 内嵌变量引用的反向索引。
- `mergedInto`：变量合并和 `resolve_variable()` 路径压缩使用的父指针。
- `externalHandle`：NotDec 用它反查 LLVM value 和所属函数。

结构 bound 本身按值创建，不原地修改。变量替换会新建 function/record/memobject bound，再替换 owner 的
bound vector，同时更新被引用变量的 `nestedBoundUsers`。因此只记录“添加了哪条边”不够；必须同时恢复
owner、相邻变量和嵌套引用反向索引。

NotDec 侧还有不能直接回退的状态：

- `PNINode::unify()` 会删除 PNDiff 节点并更新约束 worklist。
- `V2N.merge()` 会移动 value 到 SimpleType root 的反向映射。
- `MergePolicyEval` 会累计 merge event 和 bad-union 证据。
- `ObservedOldRelations`、field follow-up candidates 和 trace 也会被追加。

这些状态不能在 SimpleType rollback 后继续保留。

# 技术路线

## 1. binarysub 的 undo journal

采用“变量节点首次写入快照”，不为每一次 vector insert/erase 单独设计逆操作。事务中一个
`VariableState` 第一次即将修改时，journal 保存：节点引用、两组 bounds、`nestedBoundUsers`、
`mergedInto` 和 `externalHandle`。同一节点后续修改不重复保存。

rollback 按逆序恢复快照；commit 直接丢弃快照。快照持有旧 `SimpleType` 引用，能保证被替换的结构 bound
在事务结束前不会释放。`id`、`level`、`size` 不可变，不进入日志。

这比逐边 inverse log 多复制少量邻接 vector，但更容易保证嵌套 bound、反向索引和多次重复修改能恢复到
完全一致的顺序，第一版更稳。性能是否可接受由真实 ffplay 数据判断。

所有底层写点必须经过 journal：

- `addLowerBound()` / `addUpperBound()`。
- 邻居边删除、bound vector 元素替换和 `nestedBoundUsers` 登记/清空。
- `mergedInto`、`externalHandle` 修改。
- `resolve_variable()` 的路径压缩。
- extrusion 在事务内创建变量并给已有变量补边的路径。

事务上下文应显式传入 constrain/merge 内部实现。若 `resolve_variable()` 等公共入口难以完整传参，可用
仅在事务 RAII scope 内生效的 thread-local 当前 journal 兜底；必须保证不同线程互不共享，并在嵌套启动时
直接报错。不能使用一个进程全局 journal，因为 canonicalize 和后续分析存在并行运行场景。

## 2. 原子化的 merge API

在现有 `constrain()` / `merge_variable_into()` 之上增加由调用者持有的 transaction scope，使同一个 scope
可以连续执行多个显式 merge。任一调用返回错误后，调用者只能 rollback，不能继续使用半修改图。

transaction 同时收集但不立即发布两类事件：

- constrain observer 看到的变量对。
- 所有显式、reverse-edge、policy-replace-bound 和 policy-auxiliary merge event。

binarysub 内部用于判断和排队的 hook 仍在事务中同步执行；PNDiff、merge-eval 等只有副作用的 observer
延迟到 commit 后重放。这样 speculative merge 可以正常递归，但 rollback 不会留下外部状态。

NotDec 当前在 binarysub 调用前就执行 PNDiff unify，需要改为提交阶段执行。提交顺序保持现有不变量：先按
记录的 pair 同步 PNDiff 和旧 memory relation，再更新 `V2N`，随后写 merge-eval、field follow-up 和
call-slot decision。rollback 直接丢弃这些待发布事件。

延迟事件必须保留事务执行时的原始节点身份，不能在 commit 时先把事件两端都
`resolve_variable()`。提交前先从尚未更新的 `V2N.rev()` 收集每个事件对应的 value；然后重放 PNDiff 和
评估事件；最后把各旧 root 的 value 一次性移动到事务结束后的最终 root。这样连续和递归 merge 不会因为
中间 root 已消失而漏掉 value，也不会改变当前“先 unify PNDiff、后移动 V2N”的语义。

## 3. 冲突判定

undo journal 只提供恢复能力，不负责判断类型是否冲突。transaction 模式增加一个 merge validator，覆盖
每个实际执行的 merge task，而不只检查最外层显式 merge。validator 至少接收 from、into、merge reason
以及 policy-replace-bound 的原始 lhs/rhs，并返回 allow、conflict 或 unknown。

冲突来源分开处理：

- `constrain_impl()` 返回的 size、level、primitive、function arity、record field 等硬错误，事务失败。
- 现有 `shallow_merge_conflict()` 在普通模式仍可保留“跳过可选 merge”；调用槽事务中要把它上报为明确的
  rejected merge，由外层决定整函数 rollback。
- NotDec validator 对递归产生的结构体指针 merge 再做字段布局检查，并拒绝不同 formal 槽之间仅凭
  “属于同一函数”产生的合并。
- 缺少布局证据是 unknown，不伪装成 compatible。第一版先记录 unknown，是否让 planned merge 因 unknown
  整体失败单独控制在调用槽策略，不塞进通用 binarysub。

需要保留触发链：事务 id、目标函数、最初 planned slot merge、递归 merge reason、冲突两端和布局证据。
这既用于 `CallSlotMergeDecisions.txt`，也用于验证 rollback 的真正原因。

## 4. 按目标函数组织调用接口事务

现有实现把参数和返回值分别按 formal root 处理。需要先把候选整理成 target function，再在函数内按
`argN` / `ret` 分槽。函数体真实 ReturnInst 归到自身函数的 ret 槽；call return 归到 callee 的 ret 槽。

一个目标函数的处理顺序为：

1. 在不改图的情况下完成全部槽的 level、size 和布局预检查。
2. 预检查有明确冲突时直接跳过，不启动事务。
3. 启动一个 transaction，依次执行该函数全部参数和返回槽的 planned merge。
4. 任一递归 constraint/merge 被 validator 判为冲突，rollback 到函数处理前，全部槽记录同一个失败事务。
5. 全部成功后 commit，再统一同步 PNDiff、V2N、merge-eval 和 follow-up candidates。

事务只覆盖当前 generator 里本轮可见的候选。若 formal root 在事务开始前已经和别的函数或别的槽合并，
应记录 `preexisting-alias` 并跳过，而不是声称本事务可以撤销更早的提交。

## 5. 调试输出

扩展 `CallSlotMergeDecisions.txt`，增加 transaction id、target、outcome、planned merges、recursive merges、
conflict reason 和 rollback touched-node 数。外层槽结果区分：

- `committed`：该函数全部槽提交。
- `precheck-skipped`：执行前已有冲突。
- `rolled-back`：执行中发现递归冲突，图已恢复。
- `preexisting-alias`：事务开始前已经污染，当前事务无法处理。

binarysub trace 保留 speculative 事件，但用 begin/commit/rollback 标记事务边界。trace 是历史记录，不随
rollback 擦除。

# 分阶段实施

## 阶段一：binarysub 可回退图事务

先只实现内存 undo journal 和原子 `merge_variable_into()`，不接 NotDec 调用槽。用小图覆盖直接边、结构
bound 内嵌引用、递归 queued merge、路径压缩、externalHandle 转移，以及中途 Error 后的恢复。用稳定图
fingerprint 比较事务前和 rollback 后的全部可变状态。

## 阶段二：延迟外部副作用

把 PNDiff observer、merge-eval 和 V2N 更新收进待提交事件。先让单个
`tryMergeVariablesForPolicy()` 支持 transaction，验证失败后 PNDiff representative、V2N 反向映射和
评估计数都不变化。

## 阶段三：函数级调用接口事务

合并参数/返回值两个入口，按 target function 建 plan，接入 recursive merge validator 和新的 decision
统计。先用人工图复现“planned arg0 merge 重写出 arg0 <: arg3，内部 hook 尝试继续合并”的情况，确认
冲突后整个函数 rollback。

## 阶段四：ffplay 和性能评估

先跑两个最小 wrong-merge 组合，再逐步扩大到完整 ffplay。比较 wrong merge、fragmentation、事务提交/
回退次数、单次最大 touched nodes、临时节点数、wall time 和 peak RSS。

当前 ffplay 的 `AVFilterContext` 一侧没有字段访问，布局结果是 unknown。事务机制本身不会凭空发现完整
源码布局。要消除 `av_opt_set*` 根部污染，还需在这一阶段明确选择：把 generic `void *obj` 入口标成调用点
多态，或让调用槽 planned merge 在任一成员布局 unknown 时整组拒绝。推荐优先标记明确的 generic API，
再评估 unknown 全拒绝带来的 fragmentation，避免把保守策略一次扩大到所有普通函数。

# 测试和判断标准

- rollback 后，事务前已有变量的 bounds、nested users、mergedInto、externalHandle 和 root 解析结果逐项
  相同；重复 rollback、commit 后 rollback、嵌套事务等非法用法有明确保护。
- 结构 bound 内多层引用至少覆盖 function arg/result、record field、direct load/store，不能只测直接变量边。
- 事务失败后 PNDiff、V2N、MergePolicyEval、follow-up candidates 没有变化。
- 一个参数 merge 递归触发另一个参数 merge并发现冲突时，同一目标函数已执行的其他参数/返回 merge 也全部
  撤销。
- 成功事务的最终图与当前非事务实现一致，避免 rollback 基础设施改变正常求解结果。
- 两个 ffplay 最小组合能在 decision 文件中说明提交或回退链路；完整 ffplay 不出现数量级耗时或内存增长。
- LLVM IR、sysy、realworld 类型恢复回归保持通过，最终输出继续通过 LLVM 22 verifier。

# 风险和取舍

- 最大正确性风险是漏记隐藏写点，尤其路径压缩、nestedBoundUsers 和 extrusion。阶段一必须先用图 fingerprint
  测到这些路径，再接真实策略。
- 最大集成风险是先修改 PNDiff 再 rollback SimpleType。必须先完成副作用延迟，不能只给
  `merge_variable_into()` 套一个局部 journal 就接入生产路径。
- 节点级快照会复制高出度变量的 bound vector。它比整图 snapshot 小，但失败事务较多时仍可能变慢；需要
  用 touched nodes 和复制边数衡量，而不是只看总时间。
- 新建节点和 id 不回退会让大量失败试探增加 pool 内存。第一版先统计；若完整 ffplay 明显增长，再给
  TypeContext 增加事务 allocation arena，不在最初实现里同时解决。
- 函数级全回退比槽级回退保守。它符合“放弃合并这个函数”的目标，但可能增加 fragmentation；评估必须同时
  看 wrong merge 和 fragmented nodes，不能只看错合并归零。
- 回退机制不能替代证据策略。unknown 是否允许、哪些 `void *` API 多态，仍需单独判断并在 decision 文件中
  明确记录。

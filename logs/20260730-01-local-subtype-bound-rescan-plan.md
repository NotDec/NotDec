# 用户原始 prompt

> 之前提到的，下面这些风险较低的改动可以尝试一下：1. lower/upper 循环中先用 isVariableState() 过滤。2. 仅在 pointer/struct evidence 首次出现或升级时重新检查变量邻居，不要每新增一个 record/function bound 都全扫。

# 背景

vsftpd 的非 ASAN profile 显示，180 秒采样中 72.88% 位于新增 non-variable bound 后触发的
局部 subtype 合并 hook。当前每次回调都遍历该变量全部 lower/upper bounds，非变量 bound 也会先做
root 解析；同一变量已经有 pointer 或 struct 证据后，后续相同证据仍会重复扫描旧邻居。

# 目标

减少重复 root 查询和邻居扫描，不改变 subtype、节点合并条件和事务回退结果。ffplay 输出必须保持一致，
vsftpd 的 `resolve_variable`、`onVariableNonVarBoundAdded` 和 `shouldMergeSameFunctionStructPtrSubtype`
占比应明显下降。

# 路线

- 遍历 lower/upper bounds 时先检查原始节点是否为变量，只对变量邻居解析 root。
- 每个代表变量记录已经处理过的证据等级：无、pointer-like、struct-pointer。只有等级上升时才重查邻居。
- 节点 merge 会改变邻居集合，因此使合并两端的记录失效；事务执行期间不使用这项缓存，避免回退留下
  超前状态。
- 在 `LocalSubtypeMergeStats.txt` 记录回调、实际重查和过滤数量，并增加针对重复证据与结构体升级的测试。

# 风险和判断标准

- 新增证据可能经节点 merge 到达；merge 后必须允许再次检查。
- 事务回退不能保留事务内推测得到的证据等级。
- binarysub、MLsubGenerator 单测和 LLVM IR suite 必须通过；ffplay 输出哈希不变。
- 若 vsftpd 仍在同一阶段运行很久，用固定时间 profile 比较热点，不以是否跑完整作为判断条件。

# 实现记录（第一步已完成）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3199-3235`：`TryQueue()` 在 root 解析前过滤非变量；
  lower/upper 循环也只把变量 bound 交给该 lambda。没有新增统计字段，也没有改变 merge 或事务路径。
- `MLsubGeneratorTest` 17/17、binarysub 自测和 `notdec.type_recovery.llvm_ir.tr_level_2` 均通过。
- ffplay 输出 SHA-256 保持 `d3f53f7a4276366410c98d6b7008a41fcb5d52b8db7a9f3786e1527e331d257f`。
  RelWithDebInfo wall time 为 6.06s，和此前 5.95s 的 PNDiff lazy-trace 结果处于同一波动范围。
- vsftpd 60s profile：`resolve_variable` 从旧 profile 第一分钟的 44.53% 降到 19.37%；
  `onVariableNonVarBoundAdded` self 加上 lambda 为 39.48%，总 cycles 仅下降约 0.6%。这说明过滤有效，
  但只是在 hook 内转移了部分成本，主要瓶颈仍是重复邻居重查。

下一步先不引入三档状态表；比较是否可以仅根据新加入的 bound 做一次性证据判断，避免维护 merge/rollback
失效逻辑。

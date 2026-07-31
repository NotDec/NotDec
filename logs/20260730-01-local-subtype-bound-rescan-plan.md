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
- 暂不维护三档证据状态。先用新加入 bound 的形状做 fast-fail：默认
  `struct-pointer`/`pointer` 策略只有 function 或 memory bound 可能带来新的 pointer-like 证据，
  primitive bound 不再重查邻居；`all-local` 实验模式保留旧行为。
- `TryQueue()` 和 lower/upper 循环先过滤非变量邻居，避免无意义的 root 解析。

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

# 实现记录（第二步已完成）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3185-3203`：在
  `onVariableNonVarBoundAdded()` 入口增加 bound 形状 fast-fail。默认局部 subtype 策略遇到
  primitive bound 直接返回；`all-local` 保持原有触发行为，避免改变该实验模式语义。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3208-3252`：保留第一步的 `TryQueue()`、lower/upper
  非变量过滤。
- `MLsubGeneratorTest` 17/17、binarysub 自测和 LLVM IR type-recovery suite 通过；ffplay 输出哈希
  保持 `d3f53f7a4276366410c98d6b7008a41fcb5d52b8db7a9f3786e1527e331d257f`，本次约 6.15s，属于
  正常波动。
- vsftpd 固定 30s profile：`resolve_variable` 约从 35.97% 降到 19.13%，但
  `onVariableNonVarBoundAdded` 及其 lambda 升到约 42.51% inclusive，总 cycles 仍只有约 0.6% 改善。
  说明简单过滤有效但把成本转移到 hook 内部；下一步若继续优化，应直接减少重复邻居重查，不能只增加
  更多入口判断。

## 复杂度和维护成本评估

- 实现复杂度：低，只增加常量时间的 bound 形状判断，不维护跨 merge/rollback 的缓存状态。
- 理解成本：低，判断与现有 `hasPointerLikeEvidence()` 的证据定义一致。
- 后期维护成本：低；`all-local` 的显式例外避免实验模式悄然改变。

## 实现记录（第三步：只在证据升级时重查）

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3193-3251`：提取
  `queueLocalSubtypeMerges()`，统一 lower/upper 邻居扫描，并在解析 root 前过滤非变量
  bound。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3269-3375`：新增
  `hasPointerLikeEvidenceBeforeNonVarBound()` 和
  `hasStructPointerEvidenceBeforeNonVarBound()`。`pointer` 模式只在 pointer evidence
  首次出现时扫描，`struct-pointer` 模式在 pointer 或 struct evidence 首次出现/升级时
  扫描；通过跳过刚追加的 bound 判断旧证据，不在节点上保存需要事务回退的状态。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:263-327`：声明扫描 helper、证据判断
  helper 和嵌套 bound 回调。
- `external/binarysub/include/binarysub/binarysub-core.h:546-560`、
  `external/binarysub/src/binarysub-core.cpp:1290-1337,1551-1556`：增加
  `onVariableNestedBoundRewritten` 回调。变量 merge 改写已有 record/function bound 后，
  若新 bound 才出现 struct evidence，立即通知策略层；这样不会靠后续无关 bound 触发全量重查。
- `unittests/Retypd/MLsubGeneratorTest.cpp` 和
  `external/binarysub/src/binarysub-test.cpp:1268-1340`：覆盖重复 function bound 不重复
  扫描，以及 nested record bound 改写能触发回调。

验证：

- `./build/bin/MLsubGeneratorTest`：18/18 通过。
- `./build/binarysub`：全部自测通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过。
- 使用 `build-relwithdebinfo-20260731/bin/notdec` 对完整 vsftpd 做固定 profile；本次约束生成
  在约 12 秒完成，RelWithDebInfo 采样文件为
  `/tmp/notdec-profiles-20260731/vsftpd-evidence-gate-flat-015135/perf.data`。
  flat profile 的主要用户态热点是 `std::_Rb_tree_increment` 8.42%、`_int_malloc` 6.08%、
  `malloc` 5.85%、`_int_free` 5.09% 和 binarysub 红黑树插入 3.45%；
  `binarysub::resolve_variable` 约 0.84%，未出现 `__asan_*`。这表明重复 root 解析已不再是
  当前主热点，后续应针对 bound/类型集合的红黑树访问和分配开销单独优化，而不是继续扩大
  `onVariableNonVarBoundAdded()` 的入口判断。

本步没有改变 subtype 边、节点合并条件或事务回退结果；新增回调只处理 merge 后已经存在的
nested bound 中新出现的 evidence。

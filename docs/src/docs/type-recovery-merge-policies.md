# Type Recovery Merge Policies（类型恢复节点合并策略盘点）

本文档盘点 NotDec 类型恢复（MLsub / binarysub）中所有"合并节点"相关的机制：
求解器内置的必然合并，以及 NotDec 接入层的启发式策略。新增合并策略前先读本文档，
确认与现有机制不重复、不冲突。

## 命名约定

早期日志曾用编号速记；这些编号不是代码里的标识符，其中有的还与评测指标同名，读日志时很容易混淆。现在文档、日志和代码注释统一用下表的名字，旧编号已从日志正文移除。

| 名字 | 做什么 |
| --- | --- |
| `same-fn-struct-ptr`（同函数 struct-ptr 合并） | 同函数里两个 pointer-like、至少一侧有 struct 证据的变量合并（同层 var-var 钩子、界证据重扫的入口也是它） |
| `call-slot`（调用槽合并） | 实参↔形参、返回值↔函数返回的槽位合并 |
| `load-store-slot`（load/store 槽合并） | 同一指针上 load/store 目标合并 |
| `field-followup`（字段跟随合并） | 两个 owner 合并后，同 offset 的字段目标跟随合并 |
| `two-round-gate`（两轮闸门） | round one 只放行字段证据完全一致的 pair，其余推迟到 round two 复审 |
| `early-call-interface`（提前调用接口合并） | 在加 deferred call 边之前先合并调用槽 |
| `declared-alias`（声明式等价） | `arg_alias`/`ret_alias_of_arg`/`local_alias` 直接加双向边 |
| `reverse-edge-rule`（反向边规则合并） | 同层变量间已有反向边，求解器强制等价合并 |
| `same-level-varvar-hook`（同层 var-var 钩子） | 同层 var-var 边上的前端策略入口 |
| `bound-evidence-rescan`（界证据重扫） | 变量拿到新的 record/function 界后重扫邻居 |
| `host-merge-call`（前端直接合并） | 前端策略直接调用 `merge_variable_into` |

下表的“策略”列用现名；代码里的 `LocalSubtypeMergeMode`、`NOTDEC_MERGE_SCOPE_POLICIES`
等标识符保持原样。

## 求解器内置的必然合并（binarysub，无需策略）

这些是类型系统本体行为，不是"策略"，任何情况下都不应重复实现：

| 机制 | 位置 | 触发条件 |
| --- | --- | --- |
| `ReverseEdgeRule` 自动合并 | `binarysub-core.cpp` `constrain_impl` | 同 level 变量间出现直接反向子类型边 |
| size 检查 | `constrain_impl` | 两侧 size 必须严格相等，否则约束失败 |
| primitive 合并 | `constrain_impl` | 同名字 primitive 直接相等 |
| 结构约束 | `constrain_impl` | TMemObject 包含、TFunction 参数逆变/返回值协变 |
| extrude | `constrain_impl` | 跨 level 约束时复制类型到目标 level |

**关键性质（防止误判）**：变量之间的子类型边有传递传播，子类型环会在有限步内闭合成
完全图，从而必然出现直接反向边并触发 `ReverseEdgeRule` 合并。因此"数据流子类型环内的
节点最终相等"是求解器保证的，**不要**再设计"多跳子类型 SCC 合并"之类的策略。

**注意区分**：该性质只覆盖变量间的纯子类型边。record/function 结构 bound 嵌套引用
组成的"结构引用环"（canonicalize 折叠展开面对的环）不是子类型边，环上节点不一定
等价（record↔function 互引即为反例，见 `logs/20260808-01`）。两层"环"不要混淆。

## NotDec 接入层的合并策略（启发式 / 事务化）

按触发时机排序：

| 策略 | 位置 | 时机 | 条件 | 开关 |
| --- | --- | --- | --- | --- |
| `same-fn-struct-ptr`（同函数 struct-ptr 合并） | `shouldMergeSameFunctionStructPtrSubtype`（`MLsubGenerator.cpp:3158`），solver 处理 var-var 边时经 `SameLevelVarVarHook` 回调 | 约束求解中 | 同函数、同 level、双方 pointer-like、至少一边有 struct 证据（含 typed pointer）；`onVariableNonVarBoundAdded`（3341）在证据升级时重扫 | 默认开启 |
| `load-store-slot`（load/store 槽合并） | `applyStructPtrLoadStoreMergePolicy`（`MLsubGenerator.cpp:4976`） | 每个 SCC 求解后 | 同一指针变量上的 `ptr_load`/`ptr_store` 目标：同 level、size==PointerSize、双方 `hasStructPointerEvidence` | `--merge-struct-ptr-load-store`（默认开） |
| `call-slot`（调用槽合并） | `applyCallInterfaceMergePolicy`（`MLsubGenerator.cpp:4748`）→ `applyTransactionalCallSlotMergePolicy`（3951） | deferred call 约束应用后 + 每个 SCC post-summary | 事务化合并，`RequireStructEvidence`，冲突回退；决策写入 `CallSlotMergeDecisions.txt` | 默认开启 |
| `field-followup`（字段跟随合并） | `applyStructPtrFieldFollowupMergePolicy`（`MLsubGenerator.cpp:3655`） | post-summary | 合并后字段布局变化带来的新候选 | 默认开启 |
| level-0 SCC 合并 | `prepareSCC()`（`MLsubGenerator.cpp:5125`） | 约束生成前 | 所有 level-0 raw SCC 合成一个 generator（模块级单态作用域，保全局变量跨函数约束） | 固定行为 |
| `opaque_body` | `ConstraintsGenerator::run()`（`MLsubGenerator.cpp:5480` 附近） | 约束生成时 | summary override 标记的函数跳过函数体约束，只保留接口摘要 | summary JSON 字段 |

补充说明：

- var-var 子类型边（含 phi/select 生成的 `incoming <: result`）统一由"同函数
  struct-ptr subtype merge"检查：满足门槛的生成时直接合并；不满足门槛的靠求解器
  的环闭合（见上）兜住，其余保持单向子类型。
- 注意：默认的 struct-pointer 模式只合并 pointer-size 的变量；放宽到所有 pointer-like
  或所有同层同位宽局部值在实验中不更快（`logs/20260729-02`）。

## 按函数/按参数限定一条策略（`NOTDEC_MERGE_SCOPE_*`）

策略默认是全局的；当某个热点函数里的误合并把局部分析结果带偏时，用这一组开关把策略
限定到函数、或限定到某个参数位，而不是整体关掉（整体关掉通常两个轴都输，见
`microsub2/logs/20261021-186`）。

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_MERGE_SCOPE_FUNCS` | 逗号分隔的函数名 glob（`*`/`?`，两侧前导 `_` 忽略）；空＝功能关闭。stripped 输入下就是 IDA 的 `sub_XXXX` |
| `NOTDEC_MERGE_SCOPE_POLICIES` | 默认 `local-subtype`；可写 `reverse-edge`/`call-slot`/`field-followup`/`load-store`/`all` |
| `NOTDEC_MERGE_SCOPE_MODE` | `skip`（默认，拒绝该策略在匹配函数里的合并）/`strict`（只对 `local-subtype`、`reverse-edge` 有效：在这些函数里强制 round-one 证据闸门） |
| `NOTDEC_MERGE_SCOPE_SLOTS` | 只有 `_POLICIES` 含 `call-slot` 时生效：逗号分隔的 slot 模式（`argN`/`ret`，支持 glob，裸数字 `N`＝`argN`）。不设＝整函数；设了＝只关这些参数位上的 actual→formal 合并 |
| `NOTDEC_MERGE_SCOPE_DIAG` | 逐条打印被拒绝的合并（策略、函数、两侧变量 id 与值标签） |
| `NOTDEC_MERGE_SCOPE_DIAG_FIELDS` | 在上一行后面补两侧的原始字段标签（含 stride 与负偏移），用于区分"同一对象的两个视图"和"无关对象" |

语义：`skip` 走各策略原有的拒绝回退（`same-fn-struct-ptr`/`reverse-edge` 返回 false 后照常加
lower/upper bound，`call-slot` 是删候选），不是新造一条路径；`strict` 复用
`roundOneMergeAllowed` 的判据，只是把"是否启用"从全局 `NOTDEC_TWO_ROUND_MERGE` 换成"该函数
是否命中 scope"。

**用之前先确认拦得住**：`strict` 依赖两侧已有字段证据，而 `same-fn-struct-ptr` 的很多
合并发生在证据为空时（`%ldx`/栈重载这类值），这类合并任何字段证据闸门都看不到；判定
时先开 `NOTDEC_MERGE_SCOPE_DIAG_FIELDS=1` 看 `fields={<none>}` 的比例
（`microsub2/logs/20261023-195`）。

`NOTDEC_MERGE_VETO_VALUES`（分号分隔子串，配 `NOTDEC_MERGE_VETO_MODE=either|both`）是**实验
探针**：命中的同函数合并直接拒绝，用来问"哪一类值让合并变坏"。它不是配置项，别写进
默认配置或 benchmark 配置。

## 历史教训（设计新策略前必读）

- **求解后 merge 有真实成本**：`--merge-struct-ptr-load-store` 在 fortune 上
  bad_unions=0 但 wall 167s→528s、RSS 1.3→3.5GB（`logs/20260720-02`）。原因是边
  已经生成并传播，事后 merge 要递归改写已有 bound。优先考虑"生成时直接复用节点"。
- **无证据合并会卷进普通整数**：pure one-sided struct evidence 在 wasm32 上把普通
  i32 地址值卷进结构体链，跨函数 call-arg 是放大器（90s 超时、9.5GB，
  `logs/20260724-03`）。所以启发式策略保留 pointer-like / struct evidence 门槛。
- **递归冲突需要事务回退**：ffplay 的调用槽合并会在 bound 重写时递归触发新合并，
  外层无法阻止，需要 checkpoint 回退（`logs/20260728-01/02`）。
- **放宽合并不一定更快**：提前/放宽合并减少节点数，但 merge 改写 bounds 的成本可能
  抵消收益（`logs/20260729-02`，ffplay 全档位验证）。

## 验证口径

- sound 性：166/286 等无爆炸切片在合并策略下 out.ll / ValueTypes.txt 不应变化
  （sound 合并改变输出说明规则不成立）。
- 正确性：`--merge-eval-dir` 的 `bad_unions` 必须为 0，先于 fragmentation。
- 性能：memcached 286+get 切片、wasm32 fortune（i32 数据流回归）不可退化。

完整的带 DebugInfo 源码级 IR 评估流程、输出含义和最小化定位方法见
[Source-IR Merge Evaluation](source-ir-merge-evaluation.md)。做带
`--merge-eval-dir` 的策略修改前必须先读该文档。

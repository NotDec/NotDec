# Type Recovery Merge Policies（类型恢复节点合并策略盘点）

本文档盘点 NotDec 类型恢复（MLsub / binarysub）中所有"合并节点"相关的机制，
按语义保证强度分三层：求解器必然合并、IR 语义强相等、启发式策略。
新增合并策略前先读本文档，确认与现有机制不重复、不冲突。

## 层 0：求解器内置的必然合并（binarysub，无需策略）

这些是类型系统本体行为，不是"策略"，任何情况下都不应重复实现：

| 机制 | 位置 | 触发条件 |
| --- | --- | --- |
| `ReverseEdge` 自动合并 | `binarysub-core.cpp` `constrain_impl` | 同 level 变量间出现直接反向子类型边 |
| size 检查 | `constrain_impl` | 两侧 size 必须严格相等，否则约束失败 |
| primitive 合并 | `constrain_impl` | 同名字 primitive 直接相等 |
| 结构约束 | `constrain_impl` | TMemObject 包含、TFunction 参数逆变/返回值协变 |
| extrude | `constrain_impl` | 跨 level 约束时复制类型到目标 level |

**关键性质（防止误判）**：变量之间的子类型边有传递传播，子类型环会在有限步内闭合成
完全图，从而必然出现直接反向边并触发 `ReverseEdge` 合并。因此"数据流子类型环内的
节点最终相等"是求解器保证的，**不要**再设计"多跳子类型 SCC 合并"之类的策略。

**注意区分**：该性质只覆盖变量间的纯子类型边。record/function 结构 bound 嵌套引用
组成的"结构引用环"（canonicalize 折叠展开面对的环）不是子类型边，环上节点不一定
等价（record↔function 互引即为反例，见 `logs/20260808-01`）。两层"环"不要混淆。

## 层 1：IR 语义强相等的 copy（NotDec 接入层）

这些节点对在 C / LLVM 语义上必然相等，合并 sound，不需要证据。目前覆盖不全：

| 来源 | 现状 | 位置 |
| --- | --- | --- |
| `phi` | 只生成单向子类型边 `incoming <: result`，未专门合并 | `MLsubGenerator.cpp:8696` `handlePHINodes` |
| `select` | 只生成单向子类型边 `src <: dst`，未专门合并 | `MLsubGenerator.cpp:8848` `visitSelectInst` |
| load/store 同槽 | 仅在双方都有结构体指针证据时合并（保守） | 见下 |

LLVM IR 强制 phi/select 的 incoming 与 result 类型完全一致，因此它们不依赖
"图结构恰好成环"就必然相等。当前只在图结构恰好形成子类型环时被层 0 兜住合并；
不成环时保持单向子类型（sound 但保守，类型碎片化）。load/store 同槽的 C 语义
同样不区分 load/store 类型，见 `--merge-struct-ptr-load-store` 策略。

## 层 2：启发式策略（证据驱动 / 事务化）

按触发时机排序：

| 策略 | 位置 | 时机 | 条件 | 开关 |
| --- | --- | --- | --- | --- |
| 同函数 struct-ptr subtype merge | `shouldMergeSameFunctionStructPtrSubtype`（`MLsubGenerator.cpp:3158`），solver 处理 var-var 边时经 `PolicyReplaceBound` 回调 | 约束求解中 | 同函数、同 level、双方 pointer-like、至少一边有 struct 证据（含 typed pointer）；`onVariableNonVarBoundAdded`（3341）在证据升级时重扫 | 默认开启 |
| load/store struct-ptr merge | `applyStructPtrLoadStoreMergePolicy`（`MLsubGenerator.cpp:4976`） | 每个 SCC 求解后 | 同一指针变量上的 `ptr_load`/`ptr_store` 目标：同 level、size==PointerSize、双方 `hasStructPointerEvidence` | `--merge-struct-ptr-load-store`（默认开） |
| call arg/formal + return merge | `applyCallInterfaceMergePolicy`（`MLsubGenerator.cpp:4748`）→ `applyTransactionalCallSlotMergePolicy`（3951） | deferred call 约束应用后 + 每个 SCC post-summary | 事务化合并，`RequireStructEvidence`，冲突回退；决策写入 `CallSlotMergeDecisions.txt` | 默认开启 |
| struct field follow-up merge | `applyStructPtrFieldFollowupMergePolicy`（`MLsubGenerator.cpp:3655`） | post-summary | 合并后字段布局变化带来的新候选 | 默认开启 |
| level-0 SCC 合并 | `prepareSCC()`（`MLsubGenerator.cpp:5125`） | 约束生成前 | 所有 level-0 raw SCC 合成一个 generator（模块级单态作用域，保全局变量跨函数约束） | 固定行为 |
| `opaque_body` | `ConstraintsGenerator::run()`（`MLsubGenerator.cpp:5480` 附近） | 约束生成时 | summary override 标记的函数跳过函数体约束，只保留接口摘要 | summary JSON 字段 |

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

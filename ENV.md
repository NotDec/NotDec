# 环境变量（NotDec）

类型恢复（mlsub）与相关 pass 的开关。库内部（`external/binarysub`）的开关见 `external/binarysub/ENV.md`；microsub2/cons_gen 自己的开关见 `binarysub-private/ENV.md`。

布尔开关统一语义：未设 = 默认；`=0`/`=off`/`=false`/`=no` = 关；其它值（包括空值）= 开。

## 合并策略

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_LOCAL_SUBTYPE_MERGE_MODE` | 同函数 var-var 对怎么处理：`struct-pointer`（默认，有证据才合并）/`pointer`/`all-local`（全合并）/`none`（不合并） |
| `NOTDEC_IMPLICIT_SUBTYPE_ABLATION` | SCC0 内消融：`merge-local`（有数据流就合并）/`merge-or-cut`（合并不了就剪断调用边） |
| `NOTDEC_EARLY_CALL_INTERFACE_MERGE` | `=1` 先合并调用槽再加调用边（实验顺序） |
| `NOTDEC_TWO_ROUND_MERGE` | `=0` 关闭两轮合并（默认开） |
| `NOTDEC_TWO_ROUND_DIAG` | 打印两轮合并的拒绝/重检计数 |
| `NOTDEC_NO_REVERSE_EDGE_MERGE` | 拒绝所有 reverse_edge 自动合并（对照臂） |
| `NOTDEC_ALLOW_SHALLOW_MERGE_CONFLICT` | `=1` 浅层冲突时跳过该合并而不是回滚 |
| `NOTDEC_DISABLE_CALLSLOT_MERGE` | 关闭调用参数/返回槽的事务化合并 |

## 调用槽候选

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_SKIP_SLOT_MERGE_SLOTS` | 跳过整类槽：`ret`、`arg` |
| `NOTDEC_SKIP_SLOT_MERGE_TARGETS` | 跳过指定目标函数（逗号分隔）的调用槽候选 |
| `NOTDEC_SLOT_MERGE_EXPORT` | 把候选键 `目标函数|槽位|调用点` 导出到文件 |
| `NOTDEC_SLOT_MERGE_IMPORT` | 读入 `键<TAB>skip|apply` 覆盖候选集 |
| `NOTDEC_SLOT_MERGE_DIAG` | 打印事务化调用槽的逐条决定 |
| `NOTDEC_SLOT_MERGE_DIAG2` | 打印每个候选条目；`=eventlog`/`=both` 送事件流（见 `NOTDEC_EVENT_LOG`） |
| `NOTDEC_CALL_SLOT_DECISIONS` | 没有 `--work-dir` 也强制写 `CallSlotMergeDecisions.txt` |

## 栈区（frame）守卫

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_FRAME_NO_NONFRAME_MERGE` | 默认开；`=0` 允许栈区与非栈对象合并 |
| `NOTDEC_FRAME_NO_CROSS_FUNCTION_MERGE` | 默认开；`=0` 允许两个函数的栈区合并 |
| `NOTDEC_FRAME_NO_POLICY_MERGE` | `=1` 事务化策略合并中，一侧是栈区就整体回滚 |
| `NOTDEC_FRAME_NO_REVERSE_MERGE` | `=1` 求解器 reverse_edge 合并遇到栈区就拒绝（保留有向边） |
| `NOTDEC_FRAME_VETO_KNOWN_OBJECT_ONLY` | frame veto 只在另一侧是"可归属的非栈对象"时生效 |
| `NOTDEC_FRAME_MARK_INCLUDES_LVARS` | `=1` 栈区标记也覆盖 `%lvar` 值 |
| `NOTDEC_FRAME_VETO_DIAG` | 打印每次 frame veto 的判定与两侧取值 |
| `NOTDEC_FRAME_EDGE_TRACE` | 打印装到栈区节点上的每条前端约束；`=eventlog`/`=both` 送事件流 |
| `NOTDEC_FRAME_EDGE_BACKTRACE` | 给上一条加调用栈 |
| `NOTDEC_FRAME_REVERSE_MERGE_DIAG` | 打印 reverse_edge 上的栈区判定；`=eventlog`/`=both` 送事件流 |
| `NOTDEC_FRAME_REVERSE_MERGE_BACKTRACE` | 给上一条加调用栈 |

## 证据与布局（实验开关）

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_MEM_NO_STORE_TO_LOAD` | `=1` 关闭 store→load 流动（保留读写拆分） |
| `NOTDEC_MEM_PTR_SINGLE` | `=covariant|contravariant` 把指针两个半区退化成一个 Ptr |
| `NOTDEC_LAYOUT_OVERLAP_MODE` | 字段切片重叠的放宽程度（0–4，默认 4） |
| `NOTDEC_EXTRA_CONSTRAINTS` | 额外约束文件（与输入模块 SHA 绑定） |
| `NOTDEC_PNDIFF_POLICY_OVERRIDE` | PNDiff 策略文件 |
| `NOTDEC_IMPORT_SEMANTICS` | 按 import 函数名指定语义（分配器、字符串等） |
| `NOTDEC_ARG_ALIAS` / `NOTDEC_RET_ALIAS` | 按 `函数:位置` 指定实参/返回值的别名语义 |
| `NOTDEC_POINTER_ARGS` / `NOTDEC_POINTER_RET` | 按 `函数:位置` 声明某参数/返回值是指针 |

## 多态

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_POLYMORPHIC_FUNCTIONS` | `=1` 打开多态函数标记（内置名单） |
| `NOTDEC_POLY_FUNCS` | 多态函数策略文件 |
| `NOTDEC_POLY_NAME_EXCLUDE` | 从内置名单里排除的函数名（逗号分隔） |
| `NOTDEC_POLY_WRAPPER_DETECT` | `=0` 关闭通用 wrapper 识别 |
| `NOTDEC_POLY_MARKING_REPORT` | 识别报告输出路径；只识别、不做类型恢复 |
| `NOTDEC_POLY_MARK_ALLOCATOR_BOUNDARIES` | 标记分配器边界 |
| `NOTDEC_DEFAULT_POLY` | 默认当作多态的函数名单 |
| `NOTDEC_POLY_MONOMORPHIC` | 指定多态函数里要按单态处理的位置 |
| `NOTDEC_POLY_MONO_DIAG` | 打印单态化的判定过程 |

## 摘要与签名

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_SUMMARY_OVERRIDE` | 函数摘要覆盖文件 |
| `NOTDEC_SIGNATURE_OVERRIDE` | 函数签名覆盖文件 |

## 观测与日志

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_EVENT_LOG` | 统一事件流输出路径（8 列 TSV），配合 `NOTDEC_EVENT_FILTER` |
| `NOTDEC_DIAG` | 诊断分类列表，逗号分隔：`stats`/`graph`/`simplify`/`timing`/`field-origins`；`=1`/`=all` 全开，未设/`=0` 关（详见本节末说明） |
| `NOTDEC_MERGE_AUDIT` | 每次合并打印一行；`=eventlog`/`=both` 送事件流 |
| `NOTDEC_BINARYSUB_TRACE` | 把库/前端的事件写成整句文本，放进 work-dir 的 `binarysub-trace.log`（给人读，不是 TSV） |
| `NOTDEC_WATCH` / `NOTDEC_WATCH_SITES` | 按 vs# id 定点打印（定义在库侧；前端 hook/addsubtype 两个站点也受它控制），见 `external/binarysub/ENV.md` |
| `NOTDEC_BIGCLASS_DIAG` | 打印 upper 界 ≥1000 的合并类 |
| `NOTDEC_DEFERRED_DIAG` | 打印被推迟的调用子类型；`=<id>[,<id>]` 只看指定变量 |
| `NOTDEC_TRACE_VALUE` | 打印名字含该子串的值在求解后挂的界 |
| `NOTDEC_SIMPLIFY_STOP_AFTER_GROUPS` | 简化阶段处理 N 组后停下（调试） |
| `NOTDEC_CONVERT_PROGRESS` | 转换阶段打印进度 |
| `NOTDEC_WORKLIST_SNAPSHOT` | 打印 worklist 深度快照 |
| `NOTDEC_NON_IR_EDGE_TRACE` | 打印非 IR 访问产生的边 |
| `NOTDEC_BIDIR_DIAG` | 打印前若干对双向有向边的变量对 |
| `NOTDEC_PHI_WATCH` | 打印指定 phi 节点的合并/类型信息 |
| `NOTDEC_MALLOC_WRAPPER_DEBUG` | 打印分配器包装函数的识别过程 |
| `NOTDEC_FIELD_ORIGINS` | 打印字段级 origin 覆盖（需 `BINARYSUB_ENABLE_FIELD_ORIGINS`） |
| `NOTDEC_DUMP_TYPE_GRAPH` | 把类型图 dump 到指定路径 |
| `NOTDEC_DUMP_COMPACT_GRAPH` | 把 CompactType 图 dump 到指定路径 |
| `NOTDEC_FATNODE_DIAG` | 打印超大类型节点 |
| `NOTDEC_FATNODE_MIN` | 超大节点的 bound 数阈值 |
| `NOTDEC_FATNODE_FUNC` | 只在指定函数上做超大节点诊断 |
| `NOTDEC_FATGROW_DIAG` | 打印节点膨胀的来源 |

`NOTDEC_DIAG` 的分类：`stats`（按函数统计约束图规模）、`graph`（各阶段结束后的类型图规模）、`simplify`（简化/生成阶段的诊断输出，库侧慢路径也受它控制）、`timing`（约束求解阶段计时与计数）、`field-origins`（求解后还带 origin 的字段数，需 `BINARYSUB_ENABLE_FIELD_ORIGINS`）。取值大小写不敏感、可带空格；未列出的名字不生效；只换 gate，各类诊断的文本不变。

## 其他 pass

| 变量 | 作用 |
| --- | --- |
| `NOTDEC_DISABLE_MEMSET_STORE_FUSER` | `=1` 关闭 memset 与相邻 store 的合并 pass |
| `NOTDEC_KEEP_DEAD_STACK` | 保留死栈槽，不删 dead alloca |
| `NOTDEC_DANGEROUS_SCAN_THRESHOLD` | 危险类型模式扫描的分数阈值（默认 10000） |
| `BINARYSUB_FILTER_BULK_ORIGIN_FIELDS` | bulk-copy 字段过滤策略：`strict`/`any`（默认关） |
| `NOTDEC_EVM_HELPER_INLINE_DIAG` | EVM helper inline pass 的诊断 |
| `NOTDEC_EVM_POLY_ENTRIES` | EVM calldata 多态入口的策略 |
| `NOTDEC_EVM_POLY_STRICT` | `=1` 严格 let-polymorphism 判定 |

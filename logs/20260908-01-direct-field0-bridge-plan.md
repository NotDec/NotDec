同时改进文档和设计实现吧，但是具体做的任务还是交给terra xhigh的sub-agent

# 背景

`TMemObject` 的零偏移访问历史上有两种写法：对象根的 `directLoad`/`directStore`，以及
`fields["0"]` 中嵌套的 pointer object。此前曾尝试在约束求解器里用 `PointerViews` 和
`nestedBoundUsers` 把两种写法临时桥接起来，但这把表示差异推迟到了约束、upper-bound
索引和变量合并路径，增加了状态维护和回滚边界。

字段名 `"0"` 也可能表示普通 aggregate/EVM value 的第 0 个成员。项目决定不增加来源
标记，因此只能把“字段值明确是带 direct capability 的 `TMemObject`”作为零偏移指针
规约的前提；普通值字段和变量字段不参与该规约，相关生产路径必须绕过这个约定。

# 目标

- 在 SimpleType 构造和 hash-consing 前，将明确的 field-0 pointer capability 提升到根槽。
- 完全相同的 root/field-0 capability 只保留一份；不新增字段来源标记或 PointerViews。
- 同偏移异宽访问仍作为独立重叠证据，不在 SimpleType 阶段取最大宽度或强行合并目标。
- 保留现有 `constrain_impl` 的 load 协变、store 逆变、同宽 store→load 内源规则。
- 明确 CompactType/HType 后续表示阶段与 SimpleType 约束阶段的宽度处理差异。

# 技术判断

`AccessType::Size` 是一次访问的宽度，不是 `TMemObject` 的整体大小；`get_size(TMemObject)`
返回 `pointer_size`。因此 load8/load32 不会在外层节点大小检查中互相配对，SimpleType
的直接访问规则也只对同宽访问建立约束。CompactType 累积多个 `psize` 时使用 `max` 选择
压缩表示的代表宽度；HType 布局则把访问换算成 byte range，重叠范围组织成 union/panel，
并非简单丢弃较小访问。

# 实现记录

- `external/binarysub/src/binarysub-core.cpp:587`：新增匿名 helper
  `normalize_zero_offset_pointer_field`，由 `SimpleTypePool::getMemObject`（718-735 行）
  在排序、构造 key 和 hash-consing 前调用。它只检查一层 `fields["0"]` 的 `TMemObject`：父 root 槽
  为空时提升 child 的同极性 capability；父槽与 child 的 `(target, Size)` 完全相同时去重；
  目标或宽度冲突时保留 child，避免在 SimpleType 阶段丢失独立重叠证据。child 还有普通
  字段时，仅移除已提升的 capability，并通过同一 pool 重建剩余 child。
- 未修改 `constrain_impl`、`VariableState` 的 pointer 索引、snapshot 或 canonicalize
  逻辑；构造后节点已经采用统一 root 形式，旧的直接结构约束和 upper-bound store→load
  规则继续生效。
- `external/binarysub/src/binarysub-test.cpp:2797`、`src/binarysub-test-main.cpp:61`、
  `include/binarysub/binarysub-test.h:33`：新增
  `test_simple_type_zero_offset_normalization`，
  覆盖 load/store 提升、非零字段共存、普通值/变量 field-0 隔离、完全重复去重、异宽
  冲突保留，并验证 pointer factory 的 hash-cons identity。
- `external/binarysub/CLAUDE.md` 与 `external/binarysub/doc/二进制类型推理.md`：说明
  field-0 规约前提、aggregate/EVM 绕过要求、root 槽冲突策略，以及 SimpleType、
  CompactType、HType 对访问宽度的不同处理。

# 验证

- `cmake --build /sn640/NotDec/external/binarysub/build --target binarysub -j2`：成功。
- `./build/binarysub`（工作目录 `/sn640/NotDec/external/binarysub`）：全部既有测试和
  `test_simple_type_zero_offset_normalization` 通过，输出 `All tests passed!`。
- `git -C /sn640/NotDec/external/binarysub diff --check`：通过。
- `git -C /sn640/NotDec diff --check`：通过。

# 计划调整

原计划中的 `PointerViews`、`nestedBoundUsers` 扩展和大量 `constrain_impl` 桥接已撤销。
原因是它们解决的是构造表示不一致的后果；对明确 pointer object 在构造入口做规约后，
无需新增运行时视图或延迟索引。异宽/不同目标冲突仍保留 field-0，是因为当前 SimpleType
根槽无法同时编码两个独立 capability；若生产数据要求同一节点无条件消除这类冲突，需先
讨论新的多访问表示，不能用 `max` 静默改变 SimpleType 语义。

# Solidity revert / ABI return 基于 HType 重构计划

## 原始 prompt

规划一下接下来重构SolidityRevertPass以及ABIReturnPass(不太记得Pass名字对不对了，不对的话改一下)涉及了哪些主要的以前的文档，大致有哪几个阶段，要怎么做。（以前文档不对的地方可以改一下）当前的重构是打破solidity patterns，因为两个Pass的功能要迁移到基于类型推理的htypes结果，完成重构后再回头重新修复这两个Pass的问题。

## 涉及的旧文档

这次重构主要接着这几份文档走：

- `logs/20260602-05-evm-memory-object-type-recovery-pass-ordering-plan.md`
  - 这份是总顺序：memory object 先进入类型恢复，ABI return / revert / event 后面读类型结果。
  - 其中 `SolidityRevertPass` 可以早识别终点的说法已经不符合当前实现方向。现在这几个 pass 已经放到类型恢复后，后续也按 post-TR 语义 pass 处理。
- `logs/20260605-01-evm-type-recovery-htype-semantic-next-plan.md`
  - 这份是 HType 驱动语义恢复的总计划。
  - 需要保留的核心判断是：后续 pass 直接读 `MLsubRecovery::getResult` 的 C++ 结果，不加 HType 汇总层，不解析 `.htypes`。
- `logs/20260605-02-evm-abi-return-revert-htype-plan.md`
  - 这份是 `AbiReturnPass` 和 `SolidityRevertPass` 的主记录。
  - 已经记录了删除 `MemoryConsumer`、pass 后移、`inttoptr` 补约束、free memory pointer reload、pointer-sized access 折到 offset 0、`EventLogPass` 停止扫 memory marker 等实现。
  - 后续实现记录继续写回这份文档，不再为每个小步骤新开日志。

当前真实 pass 名字是：

- `AbiReturnPass`
- `SolidityRevertPass`
- `EventLogPass`

没有 `ABIReturnPass` 这个名字。

## 当前状态

现在的方向是有意打破旧的 `solidity_patterns` oracle：

- 旧 suite 很多检查是围绕 marker 数量和旧 memory write 匹配结果。
- 新方向要求 return / revert payload 只从 HType record field 读取。
- 如果 HType 没有给出结构体指针或字段，pass 不回退扫 `mstore` / `notdec_solidity_memory_write`，而是记录成类型恢复缺口。

所以短期内 `notdec.evm.solidity_patterns` 失败是预期状态。先完成 HType-only 重构，再回头修 oracle 和具体 pass 行为。

## 阶段 1：固定边界和失败样例

先把当前破坏范围说清楚，避免后面边修边回退：

- `AbiReturnPass`、`SolidityRevertPass`、`EventLogPass` 都放在类型恢复后。
- 三个 pass 都直接访问 `HTypeResult`，不解析 `.htypes`。
- 删除或停止依赖 `MemoryConsumer`、面向语义 pass 的 `MemoryWrite`、`notdec_solidity_memory_write` / `copy_write` payload 匹配。
- `solidity_patterns` 当前失败先按迁移中状态记录，不急着改成新 oracle。

判断标准：

- 代码里不新增新的 memory marker 路径。
- 失败用例能分清是 oracle 旧了，还是 HType 缺字段。

## 阶段 2：整理 HType buffer 读取代码

`AbiReturnPass`、`SolidityRevertPass`、`EventLogPass` 现在都有类似逻辑：

- 从 call 参数拿 `base` / `size`。
- 查 `HTypeResult::getDefaultValueType(base)`。
- 判断是不是结构体指针。
- 读取 record 的 offset field。
- 查不到就输出 debug 缺口。

这部分可以抽一个很薄的 helper，但 helper 只做读取，不做语义分类，也不扫 IR：

- 输入：`HTypeResult`、call、base value、size value、用途字符串。
- 输出：record pointee、字段列表、缺口原因。

判断标准：

- pass 内不再重复写一堆 record pointee 判断。
- helper 不引入新的“汇总层”，也不缓存 `.htypes` 文本。

## 阶段 3：重构 SolidityRevertPass

先做 revert，因为 `revert_error_string_01` 已经能从 HType 看到 offset 0 / 4 / 36 / 68 / 100。

处理顺序：

- 识别 `evm_revert(mem, base, size)`。
- `size == 0` 时标 empty revert，不需要 payload。
- 非空 revert 必须从 `base` 的 HType record 读 selector 和字段。
- 用 offset 0 selector 区分 Panic / Error(string) / custom error candidate。
- Error(string) 的 string head / length / data 只根据 record field 判断，缺字段就记录 HType gap。
- checked/bounds 关联只保留已有 guard 证据，不通过旧 store 扫描重建。

判断标准：

- `SolidityRevertPass.cpp` 不再通过 memory write marker 或 raw store 形状推 payload。
- `revert_error_string_01` 仍能标成 `error_string`。
- 字段缺失时有清楚 debug 输出，而不是静默不标或旧逻辑兜底。

## 阶段 4：重构 AbiReturnPass

再做 ABI return。return 的分类比 revert 少，但动态 return 容易触发旧 head/tail matcher，所以要更严格：

- 识别 `evm_return(mem, base, size)`。
- `size == 0` 时标 empty return。
- 非空 return 必须从 `base` 的 HType record fields 读 static word / dynamic head / tail。
- return tuple、bytes/string/array 只在 HType 字段足够时输出候选。
- returndata forward 这类不是本地 buffer 构造的情况单独保留直接 helper 识别，不用本地 memory write 兜底。

判断标准：

- `AbiReturnPass.cpp` 不再靠 memory write marker 或 mstore/copy loop 推 return payload。
- HType 缺字段的 case 进入缺口列表。
- 不为了让旧 oracle 通过而恢复旧 matcher。

## 阶段 5：修类型恢复缺口

如果阶段 3/4 卡住，优先修类型恢复，不在 pass 里补扫描：

- `base` 仍是普通整数：查 memory allocation rewrite / free memory pointer reload 是否漏合并。
- `base` 是 `Ptr<top,bottom>` 但没有 record fields：查 `inttoptr(base + const)`、store/load 约束是否落到同一节点。
- record field 是空结构：查 store value 约束是否被 lower 到 field。
- offset 0 缺失：查 pointer-sized direct access 是否应该折进 field 0。

这些问题修完后再回到 pass，不在语义 pass 内硬猜。

## 阶段 6：回头修 solidity_patterns

等两个 pass 都 HType-only 后，再修测试：

- 把旧的 marker 数量 oracle 改成新语义结果 oracle。
- 对明确是类型恢复缺口的样例，先标注原因，不把旧 matcher 结果当目标。
- 保留一两个反例测试，确保 HType 缺字段时不会回退扫 memory write。

判断标准：

- `notdec.type_recovery.evm.tr_level_2` 和相关 LLVM type recovery suite 继续通过。
- `notdec.evm.solidity_patterns` 的失败从“迁移中大量失败”收敛成少量真实缺口。
- fortune 同口径运行时间没有明显退化。

## 风险

- HType 现在还不能覆盖所有 Solidity ABI encoder 形状，尤其是动态 tail 和 returndata copy。
- 旧 `solidity_patterns` suite 会在中间阶段持续失败，不能用它作为阻塞当前迁移的唯一标准。
- 如果 helper 写厚了，容易变成新的 HType 汇总层；这次 helper 只能读已有 C++ 类型结果。

## 2026-06-09 实现记录：单字段指针按 offset 0 buffer 字段处理

本次按路线 A 继续收敛到 post-TR HType 路线。先确认
`src/Passes/PassManager.cpp` 里 EVM pipeline 已经在 `add_type_recovery_passes`
之后运行 `AbiReturnPass`、`SolidityRevertPass`、`CheckedBoundsPass` 和
`EventLogPass`，所以这次没有调整 pass 顺序。

实现改动：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:209`：
  新增 `hasHTypeBufferFieldAt` 和 `getHTypeBufferFieldStoreValues` 声明。
  这两个 helper 统一判断 record field 和透明 offset 0 单字段指针。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:85`：
  实现 `hasHTypeBufferFieldAt`。如果有 record，就按 record field 判断；
  如果没有 record 但 `HTypeBufferView::HasTransparentOffset0Field` 为真，只把
  offset 0 当作存在字段。
- `src/Passes/evm/solidity-patterns/HTypeStoreEvidence.cpp:101`：
  实现 `getHTypeBufferFieldStoreValues`，只在 HType buffer 认为该 offset 是字段时
  返回 store evidence，避免重新引入 raw store 兜底。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:57`：
  `getAbiReturnPayloadHType` 改为直接用 `hasHTypeBufferFieldAt`，删除原来对
  offset 0 store evidence 的本地特殊判断。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:60`：
  `getRevertBufferHType` 接受透明 offset 0 单字段指针。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:85`：
  `getRevertPayloadHType` 改走 `HTypeBufferView`，selector / panic code /
  error string 字段都通过 buffer field helper 判断。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:26`：
  event data 判断也接受透明 offset 0 单字段指针，避免把单字段 buffer 误报成
  missing HType。

复杂度评分：

- 实现效果：7/10。单字段指针现在能被 return / revert / event pass 按最小
  HType buffer 字段处理；但动态 tail 仍然依赖类型恢复产出更多 record fields。
- 理解成本：2/10。只加了两个薄 helper，没有新增汇总层。
- 维护成本：2/10。调用点集中在 `solidity-patterns/` 下，没有继续加长
  `SolidityPatterns.cpp`。

验证：

- `cmake --build ./build --target notdec-core -j4` 通过。

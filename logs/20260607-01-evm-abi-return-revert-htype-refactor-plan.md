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
- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.93s。

## 2026-06-09 实现记录：MemoryBufferFacts 收窄为 WrittenBases

上一轮删掉暂停的 memory write/read marker 物化后，`MemoryWrite` / `MemoryRead`
只剩 `MemoryBufferRewritePass` 内部使用。继续检查后确认：

- `MemoryRead` 没有消费者。
- `MemoryWrite` 的 payload、source offset、kind 都没有消费者。
- `MemoryWrite` 只用于判断某个 free-memory pointer base 是否有写入，从而决定是否补
  `calloc_unbounded()` allocation。

实现改动：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:15`：
  删除 `MemoryWriteKind`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:27`：
  删除 `MemoryWrite` 和 `MemoryRead`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:41`：
  `MemoryBufferFacts` 改为保存 `WrittenBases`，只记录有写入的 base。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:104`：
  删除 `getMemoryCopyWriteKind`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:358`：
  256-bit store / mstore8 / copy / mcopy 匹配到 free-memory base 时只写入
  `Facts.WrittenBases`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:366`：
  删除 memory read fact 收集。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:441`：
  allocation candidate 判断改为扫描 `WrittenBases`。

复杂度评分：

- 实现效果：5/10。继续去掉旧 marker 时代残留的数据面，保留 allocation rewrite 需要的信息。
- 理解成本：1/10。`MemoryBufferFacts` 更窄。
- 维护成本：1/10。后续不容易误以为 `MemoryBufferAnalysis` 仍提供 payload write/read facts。

验证：

- `cmake --build ./build --target notdec-core -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。

## 2026-06-09 实现记录：fixed-size tuple return helper 走 HType store evidence

`0679_19497465_c2187cbc73_f22fac5262f8` 的 `public_fees___0x51f` 还有一个 ABI
return 漏标。形状仍是 `return(mload(0x40), helper_ret - mload(0x40))`，但 helper
不是动态 string encoder，而是把 4 个 64-byte struct 连续写入 return buffer，
最后返回 `base + 256`。HType 里 return buffer 形参已经是 `struct_12*`，字段覆盖
0 / 32 / 64 / 128 / 192，子 helper 形参也有对应的 HType store evidence。

本次没有把 ABI return pass 放宽到任意跨函数写入。只接受 fixed-size tuple helper：
return base 必须是 free-memory pointer，size 必须是 helper return 减同一个
free-memory base，helper 的 buffer 形参必须有 record HType，helper 的所有 return
都必须是 `base + record_size`，并且 record 的每个非 padding 字段每 32 字节都要有
direct 或一层子 helper 的 HType store evidence。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:40`：
  `AbiReturnHelperPayload` 新增 fixed tuple evidence 标记。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:92`：
  新增 `helperReturnsBasePlusSize`，约束 fixed-size helper 的返回值必须是 buffer
  base 加 record 大小。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:110`：
  新增 `hasNestedHelperStoreEvidenceAt`，只沿 helper 内一层子 helper 传参检查 HType
  store evidence，不扫描 raw store。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:143`：
  新增 `hasFixedTupleHelperPayloadHType`，用 record range 和字段逐 32 字节验证 helper
  payload。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:290`：
  动态 helper 分支在 string/bytes 证据不满足时，再尝试 fixed tuple helper。

复杂度评分：

- 实现效果：6/10。`0679...` 的 ABI return 从 66/68 收敛到 68/68，真实
  `evm_return` 34/34 标记。
- 理解成本：4/10。比动态 string helper 多了 record range 和一层子 helper 字段覆盖
  检查，但仍限制在 ABI return pass 内。
- 维护成本：3/10。规则仍基于 HType/store evidence；后续如果类型恢复能把 helper
  写入汇总回 buffer record，这段可以收窄。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `0679_19497465_c2187cbc73_f22fac5262f8`：ABI return
  `expected_notdec.solidity.abi_return=68` /
  `actual_notdec.solidity.abi_return=68`，真实 `evm_return` 34/34 标记；该样例仍剩
  一个 helper-encoded revert 缺口。
- 临时 8 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d`、
  `0448_19495059_065877b669_4f138305be23`、
  `0657_19497309_a4d607684e_332c65dbca0f` 通过；`0340...` 和 `1991...` 仍只剩
  checked-bounds oracle 差异，`0679...` 剩上述 revert 缺口。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.89s。

## 2026-06-09 实现记录：helper-encoded revert 标成 encoded candidate

`0679_19497465_c2187cbc73_f22fac5262f8` 还剩一个 revert 漏标。它先在新分配的
free-memory buffer 写入 `Error(string)` selector，再把 `base + 4` 传给
`private__0x2d80_0x2d80` 编码动态 string payload，最后执行
`revert(mload(0x40), helper_ret - mload(0x40))`。当前 HType 能证明 selector store
和 helper 形参 offset 0 / 32 的 store evidence，但 selector 和 string payload
跨了两个 base，所以不能按普通 `error_string` 规则给出完整 string 长度。

本次只把这种 helper-encoded revert 标成 `encoded_candidate`。规则仍然很窄：revert
base 必须是 free-memory reload，size 必须是 helper return 减同一个 free-memory
base；helper 调用前同一基本块里必须有最近一次 `mstore(0x40, new_base)`；helper
buffer 实参必须是 `new_base + 4`；`new_base` offset 0 必须有 selector store
evidence；helper 形参 offset 0 / 32 必须有 HType 字段和 store evidence。

实现改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:81`：
  新增 `getLastFreeMemoryPointerStoreBefore`，只在同一基本块内找 helper call 前最近
  一次 free-memory pointer store。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:93`：
  新增 `getSiblingAddOffsetFromBase`，只处理 `%x + C1` 相对 `%x + C0` 的偏移。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:129`：
  新增 `hasHelperEncodedRevertPayload`，用 HType store evidence 证明 selector 和
  helper 动态 payload head/length。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:387`：
  `classifyRevertFromHType` 在普通 HType payload 判断前接受上述 helper 形状，并只
  标成 `encoded_candidate`。

复杂度评分：

- 实现效果：6/10。`0679...` 的 revert 从 74/75 收敛到 75/75，`encoded_candidate`
  从 0/1 收敛到 1/1。
- 理解成本：4/10。需要看一次 free-memory pointer store 和 sibling add 偏移，但规则
  只服务这个 helper-encoded revert 形状。
- 维护成本：3/10。仍基于 HType/store evidence；后续如果 HType 能直接把 `base + 4`
  payload 并回 revert buffer，这段可以收窄。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `0679_19497465_c2187cbc73_f22fac5262f8`：ABI return 68/68，
  revert 75/75，`encoded_candidate` 1/1；该样例仍剩 checked-bounds
  `memory_allocation_bounds` / `memory_allocation_pointer_bounds` oracle 差异。
- 临时 8 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d`、
  `0448_19495059_065877b669_4f138305be23`、
  `0657_19497309_a4d607684e_332c65dbca0f` 通过；`0340...`、`0679...`、`1991...`
  剩 checked-bounds oracle 差异。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.91s。

## 2026-06-09 实现记录：动态 return helper 支持形参 HType fallback

`0340_19494346_717db37a78_7324081e28fc` 里还有两个动态 ABI return 漏标。
public wrapper 的形状仍是
`return(mload(0x40), helper_ret - mload(0x40))`，helper 也是
`private__0x2cf3_0x2cf3`。区别是这两个 wrapper 里的 free-memory load 在 HType
里还是 `top:256`，但 helper 对应形参已经恢复成 `struct_0*`，并且形参上有 offset
0 / 32 的 HType store evidence。

本次没有放宽到任意 helper。规则仍然要求 return base 是 free-memory pointer，
size 是 helper return 减同一个 free-memory base，helper 实参也是同一个
free-memory base，并且后续 payload 判断继续基于 helper 形参上的 HType store
evidence。只是在 call-site 实参没有 record HType 时，允许回看 callee 形参自己的
HType。

实现改动：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:209`：
  新增 `getHTypeValueBufferView` 声明，用于直接按某个 LLVM value 自身的 HType
  构造 buffer view。
- `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp:75`：
  抽出 `makeHTypeBufferView`，避免 call-site view 和 value 自身 view 重复实现
  record / transparent offset 0 判断。
- `src/Passes/evm/solidity-patterns/HTypeBufferView.cpp:108`：
  实现 `getHTypeValueBufferView`，只读 `getExtValuePtr(Base, nullptr)` 的默认
  HType，不扫描 IR store。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:181`：
  `getDynamicReturnHelperPayloadHType` 在 helper 实参 view 没有 record 时，回看
  helper 形参 view；offset 0 / 32 仍然必须由 helper 形参上的 HType store evidence
  证明。

复杂度评分：

- 实现效果：7/10。`0340...` 的 ABI return 从 100/104 收敛到 104/104，两个漏标
  return 都补齐。
- 理解成本：2/10。新增的是 HType view 的小入口，没有新增 payload 汇总层。
- 维护成本：2/10。规则仍集中在 `AbiReturnPass.cpp` 的动态 helper 分支里。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `0340_19494346_717db37a78_7324081e28fc`：ABI return
  `expected_notdec.solidity.abi_return=104` /
  `actual_notdec.solidity.abi_return=104`，真实 `evm_return` 52/52 标记；该样例
  整体仍因为已知 checked-bounds `memory_allocation_bounds` oracle 差异失败。
- 临时 6 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d`、
  `0448_19495059_065877b669_4f138305be23`、
  `0657_19497309_a4d607684e_332c65dbca0f` 通过；`0340...` 只剩上述
  checked-bounds 差异。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.90s。

## 2026-06-09 实现记录：solidity_patterns runner 跟随 HType-only 路线

本次只改测试和审计脚本，不改 pass 行为。目标是让 `solidity_patterns`
不再通过旧 raw store 扫描给 revert 兜底，同时让 checked-bounds 审计跟随已经拆出的
`CheckedBoundsPass.cpp`。

实现改动：

- `test/run_evm_solidity_patterns_suite.py:136`：
  checked-bounds kind 到 rewrite marker 的 C++ 映射改为读取
  `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp`。
- `test/run_evm_solidity_patterns_suite.py:345`：
  `count_revert_kinds` 改为只统计 `notdec.solidity.revert` metadata，不再扫
  `evm_revert`、`inttoptr`、`store`、`returndatacopy` 的 IR 形状。
- `test/run_evm_solidity_patterns_suite.py:349`：
  panic code 没有 rewrite marker 时改读 `notdec.solidity_revert.panic_code`
  metadata，不再从 raw store 推断。
- `test/run_evm_solidity_patterns_suite.py:365`：
  compare 报告同时列出 expected / actual 两边的 key，避免实际多出的 key 被隐藏。
- `test/run_evm_solidity_patterns_suite.py:501` 和 `:610`：
  `returndata_bubble` 暂时从 revert kind oracle 跳过。当前 post-TR revert pass
  没有重新实现 returndata bubble，不能再让 runner 用旧 raw IR 逻辑补出结果。
- `scripts/audit-checked-bounds.py:33` 和 `:131`：
  checked-bounds audit 改为从 `CheckedBoundsPass.cpp` 读取 marker 映射。

当前观察：

- `checked_bounds_audit` 已经通过，输出 `cpp_marker_mapping: matched`。
- `revert_error_string_01` 用新 metadata oracle 能通过。
- 完整 `notdec.evm.solidity_patterns` 仍有大量失败，主要是 ABI return 现在不再按旧
  memory marker oracle 输出，以及 custom error / 真实合约样例里 HType 字段还不够。
  这些不通过恢复 raw store 扫描解决，后续继续按类型恢复缺口处理。

复杂度评分：

- 实现效果：5/10。测试侧不再掩盖 HType-only 路线；但完整 suite 还没收敛。
- 理解成本：1/10。删除旧脚本 matcher，oracle 更直接。
- 维护成本：1/10。checked-bounds 映射仍从 C++ 读，搬文件后不再误报 mismatch。

验证：

- `python3 -m py_compile scripts/audit-checked-bounds.py test/run_evm_solidity_patterns_suite.py`
  通过。
- `ctest --test-dir build -R '^notdec\.evm\.solidity_patterns$' --output-on-failure`
  失败，10 passed / 89 failed；其中 `checked_bounds_audit` 通过。
- 临时单样例运行 `revert_error_string_01` 通过，`checked_bounds_audit` 通过。

## 2026-06-09 实现记录：ABI return 接受 HType store evidence 单字段返回

`checked_bounds_arithmetic_01` 里 return buffer 是：

- `%0 = call ptr @calloc_unbounded()`
- `%evm.alloc.addr = ptrtoint ptr %0 to i256`
- `store i256 %evm.add, ptr %0`
- `evm_return(..., %evm.alloc.addr, 32)`

HType 已经把 `%evm.alloc.addr` 识别成 pointer，但不是 record pointer，也没有透明
offset 0 字段。这里可以按用户前面说的“单个指针也可以看作只有一个成员的结构体指针”
处理，但必须基于类型恢复导出的 store evidence，不能回退扫 raw IR。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:33`：
  `AbiReturnPayloadHType` 新增 `HasOffset0StoreEvidence`，只表示 HType store
  evidence 证明 return 前 offset 0 有 256-bit store。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:59`：
  `getAbiReturnPayloadHType` 接收 `TR.getEVMStoreEvidence()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:78`：
  当 base 是非 record pointer 且没有透明 offset 0 字段时，查询
  `getHTypeStoreValuesAtOffsetBefore(..., offset 0, Return)`。有 evidence 才继续，
  没有则仍作为 HType 缺口返回。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:110`：
  evidence-only 路径只允许 `size == 32` 的 `static_1_word`，更长 return 仍必须
  依赖 HType record / field，不扩大成泛化 `candidate`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:137`：
  `AbiReturnPass::run` 读取 `TR.getEVMStoreEvidence()` 并传给分类函数。

复杂度评分：

- 实现效果：6/10。单 word ABI return 不再因为 pointer 没形成 record 而漏标；
  动态 return 和多字段 return 仍然留给后续类型恢复字段补全。
- 理解成本：2/10。只给 `AbiReturnPass` 接入已有 HType store evidence，没有新增
  raw IR matcher。
- 维护成本：2/10。约束明确，只服务 32 字节单字段 return，误伤面较小。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.90s。
- 临时单样例运行 `checked_bounds_arithmetic_01` 通过，`checked_bounds_audit` 通过。
- 未跑 fortune，同本轮用户要求，先不处理 fortune 性能问题。

## 2026-06-09 实现记录：revert custom error 接受 HType selector evidence

`revert_custom_error_01/02/05/06` 的共同形状是：HType 已经把 `%evm.alloc.addr`
识别成 pointer，但没有形成 record；selector 的 `store` 已经进入
`TR.getEVMStoreEvidence()`，所以可以基于类型恢复 evidence 恢复 custom error。
这不是重新扫 raw IR。

实现改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:35`：
  `RevertPayloadHType` 新增 `SelectorFromStoreEvidenceOnly`，记录 selector 只来自
  HType store evidence，不来自 record field。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:152`：
  `getRevertBufferHType` 允许 non-record pointer 继续进入 payload evidence 判断。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:197`：
  如果 record / transparent field 都没有，但 offset 0 在 revert 前有 256-bit
  store evidence，则用它作为 selector evidence；offset 4 的 panic code 也只从
  evidence 取。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:284`：
  如果 selector 是 `Error(string)`，但只有 selector evidence、没有 head / length
  字段，则不降级成 custom error，避免误分类。

复杂度评分：

- 实现效果：7/10。custom error 短 payload 能恢复，仍然只依赖 HType 结果和
  HType store evidence。
- 理解成本：2/10。逻辑仍集中在 `SolidityRevertPass.cpp`，没有新增 shared 层。
- 维护成本：2/10。Error(string) 有保护，误判面较小。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时小集合 `revert_custom_error_01` 到 `revert_custom_error_06` 全部通过。
- 临时小集合 `revert_error_string_01` 到 `revert_error_string_04` 全部通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.91s。

## 2026-06-09 实现记录：revert metadata / marker helper 移到专门文件

本次只做纯搬移，避免 `src/Passes/evm/SolidityPatterns.cpp` 继续承载
`SolidityRevertPass` 专属逻辑。

实现改动：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:229`：
  删除 `insertPanicRewriteMarker`、`insertSelectorRewriteMarker`、
  `addRevertMatchMetadata` 的公共声明。这三个函数只被 `SolidityRevertPass`
  使用，不再作为共享接口暴露。
- `src/Passes/evm/SolidityPatterns.cpp:1362`：
  删除上述三个函数实现，大文件减少约 80 行。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:40`：
  在匿名 namespace 内补回 `insertPanicRewriteMarker`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:64`：
  在匿名 namespace 内补回 `insertSelectorRewriteMarker`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:93`：
  在匿名 namespace 内补回 `addRevertMatchMetadata`。

复杂度评分：

- 实现效果：6/10。没有改变 HType 分类能力，但减少了 `SolidityPatterns.cpp`
  里的 revert 专属代码。
- 理解成本：1/10。只是把只被一个 pass 使用的 helper 移到本 pass 文件。
- 维护成本：1/10。公共接口更小，后续改 revert metadata 不需要碰大文件。

验证：

- `cmake --build ./build --target notdec-core -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。

## 2026-06-09 实现记录：删除暂停的 memory write/read marker 死代码

本次只清理已经注释掉的旧 marker materialization，没有改变
`MemoryBufferRewritePass` 当前实际行为。`MemoryWrite` / `MemoryRead` facts 仍保留，
因为 allocation 判断还会用 `Facts.Writes` 判断 free-memory pointer load 是否真的被写入。

实现改动：

- `src/Passes/evm/MemoryBufferAnalysis.cpp:18`：
  删除不再递增的 `NumMemoryWrites` 和 `NumMemoryReads` 统计项。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:292`：
  删除已经注释掉的 `insertWriteMarker` 和 `insertReadMarker` 死代码。
  这段代码会物化 `notdec_solidity_memory_write`、
  `notdec_solidity_memory_copy_write`、`notdec_solidity_memory_read`，已经和当前
  HType-only 迁移方向不一致。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:533`：
  删除 `MemoryBufferRewritePass::run` 里已经注释掉的 write/read marker 调用循环。

复杂度评分：

- 实现效果：4/10。只是删死代码，但减少旧 marker 路径回流的可能性。
- 理解成本：1/10。没有新增逻辑。
- 维护成本：1/10。后续读 `MemoryBufferRewritePass` 时不会再看到暂停的旧 marker 分支。

验证：

- `cmake --build ./build --target notdec-core -j4` 通过。

## 2026-06-09 实现记录：checked-bounds 输出 helper 移到专门文件

本次只拆 `CheckedBoundsPass` 的输出侧逻辑，`matchCheckedBoundsGuard` 和它依赖的大量
guard matcher 仍留在 `SolidityPatterns.cpp`。这样不会改变 HType 后移路线，也避免一次性搬动
过多匹配代码。

实现改动：

- `include/notdec/Passes/evm/SolidityPatternUtils.h:231`：
  删除 `checkedBoundsOperandsDominateBranch`、`downgradeCheckedBoundsRewrite`、
  `addCheckedBoundsMetadata`、`rewriteCheckedBoundsGuard` 的公共声明。
- `src/Passes/evm/SolidityPatterns.cpp:5247`：
  删除 checked-bounds rewrite marker 名称、operand 可用性检查、skip 降级、
  metadata 插入和 CFG rewrite helper，大文件减少约 244 行。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp:18`：
  在匿名 namespace 内补回 `getRewriteKindCode`，只服务 checked-bounds skip marker。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp:32`：
  在本文件内补回 checked-bounds rewrite marker 名称映射。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp:110`：
  在本文件内补回 operand dominance / rematerialize 检查。
- `src/Passes/evm/solidity-patterns/CheckedBoundsPass.cpp:146`：
  在本文件内补回 skip 降级、metadata、rewrite marker 和 CFG rewrite helper。

复杂度评分：

- 实现效果：6/10。没有改变 checked-bounds 识别能力，但把 pass 输出侧代码移出大文件。
- 理解成本：2/10。`CheckedBoundsPass.cpp` 变长，但内容就是本 pass 的输出和 rewrite。
- 维护成本：2/10。公共接口更小，后续改 checked-bounds rewrite 不需要碰
  `SolidityPatterns.cpp`。

验证：

- `cmake --build ./build --target notdec-core -j4` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。

## 2026-06-09 实现记录：EventLogPass 接受单字段 store evidence

本次继续按路线 A 收敛。`AbiReturnPass` 和 `SolidityRevertPass` 已经支持
non-record pointer 通过 `TR.getEVMStoreEvidence()` 证明 offset 0 单字段 payload；
`EventLogPass` 也补同一条规则，但只限 32 字节 event data。

实现改动：

- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:26`：
  `hasEventDataHType` 增加 `ArrayRef<mlsub::EVMStoreEvidence>` 参数。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:36`：
  如果 data base 是 non-record pointer，且 `evm_logN` 的 data size 是 32，
  则用 `getHTypeStoreValuesAtOffsetBefore(..., offset 0, log)` 判断是否有
  HType store evidence。没有 evidence 时仍记录 HType 缺口，不扫 raw store。
- `src/Passes/evm/solidity-patterns/EventLogPass.cpp:69`：
  `EventLogPass::run` 读取 `TR.getEVMStoreEvidence()` 并传给 event data 判断。

复杂度评分：

- 实现效果：5/10。补齐 event data 的单字段 pointer case，和 ABI return / revert
  的 evidence 路线一致；多字段和动态 event data 仍依赖后续类型恢复字段。
- 理解成本：1/10。只是在本 pass 内接入已有 store evidence helper。
- 维护成本：1/10。没有新增 shared 层，也没有改 `SolidityPatterns.cpp`。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。
- 临时 event 小集合前三例仍失败，但 `notdec.solidity.event` 计数都匹配；
  失败来自 revert / payability / checked-bounds / ABI return 旧缺口。
- 完整 event 子集包含大真实合约样例，运行较慢，已停止；本轮不处理 fortune 性能。

## 2026-06-09 实现记录：checked-bounds 适配 post-TR native memory

`checked_bounds_array_01` 在 ABI return / revert 迁移后只剩 checked-bounds 失败：
ABI return、revert、panic code 都已匹配，两个 guard 被标成 skip。原因不是旧
memory marker 缺失，而是 `MemoryBufferRewritePass` 把部分 EVM memory object 改成了
native pointer 后，checked-bounds matcher 还只认旧的 `inttoptr` / free-memory
slot store 形状。

实现改动：

- `src/Passes/evm/SolidityPatterns.cpp:52`：
  `getIntToPtrAddress` 接受 native LLVM pointer，允许 `load i256, ptr %0`
  被后续 `matchEvmMemoryLoad` 当作 memory load。这里不恢复旧 marker，只让已经改写成
  pointer 的 memory object 进入现有 matcher。
- `src/Passes/evm/SolidityPatterns.cpp:4372`：
  新增 `findFinalizeAllocCall`，只匹配成功块里的
  `notdec_evm_finalize_alloc(base, size)`。
- `src/Passes/evm/SolidityPatterns.cpp:4963`：
  `matchMemoryAllocationPointerBounds` 在 success 条件为 true 的 `and` guard 下，
  允许 old pointer 是已改写 allocation base。
- `src/Passes/evm/SolidityPatterns.cpp:5057`：
  memory allocation pointer guard 除了旧 free-memory slot store，也接受
  `notdec_evm_finalize_alloc(oldPtr, size)` 作为成功路径证据。

复杂度评分：

- 实现效果：6/10。`checked_bounds_array_01` 从 2 个 skip 收敛为全匹配，
  checked-bounds 小集合全过。
- 理解成本：2/10。仍然改了 `SolidityPatterns.cpp` 的核心 matcher，因为这部分还没拆；
  但规则只覆盖 post-TR native memory 的两个具体形状。
- 维护成本：2/10。新增 helper 很小，后续拆 checked-bounds matcher 时可以一起搬到
  `CheckedBoundsPass.cpp` 或专门 matcher 文件。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `checked_bounds_array_01` 通过。
- 临时 checked-bounds 小集合 10 个样例全通过，skip 总数为 0。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.91s。

## 2026-06-09 实现记录：nonpayable reject 不计入 revert oracle

确认 `PayabilityGuardPass` 只消掉最外层 `msg.value == 0 ? body : revert(0,0)`
判断。failure block 没有和其它 unreachable 分支共享；它在 pre-TR 被 payability
CFG rewrite 消费后，post-TR `SolidityRevertPass` 看不到这类 `evm_revert(0,0)`。

本次按用户决策更新 oracle：nonpayable reject revert 不再计入
`notdec.solidity.revert` 和 `expected_revert_kinds.empty`。同时 payability guard 的
metadata 期望改成当前 pass 实际保留的三个点：`callvalue`、`condition`、`branch`。
failure block 的 payability metadata 会随 CFG rewrite 变成不可达细节，不再作为 oracle。

实现改动：

- `test/evm/solidity-patterns/manifest.json`：
  对有 `expected_nonpayable_functions` 的样例，把
  `expected_metadata_counts["notdec.solidity.payability_guard"]` 改成
  `3 * expected_nonpayable_functions`。
- `test/evm/solidity-patterns/manifest.json`：
  把 nonpayable reject 数从 `expected_revert_kinds.empty` 中扣掉，并按当前 runner
  语义让 `expected_metadata_counts["notdec.solidity.revert"]` 等于除
  `returndata_bubble` 外的 revert kind 总和。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- 临时 15 样例集合中，`0014_proxy_like` 和 `0011_multi_public` 从失败变为通过。
- 同一集合剩余失败为其它缺口：`0002_delegatecall_no_nonpayable` 少 ABI return 和
  encoded revert，`0441...` 少 encoded revert，`0448...` 少 ABI return。

## 2026-06-09 实现记录：ABI return 常量 base 走 HType store evidence

`0002_delegatecall_no_nonpayable` 里有 `evm_return(..., 0, 32)` 形状。HType store
evidence 已经能证明 offset 0 有对应 store，但 `base == 0` 是常量，没有 pointer
HType，原来在 `getAbiReturnBufferHType` 里会先按 missing HType 丢掉，导致单 word
return 漏标。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:48`：
  `getAbiReturnBufferHType` 对常量 base 返回当前 `HTypeBufferView`，让后面的
  `getHTypeStoreValuesAtOffsetBefore(..., offset 0, Return)` 继续用 HType store
  evidence 判断。没有 evidence 时仍然返回 missing payload，不扫 raw store。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过。
- 临时单样例 `0002_delegatecall_no_nonpayable` 的 ABI return 实际数量从 2 变为 4；
  剩余失败只剩 returndata forward / encoded revert 分类问题。

## 2026-06-09 实现记录：revert returndata forward 按 Solidity Yul 形状识别

对照 Solidity 源码后确认，外部调用失败转发下游 revert data 的常见形状是：
`returndatacopy(pos, 0, returndatasize())` 后接 `revert(pos, returndatasize())`。
这类 payload 不是本地 ABI encoder 构造，不应该走 HType payload 字段，也不再叫
`returndata_bubble`。

实现改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:41`：
  新增 `isReturndataForwardCopy`、`blockHasReturndataForwardCopyBefore` 和
  `hasSolidityReturndataForwardShape`。只接受 copy 目标和 revert base 是同一个
  SSA value，copy offset 为 0，copy length / revert length 都来自
  `evm_returndatasize()`。允许 copy 在同块 revert 前，或在 revert 块唯一前驱里。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:266`：
  `classifyRevertFromHType` 在 HType payload 判断前把上述形状标成
  `returndata_forward`。这条路径不读取 HType store evidence，也不恢复 raw store
  payload 扫描。
- `test/run_evm_solidity_patterns_suite.py:503`：
  runner 不再跳过这类 revert kind，直接按 metadata 比较。
- `test/evm/solidity-patterns/manifest.json`：
  把旧 `returndata_bubble` oracle 改成 `returndata_forward`，并删除
  `expected_returndata_bubbles`。`0002_delegatecall_no_nonpayable` 和
  `0441_19494998_776c03cc9d_8117f350cb9d` 的旧 `encoded_candidate` 期望改为
  `returndata_forward`。

复杂度评分：

- 实现效果：7/10。delegatecall / external call failure 的 forwarding revert
  不再被当成 encoded payload 缺口；`0002` 和 `0441` 收敛。
- 理解成本：2/10。规则直接对应 Solidity Yul 形状，且集中在
  `SolidityRevertPass.cpp`。
- 维护成本：2/10。没有继续加长 `SolidityPatterns.cpp`，oracle 也不再保留旧
  bubble 名字。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.91s。
- 临时 4 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d` 通过；`0448...` 仍失败，剩余差异是
  已知动态 string ABI return 缺口。
- 临时完整 solidity-patterns manifest：37 passed / 62 failed。全部
  `returndata_forward` compare 都对齐；剩余失败仍主要是 ABI return 或旧
  encoded candidate 缺口。

## 2026-06-09 实现记录：动态 string return helper 走 HType store evidence

`0448_19495059_065877b669_4f138305be23` 的 `name()` / `symbol()` 返回值形状是：
public 函数先把旧 `mload(0x40)` 传给跨函数 helper，helper 写入 ABI string
head / length / data，并返回 end pointer；public 函数再执行
`return(mload(0x40), helper_ret - mload(0x40))`。类型恢复里 helper 的 buffer
参数已有 `struct_3*`，helper 参数上的 offset 0 / 32 store evidence 也存在；
但 helper 返回值仍是 `top:256`，return 前重新加载的 free-memory base 也没有直接
带上同一个 HType。

本次没有在 ABI return pass 里恢复任意跨函数 store，也没有改类型恢复核心。只补
Solidity 动态 string helper 的窄形状：return base 必须是 `mload(0x40)`，size
必须是 `helper_ret - mload(0x40)`，helper 的某个实参必须和 return base 是同一个
free-memory base，且该实参的 HType buffer 视图和 helper 形参上的 HType store
evidence 能证明 offset 0 / 32 都被写过。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:38`：
  新增本地 `AbiReturnHelperPayload`，只记录 helper evidence 是否证明动态 head
  和 length。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:100`：
  新增 `getDynamicReturnHelperPayloadHType`，匹配
  `return(mload(0x40), helper_ret - mload(0x40))`，并用
  `getHTypeBufferView` / `getHTypeStoreValuesAtOffset` 检查 helper buffer 参数。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:162`：
  `classifyAbiReturnFromHType` 在普通 return base HType 判断前，把上述 helper
  evidence 标成 `dynamic_candidate`。

复杂度评分：

- 实现效果：7/10。`0448...` 的两个动态 string ABI return 补齐，focused suite
  从 3/4 变成 4/4。
- 理解成本：3/10。规则比普通 HType buffer 判断多看了一层 helper call，但仍限制在
  Solidity ABI return pass 内。
- 维护成本：3/10。后续如果类型恢复能把 helper end pointer / free-memory reload
  直接连回 buffer，这段可以收窄或删除。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时 4 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d`、
  `0448_19495059_065877b669_4f138305be23` 全部通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.91s。

## 2026-06-09 实现记录：动态 return helper 支持一层 tail 委托

`0657_19497309_a4d607684e_332c65dbca0f` 还有一个动态 ABI return 漏标：
`public__0x3c14f4d5_0x4ef` 调用 `private__0x2f3f_0x2f3f` 后执行
`return(mload(0x40), helper_ret - mload(0x40))`。这个 helper 自己只写
`base + 0 = 32`，然后把 `base + 32` 传给 `private__0x2efb_0x2efb` 写动态 tail
的 length / data，最后返回 inner helper 的 end pointer。

这仍然不是任意跨函数 store 汇总。本次只接受一层委托：outer helper 的 ABI buffer
参数必须有 HType buffer 字段，outer helper 写 offset 0；它把 `base + 32` 传给
inner helper，inner helper 对对应形参 offset 0 有 HType store evidence，并且
outer helper 的返回值依赖 inner helper 的返回值。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:45`：
  新增本地 `hasStoreEvidenceAt`，封装对 HType store evidence 的 offset 查询。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:53`：
  新增 `hasDelegatedStoreEvidenceAt`，只识别一层 `base + offset` 传参，并要求
  outer helper 返回 inner helper 的结果。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:185`：
  `getDynamicReturnHelperPayloadHType` 的 dynamic length 证据除了直接 offset 32
  store，也接受上述一层 tail 委托证据。

复杂度评分：

- 实现效果：6/10。`0657...` 从 ABI return 少 2 个 metadata 收敛为通过。
- 理解成本：3/10。比上一轮多了一层 helper 关系，但仍限制在一个明确 Solidity ABI
  helper 形状里。
- 维护成本：3/10。后续如果类型恢复能把 `base + 32` 的 delegated store 汇总到
  outer buffer 字段，这段可以删除。

验证：

- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `0657_19497309_a4d607684e_332c65dbca0f` 通过。
- 临时 4 样例集合中，`0011_multi_public`、`0002_delegatecall_no_nonpayable`、
  `0441_19494998_776c03cc9d_8117f350cb9d`、
  `0448_19495059_065877b669_4f138305be23` 全部通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。

## 2026-06-09 实现记录：固定单词 return helper 支持 HType store evidence

`2001_19510128_72aa4f70b3_e9ee7c9c0e79` 里还有一种固定 32 字节 ABI return
漏标：public 函数执行 `return(mload(0x40), helper_ret - mload(0x40))`，helper
只把一个值编码到 `base + 0`，然后返回 `base + 32`。类型恢复没有把 public 侧的
free-memory reload 直接连到 buffer record，但 helper 形参和它调用的下一层 helper
已有 HType pointer 和 offset 0 的 store evidence。

本次只补这个窄形状：helper 必须返回 `base + 32`，offset 0 必须能由 helper 形参的
HType 字段加直接 store evidence 证明，或者由一层 nested helper 的 store evidence
证明。没有恢复任意 helper 的 store 集合，也没有放宽到非 HType 证据。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:181`：
  新增 `hasFixedWordHelperPayloadHType`，检查 helper 返回 `base + 32`，并用
  offset 0 的 HType 字段和 store evidence 判断单词 payload。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:290`：
  `getDynamicReturnHelperPayloadHType` 在继续检查动态 string / fixed tuple 前，先接受
  上述固定单词 helper，并标成 fixed ABI return candidate。

复杂度评分：

- 实现效果：6/10。`2001...` 的 ABI return 从 70/72 收敛到 72/72。
- 理解成本：2/10。规则只多覆盖 `base + 32` 单词 helper，仍在 ABI return pass 内。
- 维护成本：2/10。后续如果 HType 能把该 helper 直接恢复成完整 buffer record，这段可以删除。

验证：

- `git diff --check` 通过。
- `cmake --build ./build --target notdec -j4` 通过，ninja 报告 no work to do。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.89s。
- 临时单样例 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 仍整体失败，但
  `notdec.solidity.abi_return` 已对齐为 72/72；剩余差异是已知的 2 个
  `encoded_candidate` revert 缺口和 checked-bounds skip。

## 2026-06-09 实现记录：raw bytes payload revert 支持 HType 证据

`2001_19510128_72aa4f70b3_e9ee7c9c0e79` 剩余两个 revert 漏标不是
`Error(string)`，也不是直接 `revert(0, returndatasize())` forwarding。它们的
形状是 Solidity bytes buffer 的 raw payload revert：长度在 `base + 0`，payload
从 `base + 32` 开始，最后执行 `revert(base + 32, mload(base))`。其中一个 buffer
来自 returndata copy，另一个是跨 helper 传入的 bytes/string buffer。

本次只识别这个窄形状：revert size 必须是 EVM memory load，revert base 必须是同一
header base 的 `+32`；header base 要有 HType offset 0 字段，或者 PHI incoming
值有 HType offset 0 字段和 store evidence。匹配后标成 `encoded_candidate`，不当作
`returndata_forward` 或 `error_string`。

实现改动：

- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:191`：
  新增 `hasRawBytesHeaderEvidence`，检查 raw bytes header 的 HType 字段和 store
  evidence；对 PHI 只接受有 HType/store evidence 的非常量 incoming。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:223`：
  新增 `hasRawBytesPayloadRevert`，匹配 `revert(base + 32, mload(base))`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:443`：
  `classifyRevertFromHType` 在普通 payload HType 分类前，把上述 raw bytes payload
  标成 `encoded_candidate`。

复杂度评分：

- 实现效果：6/10。`2001...` 的 Solidity revert 从 210/212 收敛到 212/212。
- 理解成本：3/10。多看了 `mload(base)` 和 `base + 32` 的关系，但规则仍很直接。
- 维护成本：3/10。后续如果类型恢复能把 PHI 后的 bytes buffer 保留为明确 record，
  PHI incoming fallback 可以删掉。

验证：

- `git diff --check` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.92s。
- 临时单样例 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 仍整体失败，但
  `notdec.solidity.revert` 已对齐为 212/212，`encoded_candidate` 已对齐为 2/2，
  没有未标记 `evm_revert`；剩余失败是 checked-bounds skip。

## 2026-06-09 实现记录：本函数内动态 return buffer 支持 HType 证据

`0450_19495071_6b6c9447e0_6344565f4b31` 的 `tokenName()` / `tokenSymbol()`
不是跨函数 encoder helper。public 函数自己分配 return buffer，写入 ABI dynamic
head `32`，从源 bytes/string buffer 读 length，copy payload，最后执行
`return(mload(0x40), end - mload(0x40))`。最终 return base 是 free-memory reload，
而写入证据在同函数里的 allocation base 上；两者没有同一个 SSA value。

本次只补这个窄形状：return base 必须是 free-memory load，size 必须是
`end - free-memory-load`；同函数里必须存在一个 free-memory allocation base，`end`
依赖它，且该 base 的 HType 有 offset 0 / 32 字段，offset 0 store evidence 必须是
常量 32，offset 32 必须有 HType store evidence。没有恢复通用 copy loop，也不扫描
非 HType 证据。

实现改动：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:194`：
  新增 `hasConstantStoreEvidenceAt`，用于判断 offset 0 的 ABI dynamic head 是否写入
  常量 32。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:205`：
  新增 `getInlineDynamicReturnPayloadHType`，匹配本函数内 allocation base 与
  `return(mload(0x40), end - mload(0x40))` 的关系，并要求 HType/store evidence
  证明 dynamic head 和 length。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:369`：
  `classifyAbiReturnFromHType` 在跨函数 helper 失败后，尝试上述本函数内动态
  return buffer 规则。

复杂度评分：

- 实现效果：7/10。`0450...` 和同类 4-count 动态 return 小样例收敛为通过。
- 理解成本：3/10。规则多看了同函数 allocation base，但仍限制在 ABI return pass 内。
- 维护成本：3/10。后续如果类型恢复能把 free-memory reload 直接连回 allocation
  record，这段可以收窄或删除。

验证：

- `git diff --check` 通过。
- `cmake --build ./build --target notdec -j4` 通过。
- 临时单样例 `0450_19495071_6b6c9447e0_6344565f4b31` 通过。
- 临时 5 样例集合
  `0450...`、`0726...`、`0735...`、`0038...`、`0114...` 全部通过。
- `ctest --test-dir build -R notdec.type_recovery.evm.tr_level_2 --output-on-failure`
  通过，用时 0.90s。

## 2026-06-09 当前技术决策点：tuple + storage bytes/string tail

继续扫描剩余 ABI return 缺口时，`0146_19493376_503a732d50_138a4d77d617`
只剩一个 `public_transactions_uint256__0x482` ABI return 漏标。该 return 不是前面已支持
的单动态 buffer，也不是跨函数 helper。它是固定 tuple 里带一个 storage bytes/string
tail：

- allocation base 的 HType 是 `struct_41*`，record 里有多个 32-byte slot，最后一个
  字段是 `top:256[]`。
- tuple head / static fields 有 HType store evidence。
- dynamic head slot 写入 tail length 的 offset，tail length 也有 store evidence。
- long storage bytes 分支的 payload store 写在循环 PHI `%_0x551_0x0` 上；该 PHI 的
  初始 incoming 是 `base + 160`，后续循环 incoming 是 `phi + 32`。

尝试过在 `AbiReturnPass.cpp` 本地加窄规则：识别单个 array 字段、head 指向 length、
tail base 或 tail PHI 有 store evidence。但 focused `0146...` 仍不命中。继续在 pass
里补会变成跨 PHI/循环的 tail store 归因，而不是简单消费 HType evidence。

这里需要先做技术决策：

- 路线 A：在 HType/store evidence 层把 PHI/循环 tail store 汇总到外层 buffer 字段，
  ABI return pass 继续只消费证据。
- 路线 B：在 `AbiReturnPass.cpp` 本地追踪 PHI/循环 tail store，短期能补样例，但会把
  ABI return pass 变成局部数据流分析。
- 路线 C：暂时跳过 `0146...`，继续处理其它更简单的 ABI return/revert 缺口。

当前倾向路线 A。它更符合“必须基于 HType store evidence”的路线，也避免
`AbiReturnPass.cpp` 继续膨胀成通用 ABI 编码分析。

# Memory object / buffer pass 计划

## 原始 prompt

在logs/里面模创建一个文件夹专门处理这个memory object 的相关pass，然后里面先放一个PLAN.md里面规划一下如何实现。里面先通过重新读solidity源码，总结相关部分的编译器的代码逻辑，然后后面再写具体的规划，看看solidity编译器源码对当前的规划有没有额外的启发和改进。


阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

实现阶段流程：
1. 选择一块功能实现，实现前在本次的log中再回顾生成相关的指令的solidity源码那边的相关逻辑。

改进阶段流程：实现一部分功能或者完成一轮可验证的改进
1. 找到存在问题的测试用例，并尽量选择出现频率多的问题。
2. 对每个改进点先对照 Solidity codegen 或真实 IR 形状，确认这是compiler guard，不是业务分支。
3. 增加或更新测试，跑验证：至少一个 Solidity-generated case 或真实 IR oracle
4. 每轮结束前，思考下一轮值得做或者改进的事情的候选列表，并按照值得程度排序。

完成条件：
- 完整实现相关功能
- 基于test/evm/solidity-patterns/manifest.json上测试用例增加oracle并不断改进代码，直到没有明显的问题。
- 在/sn640/NotDecChainExp/evm2llvm_apehex_pilot 上选固定的100个用例，不断改进效果，直到没有明显的问题

补充要求：
- 当前计划的目标必须是 IR rewrite，不是 memory metadata 标注。
- metadata 只能用于调试、统计和 oracle，不能作为 pass 之间的主要接口，也不能作为完成标准。
- MemoryBufferAnalysis 产出的东西必须落到 IR 里的 marker / semantic call，并且后续 ABI return、revert、event、external call 等 pass 必须逐步读取这些 marker / semantic call。
- 只新增 metadata，不算 memory object pass 的实质实现。
- 只插入 marker，但没有任何后续 pass 读取它，只能算 rewrite surface 准备，不能算完整闭环。
- 完整闭环的判断标准是：memory pass 写出稳定 IR 事实，消费者 pass 读取这些事实，并产出 ABI return / revert / event / external call 级别的语义改写。

## 背景

旧 `MemoryObjectPass` 已经删掉。原因不是 memory 语义不重要，而是旧实现太粗，只适合标候选，不适合继续扩展。现在要重新做的是更明确的 memory allocation / buffer analysis，并且结果必须落到 IR rewrite 上，给 ABI return、ABI revert encoding、event log、external call 和 ABI decode 复用。只打 metadata 不算完成，插入没人读取的 marker 也只算准备，不算真正完成。

这里的目标是 rewrite-first：每个可交付步骤都要产生或消费明确的 IR rewrite surface。第一阶段不要求立刻删除原始 `mstore/mload/copy`，可以先插入稳定的 semantic call / marker；但 marker 只是中间接口，不是最终结果。后续 pass 必须读取这些 marker，并把 ABI return、revert、event、external call 等低层 memory 用法改写成更高层的语义 call，或者在确认安全后隐藏 / 删除对应的低层指令。只插 marker、不被消费者读取，只能算准备；消费者仍然完全重新扫描原始 `mstore/revert/return/log/call` 来猜语义，也不能算完成。

本计划的主线是：

- `MemoryAllocation` 说明 buffer 从哪里来、大小怎么定。
- `MemoryWrite` 说明哪些值写进了 buffer。
- `MemoryConsumer` 单独建模，说明 buffer 最后被谁消费。
- rewrite marker / semantic call 是 pass 之间的接口，后续 pass 必须读取它们。
- metadata 只用于 debug、统计和 oracle，不能作为主要接口。

如果某个识别结果暂时还不能落到 semantic call / marker，或者没有任何后续 pass 读取它，那它只能算候选分析或 rewrite surface 准备，不能算 memory object pass 的完成项。

其中 `MemoryConsumer` 不能只是 `MemoryAllocation` 或 `MemoryWrite` 的附带字段。return、revert、event log、external call input/output、ABI decode read 都是不同消费点，后续 pass 要靠这些消费点决定怎么改写 IR。

本计划只规划主 NotDec 的 EVM Solidity memory 相关 pass，不动 bin2llvm / wasm2llvm / llvm2c。

## Solidity 源码形状

### 传统 codegen：`CompilerUtils`

源码：`/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp`

关键点：

- `CompilerUtils::freeMemoryPointer = 64`，`zeroPointer = 96`，`generalPurposeMemoryStart = 128`。
- `initialiseFreeMemoryPointer()` 初始化 `mstore(0x40, 0x80 + reservedMemory)`。
- `fetchFreeMemoryPointer()` 生成 `mload(0x40)`。
- `storeFreeMemoryPointer()` 生成 `mstore(0x40, newPtr)`。
- `allocateMemory()` / `allocateMemory(size)` 都是先读 free pointer，再加 size，再写回 `0x40`。
- `revertWithStringData()` / `revertWithError()` 先取 free pointer，在 buffer 开头写 selector，再从 `+4` 开始 ABI encode，最后用 `toSizeAfterFreeMemoryPointer()` 算 `revert(start, size)`。
- `returnDataToArray()` 对 returndata bytes 的形状是：`v := mload(0x40)`，写回 rounded end，`mstore(v, returndatasize())`，`returndatacopy(add(v, 0x20), 0, returndatasize())`。
- `encodeToMemory()` 的 ABI encoder 先以 `mem_start` 为 base，静态 head 写在 `base + head offset`，动态数据通过 tail 指针追加。

结论：传统 codegen 里 free pointer、return/revert buffer、ABI head/tail 都围绕同一个 base 指针展开。NotDec 不能只看 `mload(0x40)`，必须把 base、size、写入 offset 和最终 consumer 绑定起来。

### Yul codegen：allocation helper

源码：`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`

关键点：

- `allocateUnboundedFunction()` 生成 `allocate_unbounded()`，只返回 `mload(0x40)`，不更新 free pointer。
- `finalizeAllocationFunction()` 生成 `finalize_allocation(memPtr, size)`：
  - `newFreePtr := add(memPtr, roundUp(size))`
  - overflow / wrap 检查失败走 `panic(ResourceError)`，也就是 Panic `0x41`
  - 成功后 `mstore(0x40, newFreePtr)`
- `allocationFunction()` 组合 `allocate_unbounded()` + `finalize_allocation(memPtr, size)`。
- `allocateMemoryArrayFunction()` 先分配 array size，动态数组会 `mstore(memPtr, length)`。
- `allocateAndInitializeMemoryArrayFunction()` 分配后对 data 区域清零，简单类型用 `calldatacopy(dataStart, calldatasize(), size)`。
- `revertReasonIfDebugBody()` 使用 `let start := allocate`，按固定 ABI 布局写 Error(string)，然后 `revert(start, overallLength)`。
- `forwardingRevertFunction()` 失败转发时 `pos := allocate_unbounded()`，`returndatacopy(pos, 0, returndatasize())`，`revert(pos, returndatasize())`。

结论：新 pass 要把 `allocate_unbounded` / `finalize_allocation` 看成一组，而不是假设分配点一定马上写回 `0x40`。`finalize_allocation` 的 guard 已经和 checked-bounds 里的 Panic `0x41` 有交集，memory pass 要复用这类形状，不要另写一套猜测。

### ABI 函数

源码：`/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp`

关键点：

- `tupleEncoder()` 生成 `abi_encode_tuple_*`：输入 `headStart`，先 `tail := add(headStart, headSize)`，静态元素写 head，动态元素先写 `sub(tail, headStart)`，再递归编码 tail。
- 动态数组 / bytes decode 到 memory 时，会通过 `allocationFunction()` 分配，动态数组写 `mstore(array, length)`，数据从 `array + 0x20` 开始。
- ABI decode 中有大量 bounds check 和 revert reason helper。memory buffer pass 不应负责判断这些 check 的语义，应该只提供 buffer / allocation 事实。

结论：ABI buffer 的核心不是“有 mstore”，而是 headStart/tail 的关系。第一版可以只恢复常量 offset 的 head 写入；动态 tail 作为候选记录，不急着完整归一。

### public entry return 和 external call

源码：

- `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp`
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`

关键点：

- public entry wrapper 调用目标函数后：
  - `memPos := allocate_unbounded()`
  - `memEnd := abiEncode(memPos, returns...)`
  - `return(memPos, sub(memEnd, memPos))`
- external call 先：
  - `pos := allocate_unbounded()`
  - `mstore(pos, shl28(selector))`
  - `end := encodeArgs(add(pos, 4), args...)`
  - `call(..., pos, sub(end, pos), pos, staticReturndataSize)`
  - 成功后可能 `returndatacopy(pos, 0, returnDataSize)`，再 `finalizeAllocation(pos, returnDataSize)`，然后 ABI decode。

结论：同一个 `pos` 可以先当 call input buffer，又被复用为 output / returndata buffer。memory pass 不能简单按一个 base 一个 kind 分类，应该允许一个 allocation span 有多个 consumer role。

## 对 NotDec 的启发

1. 名字不要再叫旧的 `MemoryObjectPass`。更准确的是先做 `MemoryBufferAnalysis`，再由 pass 写出可被后续 pass 消费的 IR rewrite surface。metadata 只用于调试和 oracle，不是最终产物，也不是 pass 间的主要数据通道。
2. 第一层事实是 allocation：
   - base：`mload(0x40)` 或 `allocate_unbounded()` 返回值。
   - finalize：`mstore(0x40, newPtr)` 或 `finalize_allocation(base, size)` 展开后的等价形状。
   - size：常量、`end - base`、`returndatasize()`、rounded expression。
3. 第二层事实是 buffer writes：
   - `mstore(mem, base + const, value)`
   - `mstore(mem, base, value)`
   - `returndatacopy(base + const, 0, size)`
   - `calldatacopy(base + const, ..., size)`
4. 第三层事实是 consumer：
   - `return(base, size)`
   - `revert(base, size)`
   - `logN(..., base, size)`
   - `call/staticcall/delegatecall(..., inBase, inSize, outBase, outSize)`
5. 一段 memory 可以有多个 role，尤其 external call 的 input / output 复用。数据结构要允许多 role，不要强制单一 `kind`。

## 实现规划

整体路线是先插入保守的 semantic call / marker，再逐步迁移消费者。不要一开始删除低层 EVM memory 指令。等 ABI return、revert、event、external call 等消费者已经稳定读取 marker，并产出对应的高层语义 call 后，再判断哪些低层指令可以隐藏或删除。

### IR rewrite 硬约束

这组 pass 的完成标准不是“识别到了 memory 形状”，而是“把 memory 形状改写成后续 pass 能消费的 IR 事实”。metadata 可以保留，但只能辅助 debug、统计和 oracle，不能作为功能完成的依据。

每一轮实现如果要算作 memory object pass 的实质推进，至少要满足下面三点之一：

- 新增一个明确的 memory semantic call / marker。
- 让一个后续 pass 改成读取已有 memory marker，而不是重新猜原始 EVM memory 形状。
- 补齐一个已有 marker 的语义，使它能被后续 pass 稳定消费。

但完整闭环要同时满足两件事：memory pass 写出稳定 IR 事实，消费者 pass 读取这个事实并产生更高层语义改写。反过来，如果一轮只是在 `mstore`、`mload`、`return`、`revert`、`log`、`call` 上补 metadata，而没有新增或消费 rewrite marker，这轮只能算分析准备，不能算完成 memory object pass 的实质实现。只新增没人读取的 marker，也只能算 rewrite surface 准备，不能算完整功能。

每个主要消费者都要有对应的 IR 落点：

- return / revert / event log：要能绑定 base、size 和相关 writes。
- external call input：要能绑定 input base、input size 和 ABI 参数写入。
- external call output / ABI decode：要能绑定 output base、output size、returndatacopy 或后续 `mload` 读取。
- scratch sha3：要能明确标成 scratch，不误并入 allocation。

### 阶段 1：内部数据结构和 rewrite 输出

新增内部分析 helper，先由 memory pass 和现有 pass 按需调用。分析只是手段，不是交付结果。每个实现点都必须回答两个问题：

- 这条 memory 事实会写成哪个 IR marker / semantic call。
- 哪个后续 pass 会读取这个 marker / semantic call。

答不上来时，只能作为候选分析写进日志，不能算实现完成。metadata 可以同步加，但只能用于 debug、统计和 oracle，不能作为 pass 间接口，也不能作为完成标准。

建议文件：

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h`
- `src/Passes/evm/MemoryBufferAnalysis.cpp`

核心结构：

- `MemoryAllocation`
  - `Value *Base`
  - `Value *Size`
  - `Instruction *AllocatePoint`
  - `Instruction *FinalizePoint`
  - `bool Finalized`
- `MemoryWrite`
  - `CallBase *StoreOrCopy`
  - `Value *Base`
  - `std::optional<uint64_t> Offset`
  - `Value *ValueOrSize`
  - `Kind = MStore / MStore8 / CalldataCopy / ReturndataCopy`
- `MemoryConsumer`
  - `CallBase *Call`
  - `Kind = Return / Revert / Log / ExternalCallInput / ExternalCallOutput`
  - `Value *Base`
  - `Value *Size`

第一版只收集同一函数内 SSA use-def 能直接追到的事实，不做完整 memory SSA。它的输出不能停在 C++ 内部结构，也不能只挂 metadata；必须在 IR 里插入稳定的 marker / semantic call。实现日志里也要明确写出本轮是“新增 rewrite surface”、“消费已有 rewrite surface”，还是“候选分析准备”；只有前两类算实质推进，完整闭环还必须有消费者读取。

### 阶段 2：free pointer / allocation 识别

先支持确定性最高的形状：

- `base = evm_mload(mem, 64)`。
- `newPtr = base + size` 或 `base + roundUp(size)`。
- `evm_mstore(mem, 64, newPtr)`。
- helper 展开后的 `finalize_allocation(base, size)` 形状，包括 Panic `0x41` guard 后成功块写回。

复用 checked-bounds 里已有的 free pointer helper 思路，避免两套判断。必要时把这些 helper 从 checked-bounds 迁到 `MemoryBufferAnalysis`。

这一阶段的交付结果必须是 allocation 语义写回 IR，例如 `notdec_solidity_memory_allocation(base, size)` 或同等 semantic call。只识别 `mload(0x40)` / `mstore(0x40, newPtr)`，但没有写出可消费的 IR 事实，不算完成。

### 阶段 3：按 base 收集写入

只处理 base 明确、offset 明确的写入：

- `evm_mstore(mem, base, value)` -> offset 0。
- `evm_mstore(mem, base + C, value)` -> offset C。
- `evm_mstore(mem, C, value)` 中 C 为 0/32 的，先归类为 scratch，不挂到 allocation。
- `returndatacopy(base + C, 0, size)` 和 `calldatacopy(base + C, ..., size)` 记录 copy write。

第一版不处理 PHI base、不处理复杂 symbolic offset、不处理循环里的逐项写入，只标候选。

这一阶段的交付结果必须是 write 语义写回 IR，例如 word write、copy write、byte write 这类 marker / semantic call。后续 ABI return、revert、event、external call pass 要能读取这些 write 事实，不能各自重新扫描同一批低层 `mstore` / `mload` / copy call。

### 阶段 4：消费者接入

先接两个最稳定的消费者。这里的“接入”不是复制一份低层 pattern 逻辑，而是让消费者优先读取 memory rewrite marker / semantic call；缺 marker 时可以保留原有保守 fallback，但不能把 fallback 当作新 memory object 能力。每个消费者迁移后，都要产出更高层的语义 call，证明它不是只读了 marker 做统计：

- `AbiReturnPass`
  - `return(base, size)` 找到对应 writes。
  - 静态 `return(base, 32)` 可以恢复 offset 0 的 return value。
  - `return(base, sub(end, base))` 先记录 dynamic return buffer。
- `SolidityRevertPass`
  - `revert(base, size)` 找 selector write、Error(string)、custom error 参数。
  - 现有 revert matcher 里已有部分同 block / offset 逻辑，迁到共享 memory 分析后减少重复。

再接：

- `EventLogPass`：`logN(..., dataBase, dataSize)`。
- `ExternalCallPass`：call input/output buffer，允许 input/output 同 base。
- `StorageAddressingPass`：只读 scratch `mstore(0, key); mstore(32, slot); sha3(0,64)`，不当作 allocation。

每迁移一个消费者，都要在 manifest oracle 里检查对应 semantic call / marker 的数量，不能只检查旧 metadata。固定 100 个 apehex 用例也要统计这些 marker，避免只在小样例里闭环。

阶段 4 是 memory object pass 是否真正闭环的关键。只有“memory pass 写 marker，消费者 pass 读取 marker，并产出 ABI return / revert / event / external call 级别的 semantic call”，才算完成一个功能闭环。

### 阶段 5：IR rewrite surface

不要恢复旧 `MemoryObjectPass` 那种单纯计数 metadata。memory 相关能力必须写出明确的 IR rewrite surface，让后续 ABI / revert / event / external-call pass 能消费。metadata 可以同步保留，但只用于调试、统计和测试 oracle。实现时要优先保证 marker / semantic call 的语义稳定，metadata 不能作为主要接口。

这一阶段的硬要求是：后续 pass 要能读 IR 里的 marker / semantic call 完成改写。仅仅在原始 `mstore`、`return`、`revert`、`log` 或 `call` 上挂 metadata，不算 IR rewrite，也不算完成。仅仅插入没人读取的 marker，也只能算 rewrite surface 准备；必须在后续迭代里接入消费者，并产出 ABI return / revert / event / external call 级别的语义 call，才算完整闭环。

建议按这个闭环推进：

- `notdec_solidity_memory_consumer` 先统一描述 base / size / consumer kind。
- ABI return / revert / event / external call 分别插入更具体的 consumer marker，例如 `notdec_solidity_abi_return_memory_consumer`、`notdec_solidity_revert_memory_consumer`。
- 后续 pass 读具体 marker，不再重复猜同一段低层 memory 形状。
- 对仍然需要低层写入信息的 pass，再补 write marker，例如 word write、returndatacopy write、calldatacopy write。

第一阶段 rewrite 不急着删除原始 `mstore/mload/copy`，先插入稳定的语义 call / marker。后续 pass 必须能直接基于这些 IR marker 工作，而不是重新从低层 EVM memory call 猜一次：

- allocation rewrite：表达 base、size、finalized，例如 `notdec_solidity_memory_allocation(base, size)`。
- buffer role rewrite：表达 return / revert / event / call_input / call_output / scratch。
- write rewrite：表达 base、offset、source kind，例如常量 offset 的 word write。
- consumer rewrite：在 `return/revert/log/call` 附近绑定 buffer role，避免后续 pass 重新猜 base / size。

第二阶段再按消费者迁移情况决定是否隐藏或删除低层 `mstore/mload/copy`。只要后续 pass 还依赖低层指令，就只能 hide 不能删。最终目标不是“IR 上多一些标注 call”，而是让反编译主流程尽量基于高层语义 call 工作，低层 memory 指令只作为保守 fallback 或调试证据存在。也就是说，IR rewrite 不是可选优化，而是这组 pass 的目标本身。

完成标准里，只有 metadata 没有 rewrite surface 不算完成；有 marker 但后续 pass 仍完全绕过 marker、继续猜原始 memory 形状，也不算完成。每个主要消费者迁移后，都要留下一个 proof marker 或语义 call，证明它确实读到了 memory rewrite 结果。

## 风险

- external call 的 input/output 复用同一 `pos`，单一 kind 分类会错。
- `allocate_unbounded()` 和 `finalize_allocation()` 可能被分开很远，跨块绑定过激会误报。
- storage sha3 scratch 使用 `mstore(0/32)`，不能误当作 ABI buffer。
- checked-bounds 已经识别一部分 allocation guard，memory pass 如果重复 rewrite 会打架。
- rewrite marker 如果设计得太像普通 call，可能影响后续 pattern matcher；插入位置和命名要稳定。
- 真实 IR 里函数调用、内联 helper、优化后的表达式形状会混合出现，第一版必须保守。

## 判断标准

- 先在手写或小型 Solidity-generated case 上证明：
  - 能识别 free pointer base 和 finalize。
  - 能收集 return/revert buffer 的常量 offset writes。
  - 不把 storage sha3 scratch 当 allocation。
  - 能插入 memory allocation / buffer role / write / consumer 的 rewrite surface。
- manifest oracle 不能只看 metadata 数量，要检查对应 rewrite marker / semantic call。
- 后续消费者要逐步改成读取 memory rewrite marker / semantic call；否则 memory pass 只是旁路标注，还没有真正接入。
- 每轮实现记录都要说明本轮新增了哪个 IR rewrite surface，以及哪个消费者已经读取它；如果还没有消费者读取，只能记为分析准备，不能记为功能完成。
- 每轮实现记录都要说明这轮是否还保留 fallback。如果有 fallback，要写明 fallback 是兼容旧 IR，还是因为 marker 还没补齐。
- 固定 100 个 apehex audit 要统计新增 marker。新增 marker 如果在真实样例里长期 0 命中，需要回到 Solidity 源码和 IR 形状重新判断设计是否太理想化。
- `notdec.evm.solidity_patterns` 继续通过。
- 对 apehex 当前样例跑 audit，只统计命中和未命中，不因 memory pass 改变既有 checked-bounds / revert 行为。
- 实现过程中如果涉及 checked-bounds、external-call、event-log 的消费者，分别在对应 `logs/20260522-proj-passes/` 分类下补实现记录。

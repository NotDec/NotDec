# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后最新统计：

- pattern suite round55：88 个输出，skip `2 {65:1,1:1}`。
- batch614 round55-final：39 个输出，skip `2 {65:1,1:1}`。
- batch613 / batch619：skip `0`。

剩余 `panic 0x41` 在 `24534_19759897_d822801d63_07fc71c92888`。真实 IR 里同一个函数连续做两次 allocation：

```llvm
%oldPtr = add i256 %base, 96
call void @evm_mstore(ptr %mem, i256 %slot, i256 %oldPtr)
...
%newPtr = add i256 %base, 224
%range = icmp ugt i256 %base, 18446744073709551391
%nowrap = icmp ult i256 %newPtr, %oldPtr
%fail = or i1 %nowrap, %range
br i1 %fail, label %panic, label %success

success:
  call void @evm_mstore(ptr %mem, i256 %slot, i256 %newPtr)
```

Solidity `finalizeAllocation` 的 guard 是 `or(gt(newFreePtr, uint64max), lt(newFreePtr, memPtr))`。这里经过前一段 allocation rewrite 和优化后，range check 变成了 `base > uint64max - 224`，nowrap check 仍是 `newPtr < oldPtr`。

## 本轮选择

处理连续 allocation 后的 offset pointer bounds：

- `panic 0x41`。
- `oldPtr = base + A`，`newPtr = base + B`，`B > A`。
- delta `B - A` 必须是小的 32 对齐常量。
- guard 块里必须先把 `oldPtr` 写回某个 free pointer slot。
- success 块必须把 `newPtr` 写回同一个 slot。

不做的内容：

- 不把任意两个同 base offset 的比较都当 allocation。
- 不处理 `panic 0x01`，当前仍不能证明是 compiler guard。
- 不做跨 block 的一般数据流，只接受当前真实 IR 里同 guard block / success block 的 store transition。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:4579`
  - 增加 `findMemoryPointerStoreSlot`，找 guard block 里写入 old pointer 的 slot。
- `src/Passes/evm/SolidityPatterns.cpp:4593`
  - 增加 `findMemoryPointerStoreToSlot`，确认 success block 把 new pointer 写回同一 slot。
- `src/Passes/evm/SolidityPatterns.cpp:4609`
  - 增加 `hasMemoryPointerStoreTransition`，把 offset pointer bounds 限制在 free-pointer 更新形状上。
- `src/Passes/evm/SolidityPatterns.cpp:4621`
  - 增加 `matchConstantOffsetAllocationDelta`，只接受同 base 常量 offset 且 delta 小、32 对齐。
- `src/Passes/evm/SolidityPatterns.cpp:4970`
  - `matchMemoryAllocationPointerBounds` 支持 range check 写成 `base > uint64max - total`。
- `src/Passes/evm/SolidityPatterns.cpp:5048`
  - 在直接 `newPtr = oldPtr + size` 之外，支持 `oldPtr = base + A` / `newPtr = base + B`。
- `test/evm/solidity-patterns/manifest.json:4601`
  - 更新 `24534...` oracle：`memory_allocation_pointer_bounds` 从 3 到 4，skip 清零，CFG rewrite 从 9 到 10。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24534...`：输出过 `llvm-as`，skip `{}`，新增 marker 为 `memory_allocation_pointer_bounds(%evm.add46, 128, %evm.add57)`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round56`：88 passed, 0 failed。
- pattern suite round56：88 个输出，skip `1 {1:1}`，`llvm-as` 88/88 通过。
- batch614 round56：39 个输出，skip `1 {1:1}`，`llvm-as` 39/39 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round56.c --tr-level=2`，通过，`elapsed=16.89`。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. pattern `0334...` 和 batch614 `24543...` 的 `panic 0x01` 各 1 个。当前只知道是 assert/panic 分支，还不能证明是 Solidity compiler guard。
2. 继续抽样 apehex 后续批次。优先找 `panic 0x11/0x32/0x41/0x65` 中真实 case 数量更多、能对照 Solidity codegen 的形状。
3. 如果后续批次仍只剩 `0x01`，需要回到 Solidity 生成源头或更强上下文判断，避免把业务 assert 当作 compiler bounds。


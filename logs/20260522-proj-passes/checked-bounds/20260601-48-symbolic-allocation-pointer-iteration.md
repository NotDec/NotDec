# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后旧 batch 里主要只剩 `panic 0x01`。继续抽样 batch619 没有 skip，batch613 也没有 skip。batch614 有 10 个 skip：

- `panic 0x00` 9 个。
- `panic 0x01` 1 个。

进一步看 IR 后，`panic 0x00` 不是实际 panic code 为 0，而是 panic code 识别漏掉了通过函数参数传入的 revert buffer offset。

## 本轮选择

本轮处理 batch614 里数量最多、语义明确的 `panic 0x41` memory allocation guard。

真实 IR 形状：

- revert 块写了 `Panic(0x41)`，但 selector/code 的 mstore offset 是函数参数，原识别只能看到 selector，看不到 code。
- guard 形状来自 Solidity `finalizeAllocation`：
  - `newFreePtr := add(memPtr, size)`
  - `if or(gt(newFreePtr, uint64max), lt(newFreePtr, memPtr)) panic`
  - `mstore(freeMemoryPointer, newFreePtr)`
- 当前 IR 里 `freeMemoryPointer` 有时不是字面量 `64`，而是函数参数。所有调用点都传同一个常量 `64`。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256` 的 `finalizeAllocationFunction`。

不做的内容：

- 不处理 `panic 0x01`。
- 不处理 `24534...` 里跨两次 allocation 的剩余 `panic 0x41`，因为需要更强的数据流证明。
- 不把任意符号 mstore 都当作 free memory pointer，只接受从同一 slot `mload` 并在成功分支 `mstore` 回同一 slot 的形状。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:1415`
  - 增加 `isKnownPanicCode`。
- `src/Passes/evm/SolidityPatterns.cpp:1467`
  - 在 panic revert 识别里，若已确认是长度 36 的 panic revert 且只有一个合法 panic code 常量 mstore，就把它作为 panic code。
- `src/Passes/evm/SolidityPatterns.cpp:4413`
  - 增加 `getUniformConstantArgument`，只在函数参数所有 callsite 都传同一常量时使用。
- `src/Passes/evm/SolidityPatterns.cpp:4441`
  - 增加 `isSupportedMemoryAllocationSizeWithUniformArg`，让 allocation size 可以来自这种统一常量参数。
- `src/Passes/evm/SolidityPatterns.cpp:4452`
  - 增加 `getMemoryPointerLoadSlot` 和 `findMemoryPointerStoreForLoad`，匹配从同一 slot load/store free pointer。
- `src/Passes/evm/SolidityPatterns.cpp:4663`
  - fixed pointer bounds 允许 `newPtr` 在 guard block 里计算，并允许成功分支写回同一 memory pointer slot。
- `src/Passes/evm/SolidityPatterns.cpp:4892`
  - `finalizeAllocation` 的 or 条件允许同一 slot load/store 和统一常量参数 size。
- `test/evm/solidity-patterns/cases/24541_19759925_deec0e0bb2_47ef0dd1d7b5.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4571`
  - 增加 `24541...` oracle，检查 semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24541...`：输出过 `llvm-as`，skip `0`，`memory_allocation_pointer_bounds` 3 个，`memory_allocation_bounds` 1 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round54`：87 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round54：87 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 87/87 通过。
- batch614 round54：39 个输出，bad log 0，skip `3 {1:1,65:1,50:1}`，`llvm-as` 39/39 通过。
- batch619 round52：39 个输出，bad log 0，skip `0`。
- batch613 round52：38 个输出，bad log 0，skip `0`。
- fortune smoke：`elapsed=16.52`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch614 `24534...` 的 `panic 0x41` 1 个。形状是跨两次 allocation 的 pointer bound，需要证明 `newPtr - oldPtr` 的关系后再做。
2. batch614 `24534...` 的 `panic 0x32` 1 个。可能是 enum / small array bounds 新形状，需要单独确认。
3. pattern `0334...`、batch614 `24543...`、batch615 `24561...`、batch616 多个 `panic 0x01`。当前不能证明是 compiler-generated guard，继续保留 skip。
4. 继续抽样新 apehex 批次，优先找 `panic 0x11/0x32/0x41/0x65` 中语义明确且数量更多的形状。

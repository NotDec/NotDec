# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

本轮先抽样 batch607-612、615-618：

- batch607：skip `4 {65:4}`，都在 `24259_19755445_aefeec2314_4f43187f4106`。
- batch608：skip `1 {17:1}`。
- batch609：skip `1 {17:1}`。
- batch610：skip `1 {65:1}`。
- batch611：skip `6 {17:4,65:2}`。
- batch612：skip `13 {17:11,18:2}`。
- batch615：skip `1 {1:1}`。
- batch616：skip `4 {1:4}`。
- batch617 / batch618：skip `0`。

batch607 的 `24259...` 里有两类 `0x41`：

- 两个 storage length 上限：`sload < 2^64` 后递增 storage array length。这轮不动。
- 两个 allocation helper：`newPtr = oldPtr + 64/96`，success `mstore(64, newPtr)`。

allocation helper 的 nowrap guard 写成：

```llvm
%newPtr = add i256 oldPtr, 64
%wrap = icmp ugt i256 oldPtr, -65
%range = icmp ugt i256 %newPtr, uint64max
br i1 (or %range, %wrap), panic, success
```

这是 Solidity `finalizeAllocation` 的 wrapping-add 保护等价形状：`oldPtr > 2^256 - size - 1`。

## 本轮选择

处理 allocation pointer bounds 的 wrapping-add strict upper 形状：

- `panic 0x41`。
- `newPtr = oldPtr + size`。
- nowrap 条件是 `oldPtr > 2^256 - size - 1`。
- size 必须是小的 32 对齐常量，并且和 `newPtr` 里的 size 一致。
- success 必须写回 free pointer slot。

不做的内容：

- 不处理 storage length 上限的两个 `0x41`。
- 不处理 batch612 的 `0x11/0x12`，数量多但语义还没确认，不能抢先扩。
- 不处理 `panic 0x01`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:246`
  - 增加 `matchWrappingAddStrictUpper`，从 `2^N - size - 1` 常量里恢复小的 32 对齐 size。
- `src/Passes/evm/SolidityPatterns.cpp:5018`
  - `matchMemoryAllocationPointerBounds` 增加 `NoWrapStrictUpperSize`。
- `src/Passes/evm/SolidityPatterns.cpp:5041`
  - 识别 `oldPtr > 2^256 - size - 1` 作为 failure-side nowrap guard。
- `src/Passes/evm/SolidityPatterns.cpp:5164`
  - 校验 nowrap guard 恢复出的 size 和 `newPtr = oldPtr + size` 里的 size 一致。
- `test/evm/solidity-patterns/cases/24259_19755445_aefeec2314_4f43187f4106.ll`
  - 增加 batch607 真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4715`
  - 增加 `24259...` oracle，检查 semantic marker、skip reason、CFG rewrite。剩余两个 storage length `0x41` 继续要求 skip。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24259...`：输出过 `llvm-as`，skip 从 `{65:4}` 变为 `{65:2}`，新增 `memory_allocation_pointer_bounds` 2 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round58`：90 passed, 0 failed。
- pattern suite round58：90 个输出，skip `3 {65:2,1:1}`，`llvm-as` 90/90 通过。
- batch607 round58-final：38 个输出，skip `2 {65:2}`，`llvm-as` 38/38 通过。
- batch614 round58-final：39 个输出，skip `1 {1:1}`，`llvm-as` 39/39 通过。
- batch620 round58-final：38 个输出，skip `0`，`llvm-as` 38/38 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round58.c --tr-level=2`，通过，`elapsed=17.02`。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch607 `24259...` 剩余两个 `0x41` storage length upper-bound。需要对照 Solidity storage dynamic array push/length codegen 后再做。
2. batch612 `24448...` 的 `0x11/0x12` 共 12 个。数量最多，但要先分类是 checked arithmetic、division by zero 还是业务 guard。
3. batch610 / batch611 的剩余 `0x41`。先看是否和本轮类似，若不同再单独处理。
4. pattern/batch614/batch615/batch616 的 `panic 0x01`。仍不能证明是 compiler bounds。


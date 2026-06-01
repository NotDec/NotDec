# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后：

- pattern suite round56：88 个输出，skip `1 {1:1}`。
- batch614 round56：39 个输出，skip `1 {1:1}`。
- 新抽样 batch620 / batch621 / batch622：
  - batch620：38 个输出，skip `1 {65:1}`。
  - batch621：40 个输出，skip `0`。
  - batch622：4 个输出，skip `0`。

batch620 的剩余 `panic 0x41` 在 `24763_19761741_c9ddde2099_02b0670bed95`。真实 IR 形状：

```llvm
%size = and i256 (add (sub (add oldPtr, len), oldPtr), 31), -32
%newPtr = add i256 oldPtr, %size
%gt = icmp ugt i256 %newPtr, uint64max
%lt = icmp ult i256 %newPtr, oldPtr
br i1 (or %lt, %gt), panic, success

success:
  mstore(freePtrSlot, %newPtr)
```

函数只有一个 callsite：`oldPtr=128`，`freePtrSlot=64`。这对应 Solidity `finalizeAllocation` 的首次内存分配；`128` 是初始 free memory pointer，`64` 是 free pointer slot。

## 本轮选择

处理“统一常量参数传入初始 free pointer”的 allocation pointer guard：

- `panic 0x41`。
- `oldPtr` 必须是字面量或所有 callsite 统一传入 `128`。
- success block 必须 `mstore` 到字面量或统一参数 `64`。
- size 仍必须是已支持的 allocation size 表达式。

不做的内容：

- 不把任意函数参数当作 free pointer。
- 不处理非 `128/64` 的符号 oldPtr / slot。
- 不处理剩余 `panic 0x01`，仍不能证明是 compiler guard。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:4515`
  - 增加 `isConstantIntValueOrUniformArg`，只在所有 callsite 统一常量时把函数参数当常量。
- `src/Passes/evm/SolidityPatterns.cpp:4565`
  - 增加 `findUniformFreeMemoryPointerStore`，允许 success block 写回统一常量 `64` 的 free pointer slot。
- `src/Passes/evm/SolidityPatterns.cpp:5139`
  - `matchMemoryAllocationPointerBounds` 增加 `oldPtr=128` 且 success 写回 `slot=64` 的窄路径。
- `test/evm/solidity-patterns/cases/24763_19761741_c9ddde2099_02b0670bed95.ll`
  - 增加 batch620 真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4673`
  - 增加 `24763...` oracle，检查 semantic marker、skip reason、CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24763...`：输出过 `llvm-as`，skip `{}`，`memory_allocation_pointer_bounds` 2 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round57`：89 passed, 0 failed。
- pattern suite round57：89 个输出，skip `1 {1:1}`，`llvm-as` 89/89 通过。
- batch614 round57-final：39 个输出，skip `1 {1:1}`，`llvm-as` 39/39 通过。
- batch620 round57-final：38 个输出，skip `0`，`llvm-as` 38/38 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round57.c --tr-level=2`，通过，`elapsed=17.17`。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. pattern `0334...` 和 batch614 `24543...` 的 `panic 0x01` 各 1 个。当前只知道是 assert/panic 分支，不能证明是 compiler bounds。
2. 继续抽样 batch607-612 / 615-618，优先找 `0x11/0x32/0x41/0x65` 中还有数量的真实形状。
3. 如果后续抽样仍只剩 `0x01`，需要回到 Solidity 源码或更强上下文判断，避免误 rewrite 业务 assert。


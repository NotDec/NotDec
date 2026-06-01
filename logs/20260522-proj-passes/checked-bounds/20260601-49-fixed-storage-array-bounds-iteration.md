# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后最新统计：

- pattern suite round54：87 个输出，skip `1 {1:1}`，只在 `0334_19494307_668d201319_1354ce2e324d`。
- batch614 round54：39 个输出，skip `3 {1:1,65:1,50:1}`。
- batch613 / batch619：skip `0`。

batch614 的 `24534_19759897_d822801d63_07fc71c92888` 里剩一个 `panic 0x32`：

```llvm
%evm.lt = icmp ugt i256 %evm.calldataload, 2
br i1 %evm.lt, label %panic, label %success

success:
  %evm.add5 = add nuw nsw i256 %evm.calldataload, 9
  %evm.sload = call i256 @evm_sload(i256 %evm.add5)
```

Solidity `storageArrayIndexAccessFunction` 的 guard 是 `if iszero(lt(index, arrayLength)) panic(0x32)`。固定长度 storage array 会把长度编成常量，成功分支用固定 data slot 加 index-derived offset 访问 storage。

## 本轮选择

处理固定 storage array 的 `index > maxIndex` bounds guard：

- `panic 0x32`。
- 比较是 `index > maxIndex` 或反向等价形状。
- 成功分支必须有 `evm_sload/evm_sstore(constantSlot + index-derived-offset)`。
- rewrite 后输出 `array_bounds_storage(index, maxIndex + 1)` 并做 CFG rewrite。

不做的内容：

- 不处理剩余 `panic 0x41`，它是跨两次 allocation 的 pointer bound，还需要更强证明。
- 不把任意 `panic 0x32` + 常量比较都当作 storage array，必须看到成功分支 storage 访问。
- 不处理 pattern 里的 `panic 0x01`，当前仍不能证明是 compiler guard。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3461`
  - 增加 `getArrayLengthFromMaxIndex`，把 `index > maxIndex` 转成长度 `maxIndex + 1`。
- `src/Passes/evm/SolidityPatterns.cpp:3470`
  - 增加 `isStorageArrayIndexScale`，只接受 index、非零常量倍数、非零常量除数。
- `src/Passes/evm/SolidityPatterns.cpp:3498`
  - 增加 `isFixedStorageArraySlotOffset`，只匹配常量 storage slot 加 index-derived offset。
- `src/Passes/evm/SolidityPatterns.cpp:3512`
  - 增加 `hasFixedStorageArrayElementAccess`，要求成功分支存在对应 `evm_sload/evm_sstore`。
- `src/Passes/evm/SolidityPatterns.cpp:3558`
  - `matchArrayBounds` 支持 `UGT/ULT` 的 max-index 形状。
- `src/Passes/evm/SolidityPatterns.cpp:3589`
  - unknown array bounds 在看到固定 storage array 访问时标成 `array_bounds_storage`。
- `test/evm/solidity-patterns/cases/24534_19759897_d822801d63_07fc71c92888.ll`
  - 增加 batch614 真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4569`
  - 增加 `24534...` oracle，检查 semantic marker、skip reason、CFG rewrite、returndata bubble。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24534...`：输出过 `llvm-as`，skip 从 `{65:1,50:1}` 变为 `{65:1}`，`array_bounds_storage` marker 1 个，payload 为 `index, 3`。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round55`：88 passed, 0 failed。
- pattern suite round55：88 个输出，`llvm-as` 88/88 通过。
- batch614 round55-final：39 个输出，skip `2 {1:1,65:1}`，`llvm-as` 39/39 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round55.c --tr-level=2`，通过，`elapsed=16.70`。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch614 `24534...` 的 `panic 0x41` 1 个。形状像 `base + 224` 与 `base + 96` 的第二段 allocation pointer bound，需要证明 `oldPtr/newPtr/size` 后再做。
2. pattern `0334...` 和 batch614 `24543...` 的 `panic 0x01` 各 1 个。当前只知道是 assert/panic 分支，不能证明 compiler guard，继续保留 skip。
3. 继续抽样 apehex 后续批次，优先找 `panic 0x11/0x32/0x41/0x65` 中数量更多、能对照 Solidity codegen 的形状。


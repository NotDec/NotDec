# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 pattern suite 还有 2 个 checked-bounds skip：

- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: `panic 0x32` 1 个
- `0334_19494307_668d201319_1354ce2e324d`: `panic 0x01` 1 个

batch589 已清零。本轮额外用最新 batch617 抽样复跑，发现还有 1 个真实 case `24663...` 里有 3 个 `panic 0x11` skip，但这是另一类 checked arithmetic，留到下一轮。

## 本轮选择

本轮只处理 `2001...` 的 `panic 0x32`。

真实 IR 形状：

- 条件是 `index < length`，失败边进入 `Panic(0x32)`。
- `length` 是函数参数，不是直接的 `mload/sload/calldataload`，所以旧 matcher 只能给 `unrecognized_operands`。
- 成功边马上用同一个 `index` 做 `index * 192`，再加到内存数组 base 上，得到元素字段地址，并传给 private helper 访问。

Solidity codegen 对应 `/sn640/solidity/libsolidity/codegen/ArrayUtils.cpp:690` 的 `ArrayUtils::accessIndex`：启用 bounds check 时先取 length，再检查 `index < length`，越界触发 `PanicCode::ArrayOutOfBounds`。这里是内存数组元素访问的 compiler guard，不是业务分支。

不做的内容：

- 不把所有 `length` 为参数的 `panic 0x32` 都归成 memory array。
- 不处理 `0334...` 的 `panic 0x01`。
- 不处理 batch617 的 `panic 0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2997`
  - 增加 `isMemoryArrayIndexScale`，只接受同一 success block 里的 `index * 32N`。
  - 增加 `valueUsesMemoryArrayIndexScale`，只沿少量 `add` 追踪元素地址。
  - 增加 `hasMemoryArrayElementAccess`，要求缩放后的 index 地址被传给 call。
- `src/Passes/evm/SolidityPatterns.cpp:3099`
  - 扩展 `matchArrayBounds`：当 length 不是直接 load、但 success block 能证明存在内存数组元素地址访问时，rewrite 为 `array_bounds_memory`。
- `test/evm/solidity-patterns/manifest.json:1557`
  - 更新 `2001...` oracle：`array_bounds_memory` 从 4 到 5，移除 `panic_array_out_of_bounds`，skip 清零，CFG rewrite 从 36 到 37。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round45`：80 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round45：80 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 80/80 通过。
- batch617 round45：38 个输出，bad log 0，skip `3 {17:3}`，集中在 `24663_19760902_bbe5fabbb5_b34e557279fe`，`llvm-as` 38/38 通过。
- fortune smoke：`elapsed=17.30`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch617 `24663_19760902_bbe5fabbb5_b34e557279fe` 的 3 个 `panic 0x11`。真实 case 数量最高，需要先确认是不是 Solidity checked arithmetic helper 被清理后的形状。
2. pattern `0334...` 的 `panic 0x01`，1 个 case。需要先区分 compiler assert 和业务 assert。
3. 继续抽样更新的 apehex 批次，找是否还有更高频 checked-bounds skip。

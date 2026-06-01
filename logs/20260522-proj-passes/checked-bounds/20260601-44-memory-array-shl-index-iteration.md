# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后最新 pattern suite 只剩 1 个 skip：

- pattern round46：81 个输出，skip `1 {1:1}`，只在 `0334_19494307_668d201319_1354ce2e324d`。

继续抽样真实 apehex 批次后，batch616 还有 8 个 skip：

- `24601_19760411_36f7edebdb_cd389ea0a4e2`：`panic 0x32` 3 个。
- `24615_19760463_d297b0b41b_d1c5e980d31f`：`panic 0x01` 1 个。
- `24618_19760477_d9f98dac55_dc2bad544b95`：`panic 0x01` 1 个。
- `24620_19760498_0dd6546039_67278231360c`：`panic 0x01` 1 个。
- `24626_19760530_20785c5a43_3cd43132e796`：`panic 0x01` 1 个。
- `24612_19760427_dd0dd0b57c_933311393a34`：`panic 0x11` 1 个。

## 本轮选择

本轮只处理 `24601...` 的 3 个 `panic 0x32`。

真实 IR 形状：

- guard 是 `icmp ult i256 index, length`，失败分支 `Panic(0x32)`。
- 成功分支用 `shl i256 index, 5` 计算 `index * 32`，再加到 memory array base 上访问元素。

Solidity codegen 对应：

- `/sn640/solidity/libsolidity/codegen/ArrayUtils.cpp:690` 先取数组长度，再检查 `index < length`，失败报 `PanicCode::ArrayOutOfBounds`。

这和上一轮已支持的 `index * 32N` memory array bounds 是同一类 compiler guard，只是 IR 把乘 32 规约成了 `shl 5`。

不做的内容：

- 不处理 `panic 0x01`。当前看到的 `0334...` 和 batch616 里的 `panic 0x01` 更像 assert / 业务 invariant，不能只为清 skip 改 CFG。
- 不把任意 shift 都当作 array scale，只接受同一 index 的 `shl index, 5`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3119`
  - 扩展 `isMemoryArrayIndexScale`，在原有 `mul index, 32N` 外，额外接受 `shl index, 5`。
- `test/evm/solidity-patterns/cases/24601_19760411_36f7edebdb_cd389ea0a4e2.ll`
  - 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:4285`
  - 增加 `24601...` oracle，检查 checked-bounds kind、panic code、semantic marker、skip reason 和 CFG rewrite。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24601...`：输出过 `llvm-as`，skip `0`，`array_bounds_memory` semantic marker 7 个，checked-bounds metadata 17 个。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round47`：82 passed, 0 failed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- pattern suite round47：82 个输出，bad log 0，skip `1 {1:1}`，`llvm-as` 82/82 通过。
- batch616 round48：40 个输出，bad log 0，skip `5 {1:4,17:1}`，`llvm-as` 40/40 通过。
- fortune smoke：`elapsed=17.07`，通过。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch615 的 `panic 0x11`，4 个真实 skip。优先确认是否是 checked arithmetic，若能对上 Solidity codegen 再扩展 matcher。
2. batch616 的 `panic 0x11`，1 个真实 skip。数量少于 batch615，但可和同类 `panic 0x11` 一起看。
3. batch615 的 `panic 0x32`，2 个真实 skip。先看是否是 memory/calldata/storage array bounds 的新 IR 形状。
4. pattern `0334...` 和 batch616 的 `panic 0x01`。语义不够确定，除非能证明是 compiler-generated assert，否则继续保留 skip。

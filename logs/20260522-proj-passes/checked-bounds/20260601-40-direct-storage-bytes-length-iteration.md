# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 背景

上一轮后 pattern suite 输出还有 6 个 checked-bounds skip：

- `0648_19497230_998d150449_ea367f7fc1ac`: `panic 0x41` 2 个
- `0651_19497235_18ca6cbfa7_017843325734`: `panic 0x41` 2 个
- `0334_19494307_668d201319_1354ce2e324d`: `panic 0x01` 1 个
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: `panic 0x32` 1 个

batch589 真实 IR 输出还有 2 个 `panic 0x11` skip，分别在 `23523...` 和 `23545...`。这两个是另一类 checked arithmetic，风险不同，本轮不混在一起改。

## 本轮选择

本轮只处理 `0648...` 和 `0651...` 里的 4 个 `panic 0x41`。

真实 IR 形状是：

- 失败边是 `len > uint64_max` 后进入 `Panic(0x41)`。
- 成功边先 `sload(slot)`，再调用 private helper 解出旧 storage bytes/string 长度。
- 随后按旧长度 `> 31` 分支清理旧数据。
- 后面还有新长度 `> 31` 分支。
- 长 storage 路径把 `(len << 1) + 1` 写回同一个 slot。
- 短 storage 路径把带有 `(len << 1)` 的编码值写回同一个 slot。

Solidity codegen 对应在 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2072` 的 `copyByteArrayToStorageFunction`：它先检查 `newLen > 0xffffffffffffffff` 并触发 `PanicCode::ResourceError`，再按 `newLen > 31` 写入 storage bytes/string 编码。这里是编译器生成的长度 guard，不是业务分支。

不做的内容：

- 不把所有 `panic 0x41` 都放宽成 storage bytes/string rewrite。
- 不处理 batch589 的 `panic 0x11`，那是 checked arithmetic 方向。
- 不处理剩余 `panic 0x01` / `panic 0x32`，还需要单独确认语义。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3674`
  - 增加 `isShiftedStorageBytesLength`、`valueContainsShiftedStorageBytesLength`，识别 storage bytes/string 的 `len << 1` 编码。
  - 增加 `hasOldStorageBytesLengthBranch`，要求成功边存在基于同一个 `sload(slot)` 的旧长度 `> 31` 分支。
  - 增加 `collectReachableBlocks` 和 `hasDirectStorageByteArrayWrite`，在有限深度内确认后续存在新长度 `> 31` 分支，并把包含 `len << 1` 的值写回同一个 slot。
  - 增加 `hasDirectStorageByteArrayCopyToStorageSuccess`，覆盖 helper 被内联或拆开的 direct storage write 形状。
- `src/Passes/evm/SolidityPatterns.cpp:3988`
  - 扩展 `matchStorageByteArrayLengthBounds`，只有 helper-copy 形状或 direct-write 形状确认后才 rewrite。
- `test/evm/solidity-patterns/manifest.json:1888`
  - 更新 `0648...` oracle：`storage_byte_array_length_bounds` 增加 2，skip 清零，CFG rewrite 从 28 到 30。
- `test/evm/solidity-patterns/manifest.json:2385`
  - 更新 `0651...` oracle：同上。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round42`：78 passed, 0 failed。
- pattern suite skip：从 `6 {65:4, 1:1, 50:1}` 降到 `2 {1:1, 50:1}`。
- pattern suite round42 `llvm-as`：78/78 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 通过。
- fortune smoke：`elapsed=16.71`，通过。
- batch589 round41：38 个输出，无 bad log，skip 仍是 `2 {17:2}`，符合本轮不处理 `0x11` 的预期。

## 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. batch589 `23523...`、`23545...` 的 `panic 0x11`，2 个真实 case。真实收益最高，但要确认是常量/退化 checked arithmetic guard，不能只因为形状简单就 rewrite。
2. pattern `2001...` 的 `panic 0x32`，1 个 case。看起来和 private helper 的数组下标关系有关，需要先确认不是业务越界检查。
3. pattern `0334...` 的 `panic 0x01`，1 个 case。要区分 compiler assert 和业务 assert，语义确定前不动。

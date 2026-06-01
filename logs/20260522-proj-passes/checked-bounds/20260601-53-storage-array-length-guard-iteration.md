# 原始需求

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

## 本轮发现

当前 `test/evm/solidity-patterns/manifest.json` 里 checked-bounds 剩余 skip：

- `24259_19755445_aefeec2314_4f43187f4106`：`unrecognized_operands` 2 个，都是 `panic 0x41`。
- `0334_19494307_668d201319_1354ce2e324d`：`unrecognized_operands` 1 个，是 `panic 0x01`。

本轮选择 `24259...` 的两个 `panic 0x41`。原因是这两个真实 IR 形状一致，语义明确：

```llvm
%len = call i256 @evm_sload(slot)
%limit = call i256 @evm_shl(i256 64, i256 1)
%ok = icmp ult i256 %len, %limit
br i1 %ok, label %success, label %panic_0x41

success:
  %newLen = add i256 %len, 1
  call void @evm_sstore(i256 slot, i256 %newLen)
```

这是 Solidity dynamic storage array push 的长度上限 guard：旧 length 必须小于 `2^64`，成功后同 slot 写回 `oldLength + 1`。

不做：

- 不处理 `0334...` 的 `panic 0x01`。它可能是 assert，当前不能证明是 bounds/compiler guard。
- 不泛化所有 `sload < 2^64`。必须看到同 slot `sstore(oldLength + 1)` 才 rewrite。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:167`
  - 新增 `isUInt64LimitValue`，只用于识别常量 `2^64` 或 IR 里的 `evm_shl(64, 1)`。
- `src/Passes/evm/SolidityPatterns.cpp:4837`
  - 新增 `findStorageArrayLengthIncrementStore`，确认 success block 里同一个 storage slot 写回 `oldLength + 1`。
- `src/Passes/evm/SolidityPatterns.cpp:4869`
  - 新增 `matchStorageArrayLengthBounds`，只匹配 `panic 0x41` 的 storage dynamic array length guard。
- `src/Passes/evm/SolidityPatterns.cpp:5458`
  - 在 checked-bounds 主流程里接入该 matcher。
- `src/Passes/evm/SolidityPatterns.cpp:5541`
  - 新增 marker `notdec_solidity_rewrite_storage_array_length_bounds`。
- `src/Passes/evm/SolidityPatterns.cpp:5582`、`src/Passes/evm/SolidityPatterns.cpp:5637`
  - 允许 marker 插入前 rematerialize `oldLength + 1`。
- `test/evm/solidity-patterns/manifest.json:4742`
  - 更新 `24259...` oracle：`storage_array_length_bounds` 2 个，skip 2 -> 0，CFG rewrite 10 -> 12。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- 单 case `24259...`：
  - 输出过 `llvm-as`。
  - `notdec_solidity_rewrite_storage_array_length_bounds`：0 -> 2。
  - checked-bounds skip：2 -> 0。
  - CFG rewrite：10 -> 12。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-storage-length`：90 passed, 0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-storage-length`：65 passed, 0 failed。
- fortune smoke：
  - `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-cb-storage-length.c --tr-level=2`
  - 通过，`elapsed=16.37`。

当前 manifest 剩余 checked-bounds skip：

- `0334_19494307_668d201319_1354ce2e324d`：`unrecognized_operands` 1 个，`panic 0x01`。

## 复杂度和维护判断

- 实现效果：8/10。消掉 pattern suite 中两个真实 `0x41` skip，并输出 semantic marker 和 CFG rewrite。
- 理解成本：6/10。新增一个窄 matcher，条件直接，但 marker rematerialize 复用了 checked arithmetic 的结果重建逻辑。
- 后期维护成本：低。规则绑定 `sload(slot)`、`sstore(slot, oldLength + 1)` 和 `panic 0x41`，不容易误吃普通业务分支。

更好的方案是后续有 storage array 高层恢复后，把这个 marker 接到更明确的 storage-array push 语义；当前只做 guard rewrite。

## 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. `0334...` 的 `panic 0x01` 单点。真实 case 1 个，语义不确定，风险中等偏高，测试成本低。先看 IR，不能直接 rewrite。
2. apehex batch612 的 `0x11/0x12`。真实 case 数量更多，语义需分类，rewrite 风险中等，测试成本中等。
3. 继续抽样最新 apehex batch，统计是否还有 `panic 0x41` 新形状。真实数量未知，语义要逐个确认，风险取决于形状，测试成本中等。

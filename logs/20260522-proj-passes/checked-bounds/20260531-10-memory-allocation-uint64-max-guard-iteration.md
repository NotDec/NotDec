# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，pattern suite 还有 143 个 checked-bounds skip，其中 panic `0x41` 有 77 个，是最多的一类。apehex batch589 还有 6 个 skip，其中 panic `0x41` 有 3 个。

本轮只看真实 IR 里稳定出现的 `0x41` compiler guard：

- pattern suite 多个真实合约里有 `length > (1 << 64) - 1`，success block 里紧跟 memory array allocation：`mload(0x40)`、`mstore(free_ptr, length)`、`length << 5`、`+ 32`。
- apehex batch589 有一个固定大小分配 guard：`free_ptr < 2^64 - 64`，success block 里写回 `free_ptr + 64`。

# 本轮目标

识别并 rewrite 两个保守形状：

- memory array allocation length guard：panic code 必须是 `0x41`，上界必须是 `uint64.max`，success block 必须看到同一 length 的数组内存分配。
- fixed-size free memory pointer guard：panic code 必须是 `0x41`，上界必须是 `2^64 - fixed_size`，`fixed_size` 限制为 32 对齐且不超过 4096，success block 必须写回 `free_ptr + fixed_size`。

# 不做

- 不处理 bytes/string rounding 里的多层 mask。
- 不处理不能证明 success block 在做内存分配的普通上限判断。
- 不把 `0x41` 业务分支当作 checked bounds rewrite。

# 判断标准

- pattern suite 的 panic `0x41` skip 下降。
- apehex batch589 的 panic `0x41` skip 至少下降一个。
- 新增 rewrite 必须输出已有 semantic helper，并实际改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

## 代码

- `src/Passes/evm/SolidityPatterns.cpp:159` 新增 `isUInt64Max`，识别常量 `2^64 - 1` 和真实 IR 里的 `evm_shl(64, 1) - 1`。
- `src/Passes/evm/SolidityPatterns.cpp:190` 新增 `matchUInt64LimitMinus`，只接受 `2^64 - fixed_size`，且 `fixed_size` 必须 32 对齐、不超过 4096。
- `src/Passes/evm/SolidityPatterns.cpp:1646` 扩展 `findMemoryAllocationShift`，同时接受 `evm_shl(5, length)` 和 LLVM `shl length, 5`。
- `src/Passes/evm/SolidityPatterns.cpp:2088` 新增 `hasMemoryArrayAllocationComputation`，要求 success block 里看到 `mstore(free_ptr, length)` 和 `length << 5 + 32`。
- `src/Passes/evm/SolidityPatterns.cpp:2247` 扩展 `matchMemoryAllocationBounds`，支持 `length > uint64.max`。
- `src/Passes/evm/SolidityPatterns.cpp:2272` 新增 `matchFixedMemoryAllocationPointerBounds`，匹配固定大小分配的 free memory pointer 上界检查。
- `src/Passes/evm/SolidityPatterns.cpp:2673` 对 success block 里才出现的 `newPtr`，在 marker 插入点重建 `oldPtr + size`。

## 测试和文档

- `test/evm/solidity-patterns/manifest.json:1910` 把 `1991_19510015_7f60edd6aa_6188d2031ddf` 升成 checked-bounds 强 oracle，检查 kind、panic code、semantic marker、skip reason 和 CFG rewrite。
- `docs/evm/entry-control/checked-bounds.md:73` 补充 `2^64 - 1` 和固定大小 free pointer guard 形状。

# 验证结果

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `git diff --check`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round11`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589：38 个 `.ll`，全部通过。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 round11 的 116 个输出：0 bad。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round11.c --tr-level=2`，通过，`elapsed=17.08`。

# 效果

pattern suite：

- skip 总数：143 -> 127。
- panic `0x41` skip：77 -> 61。
- `memory_allocation_bounds` marker 引用：16 -> 37。
- `memory_allocation_pointer_bounds` marker 引用：16 -> 18。

apehex batch589：

- skip 总数：6 -> 5。
- panic `0x41` skip：3 -> 2。
- `memory_allocation_pointer_bounds` marker 引用：8 -> 9。

# 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic `0x41` bytes/string rounding 资源检查：pattern suite 剩 61 个，apehex batch589 剩 2 个。数量仍最高，但形状里有多层 mask，必须先按真实 IR 聚类。
2. panic `0x11` checked arithmetic：pattern suite 剩 25 个，apehex batch589 剩 3 个。语义明确，需要继续按 add/sub/mul 的剩余形状拆开。
3. panic `0x33` enum conversion：pattern suite 剩 11 个。语义较明确，收益中等。
4. panic `0x49` empty pop：pattern suite 剩 11 个。语义明确，但需要确认真实 IR 是否都是编译器 array pop guard。

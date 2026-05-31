# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，pattern suite 仍有 183 个 checked-bounds skip，apehex batch589 仍有 18 个。

其中 panic 0x65 数量最高：

- pattern suite：93 个，分布在 13 个以上真实 case。
- apehex batch589：11 个。

抽样后，0x65 里最稳定的一类是动态内存分配长度 guard：

```llvm
%len = call i256 @evm_calldataload(...)
%ok = icmp ult i256 %len, 18446744073709551616
br i1 %ok, label %success, label %panic_0x41

success:
  %bytes = call i256 @evm_shl(i256 5, i256 %len)
  %rounded = and i256 (add i256 %bytes, 63), -32
```

这对应 Solidity 为动态数组 / bytes 分配内存前的资源检查：长度不能超过 `uint64.max` 级别，否则 panic 0x41。

# 本轮目标

识别并 rewrite 这类 memory allocation resource guard：

- panic code 必须是 0x41。
- 条件必须是 `length < 2^64` 的成功条件，或等价失败条件。
- success block 必须使用同一个 `length` 计算 `length << 5`，并继续出现 `+63`、`& -32` 的 32 字节对齐分配尺寸。
- marker 输出 `notdec_solidity_rewrite_memory_allocation_bounds(length)`。
- 找不到分配尺寸计算时继续 skip。

# 不做

- 不处理同一分配序列后面的 free-memory-pointer overflow guard。那是 `newPtr >= oldPtr && newPtr < 2^64` 的组合条件，放到下一轮。
- 不把 panic 0x65 伪装成 array bounds。它是 resource error，要用独立 helper。
- 不处理业务里的普通 `x < 2^64` 分支；必须同时看到 panic 0x41 和内存分配尺寸计算。

# 判断标准

- pattern suite 的 panic 0x65 skip 下降。
- apehex batch589 的 panic 0x65 skip 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_memory_allocation_bounds` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

本轮已完成。

改动：

- `src/Passes/evm/SolidityPatterns.cpp:149` 新增 `isUInt64Limit`，只识别当前 IR 中的 `2^64` 常量。
- `src/Passes/evm/SolidityPatterns.cpp:1582` 新增 `findMemoryAllocationShift`，要求 success block 用同一个 length 做 `evm_shl(5, length)`。
- `src/Passes/evm/SolidityPatterns.cpp:2016` 新增 `hasMemoryAllocationSizeComputation`，继续要求出现 `+63` 和 `& -32`，确认这是 32 字节对齐的内存分配尺寸计算。
- `src/Passes/evm/SolidityPatterns.cpp:2035` 新增 `matchMemoryAllocationBounds`，只在 panic 0x41 且满足上述分配形状时 rewrite。
- `src/Passes/evm/SolidityPatterns.cpp:2152` 将 matcher 接入 checked-bounds 流程。
- `src/Passes/evm/SolidityPatterns.cpp:2192` 新增对外 marker `notdec_solidity_rewrite_memory_allocation_bounds`。
- `test/evm/solidity-patterns/manifest.json:215` 更新小型 Solidity-generated array case 的 oracle，检查 memory allocation marker 和 CFG rewrite。
- `test/evm/solidity-patterns/manifest.json:3324` 更新真实 case `0147_19493380_262c2cb2b2_c24931e22d66` 的 oracle。
- `docs/evm/entry-control/checked-bounds.md:68` 补充 panic 0x41 动态内存分配资源检查说明。

不做：

- 不处理后续 free-memory-pointer overflow guard。那一段是组合条件，下一轮单独看。
- 不处理没有 `evm_shl(5, length)`、`+63`、`& -32` 的 `length < 2^64` 分支，避免误伤业务判断。

# 验证结果

命令：

```bash
cmake --build ./build --target all -j4
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null
python3 -m py_compile test/run_evm_solidity_patterns_suite.py
python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round8
ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round8.c --tr-level=2
```

结果：

- pattern suite：78 passed, 0 failed。
- ctest：`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 通过。
- fortune smoke：通过，`elapsed=16.44`。

效果：

- pattern suite skip 总数：183 -> 175。
- pattern suite panic 0x65 skip：93 -> 85。
- pattern suite `memory_allocation_bounds` marker：0 -> 16。
- apehex batch589 skip 总数：18 -> 14。
- apehex batch589 panic 0x65 skip：11 -> 7。
- apehex batch589 `memory_allocation_bounds` marker：0 -> 8。

# 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic 0x65 free-memory-pointer overflow guard：pattern suite 剩余 0x65 里大量是 `newPtr >= oldPtr && newPtr < 2^64`，apehex batch589 也剩 7 个。语义和本轮同属 memory allocation，确定性较高，但要处理 `and` 组合条件。
2. panic 0x34 storage encoding：pattern suite 25 个，apehex batch589 4 个。形状稳定，但要单独定义 storage byte array 解码 helper。
3. panic 0x11 masked-width arithmetic：pattern suite 25 个，apehex batch589 3 个。数量还可以，但现有 `checked_add` 没有位宽参数，不能直接复用。
4. panic 0x50 剩余 array bounds：pattern suite 2 个，apehex batch589 0 个。数量少，适合后面补尾。

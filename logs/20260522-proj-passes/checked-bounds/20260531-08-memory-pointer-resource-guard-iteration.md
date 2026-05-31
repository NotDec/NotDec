# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后，panic 0x65 仍是 pattern suite 和 apehex batch589 中数量最高的 checked-bounds skip。

本轮抽样后，剩余 0x65 里有一类稳定形状：

```llvm
%size = and (add (evm_shl 5, %length), 63), -32
%oldPtr = evm_mload(mem, 64)
%newPtr = add %oldPtr, %size
%no_wrap = icmp uge %newPtr, %oldPtr
%in_range = icmp ult %newPtr, 2^64
%ok = and i1 %no_wrap, %in_range
br i1 %ok, label %success, label %panic_0x41

success:
  evm_mstore(mem, 64, %newPtr)
```

这是 Solidity 动态内存分配时对 free memory pointer 的资源检查，不是普通业务分支。

数量：

- pattern suite：13 个。
- apehex batch589：5 个。

# 本轮目标

识别并 rewrite 这类 memory allocation pointer guard：

- panic code 必须是 0x41。
- branch 成功条件必须是两个比较的 `and`：
  - `newPtr >= oldPtr`
  - `newPtr < 2^64`
- success block 必须写回 `evm_mstore(mem, 64, newPtr)`。
- marker 输出 `notdec_solidity_rewrite_memory_allocation_pointer_bounds(oldPtr, size, newPtr)`。

# 不做

- 不处理不是 `and i1` 的其它 0x65。
- 不处理只检查 `mload(0x40) < 2^64 - 64` 的变体。
- 不处理 panic 0x34 storage encoding。

# 判断标准

- pattern suite 的 panic 0x65 skip 下降。
- apehex batch589 的 panic 0x65 skip 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_memory_allocation_pointer_bounds` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

本轮已完成。

改动：

- `src/Passes/evm/SolidityPatterns.cpp:159` 新增 `isMinus32`，用于识别内存分配对齐 mask。
- `src/Passes/evm/SolidityPatterns.cpp:2044` 新增 `isMemoryAllocationSize`，要求分配 size 来自 `and (add (evm_shl 5, length), 63), -32`。
- `src/Passes/evm/SolidityPatterns.cpp:2077` 新增 `isFreeMemoryPointerLoad`，要求 old pointer 来自 `evm_mload(mem, 64)`。
- `src/Passes/evm/SolidityPatterns.cpp:2083` 新增 `findFreeMemoryPointerStore`，要求 success block 写回 `evm_mstore(mem, 64, newPtr)`。
- `src/Passes/evm/SolidityPatterns.cpp:2138` 新增 `matchMemoryAllocationPointerBounds`，只匹配 panic 0x41 下 `newPtr >= oldPtr && newPtr < 2^64`。
- `src/Passes/evm/SolidityPatterns.cpp:2242` 在 checked-bounds 主流程中先匹配该 `and i1` 形状，避免被单 cmp normalizer 归成 unsupported。
- `src/Passes/evm/SolidityPatterns.cpp:2337` 新增对外 marker `notdec_solidity_rewrite_memory_allocation_pointer_bounds`。
- `test/evm/solidity-patterns/manifest.json:215` 更新 `checked_bounds_array_01` 的 oracle。
- `test/evm/solidity-patterns/manifest.json:3324` 更新真实 case `0147_19493380_262c2cb2b2_c24931e22d66` 的 oracle。
- `docs/evm/entry-control/checked-bounds.md:68` 补充 free memory pointer overflow 检查。

不做：

- 不处理 `mload(0x40) < 2^64 - 64` 这类预检查变体。
- 不处理没有写回 `evm_mstore(mem, 64, newPtr)` 的分支。

# 验证结果

命令：

```bash
cmake --build ./build --target all -j4
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null
python3 -m py_compile test/run_evm_solidity_patterns_suite.py
python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round9
ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure
/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round9.c --tr-level=2
```

结果：

- pattern suite：78 passed, 0 failed。
- ctest：`notdec.evm.solidity_patterns` 和 `notdec.evm.solidity_rewrite` 通过。
- fortune smoke：通过，`elapsed=16.57`。

效果：

- pattern suite skip 总数：175 -> 167。
- pattern suite panic 0x65 skip：85 -> 77。
- pattern suite `memory_allocation_pointer_bounds` marker：0 -> 16。
- apehex batch589 skip 总数：14 -> 10。
- apehex batch589 panic 0x65 skip：7 -> 3。
- apehex batch589 `memory_allocation_pointer_bounds` marker：0 -> 8。

# 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic 0x34 storage encoding：pattern suite 25 个，apehex batch589 4 个。形状稳定，下一轮应定义独立 storage encoding helper。
2. panic 0x11 masked-width arithmetic：pattern suite 25 个，apehex batch589 3 个。数量相近，但需要位宽参数或独立 helper，不能直接复用 256-bit checked add/mul。
3. panic 0x65 剩余 resource guard：pattern suite 77 个，apehex batch589 3 个。pattern 数量高，但真实批次剩余少，且很多是其它 resource 形状，先分类再动。
4. panic 0x50 剩余 array bounds：pattern suite 2 个，apehex batch589 0 个。数量少，适合后面补尾。

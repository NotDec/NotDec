# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后：

- pattern suite：127 个 skip，其中 panic `0x41` 有 61 个。
- apehex batch589：5 个 skip，其中 panic `0x41` 有 2 个。

真实 IR 里稳定出现一种 bytes/string 内存分配 pointer guard：

```llvm
%rounded1 = and (length + 31), <mask clearing low 5 bits>
%rounded2 = and (%rounded1 + 63), <mask clearing low 5 bits>
%new_ptr = free_ptr + %rounded2
%ok1 = icmp ult %new_ptr, 2^64
%ok2 = icmp uge %new_ptr, free_ptr
%ok = and %ok1, %ok2
br i1 %ok, label %success, label %panic_0x41
```

success block 写回 `mstore(0x40, new_ptr)`。这和 Solidity ABI bytes/string 分配形状一致，不是业务分支。

# 本轮目标

识别并 rewrite 这类 bytes/string allocation pointer guard：

- panic code 必须是 `0x41`。
- old pointer 必须来自 `mload(0x40)`。
- new pointer 必须是 `oldPtr + roundedSize`。
- roundedSize 必须来自 `length + 31`、mask 清低 5 位、再 `+ 63`、再 mask 清低 5 位。
- success block 必须写回 `mstore(0x40, newPtr)`。

# 不做

- 不处理前半段 `length < 2^64` guard。
- 不处理没有 free pointer 写回的普通上限判断。
- 不处理其它未聚类的 `0x41` 资源检查。

# 判断标准

- pattern suite 的 panic `0x41` skip 下降。
- apehex batch589 的 panic `0x41` skip 下降。
- 新增 rewrite 必须输出 `notdec_solidity_rewrite_memory_allocation_pointer_bounds` 并改 CFG。
- pattern suite / rewrite suite / `llvm-as` 检查通过。

# 实现记录

## 代码

- `src/Passes/evm/SolidityPatterns.cpp:2116` 新增 `isMaskClearingLowFiveBits`，识别清低 5 位的对齐 mask。
- `src/Passes/evm/SolidityPatterns.cpp:2125` 新增 `isRoundedByteLength`，识别 `length + 31` 后按 32 字节对齐。
- `src/Passes/evm/SolidityPatterns.cpp:2158` 新增 `isRoundedByteAllocationSize`，识别 bytes/string 分配里 `+31/mask/+63/mask` 的 rounded size。
- `src/Passes/evm/SolidityPatterns.cpp:2202` 新增 `hasMemoryBytesAllocationComputation`，要求 success block 写 `mstore(free_ptr, length)` 并写回 `mstore(0x40, newPtr)`。
- `src/Passes/evm/SolidityPatterns.cpp:2263` 扩展 `isMemoryAllocationSize`，让现有 `memory_allocation_pointer_bounds` matcher 能接受 bytes/string rounded size。

## 测试和文档

- `test/evm/solidity-patterns/manifest.json:2082` 把 `1775_19507435_1be1a16c5d_eb304133e7c9` 升成 checked-bounds 强 oracle，检查 kind、panic code、semantic marker、skip reason 和 CFG rewrite。
- `docs/evm/entry-control/checked-bounds.md:79` 补充 bytes/string payload rounded allocation 的 `0x41` pointer guard 形状。

# 验证结果

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `git diff --check`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round12`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589：38 个 `.ll`，全部通过。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 round12 的 116 个输出：0 bad。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round12.c --tr-level=2`，通过，`elapsed=16.59`。

# 效果

pattern suite：

- skip 总数：127 -> 122。
- panic `0x41` skip：61 -> 56。
- `memory_allocation_pointer_bounds` marker 引用：18 -> 27。

apehex batch589：

- skip 总数：5 -> 4。
- panic `0x41` skip：2 -> 1。
- `memory_allocation_pointer_bounds` marker 引用：9 -> 11。

# 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic `0x11` checked arithmetic：pattern suite 剩 25 个，apehex batch589 剩 3 个。apehex 剩余全是这类，下一轮优先聚类。
2. panic `0x41` 剩余资源检查：pattern suite 还有 56 个，apehex batch589 还有 1 个。数量最高，但剩余形状更杂，需要继续分类。
3. panic `0x33` enum conversion：pattern suite 剩 11 个。语义明确，apehex 当前没有。
4. panic `0x49` empty array pop：pattern suite 剩 11 个。语义明确，apehex 当前没有。

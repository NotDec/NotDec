# 原始 prompt

持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进。

# 背景

上一轮后：

- pattern suite：118 个 checked-bounds skip，其中 panic `0x41` 有 56 个。
- apehex batch589：3 个 skip，其中 panic `0x41` 有 1 个。

pattern suite 里有 bytes/string 动态长度分配的前置长度检查：

```llvm
%ok = icmp ult i256 %length, 18446744073709551616
br i1 %ok, label %success, label %panic_0x41
success:
  mstore(free_ptr, length)
  size = round_up(length + 31) + 32
  new_ptr = free_ptr + size
  mstore(0x40, new_ptr)
```

Solidity `arrayAllocationSizeFunction` 对 byte array/string 也有同类检查：`length > 0xffffffffffffffff` 时触发 `Panic(0x41)`，然后用 `roundUp(length)` 计算 size。

# 本轮目标

让现有 `memory_allocation_bounds` matcher 接受 bytes/string 动态分配长度检查：

- panic code 必须是 `0x41`。
- 条件必须是 `length < 2^64` / `length <= 2^64 - 1` 的反向失败。
- success block 必须能确认 bytes/string allocation：写 `mstore(free_ptr, length)`，计算 rounded byte size，并写回 `mstore(0x40, newPtr)`。
- rewrite 后输出 `notdec_solidity_rewrite_memory_allocation_bounds(length)` 并改 CFG。

# 不做

- 不匹配没有 free pointer 写回的普通业务长度判断。
- 不匹配 apehex 里那个没有直接 `mstore(free_ptr, length)` 的单例。
- 不处理其它 `0x41` 固定大小或组合条件 pointer guard。

# 判断标准

- pattern suite 的 panic `0x41` skip 下降。
- 至少一个真实 IR case 升成强 oracle，检查 kind、semantic marker、skip reason 和 CFG rewrite。
- pattern suite / rewrite suite / `llvm-as` / fortune smoke 通过。

# 实现记录

## 代码

- `src/Passes/evm/SolidityPatterns.cpp:2312` 调整 `hasMemoryAllocationSizeComputation`：没有 `length << 5` 时，允许已验证的 `hasMemoryBytesAllocationComputation` 形状。

## 测试和文档

- `test/evm/solidity-patterns/manifest.json:1684` 把 `0679_19497465_c2187cbc73_f22fac5262f8` 升成 checked-bounds 强 oracle，检查 kind、panic code、semantic marker、skip reason 和 CFG rewrite。
- `docs/evm/entry-control/checked-bounds.md:91` 补充 bytes/string 长度上限检查不依赖 `length * 32`。

# 验证结果

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `git diff --check`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round14`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589：38 个 `.ll`，全部通过，输出在 `/tmp/notdec-apehex-cb-589-round14`。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 round14 的 116 个输出：0 bad。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round14.c --tr-level=2`，通过，`elapsed=16.92`。

# 效果

pattern suite：

- skip 总数：118 -> 116。
- panic `0x41` skip：56 -> 54。
- `memory_allocation_bounds` marker：23 -> 25。

apehex batch589：

- skip 总数：3 -> 3。
- panic `0x41` skip：1 -> 1。
- 这轮没有命中 apehex 剩余单例，因为它缺少同一 success block 内的直接 length store，继续放宽风险偏高。

# 剩余候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. panic `0x41` 固定大小 / pointer guard：pattern suite 还有 54 个，apehex batch589 还有 1 个。数量最高，但形状分散，需要先拆出固定 size、动态 bytes size、无 length store 三类。
2. panic `0x11` 小位宽 add/sub/mul：pattern suite 还有 21 个，apehex batch589 还有 2 个。语义较清楚，但 helper 最好带位宽，不能直接伪装成 256-bit checked arithmetic。
3. panic `0x33` enum conversion：pattern suite 还有 11 个。语义明确，apehex 当前没有。
4. panic `0x49` empty array pop：pattern suite 还有 11 个。语义明确，apehex 当前没有。

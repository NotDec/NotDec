# CheckedBoundsPass fixed pointer strict upper guard iteration

## 本轮问题

round20 后统计：

- pattern suite：71 个 checked-bounds skip，其中 panic `0x41` 有 34 个。
- apehex batch589：2 个 skip，都是 panic `0x11`，没有 `0x41`。

`0x41` 里还有固定大小 free memory pointer bump guard：

```llvm
%too_high = icmp ugt i256 %old_ptr, 18446744073709551487
br i1 %too_high, label %panic, label %success

success:
  %new_ptr = add i256 %old_ptr, 128
  call void @evm_mstore(ptr %mem, i256 64, i256 %new_ptr)
```

这里 `old_ptr > 2^64 - size - 1` 等价于 `old_ptr + size` 不能超过 `2^64 - 1`。
真实 case 里 size 是 64、128、160，且 success block 都写回 free memory pointer。

Solidity 侧证据：

- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp:72` 到 `:83`：固定大小 allocation 会读取 free memory pointer，加上常量 size，再写回 free memory pointer。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256` 到 `:3265`：新 Yul helper 的 `finalizeAllocation` 也把 free pointer 上限和写回绑定在一起。

## 选择

本轮只扩展固定 size pointer guard 的 strict upper-bound 变体：

- panic code 必须是 `0x41`。
- 条件只接受 `old_ptr > 2^64 - size - 1` 或等价反向比较。
- size 必须是 `1..4096` 且 32-byte 对齐。
- success block 必须存在 `old_ptr + size`。
- success block 必须写回 `mstore(0x40, old_ptr + size)`。

不做：

- 不处理没有 free pointer 写回的业务上限。
- 不处理动态 length/resource guard。
- 不处理 apehex 剩余两个 `0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:211`
  - 新增 `matchUInt64LimitMinusStrictUpper`，把 `2^64 - size - 1` 解析成 fixed size。
- `src/Passes/evm/SolidityPatterns.cpp:2854`
  - 扩展 `matchFixedMemoryAllocationPointerBounds`，支持 strict `ugt/ult` 上限。
  - 原来的非 strict 路径继续要求 `old_ptr` 直接来自 `mload(0x40)`。
  - strict 路径仍要求 success block 写回 `mstore(0x40, new_ptr)`。
- `test/evm/solidity-patterns/manifest.json:1824`
  - 更新 `0651_19497235_18ca6cbfa7_017843325734` 的 kind、semantic marker、skip reason 和 CFG rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:2246`
  - 更新 `0648_19497230_998d150449_ea367f7fc1ac` 的 oracle。
- `docs/evm/entry-control/checked-bounds.md:102`
  - 补充 `free_ptr > 2^64 - fixed_size - 1` 形状。

## 效果

pattern suite：

- skip 总数：71 -> 65。
- panic `0x41` skip：34 -> 28。
- `notdec_solidity_rewrite_memory_allocation_pointer_bounds` marker 总数：35。
- 新增 rewrite：6 个，来自 `0648` 和 `0651` 的真实 IR case。

apehex batch589：

- 输出：38 个。
- bad logs：0。
- `llvm-as`：38/38 passed。
- skip 总数：2 -> 2，仍是两个 panic `0x11`。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round21`：78 passed。
- pattern round21 输出 `llvm-as`：78/78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round21：38 个输出，bad logs = 0。
- apehex batch589 round21 输出 `llvm-as`：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round21.c --tr-level=2`，通过，`elapsed=16.20`。

## 下一轮候选

1. 剩余 panic `0x41` length/resource guard
   - 真实 case 数量：pattern 28 个，apehex 0 个。
   - 语义确定性：中等。需要区分 `arrayAllocationSizeFunction`、ABI dynamic bytes copy 和业务长度上限。
   - rewrite 风险：中等。
   - 测试成本：中等。
2. panic `0x11` 常量 checked exponent 残留
   - 真实 case 数量：apehex 2 个，pattern 16 个。
   - 语义确定性：低到中。要确认 Solidity literal exponent codegen 和 `evm_shl(255, 1)` helper 语义。
   - rewrite 风险：中等。
   - 测试成本：中等。
3. panic `0x0` / `0x1` assert-like guard
   - 真实 case 数量：pattern 15 个，apehex 0 个。
   - 语义确定性：低到中。要区分 compiler assert、user assert 和不可达分支。
   - rewrite 风险：中等。
   - 测试成本：中等。
4. 剩余 panic `0x21` / `0x32`
   - 真实 case 数量：pattern 各 2 个，apehex 0 个。
   - 语义确定性：低到中。当前不是简单 enum conversion 或 array bounds。
   - rewrite 风险：中等。
   - 测试成本：中等。

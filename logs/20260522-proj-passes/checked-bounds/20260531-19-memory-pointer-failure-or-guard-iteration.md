# CheckedBoundsPass memory pointer failure-or guard iteration

## 本轮问题

round19 后统计：

- pattern suite：78 个 checked-bounds skip，其中 panic `0x41` 有 41 个。
- apehex batch589：2 个 skip，都是 panic `0x11`，没有 `0x41`。

`0x41` 里还有一类 Solidity `finalizeAllocation` 的 failure-or 形状：

```llvm
%size = and i256 (add i256 %length, 31), -32
%new = add i256 %old, %size
%too_large = icmp ugt i256 %new, 18446744073709551615
%wrapped = icmp ult i256 %new, %old
%fail = or i1 %too_large, %wrapped
br i1 %fail, label %panic, label %success
```

success block 里写回：

```llvm
call void @evm_mstore(ptr %mem, i256 64, i256 %new)
```

Solidity 侧证据：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256` 到 `:3265`：`finalizeAllocation` 计算 `newFreePtr = add(memPtr, roundUp(size))`，如果 `or(gt(newFreePtr, 0xffffffffffffffff), lt(newFreePtr, memPtr))` 就 panic，然后 `mstore(0x40, newFreePtr)`。

## 选择

本轮只扩展 failure-or free memory pointer guard：

- panic code 必须是 `0x41`。
- 条件必须是 bool `or`，且 true 分支进入 panic。
- range check 只接受 `new_ptr > 2^64 - 1` 或 `new_ptr >= 2^64`。
- no-wrap check 只接受 `new_ptr < old_ptr`。
- `new_ptr` 必须是 `old_ptr + size`。
- size 支持 Solidity `roundUp(size)` 的 `and(add(size, 31), -32)` 形状。
- success block 必须写回 `mstore(0x40, new_ptr)`。

不做：

- 不处理单独的 `length > 2^64 - 1`。
- 不处理没有 free pointer 写回的业务上限。
- 不处理 range/no-wrap 拆到非局部块的形状。
- 不处理 apehex 剩余两个 `0x11` 常量 exponent skip。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2489`
  - 新增 `isRoundedMemoryAllocationSize`，识别 `roundUp(size)` 的 `and(add(size, 31), -32)`。
- `src/Passes/evm/SolidityPatterns.cpp:2734`
  - `isSupportedMemoryAllocationSize` 接受 `isRoundedMemoryAllocationSize`。
- `src/Passes/evm/SolidityPatterns.cpp:2951`
  - `matchMemoryAllocationPointerBounds` 同时支持原来的 success-and 和新增 failure-or。
  - failure-or 路径按 Solidity `finalizeAllocation` 的 `or(gt(newFreePtr,max), lt(newFreePtr,memPtr))` 解析。
  - 原来的 success-and 路径继续要求 `old_ptr` 来自 `mload(0x40)`。
- `test/evm/solidity-patterns/manifest.json:1557`
  - 更新 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 的 kind、semantic marker、skip reason 和 CFG rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:1824`
  - 更新 `0651_19497235_18ca6cbfa7_017843325734` 的 oracle。
- `test/evm/solidity-patterns/manifest.json:2246`
  - 更新 `0648_19497230_998d150449_ea367f7fc1ac` 的 oracle。

`docs/evm/entry-control/checked-bounds.md` 已经记录 `if new_free_ptr > 2^64 - 1 or new_free_ptr < old_free_ptr: panic(0x41)`，本轮没有再改文档。

## 效果

pattern suite：

- skip 总数：78 -> 71。
- panic `0x41` skip：41 -> 34。
- `notdec_solidity_rewrite_memory_allocation_pointer_bounds` marker 总数：29。
- 新增 rewrite：7 个，来自 `0648`、`0651`、`2001` 等真实 IR case。

apehex batch589：

- 输出：38 个。
- bad logs：0。
- `llvm-as`：38/38 passed。
- skip 总数：2 -> 2，仍是两个 panic `0x11`。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round20`：78 passed。
- pattern round20 输出 `llvm-as`：78/78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round20：38 个输出，bad logs = 0。
- apehex batch589 round20 输出 `llvm-as`：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round20.c --tr-level=2`，通过，`elapsed=16.42`。

## 下一轮候选

1. 剩余 panic `0x41` length/resource guard
   - 真实 case 数量：pattern 34 个，apehex 0 个。
   - 语义确定性：中等。需要区分 `arrayAllocationSizeFunction`、ABI dynamic copy 和业务长度上限。
   - rewrite 风险：中等。
   - 测试成本：中等。
2. panic `0x11` 常量 checked exponent 残留
   - 真实 case 数量：apehex 2 个，pattern 16 个。
   - 语义确定性：低到中。需要先确认 Solidity literal exponent codegen 和 `evm_shl(255, 1)` helper 语义。
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

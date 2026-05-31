# CheckedBoundsPass bytes allocation split-store iteration

## 本轮问题

round14 统计：

- pattern suite：116 个 checked-bounds skip，其中 panic `0x41` 有 54 个。
- apehex batch589：3 个 skip，其中 panic `0x41` 有 1 个。

apehex 剩余 `0x41` case：

- `/tmp/notdec-apehex-cb-589-round14/23556_19746110_74ed3a5499_be4aad36a86c.out.ll`
- 形状是 bytes calldata allocation length guard：
  - `length < 2^64` 成功才继续。
  - success block 里计算 `roundUp(roundUp(length + 31) + 63)` 和 `newPtr = freePtr + size`。
  - 紧邻后继块里写 `mstore(0x40, newPtr)` 和 `mstore(freePtr, length)`。

之前 matcher 只接受 header store 和 free pointer store 都在同一个 success block，导致这个真实 compiler guard 留成
`notdec_solidity_rewrite_checked_bounds_skip(0x41, unrecognized_operands)`。

## 选择

本轮只扩展 bytes/string allocation length guard：

- 保留 panic code `0x41` 和 `length <= uint64.max`/`length < 2^64` 条件要求。
- 继续要求 free memory pointer、rounded bytes size、`mstore(0x40, newPtr)`、`mstore(freePtr, length)` 都出现。
- 允许两个 store 落在 success block 的紧邻后继块，用来覆盖中间插入 free pointer overflow guard 的形状。

不做：

- 不放宽所有 `0x41` 上限检查。
- 不处理小整数算术、enum、array pop。
- 不接受没有 length header store 的 case。

Solidity 依据仍是 `libsolidity/codegen/YulUtilFunctions.cpp` 的 `arrayAllocationSizeFunction`：byte array/string 对 length 做 `uint64.max` 检查，随后按 bytes payload round up 分配。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2202`
  - 新增 `isRoundedByteAllocationSizeForLength`，把嵌套 bytes allocation size 和当前 length 绑定起来，避免只看形状不看数据来源。
- `src/Passes/evm/SolidityPatterns.cpp:2271`
  - 新增 `hasBytesAllocationStores`，要求同一块里同时看到 `mstore(freePtr, length)` 和 `mstore(0x40, newPtr)`。
- `src/Passes/evm/SolidityPatterns.cpp:2297`
  - 新增 `hasBytesAllocationStoresOnLocalPath`，只查看 success block 和它的直接后继。
- `src/Passes/evm/SolidityPatterns.cpp:2361`
  - 调整 `hasMemoryBytesAllocationComputation`，从 success block 找 free pointer load，再确认 new pointer、bytes size 和本地 store path。
- `test/evm/solidity-patterns/manifest.json:2118`
  - 更新 `1775_19507435_1be1a16c5d_eb304133e7c9` 强 oracle：新增 1 个 `memory_allocation_bounds` semantic marker，CFG rewrite 8 -> 9，`unrecognized_operands` 7 -> 6。
- `docs/evm/entry-control/checked-bounds.md:94`
  - 补充 bytes/string header store 可能落在紧邻后继块。

## 效果

pattern suite：

- skip 总数：116 -> 111。
- panic `0x41` skip：54 -> 49。
- `memory_allocation_bounds` marker：25 -> 30。

apehex batch589：

- 输出：38 个。
- skip 总数：3 -> 2。
- panic `0x41` skip：1 -> 0。
- 剩余 skip 都是 panic `0x11`。
- `memory_allocation_bounds` marker：4 -> 5。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round15`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round15：38 个输出，bad logs = 0。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 apehex round15 输出：38/38 passed。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null`：通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`：通过。
- `git diff --check`：通过。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round15.c --tr-level=2`，通过，`elapsed=16.41`。

## 下一轮候选

1. panic `0x41` fixed-size / pointer guard
   - 真实 case 数量：pattern 还有 49 个 `0x41` skip，apehex batch589 暂无。
   - 语义确定性：中等。要先拆分 fixed-size free pointer bump、helper-returned size、业务上限。
   - rewrite 风险：中等，不能把普通 memory bound 当 allocation。
   - 测试成本：中等。
2. panic `0x11` 小整数 add/sub/mul guard
   - 真实 case 数量：pattern 21 个，apehex 2 个。
   - 语义确定性：中等。需要确认 helper 表达的是小位宽 checked arithmetic，不要误标成 256-bit。
   - rewrite 风险：中等偏高。
   - 测试成本：中等。
3. panic `0x21` enum conversion
   - 真实 case 数量：pattern 11 个，apehex 0 个。
   - 语义确定性：较高。
   - rewrite 风险：低到中。
   - 测试成本：低。
4. panic `0x31` empty array pop
   - 真实 case 数量：pattern 11 个，apehex 0 个。
   - 语义确定性：较高。
   - rewrite 风险：低到中。
   - 测试成本：低。

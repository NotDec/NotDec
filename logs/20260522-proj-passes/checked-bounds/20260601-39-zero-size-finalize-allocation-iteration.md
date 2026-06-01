# Checked bounds zero-size finalize allocation

## 问题

上一轮后 pattern suite 还有 8 个 `checked_bounds_skip`：

- `0648_19497230_998d150449_ea367f7fc1ac`: 3 个，`{65: 3}`
- `0651_19497235_18ca6cbfa7_017843325734`: 3 个，`{65: 3}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 1 个，`{50: 1}`

本轮先看数量最高的 panic `0x41`。其中 `0648...` 和 `0651...` 各有一个相同形状：

```llvm
%evm.gt30 = icmp ugt i256 %evm.mload19, 18446744073709551615
br i1 %evm.gt30, label %panic, label %success

success:
  call void @evm_mstore(ptr %mem, i256 64, i256 %evm.mload19)
```

对照 Solidity：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256`
  `finalizeAllocationFunction` 计算 `newFreePtr := add(memPtr, roundUp(size))`，
  然后检查 `gt(newFreePtr, uint64max)` / `lt(newFreePtr, memPtr)`，最后
  `mstore(64, newFreePtr)`。
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:1296`
  有 `finalizeAllocation(freeMemoryPre, 0)`。

当 size 是 0 时，`newFreePtr == memPtr`，`lt(newFreePtr, memPtr)` 会被简化掉，
剩下单独的 `memPtr > uint64max` guard。

## 本轮选择

选择这个 zero-size finalizeAllocation 退化形状，因为：

- 两个真实 pattern case 命中同一形状；
- 有 Solidity codegen 直接对应；
- success block 里必须有 `mstore(64, checkedPtr)`，不是只按 panic code 放宽；
- 能输出已有 `memory_allocation_pointer_bounds` semantic marker 并改 CFG。

不做：

- 不把所有 `panic 0x41` 的 `x > uint64max` 都 rewrite。
- 不处理剩余 bytes/string storage 写入相关的 4 个 `0x41` skip。
- 不处理 batch589 的两个 `0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3940` 新增
  `matchZeroSizeMemoryAllocationPointerBounds`：
  - 只接受 panic `0x41`；
  - 只接受 `ptr > uint64max` / `ptr >= 2**64` 及其等价反向比较；
  - 要求 success block 中有 `evm_mstore(64, ptr)`；
  - 输出 `memory_allocation_pointer_bounds(ptr, 0, ptr)`。
- `src/Passes/evm/SolidityPatterns.cpp:4327` 在已有 fixed pointer matcher 后调用该
  matcher。
- `test/evm/solidity-patterns/manifest.json:1886` 更新 `0651...` oracle：
  `memory_allocation_pointer_bounds 4 -> 5`，`panic_resource_error 3 -> 2`，
  skip `3 -> 2`，CFG rewrite `27 -> 28`。
- `test/evm/solidity-patterns/manifest.json:2384` 同步更新 `0648...` oracle。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，
  168.62s。
- pattern skip：8 -> 6。
- `0648...` 和 `0651...` 各新增 1 个 CFG rewrite。
- apehex batch589 同口径重跑：38 个输出，bad logs 0，`llvm-as` 38/38 passed，
  剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，
  17.84s。上一轮同口径是 16.55s。

复杂度评分：

- 实现效果：3/10。减少 2 个 pattern skip，范围小但语义明确。
- 理解成本：2/10。复用已有 `memory_allocation_pointer_bounds`，只补 size 0 退化形状。
- 维护成本：2/10。成功条件绑定 `mstore(64, ptr)`，不影响其他 panic `0x41`。

## 剩余 skip 和下一轮

pattern suite 当前剩余 6 个 skip：

- `0648_19497230_998d150449_ea367f7fc1ac`: 2 个，`{65: 2}`
- `0651_19497235_18ca6cbfa7_017843325734`: 2 个，`{65: 2}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 1 个，`{50: 1}`

batch589 当前剩余 2 个 skip：

- `23523_19745765_952a215d44_d3d3b55f272b`: 1 个，`{17: 1}`
- `23545_19746035_86a085849a_4d774e840428`: 1 个，`{17: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x41` 剩余 4 个：`0648...`、`0651...` 各 2 个。数量最高，
   但现在看起来是 bytes/string 写入 storage 的长度 guard，需要继续对照 Solidity
   storage byte array copy 形状。
2. batch589 panic `0x11`：两个真实 case。真实数据价值高，但当前像常量退化 guard。
3. pattern panic `0x32`：`2001...` 一个。需要证明 private helper 返回值和数组
   index/length 的关系。
4. pattern panic `0x1`：`0334...` 一个。需要确认是 Solidity compiler assert
   还是业务 assert。

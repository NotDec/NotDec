# Checked bounds preincrement bytes allocation

## 问题

上一轮后 pattern suite 还有 48 个 `checked_bounds_skip`，其中 panic `0x41`
还有 17 个。继续看 `0x41` 后，发现 3 个真实 case 有同一种 bytes allocation
形状：

- `0189_19493609_3c0627c9e0_9d16fec0a1c2`
- `1847_19507643_148f68d102_07c7230a7676`
- `1988_19508992_89095f0aa3_688f45aabbde`

IR 形状是：

- guard 检查 `base + 1 <= uint64.max`；
- 成功路径 `mstore(oldPtr, base + 1)`，把它作为动态 bytes/string 的内存长度；
- free pointer 用 `oldPtr + ((base & -32) + 64)` 更新。

Solidity `arrayAllocationSizeFunction` 对 bytes/string 的逻辑是
`roundUp(length) + 0x20`。当 `length = base + 1` 时，当前 IR 会把它化成
`(base & -32) + 64`。这仍然是 compiler memory allocation guard，不是业务分支。

## 本轮选择

只补这个 preincrement bytes allocation size 形状。它仍然必须经过已有的
`hasMemoryBytesAllocationComputation` 路径，也就是同时看到：

- allocation size 来自当前 guard 的 `length`；
- 成功路径写入 bytes/string header length；
- 成功路径更新 free memory pointer。

不做：

- 不把任意 `base + 1 <= uint64.max` 当成 allocation guard；
- 不处理没有 header store/free-pointer store 的 `0x41`；
- 不处理剩余 `0x41` 里 returndata、calldata copy 等还没对清楚的形状；
- 不处理 apehex 剩余两个 `0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2448` 新增
  `isPreIncrementByteAllocationSize`，只接受 `length = base + 1` 和
  `size = (base & -32) + 64`。
- `src/Passes/evm/SolidityPatterns.cpp:2584` 的 `isDynamicAllocationSize`
  接入这个 bytes allocation size 形状。
- `test/evm/solidity-patterns/manifest.json:1647` 给
  `0189_19493609_3c0627c9e0_9d16fec0a1c2` 增加强 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:2448` 给
  `1988_19508992_89095f0aa3_688f45aabbde` 增加强 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:2590` 更新
  `1847_19507643_148f68d102_07c7230a7676` oracle：`memory_allocation_bounds`
  `1 -> 2`，`panic_resource_error` `2 -> 1`，semantic marker `1 -> 2`，
  `unrecognized_operands` `2 -> 1`，CFG rewrite `12 -> 13`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：48 -> 45。
- pattern panic `0x41` skip：17 -> 14。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，170.89s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.99s。

复杂度评分：

- 实现效果：5/10。减少 3 个真实 pattern skip，并补强了两个原本较弱的 oracle。
- 理解成本：4/10。需要知道 `roundUp(base + 1) + 32` 的简化结果。
- 维护成本：3/10。仍挂在 header store 和 free pointer store 证明下，误伤面小。

## 剩余 skip 和下一轮

pattern suite 当前剩余 45 个 skip：

- panic `0x41`: 14
- panic `0x0`: 14
- panic `0x11`: 10
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 剩余 panic `0x41`：数量仍最高。优先看
   `0334_19494307_668d201319_1354ce2e324d` 的连续 calldata-copy allocation
   guard，以及 `0340/1572` 这类 storage-to-memory copy helper。
2. apehex batch589 剩余两个 panic `0x11`：真实数据集命中，但还要回原始 IR
   或 Solidity codegen 确认 signed increment 形状。
3. pattern panic `0x0` / `0x1`：共 15 个，数量高，但要先区分 compiler assert
   和 user assert。
4. pattern 剩余 panic `0x21` / `0x32`：数量少，语义较明确，可以后处理。

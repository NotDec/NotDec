# Checked bounds allocation helper length guard

## 问题

上一轮后 pattern suite 还有 65 个 `checked_bounds_skip`，其中 panic `0x41` 还有 28 个。apehex batch589 只剩 2 个 skip，都是 panic `0x11`，所以本轮优先看 pattern suite 里数量更高、语义更清楚的 `0x41`。

对照 Solidity 源码：

- `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp` 里的 `abiDecodingFunctionArrayAvailableLength` / `abiDecodingFunctionByteArrayAvailableLength`
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp` 里的 `arrayAllocationSizeFunction`

真实 IR 形状是：`length > 0xffffffffffffffff` 失败走 panic `0x41`，成功路径计算 `roundUp(length)+32` 或 `length*32+32`，再调用 `private__...` 分配 helper，helper 返回指针后写 `mstore(ptr, length)`。

## 本轮选择

只支持 ABI decoder / allocation helper 形状的 length guard。判断条件是同时看到：

- 成功路径里有返回 `i256` 的 `private__...` helper 调用；
- helper 参数里有由同一个 `length` 算出的动态数组分配大小；
- helper 返回值马上作为动态数组头写入 `evm_mstore(ptr, length)`。

不做：

- 不把普通 `private__...` 调用都当成内存分配；
- 不处理业务自己的长度上限；
- 不处理还没确认 Solidity codegen 的剩余 panic `0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:133` 新增 `isPrivateHelperCall`，识别 Solidity 私有 helper。
- `src/Passes/evm/SolidityPatterns.cpp:2432` 新增 `isArrayAllocationSize`，识别 `length * 32 + 32`。
- `src/Passes/evm/SolidityPatterns.cpp:2532` 新增 `isDynamicAllocationSize`，复用 bytes / rounded bytes / array 三种动态分配大小。
- `src/Passes/evm/SolidityPatterns.cpp:2629` 新增 `hasAllocationHelperHeaderStore`，要求 helper 返回指针被写入动态数组 length header。
- `src/Passes/evm/SolidityPatterns.cpp:2648` 新增 `hasMemoryAllocationHelperCall`，把 helper call、size 参数和 header store 绑定起来。
- `src/Passes/evm/SolidityPatterns.cpp:2739` 让 bytes allocation 路径复用 `isDynamicAllocationSize`。
- `src/Passes/evm/SolidityPatterns.cpp:2755` 和 `src/Passes/evm/SolidityPatterns.cpp:2769` 把 allocation helper 证明接入 memory allocation bounds matcher。
- `test/evm/solidity-patterns/manifest.json:1557` 更新 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` oracle：`memory_allocation_bounds` 3 -> 6，`panic_resource_error` 4 -> 1，semantic marker 3 -> 6，`unrecognized_operands` 7 -> 4，CFG rewrite 29 -> 32。

## 效果

- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：65 -> 59。
- pattern panic `0x41` skip：28 -> 22。
- 新增 rewrite case：
  - `0189_19493609_3c0627c9e0_9d16fec0a1c2`: +1
  - `0258_19493864_78edd9f88e_b098925cbd2d`: +1
  - `0334_19494307_668d201319_1354ce2e324d`: +1
  - `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: +3
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.34s。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed。

复杂度评分：

- 实现效果：8/10。真实 case 上减少 6 个 `0x41` skip，apehex 没有回退。
- 理解成本：6/10。增加了 helper call + header store 绑定，逻辑比原本多，但条件集中在 allocation matcher 附近。
- 维护成本：6/10。`private__` 识别比较宽，所以必须继续保留 size 参数和 header store 两个约束。

## 剩余 skip 和下一轮

pattern suite 当前剩余 59 个 skip：

- panic `0x41`: 22
- panic `0x11`: 16
- panic `0x0`: 14
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. 剩余 panic `0x41` length/resource guard：pattern 22 个，apehex 0 个；数量最高，但要继续区分 ABI dynamic copy、storage bytes copy 和业务长度上限。
2. panic `0x11` 常量 checked exponent：pattern 16 个，apehex 2 个；真实数据集也命中，需要先确认 Solidity literal exponent codegen 和 `evm_shl(255, 1)` helper 语义。
3. panic `0x0` / `0x1` assert-like guard：pattern 15 个，apehex 0 个；可能是 compiler assert，也可能接近 user assert，不能直接放宽。
4. panic `0x21` / `0x32` 剩余少量 case：pattern 各 2 个，apehex 0 个；数量低，适合在高收益项之后处理。

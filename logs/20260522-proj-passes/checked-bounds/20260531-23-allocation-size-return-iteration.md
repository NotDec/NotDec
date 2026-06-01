# Checked bounds allocation size return

## 问题

上一轮后 pattern suite 还有 53 个 `checked_bounds_skip`，其中 panic `0x41`
有 22 个，是数量最多的一类。重新看输出后，部分 `0x41` 不是直接更新 free
memory pointer，而是在 helper 函数里只返回 allocation size：

- `0258_19493864_78edd9f88e_b098925cbd2d`：`length * 32 + 32` 后
  `ret`。
- `0334_19494307_668d201319_1354ce2e324d`：`roundUp(length) + 32`
  后 `ret`。
- `0648_19497230_998d150449_ea367f7fc1ac`：同类 helper-return 形状。

Solidity 源码 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2362`
的 `arrayAllocationSizeFunction` 正是这个 codegen：先检查
`gt(length, 0xffffffffffffffff)`，失败走 `Panic(0x41)`，成功路径只返回
`size`。所以这是 compiler allocation guard，不是业务分支。

## 本轮选择

这轮只补 helper-return 形状：如果 success block 的 terminator 是 `ret`，且返回值能被已有
`isDynamicAllocationSize` 证明是当前 `length` 的 allocation size，就允许 rewrite。

不做：

- 不处理没有 size 计算证据的普通 `Panic(0x41)`；
- 不把任意 `gt(length, uint64.max)` 都当成 allocation guard；
- 不处理 apehex 剩余两个 panic `0x11`，那两个当前还没确认到稳定的 Solidity codegen；
- 不做 metadata-only 输出，只有能改 CFG 才算本轮命中。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2687` 新增
  `hasMemoryAllocationSizeReturn`，只接受单返回值 helper，并复用
  `isDynamicAllocationSize` 验证返回值。
- `src/Passes/evm/SolidityPatterns.cpp:2780` 的
  `hasMemoryAllocationSizeComputation` 接入 helper-return 检查。
- `test/evm/solidity-patterns/manifest.json:2246` 更新
  `0648_19497230_998d150449_ea367f7fc1ac` oracle：`panic_resource_error`
  `1 -> 0`，`unrecognized_operands` `9 -> 8`，CFG rewrite `21 -> 22`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：53 -> 48。
- pattern panic `0x41` skip：22 -> 17。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，170.78s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，仍然都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.52s。

复杂度评分：

- 实现效果：6/10。pattern suite 减少 5 个 `0x41` skip，真实 batch589 没变化。
- 理解成本：3/10。只是在已有 allocation size 证明上多接受 `ret size` 这个出口。
- 维护成本：3/10。约束是 `ret` + 当前 `length` 派生出的动态 allocation size，误伤面小。

## 剩余 skip 和下一轮

pattern suite 当前剩余 48 个 skip：

- panic `0x41`: 17
- panic `0x0`: 14
- panic `0x11`: 10
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 剩余 panic `0x41`：还有 17 个，数量最高。下一轮先看
   `0334_19494307_668d201319_1354ce2e324d`、`0189_19493609_3c0627c9e0_9d16fec0a1c2`
   和 `1847_19507643_148f68d102_07c7230a7676`，区分 allocation/copy guard
   和业务长度上限。
2. apehex batch589 剩余两个 panic `0x11`：真实数据集命中，收益直接，但当前 pass 输出里操作数已被折成常量比较，要先回到原始 IR 或 Solidity codegen 确认。
3. pattern panic `0x0` / `0x1`：共 15 个，数量高，但需要区分 compiler assert、user assert 和 unreachable。
4. pattern 剩余 panic `0x21` / `0x32`：数量少，语义较明确，可以等高收益项后处理。

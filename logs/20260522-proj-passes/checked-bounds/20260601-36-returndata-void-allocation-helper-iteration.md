# Checked bounds returndata void allocation helper

## 问题

上一轮后 pattern suite 还有 18 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x32`: 2
- panic `0x41`: 1

唯一的 panic `0x41` 在 `0651_19497235_18ca6cbfa7_017843325734`：

- `returndatasize > uint64max` 时 panic `0x41`
- success block 里读取 free memory pointer
- 计算 `roundUp(returndatasize) + 32`
- 调用 void private helper 更新 allocation pointer
- `mstore(oldPtr, returndatasize)` 写 bytes header

这是 returndata bytes allocation 的 resource guard。Solidity 里相关形状：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256`
  `finalizeAllocationFunction` 对 new free pointer 做 `uint64max` 和 nowrap 检查。
- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp:122`
  `returnDataToArray` 使用 `returndatasize()` 作为 bytes length，写入 header。

## 本轮选择

选择这个 `0x41` case，因为：

- 它是 pattern suite 里最后一个 panic `0x41` skip；
- panic code 和后续 bytes allocation 行为能对上；
- 已有 `memory_allocation_bounds` helper 可复用；
- rewrite 后能改 CFG，不是 metadata-only。

不做：

- 不处理 `2001...` 的 panic `0x32`。它们仍需要 memory length load 无覆盖证明。
- 不把任意 void private helper 当 allocation helper。
- 不处理 `0x0` / `0x1`，因为要先分类 assert/unreachable/业务分支。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2997` 前置声明 `callHasArg`。
- `src/Passes/evm/SolidityPatterns.cpp:3350` 新增
  `hasVoidMemoryAllocationHelperCall`：
  - success block 必须有 `mstore(headerPtr, length)`；
  - 同一个 block 必须有 void private helper call；
  - helper 参数必须包含 `headerPtr`；
  - helper 参数还必须包含由同一个 `length` 算出的 dynamic allocation size。
- `src/Passes/evm/SolidityPatterns.cpp:3483` 和 `:3500` 让
  `hasMemoryAllocationSizeComputation` 接受这个 void helper 证明。
- `test/evm/solidity-patterns/manifest.json:1842` 更新 `0651...` oracle：
  `panic_resource_error -> memory_allocation_bounds`，skip `8 -> 7`，CFG rewrite
  `22 -> 23`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：18 -> 17。
- pattern panic `0x41` skip：1 -> 0。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.61s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.53s。

复杂度评分：

- 实现效果：3/10。减少 1 个 pattern skip，但清掉了 pattern suite 的 panic `0x41`。
- 理解成本：3/10。新增证明只看同一个 success block 的 header store 和 void helper。
- 维护成本：3/10。后续若出现跨 block allocation helper，要另加证据，不能扩大搜索范围。

## 剩余 skip 和下一轮

pattern suite 当前剩余 17 个 skip：

- `0648_19497230_998d150449_ea367f7fc1ac`: 7 个，`{0: 7}`
- `0651_19497235_18ca6cbfa7_017843325734`: 7 个，`{0: 7}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 2 个，`{50: 2}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：`2001...` 两个真实 case。可能是 array bounds，但需要证明
   memory length load 没被覆盖。
2. batch589 panic `0x11`：`23523...` 和 `23545...` 各一个。真实数据价值高，
   但当前像常量退化 guard。
3. pattern panic `0x0`：`0648...`、`0651...` 共 14 个。数量最高，但需要先分类
   assert/unreachable/业务分支。
4. pattern panic `0x1`：`0334...` 一个 assert，需要看是否是 Solidity compiler
   assert，而不是业务检查。

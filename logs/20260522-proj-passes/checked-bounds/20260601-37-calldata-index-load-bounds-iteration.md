# Checked bounds calldata index load

## 问题

上一轮后 pattern suite 还有 17 个 `checked_bounds_skip`：

- panic `0x0`: 14
- panic `0x1`: 1
- panic `0x32`: 2

两个 panic `0x32` 都在 `2001_19510128_72aa4f70b3_e9ee7c9c0e79`。其中第一个
guard 是：

- `index < length` 成功，否则 panic `0x32`
- success block 里马上计算 `index << 5`
- 用 `base + (index << 5)` 做 `evm_calldataload`

这和 Solidity calldata array index access 对得上：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2493`
  `calldataArrayIndexAccessFunction` 先检查 `index < length`，再用
  `base_ref + index * stride` 取地址。
- `/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2419`
  index access 后会对这个 calldata 地址做 read/calldataload。

第二个 `0x32` 只有 private helper 返回值和输入长度比较，success block 没有直接
calldata/memory/storage load 证据，本轮不处理。

## 本轮选择

选择第一个 `0x32`，因为：

- panic code 是 array out of bounds；
- success block 里能看到同一个 index 参与 calldata element offset；
- 可复用已有 `array_bounds_calldata` helper；
- rewrite 后能改 CFG，不是 metadata-only。

不做：

- 不处理第二个 `0x32`，因为缺少直接 element load 证据。
- 不把任意 `index < length` 都标成 calldata bounds。
- 不做跨 block 搜索，只看当前 success block。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2811` 新增
  `isCalldataArrayIndexScale`，识别 `index << 5`、`evm_shl(5, index)`、
  `index * 32`。
- `src/Passes/evm/SolidityPatterns.cpp:2835` 新增
  `isCalldataArrayElementOffset`，识别 `base + scaledIndex`。
- `src/Passes/evm/SolidityPatterns.cpp:2848` 新增
  `hasCalldataArrayElementLoad`，要求 success block 里有 `evm_calldataload` 使用
  这个 offset。
- `src/Passes/evm/SolidityPatterns.cpp:2865` 扩展 `matchArrayBounds`，当 length
  不是 mload/calldataload/sload 时，只有满足上面的 success block 证据才标成
  `array_bounds_calldata`。
- `src/Passes/evm/SolidityPatterns.cpp:4190` 调用 `matchArrayBounds` 时传入
  success block。
- `test/evm/solidity-patterns/manifest.json:1512` 更新 `2001...` oracle：
  新增一个 `array_bounds_calldata`，`panic_array_out_of_bounds 2 -> 1`，skip
  `2 -> 1`，CFG rewrite `35 -> 36`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：17 -> 16。
- pattern panic `0x32` skip：2 -> 1。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，168.53s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.32s。

复杂度评分：

- 实现效果：3/10。减少 1 个 pattern skip，并只剩一个 pattern `0x32`。
- 理解成本：3/10。新增 helper 只检查 calldata element load 的局部形状。
- 维护成本：3/10。后续如果支持更多 stride 或跨 block 形状，需要继续补真实证据。

## 剩余 skip 和下一轮

pattern suite 当前剩余 16 个 skip：

- `0648_19497230_998d150449_ea367f7fc1ac`: 7 个，`{0: 7}`
- `0651_19497235_18ca6cbfa7_017843325734`: 7 个，`{0: 7}`
- `2001_19510128_72aa4f70b3_e9ee7c9c0e79`: 1 个，`{50: 1}`
- `0334_19494307_668d201319_1354ce2e324d`: 1 个，`{1: 1}`

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. 剩余 pattern panic `0x32`：`2001...` 一个。需要继续分析 private helper
   返回值是否可证明为有效 index。
2. batch589 panic `0x11`：`23523...` 和 `23545...` 各一个。真实数据价值高，
   但当前像常量退化 guard。
3. pattern panic `0x0`：`0648...`、`0651...` 共 14 个。数量最高，但要先分类
   assert/unreachable/业务分支。
4. pattern panic `0x1`：`0334...` 一个 assert，需要确认是否是 compiler assert。

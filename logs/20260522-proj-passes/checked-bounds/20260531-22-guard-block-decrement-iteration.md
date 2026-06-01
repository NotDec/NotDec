# Checked bounds guard-block decrement

## 问题

上一轮后 pattern suite 还有 59 个 `checked_bounds_skip`，其中 panic `0x11` 有 16 个。apehex batch589 还有 2 个 `0x11`，但那两个是 `evm_shl(255, 1)` 后和常量比较，暂时没有确认到 Solidity 的固定 codegen。

这轮先处理 pattern suite 里更明确的 6 个 `0x11`：`0648_19497230_998d150449_ea367f7fc1ac` 和 `0651_19497235_18ca6cbfa7_017843325734` 各 3 个。IR 形状是：

- 先算 `add value, -1`；
- 再判断 `value == 0`；
- 失败块是 `Panic(0x11)`；
- 成功路径使用减一后的值。

Solidity 源码 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4174` 的 `decrementCheckedFunction` 对应这个语义：`if eq(value, minval) panic; ret := sub(value, 1)`。这里的 uint 场景 `minval` 是 0。

## 本轮选择

只扩展 checked increment/decrement 的结果查找位置：原来只在 success block 找 `value +/- 1`，现在如果没找到，再到 guard block 找同一个 `value` 和同一个固定步长。

不做：

- 不处理 apehex 里 `evm_shl(255, 1)` 的两个 `0x11`，因为还没确认 Solidity codegen 来源；
- 不放宽到任意 panic `0x11`；
- 不处理 signed add/sub 的复杂布尔组合。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2002` 新增 `findCheckedStepResult`：先查 success block，再查 guard block，只接受同一个输入和固定 `+1` / `-1`。
- `src/Passes/evm/SolidityPatterns.cpp:2097` 的 checked increment matcher 改用 `findCheckedStepResult`。
- `src/Passes/evm/SolidityPatterns.cpp:2115` 的 checked decrement matcher 改用 `findCheckedStepResult`。
- `test/evm/solidity-patterns/manifest.json:1824` 更新 `0651_19497235_18ca6cbfa7_017843325734` oracle：`checked_sub` 6 -> 9，`panic_checked_arithmetic` 3 -> 0，semantic marker 6 -> 9，`unrecognized_operands` 12 -> 9，CFG rewrite 18 -> 21。
- `test/evm/solidity-patterns/manifest.json:2246` 更新 `0648_19497230_998d150449_ea367f7fc1ac` 同样的 oracle。

## 效果

- build：通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：59 -> 53。
- pattern panic `0x11` skip：16 -> 10。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.72s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，仍然都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.90s。

复杂度评分：

- 实现效果：7/10。真实 pattern case 上减少 6 个 skip，范围很小。
- 理解成本：4/10。只是把已有 `value +/- 1` 查找补到 guard block。
- 维护成本：4/10。约束仍然是 panic `0x11`、`eq(value, 0/-1)` 和固定步长，后续风险低。

## 剩余 skip 和下一轮

pattern suite 当前剩余 53 个 skip：

- panic `0x41`: 22
- panic `0x0`: 14
- panic `0x11`: 10
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. apehex 剩余两个 panic `0x11` 的 `evm_shl(255, 1)` guard：真实数据集命中，但要先确认 Solidity codegen 来源，不能只按常量条件消掉。
2. pattern 剩余 panic `0x41` length/resource guard：pattern 22 个，数量最高；需要继续区分 compiler allocation/copy guard 和业务长度上限。
3. pattern panic `0x0` / `0x1` assert-like guard：共 15 个；要区分 compiler assert、user assert 和 unreachable。
4. pattern 剩余 panic `0x21` / `0x32`：数量少，可以等高收益项后处理。

# Checked bounds div cleanup operand rematerialize

## 问题

上一轮后 pattern suite 还有 24 个 `checked_bounds_skip`。其中
`0258_19493864_78edd9f88e_b098925cbd2d` 还有一个 panic `0x12`
`operand_not_available_at_guard`。

真实 IR 里这个 guard 已经被识别成 `checked_div`：

- divisor 是 `%evm.and = and (2^128 - 1), 604800`；
- guard 检查 `%evm.and == 0`，失败分支是 `panic(0x12)`；
- success block 里先计算 `%evm.and1 = and arg0, (2^128 - 1)`，再做
  `evm_div(%evm.and1, %evm.and)`。

问题不是语义不清楚，而是 semantic marker 需要 dividend，`%evm.and1` 在
success block 里，不支配 guard，所以之前降级成 skip。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:850`
的 checked div/mod helper 会先检查 divisor，再执行 div/mod。这里的 dividend
cleanup 是 guard 后的纯 `and`，可以在 guard 前重建。

## 本轮选择

只允许 `checked_div` / `checked_mod` 的 marker operand 重建 `and` 表达式，并且
要求 `and` 的两个输入都支配 guard。这样不会放宽 matcher，也不会重建有副作用或
依赖 success block 状态的值。

不做：

- 不重建任意算术表达式；
- 不处理 `0x32` loop/index guard；
- 不处理 signed arithmetic；
- 不把缺少 operand 的 rewrite 当作 metadata-only。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3921` 新增
  `checkedBoundsValueAvailableAtBranch`。
- `src/Passes/evm/SolidityPatterns.cpp:3927` 新增
  `canRematerializeCheckedBoundsOperand`，当前只接受 `and`，且两个输入必须可在
  branch 使用。
- `src/Passes/evm/SolidityPatterns.cpp:3963` 允许 `checked_div` / `checked_mod`
  的这种 operand 通过 dominance 检查。
- `src/Passes/evm/SolidityPatterns.cpp:3994` 在插入 semantic marker 前，用
  `IRBuilder` 在 guard 前重建这个 `and`。
- `test/evm/solidity-patterns/manifest.json:1679` 更新 `0258...` oracle：
  `notdec_solidity_rewrite_checked_div` `1 -> 2`，skip 清零，CFG rewrite
  `52 -> 53`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：24 -> 23。
- pattern panic `0x12` skip：1 -> 0。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，172.47s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，17.29s。

复杂度评分：

- 实现效果：3/10。只减少 1 个 pattern skip，但消掉了最后一个 `0x12` skip。
- 理解成本：3/10。重建逻辑只服务 checked div/mod 的纯 `and` operand。
- 维护成本：3/10。后续如果要支持别的表达式，必须逐类加证明，不能扩大成通用重建。

## 剩余 skip 和下一轮

pattern suite 当前剩余 23 个 skip：

- panic `0x0`: 14
- panic `0x11`: 3
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern panic `0x32`：剩 2 个，都是 `2001...` 的 loop/index array bounds。
   下一步如果做，需要证明 memory length 从 `mstore(base, len)` 到 `mload(base)`
   没被覆盖。
2. pattern 剩余 panic `0x11`：还剩 3 个，主要是 `0334` signed arithmetic 和
   `1991` packed-field 加法上界。
3. batch589 两个 panic `0x11`：真实数据仍剩两个，需要看更早 IR 或 helper 结构。
4. pattern panic `0x0` / `0x1`：数量最高，但要先分类 assert 来源。

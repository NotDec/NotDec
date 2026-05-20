# Evm2llvm 语义提升路线计划

## 原始 prompt

当前evm2llvm项目，当前bug已经修得差不多了。/sn640/NotDecChainExp/ 那边已经跑了很多结果出来。找一个有源码的结果，然后尝试分析一下源码和当前LLVM IR的对应关系，然后规划一下后一步的语义提升路线。主要是找出要把哪些底层模式给识别为更高层的语义

## 背景

`/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch010/summary.csv`
里 40 个样本都已经 `ok`。这说明当前重点可以从“能不能生成可验证 IR”转到“IR 能不能表达更接近 Solidity 的语义”。

这次选的样例是：

- 样本：`0394_19494617_261e203d6f_433b0912f7c8`
- 输出 IR：`/sn640/NotDecChainExp/apehex_evm_contracts/notdec-runs/20260520-evm2llvm-train-batch010/outputs/0394_19494617_261e203d6f_433b0912f7c8.ll`
- 源码数据：`/sn640/NotDecChainExp/apehex_evm_contracts/hex/ethereum/train/19494000_to_19494999.parquet`
- 合约地址：`261e203d6f794d29a77e77d168358f4a82caf18f`
- runtime 大小：298 字节

这个样例是一个很小的 ERC1967/OpenZeppelin Proxy 派生合约，适合看源码和 IR 的对应关系。

## 源码对应关系

源码关键部分：

- `contracts/UCCM.sol`
  - `contract UCCM is ERC721Creator`
  - 构造函数只传入 name/symbol，runtime 里基本看不到构造期逻辑。
- `contracts/manifold/ERC721Creator.sol`
  - `_IMPLEMENTATION_SLOT = 0x360894a13ba1a3210667c828492db98dca3e2076cc3735a920a3ca505d382bbc`
  - `implementation()` 返回 `_implementation()`
  - `_implementation()` 读 `StorageSlot.getAddressSlot(_IMPLEMENTATION_SLOT).value`
- `node_modules/@openzeppelin/contracts/proxy/Proxy.sol`
  - `fallback()` / `receive()` 调 `_fallback()`
  - `_fallback()` 调 `_delegate(_implementation())`
  - `_delegate()` 是 inline assembly：`calldatacopy`、`delegatecall`、`returndatacopy`、失败 `revert`、成功 `return`

当前 IR 的对应关系：

- `public_implementation___0x31`
  - 先检查 `callvalue == 0`，不满足就 `revert(0, 0)`。
  - 调 `private__0x97_0x97` 读实现地址。
  - 把地址按 160 bit mask 后写入 ABI 返回区，再 `evm_return` 32 字节。
- `private__0x97_0x97`
  - `evm_sload(0x360894...2bbc)`
  - `(1 << 160) - 1`
  - `and` 取低 160 bit
  - 返回 implementation address。
- `public___function_selector___0x0` 和 `public__0xeeeeeeee_0xf44`
  - 对未知 selector、fallback、receive 走代理逻辑。
  - 模式是读 implementation slot，复制 calldata，`delegatecall(gas, impl, 0, calldatasize, 0, 0)`，复制 returndata，然后按 delegatecall 结果 `revert` 或 `return`。

这里能明确看到：底层 IR 已经有足够信息把它恢复成“ERC1967 proxy + implementation getter + fallback delegate”。

## 先要修的硬问题

当前 IR 里多个条件分支是 `br i1 true`，例如 selector dispatch、`callvalue == 0`、delegatecall 成败分支。

看 Gigahorse facts 后，这不是源码语义，而是 lowering 取错了 `JUMPI` 条件：

- `TAC_Use.csv` 里 `JUMPI 0xb` 的两个 use 是 `0x9` 和 `0x8`。
- `0x9` 是跳转目标，`0x8` 才是 `calldatasize < 4` 的条件。
- `LlvmLowerer.cpp` 当前用 `terminal->Uses[0]` 当条件，所以常量目标被降成了非零，最后变成 `br i1 true`。

这个问题不先修，后面的 selector、require、delegatecall result 识别都会建立在错误 CFG 上。它应该作为语义提升前的第一个修复点。

## 后续要识别的高层模式

### 1. 函数选择器和 ABI 入口

底层模式：

- `calldatasize < 4`
- `calldataload(0) >> 224`
- 和常量 selector 比较
- 跳到 public function 或 fallback

提升目标：

- 识别 dispatcher。
- 把 `0x5c60da1b` 标成 `implementation()`。
- 把 `0xeeeeeeee` 这种 Gigahorse fallback/receive 占位名和真实 fallback/receive 分清楚。

### 2. payable / nonpayable 检查

底层模式：

- `callvalue()`
- `iszero`
- 条件不满足 `revert(0, 0)`

提升目标：

- 在 public function 上标 `nonpayable`。
- 把这段从普通业务逻辑里折叠掉，不要显示成手写 if。

### 3. ABI 返回值编码

底层模式：

- `mload(0x40)`
- `mstore(retptr, value)`
- `return(retptr, 0x20)`
- 地址返回前有 `and ((1 << 160) - 1)`

提升目标：

- 识别成 `returns (address)`。
- 把 160 bit mask 标成 address 类型转换，不要当普通位运算输出。

### 4. ERC1967 implementation slot

底层模式：

- `sload(0x360894a13ba1a3210667c828492db98dca3e2076cc3735a920a3ca505d382bbc)`
- 低 160 bit mask

提升目标：

- 标出 `ERC1967 implementation slot`。
- 恢复成 `_implementation()` / `implementation()` 这种“读实现地址”的语义。
- 后续如果接源码 storage layout，可以把 `StorageSlot.getAddressSlot(...).value` 对上。

### 5. Proxy fallback delegate

底层模式：

- `calldatacopy(0, 0, calldatasize())`
- `delegatecall(gas(), implementation, 0, calldatasize(), 0, 0)`
- `returndatacopy(0, 0, returndatasize())`
- `delegatecall == 0 ? revert(0, returndatasize()) : return(0, returndatasize())`

提升目标：

- 折叠成 `fallback/receive delegates to implementation`。
- 保留“失败冒泡 revert、成功返回 returndata”的语义。
- 这类模式很常见，优先级比复杂业务逻辑更高。

### 6. EVM runtime helper 分层

底层模式：

- 当前所有环境、内存、storage、call 都是 `evm_*` helper。

提升目标：

- 不急着消灭所有 helper。
- 第一阶段只识别稳定大模式：selector、ABI return、storage slot、delegate proxy、require。
- 不稳定或依赖宽类型推理的 helper 先保留，避免把 IR 改得看似高级但语义错。

## 技术路线

第一步先修 `JUMPI` 条件 use 顺序，并用这个样例验证 selector、nonpayable、delegatecall 成败 CFG 都变成真实条件分支。

第二步加一个只读分析 pass 或输出侧 matcher，不急着改核心 lowering 结构。输入是当前 LLVM IR 加 `!notdec.evm` metadata，输出先可以是注释、JSON 或命名增强，用来验证模式识别是否稳定。

第三步按上面的优先级补 matcher：

1. dispatcher / selector matcher
2. nonpayable require matcher
3. ABI return matcher
4. ERC1967 slot matcher
5. proxy delegate matcher

第四步再考虑把 matcher 结果接到 C 输出或更高层伪代码。不要一开始就把 LLVM IR 改成很重的自定义 intrinsic，否则容易影响现有 verifier 和后续优化。

## 风险

- `JUMPI` use 顺序可能在不同 facts 里不完全一致，修复时要先确认 Gigahorse 对 `JUMPI` 的 operand 约定，而不是只针对这个样例写死。
- Proxy 模式有很多变体，例如透明代理、UUPS、minimal proxy、beacon proxy。第一阶段只做 ERC1967 direct implementation slot + delegatecall 冒泡模式。
- ABI matcher 不能只靠常量 32 字节判断类型，地址、uint256、bool 都可能是 32 字节返回。地址可以先靠 160 bit mask 辅助判断。
- 源码只能用于评估，不应作为 lowering 的必要输入。真实反编译路线仍要从 bytecode/IR 自己识别。

## 判断标准

- 修复 `JUMPI` 后，这个样例里 selector、`callvalue == 0`、delegatecall result 三类分支不再是 `br i1 true`。
- matcher 能在这个样例上输出：
  - `implementation() -> address`
  - `sload(ERC1967_IMPLEMENTATION_SLOT)`
  - `fallback/receive delegatecall implementation`
  - `revert/return returndata bubbling`
- 在 batch010 里抽 5 个同类小代理样本，matcher 不误报普通合约。
- 不引入 fortune 路线性能回归；如果只改 `external/NotDec-evm2llvm`，说明不跑 fortune 的理由。如果后续接入主 NotDec pass pipeline，再按项目规范跑 fortune 对比。

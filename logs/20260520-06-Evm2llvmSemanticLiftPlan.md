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

## 源码和 IR 对应关系

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

这份 runtime 里最主要的 Solidity 语义其实只有三件事：

1. 外部有人调用合约时，先看 calldata 前 4 字节是不是某个公开函数 selector。
2. 如果 selector 是 `implementation()`，返回当前代理指向的实现合约地址。
3. 其他调用走 proxy fallback：把原始 calldata 原样转发给 implementation，并把对方返回或 revert 的数据原样冒泡给调用者。

这里能明确看到：底层 IR 已经有足够信息把它恢复成“ERC1967 proxy + implementation getter + fallback delegate”。

### 基本块级对应关系

#### `public___function_selector___0x0`

这是合约 runtime 的主入口，负责分发公开函数和 fallback/receive。

- `bb._0x0`
  - IR：`mstore(0x40, 0x80)`，然后取 `calldatasize`，判断 `calldatasize < 4`。
  - 含义：Solidity ABI 里，普通函数调用的 calldata 前 4 字节是函数 selector。如果 calldata 少于 4 字节，说明它不可能携带完整 selector，通常要走 `receive()` 或 `fallback()`。
  - 来源：这是 Solidity 编译器为所有有外部入口的合约生成的 dispatcher 逻辑，不是业务代码。
  - 分支：
    - true 到 `bb._0x1f`：calldata 太短，没有完整 selector。
    - false 到 `bb._0xc`：calldata 至少 4 字节，可以读 selector。

- `bb._0x1f`
  - IR：再次取 `calldatasize`，判断是否非零。
  - 含义：区分 `fallback()` 和 `receive()`。Solidity 里空 calldata 且带 ETH 时可以进 `receive()`；非空但无法匹配 selector 时进 `fallback()`。这个合约的两条路径最终都 delegate 到 implementation，所以 IR 里后面会合流。
  - 分支：
    - true 到 `bb._0x2b`：calldata 非空，走 fallback delegate。
    - false 到 `bb._0xf04`：calldata 为空，走 receive/fallback 占位函数 `public__0xeeeeeeee_0xf44`。

- `bb._0xf04`
  - IR：调用 `public__0xeeeeeeee_0xf44` 后返回。
  - 含义：Gigahorse 给 receive/fallback 入口起了一个占位 selector 名 `0xeeeeeeee`。源码里对应 `Proxy.receive()` 或空 calldata fallback，最终还是 `_fallback()`。

- `bb._0xc`
  - IR：`calldataload(0) >> 224`，和 `1549851163` 比较。
  - 含义：`calldataload(0)` 会读 calldata 开头 32 字节；selector 只占最高 4 字节，所以右移 224 bit 后得到 4 字节 selector。`1549851163` 是十进制，十六进制是 `0x5c60da1b`，对应 `implementation()`。
  - 分支：
    - true 到 `bb._0x1904`：调用 `implementation()`。
    - false 到 `bb._0x1c`：未知 selector，走 fallback delegate。

- `bb._0x1904`
  - IR：调用 `public_implementation___0x31` 后返回。
  - 含义：这是源码里的 `implementation() public view returns (address)`。

- `bb._0x1c`
  - IR：无条件跳到 `bb._0x2b`。
  - 含义：未知 selector 统一走 fallback。

- `bb._0x2b`
  - IR：跳到 `bb._0x5f0x0`。
  - 含义：fallback delegate 路径的入口。这里没有新语义，只是编译器/Gigahorse 切出来的中间块。

- `bb._0x5f0x0`
  - IR：`sload(0x360894...2bbc)`，然后 `(1 << 160) - 1`，再 `and`。
  - 含义：从 ERC1967 implementation slot 读实现合约地址。EVM storage word 是 256 bit，address 只有 160 bit，所以用 mask 取低 160 bit。
  - 源码对应：`StorageSlot.getAddressSlot(_IMPLEMENTATION_SLOT).value`。

- `bb._0x910x0`
  - IR：只跳到 `bb._0xd10x0`。
  - 含义：中间块，无额外 Solidity 语义。

- `bb._0xd10x0`
  - IR：`calldatacopy(0, 0, calldatasize())`，`delegatecall(gas(), impl, 0, calldatasize(), 0, 0)`，`returndatacopy(0, 0, returndatasize())`，再判断 `delegatecall == 0`。
  - 含义：这是 OpenZeppelin `Proxy._delegate()` 的核心。它把调用者原始输入复制到内存，用 `delegatecall` 调 implementation。`delegatecall` 的特点是执行 implementation 的代码，但保留当前合约的 storage、msg.sender、msg.value，所以代理合约能把逻辑放在 implementation，状态留在 proxy。
  - 分支：
    - true 到 `bb._0xef0x0`：delegatecall 失败。
    - false 到 `bb._0xeb0x0`：delegatecall 成功。

- `bb._0xef0x0`
  - IR：`revert(0, returndatasize())`。
  - 含义：把 implementation 的失败原因原样返回给外部调用者。Solidity 源码里是 assembly 的 `case 0 { revert(0, returndatasize()) }`。

- `bb._0xeb0x0`
  - IR：`return(0, returndatasize())`。
  - 含义：把 implementation 的返回值原样返回给外部调用者。源码里是 assembly 的 `default { return(0, returndatasize()) }`。

#### `public_implementation___0x31`

这是公开函数 `implementation()`。

- `bb._0x31`
  - IR：读取 `callvalue()`，判断是否为 0。
  - 含义：`implementation()` 不是 payable。Solidity 会给 nonpayable public function 自动生成检查：如果调用时带 ETH，就 `revert(0, 0)`。
  - 分支：
    - true 到 `bb._0x3c`：没有带 ETH，可以继续执行。
    - false 到 `bb._0x38`：带了 ETH，revert。

- `bb._0x38`
  - IR：`revert(0, 0)`。
  - 含义：nonpayable 检查失败。这个不是业务逻辑里的 `require`，而是 Solidity 编译器自动加的入口保护。

- `bb._0x3c`
  - IR：调用 `private__0x97_0x97`。
  - 含义：执行内部 `_implementation()`，拿到 implementation address。

- `bb._0x43`
  - IR：读 free memory pointer `mload(0x40)`，把地址 mask 到 160 bit，`mstore` 到返回区，然后 `return(retptr, 32)`。
  - 含义：ABI 编码返回值。Solidity ABI 返回一个 `address` 时也是占 32 字节，只是有效值在低 160 bit。

#### `private__0x97_0x97`

这是内部函数 `_implementation()`。

- `bb._0x97`
  - IR：`sload(0x360894...2bbc)`，再取低 160 bit。
  - 含义：读 ERC1967 implementation slot。这个固定 slot 是 `keccak256("eip1967.proxy.implementation") - 1`，OpenZeppelin 代理都用它存 implementation 地址。

- `bb._0xc9`
  - IR：返回刚才读出的地址。
  - 含义：对应源码 `return StorageSlot.getAddressSlot(_IMPLEMENTATION_SLOT).value;`。

#### `public__0xeeeeeeee_0xf44`

这是 receive/fallback 占位入口，主体和 `public___function_selector___0x0` 的 fallback delegate 路径重复。

- `bb._0xf44`
  - IR：跳到 `bb._0x5f0xf44`。
  - 含义：receive/fallback 入口。

- `bb._0x5f0xf44`
  - IR：读 ERC1967 implementation slot 并取低 160 bit。
  - 含义：拿到代理要转发到的 implementation 地址。

- `bb._0x910xf44`
  - IR：中间跳转块。
  - 含义：无额外 Solidity 语义。

- `bb._0xd10xf44`
  - IR：复制 calldata，执行 `delegatecall`，复制 returndata，判断成功失败。
  - 含义：`Proxy._delegate()`。

- `bb._0xef0xf44`
  - IR：`revert(0, returndatasize())`。
  - 含义：delegatecall 失败时冒泡失败数据。

- `bb._0xeb0xf44`
  - IR：`return(0, returndatasize())`。
  - 含义：delegatecall 成功时冒泡返回数据。

## 已修复的前置问题

旧输出里多个条件分支是 `br i1 true`，例如 selector dispatch、`callvalue == 0`、delegatecall 成败分支。

看 Gigahorse facts 后，这不是源码语义，而是 lowering 取错了 `JUMPI` 条件：

- `TAC_Use.csv` 里 `JUMPI 0xb` 的两个 use 是 `0x9` 和 `0x8`。
- `0x9` 是跳转目标，`0x8` 才是 `calldatasize < 4` 的条件。
- 旧版 `LlvmLowerer.cpp` 用 `terminal->Uses[0]` 当条件，所以常量目标被降成了非零，最后变成 `br i1 true`。

这个问题现在已经修掉：`JUMPI` 严格要求两个 use，按 EVM operand 顺序使用 `Uses[1]` 作为 condition。更新后的 0394 输出里 selector、nonpayable、delegatecall result 都已经是 `%evm.branch.cond`，不再是 `br i1 true`。

## 后续要识别的高层模式

### 1. 函数选择器和 ABI 入口

底层模式：

- `calldatasize < 4`
- `calldataload(0) >> 224`
- 和常量 selector 比较
- 跳到 public function 或 fallback

为什么会有这个模式：

- 以太坊调用合约时，输入数据叫 calldata。
- Solidity ABI 规定：调用普通 public/external 函数时，calldata 前 4 字节是函数签名 hash 的前 4 字节，也就是 selector。
- 如果 `calldatasize < 4`，说明连完整 selector 都没有，编译器不能拿它去匹配普通函数，只能走 `receive()` 或 `fallback()`。
- 如果 calldata 至少 4 字节，编译器会读取前 32 字节，再右移 224 bit，只留下最前面的 4 字节 selector。
- 这个样例里只匹配一个 selector：`0x5c60da1b`，也就是 `implementation()`。其他 selector 都进 fallback delegate。

提升目标：

- 识别 dispatcher。
- 把 `0x5c60da1b` 标成 `implementation()`。
- 把 `0xeeeeeeee` 这种 Gigahorse fallback/receive 占位名和真实 fallback/receive 分清楚。

### 2. payable / nonpayable 检查

底层模式：

- `callvalue()`
- `iszero`
- 条件不满足 `revert(0, 0)`

为什么会有这个模式：

- `callvalue()` 是这次调用附带的 ETH 数量。
- Solidity 里没有 `payable` 的 public/external 函数默认不能收 ETH。
- 编译器会在函数入口自动插入 `msg.value == 0` 检查。
- 这个样例的 `implementation()` 是 view 函数，不 payable，所以 IR 里有这段检查。

提升目标：

- 在 public function 上标 `nonpayable`。
- 把这段从普通业务逻辑里折叠掉，不要显示成手写 if。

### 3. ABI 返回值编码

底层模式：

- `mload(0x40)`
- `mstore(retptr, value)`
- `return(retptr, 0x20)`
- 地址返回前有 `and ((1 << 160) - 1)`

为什么会有这个模式：

- Solidity ABI 返回值也放在内存里。
- `0x40` 位置保存 free memory pointer，编译器用它找一块可写返回区。
- ABI 里 `address` 返回值占 32 字节，但地址本身只有 160 bit。
- 所以编译器常用 160 bit mask 保证高位清零，再 `mstore` 到 32 字节返回槽。

提升目标：

- 识别成 `returns (address)`。
- 把 160 bit mask 标成 address 类型转换，不要当普通位运算输出。

### 4. ERC1967 implementation slot

底层模式：

- `sload(0x360894a13ba1a3210667c828492db98dca3e2076cc3735a920a3ca505d382bbc)`
- 低 160 bit mask

为什么会有这个模式：

- 代理合约需要知道真正执行逻辑的 implementation 地址。
- ERC1967 规定 implementation 地址存在固定 storage slot：
  `0x360894a13ba1a3210667c828492db98dca3e2076cc3735a920a3ca505d382bbc`。
- `sload(slot)` 从合约 storage 读 256 bit word。
- address 只占 160 bit，所以读出来后再取低 160 bit。

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

为什么会有这个模式：

- fallback 的任务是：当前合约自己不处理这个 selector，而是转给 implementation。
- `calldatacopy` 把外部调用的原始参数复制到内存。
- `delegatecall` 用当前合约的 storage 上下文执行 implementation 代码，这是 proxy 的核心。
- implementation 执行完后，返回数据或错误数据会放到 returndata 缓冲区。
- `returndatacopy` 把这份数据复制出来。
- 如果 `delegatecall` 返回 0，表示失败，proxy 要 `revert` 并带上原始错误数据。
- 如果非 0，表示成功，proxy 要 `return` 并带上原始返回数据。

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

前置条件已经完成：`JUMPI` 条件 use 顺序已修，并用这个样例验证 selector、nonpayable、delegatecall 成败 CFG 都变成真实条件分支。

第一步加一个只读分析 pass 或输出侧 matcher，不急着改核心 lowering 结构。输入是当前 LLVM IR 加 `!notdec.evm` metadata，输出先可以是注释、JSON 或命名增强，用来验证模式识别是否稳定。

第二步按上面的优先级补 matcher：

1. dispatcher / selector matcher
2. nonpayable require matcher
3. ABI return matcher
4. ERC1967 slot matcher
5. proxy delegate matcher

第三步再考虑把 matcher 结果接到 C 输出或更高层伪代码。不要一开始就把 LLVM IR 改成很重的自定义 intrinsic，否则容易影响现有 verifier 和后续优化。

## 风险

- `JUMPI` 已按当前 facts 的 operand 顺序处理：`Uses[0]` 是 destination，`Uses[1]` 是 condition。后续如果 Gigahorse 改导出约定，需要同步更新这里。
- Proxy 模式有很多变体，例如透明代理、UUPS、minimal proxy、beacon proxy。第一阶段只做 ERC1967 direct implementation slot + delegatecall 冒泡模式。
- ABI matcher 不能只靠常量 32 字节判断类型，地址、uint256、bool 都可能是 32 字节返回。地址可以先靠 160 bit mask 辅助判断。
- 源码只能用于评估，不应作为 lowering 的必要输入。真实反编译路线仍要从 bytecode/IR 自己识别。

## 判断标准

- 这个样例里 selector、`callvalue == 0`、delegatecall result 三类分支不再是 `br i1 true`。
- matcher 能在这个样例上输出：
  - `implementation() -> address`
  - `sload(ERC1967_IMPLEMENTATION_SLOT)`
  - `fallback/receive delegatecall implementation`
  - `revert/return returndata bubbling`
- 在 batch010 里抽 5 个同类小代理样本，matcher 不误报普通合约。
- 不引入 fortune 路线性能回归；如果只改 `external/NotDec-evm2llvm`，说明不跑 fortune 的理由。如果后续接入主 NotDec pass pipeline，再按项目规范跑 fortune 对比。

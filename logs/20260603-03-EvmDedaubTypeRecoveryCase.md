# 原始需求

写到一个markdown文件里，开头介绍这个合约地址之类的信息，然后介绍dedaub的结果哪里还有提升空间，然后介绍一下我们当前怎么做的，把已有的IR拿过来，然后对着说明一下后面进一步恢复成带有高级语义的IR是什么样的

# apehex EVM 合约样例和 Dedaub 对照

## 1. 合约信息

本例来自 apehex EVM 数据集，适合用来展示 EVM 类型恢复和 memory 语义提升。

- 样例名：`29602_19821256_5e91c6dc71_27b2e3d99239`
- 合约地址：`0x5e91c6dc71f23b3c752ae4d2cf5ce2ac11c8fbe6`
- 区块号：`19821256`
- 交易 hash：`0x27b2e3d99239e305c1733a8a80e94851bade9837f630be0b8c2f084833c9e39f`
- bytecode 大小：`2729` bytes
- apehex manifest：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260603-evm2llvm-train-batch741/manifest.csv:3`
- 本地 bytecode：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260603-evm2llvm-train-batch741/inputs/29602_19821256_5e91c6dc71_27b2e3d99239.hex`
- evm2llvm IR：`/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260603-evm2llvm-train-batch741/outputs/29602_19821256_5e91c6dc71_27b2e3d99239.ll`
- Dedaub 输出：`/tmp/resule.sol`

这个合约主体接近一个 ERC20。Dedaub 已经识别出 `totalSupply`、`balanceOf`、`transfer`、`transferFrom`、`approve`、`allowance`、`name`、`symbol`、`decimals` 等常见接口。另外还有一个自定义函数 `Execute(address[])`，以及一个 `interference()` getter。

## 2. Dedaub 结果还有哪些提升空间

### 2.1 `0x40` free memory pointer 基本已经识别

原始 IR 里大量出现 Solidity free memory pointer 的读写：

```llvm
call void @evm_mstore(ptr %mem, i256 64, i256 128)
%evm.mload = call i256 @evm_mload(ptr %mem, i256 64)
call void @evm_mstore(ptr %mem, i256 64, i256 %evm.add)
```

Dedaub 输出里大部分这种 `mload(0x40)` / `mstore(0x40, ...)` 已经不再原样出现，而是被提升成：

```solidity
v3 = new bytes[](v0);
v4 = v5 = v3.data;
v0 = new address[](_bytes_.length);
v1 = v2 = v0.data;
```

这说明 Dedaub 已经识别了 free memory pointer 和一部分 ABI memory array 布局。剩下的低层痕迹主要是数据搬运：

```solidity
MEM[v4] = STORAGE[v9];
MEM[v1] = msg.data[v3];
MEM[0] = address(v0[v5]);
MEM[32] = 0;
```

所以这里不能说 Dedaub 完全没恢复 memory 指针。更准确的说法是：它已经恢复了 `0x40` 分配指针，但还没有完全把 memory 数据搬运提升成 `string`、`bytes`、`address[] calldata` 和 mapping key 构造这些高层语义。

### 2.2 `name` / `symbol` 没恢复成 `string`

Dedaub 当前把 storage 字符串识别成了数组：

```solidity
uint256[] _name;   // STORAGE[0x2]
uint256[] _symbol; // STORAGE[0x3]
```

`symbol()` 的输出仍然保留了 storage bytes 编码细节：

```solidity
v0 = v1 = _symbol.length >> 1;
if (!(_symbol.length & 0x1)) {
    v0 = v2 = v1 & 0x7f;
}
v3 = new bytes[](v0);
v4 = v5 = v3.data;
...
MEM[v4] = STORAGE[v9];
...
return v10;
```

这里底层语义是 Solidity `bytes/string` 的 storage 编码：短字符串内联在 slot，长字符串放在 `keccak(slot)` 后的连续 storage。更高层的恢复结果应该接近：

```solidity
string private _name;
string private _symbol;

function name() public view returns (string memory) {
    return _name;
}

function symbol() public view returns (string memory) {
    return _symbol;
}
```

### 2.3 mapping key 类型还停在 `uint256`

Dedaub 当前输出：

```solidity
mapping (uint256 => uint256) _balanceOf; // STORAGE[0x0]
mapping (uint256 => mapping (uint256 => uint256)) _allowance; // STORAGE[0x1]
```

但函数签名和 IR 都说明 key 是 address：

```solidity
function balanceOf(address account) public payable
function allowance(address owner, address spender) public payable
function transferFrom(address sender, address recipient, uint256 amount) public payable
```

更高层的 storage 类型应该是：

```solidity
mapping(address => uint256) private _balanceOf;
mapping(address => mapping(address => uint256)) private _allowance;
```

这个点适合展示类型恢复的作用：EVM 的 storage key 都是 256-bit word，但从 calldata 解码、`2^160 - 1` mask、函数签名和 mapping hash 构造里，可以把其中一部分 word 恢复成 `address`。

### 2.4 `public payable` 恢复过宽

Dedaub 把很多函数都写成了 `public payable`：

```solidity
function transfer(address recipient, uint256 amount) public payable
function approve(address spender, uint256 amount) public payable
function balanceOf(address account) public payable
```

但 dispatcher 里有：

```solidity
MEM[64] = 128;
require(!msg.value);
```

这说明运行时先统一检查 `msg.value == 0`，这些函数实际应恢复成 non-payable。更合适的是：

```solidity
function transfer(address recipient, uint256 amount) public returns (bool)
function approve(address spender, uint256 amount) public returns (bool)
function balanceOf(address account) public view returns (uint256)
```

### 2.5 `Execute(address[])` 的 ABI 解码还很低层

Dedaub 的 `Execute(address[])` 保留了很多动态数组 ABI 解码细节：

```solidity
require(_bytes_ <= uint64.max);
require(4 + _bytes_ + 31 < msg.data.length);
v0 = new address[](_bytes_.length);
...
while (v3 < 32 + ((_bytes_.length << 5) + (4 + _bytes_))) {
    require(msg.data[v3] == address(msg.data[v3]));
    MEM[v1] = msg.data[v3];
    v3 += 32;
    v1 += 32;
}
```

这些语句表达的是 `address[] calldata` 的 ABI 边界检查、元素 address 校验和 calldata 到 memory 的拷贝。源码级语义应该更接近：

```solidity
function Execute(address[] calldata accounts) public {
    require(msg.sender == _interference, "Not allowed");
    for (uint256 i = 0; i < accounts.length; ++i) {
        address account = accounts[i];
        ...
    }
}
```

### 2.6 一个固定 hash slot 没有恢复出变量语义

Dedaub 输出里有一个 hash 形式的 storage 变量：

```solidity
uint256 stor_ad3228b676f7d3cd4284a5443f17f1962b36e491b30a40b2405849e597ba5fb5;
```

IR 里对应常量 slot：

```llvm
%evm.sload21 = call i256 @evm_sload(i256 -37453343090079225298870253283504062431675071153813376750865614837522423455819)
call void @evm_sstore(i256 -37453343090079225298870253283504062431675071153813376750865614837522423455819, i256 %private.call22)
```

这说明 Dedaub 能保住读写位置，但还不能给出业务变量名或结构含义。这个点可以作为“storage slot 语义恢复仍然困难”的例子。

## 3. 我们当前的 IR 观察和后续高级语义 IR

### 3.1 当前已有 IR：函数签名已经带出 ABI 类型

evm2llvm 输出里，public 函数名已经带了一部分 ABI 类型信息。例如：

```llvm
define void @public_transferFrom_address_address_uint256__0x11b(
    ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
```

函数内有 calldata 长度检查、两个 address 参数解码和一个 uint256 参数：

```llvm
%evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata)
%private.call = call i256 @private__0x820_0x820(..., i256 4, i256 2174)
%private.call2 = call i256 @private__0x820_0x820(..., i256 36, i256 2188)
%evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 68)
```

这里 `private__0x820_0x820` 是 address 参数校验函数：

```llvm
%evm.calldataload = call i256 @evm_calldataload(ptr %calldata, i256 %_0x820arg0x0)
%evm.shl = call i256 @evm_shl(i256 160, i256 1)
%evm.sub = sub i256 %evm.shl, 1
%evm.and = and i256 %evm.calldataload, %evm.sub
%evm.eq = icmp eq i256 %evm.calldataload, %evm.and
```

这段底层语义是：

```text
load calldata word;
require(word == address(uint160(word)));
return address(word);
```

后续可以把它提升成带类型的 IR：

```llvm
%sender = evm.abi.load_address ptr %calldata, 4
%recipient = evm.abi.load_address ptr %calldata, 36
%amount = evm.abi.load_uint256 ptr %calldata, 68
call void @erc20.transferFrom_body(address %sender, address %recipient, uint256 %amount)
```

这里的目标不是立刻生成 Solidity，而是在 IR 层先去掉反复出现的 mask、calldataload 和 revert 模式，让后续类型恢复、变量命名和 C/Solidity-like 输出更稳定。

### 3.2 当前已有 IR：`symbol()` 的 storage string 到 memory 拷贝

`symbol()` 里先拿 free memory pointer，然后调用内部拷贝函数：

```llvm
%evm.mload = call i256 @evm_mload(ptr %mem, i256 64)
%private.call1 = call i256 @private__0x7d4_0x7d4(
    ptr %mem, ptr %calldata, ptr %returndata, ptr %env,
    i256 %evm.mload, i256 %private.call, i256 117591)
%evm.mload2 = call i256 @evm_mload(ptr %mem, i256 64)
%evm.sub = sub i256 %private.call1, %evm.mload2
call void @evm_return(ptr %mem, i256 %evm.mload2, i256 %evm.sub)
```

`private__0x7d4_0x7d4` 里可以看到典型 memory bytes 构造：

```llvm
call void @evm_mstore(ptr %mem, i256 %_0x7d4arg0x0, i256 32)
%evm.mload = call i256 @evm_mload(ptr %mem, i256 %_0x7d4arg0x1)
%evm.add = add i256 %_0x7d4arg0x0, 32
call void @evm_mstore(ptr %mem, i256 %evm.add, i256 %evm.mload)
...
%evm.mload9 = call i256 @evm_mload(ptr %mem, i256 %evm.add8)
call void @evm_mstore(ptr %mem, i256 %evm.add11, i256 %evm.mload9)
...
ret i256 %evm.add6
```

这段可以提升成：

```llvm
%symbol = evm.storage.load_string slot(3)
%ret = evm.abi.encode_string %symbol
call void @evm_return_abi(ptr %ret)
```

或者更接近中间层：

```llvm
%symbol_bytes = evm.storage.bytes.load slot(3)
%ret_ptr = evm.memory.alloc_abi_bytes %symbol_bytes.length
evm.memory.copy_bytes %ret_ptr.data, %symbol_bytes.data, %symbol_bytes.length
evm.return %ret_ptr, %ret_ptr.abi_size
```

这个例子说明：Dedaub 已经识别 `0x40` free memory pointer，但 `MEM[v4] = STORAGE[v9]` 这类 bytes/string 拷贝仍然可以进一步提升。

### 3.3 当前已有 IR：mapping hash 构造可以恢复成 typed storage access

`balanceOf`、`allowance`、`transfer`、`transferFrom` 里都有类似模式：

```llvm
call void @evm_mstore(ptr %mem, i256 0, i256 %evm.and)
call void @evm_mstore(ptr %mem, i256 32, i256 0)
%evm.sha3 = call i256 @evm_sha3(ptr %mem, i256 0, i256 64)
%evm.sload = call i256 @evm_sload(i256 %evm.sha3)
```

这是 Solidity mapping 的地址计算：

```text
slot = keccak256(abi.encode(key, base_slot))
value = sload(slot)
```

结合 address mask，可以提升成：

```llvm
%account = evm.typed.address %raw_account
%balance = evm.storage.mapping.load
    base_slot(0), key(address %account), value(uint256)
```

`allowance[address][address]` 是两层 mapping，可以提升成：

```llvm
%owner = evm.abi.load_address ptr %calldata, 4
%spender = evm.abi.load_address ptr %calldata, 36
%allowance = evm.storage.mapping2.load
    base_slot(1),
    key0(address %owner),
    key1(address %spender),
    value(uint256)
```

这一步的价值是把 `MEM[0]`、`MEM[32]`、`sha3` 这些临时 buffer 操作从业务逻辑里拿掉，后续输出时才能自然显示 `_balanceOf[account]` 和 `_allowance[owner][spender]`。

### 3.4 当前已有 IR：动态数组 calldata 解码可以提升

`Execute(address[])` 对应 IR 入口是：

```llvm
define void @public__0xbad2931e_0x1b1(
    ptr %mem, ptr %calldata, ptr %returndata, ptr %env) {
  %evm.calldatasize = call i256 @evm_calldatasize(ptr %calldata)
  %private.call = call i256 @private__0x8c9_0x8c9(..., i256 4, i256 %evm.calldatasize, i256 447)
  call void @private__0x326_0x326(..., i256 %private.call, i256 452)
}
```

`private__0x326_0x326` 里能看到它在循环访问数组：

```llvm
%evm.mload = call i256 @evm_mload(ptr %mem, i256 %_0x326arg0x0)
%evm.lt = icmp ult i256 %_0x387_0x0, %evm.mload
...
%evm.mul = mul i256 32, %_0x3a3_0x0
%evm.add = add i256 %evm.mul, %_0x326arg0x0
%evm.add8 = add i256 32, %evm.add
%evm.mload9 = call i256 @evm_mload(ptr %mem, i256 %evm.add8)
%evm.and12 = and i256 %evm.mload9, (2^160 - 1)
```

这段可以提升成：

```llvm
%accounts = evm.abi.load_address_array ptr %calldata, 4
%n = evm.array.length %accounts
for %i in 0..%n {
  %account = evm.array.get address, %accounts, %i
  ...
}
```

提升后，Dedaub 里这类低层代码：

```solidity
require(4 + _bytes_ + 31 < msg.data.length);
v0 = new address[](_bytes_.length);
MEM[v1] = msg.data[v3];
```

就可以消失，输出更接近：

```solidity
function Execute(address[] calldata accounts) public {
    require(msg.sender == _interference, "Not allowed");
    for (uint256 i = 0; i < accounts.length; ++i) {
        address account = accounts[i];
        ...
    }
}
```

## 4. PPT 可以怎么讲

这个例子的重点不是说 Dedaub 很差。相反，Dedaub 已经恢复出 ERC20 的主要结构，也已经识别了 `0x40` free memory pointer。真正还可以提升的是更高层的语义：

1. `uint256[] _symbol` / `uint256[] _name` 提升成 `string`.
2. `mapping(uint256 => uint256)` 提升成 `mapping(address => uint256)`.
3. `public payable` 根据统一 `require(!msg.value)` 收窄成 non-payable.
4. `MEM[0]`、`MEM[32]`、`sha3` 提升成 typed mapping access.
5. `new address[]` 加 `msg.data` 循环拷贝提升成 `address[] calldata`.
6. SafeMath 和 revert string 可以进一步内联成源码级 require.

我们当前可以从 evm2llvm IR 入手，先做“高级语义 IR”的中间提升，不急着直接生成 Solidity。核心目标是把反复出现的 EVM 模式替换成 typed operation：

```llvm
evm.abi.load_address
evm.abi.load_uint256
evm.abi.load_address_array
evm.storage.mapping.load
evm.storage.mapping2.load
evm.storage.load_string
evm.memory.alloc_abi_bytes
evm.return_abi
```

这样后续无论是走 NotDec 类型恢复、llvm2c，还是做 Solidity-like 输出，都能少处理很多底层 `mload/mstore/sha3/calldataload` 噪声。

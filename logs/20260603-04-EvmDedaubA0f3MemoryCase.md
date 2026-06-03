# 原始需求

另外再写一个文档解析这个合约  0xa0f319b73a2e5943ed69ba67056910afe3d1078f 这个的dedaub反编译结果在/tmp/a0f3.sol 这个看着没有识别好内存的分配问题。它的字节码在/tmp/a0f3.hex，尝试跑一下，然后分析dedaub的结果存在的问题，也类似写另外一个文档

# 0xa0f3 合约 Dedaub 反编译和 memory 分配问题

## 1. 合约信息

本例来自用户提供的链上合约 bytecode，重点用于观察 Dedaub 在复杂 memory 分配和 ABI 编码场景下的恢复问题。

- 合约地址：`0xa0f319b73a2e5943ed69ba67056910afe3d1078f`
- Dedaub 输出：`/tmp/a0f3.sol`
- 本地 bytecode：`/tmp/a0f3.hex`
- 本地 Gigahorse workdir：`/tmp/notdec-a0f3/work`
- 本地 evm2llvm IR：`/tmp/notdec-a0f3/a0f3.ll`
- 本地 bitcode：`/tmp/notdec-a0f3/a0f3.bc`

本地跑法：

```bash
python3 /sn640/gigahorse-toolchain/gigahorse.py \
  -w /tmp/notdec-a0f3/work \
  -j 8 -T 180 \
  --results_file /tmp/notdec-a0f3/results.json \
  --restart -i --disable_inline --disable_scalable_fallback \
  /tmp/notdec-a0f3/a0f3.hex

/sn640/NotDec/build-evm2llvm/bin/evm2llvm \
  --facts /tmp/notdec-a0f3/work/a0f3/out \
  --output /tmp/notdec-a0f3/a0f3.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-a0f3/a0f3.ll \
  -o /tmp/notdec-a0f3/a0f3.bc
```

Gigahorse 跑通，用时约 47 秒。关键统计：

- `bytecode_size: 2708`
- `Analytics_Functions: 7`
- `Analytics_PublicFunction: 4`
- `Analytics_PublicFunctionNameResolved: 4`
- `errors: 0`

`llvm-as` 可以生成 bitcode。`opt -passes=verify` 报：

```text
unrecognized architecture 'evm' provided
```

这和当前 evm2llvm 产物的 target arch 有关，不是这次 IR 生成失败。

## 2. 合约大致结构

Gigahorse 识别出的 public 函数：

```text
0x0   __function_selector__
0x51  fallback()
0x19c returnUserAccountAddress()
0x1f3 returnWalletTxCount()
0x21e callTx(bytes,address,uint256,bytes)
```

Dedaub 识别出的 storage：

```solidity
uint256 stor_1;                 // STORAGE[0x1]
uint256 _returnWalletTxCount;   // STORAGE[0x2]
uint256 _fallback;              // STORAGE[0x0] bytes 0 to 19
```

这里 `_fallback` 实际应是 address 类型。它被用于：

- `returnIsValidSendingKey(msg.sender)`
- `returnUserControlAddress(stor_1)`
- `returnStaticContractAddress()`

事件：

```solidity
Deposit(address, uint256);
Execution(address, uint256, bytes);
ExecutionFailure(address, uint256, bytes);
```

整体看，这像一个 wallet/proxy 类合约：

1. 通过 `_fallback` 指向的控制合约校验调用者。
2. `callTx(bytes,address,uint256,bytes)` 解析签名和交易数据。
3. 构造一段待签名消息，做 `keccak256`。
4. 用 `ecrecover` 恢复 signer。
5. signer 匹配后，对目标地址执行 call。
6. 根据 call 成败发 `Execution` 或 `ExecutionFailure`，并增加 `_returnWalletTxCount`。
7. fallback 在无 value 时转发静态调用，有 value 时发 `Deposit`。

## 3. Dedaub 结果的主要问题

### 3.1 memory 分配地址混乱

Dedaub 的 `callTx` 中有一段非常可疑的 memory 操作：

```solidity
MEM[MEM[64]] = 0;
MEM[32 + MEM[64]] = 0;
MEM[64 + MEM[64]] = 0;
MEM[96 + MEM[64]] = 0;
MEM[128 + MEM[64]] = 96;
MEM[64] = MEM[64] + 160;
MEM[32 + MEM[64]] = varg1;
MEM[64 + MEM[64]] = varg2;
MEM[96 + MEM[64]] = _returnWalletTxCount;
v5 = new array[](v6 - MEM[64] - 32);
```

问题在于：前五句使用的是旧的 free memory pointer，随后 `MEM[64]` 被更新；后面又继续用新的 `MEM[64]` 写字段。这会把同一个结构的字段写到两个不同的分配基址上。`v5 = new array[](v6 - MEM[64] - 32)` 里的 `v6` 甚至还没有稳定语义，看起来是反编译器在追踪内存区间时丢了基址。

本地 IR 更清楚。`/tmp/notdec-a0f3/a0f3.ll:434` 起先分配一个 160-byte 块：

```llvm
%evm.mload112 = call i256 @evm_mload(ptr %mem, i256 64)
%evm.add113 = add i256 %evm.mload112, 160
call void @evm_mstore(ptr %mem, i256 64, i256 %evm.add113)
call void @evm_mstore(ptr %mem, i256 %evm.mload112, i256 0)
...
call void @evm_mstore(ptr %mem, i256 %evm.add117, i256 96)
```

`/tmp/notdec-a0f3/a0f3.ll:449` 起又分配另一个 160-byte 块，并写入真正的消息字段：

```llvm
%evm.mload118 = call i256 @evm_mload(ptr %mem, i256 64)
%evm.add119 = add i256 %evm.mload118, 160
call void @evm_mstore(ptr %mem, i256 64, i256 %evm.add119)
call void @evm_mstore(ptr %mem, i256 %evm.mload118, i256 address(this))
call void @evm_mstore(ptr %mem, i256 %evm.add121, i256 to)
call void @evm_mstore(ptr %mem, i256 %evm.add123, i256 value)
call void @evm_mstore(ptr %mem, i256 %evm.add124, i256 txCount)
call void @evm_mstore(ptr %mem, i256 %evm.add126, i256 dataPtr)
```

所以更合理的高级语义不是反复写 `MEM[MEM[64]]`，而是：

```text
buf = memory.alloc(160)
buf[0x00] = address(this)
buf[0x20] = to
buf[0x40] = value
buf[0x60] = txCount
buf[0x80] = data_offset_or_ptr
```

然后再把 calldata bytes 拼到这个 buffer 后面，计算 `keccak256`。

### 3.2 bytes 参数解码恢复还可以，但没有提升成抽象操作

Dedaub 对两个 `bytes` 参数做了这种恢复：

```solidity
v0 = new bytes[](varg0.length);
CALLDATACOPY(v0.data, 36 + varg0, varg0.length);
v0[varg0.length] = 0;
...
v1 = new bytes[](varg3.length);
CALLDATACOPY(v1.data, 36 + varg3, varg3.length);
v1[varg3.length] = 0;
```

这说明它大体识别出 ABI dynamic bytes。对应 IR 在 `/tmp/notdec-a0f3/a0f3.ll:292` 和 `/tmp/notdec-a0f3/a0f3.ll:344`：

```llvm
%len0 = call i256 @evm_calldataload(ptr %calldata, i256 %offset0)
%base0 = call i256 @evm_mload(ptr %mem, i256 64)
call void @evm_mstore(ptr %mem, i256 64, i256 %new_free_ptr)
call void @evm_mstore(ptr %mem, i256 %base0, i256 %len0)
call void @evm_calldatacopy(ptr %mem, ptr %calldata, i256 %base0_data, i256 %src0, i256 %len0)
call void @evm_mstore(ptr %mem, i256 %base0_data_plus_len, i256 0)
```

这一步可以进一步提升成：

```llvm
%signature = evm.abi.load_bytes ptr %calldata, 4
%callData = evm.abi.load_bytes ptr %calldata, 100
```

或者：

```text
signature = abi.decode_arg_bytes(msg.data, 0)
to        = abi.decode_arg_address(msg.data, 1)
value     = abi.decode_arg_uint256(msg.data, 2)
callData  = abi.decode_arg_bytes(msg.data, 3)
```

这样后面就不需要在业务逻辑里继续暴露 `CALLDATACOPY`、`v0.data`、`v1.data` 和补零写入。

### 3.3 签名消息构造没有恢复成 ABI encoding

Dedaub 输出里这一段：

```solidity
v5 = new array[](v6 - MEM[64] - 32);
MEM[v5.data] = address(this);
v5[32] = 64;
MEM[64 + v5.data] = MEM[v1];
...
v19, v20 = ecrecover(keccak256(v5), 0xff & (0xff & v14), MEM[v0.data], MEM[v0 + 64]);
```

这里不太像真实源码。它应该是在构造签名 digest 的 bytes buffer，然后对 buffer 做 `keccak256`，再用 signature 的 `v/r/s` 调 `ecrecover`。

IR 里可见更具体的数据流：

```llvm
%message_base = call i256 @evm_mload(ptr %mem, i256 64)
...
call void @evm_mstore(..., address(this))
call void @evm_mstore(..., to)
call void @evm_mstore(..., value)
call void @evm_mstore(..., txCount)
call void @evm_mstore(..., dataPtr)
...
%digest = call i256 @evm_sha3(ptr %mem, i256 digest_data, i256 digest_len)
%signer = call i256 @private__0x901_0x901(..., signature_ptr, digest, ...)
```

高级 IR 可以表达成：

```llvm
%digest_input = evm.abi.encode_packed_or_bytes
  address(this),
  address %to,
  uint256 %value,
  uint256 %txCount,
  bytes %callData

%digest = evm.keccak256 %digest_input
%signer = evm.ecrecover_bytes65 %digest, %signature
```

关键不是一定要判断是 `abi.encode` 还是 `abi.encodePacked`，而是先把“分配 buffer、写字段、循环复制 bytes、尾部 mask、sha3”提升成一个明确的 message encoding 操作。

### 3.4 `ecrecover` 可以恢复成 bytes65 signature 解析

Dedaub 当前输出：

```solidity
if (v0.length == 65) {
    v14 = v15 = byte(MEM[v0 + 96], 0x0);
    if (0xff & v15 < 27) {
        v14 = v16 = v15 + 27;
    }
    ...
    v19, v20 = ecrecover(keccak256(v5), 0xff & (0xff & v14), MEM[v0.data], MEM[v0 + 64]);
}
```

这已经能看出签名格式，但还停在 memory offset 层。IR 里 `private__0x901_0x901` 更清楚：

```llvm
%len = call i256 @evm_mload(ptr %mem, i256 %sig)
%ok = icmp eq i256 %len, 65
%r = call i256 @evm_mload(ptr %mem, i256 (%sig + 32))
%s = call i256 @evm_mload(ptr %mem, i256 (%sig + 64))
%v_word = call i256 @evm_mload(ptr %mem, i256 (%sig + 96))
%v = byte 0, %v_word
if v < 27: v += 27
if v == 27 || v == 28:
  staticcall precompile 1
```

可以提升成：

```llvm
%sig_parts = evm.signature.parse_bytes65 %signature
%v = evm.signature.normalize_v %sig_parts.v
%signer = evm.ecrecover %digest, %v, %sig_parts.r, %sig_parts.s
```

这样 Dedaub 里的 `MEM[v0.data]`、`MEM[v0 + 64]`、`byte(MEM[v0 + 96], 0)` 都可以变成签名字段访问。

### 3.5 event 的动态 bytes 参数没有恢复好

Dedaub 输出：

```solidity
emit ExecutionFailure(varg1, varg2, v26, v31, MEM[v0 + 64]);
...
emit Execution(varg1, varg2, v32, v31, MEM[v0 + 64]);
```

但事件声明只有三个参数：

```solidity
Execution(address, uint256, bytes);
ExecutionFailure(address, uint256, bytes);
```

这里 `v31`、`MEM[v0 + 64]` 明显是反编译器在处理 event data memory 区间时泄漏出来的临时值，不应作为事件参数显示。

IR 对应两处 log：

```llvm
call void @evm_log2(..., topic ExecutionFailure, indexed_to)
call void @evm_log2(..., topic Execution, indexed_to)
```

更合理的结果应该是：

```solidity
emit ExecutionFailure(to, value, callData);
emit Execution(to, value, callData);
```

这里也说明 memory buffer 的 typed layout 没识别完整：动态 bytes 的 event data ABI 编码、长度、数据区和尾部 padding 被混进了高层输出。

### 3.6 控制流还没有完全结构化

Dedaub 里出现：

```solidity
goto 0x7c4;
```

这说明 `callTx` 的成功、失败、签名不匹配这几条路径还没有完全结构化。IR 中 `bb._0x3a5` 是统一返回块：

```llvm
%ret = phi i256 [ 0, invalid_signer ], [ 1, success ], [ 1, failure ]
return bool(ret)
```

源码级恢复可以写成：

```solidity
if (signer != expectedSigner) {
    return false;
}

bool ok = to.call{value: value, gas: gasleft() - 34710}(callData);
if (ok) {
    emit Execution(to, value, callData);
} else {
    emit ExecutionFailure(to, value, callData);
}
_returnWalletTxCount += 1;
return true;
```

## 4. 我们当前可以怎么做

这个例子比上一个 ERC20 更适合说明 memory recovery 的必要性。Dedaub 对 `0x40` free memory pointer 有一定识别，但在 `callTx` 里同时出现：

- 两个动态 `bytes` 参数解码。
- 构造签名消息 buffer。
- 对消息 buffer 做 `keccak256`。
- 从 bytes65 signature 里取 `r/s/v`。
- 调 ecrecover precompile。
- 构造带动态 bytes 的 event data。

这些模式叠在一起后，Dedaub 的 memory 区间跟踪开始不稳，输出了 `v6 - MEM[64] - 32`、`v31`、`MEM[v0 + 64]` 这类不该出现在源码级结果里的表达。

我们可以先在 IR 层做几个更小的提升，不急着直接生成 Solidity：

```llvm
evm.abi.load_bytes
evm.abi.load_address
evm.abi.load_uint256
evm.memory.alloc_bytes
evm.memory.copy_bytes
evm.abi.encode_message
evm.keccak256_bytes
evm.signature.parse_bytes65
evm.ecrecover
evm.event.emit_dynamic_bytes
```

对本例来说，目标高级 IR 可以长这样：

```llvm
%signature = evm.abi.load_bytes ptr %calldata, 4
%to        = evm.abi.load_address ptr %calldata, 36
%value     = evm.abi.load_uint256 ptr %calldata, 68
%callData  = evm.abi.load_bytes ptr %calldata, 100

%controller = evm.storage.load_address slot(0)
%is_valid = evm.staticcall.bool
  address %controller,
  selector(returnIsValidSendingKey(address)),
  address msg.sender

%expected = evm.staticcall.address
  address %controller,
  selector(returnUserControlAddress(uint256)),
  uint256 slot(1)

%digest_input = evm.abi.encode_wallet_tx
  address(this), %to, %value, storage.slot(2), %callData
%digest = evm.keccak256_bytes %digest_input
%signer = evm.ecrecover_bytes65 %digest, %signature

if %signer != %expected:
  return false

%ok = evm.call_bytes address %to, uint256 %value, bytes %callData, gas_sub(34710)
if %ok:
  evm.event.emit Execution(%to, %value, %callData)
else:
  evm.event.emit ExecutionFailure(%to, %value, %callData)

evm.storage.store slot(2), storage.slot(2) + 1
return true
```

这个中间层足够表达底层语义，又不会过早承诺完整 Solidity 源码。后续再根据这些 typed operation 生成 C、Solidity-like 伪代码，或者把类型约束接到 MLsub，都更容易。

## 5. PPT 可以怎么讲

这份样例可以作为“Dedaub 内存分配恢复不稳”的展示：

1. Dedaub 已经识别函数名和大体业务逻辑。
2. 但 `callTx` 的 memory 分配明显混乱，旧 free pointer 和新 free pointer 混用了。
3. 动态 bytes、签名消息、事件 data 都仍然带着 `MEM[...]` 和未定义临时变量。
4. 本地 evm2llvm IR 能把这些低层操作保留下来，便于继续做 pattern-based 提升。
5. 我们后续要恢复的不是单个 `mload(0x40)`，而是一组更高层对象：`bytes`、签名、digest、event data、external call input。

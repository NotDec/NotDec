# EVM 高层语义恢复后续 Pass 规划

## 原始 prompt

参考logs/archive/20260602-05-evm-memory-object-type-recovery-pass-ordering-plan.md，还有logs/20260603-03-EvmDedaubTypeRecoveryCase.md以及logs/20260603-04-EvmDedaubA0f3MemoryCase.md，看看还有哪些高层语义没有恢复，规划一下后续还要加哪些PASS负责恢复新的语义，写成一个新的logs/规划文件

## 背景

`20260602-05` 已经把路线从“每个 ABI/event/call pass 自己扫 memory 操作”改成了：

1. 先识别 memory object。
2. 把对象上的 `mstore/mload/copy/use` 暴露给类型恢复。
3. 类型恢复给出对象结构、字段、动态区间和角色。
4. 后续 Solidity 语义 pass 消费这些结果。

`20260603-03` 和 `20260603-04` 的 Dedaub 对照说明，当前剩下的问题不只是内存分配。很多地方需要把 EVM 低层操作恢复成源码层能理解的语义，例如 ABI 参数、storage mapping/string、动态 bytes、签名消息、`ecrecover`、外部 call 和动态 event 参数。

所以后续不应该继续把所有逻辑塞进 `SolidityPatterns.cpp` 或单个大 pass。更稳的做法是按语义边界拆成几个 pass：先产出事实或 marker，再做少量 rewrite，最后统一清理低层 helper。

## 当前已经有的基础

目前这条链路已经有这些基础能力：

- selector / public entry outlining。
- nonpayable guard 识别。
- revert / panic / checked bounds 的一部分识别。
- memory allocation rewrite，把 free memory pointer 改成显式分配。
- ABI return / Solidity revert / event log 的 HType 方向重构。
- storage matcher helper，但还没有完整 storage 语义 pass。

这些能力够支撑下一步高层语义恢复。关键是不要让后续 pass 再各自重复追踪同一批 memory 写入。

## 还缺的高层语义

### 1. Public ABI 参数

静态参数里，`address`、`bool`、`uintN/intN` 还需要稳定从 calldata load、mask、signextend 和 guard 里恢复出来。

动态参数里，`bytes`、`string`、`address[]` 这类参数还会暴露很多 `calldataload`、`calldatacopy`、边界检查和 memory copy。`Execute(address[])` 和 `callTx(bytes,address,uint256,bytes)` 都属于这个问题。

需要恢复成类似：

```text
arg0 = abi.arg.bytes(0)
arg1 = abi.arg.address(1)
arg2 = abi.arg.uint256(2)
arg3 = abi.arg.bytes(3)
```

这一步应该只依赖 public entry 上下文、calldata 访问事实、类型恢复结果和 guard 结果，不应该猜 ABI JSON。

### 2. Storage typed access

`20260603-03` 里 `_balanceOf` 和 `_allowance` 被 Dedaub 反编译成了 `mapping(uint256 => ...)`，但 calldata 参数和 address mask 能说明 key 是 address。

`_name` / `_symbol` 被恢复成 `uint256[]`，但 storage 访问形状符合 Solidity storage bytes/string 的短串和长串编码。

需要恢复的语义包括：

- `mapping(address => uint256)`。
- `mapping(address => mapping(address => uint256))`。
- storage `bytes` / `string` 的 short / long 两种编码。
- packed storage field 里的 address / bool / small int。

固定 hash slot 可以先保守保留成 slot summary，不要强行恢复业务变量名。

### 3. Memory bytes object

`20260603-04` 里的 `callTx` 已经能看出 bytes 参数复制到了 memory，但还没有恢复成稳定的 memory bytes 对象。

需要把这些形状统一成事实：

- `new bytes(len)`。
- `calldatacopy(bytes.data, src, len)`。
- 末尾补零。
- `bytes.length` 和 `bytes.data`。
- memory bytes 被 return / event / call / keccak 使用。

这个 pass 主要补齐 memory object 和 dynamic bytes 的关系，后面的 event、call、keccak pass 都会用到。

### 4. ABI encoding / message encoding / keccak

`callTx` 里最关键的语义不是普通内存写入，而是构造一段待签名消息，然后 `keccak256`。

需要识别：

- 固定字段连续写入 buffer。
- 动态 bytes 拼接到 buffer。
- buffer 被 `sha3` / `keccak256` 消费。
- 这个 buffer 是 `abi.encode`、`abi.encodePacked`，还是项目自定义拼接。

这里要保守。只有 offset、长度和 padding 证据足够时才命名成 `abi.encode` 或 `abi.encodePacked`。证据不足时，可以先恢复成 `message.encode(...)` 这种中性 marker。

### 5. Signature / ecrecover

`20260603-04` 里签名解析没有恢复好：从 65 字节 signature 里取 `r/s/v`，再调 `ecrecover`。

需要恢复：

- `signature.length == 65` 一类 guard。
- `r = mload(signature.data)`。
- `s = mload(signature.data + 32)`。
- `v = byte(signature.data + 64)`。
- `v` 的 `27/28` 或 `0/1` 归一。
- precompile `0x01` 的 `staticcall` / `call`。

最后可以表达成：

```text
signer = ecrecover_bytes65(digest, signature)
```

但不能把任意 65 字节数组都当成签名。需要同时看到长度、字段读取和 ecrecover precompile。

### 6. External call

当前 external call 仍容易露出 input buffer、output buffer、returndatacopy 和 bubble revert。

需要恢复：

- `call/staticcall/delegatecall/callcode` 的 target、value、gas。
- call input bytes。
- call output bytes 或 ABI decode 结果。
- `returndatacopy(0, 0, returndatasize()); revert(0, returndatasize())` 这类失败路径。
- call 成功后根据返回值触发 event 或 storage 更新。

这一步要消费 memory bytes、ABI encoding 和 return/revert 事实，不能单独重新追踪所有 buffer。

### 7. Event dynamic data

已有 `EventLogPass` 能处理一部分 event log，但动态 bytes 参数还会泄漏 memory 临时变量。

需要补齐：

- event data buffer 的 ABI tuple 结构。
- 动态 bytes/string 参数的 head/tail。
- topic 和 data 参数对应关系。
- call 成功/失败后发不同 event 的分支语义。

这可以扩展当前 `EventLogPass`，也可以拆成 `EventSemanticsPass`。如果只是补动态 data，优先扩展现有 pass；如果要引入更多 event 参数类型，建议单独拆文件。

## 建议新增或扩展的 Pass

### 1. `EvmAbiArgumentPass`

位置：类型恢复之后，其他业务语义 pass 之前。

职责：

- 根据 public entry 和 selector 区域恢复 ABI 参数。
- 把 calldata load/copy、边界检查和 HType 结果合成参数 marker。
- 处理静态参数和动态参数。

第一阶段目标是 `address`、`uint256`、`bool`、`bytes`、`string`、`address[]`。

### 2. `EvmStorageSemanticsPass`

位置：`EvmAbiArgumentPass` 之后。

职责：

- 从 `sload/sstore`、keccak slot 构造和参数类型里恢复 mapping key 类型。
- 恢复 storage bytes/string。
- 生成 storage field summary，给后端和 oracle 使用。

它应该先产出 storage 事实，不急着删除原始 `sload/sstore`。等 case 稳定后再做 rewrite。

### 3. `EvmMemoryBytesSemanticsPass`

位置：storage pass 之后，encoding / event / call pass 之前。

职责：

- 把 memory object 里的 length/data/copy/padding 恢复成 memory bytes/string。
- 标记 calldata bytes 到 memory bytes 的复制。
- 给后续 keccak、event、external call 提供 bytes 对象。

这个 pass 主要解决 `callTx` 里两个 bytes 参数和 `Execute(address[])` 里的动态数组复制。

### 4. `EvmAbiEncodingPass`

位置：memory bytes pass 之后。

职责：

- 识别连续写字段后被 return/event/call/keccak 消费的 buffer。
- 区分 ABI tuple、dynamic ABI buffer、message buffer。
- 在证据足够时恢复 `abi.encode` / `abi.encodePacked`。

这里要保守命名。签名消息场景可以先标成 message buffer，不必一次性判断源码到底写的是哪种 encode API。

### 5. `EvmSignaturePass`

位置：encoding pass 之后。

职责：

- 识别 bytes65 signature 的 `r/s/v` 解析。
- 识别 ecrecover precompile。
- 把 digest + signature 合成 `ecrecover` 语义。

它依赖 memory bytes 和 keccak digest，所以不应该太早跑。

### 6. `EvmExternalCallPass`

位置：signature pass 之后，event pass 之前。

职责：

- 恢复外部 call 的 target/value/input/output。
- 识别 call input bytes 和 returndata 使用。
- 把失败路径的 returndata revert 和成功路径的结果分开。

这一步会影响 event 判断，因为很多合约在 call 成功或失败后发不同 event。

### 7. `EvmEventSemanticsPass`

位置：external call pass 之后。

职责：

- 扩展现有 event log 识别。
- 恢复动态 bytes/string 参数。
- 消除 event data buffer 暴露出的临时 memory 操作。

如果只改当前 `EventLogPass` 就够，可以不新建 pass 名称；但代码应拆到 `solidity-patterns` 下更具体的 cpp 文件里，避免 `SolidityPatterns.cpp` 继续变长。

### 8. `EvmSemanticCleanupPass`

位置：所有高层语义 pass 之后。

职责：

- 只删除已经被高层 marker 覆盖的低层 helper。
- 保留证据不足的 `mload/mstore/copy/sload/sstore`。
- 清理仅用于 ABI bounds、nonpayable、panic、return/revert/event 的 compiler guard。

这一步要最后做，避免过早清理掉后续 pass 还需要的证据。

## 建议顺序

大致顺序可以是：

```text
SelectorEntryOutliningPass
PayabilityGuardPass
SolidityRevertPass / CheckedBoundsPass 的 guard 识别部分
MemoryBufferRewritePass
TypeRecovery
EvmAbiArgumentPass
EvmStorageSemanticsPass
EvmMemoryBytesSemanticsPass
EvmAbiEncodingPass
EvmSignaturePass
EvmExternalCallPass
AbiReturnPass
EventLogPass / EvmEventSemanticsPass
EvmSemanticCleanupPass
```

这里有两个点要注意：

- `CheckedBoundsPass` 仍然按 panic / guard 形状识别，不需要判断某个 pointer 是 ABI array、storage bytes 还是 allocation buffer。
- `AbiReturnPass`、`SolidityRevertPass`、`EventLogPass` 已经在 HType 路线上，后续可以按需要挪顺序，但不要回到各自独立扫描 memory buffer 的旧方式。

## 阶段计划

### 阶段 1：只产出事实，不做激进 rewrite

先补 `EvmAbiArgumentPass`、`EvmStorageSemanticsPass`、`EvmMemoryBytesSemanticsPass` 的 analysis / marker。

判断标准：

- 能在 `20260603-03` 样例里标出 `Execute(address[])`。
- 能在 `20260603-04` 样例里标出两个 `bytes` 参数和对应 memory bytes。
- 不删除低层 IR，避免影响现有 oracle。

### 阶段 2：storage 和动态 calldata 进入 oracle

把较稳定的 storage 和 ABI 参数恢复接入输出。

判断标准：

- `_balanceOf` key 能恢复成 address 候选。
- `_allowance` 两层 key 能恢复成 address 候选。
- `_name` / `_symbol` 能恢复成 storage string 候选。
- `Execute(address[])` 不再主要表现为裸 `calldataload/calldatacopy`。

### 阶段 3：memory bytes、event dynamic data 和 call input

把 memory bytes 对象接到 event、external call 和 return/revert 使用点。

判断标准：

- `callTx` 的 signature 和 callData 能作为两个独立 bytes 参数流到后续语义。
- `Execution(address,uint256,bytes)` / `ExecutionFailure(address,uint256,bytes)` 不再泄漏临时 memory buffer。
- external call input 能关联到 calldata bytes 参数。

### 阶段 4：message encoding、keccak 和 ecrecover

恢复签名消息构造、digest 和 signer。

判断标准：

- `callTx` 里连续写入 `address(this), to, value, txCount, data` 的 buffer 能标成 message buffer。
- `keccak256(message)` 能关联到这个 buffer。
- `ecrecover` 能关联 digest 和 signature bytes。

### 阶段 5：统一 cleanup

前面 marker 稳定后，再清理已经被覆盖的 helper。

判断标准：

- verifier 通过。
- 现有 Solidity pattern 回归不退。
- apehex smoke 没有新增 runner 失败。
- oracle 只因为高层语义变好而变化，不出现大面积丢语义。

## 风险

- ABI encode 和项目自定义 bytes 拼接容易混淆，证据不足时不要过早命名。
- storage bytes/string 的 short / long 编码需要同时看 slot 位和 keccak(slot) 访问，不能只看某一个分支。
- mapping key 的 address 推断依赖 calldata 参数、mask 和 storage hash 构造，单点证据不够稳。
- ecrecover 不能只靠 65 字节数组识别，必须看到 precompile 和 `r/s/v` 使用。
- external call 的 returndata 长度和 memory allocation size 不一定相同，不能把 output buffer 简单当成固定 ABI tuple。
- cleanup 最容易误删证据，必须最后做。

## 判断标准

后续实现这些 pass 时，至少用下面几个结果判断方向对不对：

- `20260603-03` ERC20-like 样例里，`_name/_symbol` 有 storage string 候选。
- `20260603-03` 样例里，`_balanceOf/_allowance` 的 mapping key 有 address 候选。
- `20260603-03` 样例里，`Execute(address[])` 能恢复成动态 address 数组参数。
- `20260603-04` 样例里，`callTx(bytes,address,uint256,bytes)` 的两个 bytes 参数被稳定标出。
- `20260603-04` 样例里，签名消息、`keccak256` 和 `ecrecover` 能连起来。
- 动态 bytes event 参数不再主要暴露为临时 memory 指针。
- `SolidityPatterns.cpp` 不继续增加大块 matcher，新逻辑拆到具体 cpp 文件。

# EVM Solidity Pass Pipeline Draft

## 原则

1. Pass 按 Solidity 语义领域划分，不按抽象处理阶段划分。
2. 每个 Pass 内部自己分：
   - 收集事实
   - 识别语义
   - 产出 rewrite
3. 共享底层事实只在确实被多方复用时单独抽出来。
4. metadata 只辅助测试和调试，pass 间主接口尽量是 semantic marker / helper analysis。
5. 不做一个总 rewrite pass。某个语义由哪个 pass 识别，就由哪个 pass 负责 rewrite。

## 更合理的 Pass 链路

### 1. `EvmCanonicalization`

纯 IR 稳定化。

目标：

- 清理 evm2llvm 输出里的冗余 zext / icmp / helper call 周围临时值。
- 让后续 matcher 面向更稳定的 IR 形状。

不负责 Solidity 语义识别。

### 2. `SelectorEntryPass`

识别 public selector / fallback / receive / helper body boundary。

目标：

- 标出 selector dispatcher。
- 标出 public entry。
- 标出 fallback / receive。
- 必要时 outline selector 内联 body。

输出：

- entry context。
- dispatcher / body boundary rewrite。

### 3. `PayabilityGuardPass`

识别 nonpayable guard。

目标：

- 识别 `callvalue == 0 -> revert(0, 0)`。
- 判断这是编译器生成的 nonpayable guard，不是用户业务分支。
- rewrite / hide 对应 guard。

依赖：

- `SelectorEntryPass` 的入口上下文。

### 4. `MemoryBufferPass`

共享 memory 基础设施。

目标：

- 识别被多方复用的 memory facts：
  - allocation
  - write
  - copy write
  - read
  - consumer role
- 给 ABI return / revert / event / external call 等 pass 提供统一 memory surface。

不负责：

- ABI return 最终语义。
- ABI revert payload 最终语义。
- event 参数语义。
- external call input/output 最终语义。

### 5. `RevertPass`

识别 revert exit 和 ABI revert payload。

内部可以分：

- classify exit
- parse revert buffer
- rewrite panic / Error(string) / custom error / returndata bubble

目标：

- 识别 empty revert。
- 识别 returndata bubble。
- 识别 Panic(uint256)。
- 识别 Error(string)。
- 识别 custom error。
- 产出高层 revert rewrite。

依赖：

- `MemoryBufferPass` 的 revert consumer / writes / copy writes。

后续如果 pass 变大，再考虑拆成：

- `RevertExitPass`
- `AbiRevertEncodingPass`

### 6. `CheckedGuardPass`

识别 checked arithmetic / bounds / conversion / allocation guards。

目标：

- 识别 checked add / sub / mul / div / mod / exp。
- 识别 array bounds。
- 识别 enum / bool / integer conversion guard。
- 识别 memory allocation bounds。
- 识别 storage bytes/string encoding guard。
- rewrite compiler guard。

依赖：

- `RevertPass` 的 Panic code / revert semantic marker。

### 7. `CalldataArgumentPass`

public entry 参数解析。

不叫 `AbiDecodePass`。

目标：

- 识别 public entry 的 calldata 参数。
- 识别 static calldata word。
- 识别 dynamic offset。
- 识别 dynamic length。
- 识别 calldata data area。
- 识别 calldata bounds guard。
- rewrite public entry 参数 decode。

依赖：

- `SelectorEntryPass` 的 public entry context。
- `CheckedGuardPass` / `RevertPass` 的 bounds / revert 信息。

共享：

- 使用 ABI layout helper，但自己负责 public calldata 参数语义。

### 8. `AbiReturnEncodingPass`

成功返回值编码。

目标：

- 识别 `return(buffer, size)`。
- 识别 static return word。
- 识别 dynamic bytes / string / array return。
- 识别 returndata forward。
- rewrite ABI return encoding。

依赖：

- `MemoryBufferPass` 的 return consumer / writes / reads / copy writes。

注意：

- 不应该长期自己扫 raw `evm_mstore/evm_mload`。
- 这类底层 memory shape 应尽量下沉到 `MemoryBufferPass` 或共享 helper。

### 9. `StorageAddressingPass`

识别 storage root / mapping / dynamic array slot。

目标：

- 识别 scratch memory + `sha3` 的 storage addressing。
- 识别 mapping slot。
- 识别 dynamic array data slot。
- rewrite storage address semantic marker。

依赖：

- `MemoryBufferPass` 的 scratch memory facts，或者本 pass 内部保守识别 scratch pattern。

### 10. `PackedStorageFieldPass`

识别 packed storage load/store。

目标：

- 识别 `sload/sstore` 周围的 shift / mask / or 链。
- 恢复 packed field access。
- rewrite packed field semantic marker。

依赖：

- `StorageAddressingPass` 的 storage root / slot 信息。

### 11. `StorageBytesStringPass`

识别 storage bytes/string 编码。

目标：

- 识别 storage bytes/string 短编码。
- 识别 storage bytes/string 长编码。
- 识别 length bounds。
- 识别 data area。
- rewrite storage bytes/string semantic marker。

依赖：

- `StorageAddressingPass`。
- `CheckedGuardPass`。

### 12. `EventEncodingPass`

识别 event topics 和 event data ABI encoding。

目标：

- 识别 `logN`。
- 识别 topic。
- 识别 event data buffer。
- 识别 event data word / copy writes。
- rewrite event semantic marker。

依赖：

- `MemoryBufferPass` 的 event consumer / writes / copy writes。

### 13. `ExternalCallPass`

识别 external call 相关语义。

如果保持一个 pass，内部清楚拆：

- call site
- input encoding
- success / failure
- output decode

目标：

- 识别 call / staticcall / delegatecall / callcode。
- 识别 input buffer。
- 识别 selector / ABI head / dynamic tail。
- 识别 success check。
- 识别 returndata bubble。
- 识别 output buffer。
- 识别 output word read。
- 识别 output ABI decode buffer。
- rewrite external call semantic marker。

依赖：

- `MemoryBufferPass` 的 external call input/output consumer、writes、reads、copy writes。
- `RevertPass` 的 returndata bubble 信息。

后续如果 pass 变大，再考虑拆成：

- `ExternalCallSitePass`
- `ExternalCallInputEncodingPass`
- `ExternalCallOutputDecodePass`

原则仍然是每个 pass 内部自己识别和 rewrite，不做全局 rewrite pass。

### 14. `VerifierPass`

检查输出 IR 合法性。

## 真实依赖顺序

全局排列只看真实依赖：

1. canonicalization 先。
2. selector / payability 早，因为它们提供入口上下文。
3. memory buffer 在 ABI return / revert / event / external call 前，因为 memory facts 被共享。
4. revert 在 checked guard 前，因为 checked guard 要 Panic code。
5. calldata argument 在 selector / revert / checked guard 后，因为它需要入口上下文和 bounds guard 语义。
6. storage addressing 在 packed storage / storage bytes-string 前。
7. external call output decode 需要 external call site 和 memory output facts。


# Evm2llvm Solidity 编译器模式识别 Pass 计划

## 原始 prompt

阅读 logs/20260520-06-Evm2llvmSemanticLiftPlan.md 首先盘点一下solidity编译器到底有哪些这种主动加入的底层操作。提到的这个plan里面，基于Selector恢复原始函数名这个暂时不需要关注，因为GIGA horse有一些常见的匹配出来，而且这个是evm2llvm底层lifting的部分负责的。当前需要的是尝试用llvm pass的方式，每个pass负责识别某个底层的特性，然后转换为比如nonpayable metadata标注或者什么其他的高层次语义形式。专门识别ERC1967 implementation slot这种就更不用考虑了，是源码上的编程模式，这个完全不用考虑。总之，写一个新的plan，尽量全的盘点solidity编译器这种需要识别的底层模式，然后规划一下要写哪些pass

## 背景

`20260520-06-Evm2llvmSemanticLiftPlan.md` 已经用一个 proxy 样例说明了源码、EVM、LLVM IR 的对应关系。那个 plan 里有两类内容现在先不做：

- 不做 selector 到原始函数名的恢复。Gigahorse 已经能匹配一些常见 selector，evm2llvm 底层 lifting 也会继续负责 selector 入口的基础信息。
- 不做 ERC1967 implementation slot 这类源码或库层面的模式。固定 slot 是 OpenZeppelin / ERC 编程约定，不是 Solidity 编译器必然主动生成的底层操作。

这份 plan 只关心 Solidity 编译器为了 ABI、内存、storage、安全检查、错误编码、调用语义而主动生成的低层代码。目标是后续用 LLVM pass 在当前 evm2llvm IR 上识别这些模式，并挂 metadata 或替换成更高层的语义 intrinsic。

## 总体目标

第一阶段不要急着生成好看的 Solidity 源码。先让 IR 里多出可靠的语义标注：

- public/external 函数的 `nonpayable`、fallback、receive、参数 ABI 解码、返回 ABI 编码。
- compiler-generated `require` / `assert` / panic / ABI bounds check 和用户业务判断的区别。
- address、bool、小整数、enum、bytes/string、storage packed field 这些类型线索。
- mapping、动态数组、短 bytes/string、事件、外部调用返回冒泡等常见编译器形状。

输出形式建议先保守：

- 函数级 metadata：例如 `!notdec.solidity.nonpayable`、`!notdec.solidity.entry_kind`。
- 指令级 metadata：例如 `!notdec.solidity.abi_decode`、`!notdec.solidity.cleanup`、`!notdec.solidity.panic`。
- 少量高层 intrinsic：只用于已经很稳定的结构，比如 ABI return、revert error、storage packed load/store。

## 需要识别的编译器底层模式

### 1. Selector 函数里的内联业务逻辑

典型模式：

- Gigahorse 当前会把入口分发放进名字类似 `public___function_selector___0x0` 的函数。
- 这个函数里有 selector 分发逻辑：`calldatasize < 4`、`calldataload(0) >> 224`、和 selector 常量比较。
- 有些没有单独 public 函数壳的逻辑会被内联在 selector 函数里，例如 fallback 路径、receive 路径、短分支里的直接 revert/return、delegatecall 转发等。
- 入口开头可能有 `mstore(0x40, 0x80)`。例如 0394 样例里 `public___function_selector___0x0` 开头就有 `evm_mstore(%mem, 64, 128)`。

提升目标：

- 不把 selector 分发从 `function_selector` 函数里拆走。现在单独放在那里是合理的。
- 识别 selector 分发边界，找出不属于 selector 比较链的内联逻辑区域。
- 对这些区域做函数提取候选标注，例如 `fallback_body_candidate`、`receive_body_candidate`、`selector_inlined_body_candidate`。
- `mstore(0x40, 0x80)` 只作为可选 prologue 标注，不单独作为重点 pass。

### 2. Payable / nonpayable 入口保护

典型模式：

- 函数入口读 `callvalue()`。
- 判断 `callvalue == 0`。
- 失败分支 `revert(0, 0)`。

提升目标：

- 给函数标 `nonpayable`。
- 把这段标成 compiler guard，不显示成用户手写 `if (msg.value != 0) revert()`。
- fallback/receive 要单独处理：receive 天然允许收 ETH，fallback 是否 payable 要看有没有同类 guard。

### 3. ABI 参数解码

典型模式：

- 检查 `calldatasize` 是否够长。
- 静态参数从 `calldataload(4 + 32 * index)` 读取。
- address / bool / 小整数读取后做 mask、`signextend`、范围检查。
- 动态参数读取 offset，再检查 offset、length、`offset + length` 是否越界。
- `calldatacopy` 把 bytes/string/array 拷到 memory。

提升目标：

- 标出每个 public function 的 ABI 参数槽位。
- 把 bounds check 标成 ABI 解码保护，不混进业务逻辑。
- 从 cleanup 反推出类型线索：address、bool、uintN/intN、bytes/string、动态数组。

### 4. ABI 返回值编码

典型模式：

- 读取 `mload(0x40)` 作为返回区。
- 依次 `mstore(retptr + offset, value)`。
- 动态返回值会写 head/tail、长度和数据，再更新或计算返回长度。
- 最后 `return(retptr, size)`。

提升目标：

- 标出 return ABI encode 区域。
- 对简单返回值先恢复 `returns(address)`、`returns(bool)`、`returns(uintN)` 这种类型线索。
- 对动态返回值先标 `abi_return_dynamic`，不要一开始就强行恢复复杂结构。

### 5. Revert、Error、Panic 和 custom error 编码

典型模式：

- 空 revert：`revert(0, 0)`。
- `Panic(uint256)`：写 selector `0x4e487b71`，写 panic code，再 `revert(0, 0x24)`。
- `Error(string)`：写 selector `0x08c379a0`，再按 ABI 写字符串。
- custom error：写 4 字节 selector 和参数，再 revert。
- 低级 call 失败后，可能复制 returndata 并原样 revert。

提升目标：

- 区分 `assert` / arithmetic panic / ABI panic / 普通 require / custom error。
- custom error 暂时只保留 selector 和参数结构，不查错误名。
- 把 returndata 冒泡标成 `revert_bubble`，不要误判成用户自己构造的错误。

### 6. 小类型清理和类型线索

典型模式：

- `and ((1 << N) - 1)` 清理 `uintN`、`bytesN`、address。
- `signextend` 清理 `intN`。
- bool 常见为 `iszero(iszero(x))` 或范围归一。
- enum 可能有范围检查，非法值进 panic。
- 写 memory、storage、call data 前常做清理。

提升目标：

- 给值挂 `clean_uintN`、`clean_intN`、`clean_address`、`clean_bool` 等 metadata。
- address 的 160 bit mask 优先作为类型线索，不当普通位运算展示。
- 对 enum 只标“有上界检查”，枚举名字不在本阶段恢复。

### 7. Storage packed field 读写

典型模式：

- `sload(slot)` 后 shift/mask 取 packed field。
- 写 packed field 前先 `sload(slot)`，清目标位段，再 `or` 新值，最后 `sstore(slot, merged)`。
- bool、小整数、address、bytesN 都可能被打包。

提升目标：

- 标出 `storage_field_load(slot, bit_offset, bit_width)`。
- 标出 `storage_field_store(slot, bit_offset, bit_width, value)`。
- 这只是 Solidity storage layout 规则，不识别 ERC1967、Ownable、proxy slot 等具体业务含义。

### 8. Mapping 和动态数组 storage 地址计算

典型模式：

- 用 memory scratch 区写 key 和 base slot。
- `sha3(mem, 64)` 得到 mapping 元素 slot。
- 动态数组用 `sha3(slot)` 得到数据起点，再加 index。
- 嵌套 mapping / 数组会多层 sha3。

提升目标：

- 标出 `mapping_slot(base_slot, key)`。
- 标出 `dynamic_array_data_slot(base_slot)` 和 `array_element_slot(base, index, elem_width)`。
- 先恢复地址计算关系，不猜变量名。

### 9. bytes/string 的 storage 短长分支

典型模式：

- storage slot 低 bit 判断短/长编码。
- 短 bytes/string 数据在同一个 slot 高位，长度在低字节。
- 长 bytes/string 长度在主 slot，数据从 `sha3(slot)` 开始。
- 非法编码可能触发 `Panic(0x22)`。

提升目标：

- 标出 `storage_bytes_short`、`storage_bytes_long`、`storage_bytes_length`。
- 把 `Panic(0x22)` 标成 bytes/string storage 编码错误。
- 先只做识别，不把循环拷贝直接改写成高级字符串操作。

### 10. Memory 分配和动态对象构造

典型模式：

- `mload(0x40)` 取 free memory pointer。
- 按 32 字节对齐计算新对象大小。
- `mstore(0x40, new_free_ptr)` 更新指针。
- 动态数组 memory 里第一个 word 是 length，后面是元素。
- `0x60` zero slot 常用于空动态数组。

提升目标：

- 标出 `memory_alloc(size)`。
- 标出 memory dynamic array / bytes / string 的 length 和 data 起点。
- 让 ABI decode/encode pass 能复用这些对象信息。

### 11. 事件 log 编码

典型模式：

- 非 indexed 参数按 ABI 写到 memory。
- indexed 参数放到 `log1` 到 `log4` 的 topic。
- topic0 是事件签名 hash，匿名事件没有 topic0。
- 动态 indexed 参数会先 hash。

提升目标：

- 标出 `emit_event(topic_count, topics, data_ptr, data_size)`。
- topic0 只保留常量，不在本阶段查事件名。
- 对 ERC20/721 这类常见 topic 可作为评估样例，但 pass 不写死标准事件。

### 12. 外部调用和返回数据处理

典型模式：

- ABI encode call data 到 memory。
- `call` / `staticcall` / `delegatecall`。
- 检查 success。
- 失败时可能 revert bubble，成功时按 returndata ABI decode。
- 有时先检查 `extcodesize` 或 returndata size。

提升目标：

- 标出外部调用的 kind、target、value、input buffer、output buffer。
- 标出 `returndata_decode` 和 `revert_bubble`。
- `delegatecall` 是调用语义，proxy 属于库/源码模式；这里只恢复底层调用结构，不识别“某种代理”。

### 13. 算术 checked/unchecked 保护

典型模式：

- Solidity 0.8+ 默认插入 overflow / underflow 检查。
- 加减乘除、取负、类型转换都可能失败进 `Panic`。
- `unchecked` 块里没有这些检查。

提升目标：

- 标出 compiler checked arithmetic。
- 从 panic code 区分 overflow、division by zero、array out of bounds 等。
- 后端可显示成普通 `a + b` 并保留 `checked` metadata，避免输出一堆低层比较。

### 14. 数组边界和 slice 检查

典型模式：

- 比较 index 和 length。
- 失败进 `Panic(0x32)` 等。
- 对 memory/storage/calldata array 都可能出现。

提升目标：

- 标出 `array_bounds_check(index, length)`。
- 和业务 require 分开。
- 后续结合 storage/memory/calldata 对象恢复 `arr[i]`。

### 15. Immutables、constants、metadata 和 constructor 残留

典型模式：

- runtime 里 immutable 可能变成部署时 patch 进去的常量。
- constants 直接内联。
- runtime bytecode 尾部有 metadata hash，不属于可执行语义。
- constructor 参数和 codecopy 主要在 creation code，不一定出现在 runtime。

提升目标：

- 标出疑似 immutable 常量来源，但不猜变量名。
- metadata hash 不进入业务 CFG。
- 当前 apehex runtime 路线先以 runtime 为主，creation code 单独排期。

## Pass 规划

### Pass 0：SolidityPatternAnnotationPass

作用：

- 统一保存识别结果。
- 提供 metadata 名称、统计输出和 debug dump。
- 不改变 IR 语义。

这是其他 pass 的公共底座，避免每个 pass 自己发明 metadata 格式。

### Pass 1：SelectorInlinedLogicExtractionPass

负责：

- 识别 `public___function_selector___*` 里的 selector 分发区域。
- 保留 selector 分发函数本身，不强行拆 dispatcher。
- 找出 selector 比较链之外被内联进去的逻辑。
- 给 fallback/receive 或其他内联 body 加函数提取候选 metadata。

不负责：

- selector 查函数名。
- 业务函数重命名。
- 实际 clone / split 函数。第一阶段只标注候选区域。

### Pass 2：PayabilityGuardPass

负责：

- 识别 `callvalue == 0` 加 `revert(0,0)`。
- 给函数标 `nonpayable`。
- 标出 fallback/receive 的 payable 状态。

### Pass 3：AbiDecodePass

负责：

- public/external 参数读取。
- calldata bounds check。
- 静态和动态参数的 ABI 结构。
- 参数清理带来的类型线索。

### Pass 4：AbiReturnPass

负责：

- 识别 ABI return buffer。
- 识别静态和动态返回值编码。
- 给 return 指令或函数返回区挂 metadata。

### Pass 5：SolidityRevertPass

负责：

- 空 revert、`Error(string)`、`Panic(uint256)`、custom error。
- returndata bubble。
- panic code 分类。

### Pass 6：ValueCleanupTypeHintPass

负责：

- mask、shift、`signextend`、bool 归一。
- 输出 address/bool/intN/uintN/bytesN/enum 的低置信度类型线索。

这个 pass 可以被 ABI decode、ABI return、storage pass 复用，也可以在它们之后补充全局线索。

### Pass 7：StorageFieldPass

负责：

- packed storage load/store。
- slot、bit offset、bit width。
- transient storage 可以复用同一套识别逻辑，但 metadata 要区分 storage 和 transient storage。

### Pass 8：StorageAddressingPass

负责：

- mapping slot。
- dynamic array data slot。
- nested mapping / array 的 sha3 链。
- bytes/string storage 短长编码。

### Pass 9：MemoryObjectPass

负责：

- free memory pointer 分配。
- memory dynamic array / bytes / string 对象。
- zero slot。

### Pass 10：EventLogPass

负责：

- `log0` 到 `log4`。
- topic 和 data buffer。
- 匿名事件和普通事件的区别。

### Pass 11：ExternalCallPass

负责：

- `call`、`staticcall`、`delegatecall`、`callcode` 的语义标注。
- call data encode、return data decode。
- success 检查和 revert bubble。

不负责：

- 识别 OpenZeppelin proxy、ERC1967、minimal proxy 这些库/源码模式。

### Pass 12：CheckedOperationPass

负责：

- Solidity 0.8+ checked arithmetic。
- array bounds panic。
- enum / conversion panic。

这个 pass 要依赖 `SolidityRevertPass` 的 panic 识别结果。

## 推荐阶段

第一阶段先做不会改变 IR 的 annotation：

1. `SolidityPatternAnnotationPass`
2. `SelectorInlinedLogicExtractionPass`
3. `PayabilityGuardPass`
4. `AbiReturnPass`
5. `SolidityRevertPass`

这一阶段能最快改善输出：selector 函数里的内联逻辑、nonpayable、return、panic/revert 都会更清楚。

第二阶段做类型和对象：

1. `ValueCleanupTypeHintPass`
2. `AbiDecodePass`
3. `MemoryObjectPass`
4. `StorageFieldPass`
5. `StorageAddressingPass`

这一阶段风险更高，因为需要跨块追踪值和 memory 写入顺序。

第三阶段做跨调用和事件：

1. `EventLogPass`
2. `ExternalCallPass`
3. `CheckedOperationPass`

这些更容易和业务逻辑交织，建议有足够样例后再做替换型 intrinsic。

## 风险

- 编译器版本差异很大。Solidity 0.4、0.5、0.8，以及 via-IR / legacy codegen 的模式会不同。
- optimizer 会重排、合并或删除很多中间块，matcher 不能只按固定基本块形状写。
- ABI decode、memory allocation、revert encoding 都会共用 `mstore`，需要先有轻量 dataflow，不能只按相邻指令匹配。
- 不能把库模式写进 compiler pass。ERC1967、Ownable、ERC20、ERC721 都只能作为评估样例，不应成为 Solidity 编译器模式识别规则。
- 第一阶段最好只加 metadata，不改 CFG。等误报率低了，再考虑替换成高层 intrinsic。

## 判断标准

- 在 `20260520-evm2llvm-train-batch010` 这批输出上，pass 能统计每类模式的命中数量，并能 dump 到文本或 JSON。
- 在旧 plan 的 0394 proxy 样例上，能识别 `function_selector` 里的 selector 分发边界、receive/fallback 内联 body 候选、`implementation()` 的 nonpayable guard、address return ABI encode、delegatecall returndata bubble；但不输出 ERC1967 implementation slot 语义。
- 在包含普通业务函数的样例上，compiler guard、panic、ABI bounds check 不应被当成业务 require。
- 新 pass 默认 annotation-only 时，IR verifier 通过，原有 evm2llvm 输出语义不变。
- 这次只是写 plan，不涉及主 NotDec pass pipeline；后续若接入主 pipeline，再按项目规范对 fortune 当前关注用例做同口径时间对比。

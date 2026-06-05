# EVM 类型结果驱动的语义恢复计划

## 原始 prompt

这失败的三个和之前超时的2个都在哪里记录一下，然后先跳过。如果类型恢复结果还可以，规划一下后续，即基于类型推理结果的HType做后面更多的语义识别。参考logs/20260602-05-evm-memory-object-type-recovery-pass-ordering-plan.md，然后重新写一个新的规划文件吧

## 背景

selected-apehex-80 这轮结果说明类型恢复结果基本可用：

- 80 个样本里先跳过 2 个旧 `notdec_tr_timeout`：`7435`、`18404`。
- 剩下 78 个里，75 个成功生成 HType。
- 3 个失败不在类型恢复阶段：
  - `13930`、`22492`：Gigahorse timeout。
  - `26708`：`evm2llvm` 的 `PHIIncoming missing predecessor`。

这 5 个先不作为后续语义恢复质量样本。跳过记录写在：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/selected-apehex-80/notdec-htype-20260604-eafa002-noverify/SKIPPED_CASES.md`

类型恢复结果不应该再通过一个额外的 HType 汇总层转一遍。当前 `MLsubRecovery::getResult(Mod, MAM)`
已经是后续 pass 可以消费的 analysis/result 入口，`--dump-htypes` 只是调试和 oracle 输出。

所以下一步应该按语义分类拆 pass：每个 pass 明确负责哪类 Solidity/EVM 语义，直接读取类型恢复结果和
Memory Object analysis。pass 如果 rewrite IR，需要同步维护相关类型结果，或者明确让类型恢复相关 analysis
失效并重算，不能拿旧 HType 继续解释新 IR。

## 总目标

1. 后续语义 pass 直接消费类型恢复 result，不新增只读 HType 汇总层。
2. 按语义分类拆责任：storage、external call、ABI return、revert、event、public ABI 参数分别负责自己的边界。
3. rewrite IR 时同步维护类型信息，至少保证被替换 value 的类型关系还能追踪。
4. 类型恢复结果只作为证据，不把 `integer/address/storage_key` 粗暴扩大成源码语义。

## 总顺序

推荐顺序：

1. Memory Object rewrite / access analysis 保持在类型恢复之前。
2. 类型恢复主流程生成 `MLsubRecovery::Result`。
3. 语义 pass 按下面顺序消费类型结果：
   - Storage recovery / StorageField pass
   - ExternalCallPass
   - AbiReturnPass
   - SolidityRevertPass
   - EventLogPass
   - ABI parameter recovery pass
4. 最后统一做低层 helper cleanup。

这个顺序的原因是：

- storage 和 external call 的边界比较明确，能先产出稳定语义标签。
- return/revert/event 都依赖 memory object 字段类型，应该在类型恢复后做。
- public ABI 参数需要结合 selector/public entry、calldata、bounds guard 和类型结果，放后面更稳。

## 类型结果使用方式

后续 pass 不解析 `.htypes` 文本。应该通过 `MLsubRecovery::Result` 查询：

- 某个 `Value` 的 lower / upper HType。
- 某个函数参数、返回值、call result 的类型。
- 某个 memory object field 的类型。
- semantic primitive：`prim.uint256.evm.address`、`integer`、`storage_key`。

如果现有 result API 不方便，需要先补一个很薄的查询接口，例如：

- `getValueHType(Value *)`
- `hasSemantic(Value *, "address")`
- `hasSemantic(Value *, "storage_key")`
- `getMemoryFieldType(Object, Offset)`

这个接口只是访问类型恢复结果，不做新的语义归纳。

## Rewrite 时维护类型结果

语义 pass rewrite IR 时要遵守一个规则：不能让类型结果和 IR 脱节。

可选做法：

1. 小 rewrite：维护 value replacement 映射。
   - 例如把 `%evm.call` 包成 `notdec_evm_external_call(...)`，新 call 的返回值继承旧 `%evm.call` 的类型。
   - 删除低层 marker 前，把旧 value 到新 value 的类型映射记录进类型结果或 pass-local rewrite map。
2. 大 rewrite：让类型恢复 analysis 失效并重算。
   - 如果重写了大量 memory access、CFG、函数签名，就不要复用旧 result。
   - pass manager 里应明确 `PreservedAnalyses`，避免误用旧分析。
3. 只加 metadata / marker：可以保留类型结果。
   - 如果只是给 instruction 加语义 metadata，不改变 value def-use，通常可以 preserve 类型结果。

短期先做第 1 种和第 3 种。第 2 种成本高，等真的有大 rewrite 再处理。

## 语义 pass 分工

### 1. Storage recovery / StorageField pass

负责：

- 识别 `evm_sload` / `evm_sstore`。
- 从类型结果确认 key 是否是 `storage_key`。
- 记录 storage read/write 的 key、value、所在函数、上下文。
- 识别简单 constant slot、`add(base, offset)` slot。

暂不做：

- 不从 keccak 反推 mapping。
- 不合并复杂 storage struct。

rewrite：

- 初期只加 metadata 或 side-table，不删除原始 `sload/sstore`。
- 如果后续改成高层 `notdec_storage_load(slot)`，新 value 继承原 load result 类型。

### 2. ExternalCallPass

负责：

- 识别 `evm_call`、`evm_staticcall`、`evm_delegatecall`、`evm_callcode`。
- 用类型结果确认 target 是否是 `address`，gas/value/status/size 是否是 `integer`。
- 绑定 input buffer、output buffer 和 call result。
- 输出 call kind、target、value、gas、input object、output object。

暂不做：

- 不识别 proxy/library 模式。
- 不完整 decode call input ABI。

rewrite：

- 可以把低层 call 标成 external-call marker。
- 如果生成新 helper call，返回值继承旧 call status 的类型。
- input/output buffer 的字段类型从 memory object 类型结果读取，不自己重新推。

### 3. AbiReturnPass

负责：

- 识别 `evm_return(mem, base, size)`。
- 查 return buffer object 的字段类型。
- 生成 return tuple / dynamic return 候选。
- 标记哪些 `mstore` 是 return buffer 构造。

暂不做：

- 不完整展开动态 bytes/string/array。
- 不把所有 return word 都强行命名。

rewrite：

- 初期只标记 return buffer 和字段。
- 真正隐藏低层 `mstore` 时，要把字段 value 的类型关系转移到新的 return 表达式上。

### 4. SolidityRevertPass

负责：

- 识别 empty revert、Panic(uint256)、Error(string)、custom error 候选。
- 查 revert buffer object 的 selector word 和参数字段类型。
- 保留 checked/bounds pass 已经识别的 compiler guard 结果。

暂不做：

- 不查 selector database。
- 不恢复完整 custom error 名字。

rewrite：

- Panic / Error marker 可以继承 buffer 字段类型。
- 删除低层 revert buffer 写入前，要确认这些写入没有被其他路径复用。

### 5. EventLogPass

负责：

- 识别 `evm_log0..4`。
- topics 和 data buffer 分开处理。
- topics 里如果有 `address` / `integer` 类型，作为 topic 参数类型证据。
- data buffer 字段类型来自 memory object 类型结果。

暂不做：

- 不查 event signature database。
- 不恢复事件名。

rewrite：

- 初期只加 event marker。
- 如果后续把 log 改写成高层 event emit，参数 value 继承 topic/data field 类型。

### 6. ABI parameter recovery pass

负责：

- 基于 selector/public entry、calldata load/copy、bounds guard 和类型结果恢复 ABI 参数候选。
- 区分静态 word、动态 head offset、tail length/data。
- 对 address/bool/uint/int 候选只在有上下文时提升语义。

暂不做：

- 不一次性恢复完整 ABI。
- 不用 selector database 反查函数签名。

rewrite：

- 如果把 calldata access 改写成参数 value，新参数 value 继承原 calldata-derived value 的类型。
- 被隐藏的 bounds guard 需要保留“这是 ABI bounds”的语义标记，方便调试。

## 验证

基础验证：

- selected-apehex-80 跳过 5 个问题样本，其余 75 个继续能出 HType。
- 新增语义 pass 后，至少抽查这些样本：
  - `1111`：有 storage_key / address / integer，规模适中。
  - `20900`：HType 最大，能测性能。
  - `29192`：HType 很小且 semantic 为空，验证 pass 不误报。

性能要求：

- 类型恢复和语义 pass 分开计时。
- 语义 pass 不应该让 `20900` 这类样本明显变慢。

质量要求：

- storage pass 的 key 应该主要来自 `sload/sstore`。
- external call pass 不应该把 memory base 标成 `integer`。
- return/revert/event pass 的 buffer 字段必须能追到 memory object 类型结果。

## 暂不做

- 不做 HType 文本解析式的汇总层。
- 不做 keccak 到 mapping 的反推。
- 不做 selector/event/error database 查询。
- 不做完整动态 ABI 展开。
- 不把所有 `uint160` 直接叫 address。


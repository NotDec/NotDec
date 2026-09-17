# Solidity 后端：storage helper 映射与 fallback 稳定化

承接 `logs/20260917-01-ir-and-solidity-backend-audit.md`。目标是把
pattern 生成的 Solidity 从“能生成但大量 undeclared”推进到类型层面可编译，
同时尽量保留可恢复的高层语义。

## 本轮改动

### 1. contract-level storage slot 信息

- 新增 `include/notdec-backends/Solidity/StorageInfo.h`
  - `StorageSlotMap`：top-level slot number -> `{Name, IsMapping, IsArray}`。
- `Reader::readStateVariables()` 从 HType `StorageDecl` 提取 slot number、
  状态变量名和容器形状：
  - `slot:0` / `slot_0` 两种命名都支持；
  - mapping / dynamic/static array 传给 body builder 做表达式恢复。
- `TypePrinter` 新增 `formatStateVariableType()` / `isMappingType()` /
  `isArrayType()`，把 HType 的 `slot:N.map.key/value`、`dynamic_array.elem`
  映射成 Solidity `mapping(K => V)` / `T[]` 声明。

### 2. `evm.storage.*` helper 表达式恢复

`BodyBuilder::valueExpr()` 现在识别：

- `evm.storage.slot` / `evm_sload` 常量槽 -> `slot_N`；
- `evm.storage.load` -> ref 表达式；若是数组槽再进一步映射为 `.length`；
- `evm.storage.packed.load` / `field` -> ref 表达式；
- `evm.storage.map.value` -> 直接 mapping 槽转 `slot[key]`；
- `evm.storage.dynamic_array.elem` / `static_array.elem` -> 数组槽转 `slot[i]`；
- `dynamic_array.length.load` -> `slot.length`；
- bytes/string 系列 -> 退化为 ref。

嵌套 mapping value、非直接 slot 的复杂 path 暂不硬猜：类型不可索引时退回
base 表达式，避免生成非法下标。

### 3. 未恢复值的统一 fallback

之前未识别的 SSA 值会直接打印 `private.call`、`evm_and` 这类原始名字，
solc 报 `Undeclared identifier`。现在：

- 未识别的 call 结果、`extractvalue`、未 materialize 的 `load` 在表达式中
  打印成：
  `0 /* TODO: unresolved value: <llvm name> */`；
- `evm_caller` 在整数位运算里自动转 `uint256(uint160(msg.sender))`；
- 除数为 unresolved 表达式时不再生成 `0 / 0`，而降级为 unresolved fallback。

### 4. ABI 返回/事件类型推断

- `returnsSingleBoolWord()` 支持 MemoryBufferAnalysis 后的 calloc-backed
  dynamic one-word return，函数可正确声明 `returns (bool)`；
- event topic 参数类型识别 `address` / `bool` / `uint256`，同名 event 多次
  emit 时合并参数类型；
- `formatEventStatement()` 根据 event 参数类型插入
  `address(uint160(...))` / `uint256(uint160(msg.sender))` cast；
- 重名 outlined entry 自动加 `_2` 后缀，避免 Solidity 同名同参函数重复定义。

## 验证

`notdec.evm.solidity_source` 78/78 通过，且全部 78 份输出继续可被
solc 0.8.25 编译。

pattern 全量 103 个 case：

```bash
for case in test/evm/solidity-patterns/cases/*.ll; do
  notdec "$case" -o /tmp/out.sol --tr-level=2
  solc-0.8.25 --bin /tmp/out.sol
done
```

结果：

- 本轮修改前：25/103 可编译（sanitize + data location 修复后 29/103）；
- 本轮修改后：**103/103 全部可编译**。

语义质量现状：

- 已能表达直接 storage getter、mapping/array 部分访问、bool/address 事件参数、
  简单 ABI bool/word return；
- 仍未恢复的值会非常显眼地标成
  `0 /* TODO: unresolved value: ... */`，不会伪装成正确值；
- storage store/setter 仍 intentionally 关闭（`formatStorageStore()` hook
  保留），因为 RHS 还缺 def-use/类型恢复，直接赋值容易产生 array slot
  类型错误或 packed 语义错误。

## 主链路回归

以下全部通过：

- `notdec.evm.solidity_patterns`
- `notdec.evm.solidity_rewrite`
- `notdec.evm.solidity_source`
- `notdec.type_recovery.evm.tr_level_2`
- `notdec.lifting.wasm`

## 后续

1. 恢复 `evm.storage.store` / `evm_sstore` 的 setter 输出：
   需要先补 RHS 的 demoted value materialization / 类型修复，避免重新引入
   本轮修掉的类型错误。
2. 把嵌套 mapping / struct storage path 从“退回 base 表达式”推进到真实
   Solidity member/index 表达。
3. pattern audit 固化为 CI：至少抽样跑 `solc --bin`，防止类型层面回退。
4. 处理 unresolved value 的语义恢复优先级：private call inline / helper 函数输出。

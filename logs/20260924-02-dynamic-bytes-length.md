# 20260924-02 动态 calldata 参数的长度恢复（bytes/string）

## 0. 一句话

按“对着 EVM 测试用例修”的要求，选了有源码、能对照的 `revert_error_string_04`
（`function run(bytes calldata data)`）和一个真实合约 1485（LayerZero
`lzReceive(uint16, bytes, uint64, bytes)`）：签名能恢复，但所有依赖动态参数的条件都退化成
`false /* TODO */`、返回值退化成 `0 /* TODO */`。根因是 `EvmCalldataAccessPass` 只标注常量偏移
的 ABI head 字（`notdec.solidity.calldata.index`），动态参数的 ABI tail（长度字在 `head + 4`，
元素字在 `head + 4 + 32 + 32k`）完全没标注；后端对 `evm_calldataload` 也没有任何表达式映射。
本轮补上长度字：新 metadata `notdec.solidity.calldata.length` + 后端渲染 `<arg>.length`
（`bytes`）/ `bytes(<arg>).length`（`string` 在 Solidity 里没有 `.length` 成员），并放宽条件渲染里
“含动态参数就丢弃”的判断。103 case 语料
condition TODO 4494→4440、unresolved 289→288，没有任何 case 变差；预算已按仓库约定下调。

## 1. 例子与现状

源码（`test/evm/solidity-patterns/solidity/revert_error_string_04.sol`）：

```solidity
function run(bytes calldata data) external pure returns (uint256) {
    require(data.length > 3, "calldata too short");
    return data.length;
}
```

修复前输出（fixture 的函数名没有类型编码，签名是空的）：

```solidity
function public_0x3a276523() public returns (uint256 ret0) {
    require(msg.data.length > 35);                         // ABI head 大小守卫，OK
    require(false /* TODO: evm.gt */);                     // offset < 2^64
    require(false /* TODO: evm.sgt */);                    // calldatasize > offset+35
    require(false /* TODO: evm.gt10 */);                   // length < 2^64
    require(!false /* TODO: evm.gt17.not */);              // offset+len+32 <= calldatasize
    require(false /* TODO: evm.gt23 */, "calldata too short");  // 源码的 require
    return 0 /* TODO: unresolved value: evm.calldataload9.load */;
}
```

修复后（fixture 名字按约定补上 `bytes`）：

```solidity
function run(bytes calldata arg0) public returns (uint256 ret0) {
    require(msg.data.length > 35);
    require(false /* TODO: evm.gt */);                     // head offset 检查，仍 TODO（见 §6）
    require(false /* TODO: evm.sgt */);
    require(arg0.length < 18446744073709551616);
    require(!false /* TODO: evm.gt17.not */);
    require(arg0.length > 3, "calldata too short");        // 源码 require 恢复
    return arg0.length;                                    // 源码 return 恢复
}
```

真实合约 1485（`public_lzReceive_uint16_bytes_uint64_bytes__0x2b2`）：修复前
`require(false /* TODO */)` / `if (!false /* TODO */)` 一片；修复后
`require(arg1.length <= ...)`、`require(arg3.length <= ...)` 等能读出来。

## 2. 偏差从哪来（链路）

1. pre-TR 的 `EvmCalldataAccessPass` 把 `evm_calldataload` 改写成普通 load，并只给
   **常量偏移** `4 + 32*i` 的 head 字打 `notdec.solidity.calldata.index`（见
   `annotateCalldataArgumentIndex`，要求 `dyn_cast<ConstantInt>(Offset)`）。
   动态参数的 tail 读（`%evm.add = head + 4` 之后的 load）不满足常量条件，没有标注。
2. 后端 `valueExpr` 解析 calldata load 只有一条路：`annotatedAbiArgumentExpr` →
   参数名。而且即使 head 字有标注，`isWordLikeParameter` 对 `bytes/string/数组` 参数会拒绝——
   这是**正确**的：head 字是 ABI offset，不是参数值本身。
3. 于是动态参数相关的 load 全部落进 `UnresolvedValueExpr`；`getCondition` 只要表达式里含
   未解析值（或含 `bytes/string` 标识符）就整体退化成 `false /* TODO: <值名> */`，
   源码的 require/return 表达式就丢了。
4. 另外，Solidity 参数表来自 entry 函数名的类型编码（`Reader::applyFunctionNameAndParams`，
   `public_<name>_<types>__0x<sel>`，`parseAbiParameters` 支持 `bytes`/`string` 并加
   `calldata` location）。真实语料的名字是编码过的（`public_lzReceive_uint16_bytes_...`），
   而手写的 pattern fixture `revert_error_string_04.ll` 只写了 selector，所以签名也是空的。

## 3. 是否合理

- 控制流形状合理：solc 为动态参数生成的 ABI decoder 检查、error string、fallback 都识别出来了，
  错误消息 `"calldata too short"` 完整保留。
- 表达式丢失不合理：`return 0 /* TODO */` 是语义错误级别的退化；`require(data.length > 3)`
  变成 `require(false /* TODO */)` 让输出不可用。这部分是本轮要修的。
- head offset 的三个检查（offset < 2^64、calldatasize > offset+35、offset+len+32 <= calldatasize）
  是 ABI decoder 的内部校验，Solidity 里没有对应可写的“offset 值”；合理做法是后续把这些
  decoder 脚手架整体折掉（见 §6），不是硬渲染。

## 4. 修复

1. `src/Passes/evm/SolidityPatterns.{h,cpp}`：新增 metadata key
   `notdec.solidity.calldata.length`（值是该动态参数的 ABI index）。
2. `EvmCalldataAccessPass`：新增 `annotateCalldataDynamicLength`——当被改写的 load 的偏移是
   `head + 4`（两种操作数顺序都认），且 head 是指令上带 `calldata.index` 的 load 时，给新
   load 打上 length 标注。纯后端接口标注，不改 IR 语义。
3. `BodyBuilder`：新增 `annotatedAbiLengthExpr`，把 length 标注渲染成 `<arg>.length`
   （`bytes`）或 `bytes(<arg>).length`（`string`；Solidity 的 `string` 没有 `.length` 成员，
   第一版直接写 `arg.length` 被 solc 拒绝，5 个 string 参数的 case 在编译预算里报
   "Member length not found ... in string calldata"，改为 `bytes(s).length` 后通过。
   动态数组的长度字是字节数、`.length` 是元素数，先不接）。
   放在 `valueExpr` 的 LoadInst 分支最前面，这样条件、return 存值都走同一条路。
4. `getCondition` 的“含 `bytes/string` 标识符就丢弃”换成 `containsBareDynamicValue`：
   只拒绝**裸用**动态参数（`if (data)` 这类），`data.length` / `data[i]` 是 word 类型，允许
   进入条件。
5. fixture `revert_error_string_04.ll` 的 entry 改名 `public_run_bytes__0x3a276523`，与真实语料
   的命名约定一致；pattern manifest 为三个含动态参数的 case 固定新 metadata 计数与取值，
   并把 `notdec.solidity.calldata.length` 加进 runner 的 `METADATA_ONLY_KINDS`（和
   `calldata.index` 一样只有后端注解、没有 rewrite marker，否则 runner 会去数一个不存在的
   `notdec_solidity_rewrite_calldata_length`）。
6. 新增源码锚定的回归 `test/evm/solidity-source/{cases,ir,expected}/dynamic_bytes_length_01`
   （源 = `run(bytes calldata)` 的 require/return，IR 用该 pattern case 的冻结输入），
   把渲染出的 `require(arg0.length > 3, ...)` / `return arg0.length;` 钉住。

## 5. 验证与指标

- `notdec.evm.solidity_patterns`：通过（103 case 的 metadata oracle，含新标注的 pin）。
- `notdec.evm.solidity_source`：通过（新增 `dynamic_bytes_length_01`）。
- `notdec.evm.solidity_pattern_compile`（103 case + solc）：通过；`arg.length` 表达式过 solc 0.8.25。
- 逐 case 全语料（`/tmp/metrics-fixed.tsv` → `/tmp/metrics-lenfix.tsv`）：
  condition TODO **4494 → 4440**、unresolved **289 → 288**、goto/real/dangling 不变，
  **没有任何 case 的 TODO 上升**。收益最大：1485 −13、24601 −13、2001 −12、
  `revert_error_string_04` −2。
- 预算（`compile-budget.json`）：`max_condition_todo_occurrences` 4494→4440、
  `max_unresolved_value_occurrences` 289→288，comment 里记录了原因指向本文档。

## 6. 遗留

- **元素访问**：`data[i]` 的字在 `head + 4 + 32 + 32k`，还没标注/渲染；这是同一套 matcher 的
  下一步（标注 `calldata.element = i:k`，后端渲染 `data[k]`）。
- **decoder 脚手架折叠**：三个 head-offset 检查（以及动态数组的重复 shape）应作为 solc ABI
  decoder 的内部校验整体识别并折掉，而不是渲染成 TODO；这需要 matcher 同时看 head/len/tail
  三个检查的关系。
- **动态数组**：length 标注已经打上（`checked_bounds_array_01`/`checked_bounds_calldata_array_01`
  的 manifest 已 pin），但后端不渲染（元素宽度问题），等元素访问一起做。
- 真实语料里仍有 `public__0x…` 这种没编码类型的 entry（fixture 生成侧的类型恢复失败），
  它们的签名/参数表只能等 ABI 参数识别那条线补齐。

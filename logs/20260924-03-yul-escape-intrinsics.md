# 20260924-03 Yul escape 原型：calldata 内建与 ABI 校验的可表示化

## 0. 一句话

按「不追求严格 Solidity、允许底层可表示，参考 Yul」的方向做了原型：**Yul 作 escape 层，
Solidity 仍是主输出**。pre-TR 给每个被改写的 calldata word load 打
`notdec.solidity.calldata.raw`；后端解析阶梯变成「ABI head 参数名 → `.length` →
`__notdec_calldataload(off)` intrinsic → 才轮到 unresolved」。intrinsic 是合约级
`internal pure` 函数、函数体一行 inline assembly（Yul），所以输出**仍然过 solc**。
ABI decoder 的 offset 校验不再是无法表达的 TODO：04 从 3 条 TODO 变 0，
1485 从 140 变 52；103 case condition TODO **4494 → 3452（−1042）**、
unresolved **289 → 216**、goto/real/dangling 不变、没有任何 case 变差。

## 1. 设计：为什么是「intrinsic 函数 + inline assembly」而不是新方言

- 硬约束是 `compile-budget.json` 的 `max_compile_failures: 0`：自造方言会让 103 case 的
  solc 验证网失效，下游 golden 也会断。
- Solidity 的 inline assembly 就是 **Yul**：solc 原生支持、可编译，
  `calldataload/calldatasize/sload/mload/...` 都在。
- 但 assembly 是**语句**，不能放进 `if (...)`/`require(...)` 的表达式位置。所以：
  - **表达式位置** → intrinsic 函数（`__notdec_calldataload(off)`），函数体是
    `assembly { value := calldataload(offset) }`；
  - **语句位置**（以后要做 mstore/calldatacopy/多字操作）→ 直接 `AssemblyStatement`（AST 里
    已有这个节点、Printer 已支持，只是此前没人构造）。
- 抄 Yul 的值世界统一（u256）和显式内建语义，不把整份输出做成 Yul（那会丢掉类型/ABI/事件/
  storage layout 这些最值钱的恢复结果）。

## 2. 实现

1. `SolidityPatterns.{h,cpp}`：新 key `notdec.solidity.calldata.raw`。
2. `EvmCalldataAccessPass::rewriteCalldataLoad`：每个被改写的 load 都打 raw 标记
   （表示「这是一次直接 calldata word 读」）。
3. `BodyBuilder`：
   - LoadInst 的解析阶梯：`annotatedAbiLengthExpr`（`.length`/角标）→
     `annotatedAbiArgumentExpr`（ABI word 参数）→ `rawCalldataLoadExpr`（新）→ 原 fallback。
   - `rawCalldataLoadExpr`：识别 pass 生成的
     `inttoptr(add(ptrtoint(<calldata base>), off))`，base 接受 `%calldata` 形参或
     `notdec_evm_calldata_{view,min_size}` 调用，渲染 `__notdec_calldataload(valueExpr(off))`；
     同时把 intrinsic 记进 `usedIntrinsics()`。
   - 未改写的 `evm_calldataload` call 也走同一个 intrinsic（兜底）。
   - `thread_local std::set<std::string> ActiveIntrinsicUses` +
     `clearUsedIntrinsics()/usedIntrinsics()`。
4. `Reader::readContract`：开始清空 intrinsic 集合；渲染完所有函数后，为用到的 intrinsic
   生成 prelude 函数（`internal pure`，函数体是 AssemblyStatement）。
5. 防串名（24541 暴露的）：outlined entry 的尾随 word formal 会和动态 ABI 参数同名
   （都叫 `arg1`），于是生成出 `__notdec_calldataload(arg1 + ...)` 这种
   `bytes calldata + uint256` 的非法表达式。加了两道 `containsBareDynamicValue` 保护：
   - raw offset 表达式里含裸动态标识符 → 放弃 intrinsic，保持 unresolved；
   - `valueExpr` 的 Argument/SSA-name 路径不再把动态类型名字当 word 标识符返回。
   这类形状退回 TODO（不比原型前差），但不再产生编不过的代码。

## 3. 效果

04（`run(bytes calldata data)`），修复后 `TODO=0`：

```solidity
require(__notdec_calldataload(4) < 18446744073709551616);
require(msg.data.length > __notdec_calldataload(4) + uint256(35));
require(arg0.length < 18446744073709551616);
require(__notdec_calldataload(4) + uint256(4) + arg0.length + uint256(32) <= msg.data.length);
require(arg0.length > 3, "calldata too short");
return arg0.length;
...
function __notdec_calldataload(uint256 offset) internal pure returns (uint256 value) {
    assembly { value := calldataload(offset) }
}
```

1485 LayerZero `lzReceive`：TODO 140 → 52，两个 `bytes` 参数的整套 decoder 前导可读：

```solidity
require(__notdec_calldataload(36) <= (uint256(1) << uint256(64)) + uint256(type(uint256).max));
require(__notdec_calldataload(36) + uint256(35) < msg.data.length);
require(arg1.length <= ...);
require(__notdec_calldataload(36) + uint256(4) + arg1.length + uint256(32) <= msg.data.length);
```

指标（103 case，相对本轮起点）：

| 指标 | before | after | delta |
| --- | --- | --- | --- |
| condition TODO | 4494 | **3452** | −1042 |
| unresolved value | 289 | **216** | −73 |
| helper call sites | 2421 | **2751** | +330（原先是未解析实参，现在渲染成调用） |
| helper functions | 1313 | 1313 | 0 |
| goto / real / dangling | 1192 / 1111 / 5 | 同 | 0 |
| 变差的 case | — | 0 | — |

收益最大：24601 −136、0189 −119、1485 −99、2001 −86、0258 −54、26499 −35、25596 −29。

## 4. 验证与预算

- `notdec.evm.solidity_patterns`、`notdec.evm.solidity_source`（`dynamic_bytes_length_01`
  的 golden 已按新输出更新）、`notdec.evm.solidity_pattern_compile`（103 case + solc 0.8.25）
  全绿；solc 失败 0。
- 预算重锚：condition TODO 4440→3452、unresolved 288→216、
  helper floors 1276/2408→1313/2751；goto/real/dangling 不变。
- 新增源码锚定：`notdec.evm.solidity_source/dynamic_bytes_length_01` 现在同时钉住
  `arg0.length` 语义和 `__notdec_calldataload` 的 ABI 校验形态。

## 5. 下一步（同一阶梯，按收益排序）

1. **memory/storage intrinsic**：`__notdec_mload(off)`、`__notdec_sload(slot)`、
   `__notdec_msize()`、`__notdec_sha3(off,len)`。剩余 3452 条 TODO 里很大一部分条件的叶子是
   `evm_sload`/`evm.mload`/`evm.mloadNN`，接上后能再降一截。注意 `sload` 的 prelude 要用
   `view`（调用者不是 pure，没问题），`mload` 先确认 solc 的 pure 限制。
2. **语句位置 escape**：`mstore/calldatacopy/memcpy/return` 这些目前靠 `AbiReturnPass`/
   `notdec.solidity.abi_return` 走高层；剩下没折掉的用 `AssemblyStatement` 直接输出 Yul。
3. **折叠 matcher**：有了 intrinsic 形态，`__notdec_calldataload(4)` 这类可以再被折叠回
   `arg0` 相关的语义（或判定为 decoder 内部校验后整体省掉），高层/低层两种表示现在都存在，
   折叠只是重写，不再丢信息。
4. **raw 标记进 pattern oracle**：把 `notdec.solidity.calldata.raw` 计数加进若干 pattern case 的
   `expected_metadata_counts`（并按需加入 runner 的 `METADATA_ONLY_KINDS`），让 escape 的使用量
   也受棘轮约束。

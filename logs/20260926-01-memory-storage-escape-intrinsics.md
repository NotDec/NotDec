# 20260926-01 memory/storage escape 阶梯：mload/sload/sha3/msize/callvalue

## 0. 一句话

沿用上一轮的 escape 阶梯，把剩下的低层 word 读取接成 intrinsic：`__evm_mload(off)`、
`__evm_sload(slot)`、`__evm_sha3(off,len)`、`__evm_msize()`、`__evm_callvalue()`。
103 case condition TODO **3452 → 3211（−241）**、unresolved **216 → 155（−61）**、
real jump 1111 → 1110；25 个 case 变好，唯一变差的是 case 24612 的 dangling goto 5 → 6
（下面第 4 节记录原因，已按约定写进 budget 注释）。

同一轮还补了**语句位置**的 `__evm_sstore(slot, value)`：`formatStorageStore` 只处理具名 slot，
未识别的 `evm_sstore` 此前**不产生任何 statement**（静默丢掉副作用）。接上 escape 后 189 条存储写
回到输出，指标零代价（TODO/unresolved/goto/dangling 全部不变）、103 case 仍然全过 solc。

## 1. 起因：`evm.branch.cond.not` 是当时最大的单点 TODO

按剩余 condition TODO 的名字排序，第一名是 `evm.branch.cond.not`（171 条），远高于
`evm.lt.*`（各 20~40）。它不是一个命名差错：`--print-after-all` 显示

```llvm
; *** IR Dump After InstCombinePass ***
%evm.branch.cond.not = icmp eq i256 %evm.callvalue, 0
br i1 %evm.branch.cond.not, label %bb.ok, label %bb.revert
```

即 LLVM 的 InstCombine 把 `icmp ne %evm.callvalue, 0` + `br` 反转成 `icmp eq ..., 0` 并把新值命名
为 `<原名>.not`。这份条件本身完全可表达（`require(callvalue == 0)`，就是 non-payable 的
payability guard），真正卡住的是叶子 `evm_callvalue`：`evmEnvBuiltinExpr` 里刻意没有它，注释写的
理由是「msg.value 在 non-payable 函数里非法」。这个理由**是成立的**（solc 0.8.25 实测：
`"msg.value" and "callvalue()" can only be used in payable public functions`，且 assembly 里直接写
`callvalue()` 在 `pure` 函数里也会报错），但结论不该是「整条条件退化成 TODO」。
所以 callvalue 走 escape：`internal view` 的 `__evm_callvalue()`，函数体
`assembly { value := callvalue() }`。

## 2. 实现

新增的 prelude intrinsic（`Reader::makeIntrinsicFunction`，每个合约按用到的集合生成一次）：

| intrinsic | 签名 | mutability | Yul 体 |
| --- | --- | --- | --- |
| `__evm_mload` | `(uint256 offset) returns (uint256 value)` | pure | `value := mload(offset)` |
| `__evm_msize` | `() returns (uint256 value)` | pure | `value := msize()` |
| `__evm_sha3` | `(uint256 offset, uint256 size) returns (uint256 value)` | pure | `value := keccak256(offset, size)` |
| `__evm_sload` | `(uint256 slot) returns (uint256 value)` | **view** | `value := sload(slot)` |
| `__evm_callvalue` | `() returns (uint256 value)` | **view** | `value := callvalue()` |

`sload`/`callvalue` 读的是状态/环境，在 `pure` 里 solc 直接报错，所以这两个 prelude 是 `view`；
调用方（public entry、helper）本来就没有 mutability 标注（= non-payable），从它们调用 `view` 合法。

`BodyBuilder` 侧的解析点：

1. **memory load**（`rawMemoryLoadExpr`）：EVM memory 在 IntToPtr 模型下就是
   `inttoptr <offset word>` 之后一次 `load i256`（另一种模型是 GEP 进
   `@notdec_evm_memory`）。识别 `memoryOffsetOfPointer()` 拿 offset，渲染
   `__evm_mload(offset)`。
   两道保护：
   - 只看 `i256` 的 load（`mstore8` 的 i8 读、`calloc` 后备缓冲的指针 load 都不碰）；
   - `dependsOnPointerAddress()`：沿 def-use 链发现 `ptrtoint` 就放弃 —— calldata/returndata 也是
     `inttoptr(算术)` 的形状，没有这道判断会把 calldata 读误标成 memory 读。
2. **storage load**：`storageHelperExpr` 已能把具名 slot 还原成状态变量；它返回 nullopt 时
   （未识别 slot、mapping/array 这类整体读）不再直接落到 unresolved，而是渲染
   `__evm_sload(slot)`，slot 用原始操作数表达式。
3. **`evm_msize(env)` / `evm_sha3(mem, off, size)` / `evm_callvalue(env)`**：出现在 call 路径。
   第一个实参是四个 runtime 指针之一（生成的 Solidity 不声明它们），按既有约定丢掉；
   `evm_sha3` 取 args 1/2 渲染成 `__evm_sha3(off, size)`（Yul 的 `keccak256(offset, size)`）。
4. **实参卫生**：prelude 形参都是 `uint256`，而实参表达式可能是 `address` 或 `bool`
   （例如 `__evm_sload(arg1)` 里 `arg1` 是 `address` 型 ABI 参数）。所有 escape 实参统一过
   `wordOperandExpr`（address → `uint256(uint160(...))`，i1 → `x ? 1 : 0`）。
   这一步是修 3 个 solc 失败（0648/0651/24541：`Invalid implicit conversion from address to
   uint256 requested`）时补上的，`__evm_calldataload` 也一并补了。
5. **操作数未解析就不 escape**：`__evm_mload/sload/sha3` 都要求实参表达式
   `!containsUnresolvedValue() && !containsBareDynamicValue()`。否则
   `__evm_sload(0 /* TODO: evm.alloc.addr */)` 会把 1 条占位符变成 2~3 条（case 24601 实测
   unresolved 6 → 11），反而不如保留原来那条紧凑的 `evm_sload`。

6. **语句位置的 SSTORE escape**（`formatStorageStoreEscape`）：`evm_sstore` 且 slot 不是可命名
   状态变量时，`formatStorageStore` 返回 nullopt 后原本什么都不输出。现在补一条
   `__evm_sstore(slot, value)`（`internal` + `assembly { sstore(slot, value) }`，**不带**
   `pure/view`，因为它是状态写）。语句位置不需要新的 AST 节点：intrinsic 是普通函数调用，
   直接作为 ExpressionStatement 输出，比在函数体里塞 `AssemblyStatement` 更安全
   （assembly 只能引用局部变量名，不能放任意表达式）。
   两个操作数同样要求已解析且非裸动态标识符，并统一过 `wordOperandExpr`。

7. **命名：`__notdec_*` → `__evm_*`**（`__evm_calldataload`、`__evm_mload`、
   `__evm_sload`、`__evm_sstore`、`__evm_sha3`、`__evm_msize`、`__evm_callvalue`）。
   先澄清一层：这些**不是 Yul 函数**，而是合约级 `internal` Solidity 函数（prelude），
   Yul 只出现在函数体里（`assembly { value := sload(slot) }`）；调用点是普通 Solidity 表达式，
   出现在 `require(...)`/`if (...)`/算术里。之所以必须是函数、不能把 assembly 直接内联到调用点：
   inline assembly 只能出现在**语句**位置，而且实参必须是 Solidity 变量/字面量，不能把
   `a + b` 这类任意表达式塞进 Yul 调用实参；函数调用则同时支持表达式位置和任意实参。
   为什么不用裸 `evm_sload`：输入 IR 的 runtime intrinsic 就叫 `@evm_sload`，而 unresolved 诊断
   打印的正是这个名字（`0 /* TODO: unresolved value: evm_sload */`），「escape 调用」和
   「未解析占位」会变成同一个字符串；将来把 escape 用量计入 pattern oracle 时也会和残留计数混淆。
   `__evm_` 前缀全树 0 冲突（helper 名来自 `sanitizeIdentifier(IR 名)`，frontend 只产生
   `public_*`/`private__0x*`/`bb.*`/`evm.*`）。历史 log（`20260924-03`）保留当时的
   `__notdec_*` 名字，不改写记录。

## 3. 效果

指标（103 case，相对本轮起点 = 上一轮 escape 原型提交）：

| 指标 | before | after | delta |
| --- | --- | --- | --- |
| condition TODO | 3452 | **3211** | −241 |
| unresolved value | 216 | **155** | −61 |
| goto | 1192 | 1192 | 0 |
| real jump | 1111 | 1110 | −1 |
| dangling goto | 5 | 6 | +1（见 4.1） |
| helper functions | 1313 | **1548** | +235 |
| helper call sites | 2751 | **3125** | +374 |
| 变差的 case | — | 1（24612，仅 dangling） | — |

escape 使用量（含 SSTORE escape 之后）：`__evm_calldataload` 1969、`__evm_sload` 351、
`__evm_sha3` 280、`__evm_mload` 262、`__evm_callvalue` 202、`__evm_sstore` 189、
`__evm_msize` 1。其中 sstore/sload/sha3 的绝对值在接上 SSTORE escape 后一起上涨，是因为
`__evm_sstore` 让一批 helper body 变干净从而可发射，连带把 body 里的其它 escape 也带进输出。
SSTORE escape 的典型形态（读-改-写一眼可辨，折叠 matcher 的现成输入）：

```solidity
__evm_sstore(__evm_sha3(0, 64), arg0 + __evm_sload(__evm_sha3(0, 64)));   // map[k] += arg0
__evm_sstore(__evm_sha3(0, 64), __evm_sload(__evm_sha3(0, 64)) & uint256(type(uint256).max - 255) | uint256(1));  // map[k] = (map[k] & ~0xff) | 1
__evm_sstore(uint256(uint160(arg1)), ...);                                         // map[addr] = ...
```

变好最多：25928 −77、0651 −43、0648 −42、24534 −32、24763 −15、24541 −14。典型输出：

```solidity
// 24534 DOMAIN_SEPARATOR（原来是 require(false /* TODO: evm.branch.cond.not */)）
require(__evm_callvalue() == 0);

// 24612
if (temp_2 > temp_1) { ... private__0xe5f_0xe5f(..., __evm_sload(arg0), 3249); }

// 1274
if (!(__evm_sload(__evm_sha3(0, 64)) & uint256(255) == 0)) { ... }
```

helper 数量上升是 escape 的直接后果：helper body 里原本有 unresolved 值就整体不发射
（`blockIsFullyRecovered`），现在 escape 把 body 洗干净，helper 得以发射、调用点从 TODO 变成
真实调用。这也是 24612 结构变化的原因（见下）。

## 4. 副作用与取舍

### 4.1 dangling goto 5 → 6（case 24612），已按约定抬预算

24612 的 helper 调用块原来是**空的**：调用点的 helper 不可发射，block 没有 statement，于是被
structurer 当成 `isClosedTerminalForkLeaf`（空 Fallthrough 块）折叠掉，连 `if (arg1 > slot_7[arg0])`
这条检查一起**静默消失**。escape 让 helper 可发射后块不再为空，结构器保留了这条分支；跳转目标是
revert 块，而该块的 `Label` 已经被渲染器的 `require` 折叠（`if (cond) { revert } else { body }`
→ `require(cond)`）吃掉了，于是留下一条 `// goto block_12` 悬空。

即：**新行为是「保留检查 + 一条诊断 goto」，旧行为是「整条检查丢掉」**。这一条按
`compile-budget.json` 的约定抬了 `max_dangling_goto_occurrences` 5 → 6，并在 comment 里写明。
彻底的修法（让 `require` 折叠不吃掉仍被其它边引用的块的 Label）留给结构器一侧，不在本轮范围。

### 4.2 escape 是「可表示性」，不是 memory 语义还原

生成的合约里 `assembly { mload(offset) }` 读的是**生成代码自己的**内存，而 `offset` 来自原
EVM 程序的 memory 布局（`0x40` 是原程序的 free memory pointer，在生成合约里是 solc 的 free
memory pointer）。所以 `__evm_mload(64)` 只是把「这里有一个真实的 word 读」表达出来，
不代表数值等价。价值在于：(a) 不再退化成 `0 /* TODO */`；(b) 后续折叠 matcher 有了统一的
中间形态（`__evm_sha3` + `__evm_sload` 正是 `sload(keccak256(slot, key))` 即
`mapping[key]` 的形状，`__evm_mload/__evm_calldataload` 是 decoder 读的形状）。

### 4.3 跨会话干扰（已排除）

本轮所有 Solidity 指标都在**同一个顶层工作树**上 A/B：把本轮 submodule 改动 `git stash` 后重建的
二进制（= HEAD submodule + 另一会话未提交的 `src/TypeRecovery/mlsub` WIP）跑同一套指标，
todo/unres/goto/real/dang **逐 case 完全相同**，所以上表全部归因于本轮改动。

但 `notdec.evm.solidity_patterns` 现在有 1 个失败 case
`24259_19755445_aefeec2314_4f43187f4106`（`expected notdec.solidity.abi_return=10 (>=)` /
`actual=8`），**不是本轮造成的**：该失败来自那份 mlsub WIP（stash 掉本轮改动后仍是 8；
用 HEAD 时期二进制 `/tmp/notdec.intr2` 是 10，与 manifest 相符）。pattern suite 数的是
`-o out.ll` 输出的 IR metadata，而本轮只改 Solidity 后端，不碰 IR。

## 5. 验证

```bash
ninja -C build-notdec-nothreads notdec
ctest --test-dir build-notdec-nothreads -R notdec.evm.solidity --output-on-failure
# notdec.evm.solidity_patterns: 102/103（唯一失败 24259 见 4.3，另一会话 WIP）
# notdec.evm.solidity_pattern_compile: 103 case，compile_failures=0，
#   unresolved 155/216、condition TODO 3211/3452、goto 1192/1192、real 1110/1111、
#   dangling 6/6（已抬）、helper 1548/1548、call sites 3112/3112
# notdec.evm.solidity_source: 通过
# notdec.evm.solidity_rewrite: 通过
```

## 6. 下一步

1. **MSTORE 的语句位置 escape**（SSTORE 已完成）：`collectStatements` 根本不看 `StoreInst`，
   所以 memory store 仍然静默丢弃。难点不是 escape 本身（照抄 `__notdec_mstore(off,val)`），
   而是**去重**：return buffer 的 store 已经被 `formatSingleWordReturn`/`formatMultiWordReturn`
   反向扫描重建成 `return x`，如果无条件输出 mstore 会重复。需要在 `collectStatements` 之前
   先扫一遍本 basic block 的 `evm_return`/`evm_call*`，收集它们认领的 store（或把两个 formatter
   改成返回「认领的 store 集合」），再对未认领的 store 做 escape。
2. **折叠 matcher**：`__evm_sha3(slot, len)` + `__evm_sload` → `mapping[key]`；
   `__evm_mload(0x40)` → free memory pointer 语义；`__evm_calldataload` → ABI 参数。
   需要一层按基本块的内存值编号（`mstore(off, v)` 的 def-use），这是下一块较大的工作。
3. **payability 声明**：`notdec.solidity.nonpayable` 目前只被写、没有被读。消费它（并把没有该
   标记的 entry 声明成 `payable`）之后，`require(__evm_callvalue() == 0)` 可以回写成
   `require(msg.value == 0)`，可读性更好；代价是要连带处理 payable 语义。
4. **escape 用量进 pattern oracle**：把 `__notdec_*` 的使用计数（或对应 IR 标记）加进若干
   pattern case 的 `expected_metadata_counts`，让 escape 覆盖面也受棘轮约束。

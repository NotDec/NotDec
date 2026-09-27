# 20260926-07 补齐 TODO 统计口径 + 剩余环境/状态操作码 escape

## 0. 一句话

先把**统计口径**补齐：棘轮此前只数 `false /* TODO` 这一种形态，于是
`/* TODO: switch ... */`（67 条）和 `TODO: recover ...`（**610 条**）完全不可见——现在都是独立预算项。
然后按第 1 项清单补上剩余环境/状态操作码的表达式 escape（`__evm_returndatasize` 等 9 个）：
condition TODO **2270 → 2089（−181）**、unresolved **117 → 97（−20）**，而新纳入统计的 recover 610 说明
"整块 body 没恢复出来"才是下一个真正的大头。

## 1. 统计口径（原来的盲区）

- `max_switch_todo_occurrences`：`/* TODO: switch <value> */`（0114 一个 case 就有 16 条）。
- `max_recover_todo_occurrences`：`// TODO: recover remaining body` 与 `/* TODO: recover body */`，
  即 structurer 没能恢复出完整 body 的函数（**610 处**）。
两者都锚定到当前实际值（67 / 610），作为下一轮要压下去的基线。口径本身写在 budget comment 里。

## 2. 新增的操作码 escape

`evm2llvm` 把 EVM 的弹出顺序原样传下来，而下面每个操作码都对应同名同参序的 Yul 内建，所以实现是一张表
（`BodyBuilder` 的 `kEvmBuiltinEscapes`）：**前面的指针实参是运行期句柄（丢弃），后面的字实参按序传**。

| IR 调用 | 字实参 | prelude → Yul | mutability |
| --- | --- | --- | --- |
| `evm_returndatasize` | 0 | `returndatasize()` | view |
| `evm_codesize` | 0 | `codesize()` | view |
| `evm_extcodesize` | 1 | `extcodesize(arg0)` | view |
| `evm_extcodehash` | 1 | `extcodehash(arg0)` | view |
| `evm_tload` | 1 | `tload(arg0)` | view |
| `evm_blockhash` | 1 | `blockhash(arg0)` | view |
| `evm_byte` | 2 | `byte(arg0, arg1)` | pure |
| `evm_create` | 3 | `create(arg0, arg1, arg2)` | 默认（状态写） |
| `evm_tstore` | 2 | `tstore(arg0, arg1)` | 默认（状态写） |

- 形参统一叫 `arg0..`：**不能**用 `gas`/`number` 这类名字，它们在 Yul 里是内建，会被解析成函数
  （上一轮 CALL escape 的 `gas` 就是这么炸的，28 个 case 编译失败）。
- 结果没人使用的 void 调用（`evm_tstore`）走语句位置输出，避免静默丢副作用。
- 实参照旧过 `wordOperandExpr`，任一未解析就放弃 escape（保持原 TODO）。

使用量：`__evm_returndatasize` 307、`__evm_extcodesize` 210、`__evm_codesize` 64、
`__evm_byte` 17、`__evm_create` 6、`__evm_extcodehash` 3（`tload/blockhash/tstore` 暂时 0 次，
它们的实参还没解析到；`__evm_alloc_base` 这轮 2151 次）。

## 3. 指标

| 指标 | before | after |
| --- | --- | --- |
| condition TODO | 2270 | **2089** |
| unresolved | 117 | **97** |
| goto / real / dangling | 1198 / 1116 / 6 | 1218 / 1136 / 6 |
| switch TODO（新） | 未统计 | **67（锚定）** |
| recover TODO（新） | 未统计 | **610（锚定）** |
| helper functions / call sites | 1835 / 4583 | **1943 / 5038** |
| 变好 case | — | 42（25928 −17、0258 −16、1991 −11、1636 −8…） |

goto/real 各 +20 仍是同一类结构性代价（新渲染出的调用/语句让结构器保留了以前会折掉的分支），
已按惯例写进 budget comment。

## 4. 下一步

1. **recover 610**：这是新暴露出来的最大口径。`TODO: recover remaining body` 来自
   `analyzeBodyCompletion`（`HasGoto || !ReachesExit`），说明 structurer 对某些函数只恢复了一部分。
   值得先量分布（哪些 case、哪些函数、是不是集中在 helper）、再决定是修结构器还是修渲染。
2. **switch 67**：switch 的条件/分支值不可渲染（0114 的 16 条就是 `evm.mload*`）。等第 3 项把叶子补齐后
   应该自然下降。
3. COPY 家族语句 escape（`returndatacopy` 581 等）——现在它们的长度实参 `evm_returndatasize` 已经可渲染，
   可以点火了。

## 5. 验证

`notdec.evm.solidity_pattern_compile`：103 case、`compile_failures=0`（三档 pipeline 回退），
unresolved 97/97、condition TODO 2089/2089、goto 1218/1218、real 1136/1136、dangling 6/6、
switch 67/67、recover 610/610、helper 1943/1943、call sites 5038/5038。

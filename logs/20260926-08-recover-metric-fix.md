# 20260926-08 recover 610 的定位：其实是 95 + 515，以及真正的大头是 real jump

## 0. 一句话

上一轮新加的 `recover_todo` 计数**写错了口径**：它 `count("TODO: recover")`，把
`TODO: recover remaining body`（**body 只恢复了一部分**）和 `TODO: recover event signature`
（**事件的 ABI 参数表没恢复出来**）混在一起，报出 610。拆开后是 **95 + 515**，两个都不该合成一个数。
继续往下看这 95：分布在 48 个 case、最多 9 条，属于长尾；它们的共同成因是**函数里存在真实跳转
（`// goto block_N` 只是注释，控制流实际丢了）或 tree 没走到 exit**——也就是说真正的大头是
`real_jump = 1136`，而不是这个 TODO 本身。

## 1. 口径修正

| 指标 | 含义 | 实际 |
| --- | --- | --- |
| `switch_todo` | `/* TODO: switch <value> */` | 67 |
| `recover_body_todo` | `TODO: recover remaining body` + `/* TODO: recover body */` | **95**（0 条 empty-body） |
| `event_signature_todo` | `TODO: recover event signature` | **515** |

budget 里 `max_recover_todo_occurrences: 610` 已删除，换成上面两个键（95 / 515），并在 comment 里写明
"610 是计数写错"这件事。

## 2. 95 条 body gap 的形态

- 分布：48 个 case 有，最多的是 2001（9）、25928（7）、1991（4），其余 1~3 —— **长尾，没有单点根因**。
- 触发条件（`BodyBuilder` 末尾）：

```cpp
if (!Result.Statements.empty() && !isTerminalStatement(Result.Statements.back())) {
  BodyCompletion Completion = analyzeBodyCompletion(Cfg, Tree, Tree.root());
  if (Completion.HasGoto || !Completion.ReachesExit) { ... "TODO: recover remaining body" ... }
}
```

  即"最后一条语句不是终结语句"且（tree 里存在 Goto 节点 或 没走到 exit）。典型输出形态：

```solidity
  // block_31:
  if (!false /* TODO: evm.gt91.i.i.not */) {
    // goto block_32          // ← 注释形式的跳转：控制流在这里丢了
  }
  // block_46:
  revert(); // custom_error_candidate, ...
// TODO: recover remaining body
```

  注意 `// goto block_N` 和 `// block_N:` **都是注释**，所以每条 real jump 都是"输出的控制流与原程序不一致"
  的位置；`recover remaining body` 只是把这种函数再标一次。

## 3. real jump 的分布（真正要压的量）

corpus `goto = 1218`（其中 fallthrough 76、dangling 6）→ `real = 1136`：

| case | real | goto | todo |
| --- | --- | --- | --- |
| 0258 | 78 | 80 | 61 |
| 24601 | 72 | 72 | 78 |
| 25928 | 72 | 72 | 62 |
| 0657 | 42 | 42 | 6 |
| 0118 | 39 | 39 | 8 |
| **0029** | **36** | **68** | 652 |
| 0340 / 0637 / 0870 / 1572 | 各 36 | 36 | 6~8 |

两个观察：
1. 有一批 case 的 real jump 是 **36 / 42 这种整齐的数字**，而且它们的 condition TODO 很少（6~8）——说明
   这些函数的**体已经基本恢复，只是结构器对某种 CFG 形状放弃了结构化**，属于"最后一公里"的类型，
   比 0029 那种"值都还没解析"的更好下手。
2. 0029 的 goto 68 里有 32 条是 fallthrough（跳到下一个块），real 36；它的 TODO 652 主要还是值的问题。

## 4. 下一步建议

1. **挑一个"36 real jump + todo≈6"的 case（0340 或 0870）**，用 `NOTDEC_STRUCT_DEBUG=1` 看结构器在哪一层
   放弃（`[struct-if]`/`[struct-virt]`/`lastResortRefinement`），判断是
   - 结构器可以修的判定（收益：控制流正确性，不只是计数），还是
   - 渲染器可以把某种 goto 形态降级成 `break`/`continue`/`if`（收益同上）。
2. `event_signature` 515：这是**另一个独立问题**（事件 ABI 不可恢复，因为没有 ABI 信息）。
   可先量：这 515 条里有多少是"log 的 data 全是单字、topics 全为 32 字节"从而可以推断成全
   `uint256`/address 参数的——如果有很大比例，就值得做一轮事件参数恢复；否则保持注释即可。
3. `switch` 67：等 mload 叶子补齐后应自然下降，暂不专门做。

## 5. 验证

计数改动只在 runner（`recover_body_todo` / `event_signature_todo` 分开）与 budget；
下一轮 compile suite 会以 95 / 515 为基线。其余指标不变（2089 / 97 / 1218 / 1136 / 6 / 67 / 1943 / 5038）。

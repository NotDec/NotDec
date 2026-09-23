# 20260922-01 内联 A/B 的真实基线，以及 structurer 失败的根因定位

> **【2026-09-23 注】** 本文的“outlined 基线”是**诊断视角**，不是产品口径：pre-inline 形状会把
> 不同入口的 calldata 类型合在一起（模块级记录并集，见 `20260920-01` §3），实测
> record==annotated 1858/2316 vs 内联 2264/2316。`NOTDEC_EVM_INLINE` 开关已删除（`20260923-04`）。
> §8 P0 的“让基线诚实”应理解为“测量要诚实”（别把后端丢 helper 的结果当成绩），不是
> “把 outlined 抬成口径”。本文的根因结论——失败形状是共享 join 而不是 duplication——与内联无关，
> 仍然有效。

结论先行：**不建议先迁移 angr 的 duplication/similarity（ISD）那条线**。
"outlined 下结构化得很干净" 是后端 **丢弃 helper** 造成的幸存者偏差；同一批函数体在
**没有内联** 的情况下同样结构化失败。失败点不在重复（duplication），而在 Phoenix 的
`reduceIfOnce` 无法处理"共享 join（多前驱）"的形状，最后走 last-resort 虚边 → goto。

## 1. 术语澄清

- `outline`（老 pass，selector/entry outlining）：把 dispatcher/entry 主体抽成
  `public_<name>___0x...` 函数，两个形状都开。
- 本次 A/B 的 `outlined` = `NOTDEC_EVM_INLINE=0`，指 **solc FullInliner 保留下来的 Yul
  小函数没有被我们重新内联**，与 outline pass 无关。
- `--algo` 对 Solidity 渲染路径 **无效**：`BodyBuilder.cpp:2916` 硬编码
  `DefaultStructurerName="sailr"`；`--algo` 只喂给 C CLI 的 StructuralAnalysis
  (`Commandlines.def:163`)。本会话给 BodyBuilder 加了一个默认保持不变的
  `NOTDEC_STRUCTURER` 环境变量开关（未提交），才能做 A/B。

## 2. outlined 基线是假的：后端会丢弃渲染不完整的 helper

`Reader::readContract`（`lib/Solidity/Reader.cpp:361-403`）先渲染 helper，凡
`blockIsFullyRecovered` 为假就 **把该 helper 从输出里丢掉**，并连带把调用点也丢掉。

实测（`logs/artifacts/20260922-structurer-duplication/`）：

| case | outlined .sol 里 `// block_` 注释总数 | IR 里被丢掉的 helper |
| --- | --- | --- |
| 0118 | 27（40 个函数） | `private__0xfa9_0xfa9` 78 块 |
| 0637 | 35（66 个函数） | `private__0x19ce_0x19ce` 65 块 |
| 0029 | 17（7 个函数） | `private__0x2bf/0x3bb/0x8e4/0xb56...` 整条 Yul 链 |

把 helper 改名成 `public_*` 强制渲染（`--frozen-tr-input-ir` 回灌）后：

| helper | IR 块数 | 渲染出的 goto |
| --- | --- | --- |
| 0118 `private__0xfa9` | 78 | **70**（flat 下 transfer 是 91） |
| 0637 `private__0x19ce` | 65 | **57**（flat 下 transferFrom 是 78） |
| 0029 `private__0x2bf` | 6 | 0 |

即：同一函数体，**不内联、独立成函数**时一样失败。outlined 的 7/0/12/9/2 个 goto 是
"把失败函数删掉"之后的数字，不是 structurer 的真实表现。

## 3. 最小复现 + 单变量干预：失败形状 = 共享 join

outlined 0118 `public_transferFrom`（9 块，无环可归约，完全没内联）：

```
0 -> 1
1 -> {2,7}    // calldatasize > 99
2 -> {5,3}    // sload == MAX
3 -> {6,4}    // sload < arg2
4 -> 5        // 与 2 共享 join 5
5 -> ret
```

渲染 = 7 个 goto（`// goto block_1/2/3/4/5/6`）。把 join 块 5 为前驱 4 **克隆一份**
（等价于 tail duplication，其余一律不动）后：**7 → 0 个 goto**，输出变成干净的嵌套 if/else。

- 复现脚本：`clone_join.py`；产物 `frozen/0118_unshare.sol`。
- 反过来的方向（合并重复块）只会 **制造更多共享 join**。
- 归约性不是原因：flat 0029 的 entry 有 196 个循环但 0 个不可归约；flat 0118 transfer
  甚至 0 个循环。

## 4. flat 的 goto 洪峰：另一个形状（guard 链）+ 规模放大

flat 0118 `transfer`：101 块、63 个条件分支、20 个 join 块、27 处 "then 后继就是 join"。
典型形状是 `[8]->{9,12} [9]->{12,10} [10]->{12,11} [11]->{12,95}`：多个 guard 都跳向同一
continuation，structurer 渲染成 `if (c) { goto J }` 链。把 **所有 terminal join**（ret/
unreachable，共 36 处）克隆拆开后，flat 0118 只从 194 → 182 个 goto（transfer 91→86）。
所以 0118 的 goto 洪峰主要是"guard 链 + 不复制 continuation"的合法回退，不是重复导致。

flat 0029 是规模问题：`public_start`/`public_withdrawal` 各 **912 块、16 个 switch、
1677 个 call**，而 outlined 全模块只有 104 块 —— 内联把整份合约塞进了每个 entry
（该文件 cond_todo 692）。

**旁证**：`NOTDEC_EVM_INLINE=all`（内联更多、复制更多）全语料 goto/cond_todo =
**2683/3239**，反而低于 calldata 档的 5437/4647。若"复制"是主因，这个方向说不通。

## 5. 现有 SAILR deopt 在这批形状上没有任何正收益

加了 `NOTDEC_STRUCTURER` 后对 5 个重灾 case 做 A/B：

| structurer | goto | cond_todo |
| --- | --- | --- |
| sailr（含现有 7 个 deopt pass） | 933 | 828 |
| structured-phoenix（无 deopt） | **887** | **812** |
| structured-goto | 0 | 0（该 structurer 输出把每个块平铺成 label，丢失控制流，不可比） |

即现有 deopt 管线（含我们已移植的简化 DuplicationReverter）在这些输入上是**负收益**。

## 6. 代码级根因（angr 迁移无关）

- `RegionIdentifier` 只有 Root/NaturalLoop 两种 region，没有条件 region，也没有
  conditional follow；join/follow 完全在 `reduceIfOnce`（`PhoenixStructurer.cpp:334-460`）
  里现算。
- 它的所有 if schema（`:379-411`）都要求 **两条臂的前驱数 == 1** → 多前驱的 join
  **永远不能当臂**；`:394-399`（terminal-then + linear follow）把 follow 留在外面。
- 剩下那条边匹配不到 schema → `lastResortRefinement` 每轮虚化一条边
  (`:3926-3979`) → `buildVirtualizedBranchSource`(`:1515-1523`) 生成
  `if (c) { goto X }` + 尾部 goto；而 `rewriteStructuredSourceTargetTransfer`
  (`:1503-1509`) 不认识"已有 If root"，于是把条件 **再渲染一遍**（就是我在
  outlined/flat 里看到的 `if (c) {...} if (!c) {...}` 成对输出）。
- 迭代到不动点（上限 1000），没有 dump 设施；只能看渲染注释。

## 7. angr ISD/duplication 线为什么不划算

- angr `DuplicationReverter`（`/sn640/angr/.../duplication_reverter.py`）是 **condensing**：
  在共同条件支配者下合并两个相似兄弟子图；goto-anchored、要求语句级 `matches` 相等、
  每次只合并一对、`max_opt_iters=5`，且只在非默认 full preset 打开；没有任何内联去重
  的测试/文档。合并结果 = 更多共享 join = 我们 `reduceIfOnce` 的死穴。
- 真正对症的是 **反方向（duplication / tail duplication）**：把共享 join/guard 链的
  continuation 复制开，让每条臂变成单前驱。angr 里对应
  `ReturnDuplicatorLow/High`、`CrossJumpReverter`（DUPLICATING_OPTS），不是 ISD。
- 我们的 structurer 失败甚至在 9 块的无内联函数上复现，所以"内联暴露的问题"这个方向
  判断是对的，但"用 similarity/去重来修"是错的。

## 8. 建议的下一步（按性价比）

1. **P0 让基线诚实**：suite 增加/统计 "helper 因 blockIsFullyRecovered 被丢弃" 的数量
   （或加一个 `--render-dropped-helpers` 诊断模式，把被丢的 helper 当 public entry 渲染），
   否则 outlined 口径永远不可比。
2. **P1 修 structurer 的两处**（最小复现已就绪）：
   - `reduceIfOnce` `:394-399`：当 fallthrough/follow 同时也是某条臂的后继（共享 join）
     时，要么把 follow 吸进真正的 if/else，要么复制 join 块（tail duplication）；
   - `buildVirtualizedSource`：剩余边应折进已有 If root（设置 Then/Else/Follow），
     而不是再拼一个 `if (c) { goto }`。
   验收：9 块最小复现（建议加进 `test/structuring/`）+ 5 个重灾 case + compile-budget
   (goto 5437 / TODO 4647) + 两套 pattern 口径。
3. **P2 加 structurer 的 tree/goto dump**（env 开关），现在只能靠渲染注释 debug；
   同时把 `NOTDEC_STRUCTURER` 这类开关保留下来做 A/B。
4. **P3 内联 scope 收敛**：阻止"一个 entry 吞掉整份合约"（0029: 912 块/16 switch）。
   与 structurer 正交，直接缩小输入规模。
5. **P4 cond_todo**（4647）单独查：0029 的 `false /* TODO: evm.eq.i.i.i.i */` 是另一类
   问题（值/条件未恢复），别和 goto 混在一个预算里归因。

## 9. 本会话产物

- 证据/脚本：`logs/artifacts/20260922-structurer-duplication/`
  （`ir_analyze.py`、`ir_joins.py`、`ir_reducibility.py`、`dump_cfg.py`、
  `clone_join.py`、`clone_terminal_joins.py`、`ab_algo.sh`、`outlined/`、`flat/`、
  `frozen/`、`allmode/`）
- 未提交改动：`external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp` 的
  `NOTDEC_STRUCTURER` 诊断开关（默认行为不变）。

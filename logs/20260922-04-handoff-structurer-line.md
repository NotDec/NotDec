# 20260922-04 交接：structurer 修复线与 EVM 扁平形状现状

## 0. 一句话

"扁平形状 goto 洪峰 = 内联复制" 的旧结论是错的：outlined 基线是后端**丢弃渲染不完整的 helper**
造成的幸存者偏差。真正的失败形状是共享 join / 终止臂 follow，已在 Phoenix 里修好：
103 case flat goto **5437 → 1787**，solc 0 失败，5 套 EVM solidity 测试全绿。angr 的
duplication/similarity(ISD) 方向相反，已判定不迁移。

## 1. 本会话提交（全部未 push）

子模块 `external/NotDec-llvm2c`（顶层随之带指针）：

| 子模块 | 顶层 | 内容 |
| --- | --- | --- |
| `fabd348` | `e049d544` | 终止臂 follow continuation（`isSequenceContinuation`）+ `NOTDEC_STRUCT_DEBUG` / `NOTDEC_STRUCTURER` + 最小复现单测；goto 5437→3640 |
| `0f3c38f` | `5cc73cb3` | 根因二：`collapseNodes` 入口成员顺序（丢入口块/丢 `temp_N` 声明）+ 放宽 continuation 到任意单活边 + `firstRenderedBlock` 处理 While；goto 3640→**1787** |
| `99ad63c` | `252527a8` | 修复 `structuring-analysis-test`（AST API 漂移，target 之前根本编译不过），单测门禁恢复 |
| `57d1ad8` | `d1848daa` | `EqualTargetIndex` 极性（`NotEqual ? 1 : TrueTargetIndex`）+ 恢复被它卡住的测试 |

日志：`logs/20260922-01-outlined-baseline-and-structurer-root-cause.md`（调查）、
`logs/20260922-02-structurer-terminal-arm-join-fix.md`（修复一）、
`logs/20260922-03-entry-continuation-and-block-order-fix.md`（修复二）。
证据：`logs/artifacts/20260922-structurer-duplication/`（12M，含脚本、flat/outlined TR-input IR、fix3 语料）。

## 2. 指标（103 case，默认 flat 口径）

| 指标 | 会话前 | 现在 | 预算(`compile-budget.json`) |
| --- | --- | --- | --- |
| goto | 5437 | **1787** | 1787 |
| real jump | 4604 | **1695** | 1695 |
| dangling goto | 501 | 58 | 58（**上调** 55→58，理由见 20260922-03） |
| condition TODO | 4647 | 4078 | 4078 |
| unresolved value | 289 | 297 | 297（**上调** 289→297，理由同上：以前被丢弃的语句现在渲染出来） |
| solc 编译失败 | 0 | **0** | 0 |
| helper functions / call sites | — | 1276 / 2442 | 1276 / 2408 |

outlined 口径会话中期测过 goto 424→233、cond_todo 729→658；**P1/P2 之后未复测**（见 §4.1）。
`ctest -R notdec.evm.solidity` **5/5 通过**；`structuring-analysis-test` exit 0。

## 3. 关键结论与设施

- **outlined 基线不可信**：`lib/Solidity/Reader.cpp:361-403` 会把 `blockIsFullyRecovered` 为假的
  helper 整个丢弃（0118 丢 78 块的 `private__0xfa9` 等），"0-12 个 goto" 是删掉难函数后的数字；
  把 helper 改名 `public_*` 强制渲染，同一函数体照样 57-70 个 goto（无内联）。
- **失败形状**：`reduceIfOnce` 终止臂 schema 折出的节点 tail 是终止块但仍有唯一活边（隐式
  continuation）；`reduceSequenceOnce` 原先不认 → 每轮 last-resort 虚化一条边 → goto 级联。
- **第二个根因**：`MutableRegionGraph::collapseNodes` 按成员创建顺序累积 Blocks，合并后
  `Blocks.front()` 不再是节点入口，最终组装（按 `Blocks.front()` 排序）会把带入口块的节点排在
  以无条件转移结尾的节点之后，被 `dropUnreachableAfterRenderedTransfer` 当死代码删掉
  （`uint256 temp_N;` 因此消失 → 17 个 `Undeclared identifier`）。
- **angr 不迁移**：`DuplicationReverter` 是 goto-anchored 的相似度**合并**，会把 join 变得更多；
  我们对症的是 duplication（拆共享 join）。详见 20260922-01 §7。
- **调试设施**：`NOTDEC_STRUCT_DEBUG=1`（region 图/pattern/virt/最终树/丢失块 dump）、
  `NOTDEC_STRUCTURER=structured-phoenix|structured-goto|sailr`（Solidity 路径选 structurer；
  `--algo` 只作用于 C CLI）。
- 构建：`ninja -C build-notdec-nothreads notdec`；单测 target `structuring-analysis-test`，
  二进制在 `build-notdec-nothreads/external/NotDec-llvm2c/bin/structuring-analysis-test`；
  solc 在 `/sn640/EthIR/source/solcv8.25`。
- 不要动其他会话脏文件：`CMakeLists.txt`、`external/NotDec-bin2llvm`、`external/binarysub`、
  `llvm-22.1.0.nothreads.obj`。

## 4. 接下来

1. **复测并重锚 outlined 口径**：P1/P2 之后重跑 `NOTDEC_EVM_INLINE=0` 全语料，若指标下降就
   收紧（outlined 是 pattern 套件的默认口径之一，`notdec.evm.solidity_patterns_outlined` 是 pin 的精确值）。
2. **清掉剩余 `TODO(structuring)`**（`test/structuring/structuring_analysis_test.cpp`）：
   - 三个 `refineCyclic` 手搭 CFG 的 DoWhile/InfiniteLoop 期望（疑似受 0f3c38f 折叠影响，需重定形状，
     不是简单数值修正）；
   - 拷贝-phi 的 dephication 渲染（`_copy1` 命名消失，现渲染为 `p = a; / p = b;` + unresolved TODO）。
3. **P3 cond_todo 4078 / 内联 scope**：0029 的 entry 各 912 块、16 个 switch（整份合约被内联进每个
   entry），属内联 scope 而非 structurer。收窄到"只展开 calldata 偏移相对形参不恒定的 reader"
   （`EvmCalldataAccessPass` 已算 polymorphic 集合）；注意 `all` 档 goto 2683 低于当时 calldata 档，
   说明 scope 还有空间但方向要实测。
4. **收尾**：全量 `ctest`（含 4 套 type_recovery）→ push 子模块 + 顶层。
5. 可选：`logs/artifacts/20260922-structurer-duplication/` 12M 若嫌大，可只保留脚本 + frozen 最小复现。

## 5. 未验证/已知风险

- 放宽后的 continuation 规则只在 103-case pattern 语料 + EVM solidity 套件上验证过，
  `test/evm/solidity-source`（真实合约转出）与 type_recovery 套件在本会话最后一次修改后**未全量重跑**。
- `EqualTargetIndex` 修正改变条件极性，已过 5 套 EVM solidity，但 source/rewrite 之外的路径未单独验证。

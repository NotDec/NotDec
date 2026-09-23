# 20260923-03 交接：structuring TODO 清零；发现 fabd348 的 C 路径回归（outlined 棘轮已作废）

## 0. 一句话

交接文档 20260922-04 的 ①② 做完了：~~outlined 口径建了独立预算棘轮~~（**2026-09-23 更正：这个棘轮
方向错误，已随 `NOTDEC_EVM_INLINE` 开关一起删除**，见 `20260923-04-remove-evm-inline-switch.md`）；
6 处 `TODO(structuring)` 全部清零
（5 处按折叠后形状重定，1 处是后端真 bug：phi 反 phi 化变量没有声明、读取渲染成 `return 0 /* TODO */`）。
全量 ctest 之后 6 套 EVM solidity + lifting 全绿，但发现 **`structuring-source-cases` 是 fabd348 引入的回归**
（7 个 source case 渲染出 `return` 之后的重复语句），4 套 type_recovery 则是 2026-08-03 以来的快照漂移。
**建议先修回归再 push**（本会话没有 push）。

## 1. 本会话提交（全部未 push）

| 顶层 | 子模块 NotDec-llvm2c | 内容 |
| --- | --- | --- |
| `48da846c` | — | ~~outlined 独立预算~~（**已作废**，见下一行） |
| `21417e47` | `a147cf7` + `16971a7` | phi dephication 变量声明/读回；6 处 `TODO(structuring)` 清零 |
| `924cbd2f` | — | 删除 `NOTDEC_EVM_INLINE` 开关：calldata 链内联是唯一形状；删除 outlined 预算与两套 outlined 测试 |

日志：`logs/20260923-04-remove-evm-inline-switch.md`（本次更正）、
`logs/20260923-02-structuring-todo-cleanup.md`、
`logs/20260923-01-outlined-diagnostic-snapshot.md`（已改写为诊断快照）。

## 2. 指标（103 case）

| 指标 | 内联（唯一形状；交接值 → 现在） | pre-inline（诊断，形状已删除） | all（历史诊断，已删除） |
| --- | --- | --- | --- |
| goto | 1787 = 1787 | 424 → **140** | **742** |
| real jump | 1695 = 1695 | 343 → 100 | 678 |
| dangling | 58 = 58 | 69 → 19 | 44 |
| condition TODO | 4078 = 4078 | 729 → 558 | **2603** |
| unresolved | 297 = 297 | 264 → 267 | 349 |
| helper funcs / call sites | 1313 / 2442 | 1339/1301 → 1383/1394 | **182 / 292** |
| solc 失败 | 0 | 0 | 0 |

- 内联口径复跑逐项等于交接文档，说明基线可比；phi 修复后**逐项不变**（语料不受影响）。
- 后两列现在只是历史诊断：两个形状都随开关删除了。它们的用途是说明“结构器修复不是内联特有的”
  （pre-inline 的 goto 同样 424→140），以及内联范围是 ABI 覆盖与结构器输出之间的权衡
  （`all` 档 goto/cond_todo 更低，但 helper 覆盖崩到 182/292，floor 是 1276/2408）。

## 3. 全量 ctest（24 项）归因

通过：当时的 6 套 `notdec.evm.solidity*`；删除 outlined 两套后是 4 套（patterns / pattern_compile /
rewrite / source，已在 `924cbd2f` 之后复跑全绿）、`notdec.lifting.wasm`、
`sysy-tests`、`phi-demote` / `phi-demote-htypes`（先把 `phi-demote-test` 建出来）、
`legacy-phoenix-removed` / `structured-phoenix-available` / `shared-structurer-registry`
（先把 `notdec-llvm2c` 建出来，此前它们因 `FileNotFoundError` 全挂）。

| 测试 | 状态 | 归因 |
| --- | --- | --- |
| `structuring-source-cases` (#10) | **回归** | 5355022 通过，fabd348 起失败；见 §4 |
| `structuring-smoke` (#5) | 预先存在 | 5355022（会话前）同样失败，失败清单基本一致（C 侧 deopt/dephication 期望过期） |
| `sailr-bench2-migration` (#9) | 预先存在 | 5355022 同样失败 |
| `notdec.type_recovery.*` (#21-24) | 预先存在 | `test/type-recovery/*/expected` 最后更新 2026-08-03（`12ea5c00`），之后 type recovery 改过（如 `b0a31602`）；用会话前的二进制复跑得到**相同**输出，与本次改动无关 |
| `MLsubGeneratorTest_NOT_BUILT` / `TypeBuilderTest_NOT_BUILT` (#12/13) | 环境 | 目标没建（其他会话的 binarysub 线） |

## 4. 回归细节：`structuring-source-cases`（fabd348）

复现（HEAD 或 fabd348 上都会失败，5355022 通过）：

```
cd /sn640/NotDec
llvm-22.1.0.obj/bin/clang -S -emit-llvm -O2 -fno-discard-value-names \
  -Wno-implicit-function-declaration \
  external/NotDec-llvm2c/test/structuring/source-cases/cases/002_loop_break_continue.c -o /tmp/case.ll
NOTDEC_STRUCT_DEBUG=1 build-notdec-nothreads/external/NotDec-llvm2c/bin/notdec-llvm2c \
  /tmp/case.ll -o /tmp/case.c --algo=structured-sailr
```

输出末尾（`sink`/`return` 出现两次，第二份是 return 之后的死代码）：

```c
    } while (n_addr_018_reload > 1);
    sink(*(int *)&sum_0_lcssa_reg2mem);
    return *(int *)&sum_0_lcssa_reg2mem;
    sink(*(int *)&sum_0_lcssa_reg2mem);
    return *(int *)&sum_0_lcssa_reg2mem;
}
```

7 个 case 触发：`tail_early_return`、`nested_loop_switch`、`nested_loop_break_only`、`loop_two_returns`、
`loop_switch_shared_latch`、`loop_break_continue`、`guarded_loop_skip`（全部 `O2:structured-sailr`）。
oracle 规则：`return X;` 后面不允许接语句（`test/structuring/source-cases/manifest.json` 的 `regex_absent`）。

**机制（当前证据）**：`fabd348` 给 `isSequenceContinuation` 加了 "单活边 + 终止 tail" 的放宽之后，
自然循环 region 的结构根会把 follow 块一起吸收（`[struct-cfg] final-overlay ... node 4 blocks={1,2,3}`），
而外层 Root region 的图里 follow 仍是独立节点；`structureRegion` 最后按 `Blocks.front()` 组装所有 active 节点，
于是同一批块被渲染两次。`dropUnreachableAfterRenderedTransfer` 只丢"直线且不可被 label/循环进入"的兄弟，
重复块因为有虚拟边可进入而保留。

**试过但无效**：在 `reduceSequenceOnce` 里禁止 `RegionKind::NaturalLoop` 吸收 `R.Successors` 里的节点
（实验已回退）—— source-cases 仍然失败，说明吸收不是走那条路径（可能在 overlay/preprocess 或
`reduceIfOnce` 的 terminal-arm 折叠里）。下一位可以从 `[struct-if]`/`[struct-cfg]` dump 入手确认吸收点。

**注意权衡**：`isSequenceContinuation` 的这次放宽正是 flat goto 5437→1787 的主要来源，改动必须同时跑
103 case flat + outlined 两套预算和 `structuring-analysis-test`，不能只让 source-cases 变绿。

## 5. 接下来（按优先级）

1. **修 §4 回归**（未 push 前必须处理）；验收用 `structuring-source-cases` + 编译预算 + 单测
   （不再有 outlined 预算）。
2. **P3 的正解是 per-call-site 实例化，不是收窄内联范围**：让 `KIND_MLSUB_POLYMORPHIC_FUNCTION`
   也能隔离 public entry（`NOTDEC_EVM_POLY_ENTRIES=1` 已可复现），先解 case 24259 的约束 worklist
   不收敛。那之后连 pre-inline 形状都能给出正确 ABI 参数，内联范围才值得再讨论
   （`restrictInliningToCalldataChains` 的 seed/闭包只是次要旋钮）。
3. **type_recovery 快照刷新/重锚**：4 套 suite 自 2026-08-03 起漂移（含新增 `bytes:` 注解、stack-canary 改动），
   与本次结构器线无关，但要单独决定是重锚还是修行为。
4. push 子模块 + 顶层（两边都有一长串未 push 提交）。

## 6. 环境注意

- 其他会话正在改 `external/binarysub`（脏）+ `build-notdec-nothreads2`，不要动。
- `build-notdec-nothreads` 里现在 `notdec-llvm2c`、`phi-demote-test` 已构建（此前缺失）。
- 单测 target `structuring-analysis-test`，二进制在 `build-notdec-nothreads/external/NotDec-llvm2c/bin/`。
- 基线实验 commit：`5355022`（fabd348 的父）；`git checkout 5355022 && ninja -C build-notdec-nothreads notdec-llvm2c`。

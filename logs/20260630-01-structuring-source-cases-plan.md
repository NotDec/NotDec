# 原始 prompt

> 当前需要改成测试用例驱动的改进方式，现在单独对着这个fortune还是太大了。看一下怎么规划测试文件夹比较合适。Angr那边是怎么测试的？如果简单一点的话，可以就存一些各种各样的源码，使用clang，开启优化给它编译为IR，然后再走LLVM2C，怎么再怎么评判一下控制流语句的恢复是否一致，是否够好
>
> 按这个推进一下

# 背景

只盯 `/tmp/notdec-fortune-wasm-sailr-tr2-20260629.c` 太粗。fortune 能暴露大量 goto，但函数大、来源复杂，很难判断一次 SAILR 修改到底修的是哪个 CFG 形状，也很难防止后续回退。

angr 自身有两层做法：一层是在 decompiler 测试里跑真实二进制，再对输出文本做结构化断言；另一层是 SAILR evaluation，把源码编译、反编译后统计 `gotos`、`cfged` 等指标。NotDec 现在先采用轻量做法：保存小源码，clang `-O2` 编译成 LLVM IR，再走 `notdec-llvm2c --algo=structured-sailr`，用文本和计数 oracle 卡住结构恢复结果。

# 目标

新增一套小而稳定的 source-driven structuring suite：

- 每个 case 只覆盖一个常见控制流形状。
- 输入保留 C 源码，不直接手写复杂 IR。
- runner 负责 clang 编译、调用 llvm2c、检查输出。
- oracle 先检查 `if` / `while` / `switch` / `break` / `continue` / `goto` 这些结构信号，不做重型 CFG 等价。
- 后续每修一个 SAILR goto 问题，先缩成新 case，再改算法。

# 路线

测试放在 `external/NotDec-llvm2c/test/structuring/source-cases/`。原因是这套测试直接验证 llvm2c 后端的 structuring 输出，不依赖 NotDec 前端、类型恢复或 wasm lifting。

当前第一批 case 覆盖：

- diamond if/else 合流。
- loop 里的 break/continue。
- switch cluster。
- tail merge return。
- nested loop + switch + continue。

评判标准先分两类：

- 硬性坏味道：禁止 `goto structured_block_X;` 后面马上落到同名 label、`while (1)` 或 `if (` 这类明显可以结构化的入口。
- case 局部指标：要求出现预期结构，限制 `goto` 最大数量。已知还不能完全消除的 loop case 暂时给上限，不要求 0。

# 风险

文本 oracle 不等于语义等价。它适合当前阶段快速压住 goto 回归，但不能证明所有控制流都恢复正确。

clang `-O2` 会把太简单的源码优化成 `select` 或 lookup table，所以 case 里用外部调用保留真实分支形状。

后续如果要比较“是否和源码控制流一致”，再补 CFG 级比较或接入类似 SAILR evaluation 的 source/decompiled metric。当前先不做，避免测试框架过重。

# 判断标准

- `structuring-source-cases` 可以通过 CTest 跑起来。
- 第一批 case 有明确源码、manifest 和 oracle。
- 当前 SAILR 输出的 goto 数量被记录下来，后续减少 goto 时可以收紧阈值。
- 新增测试不影响现有 structuring smoke。

# 实现记录

改动：

- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:26`：新增 `structuring-source-cases` CTest 入口。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:31`：`default_clang()` 优先使用 `NOTDEC_SOURCE_CASES_CLANG`，其次使用仓库内 LLVM 22 clang。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:41`：`count_metric()` 统计 `goto`、`break`、`continue`、`switch`、`while`、`do`。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:57`：`merge_oracle()` 合并全局 bad-pattern oracle 和 case 局部 oracle，避免局部配置覆盖全局规则。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:72`：`check_oracle()` 支持 contains、absent、regex、精确计数、上下限和顺序检查。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:131`：`run_case()` 对每个 case 执行 clang -> LLVM IR -> notdec-llvm2c -> oracle 检查。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:207`：`main()` 支持 manifest、clang、work-dir 和 keep-work-dir 参数。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:1`：定义默认算法、优化级别、全局 bad-pattern oracle 和 5 个 case 的局部 oracle。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/001_diamond_if_else.c:5`：diamond if/else 合流 case。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/002_loop_break_continue.c:4`：loop break/continue case。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/003_switch_cluster.c:7`：switch cluster case。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/004_tail_merge_return.c:3`：tail merge return case。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/005_nested_loop_switch.c:4`：nested loop + switch case。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases --keep-work-dir
ctest --test-dir build -R structuring-source-cases --output-on-failure
```

结果：全部通过，CTest 用时约 1.04s。

当前输出指标：

| case | goto | break | continue | switch | while |
| --- | ---: | ---: | ---: | ---: | ---: |
| diamond_if_else | 0 | 0 | 0 | 0 | 0 |
| loop_break_continue | 3 | 4 | 2 | 0 | 1 |
| switch_cluster | 0 | 4 | 0 | 1 | 0 |
| tail_merge_return | 0 | 0 | 0 | 0 | 0 |
| nested_loop_switch | 4 | 2 | 1 | 1 | 1 |

评分：

- 实现效果：8/10。已经能驱动 SAILR 小步改进，并把当前 goto 数量固定下来。
- 复杂度：3/10。一个 Python runner、一个 manifest、几个小 C 文件，理解成本低。
- 维护成本：3/10。新增 case 只需要加 C 文件和 manifest oracle；风险主要是文本 oracle 后续需要收紧。

# 2026-06-30 实现记录：修 source-cases 暴露的 loop structuring 问题

这轮基于 5 个 source case 跑 SAILR，发现两个明确问题：

- `loop_break_continue` 里结构树把已经结构化的 loop source 再包了一层 branch source rewrite，导致 do-while 后面出现回到 loop body 的尾巴。这个输出会把 `call < 0` 的早退 break 错接回循环，属于语义 bug。
- `nested_loop_switch` 里入口 guard 没折成 `if (...) return;`，而是按 block id 把 return 放在 loop 前；同时 return 后还残留不可达语句。

改动：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1455`：`buildVirtualizedBranchSource()` 遇到 source 已经包含 structured loop 时，不再额外生成 branch fallback，避免 loop 后重复补回跳。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1804`：`rewriteLoopSuccessorExits()` 成功把 loop successor exit 重写成 `break` 后，同步 virtualize graph edge 并 detach overlay edge。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2205`：`endsWithUnconditionalRenderedTransfer()` 把 `Return` / `Unreachable` 也视为无条件结束，并识别 then/else 都结束的 `if`，用于删除后续死 `break`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2219`：`containsEnterableNode()` 只把被控制转移命中的 label 视为可进入，避免 return 后未命中 label 阻止不可达清理。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2296`：`dropGotoIntoFollowingNode()` 对 `if` 分支跳到下一块的情况删除该分支，让它自然 fallthrough。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2381`：新增 `collectGotoTargets()`，递归收集 tree 内真实控制转移目标，避免删掉 loop 内仍被 if/goto 命中的 label。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2434`：新增 `foldGuardedTerminalIntoSequence()`，把 `if goto return-block; goto loop-entry; label return-block; return; loop` 折成 guard return + 后续 loop；折叠前要求 terminal label 只有一个控制转移引用。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2571`、`:2693`：fold 和 cleanup 先复制子节点列表/结构节点再 `addNode()`，避免 `StructuredTree` vector 扩容后引用失效。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2688`：`cleanupStructuredGotos()` 接入 guarded terminal fold。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:3368`：do-while 只在 while follow 找不到、但 do-while follow 明确时优先，避免错误偏向 do-while。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:166`：新增 `RenderContext`，只在真实 loop/switch 上下文里渲染 `break` / `continue`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:542`：`collectGotoTargets()` 对上下文外的 `break` / `continue` 收集 label，保证后续降级为 goto 时 label 会输出。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:733`：上下文外的 `break` / `continue` 降级为 `goto target`，避免函数体顶层输出非法 `continue;`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:7`：新增 bad-pattern oracle，禁止顶层 `break;` / `continue;` 和 return 后不可达语句。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:31`、`:66`：收紧 `loop_break_continue` 的 goto 上限到 1，`nested_loop_switch` 的 goto 上限到 2。

验证：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：全部通过。

当前输出指标：

| case | goto | break | continue | switch | while | do |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| diamond_if_else | 0 | 0 | 0 | 0 | 0 | 0 |
| loop_break_continue | 1 | 2 | 1 | 0 | 1 | 1 |
| switch_cluster | 0 | 4 | 0 | 1 | 0 | 0 |
| tail_merge_return | 0 | 0 | 0 | 0 | 0 | 0 |
| nested_loop_switch | 2 | 1 | 1 | 1 | 1 | 1 |

仍保留的问题：`nested_loop_switch` 的两个 goto 来自 switch case body 被留在 switch 后面的标签区。要继续降到 0，需要专门做 loop-header switch 的 case body 折叠或复制，不能和这次的线性 cleanup 混在一起。

fortune 后端 smoke：

```bash
/usr/bin/time -f '%e' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-source-cases-smoke.c --algo=structured-sailr
```

三次早期同口径运行：95.94s、95.83s、96.03s。修掉 cleanup 引用失效后复跑一次：97.20s，正常退出。

评分：

- 实现效果：8/10。修掉两个明确坏形状，`loop_break_continue` 降到 1 个 goto，`nested_loop_switch` 降到 2 个 goto。
- 复杂度：5/10。新增 cleanup fold 和 renderer 上下文，范围不大但要维护 tree 语义。
- 维护成本：5/10。后续如果加入 CFG 级 oracle，需要把这些 cleanup 规则再做更系统的 reachability 检查。

# 2026-06-30 实现记录：继续增加 source case 和 xfail 记录

这轮继续按“小源码 case 驱动”的方式推进，新增两个 case：

- `early_return_chain`：覆盖连续 guard return。当前 SAILR 输出 0 个 goto，作为通过 case 固定下来。
- `nested_loop_break_continue`：覆盖外层 loop 内嵌内层 loop，内层有 break/continue，外层 latch 在内层之后。当前 SAILR 把内层 loop 拆到外层 loop 外面，输出 3 个 goto，并把外层 latch 放到 `break/continue` 后面的不可达位置。这是明确坏形状，但修复涉及 loop region/overlay 归约顺序，先作为 xfail 记录，不在本轮硬改。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/cases/006_early_return_chain.c:3`：新增连续 early return case。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/007_nested_loop_break_continue.c:4`：新增嵌套 loop break/continue case。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:198`：支持 `xfail` 和 `xfail_contains`；只有所有失败都匹配已知失败文本时才算 expected failure，意外通过会报错。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:70`：接入 `early_return_chain`，要求 `goto<=0`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:78`：接入 `nested_loop_break_continue`，严格要求 `goto<=0`，当前标为 xfail，等待后续专门修 loop 嵌套归约。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-goal
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-goal --keep-work-dir
```

结果：通过。`nested_loop_break_continue` 仍是 expected failure，当前输出指标为 `goto=3, break=3, continue=2, while=2, do=2`。

本轮只改测试和 runner，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：新增 guarded loop skip 小 case

这轮继续把大 case 拆小，新增 `guarded_loop_skip`，专门覆盖入口 guard 直接跳过 loop 到后续块的形状。当前 SAILR 输出仍有 1 个 goto：

```c
if (n <= 0) {
    goto structured_block_6;
}
do { ... } while (...);
structured_block_6:
```

这个问题也出现在 `loop_break_continue`，但新 case 去掉了 break/continue，只留下 guard + loop + loop 后 sink/return，后续可以更集中地修。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/cases/008_guarded_loop_skip.c:1`：新增外部 `next()` 调用，避免 clang `-O2` 把 loop 折成闭式表达式。
- `external/NotDec-llvm2c/test/structuring/source-cases/cases/008_guarded_loop_skip.c:4`：新增 guarded loop source case。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:89`：接入 `guarded_loop_skip`，期望 `goto<=0`，当前标为 xfail。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-guarded
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-guarded --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：通过。`guarded_loop_skip` 是 expected failure，当前输出指标为 `goto=1, break=0, continue=0, while=1, do=1`。

本轮只增加测试和已知失败记录，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：收紧 xfail 精确匹配

这轮没有改 SAILR 算法，只收紧 source-case runner 对 xfail 的判断。之前 `xfail_contains` 只要求每个失败都匹配一组宽泛文本，`loop_switch_latch` 用 `unexpected pattern` 会把其它全局坏形状也吞掉。新增 `xfail_exact` 后，已知失败的数量和文本都要匹配，防止 xfail 掩盖新问题。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:121`：新增 `is_expected_xfail()`，保留旧的 `xfail_contains` 宽松语义。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:122`：新增 `xfail_exact`，要求失败数量相同，且 failure 和 expected 双向匹配。
- `external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py:220`：runner 改为统一调用 `is_expected_xfail()`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:102`：`loop_switch_latch` 改用 `xfail_exact`，只允许 return 后 label 坏形状和 `goto<=0` 两个已知失败。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-xfail-exact
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-xfail-exact --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：通过。本轮只改测试 runner 和 manifest，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：收紧 loop switch latch oracle

这轮继续收紧 `loop_switch_latch` 的 xfail oracle。这个源码只有一个 loop，当前输出却有两个 `while (1)`，说明除了 goto 和 return 后 label 外，还存在多包了一层 loop 的坏形状。把 `while<=1` 加到 oracle 后，xfail 只允许这三个已知失败。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:104`：`loop_switch_latch` 的 `xfail_exact` 新增 `expected while<=1`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:110`：`loop_switch_latch` 新增 `max_counts.while=1`。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-tight-oracle2
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-tight-oracle2 --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：通过。`loop_switch_latch` 当前指标是 `goto=1, break=2, continue=3, switch=1, while=2, do=0`。本轮只改 manifest oracle，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：新增 loop two returns 小 case

这轮继续把 `loop_switch_latch` 里的坏形状拆小。新增 `loop_two_returns`，只覆盖 loop 内 early return 和 loop 后 return。当前 SAILR 输出没有 goto，但出现连续 return：

```c
return *(int *)&total_0_reg2mem + 1;
return total_0_reload;
```

这说明 return 后不可达语句可以独立复现，不必绑在 loop-switch-latch 的 switch/latch 问题里。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/cases/010_loop_two_returns.c:1`：新增 loop 内 early return 的小 source case。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:113`：接入 `loop_two_returns`，当前标为 xfail，只允许 return 后不可达语句这个已知失败。

验证：

```bash
rm -rf /tmp/notdec-structuring-source-cases-loop-two-returns
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-loop-two-returns --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：通过。`loop_two_returns` 是 expected failure，当前指标是 `goto=0, break=2, continue=1, switch=0, while=1, do=1`。本轮只增加测试和已知失败记录，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：修 guarded loop skip cleanup

这轮修掉 `guarded_loop_skip` 暴露的小问题。实际 tree 形状是：

- prefix sequence：初始化语句 + `if (skip) goto after`
- 紧跟一个已经结构化的 loop
- 再紧跟 after-loop label/body

之前 cleanup 只会删除跳到“下一个节点入口”的 goto，但这里 goto 是跳过一个 loop 到后续块，所以保留了 goto。新规则只匹配这个连续形状，把它改成 `if (!skip) { loop; } after:`，不删除 after-loop label/body，因此 loop 内已有 break 仍能落到后续块。

改动：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2532`：新增 `foldGuardedLoopSkipIntoSequence()`，折叠 preheader guard 跳过紧邻 loop 的形状。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2574`：在 `buildSequenceFromRange()` 之前复制 `IfNode` 和 `Prefix`，避免 `StructuredTree` 扩容后旧指针悬空。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:2757`：把新 fold 接入 `cleanupStructuredGotos()`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:26`：把 `loop_break_continue` 的 goto 上限从 1 收紧到 0。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:89`：去掉 `guarded_loop_skip` 的 xfail，保持 `goto<=0`。

验证：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-guarded-fix3
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-guarded-fix3 --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
/usr/bin/time -f '%e' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-guarded-loop-fix.c --algo=structured-sailr
```

结果：全部通过。fortune smoke 正常退出，耗时 `94.59s`，和之前 `95.94s/95.83s/96.03s/97.20s` 同量级。

当前输出指标：

| case | goto | break | continue | switch | while | do |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| diamond_if_else | 0 | 0 | 0 | 0 | 0 | 0 |
| early_return_chain | 0 | 0 | 0 | 0 | 0 | 0 |
| guarded_loop_skip | 0 | 0 | 0 | 0 | 1 | 1 |
| loop_break_continue | 0 | 2 | 1 | 0 | 1 | 1 |
| nested_loop_break_continue | 3 | 3 | 2 | 0 | 2 | 2 |
| nested_loop_switch | 2 | 1 | 1 | 1 | 1 | 1 |
| switch_cluster | 0 | 4 | 0 | 1 | 0 | 0 |
| tail_merge_return | 0 | 0 | 0 | 0 | 0 | 0 |

评分：

- 实现效果：8/10。修掉一个独立 xfail，并顺带把 `loop_break_continue` 降到 0 个 goto。
- 复杂度：4/10。只是 cleanup 的一个窄 fold，但要注意 `StructuredTree` 扩容后的指针失效。
- 维护成本：4/10。规则匹配严格，后续如果做 CFG 级 oracle，可以用它覆盖更多 skip-loop 形状。

# 2026-06-30 实现记录：新增 loop switch latch 小 case

这轮继续拆 `nested_loop_switch` 的剩余 goto。新增 `loop_switch_latch`，去掉 `sink()` 和额外 tail merge，只保留 loop header switch、case 0 continue、case 1 走共享 latch、default return。

当前输出还有 1 个 goto，并且出现 return 后再落到 label 的坏形状：

```c
if (limit_addr_09_reload <= 1) {
    goto structured_block_7;
}
...
return add;
structured_block_7:
return *(int *)&total_0_ph14_reg2mem;
```

这个 case 说明 `nested_loop_switch` 不是单纯文本 cleanup。要把 switch case 的 goto 降掉，通常需要把共享 latch 复制进 case body；但复制后 case body 里的 `break` 必须表示跳出外层 loop，而不是跳出 switch。当前 renderer 对 `break` / `continue` 只看是否处于 switch/loop 上下文，没有按目标块区分，所以这轮不直接改算法。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/cases/009_loop_switch_latch.c:1`：新增最小 loop-switch-latch source case。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:98`：接入 `loop_switch_latch`，期望 `goto<=0`，当前标为 xfail，匹配已知的 goto 和 return 后 label 坏形状。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-loop-switch-latch
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-loop-switch-latch --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：通过。`loop_switch_latch` 是 expected failure，当前输出指标为 `goto=1, break=2, continue=3, switch=1, while=2, do=0`。

本轮只增加测试和已知失败记录，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：新增 tail early return 小 case

这轮继续查 `loop_two_returns`，尝试过两条小修路线：

- 让 terminal-to-terminal exit 只有 return payload 相同才转成 `break`。
- 在 renderer 里按 `break` 的目标区分 loop follow 和其它 terminal return。

两条都没有形成可提交修复：前者会把现有 xfail 形状改乱但仍不解决 `loop_two_returns`；后者需要可靠维护 loop 的 break target，继续做会变成结构树目标语义重构，不适合在这个小 case 上硬猜。

因此本轮先把问题再拆小，新增 `tail_early_return`。它去掉 header guard 的额外 goto，只保留 loop 尾部 early return 和 loop 后 return。当前输出仍会把 early return 误结构成 `break`，最后生成连续两个 return：

```c
if (call > -1) {
    continue;
} else {
    break;
}
...
return *(int *)&total_0_reg2mem + 1;
return add;
```

这个 case 更清楚地说明问题在 “tail `if (continue) else break` 折 do-while 时没有区分不同 terminal exit”，后续应从结构树的 loop follow / target 语义修，不应只在 renderer 里猜。

改动：

- `external/NotDec-llvm2c/test/structuring/source-cases/cases/011_tail_early_return.c:3`：新增尾部 early return 小 case。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:127`：接入 `tail_early_return`，期望 `goto<=0`，当前只允许 return 后不可达语句这个已知失败。

验证：

```bash
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-tail-early-return-xfail
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-tail-early-return-xfail --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
```

结果：全部通过。`tail_early_return` 是 expected failure，当前只触发 return 后不可达语句，不触发 goto 失败。

本轮只增加测试和已知失败记录，没有改 SAILR 算法，所以不跑 fortune 性能 smoke。

# 2026-06-30 实现记录：修 loop terminal break 渲染

这轮修掉 `loop_two_returns` 和 `tail_early_return` 的 xfail。两个 case 的共同形状是 loop 有两个 return 出口：一个是 loop 正常结束后的 return，另一个是 loop body 里的 early return。之前 SAILR 会把两个 terminal exit 都结构成 `break`，C 里的 `break` 只能落到 loop 后的一个位置，结果生成连续 return，语义错。

这次不改 region 归约，只补齐 loop node 的 break target，并让 renderer 在 `break` 目标不是当前 loop follow、且目标是 return terminal block 时直接渲染成 return。这样 early return 不再被误写成 `break`，后续同一个 terminal block 也不会再重复输出。

改动：

- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:131`：新增 `graphNodeEntryBlock()`，把 graph follow node 转成 loop 的 `BreakTarget`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:852`、`:919`、`:1040`：线性 while / while-with-break / do-while reducer 写入 loop `BreakTarget`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:1083`、`:1089`：self-loop reducer 写入条件分支另一侧作为 `BreakTarget`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:3320`、`:3375`、`:3402`、`:3592`：graph natural loop 和 fallback loop 写入 `BreakTarget`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:165`：新增 `InlinedTerminalTargets`，避免 targeted early return 后重复输出同一 terminal block。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:174`：`RenderContext` 增加 `BreakTarget`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:558`：收集 label 时按 `BreakTarget` 判断目标不匹配的 break；可内联 return 的目标不再生成 label。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:575`：进入 while / do-while / infinite loop 时把 loop `BreakTarget` 传给子节点。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:778`：新增 `canRenderBreak()`，只有目标为空、目标未知或目标等于当前 `BreakTarget` 时才输出 C `break;`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:787`、`:792`：新增 targeted return 内联逻辑。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:114`、`:123`：去掉 `loop_two_returns` 和 `tail_early_return` 的 xfail，作为正常通过 case 固定下来。

验证：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-break-target4
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-break-target4 --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
/usr/bin/time -f '%e' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-break-target-final.c --algo=structured-sailr
```

结果：全部通过。fortune smoke 正常退出，耗时 `94.86s`，和之前 `94.59s/95.94s/95.83s/96.03s/97.20s` 同量级。

这轮输出里：

- `loop_two_returns`：`goto=0`，early return 输出为 loop 后 `return total_0_reload;`，正常 loop exit 输出为 loop 内 guard return。
- `tail_early_return`：`goto=0`，early return 输出为 loop 后 `return add;`，正常 loop exit 输出为 loop 内 guard return。

评分：

- 实现效果：8/10。修掉两个明确 xfail，且没有引入新 goto。
- 复杂度：5/10。需要把 loop follow target 传到 renderer，但没有改 region 识别和复制策略。
- 维护成本：5/10。后续如果支持非 return 的 targeted break，还需要更完整的 target-aware control transfer；当前只处理明确 return terminal。

# 2026-06-30 实现记录：内联 goto 到 return terminal

这轮继续收窄 `loop_switch_latch` 的 xfail。上一轮 targeted `break` 已能内联 return，但普通 `goto` 到 return terminal 仍会生成 label，导致 `loop_switch_latch` 同时触发 goto、return 后 label、while 过多三个失败。

这次只复用已有 `renderTargetedTerminal()`，让 renderer 遇到 `Goto` 指向 return terminal block 时直接输出 return；label 收集也跳过这种可内联目标。没有改 SAILR region 归约。

改动：

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:554`：`collectGotoTargets()` 对可内联 return terminal 的 `Goto` 不再收集 label。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:743`：`Goto` 渲染先尝试 `renderTargetedTerminal()`，失败时才输出 `goto label`。
- `external/NotDec-llvm2c/test/structuring/source-cases/manifest.json:102`：把 `loop_switch_latch` 的 `xfail_exact` 收紧为只允许 `expected while<=1`。

验证：

```bash
cmake --build ./build --target notdec-llvm2c-exe -j4
python3 -m py_compile external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py
rm -rf /tmp/notdec-structuring-source-cases-goto-return2
python3 external/NotDec-llvm2c/test/structuring/source-cases/run_source_structuring_suite.py \
  --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  --work-dir /tmp/notdec-structuring-source-cases-goto-return2 --keep-work-dir
ctest --test-dir build -R 'structuring-(source-cases|analysis)|structured-phoenix-available|legacy-phoenix-removed|shared-structurer-registry' --output-on-failure
/usr/bin/time -f '%e' ./build/external/NotDec-llvm2c/bin/notdec-llvm2c \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune-goto-return.c --algo=structured-sailr
```

结果：全部通过。fortune smoke 正常退出，耗时 `96.64s`，仍在之前 `94.59s/94.86s/95.94s/95.83s/96.03s/97.20s` 范围内。

当前影响：

- `loop_switch_latch`：`goto=0`，不再触发 return 后 label，只剩 `while=2` 的 expected failure。
- `nested_loop_break_continue`：`goto` 从 3 降到 2，但结构仍明显不对，继续保留 xfail。

评分：

- 实现效果：7/10。减少了明确无用 goto，并收紧了 xfail。
- 复杂度：2/10。只复用已有 terminal return 内联逻辑。
- 维护成本：3/10。只处理 return terminal，不扩展非 terminal goto。

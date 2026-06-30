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

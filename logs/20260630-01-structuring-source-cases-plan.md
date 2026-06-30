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

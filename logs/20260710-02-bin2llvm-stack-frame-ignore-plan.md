# 原始 Prompt

> 先修复一下那个stack-frame rewrite 给出的模块级忽略集合吧，是整个程序都没有用rbp作为栈底部寄存器吗，应该识别这种情况，仅返回RSP啊，看看具体问题

# bin2llvm stack-frame ignored register 修复计划（已完成）

## 背景

fortune 的 stack-frame rewrite 最终返回 `{RBP, RSP}` 作为模块级 ignored register。
调试确认只有 `FUN_3470` 替换了两个 RBP load，但这个函数并不把 RBP 固定当作 frame
pointer：入口先用 RBP 保存 RDX，后续局部路径才把 RSP 派生地址放进 RBP。

当前 load 替换本身按控制流跟踪 RBP 当前值，只替换确认来自 RSP 的 load；问题在于替换
成功后又把 RBP 放进整个模块的 ignored register 集合。这会让 `main` 中正常的
`RDI -> RBP` 参数传递不参与 RegisterSummary。

## 目标

1. 保留已经确认安全的 RSP 派生 RBP load 替换。
2. 不再因为局部 RBP load 被替换，就忽略整个模块的 RBP。
3. ignored register 默认只包含由 stack-frame rewrite 完整接管的 RSP。
4. fortune 的 `main` 能重新识别 RDI 入口读取，不再因 RBP 全局忽略而漏掉 argc。

## 技术路线

- `replaceFramePointerLoads()` 继续按基本块跟踪 RBP 当前值，只替换确定来自 RSP 的 load。
- `runNativeStackFrameRewrite()` 不再把命中的 frame register 加入模块级
  `IgnoredRegisters`。
- 保留 RSP ignored，因为 RSP 的栈地址语义由 stack-frame rewrite 和 cleanup 专门处理。
- 增加跨函数回归：一个函数触发 RBP 的局部栈地址替换，另一个函数通过 RBP 暂存 RDI；
  后者的 RDI 仍必须被 RegisterSummary 识别为 `ReadEntry`。

## 风险

1. RBP bookkeeping 不再由 ignored-register cleanup 统一删除，需要确认现有 SummarySSA 和
   DCE 能清理已替换后的无用访问。
2. 某些真正使用固定 frame pointer 的函数可能保留少量 RBP store，但不能以牺牲其他函数
   的参数语义为代价做模块级忽略。
3. 本次只修 ignored register 粒度，不处理 fortune 中独立存在的 variadic R9 误判和
   zero-demand RSI 丢失问题。

## 判断标准

1. stack-frame 单元测试通过，RSP 仍在 ignored register 集合中。
2. RBP frame-derived load 仍能被替换，但 RBP 不进入模块级 ignored register 集合。
3. 跨函数测试中，RDI 仍被识别为 `ReadEntry`。
4. fortune IR 通过 LLVM 22 verifier。
5. fortune 的 `main` 参数中恢复 RDI，且 `@RDI` register residue 消失或明显减少。

## 实现记录

完成日期：2026-07-10。

### 原因

gdb 确认 fortune 中只有 `FUN_3470` 命中 `replaceFramePointerLoads()`，共替换两个 RBP
load。该函数入口先执行 `mov %rdx, %rbp`，后续局部路径才执行类似
`lea 0x40(%rsp), %rbp` 的操作，因此 RBP 不是固定 frame pointer。

原实现的路径敏感替换是正确的，但只要任意函数替换过 RBP load，就会把 RBP 加入模块级
`IgnoredRegisters`。这让 `main` 中正常的 `mov %edi, %ebp` 不参与 RegisterSummary，
最终漏掉 RDI 参数。

### 改动

- `include/notdec-bin2llvm/passes/summary/NativeStackFrame.h:23-27`
  - 说明 ignored register 只能包含语义被 stack rewrite 完整接管的寄存器。
- `lib/passes/summary/NativeStackFrame.cpp:669-687`
  - `runNativeStackFrameRewrite()` 仍调用 `replaceFramePointerLoads()`。
  - 命中 RBP load 后只累计替换数量，不再把 RBP 加入模块级 ignored 集合。
  - stack pointer 仍加入 ignored 集合，x86-64 下为 RSP。
- `tests/native_register_summary_ssa_test.cpp:4233-4329`
  - 更新 RBP frame-load 测试，要求局部替换成功但 RBP 不被全局忽略。
  - 新增跨函数回归，确认另一个函数的 `RDI -> RBP` 仍产生 `RDI ReadEntry=true`。
- `ARCHITECTURE.md:258-263`
  - 更新 stack-frame rewrite 和 ignored register 的实际语义。

### 验证

通过：

```bash
cmake --build build --target \
  native_register_summary_test \
  native_register_summary_ssa_test \
  notdec-native-llvm -j4

./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test
```

fortune 产物：

- IR：`/tmp/notdec-bin2llvm-fortune-stack-ignore-fix-20260710/fortune.native.ll`
- bitcode：`/tmp/notdec-bin2llvm-fortune-stack-ignore-fix-20260710/fortune.native.bc`
- warning：
  `/tmp/notdec-bin2llvm-fortune-stack-ignore-fix-20260710/register-ssa-warnings.tsv`

LLVM 22 `llvm-as` 和 `opt -passes=verify` 均通过。修复后：

```llvm
define i64 @main(i64 %RSI.arg, i32 %RDI.arg, i64 %R9.arg)
```

- RDI 正确进入 `main` 参数并连接到 argc 数据流。
- `@RDI`、`@RBP`、`@RSP` 和对应 partial read/write residue 均消失。
- `recode_string` 的两个 callsite 都推断为 2 个参数，不再输出 arity 不一致 warning。
- RSI 被 zero-demand 改成 0、R9 variadic 假参数仍是独立问题，本次未处理。

### 评价

- 实现效果：9/10。修复了 RBP 模块级忽略导致的参数漏判，同时保留局部栈地址恢复。
- 复杂度：9/10。只删除错误的全局状态扩散，没有新增新的分析域或特殊匹配。
- 后期维护成本：9/10。ignored register 的边界更清楚，回归测试覆盖跨函数误伤场景。

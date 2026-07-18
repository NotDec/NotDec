# 原始 prompt

你这里第一点是对的，第一点可以直接先修复了吧。对于外部call建模时，仅保留callee-saved，其他 volatile 寄存器先按 clobber 处理。

但是第二点里面其实怎么样造unknown值那都是不太对的。就直接正常地把值取出来就行，不要管那么乱七八糟的吧？即使存在这种clobber的问题，也最多打印个warning。

但是我记得关于call的参数和返回值的东西，是不是会先显式表示出来，后面再rewrite成真正的参数和返回值。为什么这里要去操心这种call clobber的问题，而不是简单地把参数对应的寄存器的值直接拿出来？难道就是最后这步SummarySSA rewrite做的？

# 背景

`NativeRegisterSummary` 之前对外部 call 的寄存器影响只看 ABI 输出和 killed-by-call 表。对 x86-64 SysV 来说，这会漏掉很多 volatile GPR，比如 `R8`。结果是：某个 call 之前写过 `R8`，之后另一个 vararg call 没重新写 `R8`，分析仍可能把旧 `R8` 当作第二个 call 的参数。

这不符合 ABI。外部 call 后，除了 callee-saved / unaffected 寄存器和 `FS_OFFSET`、`GS_OFFSET` 这种段基址，其它通用寄存器都不能默认透明保留。

# 修改

## external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp

- 第 419 行新增 `isSegmentBaseUnit()`，把 `FS_OFFSET`、`GS_OFFSET` 作为外部 call 后仍保留的特殊寄存器。
- 第 1969 行修改 `applyAbiCallClobbers()`：
  - 跳过 ignored、ABI unaffected、segment base。
  - 其它寄存器按 full mask 写成 `CallProduced`。
  - 不再只依赖 ABI output / killed-by-call 表。
- 第 2154 行修改 `applyBackwardCallDemand()`：
  - 外部 call 反向传播时，跳过 ignored、ABI unaffected、segment base。
  - 其它寄存器的 full demand 直接被 call 截断。

## external/NotDec-bin2llvm/tests/native_register_summary_test.cpp

- 第 593 行附近更新 `testUnknownExternalCallsiteEvidenceClassifiesOrigins()`。
- 现在测试期望跨过外部 call 的 `RSI` 被标成 `CallProduced`，不再是 `ForwardedEntry`。

## external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp

- 第 2924 行新增 `testKnownVarArgDoesNotReuseStaleCallerSavedTail()`。
- 覆盖场景：第一次 `__fprintf_chk` 使用 `R8` vararg，第二次 call 只重新定义到 `RCX`。第二次不能复用第一次 call 前的旧 `R8`。
- 第 7505 行把该测试加入 main。

# 验证

命令：

```bash
cmake --build external/NotDec-bin2llvm/build --target native_register_summary_test native_register_summary_ssa_test -j4
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
cmake --build external/NotDec-bin2llvm/build --target notdec-native-llvm -j4
```

结果：全部通过。

fortune 重新跑在：

```text
/tmp/notdec-bin2llvm-fortune-external-clobber-20260718-163451/fortune.native.ll
```

检查结果：

- `llvm-as` 通过。
- 原来错误的第二个 `__fprintf_chk(..., i64 0, i64 0)` 不再复用旧 `R8`。
- `fortune.native.ll` 里没有 raw GPR global load/store。
- `fortune.native.ll` 里没有 `summary_return` / `summary_clobber` 残留。

# 评价

实现效果：8/10。修掉了外部 call 后 caller-saved 寄存器被当作透明保留的问题。

复杂度：2/10。只改 call effect 建模和测试，没有引入新结构。

维护成本：2/10。规则直接来自 ABI：只保留 unaffected/callee-saved 和段基址。

剩余问题：`NativeRegisterSummarySSA` 中 unknown helper 的生成策略仍需单独讨论。本次没有修改这部分，避免把 call clobber 规则和 unknown 值策略混在一起。

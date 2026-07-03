# 原始 prompt

先尝试按照方案B推进一下试试吧

## 背景

bin2llvm native 的 register summary 之前只知道某个寄存器是否被入口读取、是否在出口被需求。这个粒度对 x86-64 的 XMM/ZMM backing register 不够：ABI 里的 `XMM0_Qa` 实际只对应 lifted `ZMM0` 的低 64 bit，但 SummarySSA 会把整 `i512` 当成函数参数、返回值或 clobber。

## 目标

保留宽寄存器 backing model，但在 summary 里记录 entry/exit demand mask。后续签名重写时，如果 XMM/ZMM 只需求 ABI low lane，就降成 `double/float` 这样的 ABI slot；只有真正需求宽寄存器时才保留整 `i512`。

## 实现记录

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h:32` 给 `NativeRegisterSummaryRegister` 增加 `EntryDemandMaskHex`、`ExitDemandMaskHex`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:104` 将 `FunctionDemand` 从寄存器集合改成 `GlobalVariable -> APInt mask`，让 top-down demand 能保留 bit 粒度。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:230` 新增 ABI storage mask 计算，`XMM0_Qa` 这类没有 register space 的别名按 low-lane 处理。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:623` 收集 ABI input/output/killed-by-call 时同步记录 mask，并把 root 函数默认返回需求限制到第一个整数返回寄存器。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:1297` 在 caller demand 分析里增加 value bit-demand，load register 时只把实际使用到的 bit mask 传播给 callee exit demand。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:1405` 在 metadata 里写出 `entry_demand_mask`、`exit_demand_mask`，并在 public summary 中导出 hex mask。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1146` 读取 public summary 的 demand mask。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1290` 增加 float ABI slot demand 匹配，`shapeForInternalFunction()` 对低 32/64 bit 的 XMM/ZMM demand 生成 `float/double` slot。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2885` 对未知外部调用，不再把 float ABI backing unit 的 killed-by-call 直接变成整 `ZMM` clobber helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_test.cpp:334` 增加 root demand 回归，确认只有整数 ABI 返回被默认需求。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3635` 增加未知外部 float clobber 回归，确认不会生成 `summary_clobber.i512`。

## 验证

- `cmake --build ./build --target native_register_summary_ssa_test native_register_summary_test notdec-native-llvm -j4`：通过。
- `./build/bin/native_register_summary_ssa_test`：通过。
- `./build/bin/native_register_summary_test`：通过。
- fortune native summary-residue audit：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-demand-mask-final-20260703124750`
  - IR：`/tmp/notdec-bin2llvm-fortune-demand-mask-final-20260703124750/fortune-executable.summary-residue.ll`
  - `seconds=7`，`lines=3242`
  - `summary_clobber.i512=0`
  - `summary_return.i512=0`
  - `ZMM*.clobber=0`
  - `ZMM*.return=0`
  - `i512 %ZMM*.arg=1`，剩余位置是内部递归函数接口，不是外部 ABI clobber 残留。
- `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-fortune-demand-mask-final-20260703124750/fortune-executable.summary-residue.ll -o /tmp/notdec-bin2llvm-fortune-demand-mask-final-20260703124750/fortune-executable.summary-residue.bc`：通过。

## 效果和风险

- 实现效果：8/10。fortune 里整 ZMM summary helper 已清掉，`main` 不再被误改成 `double/i512` 返回。
- 复杂度：6/10。summary 多了一层 bit mask，但范围只在 register summary / SummarySSA 内部。
- 维护成本：6/10。后续如果要支持更多 vector ABI slot，需要继续扩展 slot 类型判断；当前只按 low-lane `float/double` 保守处理。
- 性能：fortune 同口径 6-7 秒，没有看到明显退化。

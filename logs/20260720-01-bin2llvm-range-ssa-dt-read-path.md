本次用户 prompt：

> 按照这个改一下吧。

# bin2llvm range SSA 强制带 DominatorTree 读值实现记录

## 背景

fortune 里 CMOV 附近的 `range_unknown` 不是 CMOV 内部 CFG 本身造成的。直接原因是 `rewritePartialReads()` 走了无 DominatorTree 的 range read 路径，`completeRangePhi()` 补 PHI incoming 时无法判断跨 block 值是否支配 terminator，于是把可用的 passthrough 值误判为 unknown。

## 修改内容

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3606`
  - `valueDominatesUse()` 改成必须接收 `llvm::DominatorTree &`，不再允许 `nullptr`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3628`
  - 删除无 DT 的 `assembleRangeRead()` / `readAccessRange()` 旧入口，只保留 `assembleRangeReadIfDominating()` / `readAccessRangeIfDominating()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3682`
  - `readAccessRangeIfDominating()` 对 `tryCurrentCoveredAccess()` 的短路结果也统一做支配性检查，避免绕过 DT。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3737`
  - `rewritePartialReads()` 构建 `llvm::DominatorTree domTree(Function)`，partial read 重写统一走带 DT 的 range read 路径。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4693`
  - `readRangeBefore()` / `readRangeEntry()` / `readRangeExit()` / `completeRangePhi()` 全部改成显式传 `llvm::DominatorTree &`，去掉默认 `nullptr` 后门。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:1349`
  - 新增 `testPartialReadLoopPassthroughUsesDominatorTree()`，覆盖 loop/join 中未被当前分支修改的 partial range 应透传，不应生成 `notdec.unknown.*`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:7723`
  - 将新测试接入 `native_register_summary_ssa_test` 主流程。

## 验证

- 构建：
  - `cmake --build external/NotDec-bin2llvm/build --target notdec-native-llvm -j4`
  - `cmake --build external/NotDec-bin2llvm/build --target native_register_summary_ssa_test -j4`
- 单测：
  - `external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test`
- fortune：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-dt-range-20260720-054800`
  - `llvm-22.1.0.obj/bin/llvm-as fortune.native.ll`
  - `llvm-22.1.0.obj/bin/opt -passes=verify fortune.native.bc -disable-output`
  - `python3 external/NotDec-bin2llvm/scripts/native-register-residue-audit.py --details fortune.native.ll`

验证结果：

- `native_register_summary_ssa_test` 通过。
- fortune IR 通过 `llvm-as` 和 `opt -passes=verify`。
- fortune register residue audit 只有表头，raw register 残留为 0。
- fortune 的 `call_arg_uses_unknown_value` / `vararg_arg_uses_unknown_value` warning 消失。
- fortune `bb_2a90` / `bb_2aa6` / `bb_2aad` 附近 RCX/RSI/R9 的 `range_unknown` 消失，PHI incoming 改为正常 passthrough 值。

## 评分

- 实现效果：9/10。直接堵住无 DT range read 入口，并修复 fortune 目标问题。
- 复杂度：7/10。删除旧入口后代码更少，但函数签名收紧影响 SummarySSA 内部多处递归读值。
- 维护成本：8/10。后续新增 range read 路径必须显式传 DT，更不容易复发。

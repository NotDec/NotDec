# bin2llvm SummarySSA delayed cleanup implementation

## 背景

本次修复的是 SummarySSA 清理顺序问题：`rewritePartialReads()` 和重复 partial-read xor 折叠后，旧 helper 还会作为 `Replacement` 的 key 被后续 range read / call argument binding 使用。过早 `eraseFromParent()` 会让 LLVM 复用旧指针地址，导致 `resolve()` 命中陈旧映射，进而出现错误参数绑定和虚假 `0`。

## 实现

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1995`-`2041`
  - 把 replaced partial read / folded xor / replaced load 的物理删除推迟到 liveness cleanup 之后。
  - post-signature cleanup 也采用同样顺序。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2050`-`2056`
  - 将延迟删除列表改成 `llvm::WeakVH`，避免列表里保留已删除 instruction 指针。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3449`-`3466`
  - 增加 replaced metadata 判断和 `forgetReplacementValue()`，删除 helper 前同步清掉 `Replacement`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3468`-`3538`
  - replaced partial read 不立即删除，只标记 metadata 并延迟清理；删除前保护 signature side table 里记录的值。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3362`-`3428`
  - range 拼接支持 `allowUnknownSegments`，用于保留已知片段并把无合法来源的片段变成 `freeze poison`。
  - 如果 range 是被 unknown call/clobber 阻断，则仍然保持严格失败，避免把 call 后 raw load 错替换成 poison。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4747`-`4782`
  - `entryRangeInput()` 只在 summary fact 允许 `MayEntry` 时生成入口 load / partial_read，避免为 `may_entry=false` 的寄存器凭空造 entry read。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4580`-`4582`、`5017`-`5035`、`5293`-`5297`
  - full load replacement 和 function return collection 使用 unknown-segment 模式；callsite 参数推断保持严格读取，避免 unknown 参数撑大 arity。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3858`-`3895`
  - dead store / partial write cleanup 不再递归删除 operand 链，避免删掉 signature rewrite side table 里还会使用的值。
  - liveness 忽略已替换的 SummarySSA helper，但保留被 call arg / return side table 记录的 store/value。

## 验证

- `cmake --build external/NotDec-bin2llvm/build --target native_register_summary_ssa_test -j4`
- `external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test`
- `ctest --test-dir external/NotDec-bin2llvm/build -R notdec.native_register_summary.ssa --output-on-failure`
- `cmake --build external/NotDec-bin2llvm/build --target notdec-native-llvm -j4`
- fortune native：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-clean-range-unknown-20260717-060020`
  - `llvm-as` 通过。
  - `opt -passes=verify` 通过。
  - `__fprintf_chk(i64 0, i64 0, i64 0)` 数量为 `0`。
  - `register-ssa-warnings.tsv` 有 6 条 warning，仍是外部签名推断类 warning。
  - residue audit 剩 1 条 `ZMM1.range_entry`，对应 `FUN_32e0` 的 `ZMM1 may_entry=true / exit_demand=true`，不在本次清理里硬删。

## 评分

- 实现效果：8/10。修掉 stale replacement 和虚假 `0`，SummarySSA 单测和 fortune verify 通过；fortune 仍有一个向量 entry residue，需要单独分析。
- 复杂度：6/10。增加了延迟删除和 unknown-segment 开关，但范围集中在 SummarySSA。
- 维护成本：6/10。side table 保护比递归删除更直观，后续如果新增 side table，需要继续遵守“记录值先不物理删”的规则。

## 风险

- `rangeMayComeFromEntry()` 当前在 summary fact 缺失时保持兼容，仍允许 entry read；这是为了避免误伤无 summary 的边界场景。
- `allowUnknownSegments` 只在 full load replacement 和 function return collection 打开；如果后续发现其他路径也需要保留部分已知片段，应逐个审查，不能直接全局放开。

## 补充：vararg 未知值不再落成 0

### 背景

fortune 里还残留 `__fprintf_chk(..., i64 0, i64 0)` 作为后面的 vararg 参数。对 no-instcombine IR 反查后确认，这些值不是源码里的真实 0，而是 R8/R9 等未知 register range 经 `freeze poison`、PHI、`notdec.reg.insert` 拼接后，后续优化任选成了 0。

### 实现

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/passes/summary/NativeRegisterSummary.h:17`-`22`、`55`-`60`
  - 在 call input slot 和 callsite evidence 上记录 `Float`，让已知 vararg 的整数槽和浮点槽能分开推断。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:1388`-`1400`
  - callsite evidence 传递 `Float` 标记。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1150`-`1163`
  - 抽出 `floatTypeForSizeBits()`，供 ABI float slot 和 callsite vararg slot 复用。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1445`-`1465`、`1530`-`1578`
  - call input slot 保留 float 属性；vararg 推断前把 integer / float evidence 分开并重新编号。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1581`-`1629`、`1659`-`1721`
  - 对 SysV vararg 使用 `RAX/AL` 的 SSE 参数数量提示；`__fprintf_chk("%5.2f%%", double)` 这类调用现在会绑定 XMM/ZMM 低 64 位为 `double`，不会退成整数尾参。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5071`-`5118`
  - `callParamSlots()` 支持把 callsite 推断出的 float vararg slot 转成 `FloatRegister` signature slot。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:343`-`350`、`6233`-`6306`、`6473`-`6481`
  - 新增临时 `notdec.register.vararg_unknown.*` helper；vararg 尾参如果是已知 0、`freeze poison`、或经 PHI / select / cast / binop / `notdec.reg.insert` / `notdec.reg.extract` 依赖未知占位，则先改写成这个 helper。
- `external/NotDec-bin2llvm/include/notdec-bin2llvm/passes/summary/NativeRegisterFinalCleanup.h:21`-`28`、`external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterFinalCleanup.cpp:82`-`84`、`193`-`219`、`394`-`395`、`430`-`438`
  - FinalCleanup 把 `notdec.register.vararg_unknown.*` 降成 LLVM `poison`，并统计 `vararg_unknown_helpers_lowered`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:155`-`194`
  - 增加 mixed integer/float vararg ABI 测试辅助。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2913`-`2977`
  - 新增 `testKnownVarArgUsesSseCountForFloatTail()`，覆盖 `AL=1` 时优先使用 float vararg。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2979`-`3032`
  - 新增 `testKnownVarArgZeroTailUsesPoison()`，覆盖未证明的零尾参降成 poison。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3034`-`3120`
  - 新增 `testKnownVarArgUnknownPhiTailUsesPoison()`，覆盖未知 PHI 经 `notdec.reg.insert` 拼接后作为 vararg 的路径。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:7426`-`7428`
  - 注册上述新增测试。

### 验证

- `cmake --build external/NotDec-bin2llvm/build --target native_register_summary_ssa_test notdec-native-llvm -j4`
- `external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test`
- `ctest --test-dir external/NotDec-bin2llvm/build -R notdec.native_register_summary.ssa --output-on-failure`
- fortune native：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-vararg-poison-20260717-190041`
  - `llvm-as` 通过。
  - `opt -passes=verify` 通过。
  - `rg "__fprintf_chk\\([^\\n]*i64 0"` 无结果。
  - `__fprintf_chk` 中未知尾参现在显示为 `poison`；float 百分比输出保留为 `double`。
  - `register-ssa-warnings.tsv` 仍是外部签名推断 warning。
  - residue audit 仍剩 1 条 `ZMM1.range_entry`，和本次 vararg poison 修复无关。

### 评分

- 实现效果：8/10。fortune 的假 0 vararg 已清掉，float vararg 也正确落成 `double`；剩余 ZMM1 residue 仍需单独处理。
- 复杂度：6/10。多了一个 vararg unknown helper，但只在 signature rewrite 到 FinalCleanup 之间存在。
- 维护成本：5/10。unknown 依赖检查集中在 vararg 尾参绑定处，后续如果新增 register glue helper，需要把它加入递归检查。

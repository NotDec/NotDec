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

## 补充：unknown 不再用 freeze poison 表示

### 背景

上一轮把部分不确定 vararg 降成 `poison`，但继续排查后确认 LLVM 可以把 `freeze poison` 任选成具体常量，fortune 中就可能出现源码里不存在的 `0`。这次把“确实不知道的值”改成外部 opaque helper call，让优化器不能把它折叠成固定常量。

### 实现

- `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:42`-`80`
  - 新增 `notdec.unknown.<type>` helper 创建逻辑，同名同类型才复用；如果已有同名不同类型声明，则创建 `.typed` 后缀版本，避免 call 返回类型被改坏。
- `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp:1358`、`1421`-`1424`、`1443`-`1480`
  - unmodeled varnode、无 predecessor 的 missing value、non-dominating PHI missing incoming 都改为 `notdec.unknown.*()`，不再生成 `freeze poison`。
- `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:37`-`75`
  - Heritage lowering 也使用同一类 `notdec.unknown.<type>` helper。
- `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp:691`-`716`、`770`-`771`
  - register input temp、stack input temp、unmodeled varnode fallback 都改成 opaque unknown helper，同时保留原有 metadata。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:332`-`370`
  - 删除 `frozenPoisonBefore()` / `frozenPoisonAt()`，改为 `unknownValueBefore()` / `unknownValueAt()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:424`-`450`、`800`-`815`、`2077`-`2084`
  - `notdec.unknown.*` 不再当未知 external、analyzable call 或可重写函数签名处理，避免 helper 自己参与 register summary / signature rewrite。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5845`-`5849`、`5875`-`5878`、`6496`-`6498`、`6547`-`6568`
  - foreign argument、跨函数 value localize、缺失 call 参数、缺失 return/range return 都改成同类型 opaque unknown；return helper 替换前补了类型检查，避免 RAUW 类型不一致。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:6672`-`6683`
  - 残留 helper warning 反查 source call 时只认 analyzable call，避免把 `notdec.unknown.*` 或 register glue helper 误报成 callee。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:448`-`455`、`1432`-`1457`、`1460`-`1505`
  - 单测 oracle 从 freeze poison 改成 opaque unknown helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:1289`-`1294`、`5538`-`5542`
  - pass 可能重写函数签名并替换旧 `Function`，测试改为 pass 后重新从 module lookup，避免用悬空指针。

### 验证

- `cmake --build external/NotDec-bin2llvm/build --target notdec-native-llvm native_register_summary_ssa_test -j4`
- `external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test`
- fortune no-instcombine / no-residue：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-opaque-unknown-20260718-071408`
  - `llvm-as` 通过。
  - `freeze .*poison` 无结果。
  - `notdec.unknown.*` 声明同名同类型，例如 `declare i32 @notdec.unknown.i32()`。
  - `register-ssa-warnings.tsv` 不再把 `notdec.unknown.*` 或 `notdec.reg.extract.*` 当 callee 报警。
- fortune 默认链路：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-opaque-unknown-final-20260718-071729`
  - `llvm-as` 通过。
  - `freeze .*poison` 无结果。
  - `rg "__fprintf_chk\\([^\\n]*i64 0"` 无结果。
  - 最终 IR 没有寄存器全局 load/store，也没有 `notdec.register.summary_return` / `summary_clobber` 残留。
  - warning 文件只剩 6 条签名推断类 warning。

### 评分

- 实现效果：8/10。假 0 的根源从 freeze poison 改成 opaque unknown，fortune 默认输出不再出现 `__fprintf_chk` 的伪 0 vararg，也没有寄存器 residue。
- 复杂度：5/10。新增 helper 在三个 lowering/summary 文件里各有一份局部实现，逻辑简单，但后续可以考虑抽公共工具函数减少重复。
- 维护成本：4/10。`notdec.unknown.*` 被明确排除出 register summary 和签名重写，后续新增 helper 也应按这个边界处理。

### 风险

- `notdec.unknown.*` 是外部 opaque call，会保留“不知道”的事实；这会让 IR 比直接 `poison` 更保守，也更容易 debug。
- `PcodeToLLVM::unknownValueAtEnd()` 在没有 terminator 时仍只能返回裸 `poison`，这是非法 block 兜底路径，正常 lowered block 不走这里。

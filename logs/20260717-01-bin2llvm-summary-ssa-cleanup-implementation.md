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

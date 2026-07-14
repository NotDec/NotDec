# 原始 prompt

按照这两个步骤来：首先盘点一下相关pass会生成的这种底层指令，包括shl+or这种组合，考虑设计专用intrinsic，使得相关这些pass只会生成intrinsic，不会生成任何IR原生指令操作，考虑是否可以做到这一点。
其次就是这里提到的，识别完整传递，改为直接的完整的值传递。

形成一个规划文档

# 背景

当前 native register elimination 里已经有 `notdec.partial_read.*` 和
`notdec.partial_write.*`。它们解决了 lifting 阶段窄寄存器访问伪装成整寄存器
`load/store` 的问题。

但 SummarySSA 里面还会在重写阶段自己生成一些底层整数操作：

- 用 `lshr/trunc` 从完整值里抽某个 bit range。
- 用 `zext/shl/or` 把多个 range 拼成完整值。
- 用 `and/shl/or` 把 partial write 合回旧完整值。
- entry argument / return slot / call argument range 处理中也会生成类似的
  `lshr/trunc/zext/shl`。

这些操作本来只是寄存器 range 重写的内部胶水，但进入普通 LLVM IR 后，很容易被
`InstCombine` 改形状，也容易被后续分析当成真实程序逻辑。结果就是 matcher 越写越多，
仍然很脆。

这份计划只处理 bin2llvm native 寄存器重写内部生成的 bit glue，不改变真实 pcode
语义里的 `INT_SHL`、`INT_OR` 等程序操作。

# 目标

分两步推进。

第一步，盘点并收口所有由寄存器重写 pass 自己生成的底层位操作，设计专用 helper /
intrinsic，让这些内部 glue 不再直接生成 `shl/or/and/lshr/trunc/zext` 组合。

第二步，在完整 range SSA 里识别完整值传递。只要一个 full access 的所有 segment
都来自同一个 covered value，就直接传递这个完整值，不再拆 segment 再拼回去。

目标不是禁止整个 native IR 里出现 `shl/or`。真实程序里的 shift/or 必须保留。这里要禁止的是：

```text
SummarySSA 为了表示寄存器 range 抽取、插入、拼接而主动造出的原生 LLVM bit ops。
```

# 当前会生成的底层操作盘点

## 1. lifting / storage 层

相关文件：

- `external/NotDec-bin2llvm/lib/RegisterStorage.cpp`
- `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeRegisterPartialRead.h`
- `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeRegisterPartialWrite.h`

当前状态：

- partial read 已经生成 `notdec.partial_read.iFULL.iREAD(ptr @REG, i64 offset)`。
- partial write 已经生成 `notdec.partial_write.iFULL.iWRITE(ptr @REG, value, i64 offset)`。
- full-width register read/write 仍然是普通 `load/store @REG`。
- 局部还会为了类型对齐生成 `zext/trunc`，例如写入值宽度和 register global 宽度不同。

判断：

- 这里已经基本是对的。
- full read/write 不应该 helper 化，否则会让普通完整寄存器值更难看。
- 必须确认 partial helper 不被签名重写、外部函数推断、cleanup 误处理。

## 2. SummarySSA 的 partial write 合成

相关函数：

- `replacePartialRegisterValue()`

当前形状：

```llvm
%wide = zext/trunc %part to iFULL
%shifted = shl %wide, offset
%bits = and %shifted, writeMask
%keep = and %old, ~writeMask
%new = or %keep, %bits
```

这是典型“内部 glue”。它应该替换成 value-level helper：

```llvm
%new = call iFULL @notdec.reg.insert.iFULL.iWRITE(iFULL %old, iWRITE %part, i64 offset)
```

语义：以 `%old` 为 base，把 `%part` 写入 `[offset, offset + writeWidth)`，返回新的完整值。

## 3. SummarySSA 的 range 抽取

相关函数：

- `extractPartialRegisterValue()`
- `extractBitsFromInteger()`
- `extractReturnRange()`
- `integerEntrySlotReplacement()`
- `rangeEntrySlotReplacement()`

当前形状：

```llvm
%shifted = lshr %full, offset
%part = trunc %shifted to iREAD
```

这也属于内部 glue。应该替换成：

```llvm
%part = call iREAD @notdec.reg.extract.iFULL.iREAD(iFULL %full, i64 offset)
```

语义：从 `%full` 的 bit offset 处读取 `READ` 位。`offset = 0` 表示最低位。

## 4. SummarySSA 的 range 拼接

相关函数：

- `assembleRangeRead()`
- `assembleRangeReadIfDominating()`
- `readSlotRangeBefore()`

当前形状：

```llvm
%a.wide = zext/trunc %a to iOUT
%b.wide = zext/trunc %b to iOUT
%b.shift = shl %b.wide, offset
%out = or %a.wide, %b.shift
```

这个形状最容易污染后续分析。优先不要生成它，而是先识别完整传递。

只有 segment 来源真的不同，必须拼接时，使用 helper：

```llvm
%tmp0 = call iOUT @notdec.reg.insert.iOUT.iA(iOUT zeroinitializer, iA %a, i64 offsetA)
%out  = call iOUT @notdec.reg.insert.iOUT.iB(iOUT %tmp0, iB %b, i64 offsetB)
```

这里用 zero 作为 compose 的起点只允许在所有输出 bit 都被 segment 完整覆盖时使用。
如果不是完整覆盖，不能用 zero 补缺口，必须生成 explicit unknown 或 warning。

## 5. call argument / return slot cast

相关函数：

- `readSlotRangeBefore()`
- `castSlotValueToRegister()`
- `extractReturnRange()`

这里有两类操作：

- 纯 ABI 类型调整，例如 integer slot 的 `zext/trunc`。
- 寄存器 bit range 抽取、插入，例如 float slot 放到 ZMM 低 lane。

重构必须覆盖所有“寄存器 bit range 抽取/插入”。普通 ABI 类型调整不属于 register glue，
只有在它确实只是函数签名类型适配时，才允许保留原生 `zext/trunc`。

# Helper / Intrinsic 设计

这里说的 intrinsic 是 NotDec 内部 helper function，不是 LLVM 官方 intrinsic。

建议先加两个 value-level helper：

```llvm
declare iREAD @notdec.reg.extract.iFULL.iREAD(iFULL %full, i64 %bit_offset)
declare iFULL @notdec.reg.insert.iFULL.iWRITE(iFULL %base, iWRITE %value, i64 %bit_offset)
```

命名和已有 helper 对称：

- `notdec.partial_read.*` / `notdec.partial_write.*`：面向 register global 指针，主要在 lifting 和 entry range read 里使用。
- `notdec.reg.extract.*` / `notdec.reg.insert.*`：面向 SSA value，主要在 SummarySSA 内部使用。

基本规则：

- `bit_offset = 0` 表示最低位。
- offset 必须是常量。
- helper 必须被排除出普通外部函数签名推断。
- helper 必须被排除出 ABI call effect。
- helper 必须有明确的无副作用属性。
- helper 不应该残留在 bin2llvm 最终 native 输出里；最终 cleanup 要么消掉，要么 lower 成普通 LLVM bit ops，并在 warning 里说明。

不做 vararg `compose` helper。原因是 LLVM vararg call 会污染签名推断和 call effect，
offset/value 成对参数也更容易出错。compose 统一用 `insert` 链表达，这样每一步都有明确
类型和 offset，也更容易验证每一段覆盖了哪些 bit。

# 是否能做到“不生成任何 IR 原生位操作”

结论：在 register glue 这条内部路径上可以基本做到；在整个 native IR 上不能，也不应该做到。

可以做到的范围：

- partial write 合回完整值：用 `notdec.reg.insert`。
- 从 covered full value 抽 segment：用 `notdec.reg.extract`。
- 多 segment 拼 full value：优先完整传递；必要时用 `notdec.reg.insert` 链。
- entry range、return range、call slot range 的寄存器 bit 抽取：用 `notdec.reg.extract`。

不应该做到的范围：

- pcode 真实 `INT_SHL`、`INT_OR`、`PIECE`、`SUBPIECE` 等程序语义。
- 普通用户代码里的 bit operation。
- ABI 层纯类型适配的简单 `zext/trunc`，只要它不是寄存器 bit range glue。

所以判断标准应该是“SummarySSA 自己命名生成的 `.part_shift/.part_or/.partial_keep`
等 glue 指令消失”，不是 IR 全局没有 `shl/or`。

# 完整传递识别

完整传递识别应该放在拼接之前，是主路径。

## 1. 单 block / 普通 access

`readAccessRange(global, offset, width)` 读取多个 canonical segment 时，先检查：

- access 覆盖的 canonical segments 完整连续。
- 每个 segment 的 current def 都是 `RangedSSAValue`。
- 这些 `RangedSSAValue` resolve 后来自同一个 `Value`。
- 同一个 `CoveredRange` 覆盖当前 access range。
- 这个 covered value 在 use 点支配当前 use。

满足时直接返回完整 covered value，或用 `notdec.reg.extract` 从 covered value 抽出当前
access range。

不满足时，才读 segment 并进入 compose 后备路径。

## 2. PHI 合流处

如果 merge block 里读 full range，不能立刻把每个 segment 的 PHI 拼起来。先尝试识别：

- 所有 segment PHI 在同一个 merge block。
- incoming edge 数和 predecessor edge 数一致。
- 对每条 incoming edge，这些 segment 都来自同一个 full covered value。
- 每条 edge 上的 covered value 都能在 predecessor terminator 前使用。

满足时，可以直接建立 full-width materialization PHI：

```llvm
%full = phi i64 [ %a.full, %pred1 ], [ %b.full, %pred2 ]
```

这个 PHI 不是新的 Braun variable。Braun variable 仍然是 canonical segment。它只是 full
access 的 materialized value，并且可以记录为覆盖当前 access range。

不满足时，保留 segment PHI，再用 helper compose。

## 3. partial write 后的完整值

如果有：

```text
old full value = %old
write low 16 = %x
read full 64
```

不要把低 16 和高 48 分开拼成 full。优先生成：

```llvm
%new = call i64 @notdec.reg.insert.i64.i16(i64 %old, i16 %x, i64 0)
```

然后把 `%new` 作为 covered full value 写回所有 canonical segments。

这样后续 full read 可以继续完整传递 `%new`。

# 迁移路线

## 阶段 A：完整 helper 设计和保护

目标：

- 新增 `notdec.reg.extract.*` 和 `notdec.reg.insert.*` 的名字解析、声明创建、call 解析。
- 把它们排除出外部函数签名推断、ABI call effect、summary unknown call 统计。
- final cleanup 能识别这些 helper，残留时打印 warning。

判断标准：

- helper 不会被重命名成 `@1`。
- helper 不会生成 `summary_clobber` 或未知外部调用 warning。

## 阶段 B：完整收口 SummarySSA 内部 bit glue

目标：

- `replacePartialRegisterValue()` 改用 `notdec.reg.insert`。
- `extractPartialRegisterValue()` / `extractBitsFromInteger()` 改用 `notdec.reg.extract`。
- entry range / return range 里属于寄存器 bit 抽取的部分改用 helper。
- `assembleRangeRead*()` 的 fallback compose 改成 `insert` 链，不再生成 `zext/shl/or`。

判断标准：

- SummarySSA 自己生成的 `.part_shift/.part_or/.partial_keep/.partial_bits` 这类指令消失。
- 真实程序语义里的 `shl/or` 不受影响。
- `llvm-as` 和 `opt -passes=verify` 通过。

## 阶段 C：完整接入完整传递识别

目标：

- `RangedSSAValue` 严格记录 `Value + CoveredRange`。
- `readAccessRange()` 先尝试直接复用完整 covered value。
- PHI 合流处尝试 full-width materialization PHI。
- partial write 能在有完整 base 时生成新的完整 covered value。

判断标准：

- 原始 full-width register-to-memory store 不再因为 canonical segment 被拆成多个
  `partial_read + compose`。
- partial write 后的 full read 优先表现成 `notdec.reg.insert`，不是 segment 拼接。
- fortune 里之前类似 `%R8.full_range.part_wide / part_shift / part_or` 的形状消失。

## 阶段 D：统一验证、cleanup 和 lowering

目标：

- 完整重构完成后再跑 fortune、lighttpd-angel 和至少一个 Bench2 小二进制。
- helper 如果已经完全被 register elimination 消费，直接删除。
- helper 如果仍残留，warning 必须明确写出函数、helper、range，并判断它是否还能继续被消除。
- bin2llvm 最终 native 输出不能残留 `notdec.reg.extract/insert`；要么 lower 成普通 LLVM bit ops，要么明确阻断并报错。

判断标准：

- bin2llvm 最终 native 输出不含未声明语义的 `notdec.reg.extract/insert`。
- warning 文件能区分“真实寄存器残留”和“内部 helper 未清理”。
- 重构后再统一修复测试暴露的问题，不在每个小步骤后围绕单点样例反复补丁。

# 风险

- helper 会挡住 LLVM 普通优化，所以必须有 cleanup / lowering。
- 如果 compose 用 zero 作为起点但 coverage 检查有 bug，会重新制造虚假 0。必须只有完整覆盖时才能这么做。
- full-width materialization PHI 不能污染 Braun variable。segment PHI 仍是唯一 SSA variable。
- offset 不能只放 metadata，必须是 call 参数，否则语义不可靠。
- 真实程序里的 pcode bit operation 不能误替换成 register helper。

# 判断标准

必须满足：

- `llvm-as` 和 `opt -passes=verify` 通过。
- register glue 路径不再生成新的 `shl/or/and/lshr/trunc/zext` 组合。
- 真实程序语义里的 `shl/or` 仍然保留。
- helper 不参与外部函数签名推断和 ABI call effect。
- full access 能复用 covered full value。
- helper 残留有明确 warning，不能静默进入最终主链路。

不接受：

- register glue 路径继续生成新的原生 bit-op 组合。
- helper 静默进入 bin2llvm 最终 native 输出。
- full-width 访问在可证明完整传递时仍被拆成 segment compose。

# 不做什么

- 不把所有 LLVM `shl/or` 都替换成 helper。
- 不改真实 pcode operation 的 lowering 语义。
- 不做 vararg compose helper。
- 不把 helper 当普通外部函数交给签名推断。
- 不用 metadata 承载 offset 这种核心语义。

# 实现记录（已完成）

实现范围只在 `external/NotDec-bin2llvm` native 链路。

## 代码改动

- `include/notdec-bin2llvm/NativeRegisterValueRange.h:49`、`lib/NativeRegisterValueRange.cpp:64`
  新增 value-level register range helper 的识别、声明创建和 call 解析：
  `notdec.reg.extract.iFULL.iREAD(full, offset)`、`notdec.reg.insert.iFULL.iWRITE(base, value, offset)`。
- `lib/CMakeLists.txt` 把 `lib/NativeRegisterValueRange.cpp` 接入 core library。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:2161`、`:2190`
  把 SummarySSA 自己生成的 partial insert / extract 改成 helper call。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:3162`、`:3207`
  在 range read 拼接前先识别同一个 covered full value，能完整传递时直接复用原值。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:4090`
  让 `RangedSSAValue` 可以记录一个 SSA value 覆盖多个 canonical range。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:5429`、`:5455`
  entry / return / call slot 的寄存器 bit range 抽取、插入改用 helper。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:6008`
  在签名重写映射阶段折叠完整的 insert-of-extract 链，避免拆开后再拼回完整值。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:399`、`:741`、`:1882`
  排除 `notdec.reg.extract/insert`，避免它们参与未知外部函数签名推断和 ABI call effect。
- `lib/passes/summary/NativeRegisterSummarySSA.cpp:4696`
  修复 rewritten external call 后 ABI preserved / clobber 的处理，避免把 preserved register 当 Unknown。
- `lib/passes/summary/NativeRegisterFinalCleanup.cpp:88`、`:114`、`:148`
  final cleanup 会 lower `notdec.reg.extract/insert`，随后跑本地 DCE/ADCE 和 GlobalDCE，最终删除 helper declaration。
- `include/notdec-bin2llvm/passes/summary/NativeRegisterFinalCleanup.h:27`
  增加 `ValueRangeHelpersLowered` 计数。
- `lib/passes/summary/NativeRegisterSummary.cpp`
  stack-slot saved-register 模型只对 ABI unaffected 且非 stack pointer 的寄存器生效，避免 caller-saved 入参 spill 被误吞掉。
- `lib/passes/summary/NativeStackCanaryCleanup.cpp:854`、`:889`
  在删除 stack canary check 时，同步删除同一 saved slot 上由 `FS:0x28` 保存进去的 prologue store，避免 check 删除后留下 `FS_OFFSET` load。

## 测试改动

- `tests/native_register_summary_ssa_test.cpp:1529`
  覆盖 rewritten external call 后 preserved register 不应残留 register load/store。
- `tests/native_register_summary_ssa_test.cpp:4419`
  覆盖 caller-saved entry stack spill 必须算作 ReadEntry。
- `tests/native_register_summary_ssa_test.cpp:4717`
  覆盖真实 stack canary prologue save 删除。
- `tests/native_register_summary_ssa_test.cpp:6638`
  覆盖 value-range helper lowering 后死 partial read 能继续被 cleanup 删除。
- `tests/native_register_summary_ssa_test.cpp:6729`
  覆盖 final cleanup 不留下 `notdec.reg.extract/insert` declaration 或 call。

## 验证

已通过：

```bash
cmake --build build --target native_register_summary_ssa_test native_register_summary_test pcode_to_llvm_test notdec-native-llvm -j4
./build/bin/native_register_summary_ssa_test
./build/bin/native_register_summary_test
./build/bin/pcode_to_llvm_test
```

fortune 重新生成结果：

```text
/tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/fortune.native.ll
/tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/fortune.native.bc
/tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/register-ssa-warnings.tsv
```

验证命令：

```bash
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/fortune.native.ll -o /tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/fortune.native.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify /tmp/notdec-bin2llvm-fortune-value-range-20260714-final2/fortune.native.bc -disable-output
```

扫描结果：

- `notdec.reg.extract/insert` call / declare：0。
- `notdec.partial_read/partial_write`：0。
- `notdec.register.summary_return/summary_clobber/summary_ssa`：0。
- x64 register global 访问残留：0，包括 `FS_OFFSET`。

## 评分

- 实现效果：9/10。fortune 上 register helper、summary helper、寄存器全局访问都已清零。
- 复杂度：6/10。新增 helper 和 covered value 复用会增加阅读成本，但比继续匹配 `shl/or` 形状更可控。
- 维护成本：5/10。后续主要维护点是 helper parser/lowering 和 SummarySSA covered range 约束。

更简单的方案是只在 cleanup 里继续匹配 `shl/or` 形状，但它仍然会受 InstCombine 改写影响，不能解决计划里的核心问题。

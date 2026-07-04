# 原始 prompt

按这个写一个新的logs/规划。完成规划文件后，再启动一个类似的goal，更新整个链路支持这个intrinsic函数

# 背景

当前已经把 partial register write lift 成 `notdec.partial_write.iFULL.iWRITE(ptr @REG, value, i64 bit_offset)`，避免窄写时为了保留高位而先读完整寄存器。

新的残留问题出现在读侧。很多 x86 窄寄存器读在 IR 里仍表现成：

```llvm
call void @notdec.partial_write.i64.i32(ptr @R9, i32 %v, i64 0)
%R9D = load i64, ptr @R9
%low = and i64 %R9D, 4294967295
```

这里 `%R9D` 实际是读低 32 位，但 lifting 写成了 full-width `load i64 @R9`。SummarySSA 只能看到完整寄存器被读，于是会保守保留旧高位，导致 partial write、raw load、raw store 和 register global 残留。

# 目标

在 bin2llvm native 链路中引入 partial register read intrinsic，让窄寄存器读不再伪装成 full-width load。

建议形式：

```llvm
declare i32 @notdec.partial_read.i64.i32(ptr, i64)
declare i8  @notdec.partial_read.i64.i8(ptr, i64)
declare i64 @notdec.partial_read.i512.i64(ptr, i64)
```

语义：

```llvm
%v = call i32 @notdec.partial_read.i64.i32(ptr @RAX, i64 0)
```

表示从 full-width register global `@RAX` 的 bit offset 0 读取 32 位，返回 `i32`。`bit_offset = 0` 表示最低位；读取高 32 位时传 `32`。offset 语义和 `notdec.partial_write.*` 保持一致。

# 技术路线

范围先限制在 `external/NotDec-bin2llvm` 的 native 链路。

1. 新增 partial read helper 解析和创建接口。
   - 文件建议和 partial write 对称，例如 `NativeRegisterPartialRead.{h,cpp}`。
   - helper 名字使用 `notdec.partial_read.iFULL.iREAD`。
   - parser 返回目标 register global、返回值宽度、full width 和 bit offset。

2. lifting 阶段生成 partial read helper。
   - 在 `RegisterStorage` 或当前寄存器读路径中判断读取宽度是否小于 register global 宽度。
   - full-width read 继续生成普通 load。
   - partial read 生成 `call @notdec.partial_read.*(ptr @REG, i64 offset)`。
   - 不在 lifting 阶段额外生成 `load full -> trunc/and`。

3. Summary / SummarySSA 支持 partial read。
   - partial read 是 register read，不是普通外部 call。
   - bottom-up summary 遇到 partial read 时设置对应 register 的 `ReadEntry`，但后续最好按 bit mask 精化；第一版可以仍按 whole-register 保守。
   - SummarySSA 的 `readValueBefore()` 遇到 partial read 时，应该从当前 SSA register value 中抽取对应 bit range，返回窄整数。
   - 如果前面刚好有同 offset、同宽度的 partial write，可以直接得到写入值，不需要旧完整寄存器。
   - liveness 删除要把 partial read 当成 register use；final cleanup 也要把残留 partial read 计入 register residue。

4. 签名重写阶段保护 helper。
   - `notdec.partial_read.*` 和 `notdec.partial_write.*` 一样，不应该进入外部函数签名推断和 replacement function 流程。
   - 避免再次出现 helper 被重命名成 `@1` 的问题。

5. 回归测试和 fortune 验证。
   - pcode/lifting 测试：EAX/AL/AH 这类窄读应生成 partial read helper。
   - SummarySSA 测试：`partial_write -> partial_read` 可被消掉或折成直接值。
   - helper name 测试：partial read helper 不被签名重写改名。
   - fortune native 输出中重点看：
     - `call void @1(ptr ...)` 仍为 0。
     - `notdec.partial_read` 不应作为未知外部函数污染签名。
     - `partial_write -> load full -> and lowbits` 这类残留应明显下降。
     - `llvm-as` 和 `opt -passes=verify` 通过。

# 风险

- partial read 会让读写两侧更清楚，但当前 summary 仍是 whole-register 粒度，跨块和 Phi 场景仍可能保守。
- ZMM/XMM lane 的 partial read 需要特别注意 offset、返回类型和 ABI float slot 的关系，第一版可以先支持整数 IR 语义，不急着做更细的 float 类型恢复。
- 如果 partial read 残留到主 NotDec 中端，会成为新的外部 call 形状；final cleanup 和 warning 必须明确提示。
- AH 这类非 0 offset 的 8 位读写要有专门测试，避免 offset 语义反了。

# 判断标准

- partial register read 不再 lift 成 full-width `load @REG` 加 `and/trunc`。
- `notdec.partial_read.*` 不参与外部签名推断和签名重写。
- SummarySSA 能消费简单的 `partial_write -> partial_read`。
- fortune 当前关注 IR 里，原先由低位读造成的 raw register load/store 明显减少。
- 验证命令通过，运行时间和之前 fortune 同口径相比没有明显退化。

# 不做什么

- 不把普通内存 bitfield load 改成 partial read helper。
- 不在第一版里重写完整 bit-mask summary 框架。
- 不把 full-width register load 改成 helper。
- 不把 helper 暴露给主项目类型恢复当普通外部函数处理。

# 实现记录（已完成）

## 改动位置

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeRegisterPartialRead.h:26` 新增 `NativeRegisterPartialReadInfo`，说明 helper 用来保留窄读位段，避免伪造成整寄存器读。
- `external/NotDec-bin2llvm/lib/NativeRegisterPartialRead.cpp:33` 到 `:93` 新增 helper 名字判断、宽度解析、声明创建和 call 解析。
- `external/NotDec-bin2llvm/lib/CMakeLists.txt:7` 把 `NativeRegisterPartialRead.cpp` 加入 core。
- `external/NotDec-bin2llvm/lib/RegisterStorage.cpp:199` 的 `RegisterStorage::read()` 保留 full-width load；partial read 改成生成 `notdec.partial_read.iFULL.iREAD(ptr @REG, i64 bit_offset)`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:270` 的 `globalBitWidth()` 和 `addDemand()` 统一 demand mask 到 register global 位宽，避免 partial read 的 i32 demand 和 i64 register demand 混用触发 APInt 宽度断言。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:424`、`:1124`、`:1400` 把 partial read 排除出普通 call，并在 summary transfer / top-down demand 中按 register read 处理。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:825`、`:996`、`:1762` 把 partial read 排除出未知外部函数、可分析 call 和签名 shape。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2056` 新增 `readCoveredPartialWriteBefore()`：partial read 的位段完全被前面的 partial write 覆盖时，直接从写入值取位，不再为了旧高位放弃重写。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2628` 的 `rewritePartialReads()` 支持把 partial read 替换为 SSA 值；`:2685` 的 `removeDeadReplacedPartialReads()` 删除已替换 helper。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2976` 在 `readValueBefore()` 回溯时跳过 partial read，修复连续窄读互相阻塞的问题。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterFinalCleanup.cpp:58` 把残留 partial read 计入 register residue。

## 测试

- `external/NotDec-bin2llvm/tests/pcode_to_llvm_test.cpp:1389` 新增 lifting 测试，确认 EAX 读生成 `notdec.partial_read.i64.i32`，不再生成 raw `load @RAX`。
- `external/NotDec-bin2llvm/tests/native_register_summary_test.cpp:252` 新增 summary 测试，确认 partial read 产生低 32 位 entry demand。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3908`、`:3954`、`:4039`、`:4524` 覆盖 partial read 被 SummarySSA 消费、连续 partial read、签名重写不改名、final cleanup residue 统计。

验证命令：

```bash
cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4
./build/bin/pcode_to_llvm_test
./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test
/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/fortune.ll -o /tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/fortune.bc
/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify /tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/fortune.bc -o /dev/null
git -C external/NotDec-bin2llvm diff --check
```

## fortune 结果

输出路径：

- IR：`/tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/fortune.ll`
- summary：`/tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/summary.json`
- warning：`/tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/register-ssa-warnings.txt`

同口径命令耗时 `seconds=6.94 user=6.91 sys=0.03 maxrss=170664`。之前 partial helper name 修复后的 fortune 是 `seconds=7.10`，没有观察到性能退化。

关键计数：

- `call void @1(ptr ...)`：0
- `declare void @1(ptr ...)`：0
- `notdec.partial_read` call：60，declare：4
- `notdec.partial_write` call：34，declare：4
- `notdec.register.summary_return`：2
- `notdec.register.summary_clobber`：37
- `notdec.register.access`：100
- warning 行数：43
- raw register global load：10
- raw register global store：6

对比前一版 partial-write-only 结果：`notdec.partial_write` 从 79 降到 34，`summary_clobber` 从 40 降到 37，warning 从 46 降到 43；新增的 partial read 残留主要集中在未知入口/PHI 上的连续同寄存器读，以及 ZMM 分段搬运。

## 方案评分

- 实现效果：8/10。窄读不再 lift 成整寄存器 load，fortune 残留明显下降；仍有 60 个 partial read 残留，后续可继续针对同值读 CSE、ZMM lane 搬运优化。
- 复杂度：6/10。新增 helper 和 SummarySSA 支持是直接的，但 `readCoveredPartialWriteBefore()` 增加了一条局部位段规则，需要后续维护时注意 offset 语义。
- 维护成本：6/10。代码和 partial write 基本对称，测试覆盖了 lifting、summary、SSA、签名重写和 cleanup；后续如果做 bit-level register domain，可以把这条局部规则并入统一 demand/definition 模型。

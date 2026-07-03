# 原始 prompt

感觉partial write的相关的东西还是太别扭了。要不写一个新的规划文件，从这个角度设计：首先定义一套全局变量整数的 partial write的 intrinsic function（可能只需要一个函数？：notdec.partial_write.i64.i32(@RAX, %value32, %bit_offset)），然后在 lifting 的过程中，就直接对于这种 partial 寄存器访问，Lift成这种intrinsic方式的调用，而不是那种别扭的，先加载出寄存器之前的值，然后再和新的值拼起来再store回去的方式。让当前的所有的链路都支持这个intrinsic 

## 背景

bin2llvm native 当前把 partial register write 降成普通 LLVM 整数操作：先 `load @REG` 取旧寄存器值，再用 mask 保留未写 bit，把新值插进去，最后 `store @REG`。这个 IR 对底层语义是直观的，但对后续分析很别扭。

主要问题是：这个 `load @REG` 很多时候不是源码意义上的读寄存器，而只是为了拼接未写部分。后面的 register summary、SummarySSA、bit demand 需要再识别 keep-high pattern，避免把它误判成入口寄存器读取。现在已经有局部识别和 demand mask 补救，但问题仍然容易扩散到 `readEntry`、ZMM/XMM backing register、递归函数 summary 和函数签名重写。

更干净的方向是把 partial write 在 lifting 阶段就表达成一类明确语义：它是“写寄存器的一部分”，不是“读旧寄存器再写回”。旧值是否需要保留、哪些 bit 可能来自入口，交给专门的 lowering 或 summary 规则处理，而不是让普通 LLVM dataflow 自己猜。

## 目标

引入一个小的 native register partial-write intrinsic 表达整数全局寄存器的部分写入。初步形式可以是：

```llvm
call void @notdec.partial_write.i64.i32(ptr @RAX, i32 %value32, i64 %bit_offset)
```

含义是：把 `%value32` 写入 `@RAX` 指向的 64-bit 寄存器，从 `%bit_offset` 开始覆盖 32 bit；未覆盖 bit 保持原寄存器值。

目标不是先做复杂的通用 bitvector IR，而是先解决 native register elimination 链路里最痛的 partial write 表达问题：

- lifting 阶段不再为 partial register write 人工制造 keep-high `load/and/or/store`。
- register summary 能直接把 intrinsic 视为 partial write，而不是把它拆成一次读和一次写。
- SummarySSA 能根据当前 register SSA value 和写入 mask，把 intrinsic 降成新的 SSA 寄存器值。
- final cleanup / DCE 能在寄存器消除完成后清掉 unused intrinsic declaration。
- 输出给主 NotDec 中端前，不残留无法处理的 partial-write intrinsic；除非明确处于 debug / residue 模式。

## 技术路线

### 1. Intrinsic 形状

先只支持整数 backing register，避免一开始把 vector lane、flags alias、memory alias 都混进来。

建议使用一组按宽度命名的 declaration：

```llvm
declare void @notdec.partial_write.i64.i8(ptr, i8, i64)
declare void @notdec.partial_write.i64.i16(ptr, i16, i64)
declare void @notdec.partial_write.i64.i32(ptr, i32, i64)
declare void @notdec.partial_write.i128.i64(ptr, i64, i64)
```

名称里带 full width 和 write width，原因是 LLVM function type 本身不能表达 `ptr @RAX` 指向的寄存器宽度。后续 pass 只靠 callee name 和参数类型就能知道 mask 宽度，不需要回查太多 metadata。

参数建议固定为：

1. `ptr`：目标 register global。
2. `iN`：写入值，N 是 write width。
3. `i64`：bit offset。它按目标寄存器的最低位起算，`0` 表示从最低 bit 开始写；例如 64 位寄存器写高 32 位时，offset 传 `32`，覆盖范围是 `[32, 64)`。第一版要求 offset 是常量，且 `offset + write_width <= full_width`；非常量先 fallback 到旧 lowering 或报 warning。

返回值用 `void`，因为它是对 register global 的写入。SummarySSA 内部可以把它改写成新的 SSA register value，但 intrinsic 本身不需要返回新值。

### 2. Lifting 侧生成

在 RegisterStorage 或等价的寄存器写入封装里改 partial write lowering：

- full-width write 继续生成普通 `store`。
- partial write 生成 `notdec.partial_write.*`。
- 如果目标不是已知 native register global，或者 offset/width 不能静态确定，保守沿用旧 `load/mask/store`，并打印 warning。

这个改动要尽量靠近当前生成 keep-high 模式的位置。这样后续 ddisasm/Ghidra 元信息、register metadata、函数边界逻辑都不需要感知这件事。

### 3. Bottom-up register summary

summary 遇到 `notdec.partial_write.*(@REG, value, offset)` 时：

- 对被覆盖 bit：认为它们被函数内部写入，清掉这些 bit 的 entry 来源。
- 对未覆盖 bit：不因为 intrinsic 本身设置 `readEntry`。保持原来的来源状态。
- 对写入值 `%value`：照常分析它自己的 use-def。如果 `%value` 来自其他寄存器 load，那是正常的读证据。

如果 summary 仍然是 bool 粒度，第一版可以把 partial write 视为 write register，但不要视为 read register。更好的版本是同步推进到 mask 粒度：

```text
entryMask[REG] &= ~writeMask
nonEntryMask[REG] |= writeMask
readEntryMask[REG] 不因 intrinsic 自动变化
```

这正好和前面 bit demand 的思路对齐：partial write 不再伪造一个全寄存器读，`readEntry` 只来自真实读取。

### 4. SummarySSA 重写

SummarySSA 需要把 intrinsic 放进 register SSA 构造里消费掉。

当看到：

```llvm
call void @notdec.partial_write.i64.i32(ptr @RAX, i32 %v, i64 0)
```

如果当前 SSA register value 是 `%old_rax`，重写成：

```llvm
%v64 = zext i32 %v to i64
%new_rax = or (and %old_rax, KEEP_MASK), shl %v64, 0
```

然后把当前 `RAX` 的 SSA value 更新为 `%new_rax`，并删除 intrinsic。注意这里的 keep-high 拼接只存在于 SummarySSA 内部的 SSA value 上，不再通过 `load @RAX` 进入原始 register memory dataflow。

如果后续 bit demand 证明未写 bit 不被需要，可以继续用现有 zero-demand metadata 标记，把 `%old_rax & KEEP_MASK` 替换成 0。这样 debug 信息仍然能说明这个 0 是 demand pruning 引入的。

### 5. 其他链路兼容

需要让所有会看 register load/store 的 pass 认识这个 intrinsic，至少包括：

- native register summary：把它当 partial write。
- NativeRegisterSummarySSA：消费并删除它。
- NativeRegisterFinalCleanup：把残留 intrinsic 算作 register residue；没有 use 后让 GlobalDCE 清掉 declaration。
- prototype recovery / heritage SSA：如果这些 pass 在 SummarySSA 前后读取 register write pattern，需要么跳过 intrinsic，要么把它按 partial write 理解。
- IR verifier / asm 输出：确保 declaration 是普通 LLVM call，不依赖 LLVM 自定义 intrinsic 机制，避免和 LLVM intrinsic ID 冲突。

主 NotDec 中端原则上不应该看到这个 intrinsic。bin2llvm 输出给主项目前必须已经跑过 SummarySSA 或等价 lowering。如果用户选择 debug 输出 raw IR，则允许看到它。

## 判断标准

1. fortune raw lifted IR 中，整数 partial register write 不再出现为了保留旧值而生成的 `load @REG -> and -> or -> store @REG` 模式。
2. register summary 中，partial write 不会单独导致对应寄存器 `readEntry=true`。
3. fortune 的 summary-residue IR 不残留 `notdec.partial_write.*` call。
4. 现有 register elimination 后的 `llvm-as` 验证继续通过。
5. fortune 同口径运行时间不明显退化。

## 风险

- x86 的 partial register 语义有细节：例如写 `EAX` 会清零 `RAX` 高 32 bit，这不是普通 partial write，而是架构定义的 zero-extension full write。lifting 侧必须区分。
- flags 寄存器和 vector/XMM/ZMM lane 可以后续支持，但第一版不要急着复用同一个 intrinsic 覆盖所有情况。
- 如果某些 pass 依赖旧的 `load/and/or/store` 形状做 pattern match，需要同步改成识别 intrinsic，否则会出现阶段顺序相关 bug。
- 非常量 bit offset 暂时不建议支持。它会让 summary 和 SSA rewrite 都复杂很多，也不像常见寄存器 alias 写入。
- 如果 intrinsic 残留到主 NotDec 类型恢复，会变成新的未知外部调用，污染类型和控制数据流。因此 final cleanup 前必须有 residue 检查或 warning。

## 不做什么

- 不把普通内存 bitfield write 也改成这个 intrinsic。
- 不先设计完整的 vector lane intrinsic。
- 不把 LLVM IR 全面换成自定义寄存器 IR。
- 不为了这个方案重写现有 register summary 框架。

## 初步结论

这个方向比继续补 keep-high pattern 更清楚。它把 partial write 的语义放在 lifting 阶段显式表达，后续分析不需要从普通整数运算里反推“这是不是一次假读”。

第一版建议只覆盖整数 register global、常量 offset、常量 write width。这样能先把 RAX/EAX/AX/AL/AH 这类问题收住，再根据 fortune 和更多 Bench2 用例决定是否扩展到 XMM/ZMM lane。

## 实现记录（2026-07-03）

本次按计划先做整数寄存器 partial write，不扩展到 vector lane 或普通内存 bitfield。

### 代码改动

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeRegisterPartialWrite.h:21` 新增 `NativeRegisterPartialWriteInfo`，声明 partial-write helper 的命名、创建和解析接口。
- `external/NotDec-bin2llvm/lib/NativeRegisterPartialWrite.cpp:33` 实现 `notdec.partial_write.iFULL.iWRITE` 名字识别；`lib/NativeRegisterPartialWrite.cpp:60` 创建 helper declaration；`lib/NativeRegisterPartialWrite.cpp:74` 解析 call 的目标 global、写入值、宽度和 bit offset。
- `external/NotDec-bin2llvm/lib/CMakeLists.txt:7` 把 `NativeRegisterPartialWrite.cpp` 加入 core library。
- `external/NotDec-bin2llvm/lib/RegisterStorage.cpp:226` 保持 full-width register write 仍生成普通 `store`；`lib/RegisterStorage.cpp:235` 开始把 partial write 降成 `notdec.partial_write.*(ptr @REG, value, i64 offset)`，不再生成旧值 `load @REG`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:404` 让 bit-demand 把 partial helper 的 value 参数当作真实 use；`NativeRegisterSummary.cpp:613` 不把 partial helper 当普通 call；`NativeRegisterSummary.cpp:753` 对 partial write 只设置 `MayNonEntry`，不因为 helper 本身设置 `ReadEntry`；`NativeRegisterSummary.cpp:1099` 和 `NativeRegisterSummary.cpp:1365` 分别接入 bottom-up transfer 和 top-down demand。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1819` 调整顺序，先记录函数返回值，再做 partial-demand rewrite，避免返回值不是 LLVM use 时被提前删掉；`NativeRegisterSummarySSA.cpp:1952` 用当前 SSA old value 和写入 mask 合成 partial write 后的新寄存器值；`NativeRegisterSummarySSA.cpp:1990`、`NativeRegisterSummarySSA.cpp:2203`、`NativeRegisterSummarySSA.cpp:2553` 保留会被签名重写返回值使用的 value，避免悬空引用；`NativeRegisterSummarySSA.cpp:2412` 让 partial helper 参与 zero-demand rewrite；`NativeRegisterSummarySSA.cpp:2661` 按 liveness 删除 dead partial helper，仍 live 的 helper 保留为 residue；`NativeRegisterSummarySSA.cpp:2810` 在 `readValueBefore()` 中消费 partial helper。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterFinalCleanup.cpp:49` 把 partial helper 算作 register residue；`NativeRegisterFinalCleanup.cpp:81` 把它计入 remaining register access，避免 metadata 被误清。

### 回归测试

- `external/NotDec-bin2llvm/tests/pcode_to_llvm_test.cpp:174` 新增 EAX 写入 pcode；`tests/pcode_to_llvm_test.cpp:1330` 验证 partial register write 生成 `notdec.partial_write.i64.i32`，且不再生成旧值 `load @RAX` 或 `store @RAX`。
- `external/NotDec-bin2llvm/tests/native_register_summary_test.cpp:213` 验证 partial helper 本身不会让 RAX `ReadEntry=true`，但会保留 `MayEntry` 并设置 `MayNonEntry`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:241` 新增 helper call 检查；`tests/native_register_summary_ssa_test.cpp:3856` 验证 SummarySSA 会消费 helper 并替换后续 register load；`tests/native_register_summary_ssa_test.cpp:3896` 覆盖“返回入口 RAX + 另一条路径 partial write”的悬空值问题；`tests/native_register_summary_ssa_test.cpp:4292` 验证 final cleanup 会把残留 partial helper 当作 residue。

### 验证

构建和单测：

```bash
cmake --build ./build --target native_register_summary_ssa_test native_register_summary_test pcode_to_llvm_test notdec-native-llvm -j4
./build/bin/pcode_to_llvm_test
./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test
git diff --check
```

fortune：

```bash
./build/bin/notdec-native-llvm /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-partial-write-final-20260703174349/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-partial-write-final-20260703174349/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-partial-write-final-20260703174349/fortune.ll
```

结果：

- 运行时间：`seconds=7.30`。
- IR 路径：`/tmp/notdec-bin2llvm-fortune-partial-write-final-20260703174349/fortune.ll`。
- `llvm-as` 和 `opt -passes=verify` 通过。
- `notdec.partial_write` 残留：0。
- `summary_return` 残留：2。
- `summary_clobber` 残留：40。
- raw register load/store 残留：73。
- register global 残留：12。
- warning 文件：`/tmp/notdec-bin2llvm-fortune-partial-write-final-20260703174349/register-ssa-warnings.txt`，45 条。

同时补跑了：

```bash
./build/bin/notdec-native-llvm /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime --no-summary-register-residue-removal \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-noresidue-fix3-20260703174109/summary.json \
  -o /tmp/notdec-bin2llvm-fortune-noresidue-fix3-20260703174109/fortune.ll
```

该 no-residue-removal 输出也通过 `llvm-as` 和 `opt -passes=verify`。

### 评分

- 实现效果：8/10。partial write 的假读从 lifting 源头消掉，fortune 最终 IR 不残留 partial helper。剩余 register/summary helper 主要还是外部调用签名、clobber 和未完全消除的寄存器访问问题。
- 复杂度：6/10。新增 helper 解析代码很小，但 SummarySSA 需要处理返回值记录、bit-demand 和 liveness 的交互，理解成本有增加。
- 维护成本：6/10。方案比继续匹配 keep-high pattern 更清楚；后续如果做 mask 粒度 summary，可以沿用 helper 的解析和 transfer 入口。

更好的后续方案是把 summary 的 bool 粒度升级到 bit-mask 粒度，这样 partial write 的 liveness 和 demand 不需要继续按 whole-register 保守处理。

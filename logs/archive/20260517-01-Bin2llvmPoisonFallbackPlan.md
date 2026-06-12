# 2026-05-17 bin2llvm poison fallback 问题分析计划

## 用户原始 prompt

```text
当前的bin2llvm项目，遇到底层的寄存器varnode的时候，会直接生成poisoned value。比如最近遇到的那个logs/20260517-01-Bin2llvmPoisonFallbackPlan.md里面的RSP。为了语义的正确性，当前还是应该根据架构对寄存器的描述，为每个寄存器都生成一个全局变量，然后遇到这种情况就访问对应的全局变量。考虑按需创建，这样就仅会创建这些被使用到的寄存器。全局变量的名字还是用寄存器的常用名字，同时考虑通过meta data去增加额外的信息，比如他在p code里的具体的识别信息（寄存器内存块里面的偏移和大小等）。
后续也会单独实现非high function的底层pcode转LLVMIR的功能，所以当前实现的时候，就可以提前按照完全的纯寄存器底层PCode转IR的角度写代码。
寄存器内存段的拆分一定要按照这种方式：按照必定不可能被重叠访问的边界去切分，即不可能会有这样的访问范围，使得该边界在该访问内部。比如RAX可以被EAX，AX，AL等方式访问一部分，那么就按最大的RAX单独划分出来，作为全局变量。更小的访问就基于这个大的全局变量做切割再访问。(另外，访问EAX时会清空RAX高位这种语义已经被PCode考虑了，不需要再调研看是否考虑到)

另外，在agents.md里面说明一下以后这种logs下面的plan文档的写作方式：首先将所有的用户的原始prompt需求放在顶部。然后再写其他内容
```

```text
底层 PcodeToLLVM.cpp 当前缺 registerName的问题，能否通过架构定义，找到对应的sla文件和寄存器的定义，从而得到寄存器名字？
```

```text
按照这种方式写规划log然后实现吧
```

## 背景

`20260515-01-Bin2llvmModulePipelinePlan.md` 里已经用 Bench2 跑了两个模块级小样例：

1. `libuv module-limit5`：5 个函数 body 都生成，`llvm-as` 通过，没有 poison fallback。
2. `vsftpd module-limit5`：5 个函数 body 都生成，`llvm-as` 通过，但 `FUN_00106740` 有 95 条 poison fallback。

这说明模块链路已经能生成合法 IR，但 `vsftpd` 暴露了语义缺口。`llvm-as` 通过只是结构合法，不代表 IR 语义正确。

当前先不改代码，先把问题分清楚。目标不是把 warning 静音，而是确认哪些 warning 应该被真实语义替代，哪些只能暂时保留为明确的未知输入。

## 当前触发点

相关代码都在 `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`：

1. `read(...)`
   - 已有 `Values` 里找不到 varnode，且 varnode 不是 constant，就生成 `freeze poison`。
   - 这会打印：`read uninitialized varnode ...`
2. `readPhiIncoming(...)`
   - pending PHI 最后补 incoming 时，如果 incoming varnode 不在 `Values`，且不是 constant，就用 typed poison。
   - 这会打印：`PHI incoming varnode is unavailable: ...`
3. 其他 fallback
   - PHI incoming resize 失败、predecessor 没 terminator、`BRANCHIND` 无 successor、fallthrough 非 void return 也会用 poison。
   - 本次 `vsftpd` 没触发这些，先不作为主线。

当前 warning 是有价值的。它把以前静默的语义缺失暴露出来了，下一步不能删 warning，也不能简单把 poison 改成 0。

## `vsftpd` 具体场景

输入：

```text
/sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json
```

触发函数：

```text
FUN_00106740
blocks: 330
ops: 40913
varnodes: 67398
```

warning 分布：

```text
total: 95
read uninitialized varnode: 75
PHI incoming varnode is unavailable: 20
```

### 场景 1：入口 live-in 寄存器

例子：

```text
vn:65783 register RSP, isInput=true, def=null
vn:65791 register FS_OFFSET, isInput=true, def=null
```

使用方式：

1. `RSP` 被 `PTRSUB` 用来计算栈地址。
2. `FS_OFFSET` 被 `INT_ADD` 用来计算 TLS / stack canary 类地址。

这里不是普通“未初始化局部变量”。它们是函数入口时已经存在的机器状态。

当前 lowering 只把 Ghidra function params 映射进 `Values`，没有给这些入口 live-in register 建 LLVM 参数或专门 intrinsic，所以读到时只能 fallback。

初步判断：

1. `RSP` 应该接入栈模型。至少要作为函数入口 stack base，而不是 poison。
2. `FS_OFFSET` 应该作为 TLS base / special register 输入。第一版可以先建 external helper 或函数参数，但不能是随机 poison。

### 场景 2：Ghidra `INDIRECT` 读到 ram / global input

例子：

```text
vn:210623 ram:0012601c DAT_0012601c int, isInput=true, def=null
vn:210622 ram:00126020 DAT_00126020 undefined4, isInput=true, def=null
vn:210621 ram:00126bf8 DAT_00126bf8 char *, isInput=true, def=null
```

使用方式集中在：

```text
INDIRECT inputs [global_or_stack_input, effect_varnode] -> output
```

这些 `ram:*` varnode 是地址绑定的全局数据。它们不是当前函数内某个 op 的输出，也不是函数参数。

当前 `INDIRECT` 被当成 copy-like：

```text
output = read(input0)
```

如果 input0 是 `ram` 且没有 def，就会变成 `freeze poison`。这对全局变量语义不对。更合理的表示是：

1. 如果 varnode 是 `ram` address-tied input，按真实地址生成 load。
2. 如果后续导出 initialized memory map，再把初始值和 section 信息挂到 module 里。
3. `INDIRECT` 的第二个输入表示这个值受某个 side effect 影响，不能无脑等同普通 COPY；第一步至少要把“读全局地址”表达对。

### 场景 3：入口栈槽参与循环 PHI

例子：

```text
bb:1 in [bb:0, bb:2]
MULTIEQUAL inputs [vn:210521, vn:201948] -> vn:202391
```

其中：

```text
vn:210521 Stack[-0x3a1], isInput=true, def=null
```

同类 warning 有 18 条 stack input。它们都在 `bb:1` 的 PHI 上，第一条 incoming 来自入口路径 `bb:0`，第二条 incoming 来自循环回边 `bb:2`。

这类值不是 lowering 顺序问题。`finalizePendingPhis(...)` 已经等所有 block lowering 完成后再补 incoming。问题是入口路径上的 stack input 本来就没有 `Values`。

初步判断：

1. 如果 Ghidra 把栈槽标成 `isInput=true`，它表示函数入口时这个栈位置可能有值。
2. 对真正的 local stack slot，入口值可能是未定义；但如果它参与 PHI，随便用 poison 会影响后续循环语义。
3. 需要先确认这些 stack input 是 Ghidra 为大块局部/数组初始化建的保守输入，还是确实代表 caller-provided stack memory。

### 场景 4：callee-saved register 参与 PHI

例子：

```text
bb:307 in [bb:3, bb:306]
MULTIEQUAL inputs [vn:65784, vn:52733] -> vn:52909
vn:65784 register R15, isInput=true, def=null

MULTIEQUAL inputs [vn:65787, vn:52194] -> vn:52262
vn:65787 register R13, isInput=true, def=null
```

`R15` / `R13` 是入口 live-in register。Ghidra 名字里也能看到 `unaff_R15`、`unaff_R13`。

初步判断：

1. 这类值应按函数入口 live-in 建模。
2. 对 x86-64，callee-saved register 在函数里可能被用作保存跨块状态。如果入口值参与 PHI，poison 会把这条路径语义抹掉。
3. 第一版可以把这种 register input 统一变成内部 LLVM 参数或 `notdec_livein_<reg>` helper。后续再决定是否暴露到模块函数签名。

## 根因分类

当前 95 条 warning 不是一个问题，而是三类：

1. 入口机器状态没建模
   - `RSP`、`FS_OFFSET`、`R13`、`R15`。
   - 需要 live-in register 规则。
2. 地址绑定内存 input 没建模
   - `ram:*` 全局数据、`Stack[-...]` 栈槽。
   - 需要按地址空间分开处理：global memory、stack frame、未知 caller stack。
3. `INDIRECT` 语义太粗
   - 当前当 COPY 处理，能跑通小样例，但对真实项目会把 global / stack input 读成 poison。
   - 需要至少识别 `ram` / `stack` address-tied input，不要把它当普通 SSA 值。

不是主要根因：

1. 不是 PHI 插入位置问题。之前已经把 `MULTIEQUAL` 先 lower，PHI 在 block 顶部。
2. 不是 PHI 延迟补 incoming 的顺序问题。缺的是入口 input 的定义，不是后续 block 还没 lower。
3. 不是模块符号表问题。checker 里 direct call 都能解析到 external，没有 unknown call。

## 修复目标

第一阶段目标：减少 poison fallback，同时不伪造错误语义。

具体目标：

1. `libuv module-limit5` 继续无 warning。
2. `vsftpd module-limit5` 里 `RSP`、`FS_OFFSET`、`R13`、`R15` 不再走 poison fallback。
3. `ram:*` address-tied input 不再直接 poison，至少通过真实地址 load 表达。
4. `PHI incoming` 如果是入口 live-in register 或可建模 stack/global input，应补真实 incoming。
5. 剩余无法确认语义的 stack input，要保留 warning，并在日志里明确分类，不能静默降级。

## 技术路线

### 阶段 1：加诊断，不改语义

目标是把 warning 变成可统计、可定位的问题清单。

建议输出：

1. function name。
2. varnode id、space、address、size、register name、high variable、high type。
3. 使用它的 op mnemonic、op id、parent block。
4. 是否 `isInput`、是否 address-tied、是否有 def。

判断标准：

1. 能直接从 lower 日志看出是 live-in register、global ram，还是 stack input。
2. `vsftpd module-limit5` 的 95 条 warning 能自动分组。

### 阶段 2：入口 live-in register 建模

先处理 register input。

建议规则：

1. `RSP`：生成函数级 stack base。短期可以是 hidden live-in value；后续要和 stack/local 恢复接上。
2. `FS_OFFSET`：生成 TLS base input 或 helper，例如 `notdec_livein_FS_OFFSET`。
3. 其他入口 register：生成 live-in value，命名保留 register name。

这里有一个取舍：

1. 加到 LLVM 函数参数里最直观，但会改变导出函数 prototype。
2. 用 module-level external global/helper 更少改签名，但语义会更隐式。

第一版建议先用内部 helper 或 hidden alloca/global 形式，不急着改公开函数签名。等 Bench2 多项目稳定后再统一 ABI 表达。

判断标准：

1. `vsftpd module-limit5` 中 register 类 poison fallback 清零。
2. 生成 IR 仍通过 verifier 和 LLVM 22 `llvm-as`。

### 阶段 3：address-tied ram input 转真实地址 load

先只处理 `space == ram && isAddressTied == true && def == null`。

建议 lowering：

1. 取 varnode address，例如 `ram:0012601c`。
2. 转成整数地址。
3. 用现有 `memoryPointer(address)` 或后续 `inttoptr` 模型 load 对应 size。

这和 initialized memory map 是两件事：

1. 本阶段只保证“读全局地址”不是 poison。
2. 是否知道初始字节，留给 memory map 导出。

判断标准：

1. `vsftpd` 中 `DAT_0012601c` 这类 `ram` warning 消失。
2. IR 里能看到明确的 load，而不是 `freeze poison`。

### 阶段 4：stack input 分类处理

stack input 最容易做错，不能一上来全部替换成 0。

先分三类：

1. `RSP + offset` 形成的局部地址：应由 stack frame 模型负责。
2. PHI 初始 incoming 的 stack slot：先确认是否对应 loop 前已经 store 过的 local，还是 Ghidra 的入口保守输入。
3. caller stack / ABI 传参残留：如果确实来自 caller，应作为 live-in memory。

短期规则：

1. 能证明是 local stack slot 且入口未定义：可以保留 typed poison，但 warning 要说明是 local stack initial value。
2. 能从前面 STORE / INDIRECT effect 推出值来源：接真实 stack memory load。
3. 不能证明：保留 warning，不把它静默变成 0。

判断标准：

1. `PHI incoming varnode is unavailable` 从单一 warning 变成具体分类。
2. 不引入新的 verifier 失败。

### 阶段 5：重新检查 `INDIRECT`

当前 `INDIRECT` 走 copy-like。这个策略只适合一部分场景。

需要补充 Ghidra P-Code 语义判断：

1. `INDIRECT(input0, input1)` 中 input1 通常关联 call / store side effect。
2. 如果 input0 是地址绑定 memory varnode，输出应代表 side effect 后的该内存值。
3. 如果 input0 是 register / unique，copy-like 可能暂时可接受。

第一版不要尝试完整 memory SSA，只处理明显的 address-tied ram / stack。

判断标准：

1. `vsftpd` 的 global `INDIRECT` 不再靠 poison。
2. 不破坏已有 `libuv` 和旧单函数 smoke。

## 验证计划

最小验证：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-check notdec-heritage-module-llvm notdec-heritage-llvm -j4

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json \
  -o /tmp/vsftpd-module-limit5.ll 2>/tmp/vsftpd-module-limit5.lower.log

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/vsftpd-module-limit5.ll \
  -o /tmp/vsftpd-module-limit5.bc
```

回归验证：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json \
  -o /tmp/libuv-module-limit5.ll 2>/tmp/libuv-module-limit5.lower.log

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/libuv-module-limit5.ll \
  -o /tmp/libuv-module-limit5.bc
```

检查项：

1. `vsftpd` poison fallback 数量按分类下降。
2. `libuv` 继续 0 fallback。
3. `.ll` 都能通过 LLVM 22 `llvm-as`。
4. 新增 IR 不出现明显错误，比如把全局地址读成常量 0。

## 性能关注

这次仍只影响 `external/NotDec-bin2llvm`，不接 NotDec 主 pass pipeline，不需要跑 `fortune.o3.wasm.ll`。

Bench2 需要记录：

1. checker 时间。
2. lowering 时间。
3. `llvm-as` 时间。
4. fallback 分类数量。

如果只是增加诊断，预期性能影响很小。如果引入 memory map 或 stack frame 查询，需要重新看 `vsftpd module-limit5` lowering 时间，当前参考是 `real 11.01s`。

## 风险

1. 把所有 input varnode 都当 live-in 参数会让函数签名膨胀，真实项目会很难读。
2. 把 stack input 直接改成 0 会隐藏 bug，语义比 poison 更危险。
3. `ram` address-tied input 用 load 能改善语义，但没有 initialized memory map 时仍不知道初始内容。
4. `INDIRECT` 牵涉 Ghidra memory SSA，不能一次做全；先处理最明确的 global / stack 场景。
5. `RSP` 建模如果和后续 stack/local recovery 冲突，后面要再统一。

## 方案评分

实现效果预期：7/10。能先解决最明显的 live-in register 和 global input，不再把真实地址读成 poison。

复杂度预期：6/10。诊断很简单；live-in register 和 ram load 不难；stack input 和 `INDIRECT` 完整语义需要分阶段。

维护成本预期：6/10。按 varnode space 分类后边界更清楚，但后续要和模块 memory map、stack frame、ABI 表达统一。

更好的方案是从 Ghidra 导出更完整的函数入口 live-in、memory map 和 `INDIRECT` effect 信息，再 native lowering 按这些结构处理。当前先用已有 JSON 字段做最小修复，能更快验证 Bench2。

## 2026-05-18 实现记录

这次实现了寄存器 varnode 的全局变量建模。`ram` / `stack` input 仍按原分类保留 poison warning，后续单独处理。

### 已改文件

1. `AGENTS.md`
   - 第 57 行：补充 `logs/` 下 plan 文档必须先保留用户原始 prompt。
2. `external/NotDec-bin2llvm/include/notdec-bin2llvm/RegisterStorage.h`
   - 第 18 行 `RegisterInfo`：描述架构寄存器范围。
   - 第 25 行 `RegisterAccess`：描述一次 p-code 寄存器访问。
   - 第 36 行 `RegisterStorage`：按重叠寄存器范围合并成一个 backing global，并提供读写切片。
3. `external/NotDec-bin2llvm/lib/RegisterStorage.cpp`
   - 第 60 行 `RegisterStorage::RegisterStorage(...)`：把同一 space 内互相重叠的寄存器区间合并；相邻但不重叠的不合并。
   - 第 134 行 `globalFor(...)`：按需创建外部全局变量，名字优先用寄存器名，并挂 `notdec.register` metadata。
   - 第 164 行 `bitOffset(...)`：按访问相对 offset 计算切片 bit offset；当前支持 big-endian 参数。
   - 第 192 行 `read(...)`：从 backing global load，再按访问范围截取。
   - 第 213 行 `write(...)`：小寄存器写入时保留 backing global 其他位，只更新访问范围。
4. `external/NotDec-bin2llvm/include/notdec-bin2llvm/Pcode.h`
   - 第 44 行 `VarnodeView`：增加 `IsRegister` 和 `RegisterName`。
   - 第 62 行 `PcodeProgram`：增加完整 `Registers` 表和 `IsBigEndian`。
5. `external/NotDec-bin2llvm/tools/SleighBytes.cpp`
   - 第 91 行 `convertVarnode(...)`：用 `Sleigh::getExactRegisterName(...)` / `getRegisterName(...)` 给底层 varnode 补寄存器名。
   - 第 202 行 `collectRegisters(...)`：用 `Sleigh::getAllRegisters(...)` 导出完整寄存器表。
   - 第 343 行：记录架构 endian。
   - 第 344 行：收集完整寄存器表。
6. `external/NotDec-bin2llvm/lib/PcodeToLLVM.cpp`
   - 第 56 行 `PcodeLowerer::lower(...)`：为底层 P-Code lowering 创建 `RegisterStorage`。
   - 第 274 行 `read(...)`：未在 `Values` 中找到的 register varnode 改为从寄存器 global 读取。
   - 第 298 行 `write(...)`：register output 写回寄存器 global。
7. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 173 行 `registerInfosForHeritageProgram(...)`：从 Heritage JSON 里已有 register varnode 收集寄存器范围。
   - 第 354 行 `read(...)`：live-in register 不再走 poison fallback，改为 load 对应 global。
   - 第 380 行 `write(...)`：register output 同步写回对应 global。
   - 第 1155 行 `readPhiIncoming(...)`：PHI incoming 如果是 register varnode，就在 incoming block terminator 前 load。
   - 第 1537 行：`HeritageLowerer` 持有 `RegisterStorage`。
8. `external/NotDec-bin2llvm/lib/CMakeLists.txt`
   - 增加 `RegisterStorage.cpp` 到 `notdec-bin2llvm-core`。

### 验证

构建：

```bash
cmake -S external/NotDec-bin2llvm -B external/NotDec-bin2llvm/build -G Ninja \
  -DNOTDEC_BIN2LLVM_ENABLE_SLEIGH=ON \
  -DNOTDEC_BIN2LLVM_SLEIGH_SOURCE_DIR=/sn640/sleigh \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE
cmake --build external/NotDec-bin2llvm/build \
  --target notdec-heritage-module-llvm notdec-sleigh-llvm -j4
```

说明：这次配置时 `/sn640/sleigh` 的 CMake 仍触发了 Ghidra FetchContent，实际下载到了
`external/NotDec-bin2llvm/build/_deps/ghidrasource-src`。

Heritage module 验证：

```bash
external/NotDec-bin2llvm/build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json \
  -o /tmp/notdec-vsftpd-reg.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-vsftpd-reg.ll \
  -o /tmp/notdec-vsftpd-reg.bc
```

结果：

1. `llvm-as` 通过。
2. `vsftpd module-limit5` poison fallback 从 95 条降到 91 条。
3. `RSP`、`FS_OFFSET`、`R13`、`R15` 已生成全局变量，例如 `@RSP = external global i64, !notdec.register ...`。
4. 剩余 91 条是 `ram` / `stack` input，不属于本次寄存器建模。

干净用例：

```bash
external/NotDec-bin2llvm/build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json \
  -o /tmp/notdec-libuv-reg.ll
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-libuv-reg.ll \
  -o /tmp/notdec-libuv-reg.bc
```

结果：`llvm-as` 通过，没有新增 poison fallback。

底层 Sleigh P-Code 验证：

```bash
external/NotDec-bin2llvm/build/bin/notdec-sleigh-llvm \
  /sn640/ghidra/Ghidra/Processors/x86/data/languages/x86-64.sla \
  4889c8 \
  -o /tmp/notdec-sleigh-reg.ll \
  -s /sn640/ghidra/Ghidra/Processors/x86/data/languages/x86-64.pspec
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-reg.ll \
  -o /tmp/notdec-sleigh-reg.bc
```

结果：生成 `@RAX` 和 `@RCX`，`mov rax, rcx` 通过寄存器 global 表达。

```bash
external/NotDec-bin2llvm/build/bin/notdec-sleigh-llvm \
  /sn640/ghidra/Ghidra/Processors/x86/data/languages/x86-64.sla \
  89c8 \
  -o /tmp/notdec-sleigh-eax.ll \
  -s /sn640/ghidra/Ghidra/Processors/x86/data/languages/x86-64.pspec
llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-sleigh-eax.ll \
  -o /tmp/notdec-sleigh-eax.bc
```

结果：`EAX/ECX` 访问没有单独建 `@EAX/@ECX`，而是基于 `@RAX/@RCX` 做切片。

### 性能

这次只改 bin2llvm lowering，不涉及 NotDec 主 pass pipeline、类型恢复、结构体合并或 pointer analysis，所以没有跑 fortune 当前关注用例。

对 Bench2 小样例，`vsftpd module-limit5` 和 `libuv module-limit5` 都能正常生成并通过 `llvm-as`。新增逻辑按需创建寄存器 global，数量跟实际使用到的 backing register 有关，不会为未使用寄存器建全量 global。

### 评分

实现效果：8/10。寄存器 live-in 和底层 Sleigh register varnode 已不再退到 poison，底层路径能从架构定义拿寄存器名。

复杂度：4/10。新增了一个共享 `RegisterStorage`，但逻辑集中，两个 lowering 只接读写入口。

维护成本：4/10。后续如果要更精确处理特殊寄存器、flags 或多 piece register，需要继续扩展 `RegisterStorage`，但现在的入口是清晰的。

更好的方案：Heritage JSON 目前只有已出现的 register varnode，没有完整架构寄存器表。更完整的方案是让 Heritage module JSON 也导出架构 register table，这样 high function 路径和底层 Sleigh 路径可以完全同口径合并寄存器范围。

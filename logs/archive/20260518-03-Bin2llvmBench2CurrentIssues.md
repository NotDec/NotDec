# 2026-05-18 bin2llvm 真实项目转 IR 现有问题

## 用户原始 prompt

```text
logs/写一个新的文档，分析当前bin2llvm项目在那几个真实项目上转IR的现有问题，包括poison值生成的问题等
```

## 背景

本文只整理当前已有 Bench2 产物和近期日志，不把未验证的修复当成事实。

主要参考：

1. `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/results.tsv`
2. `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/*/*.check.log`
3. `/sn640/NotDec-Exp/Bench2/bin2llvm-ir/*/*.lower.log`
4. `logs/20260515-01-Bin2llvmModulePipelinePlan.md`
5. `logs/20260515-02-Bin2llvmControlPhiFixPlan.md`
6. `logs/20260517-01-Bin2llvmPoisonFallbackPlan.md`
7. `logs/20260518-01-Bin2llvmModulePcodeTextDump.md`

当前结论先分两层：

1. 单函数样例：多数项目已经能导出 JSON、lower 成 `.ll`，并通过 LLVM 22 `llvm-as`。
2. 模块级样例：`libuv module-limit5` 比较干净；`vsftpd module-limit5` 能生成合法 IR，但暴露大量 poison fallback。

`llvm-as` 通过只是说明 IR 结构合法，不说明语义已经对。

## 当前结果概览

`results.tsv` 中已有成功单函数样例：

1. `vsftpd`: `vsf_sysutil_sockaddr_set_ipv4addr.cold`
2. `libuv`: `uv__cancelled`
3. `memcached`: `drive_machine.lto_priv.0.cold`
4. `lighttpd`: `main_init_once`
5. `tmux`: `cmd_display_menu_args_parse.lto_priv.0.cold`
6. `openssh`: `window_change_handler.lto_priv.0`
7. `wolfssl`: `wc_PKCS7_DecodeEncryptedData.cold`
8. `redis`: `dictSdsKeyCompare.cold`
9. `libicu`: `_ZN6icu_7412LocalPointerINS_9UVector32EED1Ev`
10. `python`: `_PyPegen_fill_token.cold`
11. `wrk`: `parse_url_char.cold`
12. `ffmpeg`: `fg_create.cold`

已知失败：

1. `vim/lalloc.cold`
   - 一次是 `ghidra_failed_124`。
   - 一次是 `ghidra_autoanalysis_timeout_runtime_6min`。
   - 日志里还能看到不少 `R_X86_64_COPY` relocation warning。这不一定是直接失败原因，但说明 Ghidra 对这个 ELF 的装载状态不干净。
2. `php/zm_info_date`
   - checker 通过。
   - lowering 后 module verifier 失败。
   - 错误包括 `Instruction does not dominate all uses!` 和 `PHI nodes not grouped at top of basic block!`。
3. 旧的 `vsftpd/main.json` 单函数 lowering
   - `CBRANCH target block is unknown: ram:001077ac`。
   - 后续模块级 `vsftpd module-limit5` 已能避开这个问题，说明 CBRANCH 兜底修复对部分场景有效，但还需要更大样本确认。

## 模块级现状

### `libuv module-limit5`

现象：

1. Ghidra 导出 5 个函数，成功 5 个，失败 0 个。
2. checker 通过。
3. lowering 输出 5 个函数 body，失败 0 个。
4. direct call 4 个，unknown call 0 个。
5. lower log 没有 poison fallback warning。
6. `.ll` 能通过 LLVM 22 `llvm-as`。

判断：

1. 模块 schema、函数声明、external declaration、direct call 解析这条链路基本能用。
2. 它不能代表真实项目问题已经解决，只能作为干净回归样例。

### `vsftpd module-limit5`

现象：

1. Ghidra 导出 5 个函数，成功 5 个，失败 0 个。
2. checker 通过。
3. direct call 240 个，unknown call 0 个。
4. lowering 输出 5 个函数 body，失败 0 个。
5. `.ll` 能通过 LLVM 22 `llvm-as`。
6. lower log 里有 95 条 poison fallback warning，集中在 `FUN_00106740`：
   - `read uninitialized varnode`: 75 条
   - `PHI incoming varnode is unavailable`: 20 条

判断：

1. `vsftpd module-limit5` 是当前最有价值的小型真实问题集合。
2. 它不是 IR 合法性问题，而是语义缺口问题。
3. 这些 warning 不能静音，也不能直接把 poison 改成 0。

## Poison fallback 分类

当前 poison fallback 主要来自 `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`：

1. `read(...)`
   - `Values` 里找不到 varnode，且 varnode 不是 constant。
   - 当前会生成 `freeze poison`。
2. `readPhiIncoming(...)`
   - 延迟补 PHI incoming 时，incoming varnode 仍然不可用。
   - 当前会用 typed poison。
3. 少数结构性兜底
   - PHI incoming resize 失败、predecessor 没 terminator、`BRANCHIND` 无 successor、fallthrough 非 void return。
   - 这类不是当前 `vsftpd module-limit5` 的主因。

### 1. 入口机器状态没有建模

典型 varnode：

```text
vn:65783 register RSP, isInput=true, def=null
vn:65791 register FS_OFFSET, isInput=true, def=null
vn:65784 register R15, isInput=true, def=null
vn:65787 register R13, isInput=true, def=null
```

问题：

1. 这些不是普通未初始化局部变量。
2. 它们是函数入口时已经存在的机器状态。
3. `RSP` 参与栈地址计算。
4. `FS_OFFSET` 参与 TLS / stack canary 类地址计算。
5. `R13`、`R15` 这类 callee-saved register 可能作为跨块状态参与 PHI。

正确方向：

1. register live-in 应该建成明确的输入或全局机器状态。
2. `RSP` 后续要和 stack model 对齐。
3. `FS_OFFSET` 后续要和 TLS / segment base 表达对齐。

近期日志 `20260517-01` 记录过寄存器 global 建模的实现和验证结果，但当前可复查的 Bench2 lower log 仍有 95 条旧 warning。后续需要用当前源码重新构建并重跑，确认 register 类 warning 是否真正清零。

### 2. 地址绑定 `ram` input 没建模

典型 varnode：

```text
vn:210623 ram:0012601c DAT_0012601c int, isInput=true, def=null
vn:210622 ram:00126020 DAT_00126020 undefined4, isInput=true, def=null
vn:210621 ram:00126bf8 DAT_00126bf8 char *, isInput=true, def=null
```

问题：

1. 这些是地址绑定的全局数据。
2. 当前 lowering 把它们当成普通 SSA 值读取。
3. 没有 def 时就退到 `freeze poison`。
4. 这会丢掉“从真实地址读取全局内存”的语义。

正确方向：

1. 对 `space == ram && isInput == true && def == null` 的地址绑定 varnode，至少生成按真实地址的 load。
2. 初始化内容、section、relocation 可以后续做 memory map。
3. 第一阶段不要伪造值，先把地址访问表达出来。

### 3. stack input 参与 PHI

典型场景：

```text
bb:1 in [bb:0, bb:2]
MULTIEQUAL inputs [vn:210521, vn:201948] -> vn:202391
vn:210521 Stack[-0x3a1], isInput=true, def=null
```

问题：

1. 这不是 PHI lowering 顺序问题。
2. PHI 已经延迟到 block lowering 完成后补 incoming。
3. 缺的是入口路径上的 stack input 语义。
4. 如果直接用 poison，循环初值语义会被抹掉。

正确方向：

1. 先区分 local stack slot、caller stack input、Ghidra 保守入口值。
2. 能证明是 local 且确实未定义的，可以保留 typed poison，但 warning 要说清楚。
3. 能关联到真实 stack memory 的，要生成 load。
4. 不能直接改成 0。

### 4. `INDIRECT` 被当成 copy-like

当前很多 global / stack input 是经 `INDIRECT` 使用：

```text
INDIRECT inputs [global_or_stack_input, effect_varnode] -> output
```

问题：

1. 当前近似成 `output = read(input0)`。
2. 如果 `input0` 是 address-tied memory varnode，`read(input0)` 很容易退到 poison。
3. `INDIRECT` 的第二个输入通常代表 call/store 这类 side effect，不能长期当普通 COPY。

正确方向：

1. 短期先处理明显的 `ram` / `stack` address-tied input。
2. 长期需要理解 Ghidra memory SSA effect。
3. 不建议一次做完整 memory SSA，否则容易把边界搞大。

## PHI 和 verifier 问题

`php/zm_info_date` 说明当前仍有 PHI 结构问题：

```text
Instruction does not dominate all uses!
PHI nodes not grouped at top of basic block!
module verification failed
```

这和 `vsftpd module-limit5` 的 poison warning 不是同一层问题：

1. `php` 是 IR verifier 失败，输出不能当合法 IR。
2. `vsftpd module-limit5` 是 verifier 通过，但语义有 poison 缺口。

后续排查顺序建议：

1. 先用 `php/one-zm_info_date.json` 复现 verifier 错误。
2. 确认是 `MULTIEQUAL` 没提前 lower，还是非 PHI 指令插到了 PHI 前面。
3. 再看 dominance 错误是不是由提前创建 `freeze poison` 或 incoming cast 插入位置导致。

## Ghidra / Bench2 输入问题

`vim/lalloc.cold` 当前主要卡在 Ghidra 侧：

1. `ghidra_failed_124`
2. `ghidra_autoanalysis_timeout_runtime_6min`
3. 大量 `R_X86_64_COPY` relocation warning

判断：

1. 这不是 native lowering 里一两个 opcode 能解决的问题。
2. 需要单独记录 Ghidra analysis 时间、导入 warning、decompile timeout。
3. 对大目标应该先保留失败原因，不要把它和 IR lowering bug 混在一起。

## 当前优先级

建议按这个顺序处理：

1. `php/zm_info_date` 的 verifier 失败
   - 这是硬失败，必须先让 IR 合法。
2. `vsftpd module-limit5` 的 poison fallback 分类
   - 这是语义问题，当前最集中。
3. register live-in 当前实现状态复核
   - 重新构建当前源码，确认 `RSP`、`FS_OFFSET`、`R13`、`R15` 是否还 fallback。
4. `ram` address-tied input
   - 全局数据不能继续 poison。
5. stack input 和 `INDIRECT`
   - 先分类，再逐步建模。
6. `vim` Ghidra 失败
   - 另开日志处理 Ghidra 侧超时和 relocation。

## 判断标准

短期判断标准：

1. `libuv module-limit5` 继续 0 poison fallback，`llvm-as` 通过。
2. `vsftpd module-limit5` 仍能 5 个 body 全部 lower，`llvm-as` 通过。
3. `vsftpd module-limit5` 的 fallback 数量要按类别下降，不能只减少 warning 文本。
4. `php/zm_info_date` 能通过 verifier。
5. 每次降低 poison 数量时，要看 IR 里是否变成真实 load / register global / stack model，而不是常量 0。

## 性能关注

这篇文档只分析已有产物，不改代码，不影响 NotDec 主 pass pipeline。

后续只改 `external/NotDec-bin2llvm` 时，不需要跑 `fortune.o3.wasm.ll` 同口径性能；但需要记录 Bench2 小样例同口径时间：

1. Ghidra analysis / export 时间。
2. native checker 时间。
3. native lowering 时间。
4. LLVM 22 `llvm-as` 时间。
5. poison fallback 分类数量。

如果改到 NotDec 主链路、类型恢复、结构体合并、pointer analysis 或 pass pipeline，再按项目规范对比 `fortune.o3.wasm.ll`。

## 2026-05-18 实现记录：vsftpd poison fallback 降低

这次只改 `external/NotDec-bin2llvm`。目标是让 `vsftpd module-limit5` 里已经能明确建模的值不再退到 poison：

1. register live-in 继续走寄存器 global。
2. address-tied `ram` input 直接按地址 load。
3. address-tied `stack` input 用 `RSP + offset` 形成地址后 load。
4. 剩余真正无法建模的 fallback 改成更具体的 error 日志，带 varnode 和 op 信息。

### 已改文件

1. `external/NotDec-bin2llvm/include/notdec-bin2llvm/HeritagePcode.h`
   - 第 23 行 `HeritageVarnode::IsAddressTied`：保存 Ghidra JSON 里的 address-tied 标记。
2. `external/NotDec-bin2llvm/lib/HeritagePcode.cpp`
   - 第 246 行 `readVarnodes(...)`：读取 `isAddressTied`。
3. `external/NotDec-bin2llvm/lib/HeritageToLLVM.cpp`
   - 第 37 行 `printPoisonFallbackError(...)`：剩余 poison fallback 从宽泛 warning 改成 error 文本。
   - 第 277 行 `describeCurrentOp(...)` 和第 287 行 `describeVarnode(...)`：fallback 日志输出 varnode 的 space、address、offset、size、register/high 变量和当前 op/block。
   - 第 396 行 `read(...)`：register 仍先走 `RegisterStorage`；如果是 address-tied input，再尝试走内存 load；只有两者都失败才生成 poison。
   - 第 1071 行 `readStackPointer(...)`：从 `RSP` register global 读当前栈基址。
   - 第 1087 行 `addressForAddressTiedInput(...)`：`ram` 用绝对地址，`stack` 用 `RSP + offset`。
   - 第 1106 行 `readAddressTiedInput(...)`：对 address-tied input 生成 `load`。
   - 第 1134 行 `memoryPointer(...)`：把内存地址转成 `inttoptr`，不再走 1MiB `@notdec_ram` 假数组。
   - 第 1152 行 `lowerLoad(...)` 和第 1168 行 `lowerStore(...)`：普通 LOAD/STORE 也使用 `inttoptr` 地址。
   - 第 1241 行 `readPhiIncoming(...)`：PHI incoming 如果是 register 或 address-tied memory input，在 incoming block terminator 前生成对应 load。
   - 第 1593 行 `lowerBlock(...)`：记录当前 op，供 fallback 日志说明具体位置。

### 验证

构建：

```bash
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-llvm -j4
cmake --build /tmp/notdec-bin2llvm-build --target notdec-heritage-module-check notdec-heritage-llvm -j4
```

`vsftpd module-limit5`：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json \
  -o /tmp/notdec-vsftpd-poison-fix.ll \
  2>/tmp/notdec-vsftpd-poison-fix.lower.log

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-vsftpd-poison-fix.ll \
  -o /tmp/notdec-vsftpd-poison-fix.bc
```

结果：

1. `llvm-as` 通过。
2. lower log 中 poison fallback 从 95 条降到 0 条。
3. IR 中没有 `poison`，也没有 `@notdec_ram`。
4. 能看到 `@RSP`、`@FS_OFFSET`、`@R13`、`@R15` 等 register global。
5. 能看到 `vn:210623.mem = load i32, ptr inttoptr (i64 1204252 to ptr)` 这类 `ram` input load。
6. 能看到 `vn:210521.mem` 这类 `stack` input load，地址由 `RSP + offset` 生成。

`libuv module-limit5`：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json \
  -o /tmp/notdec-libuv-poison-fix.ll \
  2>/tmp/notdec-libuv-poison-fix.lower.log

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-libuv-poison-fix.ll \
  -o /tmp/notdec-libuv-poison-fix.bc
```

结果：`llvm-as` 通过，仍然没有 poison fallback。

checker：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/module-limit5.json

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-module-check \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/module-limit5.json
```

结果：两个 module JSON 都是 `status: ok`；`vsftpd` 的 240 个 direct call 都解析到 external；`libuv` 的 4 个 direct call 都解析到 external。

单函数回归：

```bash
/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/vsftpd/1-vsf_sysutil_sockaddr_set_ipv4addr.cold.json \
  -o /tmp/notdec-vsftpd-single-poison-fix.ll

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-vsftpd-single-poison-fix.ll \
  -o /tmp/notdec-vsftpd-single-poison-fix.bc

/tmp/notdec-bin2llvm-build/bin/notdec-heritage-llvm \
  /sn640/NotDec-Exp/Bench2/bin2llvm-ir/libuv/1-uv__cancelled.json \
  -o /tmp/notdec-libuv-single-poison-fix.ll

llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-libuv-single-poison-fix.ll \
  -o /tmp/notdec-libuv-single-poison-fix.bc
```

结果：两个单函数样例都通过。

### 性能

这次只改 bin2llvm，不接 NotDec 主 pass pipeline，不需要跑 `fortune.o3.wasm.ll`。

同口径小样例计时：

1. `vsftpd module-limit5` lowering：`real 11.22s`，`user 10.99s`，`sys 0.23s`。
2. `vsftpd module-limit5` `llvm-as`：`real 0.65s`，`user 0.63s`，`sys 0.02s`。
3. `libuv module-limit5` lowering：`real 0.02s`，`user 0.01s`，`sys 0.00s`。

对比 `20260517-01` 里记录的 `vsftpd module-limit5` lowering `real 11.01s`，这次变化很小。

### 仍需注意

1. `stack` input 现在按 `RSP + offset` load，比 poison 更接近底层语义，但还不是完整 stack/local recovery。
2. `ram` input 现在按绝对地址 `inttoptr` load，还没有接 ELF memory map、section 初始化和 relocation。
3. `INDIRECT` 仍不是完整 memory SSA；这次只是让其读到的 address-tied input 不再 poison。
4. `php/zm_info_date` 的 PHI/verifier 失败不在本次范围内。

实现效果：8/10。`vsftpd module-limit5` 当前 poison fallback 已清零，并且 IR 仍通过 `llvm-as`。

复杂度：5/10。改动集中在 varnode 读取和 lowering，新增规则少，但 stack/RAM 语义后续还要继续细化。

维护成本：5/10。address-tied input 的处理路径清楚；后续接 memory map 和 stack recovery 时需要替换这层简单 load。

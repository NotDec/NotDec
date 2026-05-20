# 2026-05-20 Bin2LLVM native relocation / PLT 计划

## 用户原始 prompt

```text
进一步推进计划该怎么做？
```

```text
写成一个计划，按照这样的思路：首先是介绍Ghidra相关的部分是怎么实现的，代码具体是哪些文件。其次就是按照复刻Ghidra的策略的思路去规划接下来怎么做。最好把这种写计划的思路给加到agents.md里面，但是，这个思路的应用范围仅局限于bin2llvm的native链路
```

## 背景

上一阶段已经完成 native discovery 第一版：

1. `NativeProgramState`
2. `NativeAnalysisManager`
3. `ElfLoadAnalyzer`
4. `ElfEntryAnalyzer`
5. `ElfSymbolAnalyzer`
6. `ReportAnalyzer`
7. `notdec-native-discover`

libuv 当前结果是：

1. `function seeds: 311`
2. `elf-symbol: 307`
3. `dt-init: 1`
4. `dt-fini: 1`
5. `dt-init-array: 1`
6. `dt-fini-array: 2`

下一步不急着做 eh_frame 或递归 decode。先做 relocation / PLT，原因很直接：

1. init/fini array、GOT、PLT 都依赖 relocation 后的指针。
2. 后续 direct call decode 看到 PLT stub 时，需要能映射到外部符号。
3. Ghidra loader 本身也是先处理 relocation，再创建入口和后续 analysis。

本阶段目标不是完整 relocation engine，而是先支持 libuv 当前 native discovery 和后续 direct-flow 需要的信息。

## Ghidra 相关实现

### 1. relocation table 的解析顺序

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/util/bin/format/elf/ElfHeader.java`

关键函数：

1. `parseRelocationTables(...)`
2. `parseDynamicRelocTable(...)`
3. `parseJMPRelocTable(...)`
4. `parseSectionBasedRelocationTable(...)`

Ghidra 的做法：

1. 先解析 `DT_REL`。
2. 再解析 `DT_RELA`。
3. 再按 `DT_PLTREL` 解析 `DT_JMPREL` / `DT_PLTRELSZ`。
4. 再处理 Android packed relocation、RELR。
5. 最后补 section header 里能看到的 relocation section。
6. 注释里明确提到顺序重要：GOT / PLT relocation 要晚一些处理。

native 要复刻的点：

1. 第一版先从 LIEF 的 relocation iterator 取结果，不自己重写 ELF relocation table parser。
2. 但 report 里要区分普通 dynamic relocation 和 PLT/GOT relocation。
3. 后续如果 LIEF 给的信息不够，再按 Ghidra 的 `DT_JMPREL` 路径补手写解析。

### 2. loader 应用 relocation 并写回 Program memory

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/ElfProgramBuilder.java`

关键函数：

1. `processRelocations(...)`
2. `processRelocationTable(...)`
3. `processRelocationTableEntries(...)`
4. `addArtificialRelocTableEntry(...)`

Ghidra 的做法：

1. 找到 relocation table。
2. 算 relocation address。
3. 找 symbol table 和 symbol name。
4. 调架构相关 handler。
5. handler 成功后直接改 Program memory。
6. 不管成功、失败、跳过，都会把结果写入 Program 的 relocation table。
7. 后续 `createDynamicEntryPoints(...)` 读 init/fini array 时，会根据 relocation table 和 memory 状态把元素当 pointer 处理。

native 要复刻的点：

1. `NativeProgramState` 增加 relocation 表，不静默丢 unsupported relocation。
2. `readPointer(...)` 先查 relocation 后的 pointer 值，再读原始 segment bytes。
3. 第一版只对 discovery 需要的 relocation 计算 pointer 值，不直接改 segment bytes。
4. report 里输出 applied / unsupported / skipped 数量。

### 3. x86-64 relocation 的具体语义

源码：

1. `/sn640/ghidra/Ghidra/Processors/x86/src/main/java/ghidra/app/util/bin/format/elf/relocation/X86_64_ElfRelocationType.java`
2. `/sn640/ghidra/Ghidra/Processors/x86/src/main/java/ghidra/app/util/bin/format/elf/relocation/X86_64_ElfRelocationHandler.java`
3. `/sn640/ghidra/Ghidra/Processors/x86/src/main/java/ghidra/app/util/bin/format/elf/relocation/X86_64_ElfRelocationContext.java`

关键逻辑：

1. `R_X86_64_RELATIVE` / `R_X86_64_RELATIVE64`
   - 值是 `imageBaseAdjustment + addend`。
2. `R_X86_64_IRELATIVE`
   - 也是 base + addend，但语义是 resolver 地址，第一版不要直接当函数入口。
3. `R_X86_64_GLOB_DAT` / `R_X86_64_JUMP_SLOT`
   - 值是 `symbolValue + addend`。
4. `R_X86_64_GOTPCRELX` / `R_X86_64_REX_GOTPCRELX`
   - Ghidra 会尝试把部分 indirect call/jmp relax 成 direct call/jmp。
5. `X86_64_ElfRelocationContext.requiresGotEntry(...)`
   - 一部分 GOTPCREL relocation 需要 GOT entry。

native 第一版要保守：

1. 只实现 `RELATIVE`、`GLOB_DAT`、`JUMP_SLOT`，必要时记录 `IRELATIVE`。
2. 不先做 GOTPCRELX 指令改写。
3. 不先支持 TLS、COPY、SIZE、PC-relative 等和入口发现无直接关系的类型。
4. unsupported relocation 必须进入 report。

### 4. dynamic entry 和 relocation 的关系

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/ElfProgramBuilder.java`

关键函数：

1. `processEntryPoints(...)`
2. `createDynamicEntryPoints(...)`

Ghidra 的做法：

1. `processRelocations(...)` 在 `processEntryPoints(...)` 之前。
2. `createDynamicEntryPoints(...)` 处理 `DT_INIT`、`DT_FINI`、`DT_INIT_ARRAY`、`DT_PREINIT_ARRAY`、`DT_FINI_ARRAY`。
3. array 元素如果有 relocation，就按 pointer 解释。
4. 最后创建 entry function。

native 当前已经有 `ElfEntryAnalyzer`，但它读 array pointer 时还是直接读原始 bytes。下一步要改成 relocation-aware。

### 5. OperandReferenceAnalyzer 和 PLT 的后续关系

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/OperandReferenceAnalyzer.java`

关键函数：

1. `checkForExternalJump(...)`
2. `getAddressTable(...)`
3. `createFlowTable(...)`
4. `checkForPointer(...)`
5. `createDisassemblyCommandsForAddress(...)`

Ghidra 的做法：

1. 看到已有 instruction reference 后，按 flow / pointer / table 分流。
2. 外部跳转会走 `checkForExternalJump(...)`。
3. 可能是代码地址时，会检查 executable、普通引用、subroutine 合法性。
4. 发现新 code 后再调 disassembly。

native 的关系：

1. 本阶段只建 PLT / external symbol 映射。
2. 后续 `FunctionDecodeAnalyzer` 遇到 direct call 到 PLT stub，才能记录 external call，而不是误把 PLT stub 当普通内部函数。

## 接下来怎么做

### 阶段 1：扩展 NativeProgramState

目标：

1. 记录 relocation entry。
2. 记录 relocation 后的 pointer 值。
3. 记录 PLT stub 到外部符号的映射。

建议数据：

```text
NativeRelocation:
  address
  type
  typeName
  symbolName
  symbolValue
  addend
  tableKind
  status
  computedValue

NativePltEntry:
  stubAddress
  gotAddress
  symbolName
```

判断：

1. `NativeProgramState::readPointer(address)` 先查 `RelocatedPointers[address]`。
2. 没有 relocation 值时，再走原始 bytes。
3. relocation 统计不要只存在 report 里，后续 decode 还要用。

### 阶段 2：实现 RelocationPltAnalyzer

目标：

1. 遍历 LIEF relocation。
2. 识别 x86-64 `RELATIVE`、`GLOB_DAT`、`JUMP_SLOT`、`IRELATIVE`。
3. 计算 discovery 需要的 pointer 值。
4. 记录 unsupported relocation type。
5. 输出 relocation report。

第一版规则：

1. `R_X86_64_RELATIVE`
   - 对 shared object，当前 image base 先按 0 处理，所以值是 `addend`。
   - 如果后续支持 non-zero image base，再加 base。
2. `R_X86_64_GLOB_DAT`
   - 如果 symbol 有 defined value，记为 `symbolValue + addend`。
   - 如果 symbol 是 imported，记录 external symbol，不伪造内部地址。
3. `R_X86_64_JUMP_SLOT`
   - 记录 GOT slot 到 external symbol。
   - 用 `.plt` / `.plt.sec` 的 stub 顺序建立 stub 到 symbol 的映射。
4. `R_X86_64_IRELATIVE`
   - 记录 resolver 地址，不自动升 function seed。

风险：

1. LIEF relocation type 枚举和 Ghidra type id 名称不一定完全一致，先以 x86-64 数字 id 做 fallback。
2. `.plt`、`.plt.got`、`.plt.sec` 的 stub 大小和布局要实测 libuv，不要写死到所有 ELF。
3. PLT stub 映射错会影响后续 call classification，所以 report 要列前若干项给人工检查。

### 阶段 3：改 ElfEntryAnalyzer 使用 relocation-aware pointer

目标：

1. init/fini array 读取走 `NativeProgramState::readPointer(...)`。
2. 如果 array 元素依赖 relocation，report 里能看出 relocation 被使用。
3. libuv 当前 seed 数不应下降。

验收：

1. `DT_INIT = 0x8000`。
2. `DT_FINI = 0x29240`。
3. `DT_INIT_ARRAY` 能读出 `0x9e30`。
4. `DT_FINI_ARRAY` 能读出 `0x9df0`、`0x9c70`。
5. relocation-aware 和原始 bytes 结果一致时，仍要记录来源，方便后续排错。

### 阶段 4：report 增强

目标：

1. 输出 relocation 总数。
2. 按 type 统计。
3. 按 status 统计。
4. 输出 PLT external symbol 数量。
5. 输出 unsupported relocation 类型和数量。
6. 输出前若干个 PLT stub 映射样例。

建议 report 结构：

```text
relocations:
  total:
  applied:
  unsupported:
  by type:
plt:
  external symbols:
  stubs:
```

### 阶段 5：为后续 decode 留接口，但不实现 decode

目标：

1. 提供 `lookupPltExternal(address)`。
2. 提供 `isPltStub(address)`。
3. 提供 `lookupRelocatedPointer(address)`。

不做：

1. 不在本阶段跑 libsla decode。
2. 不创建 direct-flow seed。
3. 不把 PLT stub 当内部函数 seed。

## 判断标准

短期验收：

1. `notdec-native-discover` 能在 libuv report 里输出 relocation 统计。
2. 能列出 PLT 外部符号。
3. init/fini array 读取改成 relocation-aware，seed 数不下降。
4. unsupported relocation 有统计，不静默忽略。

中期验收：

1. 后续 `FunctionDecodeAnalyzer` direct call 到 PLT 时，可以解析成 external call。
2. direct call 到内部 executable 地址时，仍能加 function seed。
3. relocation / PLT 数据能复用于 Bench2 其他 x86-64 shared object。

## 不做什么

1. 不做完整 Ghidra relocation engine。
2. 不支持所有架构。
3. 不做 GOTPCRELX 指令改写。
4. 不处理 TLS relocation。
5. 不处理 COPY relocation。
6. 不把 `IRELATIVE` resolver 自动当普通函数入口。
7. 不做线性扫描 `.text`。

## 性能和风险

性能：

1. relocation 扫描是按 relocation entry 数量线性处理。
2. 当前只在 `notdec-native-discover` 工具里运行，不接 NotDec 主 pass pipeline。
3. 不需要对 fortune 当前关注用例做同口径时间对比。

风险：

1. PLT stub 映射容易受 `.plt` / `.plt.sec` 布局影响。第一版要在 report 中展示样例，不要只给数量。
2. imported symbol 没有内部地址，不能硬塞成 executable seed。
3. 如果 LIEF 对 relocation table 分类不足，需要回到 Ghidra 的 `DT_JMPREL` 思路，自己读 dynamic table。
4. relocation-aware pointer read 如果覆盖原始 bytes，要能追踪来源，否则后面排错困难。

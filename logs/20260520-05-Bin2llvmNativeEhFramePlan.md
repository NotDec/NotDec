# 2026-05-20 Bin2LLVM native eh_frame 函数入口计划

## 用户原始 prompt

```text
接下来继续推进计划该怎么做？
```

```text
按这个方式写一个计划
```

## 背景

native discovery 现在已经完成两步：

1. symbol / init / fini 入口发现。
2. relocation / PLT 第一版。

libuv 当前基线：

1. `function seeds: 311`
2. `elf-symbol: 307`
3. `dt-init: 1`
4. `dt-fini: 1`
5. `dt-init-array: 1`
6. `dt-fini-array: 2`
7. relocation report 能看到 `X86_64_RELATIVE`、`GLOB_DAT`、`JUMP_SLOT`。
8. PLT external symbols 为 215。

下一步建议做 `.eh_frame`，先不要做递归 decode。

原因：

1. `.eh_frame` 里的 FDE 是高可信函数范围来源。
2. FDE start 可以补 symbol 之外的函数入口。
3. FDE range 可以给后续 `FunctionDecodeAnalyzer` 当边界，降低串函数风险。
4. libuv 有 `.eh_frame_hdr` 和 `.eh_frame`，适合做当前验证对象。

本阶段目标很窄：

1. 解析 FDE 的 `pc begin` 和 `pc range`。
2. FDE start 加入 high-confidence seed，source 为 `eh-frame`。
3. FDE range 写入 `NativeFunctionSeed`，后续 decode 使用。
4. 不处理 LSDA、landing pad、try/catch 注释、异常语义。

## Ghidra 相关实现

### 1. GccExceptionAnalyzer 调度 eh_frame 分析

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/GccExceptionAnalyzer.java`

关键函数：

1. `added(...)`
2. `handleStandardSections(...)`
3. `analyzeEhFrameHeaderSection(...)`
4. `handleDebugFrameSection(...)`
5. `processCallSiteRecord(...)`

Ghidra 的做法：

1. analyzer 被 AutoAnalysis 调度。
2. `handleStandardSections(...)` 先分析 `.eh_frame_hdr`。
3. 再分析 `.eh_frame`。
4. `.eh_frame` 返回 `RegionDescriptor` 列表。
5. 每个 `RegionDescriptor` 有 instruction address range。
6. 如果有 LSDA call site table，再处理 landing pad 和 try/catch 注释。

native 要复刻的点：

1. 也做独立 `EhFrameAnalyzer`。
2. 先处理 `.eh_frame_hdr` 和 `.eh_frame`。
3. 只保留 FDE start/range。
4. 不复刻 `processCallSiteRecord(...)`，第一版不处理 landing pad。

### 2. EhFrameHeaderSection 解析 `.eh_frame_hdr`

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/sections/EhFrameHeaderSection.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/ExceptionHandlerFrameHeader.java`
3. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/FdeTable.java`

关键函数：

1. `EhFrameHeaderSection.analyze(...)`
2. `EhFrameHeaderSection.analyzeSection(...)`
3. `processEncodedFramePointer(...)`
4. `getFdeTableCount(...)`
5. `createFdeTable(...)`
6. `FdeTable.create(...)`

Ghidra 的做法：

1. 找 `.eh_frame_hdr` memory block。
2. 读 header，拿到：
   - eh_frame pointer encoding
   - FDE count encoding
   - FDE table encoding
3. 解析 FDE count。
4. 解析 binary search table。
5. table 每项一般包含：
   - initial location
   - FDE record address
6. 对 initial location 建 code-flow-ish reference，对 FDE 地址建 data reference。

native 第一版的判断：

1. `.eh_frame_hdr` 可以先用来统计 FDE table 和做 sanity check。
2. 但函数 range 仍以 `.eh_frame` 里的 FDE 记录为准。
3. 如果 `.eh_frame_hdr` 解析复杂，可以先跳过 table，只直接扫 `.eh_frame`。
4. report 里要说明 `.eh_frame_hdr` 是否存在、是否解析、FDE count 是否匹配。

### 3. EhFrameSection 线性解析 `.eh_frame`

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/sections/EhFrameSection.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/sections/AbstractFrameSection.java`
3. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/Cie.java`
4. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/FrameDescriptionEntry.java`

关键函数：

1. `EhFrameSection.analyze(...)`
2. `EhFrameSection.analyzeSection(...)`
3. `getCie(...)`
4. `FrameDescriptionEntry.create(...)`
5. `FrameDescriptionEntry.createPcBegin(...)`
6. `FrameDescriptionEntry.createPcRange(...)`

Ghidra 的做法：

1. 从 `.eh_frame` 起始地址线性走。
2. 先读 CIE。
3. 再读紧跟的一个或多个 FDE。
4. FDE 使用 CIE 里的 FDE address encoding 解析 `pc begin`。
5. `pc range` 用地址大小读取。
6. 构造 `AddressRange(pcBegin, pcBegin + range - 1)`。
7. 对 `pcBegin` 执行 `CreateFunctionCmd`。
8. FDE 之后剩余部分可能是 augmentation data 或 call frame instructions。

native 要复刻的点：

1. 能识别 CIE 和 FDE。
2. 能从 CIE 读取 FDE encoding。
3. 能按 encoding 解出 `pc begin`。
4. 能读 `pc range`。
5. 对 `pc begin` 加 function seed。
6. range 写入 function seed。

native 第一版可以简化：

1. 只支持 libuv 当前 x86-64 ELF 会用到的 DWARF EH encoding。
2. 先支持常见的 `DW_EH_PE_pcrel | sdata4` 和 `DW_EH_PE_absptr`。
3. CIE augmentation 只读到 FDE encoding 所需字段。
4. call frame instructions 可以跳过。

### 4. FrameDescriptionEntry 创建函数和范围

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/FrameDescriptionEntry.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/RegionDescriptor.java`

关键函数：

1. `create(...)`
2. `createPcBegin(...)`
3. `createPcRange(...)`
4. `getRange(...)`
5. `RegionDescriptor.setIPRange(...)`

Ghidra 的做法：

1. `createPcBegin(...)` 通过 `cie.getFDEDecoder().decodeAddress(...)` 得到 start。
2. `createPcRange(...)` 得到 range。
3. `pcEndAddr = pcBeginAddr.add(intPcRange - 1)`。
4. `RegionDescriptor` 保存这个范围。
5. `CreateFunctionCmd(pcBeginAddr)` 创建函数。

native 要复刻的点：

1. `NativeFunctionSeed` 增加 range 字段。
2. eh_frame range 作为高可信 range 来源。
3. 如果同一地址已有 symbol seed，只补 source 和 range，不重复创建。
4. 如果 FDE start 不在 executable segment，记录 invalid，不加入 seed。

## 接下来怎么做

### 阶段 1：扩展函数 seed range

目标：

1. `NativeFunctionSeed` 增加函数范围。
2. symbol size 和 eh_frame range 都能写入。
3. report 能显示 range 来源统计。

建议字段：

```text
NativeFunctionSeed:
  Address
  Size
  RangeStart
  RangeEnd
  RangeSource
  PrimaryName
  Aliases
  Sources
  Confidence
```

规则：

1. symbol size 非 0 时，可以填 range source 为 `elf-symbol`。
2. eh_frame range 非 0 时，优先作为 decode 边界。
3. 如果 symbol size 和 eh_frame range 不一致，先记录 note，不强行修正。

### 阶段 2：新增 EhFrameAnalyzer

目标：

1. 在 `NativeAnalysisManager` 中注册 `EhFrameAnalyzer`。
2. 顺序放在 `ElfSymbolAnalyzer` 之后，`ReportAnalyzer` 之前。
3. 解析 `.eh_frame`，提取 FDE start/range。
4. FDE start 加 high-confidence seed，source 为 `eh-frame`。

建议第一版顺序：

```text
ElfLoadAnalyzer
RelocationPltAnalyzer
ElfEntryAnalyzer
ElfSymbolAnalyzer
EhFrameAnalyzer
ReportAnalyzer
```

### 阶段 3：实现最小 DWARF EH reader

目标：

1. 按 section bytes 读取 little-endian 数据。
2. 支持 32-bit record length。
3. 支持 CIE / FDE 区分。
4. 支持 ULEB128 / SLEB128。
5. 支持当前 libuv 需要的 pointer encoding。

第一版支持：

1. `DW_EH_PE_absptr`
2. `DW_EH_PE_pcrel`
3. `DW_EH_PE_udata4`
4. `DW_EH_PE_sdata4`
5. `DW_EH_PE_udata8`
6. `DW_EH_PE_sdata8`
7. `DW_EH_PE_omit`

不支持时：

1. 不崩。
2. 记录 unsupported encoding。
3. report 里输出数量和样例。

### 阶段 4：解析 CIE 中的 FDE encoding

目标：

1. 读取 CIE augmentation string。
2. 处理常见 `z...` augmentation。
3. 找到 `R` 字段，得到 FDE address encoding。

规则：

1. 没有 `R` 时，默认按 `DW_EH_PE_absptr`。
2. 有 `z` 时要先读 augmentation data length。
3. `L`、`P` 暂时只跳过对应字段，不处理 LSDA/personality 语义。
4. call frame instructions 全部跳过。

### 阶段 5：解析 FDE start/range

目标：

1. 读取 FDE 的 CIE pointer。
2. 找到对应 CIE。
3. 用 CIE 的 FDE encoding 解 `pc begin`。
4. 用同类 size 或地址大小读 `pc range`。
5. 写入 function seed。

过滤规则：

1. start 为 0：跳过。
2. range 为 0：记录为 invalid，暂不加入 range；是否加入 seed要谨慎，第一版建议不加。
3. start 不在 executable segment：跳过并记录。
4. start + range 溢出或跨出 executable segment：记录 note，range 先不作为 decode 边界。

### 阶段 6：report 增强

输出：

1. `.eh_frame_hdr` 是否存在。
2. `.eh_frame` 是否存在。
3. CIE 数量。
4. FDE 数量。
5. 成功解析的 FDE 数量。
6. 新增 seed 数量。
7. 与已有 seed 重叠数量。
8. invalid / unsupported 数量。
9. range source 统计。

libuv 目标：

1. `sources` 里出现 `eh-frame`。
2. seed 总数超过 311。
3. report 能看到一批 `eh-frame` range。

## 判断标准

短期验收：

1. `notdec-native-discover` 能跑 libuv，不崩。
2. report 出现 eh_frame 统计。
3. `eh-frame` source 数量大于 0。
4. seed 数超过 311。
5. symbol seed 不丢失。

中期验收：

1. FDE range 能作为后续 function decode 的边界。
2. range 冲突能在 report 里看见。
3. 对没有 `.eh_frame` 的 ELF 能正常跳过。

## 不做什么

1. 不做 LSDA 语义。
2. 不做 landing pad 入口发现。
3. 不创建 try/catch 注释。
4. 不做 `.debug_frame`，除非 libuv 当前验证需要。
5. 不做全量 DWARF CFI 解释。
6. 不把 unsupported encoding 猜成函数入口。
7. 不启动递归 decode。

## 风险

1. CIE / FDE pointer encoding 细节容易错，特别是 `pcrel` base 地址。
2. FDE range 和 symbol size 可能不一致，不能静默覆盖。
3. `.eh_frame_hdr` table 和 `.eh_frame` 扫描结果可能不完全一致，要 report 出来。
4. 解析异常时要跳过当前 record，不要让整个 discover 失败。
5. 如果第一版 DWARF reader 写太大，后续维护成本会高；只支持 libuv 当前用到的 encoding。

## 性能影响

1. `.eh_frame` 解析是按 section bytes 线性扫描。
2. 当前只接 `notdec-native-discover`，不接主 NotDec pass pipeline。
3. 不需要对 fortune 当前关注用例做同口径时间对比。

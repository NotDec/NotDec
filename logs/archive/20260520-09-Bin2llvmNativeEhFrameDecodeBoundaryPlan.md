# 2026-05-20 Bin2LLVM native eh_frame 解码边界计划

## 用户原始 prompt

```text
按照Agent.md里面的那个逻辑，先看看ghidra那边是怎么实现的，然后用复刻的思路写一个计划
```

## 当前目标和 native 状态

前两步已经完成：

1. native discovery 已经能解析 `.eh_frame`，并把 FDE `pc begin` 加入 function seed。
2. `.eh_frame` range 已经写入 `NativeFunctionSeed` 的半开区间 `[RangeStart, RangeEnd)`。
3. `.eh_frame_hdr` table 已经解析，并和 `.eh_frame` 扫描结果做交叉校验。

libuv 当前结果：

1. `function seeds: 484`
2. `eh-frame: 480`
3. `FDE: 480`
4. `parsed FDE: 480`
5. `hdr FDE count: 480`
6. `hdr table entries: 480`
7. `hdr matched starts: 480`
8. `hdr FDE address matches: 480`
9. invalid / unsupported 都是 0。

现在缺的是：这些 range 还没有进入 native lifting。

当前 native lifting 仍是手动模式：

1. `external/NotDec-bin2llvm/tools/notdec-native-pcode.cpp`
   - 需要 `-a <address> -l <length>`。
   - 直接调用 `collectSleighPcode(loadImage, specOptions, Address, Length, cerr)`。
2. `external/NotDec-bin2llvm/tools/notdec-native-llvm.cpp`
   - 同样需要 `-a <address> -l <length>`。
   - 也是按手动 length lift。
3. `external/NotDec-bin2llvm/lib/SleighLift.cpp`
   - `collectSleighPcode(...)` 从 `address` 开始线性调用 `engine.oneInstruction(...)`。
   - 循环条件是 `current < address + length`。
   - 目前不跟 CFG，也没有函数种子驱动。

所以本阶段目标不是“一步做完整函数反编译”，而是先把 `.eh_frame` range 变成安全的解码边界。

## Ghidra 相关实现

### 1. `.eh_frame` 解析和函数创建

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/structures/ehFrame/FrameDescriptionEntry.java`
2. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/RegionDescriptor.java`

关键函数：

1. `FrameDescriptionEntry.create(...)`
2. `FrameDescriptionEntry.createPcBegin(...)`
3. `FrameDescriptionEntry.createPcRange(...)`
4. `RegionDescriptor.setIPRange(...)`

Ghidra 做法：

1. 用 CIE 里的 FDE decoder 解出 `pc begin`。
2. 读取 `pc range`。
3. 构造 `pcBegin..pcBegin+range-1`。
4. 把这个范围保存到 `RegionDescriptor`。
5. 调用 `CreateFunctionCmd(pcBeginAddr)` 创建函数。

关键点：`CreateFunctionCmd(pcBeginAddr)` 没有传显式函数体范围。

这说明 Ghidra 的 `.eh_frame` 逻辑主要提供函数入口和 protected range，不是直接把 FDE range 当成完整函数 body。

### 2. GccExceptionAnalyzer 如何使用 range

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/plugin/exceptionhandlers/gcc/GccExceptionAnalyzer.java`

关键函数：

1. `handleStandardSections(...)`
2. `processCallSiteRecord(...)`
3. `disassembleIfNeeded(...)`

Ghidra 做法：

1. `EhFrameSection.analyze(...)` 返回 `List<RegionDescriptor>`。
2. `handleStandardSections(...)` 把每个 region range 加入 `ehProtected`。
3. LSDA call-site 处理也会把 call site 和 landing pad 加入 protected set。
4. `disassembleIfNeeded(...)` 对 LSDA 地址调用 `new DisassembleCommand(address, null, true)`。

关键点：这里没有把 `.eh_frame` range 作为 disassembler 的 restricted set 传进去。

### 3. 通用 disassembly 的 restricted set

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/cmd/disassemble/DisassembleCommand.java`

关键函数：

1. `DisassembleCommand(Address start, AddressSetView restrictedSet, boolean followFlow)`
2. `DisassembleCommand(AddressSetView startSet, AddressSetView restrictedSet, boolean followFlow)`
3. `doDisassembly(...)`
4. `doDisassemblySeeds(...)`

Ghidra 做法：

1. `DisassembleCommand` 可以接收 `restrictedSet`。
2. 最后调用 `disassembler.disassemble(seedSet, restrictedSet, initialContextValue, followFlow)`。

这给 native 复刻提供了边界模型：如果要限制解码范围，应该在 decode 边界层做，而不是把 range 当成函数语义。

### 4. 函数 body 发现

源码：

1. `/sn640/ghidra/Ghidra/Features/Base/src/main/java/ghidra/app/cmd/function/CreateFunctionCmd.java`
2. `/sn640/ghidra/Ghidra/Framework/SoftwareModeling/src/main/java/ghidra/program/model/block/FollowFlow.java`

关键函数：

1. `CreateFunctionCmd.createFunction(...)`
2. `CreateFunctionCmd.getFunctionBody(...)`
3. `FollowFlow.followCode(...)`
4. `FollowFlow.followInstruction(...)`

Ghidra 做法：

1. 如果 `CreateFunctionCmd` 没有显式 body，就调用 `getFunctionBody(...)`。
2. `getFunctionBody(...)` 用 `FollowFlow` 从入口跟控制流。
3. 默认不跟 `COMPUTED_CALL`、`CONDITIONAL_CALL`、`UNCONDITIONAL_CALL`、`INDIRECTION`。
4. `FollowFlow.followInstruction(...)` 会跟显式 flow 和 fallthrough。
5. 如果不允许流入已有函数，并且 fallthrough 到函数 symbol，就停止。

关键点：Ghidra 的函数体是 flow-derived，不是简单等于 FDE range。

## native 复刻思路

### 基本判断

不能把 `.eh_frame` range 直接当成函数 body。

更稳的复刻方式是：

1. `.eh_frame` 继续作为高可信 seed 和 decode 边界来源。
2. native lifting 先复刻 Ghidra 的 restricted set 思路。
3. 当前 `collectSleighPcode` 是线性 length 模型，所以第一版用 `[RangeStart, RangeEnd)` 换算出 length。
4. 后续再做 CFG-following decode，不在本阶段塞进去。

### 第一版规则

对 `RangeSource == "eh-frame"` 且 range 有效的 seed：

1. decode start 使用 `seed.Address`。
2. decode end 使用 `seed.RangeEnd`。
3. length 使用 `seed.RangeEnd - seed.Address`。
4. 如果 `seed.Address < RangeStart` 或 `seed.Address >= RangeEnd`，跳过并 report。
5. 如果 range 不在 executable segment 内，跳过并 report。
6. 如果没有 range，继续要求显式 `-l`，不猜长度。

这个规则等价于 native 当前能力下的 restricted set。

### 保守处理

1. FDE range 只限制解码，不声明这是完整函数体。
2. 不因为 range 结束就生成 return。
3. 不把 out-of-range branch 目标递归 lift。
4. 不把 landing pad 自动当成普通函数入口。
5. 不把 `.eh_frame_hdr` table 当成新的 range 来源，它只做校验。
6. symbol size 和 eh_frame range 冲突时，继续保留 note，不静默覆盖。

## 阶段计划

### 阶段 1：抽出 seed 到 decode request 的转换

目标：

1. 新增一个很小的 helper，把 `NativeFunctionSeed` 转成 native decode request。
2. decode request 至少包含 start、end、length、range source。
3. 第一版只服务工具层，不碰 p-code lowering。

建议位置：

1. 声明放在 `external/NotDec-bin2llvm/include/notdec-bin2llvm/NativeAnalysis.h` 或新的轻量头。
2. 实现先放在 `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp`，如果文件继续变大再拆。

判断：

1. 输入 eh-frame seed，能得到 `[Address, RangeEnd)`。
2. 输入无 range seed，返回空。
3. 输入非法 range，返回错误原因，report 能看见。

### 阶段 2：给 native pcode / llvm 工具加按 seed 解码入口

目标：

1. 保留现有 `-a/-l` 手动模式。
2. 新增按函数 seed 自动选择 length 的模式。
3. 优先支持 `--function <addr>` 或类似参数，从 discovery 结果里找 seed。

建议行为：

1. 如果用户传 `-a/-l`，完全走旧逻辑。
2. 如果用户传函数地址但没传 `-l`，先跑 native discovery。
3. 找到同地址 seed，且有 eh-frame range，就用 range 生成 length。
4. 找不到 range 就报错，让用户显式传 `-l`。

这样不会破坏当前 smoke test，也不会让工具突然猜长度。

### 阶段 3：在 Sleigh lifting 边界做 clamp

目标：

1. `collectSleighPcode(...)` 仍接收 address 和 length。
2. 调用侧保证 length 来自 range。
3. 先不改 `collectSleighPcode` 的内部循环。

原因：

1. 当前 lifting 是线性 oneInstruction。
2. 直接在这一层引入 CFG 会扩大改动面。
3. 当前最容易验证的是：同一个 seed 解码不会越过 FDE range。

### 阶段 4：报告和验证

目标：

1. 工具输出本次 decode 使用的 range source。
2. 如果因为 range 缺失或非法失败，错误信息要指出原因。
3. 加一个 libuv 函数做 smoke。

建议验证对象：

1. libuv 任意一个 `.eh_frame` seed。
2. 选一个短 range FDE，确认解码不会越过 `RangeEnd`。
3. 选一个已有 symbol seed，确认手动 `-a/-l` 仍可用。

## 判断标准

短期验收：

1. `notdec-native-pcode` 旧的 `-a/-l` 行为不变。
2. `notdec-native-llvm` 旧的 `-a/-l` 行为不变。
3. 新的按 seed 模式能对 libuv 的 eh-frame seed 自动选择 length。
4. 输出能说明本次 length 来自 `eh-frame` range。
5. 对缺 range 的地址不会猜，会要求显式 length。

中期验收：

1. native decode 不再需要人工为 eh-frame seed 填 length。
2. `.eh_frame` range 可以作为后续 CFG-following decode 的 restricted set。
3. 不因为 FDE range 误判函数体，避免后面和真实 CFG 冲突。

## 风险

1. 当前 `collectSleighPcode` 是线性解码，遇到内嵌数据仍可能解错；range 只能防止串到下个函数，不能解决 CFG 问题。
2. FDE range 可能覆盖 landing pad 或异常相关代码，不能简单当作普通函数 body。
3. 某些函数没有 `.eh_frame` range，必须保留手动 length 或后续 symbol-size fallback。
4. 如果工具自动跑 discovery，会增加启动成本；第一版只在用户请求 seed 模式时触发。
5. `NativeAnalysis.cpp` 已经偏大，如果再继续塞 decode request 逻辑，后续最好拆文件。

## 不做什么

1. 不做 CFG-following decode。
2. 不做 recursive descent disassembler。
3. 不做函数 body 恢复。
4. 不做 LSDA / landing pad 语义。
5. 不让 `.eh_frame_hdr` 覆盖 `.eh_frame` range。
6. 不改变现有 `-a/-l` 默认行为。
7. 不把没有 range 的 seed 自动 lift。

## 性能影响

1. 新模式只有在用户请求按 seed decode 时才跑 discovery。
2. `collectSleighPcode` 本身仍按 length 线性运行，复杂度不变。
3. 本阶段不接主 NotDec pass pipeline，不影响 fortune 当前关注用例。
4. 如果后续把 seed decode 接进批量 Bench2，需要再记录同口径运行时间。


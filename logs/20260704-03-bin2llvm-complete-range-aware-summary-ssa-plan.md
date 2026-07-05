# 原始 prompt

改进logs/20260704-02-bin2llvm-range-aware-summary-ssa-plan.md，规划一个完整SSA的实现方案出来到新的logs/文件。必须认真阅读external/NotDec-bin2llvm/docs/analysis/braun-ssa-construction.md并认真思考具体细节，对每一个具体细节都要思考是不是一定是对的。

# 背景

`logs/20260704-02-bin2llvm-range-aware-summary-ssa-plan.md` 已经把方向定成 range-aware SummarySSA：寄存器变量不再只有 `RAX`、`ZMM0` 这种 whole register，而是可以是 `RAX[0:32]`、`ZMM0[0:64]` 这样的 bit range。

当前代码已经有一部分 range 逻辑：

- `RegisterRangeKey` / `BlockRangeKey`
- 动态 range boundary planner
- partial read 的 range SSA
- range PHI
- partial write 的 range liveness
- call 参数和 return collection 的 ABI slot range 读取

但它还不是完整 SSA。现在仍有两套路径：

- whole-register path：`readValueBefore()` / `EntryValue` / `ExitValue` / `PendingPhi`
- range path：`readRangeBefore()` / `EntryRangeValue` / `ExitRangeValue` / `PendingRangePhi`

这会继续留下边界问题。partial write 还不是真正的 `writeRange()`；full load/store 还会回到 whole register；entry range input 仍可能先构造 whole entry value 再 extract；call effect 很多地方仍按 whole global kill/read。

`external/NotDec-bin2llvm/docs/analysis/braun-ssa-construction.md` 里最关键的一点是：算法里的 variable 必须有清楚的一等身份，`readVariable()` 和 `writeVariable()` 必须维护同一套 `currentDef`。如果 partial write 只是 matcher 或 liveness 特判，而不是写入这个 variable，后续读 range 时仍然要靠反向扫描补洞。这不是完整 SSA。

# 目标

把 `NativeRegisterSummarySSA` 重构成一套完整的 range SSA。

核心目标不是多清几个 helper，而是让寄存器状态的 SSA 变量统一变成 canonical segment：

```text
Variable = RegisterRangeKey(global, bit_offset, bit_width)
```

所有寄存器读写都走同一套接口：

```text
readSegmentBefore(segment, before)
readSegmentEntry(block, segment)
readSegmentExit(block, segment)
readAccessRange(global, offset, width, before)

writeSegment(segment, value)
writeAccessRange(global, offset, width, value)
```

最后应达到：

- partial read/write 是一等 SSA 操作，不依赖 whole-register fallback。
- full load/store 只是多个 segment 的拼接/拆分。
- PHI 类型是 segment 宽度，不为了低位访问生成 `i64` / `i512` PHI。
- entry input 按 segment materialize，只有需要完整 register 时才拼完整值。
- call return/clobber/read/preserve 按 ABI slot range 生效。
- range liveness 使用 `EntryDemandMask` / `ExitDemandMask`，不把 demanded low lane 扩成整个 register。
- 旧 whole-register SSA cache 最终删除。

# 总体设计

## 1. SSA 变量只使用 canonical segment

不能直接把任意访问范围当变量。访问范围会互相包含：

```text
AL       RAX[0:8]
AH       RAX[8:8]
EAX      RAX[0:32]
RAX      RAX[0:64]
XMM0_Qa  ZMM0[0:64]
```

如果同时把 `RAX[0:32]` 和 `RAX[0:8]` 当 SSA variable，会出现“一个写入定义另一个变量的一部分”的问题。这不是 Braun SSA 的简单 variable 语义。

所以必须先做 canonical segment planner：

1. 每个 register global 总是加入 `0` 和 full width。
2. partial read/write 加入访问边界。
3. full load/store 只加入 `0/fullWidth`。
4. ABI input/output/clobber/preserve 加入 ABI slot 边界。
5. summary return/clobber、signature param/return 加入对应 slot 边界。
6. `EntryDemandMask` / `ExitDemandMask` 的连续非零区间加入边界。

排序后相邻 boundary 形成 segment。Braun variable 只能是这些 segment。

这个设计是必要的。动态切分比按 8bit 全拆更稳：`ZMM0` 不会默认拆成 64 个 byte segment；但如果真实出现 `AL/AH`，也能自然切成 `[0:8]`、`[8:8]`、`[16:16]`、`[32:32]`。

## 2. `currentDef` 改成 range currentDef，并让 value 带 covered range

Braun 文档里的 `currentDef[var][block]` 在这里应对应：

```text
CurrentDef[(block, RegisterRangeKey)] = RangedSSAValue
```

不要只存裸 `llvm::Value *`。一个 segment 当前可能来自更大的 SSA value，例如：

```text
CurrentDef[(block, RAX[0:8])] = { %rax64, RAX[0:64] }
CurrentDef[(block, RAX[8:8])] = { %rax64, RAX[0:64] }
```

这表示多个小 segment 都可以从同一个 full-width value 里抽取。直接允许这种表示，比为每个小 segment 立刻生成 extract 更省，也更贴近“当前定义来自哪里”。

建议新增：

```cpp
struct RangedSSAValue {
  llvm::Value *Value;
  RegisterRangeKey CoveredRange;
};
```

规则：

- `CoveredRange` 必须显式存在，不能用空 range 表示“完整寄存器”。
- 完整寄存器值写成 `[0, fullWidth)`，例如 `RAX[0:64]`、`ZMM0[0:512]`。
- `CoveredRange` 必须覆盖 key 里的 segment。
- 如果 `CoveredRange == segment`，可以直接使用 `Value`。
- 如果 `CoveredRange` 更大，读 segment 时从 `Value` extract。
- 如果 `CoveredRange` 覆盖不了 segment，就是 planner 或 transfer bug，不能 fallback 到 whole-register path。

不用空 range 表示完整值，是为了避免语义混乱。`RAX` 的完整值是 `[0:64]`，`ZMM0` 的完整值是 `[0:512]`，ABI slot 的完整值又可能只是 `[0:64]`。显式 range 更容易检查，也更适合 warning。

但 PHI 和稳定的 segment 读取仍应 normalize 到 segment 宽度。也就是说：

- `readSegmentBefore(segment)` 对外返回 segment 宽度的整数值。
- 创建 PHI 时一律创建 segment 类型的 PHI，比如 `phi i8`、`phi i32`、`phi i64`。
- `RangedSSAValue` 可以在 cache 内表达“这个 segment 来自更大的 value”，但不能让 PHI 变成更大的 value 类型。
- access range 拼接仍只发生在 use 点，不写入 segment cache。

当前已有 `EntryRangeValue` / `ExitRangeValue` / `PendingRangePhi`，但缺少真正的 block-local write 状态，而且 value 还只是裸值。`readRangeBefore()` 仍然从 use 往前扫描 store/call/partial_write，这能工作，但不是完整 `writeVariable()` 模型。

完整方案建议把 builder 拆成两层：

```text
RangeSSAState:
  EntryDef[(block, range)] -> RangedSSAValue
  ExitDef[(block, range)]  -> RangedSSAValue
  LocalWrites[(instruction, range)] 或 BlockEvents
  PendingPhi[(block, range)]
  Resolving[(block, range)]
  EntryInputs[range] -> RangedSSAValue

RangeAccessRewriter:
  把 load/store/partial helper/call effect 转成 read/write range events
```

这个计划不能再允许 `readRangeBefore()` 长期靠反向扫描补定义。后续迁移的硬目标是：

- block 内按指令顺序预处理成 range events。
- `writeSegment()` 必须更新 `CurrentDef[(block, range)]`。
- `readSegmentBefore()` 只能读取当前 block 已知 def、递归 predecessor def，或者创建 range PHI。
- 不能因为当前 block 没扫到定义就退回 whole-register cache。

要点是：一旦某条指令定义了 segment，就必须进入同一套 `writeRange` / `currentDef` 语义。不能一部分定义写到 whole-register cache，另一部分定义只靠局部 matcher。

## 3. read/write 接口语义

规则：

- `readSegmentBefore` 返回 segment 宽度的整数值。
- `readAccessRange` 读取覆盖 access range 的所有 segment，并按 offset 从低到高拼接。
- `writeSegment` 写入 canonical segment，value 类型必须等于 segment 类型。
- `writeAccessRange` 把 value 拆成覆盖的 segments，分别定义。
- 如果 access range 无法被 planned segments 完整覆盖，不能偷偷退回 whole register；应直接报 warning 或使用明确 unknown。

fallback 到 whole register 会让完整 range SSA 退回旧问题：低位读写又可能拉入高位 entry input。

## 4. full load/store 也必须走 range

完整方案里不应该再有“full load 是特殊主路径”。full load 应该只是：

```text
readAccessRange(REG, 0, fullWidth)
```

full store 应该只是：

```text
writeAccessRange(REG, 0, fullWidth, value)
```

这样才能保证语义对称：

- 写 `EAX[0:32]` 后读 `RAX[0:64]`，只在读完整值时补高 32 位。
- 写 `RAX[0:64]` 后读 `AL[0:8]`，直接从低 8 位 segment 取值。
- 写 `XMM0_Qa[0:64]` 后读 `ZMM0[0:512]`，只有完整 ZMM 被真实观察时才拼出高位。

full load 如果真的被保留，可能生成多段拼接。这是正确成本。后续可以依赖 InstCombine，也可以本地合并相邻同源 segment，但不能因此保留 whole-register SSA 主路径。

# Braun SSA 细节

## 1. CFG 已完整，所以 block 可以视为 sealed

`NativeRegisterSummarySSA` 是在已有 LLVM CFG 上跑 pass，不是边生成 CFG 边建 SSA。因此不用实现外部可见的 `sealBlock()` 流程。

但 incomplete PHI 规则仍然必须保留：

- 多 predecessor 时，先创建 PHI。
- 立刻把 PHI 写入 `(block, range)` cache。
- 再递归读取 predecessor exit。
- 最终必须补齐 incoming。

当前 `PendingRangePhi` / `finalizePendingPhis()` 是对的方向。完整方案要把它变成唯一 PHI 机制，并删除 old `PendingPhi`。

## 2. 循环必须先注册 PHI

在 range SSA 中，循环里的 `x` 可能是 `RAX[0:32]`。如果 `readSegmentEntry(header, RAX[0:32])` 发现多个 predecessor，就必须：

1. 创建 `phi i32`。
2. 放进 `EntryDef[(header, RAX[0:32])]`。
3. 再去读 predecessor。

不能先递归再创建 PHI，否则 body 回读 header 时会无限递归。

重构时容易被“先 assemble 多个 segment”打乱。正确做法是 segment 级别先构建 SSA；拼接 access range 只是 use 点 materialization，不参与 PHI 递归。

## 3. trivial PHI 删除要更新所有 range cache

Braun 文档里容易漏的一点是：删除 PHI 后，所有 cache 不能再指向旧 PHI。

完整方案需要统一 `resolve(value)`，这些地方都必须用它：

- EntryDef
- ExitDef
- PendingPhi
- EntryInputs
- signature call args / returns 中暂存的 value

删除规则：

```text
phi(x, x)      -> x
phi(self, x)   -> x
phi(self only) -> unknown / frozen poison，不用 undef 静默掩盖
```

对 self-only PHI，不建议直接 `undef`。native register 消除里 unknown 更好 debug。可以继续使用当前 frozen poison 风格，并附 metadata，说明这是 range PHI 没有真实 incoming。

## 4. dominance 检查不能用来静默放弃 SSA

当前代码里有些地方会因为 value 不支配 use 而返回 nullptr，再 fallback。完整 range SSA 里这类 fallback 要收紧。

原则：

- PHI incoming value 只需要支配 predecessor terminator，不需要支配 PHI 本身。
- use 点拼接出来的 segment value 必须支配 use。
- 如果 call return/clobber materialize 的值不支配后续 use，说明插入点错了，应该修插入点或在合流处建 PHI，不应该退回 whole-register read。

因此重构时要把“支配失败”当成 bug/warning，而不是作为走旧路径的理由。

## 5. irreducible CFG 第一版只保证正确

Braun 基础算法在 irreducible CFG 上可能留下非 minimal PHI。第一版可以接受，只要：

- PHI incoming 完整。
- verifier 通过。
- trivial PHI 已删除。
- 没有 operandless PHI。

不建议第一版就做 SCC PHI cleanup。除非 fortune 或 Bench2 里出现明显 PHI 爆炸，再单独规划。

# 关键语义设计

## 1. partial write 不能再读旧 full value

当前 partial write helper：

```text
notdec.partial_write.iFULL.iWRITE(ptr @REG, value, bit_offset)
```

完整 range SSA 中，它应该只定义写入范围覆盖的 segments。

如果写入范围没有覆盖某个 segment，那么该 segment 没有被写，后续读取它时自然沿 CFG 往前找旧定义。

例如：

```text
write RAX[0:32] = v32
read  RAX[32:32]
```

高 32 位读取不应该看到低 32 位写，也不应该为了保留高位去读旧 `RAX[0:64]`。

如果写入范围只覆盖一个 canonical segment 的一部分，说明 planner 边界不完整。正确处理是 planner 提前切开，而不是 write 阶段临时生成非 canonical range。

## 2. partial read 只是 readAccessRange

partial read helper 应该消失在这一层：

```text
notdec.partial_read.i64.i8(@RAX, 8)
```

重写成：

```text
readAccessRange(RAX, 8, 8)
```

如果它跨多个 segment，就拼接。拼接只发生在 use 点，不把拼接结果写回 SSA cache。cache 只保存 segment。

## 3. ABI slot 是 range，不是 register name

ABI metadata 里经常写 `XMM0_Qa`，但 lifted IR 里真实 global 是 `ZMM0`。完整方案里要把 ABI slot 明确变成：

```text
NativeSignatureSlot {
  Unit = ZMM0,
  OffsetBits = 0,
  SizeBits = 64,
  Type = double
}
```

所有 call 语义都按 slot range 应用：

- input：读取 slot range。
- output：定义 slot range。
- clobber：定义 slot range 为 unknown。
- preserve：不写 slot range。

不要再用“`ZMM0` 这个 whole unit 是 float output”来判断 effect。否则低 64 位 ABI 会继续污染整个 `i512`。

## 4. unknown external call 要分清 read 和 clobber

对未知外部函数，不能为了保守就把所有 ABI input 都变成真实参数证据，也不能把所有 ABI output 都当 demanded return。

建议规则：

- 已推断出的参数数量内，ABI input slot 是 read。
- 已知 prototype 的 output slot 是 return。
- unknown prototype 的主要整数 return 可以按当前策略保留 `RAX`，但 `RDX` 这类大概率非返回寄存器不要默认当 return。
- float output 只有已知 prototype 或明确 callsite use 时才作为 return。
- killed-by-call slot 是 clobber，但只 kill 对应 range。

如果只能猜，就输出 warning，并把猜测限制到最小。

## 5. demand mask 要真正变成 range seed

当前 `EntryDemandMask` / `ExitDemandMask` 已存在，但很多地方仍会因为一个 bool 把整个 global 插入 live set。

完整方案里：

- `ExitDemandMask` 的非零 bit 区间转成 live ranges。
- `EntryDemandMask` 的非零 bit 区间决定需要 materialize 哪些 entry segments。
- `MayNonEntry` 只能说明这个 register 有内部新值，不代表所有 bit 都 demanded。
- `ReadEntry` 也要逐步从 bool 过渡为 mask；如果暂时没有 mask，只能作为保守 full mask，但要在 warning/metadata 里标出来。

这个点对递归 ZMM 问题很关键。只要 readEntry 还是 whole-register bool，就可能把低 lane 问题扩成整个 `i512`。

# 迁移方案

## 当前状态

前六个阶段已经把 fortune 这条链路跑干净了：partial helper、summary helper、raw register load/store 都没有残留。但这还不是完整 range SSA。当前代码仍有旧路径：

- `readValueBefore()` 仍维护 whole-register 读取逻辑。
- `PendingPhi` 仍和 `PendingRangePhi` 并存。
- `EntryInputs` 仍按 whole register 缓存入口值。
- `LocalRangeWrites` 只是 instruction-local cache，不是 block-level `currentDef`。
- `readRangeBefore()` 仍在 use 点附近反查 store、partial helper、call effect。

后续迁移目标不是继续补 matcher，而是把寄存器状态改成唯一的 range SSA 主路径。

## 第七阶段：建立 block-local range currentDef

目标：先把一条基本块内部的读写改成明确的 `currentDef` 更新。

工作：

- 新增 `RangedSSAValue { Value, CoveredRange }`。
- 新增 `CurrentDef[(block, RegisterRangeKey)] -> RangedSSAValue`。
- 在每个基本块内按指令顺序处理 range events。
- `writeAccessRange()` 拆出的每个 segment 必须写入 `CurrentDef`。
- `readSegmentBefore()` 在同一个 block 内只能读取前面已经写入的 `CurrentDef`。
- 删除 `LocalRangeWrites` 作为主要定义来源；最多临时保留为断言或调试辅助。

判断：

- full store 后 partial read 由 `CurrentDef` 命中，不靠重新扫描 store。
- partial write 后同 block 多次读取同一 segment，只复用同一个 segment def。
- 如果 access range 不能被 planned segments 覆盖，直接 warning 或失败，不退回 whole-register。

## 第八阶段：用 Braun-style predecessor 递归替换 range 反查

目标：跨 block 读取只通过 predecessor exit 和 range PHI。

工作：

- `readSegmentEntry(block, range)` 实现 Braun SSA 逻辑。
- 单 predecessor 直接读 predecessor exit。
- 多 predecessor 先创建 `phi`，写入 `CurrentDef[(block, range)]`，再补 incoming。
- PHI 类型必须等于 segment 宽度，例如 `i8`、`i32`、`i64`。
- `PendingRangePhi` 成为唯一 PHI 机制。
- 禁止 `readRangeBefore()` 从 use 点跨 block 反向扫描。

判断：

- 分支 merge 低位读只生成低位 PHI。
- 循环 header 先注册 PHI，不递归爆栈。
- call return range 跨 block 使用时通过 PHI，不直接使用不支配的 call 后 value。

## 第九阶段：full load/store 删除 whole-register 主路径

目标：full load/store 只是 range access，不再有单独的 whole-register SSA。

工作：

- full load 统一为 `readAccessRange(REG, 0, fullWidth)`。
- full store 统一为 `writeAccessRange(REG, 0, fullWidth, value)`。
- `readValueBefore()` 只保留为薄 wrapper，内部调用 `readAccessRange()`。
- 删除 `EntryValue`、`ExitValue`、whole-register `PendingPhi` 相关状态。
- 删除 `PendingPhi` 后，`finalizePendingPhis()` 只处理 range PHI。

判断：

- `NativeRegisterSummarySSA.cpp` 不再有独立 whole-register `PendingPhi`。
- 低 64 位 XMM/ZMM 使用不会生成 `phi i512`。
- 写低位后读完整寄存器时，只在 use 点拼接需要的 segments。

## 第十阶段：entry input 全面改成 range input

目标：入口值按 demanded segment materialize，不再先构造 full register。

工作：

- `entryRangeInput()` 直接创建 segment 参数或 frozen unknown。
- 删除 whole-register `EntryInputs` 缓存。
- 如果确实需要完整寄存器，使用多个 segment input 拼接。
- signature rewrite 只消费 range input，不再依赖最近 full store 或 full entry load。

判断：

- 低位参数函数不再有 entry `load i64 @RDI`。
- `ZMM0[0:64]` 参数/返回不暴露 `i512`。
- recursive pass-through ZMM 没有真实 observer 时，不创建 ZMM entry input。

## 第十一阶段：call effect 只走 range event

目标：call 的 read、return、clobber、preserve 都不再按 whole register 生效。

工作：

- `callEffects(call)` 只返回 range effects。
- internal direct call 使用 callee entry/exit demand mask。
- known external 使用 prototype ABI slot。
- unknown external 使用推断出的 arity 和 ABI fallback，并输出 warning。
- call return/clobber materialize 出来的值必须直接写入对应 segment 的 `CurrentDef`。

判断：

- `summary_clobber.i512` 不因为低 lane ABI 出现。
- unknown external warning 能说明哪些 range 被假设为 read、return、clobber。
- clobber 只 kill 对应 ABI range，不 kill whole ZMM。

## 第十二阶段：删除旧路径并固定验收

目标：让完整 range SSA 成为唯一实现，而不是默认先试 range、失败再走旧逻辑。

工作：

- 删除 whole-register `PendingPhi`。
- 删除 whole-register `EntryInputs`。
- 删除扫描式 `readRangeBefore()` 主路径。
- 删除任何“dominance 失败就 fallback 到 whole-register”的分支。
- cleanup、metadata、warning 都按 range SSA 的结果输出。

判断：

- `NativeRegisterSummarySSA.cpp` 中不再有独立 whole-register SSA cache。
- fortune 当前干净结果不回退。
- 新增 range SSA 单测覆盖 partial range、full read/write、call effect、entry input、recursive demand。
- fortune 同口径运行时间不能明显退化；如果超过 10%，先定位 segment 数量、PHI 数量或 block event 处理成本。

# 需要避免的错误

1. 不要用 whole-register fallback 掩盖 range planner 缺边界。一个 access 不能被 segment 完整覆盖，就是 planner 有 bug。
2. 不要让拼接值进入 segment cache。cache 只存 canonical segment。拼接是 use 点临时值。
3. 不要在 dominance 失败时静默退回旧路径。这通常说明 call value 插入点或 PHI incoming 处理错了。
4. 不要把 call argument 全部当 demand root。internal direct call 参数 demand 应来自 callee entry demand；external/indirect 才保守当 observer。
5. 不要过早做 irreducible PHI SCC cleanup。先保证 PHI 完整和 verifier 通过。
6. 不要把 unknown 写成 0。0 只用于明确的 zero-demand rewrite，并带 metadata。未知 range 应使用 frozen poison 或 helper warning。

# 测试计划

必须补这些单测：

- `AL/AH/EAX/RAX` 动态切分，确认 segments 是 `[0:8]`、`[8:8]`、`[16:16]`、`[32:32]`。
- 分支两边写 `RAX[0:32]`，merge 读 `RAX[0:32]`，生成 `phi i32`。
- 循环里 `RAX[0:32]` 自增，只生成低位 PHI。
- 写 `RAX[0:32]` 后读 `RAX[32:32]`，高位不依赖低位写。
- 写 `RAX[0:32]` 后读 `RAX[0:64]`，按需拼接低位新值和高位旧值。
- `XMM0_Qa -> ZMM0[0:64]` 参数，不生成 `i512` 参数。
- `XMM0_Qa -> ZMM0[0:64]` 返回，不生成 `summary_return.i512`。
- unknown external 只 clobber ABI killed range，不 kill whole ZMM。
- recursive pass-through ZMM 没有真实 observer 时，不产生 ZMM entry input。
- 支配关系测试：call return range 在跨 block use 时必须通过 PHI，不允许直接使用不支配的 extract。

fortune smoke 必须固定同口径：

- 使用 `/sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune`。
- 使用 `/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as` 和 `opt -passes=verify`。
- 记录运行时间、maxrss、warning 文件路径。
- 统计 `partial_read`、`partial_write`、`summary_return`、`summary_clobber`、raw register load/store、register metadata。

# 风险

- PHI 数量可能增加。动态切分能降低风险，但 full-width access 混合很多小 range 时仍会拼接较多 IR。
- call range effect 如果建模过窄，可能误删真实 clobber；如果过宽，又会回到 whole ZMM 残留。
- entry input 从 whole register 改成 segment 后，signature rewrite 的 shape 选择会更敏感，需要 warning 清楚。
- 递归函数需要 least fixed point。不能让 internal call argument 自己证明自己 live。
- 删除旧 whole-register SSA cache 时风险较高，应在前几阶段都保留开关或局部 wrapper，最后再删。

# 判断标准

完成后应满足：

- `NativeRegisterSummarySSA.cpp` 里不再有独立 whole-register `EntryValue/ExitValue/PendingPhi` 主路径。
- partial read/write helper 不再作为常规输出残留。
- 低位 XMM/ZMM ABI 不生成 whole `i512` 参数、返回或 clobber helper。
- fortune 当前干净结果不回退：raw register load/store 保持 0，`summary_clobber` 保持 0，只允许真实 unresolved indirect return 保留 warning。
- 单测覆盖 partial range、full read/write、call effect、entry input、recursive demand。
- fortune 同口径运行时间不能明显退化。如果超过 10%，需要先定位是 segment 数量、PHI 数量还是 repeated backward scan 导致。

# 不做什么

- 不改 lifting 端 partial read/write intrinsic 语义。
- 不按 8bit 默认全拆所有寄存器。
- 不在这个阶段动主 NotDec 中端类型恢复。
- 不用 0 替代 unknown。
- 不为了让 fortune 更干净盲删 unresolved indirect return。

# 实现记录：阶段一 planner/audit

本阶段只补 range planner 的边界和审计信息，不改 full load/store、partial write、call effect 的实际 SSA 重写语义。

## 已完成

- `external/NotDec-bin2llvm/include/notdec-bin2llvm/passes/summary/NativeRegisterSummarySSA.h:54`、`:102`：在函数级和总 summary 中加入 `RangeRegistersPlanned`、`RangeSegmentsPlanned`、`RangeReadEvents`、`RangeWriteEvents`、`RangeClobberEvents`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:127`：加入 `RangeEventKind`，用于审计 read/write/clobber 事件。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2742`：新增 ABI slot、summary demand mask 到 range boundary 的转换逻辑。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2847`：`planRegisterRanges()` 现在会把 ABI input/output、call killed/preserved register、`EntryDemandMask`、`ExitDemandMask` 纳入 planned segment，并统计 range read/write/clobber 事件。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4748`、`:4775`、`:5559`：把 range planner 计数写入函数 metadata、总 summary 聚合和文本输出。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:305`：新增 `summarySsaMetadataUInt()`，用于检查 `notdec.register.summary_ssa` metadata 里的计数。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5090`、`:5117`：新增 ABI float slot 和 demand-mask range split 两个 planner 测试。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage1-20260704163204`
  - 命令使用 `build/bin/notdec-native-llvm /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune --all-confirmed --skip-runtime`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=8.87 user=8.85 sys=0.01 maxrss=171512`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=2`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage1-20260704163204/register-ssa-warnings.txt`，共 7 行。

## 复杂度评估

- 实现效果：7/10。阶段一已经能暴露 planner 切分规模和 range 事件，方便后续判断 whole-register fallback 来自哪里；但还没有改变实际 SSA 主路径。
- 理解成本：3/10。新增逻辑集中在 `planRegisterRanges()` 附近，主要是补边界和计数，没有引入新的重写模型。
- 维护成本：3/10。计数是审计信息，后续完整 range SSA 落地时可以继续复用；如果 planner 改成更严格的 mandatory coverage，只需要调整这里的统计口径。

# 实现记录：阶段二 partial write range def

本阶段先把 partial write 的定义侧接到统一 range 接口上。仍保留 backward scan，不引入完整 block event 预处理，也不删除 whole-register path。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:126`：新增 `InstRangeKey`，用于记录某条写指令定义了哪个 canonical range。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1961`：新增 `LocalRangeWrites`，保存 local write 产生的 segment value。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3046`、`:3117`：新增 `readAccessRange()` 和 `readAccessRangeIfDominating()`，把 access range 读取统一转成 planned segments 拼接。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3137`、`:3162`：full-range 读取和 partial read rewrite 改为先走 `readAccessRange*()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3732`、`:3743`、`:3766`：新增 `writeSegment()`、`writeAccessRange()`、`writtenSegment()`，把写入值拆成 canonical segment 并缓存。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3788`、`:3805`：`readRangeBefore()` 遇到 full store 或 partial write 时，先调用统一 write semantics，再读取对应 segment。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5044`、`:5942`：新增并注册 loop partial write 测试，确认循环里 `RAX[0:32]` 自增只生成 `i32` PHI，不生成 whole-register `i64` PHI。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage2-20260704173037`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=9.14 user=9.11 sys=0.02 maxrss=171448`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=2`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage2-20260704173037/register-ssa-warnings.txt`，共 7 行。

## 复杂度评估

- 实现效果：7/10。partial write 已经能作为 range def 被读回，循环低位写回能生成 `i32` PHI；但 block-local currentDef 还没有完整落地。
- 理解成本：4/10。新增接口数量较少，但 `readRangeBefore()` 仍是扫描式实现，后续完整 SSA 时还要继续收敛。
- 维护成本：4/10。`LocalRangeWrites` 是过渡结构，可以迁移到后续 `CurrentDef[(block, range)]`，不应该长期扩展成复杂 matcher。

# 实现记录：阶段三 full load/store range wrapper

本阶段把 full load 的入口收敛到 `readValueBefore()`，并让它先尝试 full-range 读取。旧 whole-register 扫描仍作为 fallback 保留，`RSP` 和 segment base register 先不走 range-first，避免影响栈指针和 canary 基址这类特殊逻辑。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3188`、`:3194`：`rewriteLoads()` 不再自己直接调用 `readFullRangeValueBefore()`，统一交给 `readValueBefore()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3992`、`:3995`、`:3998`：`readValueBefore()` 增加 range-first wrapper，先用 `readFullRangeValueBefore()` 尝试按 planned segments 拼回整寄存器值，失败后继续走旧扫描路径。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4640`、`:5976`：新增并注册 `testFullStoreFeedsPartialReadThroughRangeSSA()`，确认 full `store @RAX` 后的低 32 位 partial read 可以通过 range SSA 消除，不留下 partial read helper 或原始 `RAX` load。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage3-20260704174012`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=9.71 user=9.59 sys=0.02 maxrss=170364`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=2`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage3-20260704174012/register-ssa-warnings.txt`，共 7 行。
  - `run.stdout` / `run.stderr` 都为空。

## 复杂度评估

- 实现效果：7/10。full load 现在默认先走 range 读取，full store 已能通过阶段二的 `writeAccessRange()` 被 partial read 消费；fortune 当前结果没有回退。
- 理解成本：4/10。改动入口很小，但因为 whole-register fallback 还在，读代码时仍要同时理解两条路径。
- 维护成本：4/10。这是保守过渡，后续完整 `CurrentDef[(block, range)]` 落地后，可以再删除旧 whole-register cache 和扫描 fallback。

# 实现记录：阶段四 call effect range helper

本阶段把直接调用产生的窄返回值接到 range helper，并让签名重写把这些 helper 替换成真实 call 返回值。间接调用仍保留 full-register helper，因为它没有稳定 callee shape，不能靠签名重写清掉；如果强行生成多个窄 helper，会让 fortune 的 unresolved helper 和 warning 明显回退。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:261`：新增 `RangeReturnHelper`，记录直接调用产生的窄 register range 返回 helper，供签名重写阶段替换。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1632`、`:1645`：`addDemandedExternalReturns()` 现在同时看 whole-register return helper 和 range return helper，避免只有低位返回被使用时漏掉外部函数返回签名。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3812`、`:3872`：`readRangeBefore()` 遇到直接调用且读取非整寄存器 range 时，生成 `summary_return.iN` / `summary_clobber.iN`；间接调用和整寄存器读取继续走旧 full helper。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4567`：`readSlotRangeBefore()` 增加 dominance 检查，并把新建 cast 插到 range bits 之后，修掉同块内 bits 定义不支配 use 的 verifier 问题。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4715`、`:4787`、`:4823`：新增 `callRangeValue()`、`callRangeValueHelper()`、`callRangeValueNode()`，metadata 里带上 `bit_offset` 和 `bit_width`，方便排查窄 helper 来源。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5125`、`:5565`：签名重写后用 `extractReturnRange()` 从新 call 返回值里抽取对应 range，替换并删除 range return helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4446`：扩展 libm float ABI 测试，允许 `log` 被 LLVM 折成 intrinsic，并断言低位返回 helper 不残留。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage4-20260705031101`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=9.36 user=9.32 sys=0.03 maxrss=171256`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=2`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - `range_segments_planned=1`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage4-20260705031101/register-ssa-warnings.txt`，共 7 行。
  - `run.stdout` / `run.stderr` 都为空。

## 复杂度评估

- 实现效果：8/10。直接调用的窄返回 helper 可以被签名重写消掉，fortune 保持阶段三的寄存器清洁度；间接调用仍保留 unresolved return helper，这是当前信息不足下的保守结果。
- 理解成本：5/10。多了一类 range helper 和返回抽取逻辑，但路径集中在 call effect 与签名重写两处，没有改变现有 call shape 构造。
- 维护成本：4/10。range helper 是过渡结构，后续完整 range-aware SSA 落地后仍可复用；关键约束是不能把间接调用也纳入这条路径。

# 实现记录：阶段五 entry input 和 signature rewrite range 化

本阶段让内部函数签名直接使用单段 demanded range。这样 `RDI[0:32]` 这类入口需求可以变成 `i32` 参数，不再为了低位读取制造整寄存器 entry load。最后一个 fortune 残留来自 `open` 这种外部变参调用：summary 阶段不知道后续 ABI 入参是否会被变参消费，导致 `RCX` 被误收窄成低 32 位。本次在 summary 的 backward call transfer 里对已知变参外部函数保守保留完整 ABI input demand。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1291`：新增 `integerSlotForSingleDemandRange()`，把连续单段 integer demand 转成带 `OffsetBits` / `SizeBits` 的窄 signature slot。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1515`、`:1538`、`:1561`、`:1580`：`shapeForInternalFunction()` 对 entry / exit demand 优先使用单段窄 slot，多段或不明确时仍回退整寄存器。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4392`、`:4415`、`:4448`：entry 参数查找改为同时返回 slot，并按 slot range 从参数里抽取入口 range。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5481`：新增 `integerEntrySlotReplacement()`，把 `trunc entry`、`lshr entry`、`and entry, mask` 这类从 entry load 派生的窄表达式替换成窄参数。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5606`：`rewriteInternalFunctionBody()` 只在类型一致时直接替换 entry load；类型不一致时尝试替换派生表达式，避免 RAUW 类型错误。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:655`：新增已知外部变参函数识别，覆盖 `open` / `open64` / `fcntl` / `ioctl` / `printf` / `sscanf` 等。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:1482`：已知外部变参调用在 backward demand 中保留完整 ABI input mask，防止 caller entry demand 被部分读取误收窄。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3358`、`:5992`：新增并注册窄 entry range 参数测试，确认 `RDI[0:32]` 变成 `i32` 参数且不留下 partial read helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_test.cpp:296`、`:628`：新增并注册 `open` 变参场景测试，确认 `RCX` 的 entry demand mask 保持完整 64 位。

## 验证

- `cmake --build build --target native_register_summary_test native_register_summary_ssa_test -j4 && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage5-vararg-20260705044314`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=9.44 user=9.40 sys=0.03 maxrss=169684`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=0`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage5-vararg-20260705044314/register-ssa-warnings.txt`，共 7 行。
  - `stdout.txt` / `stderr.txt` 都为空。

## 复杂度评估

- 实现效果：9/10。fortune 的 raw register load/store 保持 0，`summary_return` 和 `summary_clobber` 也降到 0；阶段五原先因为 `open` 变参导致的 `RCX.entry` 残留已清掉。
- 理解成本：5/10。签名 slot 多了 range 信息，entry load 派生替换需要看几个常见表达式形状；不过逻辑集中在签名构造和 post-signature cleanup。
- 维护成本：5/10。已知变参函数表在 summary 和 SummarySSA 里暂时重复，后续最好抽成共享 helper；当前先保持小改动，避免扩大阶段五范围。

# 实现记录：阶段六 range liveness 和 cleanup 收尾

本阶段先完成 range liveness 的收尾小闭环，不继续大改完整 SSA 主路径。核心是让 dead store cleanup 不再按“某个寄存器还有任意 live range”保留整寄存器 store，而是按写入范围和真实 live range 是否重叠判断。同时，函数出口 live seed 使用 `ExitDemandMask`，外部/直接调用的返回或 clobber effect 也按 ABI range 删除。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3567`：dead store 判断从 `hasLiveGlobalRange()` 改成 `hasLiveWriteRange()`，避免高位/低位 range 互相误保留。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3671`：`transferCallLiveness()` 对 return/clobber 调用 `eraseRegisterEffectRanges()`，float ABI 输出只擦除对应低 lane。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3693`：`addExitLiveRegisters()` 使用 `ExitDemandMask` 通过 `insertMaskRanges()` 插入 live range，不再直接插入整个寄存器。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3699`：新增 `rangesOverlap()`，统一判断写入 range 和 live range 是否相交。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3709`：新增 `insertMaskRanges()`，把 demand mask 切成 planned ranges 后加入 live 集合。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3739`：新增 `eraseRegisterEffectRanges()`，让 float output ABI 按 slot range 擦除，而不是擦掉整个 ZMM backing register。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3831`：新增 `hasLiveWriteRange()`，按 planned write ranges 和 live ranges 的 overlap 判断 store 是否仍有用。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:285`：新增 `hasRegisterStore()` 测试 helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5297`、`:6087`：新增并注册 float call effect range liveness 测试，确认 `log(double)` 这种低 lane float output 不会让整 ZMM store 残留。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage6-20260705110651`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=9.55 user=9.52 sys=0.02 maxrss=170268`
  - `partial_read=0`
  - `partial_write=0`
  - `summary_return=0`
  - `summary_clobber=0`
  - raw register load/store：`0/0`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage6-20260705110651/register-ssa-warnings.txt`，共 7 行。
  - `stdout.txt` / `stderr.txt` 都为空。

## 复杂度评估

- 实现效果：8/10。fortune 保持干净，range liveness 的 exit seed、call effect、dead store 判断都已不再无条件按整寄存器处理；但 full load 的 raw access 仍会按整寄存器插入 live ranges，这是保守行为。
- 理解成本：4/10。改动集中在 liveness transfer 和几个 helper，没有改变 SSA 构造流程。
- 维护成本：4/10。当前 helper 都复用 planned ranges；后续如果完整 `CurrentDef[(block, range)]` 落地，可以继续保留这套 liveness 判断。

# 实现记录：阶段七 block-local range currentDef

本阶段先落地同一基本块内的 range `CurrentDef`。目标不是一次性删除跨 block PHI 和旧 whole-register wrapper，而是让同一 block 内的 store、partial write、call return/clobber effect 先按顺序写入 range 当前定义，读点从这个状态拿值，不再从 use 点向前找最近写入。

## 已完成

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:126`：新增 `RangedSSAValue`，显式记录 `Value` 和 `CoveredRange`，避免裸 LLVM value 被误当成完整寄存器值。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2076`：新增 `CurrentDef[(block, range)]`，作为第一个 block-local range currentDef 缓存。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3934`：`writeSegment()` 在保留 `LocalRangeWrites` 过渡缓存的同时，把 segment 写入 `CurrentDef`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3967`：新增 `currentSegment()`，统一校验 cached value 的 covered range 和 LLVM 类型。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3991`：`readRangeBefore()` 改成从 block 入口顺序处理到读点，store、partial write、call return/clobber 都更新 `CurrentDef`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4024`、`:4079`：遇到覆盖不完整的 partial write 或 unknown call 时，只标记当前 range blocked，继续允许后面更近的写入覆盖它；读点前仍 blocked 时才返回 unknown。

## 验证

- `cmake --build build --target native_register_summary_ssa_test -j4 && build/bin/native_register_summary_ssa_test`
- `cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4 && build/bin/pcode_to_llvm_test && build/bin/native_register_summary_test && build/bin/native_register_summary_ssa_test`
- fortune smoke：
  - 输出目录：`/tmp/notdec-bin2llvm-fortune-range-stage7-fixed-20260705145413`
  - `llvm-as` 和 `opt -passes=verify` 通过。
  - 时间：`seconds=10.25 user=10.24 sys=0.01 maxrss=171532`
  - warning 文件：`/tmp/notdec-bin2llvm-fortune-range-stage7-fixed-20260705145413/register-ssa-warnings.txt`，共 7 行。
  - 精确 helper call 统计：`partial_read_calls=0`、`partial_write_calls=0`、`summary_return_calls=1`、`summary_clobber_calls=0`。
  - entry load 统计：`entry_loads=5`，与阶段六同口径一致。
  - raw register store：`0`。

## 复杂度评估

- 实现效果：7/10。同一 block 内已经有真正的 range `CurrentDef` 写入和读取，fortune 结果没有回退；但 cache 仍按读点重建，跨 block 还没有改成唯一的 Braun-style currentDef。
- 理解成本：5/10。`readRangeBefore()` 从 backward scan 变成 forward transfer，语义更接近计划，但仍和 `LocalRangeWrites`、`EntryRangeValue` 并存，读代码时需要知道这是过渡状态。
- 维护成本：5/10。当前实现小而可控，后续第八阶段应把 predecessor 递归和 range PHI 接到同一套 `CurrentDef`，并逐步删除 `LocalRangeWrites`。

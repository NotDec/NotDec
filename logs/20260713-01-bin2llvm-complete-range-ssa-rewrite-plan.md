# 原始 prompt

不能这样一次改一个小改，导致功能无法落地且对接混乱。详细规划一下，如何一次性重写完整 range SSA，形成一个单独的规划文件

# 本次修订 prompt

直接彻底改进plan吧，而不是机械地补几个点

# 背景

`NativeRegisterSummarySSA` 现在已经有一些 range-aware 代码，但还不是一套完整 SSA。

真正的问题不是某个 matcher 漏了，而是表示层不统一：

- 一部分代码把 register 当 whole global。
- 一部分代码把 register 拆成 bit range。
- 一部分代码按原始 load/store 大小读写。
- 一部分代码按 planner 切好的 segment 读写。
- fallback 路径还会在 range 失败后退回旧 whole-register 逻辑。

这和 Braun SSA 的基本要求冲突。Braun 里的 variable 必须是一等、稳定、唯一的东西，所有 `readVariable` / `writeVariable` 都围绕同一套 `currentDef` 工作。当前 native range SSA 最大的问题，就是 full access、partial access、call effect、entry value、cleanup 没有完全落到同一套 variable 和 currentDef 上。

这份计划只处理 range SSA 的表示和重写，不处理下游接口恢复策略。

# 目标

一次性重写 `NativeRegisterSummarySSA` 的 range SSA 主闭环。

目标不是多清几个 helper，而是让寄存器状态满足这几个规则：

1. **SSA variable 只使用 canonical range**，例如 `RAX[0:8]`、`RAX[8:8]`、`RAX[16:16]`、`RAX[32:32]`。
2. **LLVM SSA value 可以覆盖多个 canonical range**，例如一个 i64 value 覆盖 `RAX[0:64]`。
3. **原始访问大小要保留**，full-width register read/write 不应该因为内部切 range 就强制拆成多个 partial read 再拼回去。
4. **所有寄存器读写都走同一套接口**，包括 full load/store、partial helper、entry value、call read/return/clobber、liveness。
5. **旧 whole-register SSA fallback 退出主路径**，失败只能产生明确 unknown 或 warning，不能静默退回旧逻辑。

# 核心设计

## 1. Canonical Range 是唯一 SSA Variable

Braun 算法里的 variable，在这里就是：

```text
RegisterRangeKey(global, bit_offset, bit_width)
```

planner 的职责是先确定每个 register 的 canonical ranges。后续 SSA 只对这些 canonical ranges 建 `currentDef` 和 PHI。

不能把任意访问范围都当 variable。例如同时把 `RAX[0:64]`、`RAX[0:32]`、`RAX[0:8]` 都当 variable，会重新引入“一个写入定义另一个变量的一部分”的问题。

## 2. Covered Value 只是来源属性，不是新 Variable

一个 LLVM value 可以覆盖比当前 canonical range 更大的 bit 区间。

```cpp
struct RangedSSAValue {
  llvm::Value *Value = nullptr;
  RegisterRangeKey CoveredRange;
};
```

例如：

```text
CurrentDef[(block, REG[0:16])]  = { %reg64, REG[0:64] }
CurrentDef[(block, REG[16:48])] = { %reg64, REG[0:64] }
```

这不表示 `REG[0:64]` 是 Braun variable。真正的 variable 仍然是 canonical segment。`CoveredRange` 只说明 `%reg64` 可以被用来 materialize 哪些 segment。

规则：

- `CoveredRange` 必须显式存在，不能用空 range 表示完整寄存器。
- `Value` 的 bit width 必须等于 `CoveredRange.BitWidth`。
- `CoveredRange` 必须覆盖 cache key 对应的 canonical range。
- PHI 永远按 canonical range 类型创建，不按 covered range 类型创建。

## 3. Full Access 保形，Segment SSA 保底

原始 full-width 访问应该优先保留 full-width value。

底层访问类似：

```text
mov %reg64, [mem]
```

理想 IR 是：

```llvm
store i64 %reg64, ptr %slot
```

不是：

```llvm
%a = partial_read REG[0:16]
%b = partial_read REG[16:48]
%v = or (zext %a), (shl (zext %b), 16)
store i64 %v, ptr %slot
```

但如果中间某个 segment 被 partial write 改过，full read 就必须拼接。这是正确的：

```text
write REG[0:16] = x
read  REG[0:64]
```

这时低 16 位来自 `x`，其它位来自旧 covered value，不能继续直接返回旧 `%reg64`。

# 事件层

## 1. 先构建 Block Register Events

Braun 文档强调 predecessor 必须是 filled block。这里 CFG 已经完整，但 block 内寄存器事件也必须先完整。

重写前先为每个 block 收集 register events：

```text
RegisterEvent {
  Instruction *Inst;
  Kind: Read | Write | CallRead | CallReturn | CallClobber | Unknown;
  AccessRange;
  AccessShape;
  Value;
}
```

作用：

- 把原始 IR 访问大小记录下来。
- 把 partial helper、full load/store、call effect 统一成事件。
- 避免边删 IR 边查询 currentDef，导致状态受 rewrite 顺序影响。

## 2. Block 内按事件顺序处理

每个 block 内按 instruction order 处理事件。

- write event 调用 `writeAccess` 或 `writeCoveredValue`。
- read event 调用 `readAccessBefore`，再替换原始 use。
- call clobber 写 explicit unknown。
- call preserve 不改 currentDef。

不能随机从某个 use 开始扫 block 并移动全局 cursor。`readBefore(inst)` 必须基于该 instruction 前缀的状态。

实现上可以有两种方式：

1. 顺序 rewrite 每个 block，维护当前 block 的 prefix state。
2. 如果需要随机查询，必须缓存 `(block, instruction, range)` 前缀状态。

第一版优先选 1，代码简单，也更贴近 Braun 的 `writeVariable` / `readVariable` 流程。

# SSA 状态

建议统一成一组状态：

```text
CurrentDef[(block, range)] -> RangedSSAValue
EntryDef[(block, range)]   -> RangedSSAValue
ExitDef[(block, range)]    -> RangedSSAValue
PendingPhi[(block, range)] -> PHI
Resolving[(block, range)]  -> recursion guard
EntryInputs[covered range] -> RangedSSAValue
Replacement[value]         -> value
UnknownDef[(block, range)] -> explicit unknown marker
```

说明：

- `CurrentDef` 是 block 当前前缀状态。
- `EntryDef` 是 block 入口状态缓存。
- `ExitDef` 是 block 出口状态缓存。
- `PendingPhi` 只服务 canonical range。
- `Replacement` 负责 trivial PHI 删除和后续 resolve。
- `UnknownDef` 不能被普通 covered value 混淆。

# 读写接口

## 1. Segment 读

```text
readSegmentBefore(range, before) -> segment-width value
readSegmentEntry(block, range)   -> segment-width value
readSegmentExit(block, range)    -> segment-width value
```

读取规则：

1. 查当前状态里的 `RangedSSAValue`。
2. 如果 `CoveredRange == range`，直接返回 `Value`。
3. 如果 `CoveredRange` 包含 `range`，在正确插入点 extract。
4. 如果没有定义，走 Braun 递归读 predecessor。
5. 如果是 unknown，返回带 metadata 的 frozen poison。

## 2. Access 读

```text
readAccessBefore(global, offset, width, before, shape) -> access-width value
```

读取规则：

1. 找覆盖 access range 的 canonical segments。
2. 读取每个 segment。
3. 如果所有 segment 都来自同一个 covered value，且这个 covered value 覆盖整个 access range，直接从 covered value materialize access-width value。
4. 否则按 segment 拼接。

这里的 `shape` 只用于决定 materialize 偏好，不参与 SSA variable 选择。

建议第一版只保留很少的 shape：

```text
FullAccess
PartialAccess
SlotAccess
InternalAssemble
```

## 3. Covered 写

```text
writeCoveredValue(block, coveredRange, value)
```

作用是把同一个 `value` 绑定到 `coveredRange` 覆盖的所有 canonical ranges。

例如 full i64 写入：

```text
writeCoveredValue(REG[0:64], %v)
```

如果后续写 `REG[0:16]`，只替换被覆盖的 segment，其它 segment 仍可指向旧 `%v`。

## 4. Access 写

```text
writeAccessRange(global, offset, width, value, shape)
```

规则：

- `FullAccess` 优先调用 `writeCoveredValue`。
- x86-64 低 32 位写清高位，应生成 i64 covered value 后写 `REG[0:64]`。
- 普通 partial write 只定义写入范围覆盖的 canonical segments。
- 如果写入范围无法被 canonical segments 完整覆盖，说明 planner 有 bug，不能临时 fallback。

# PHI 设计

## 1. PHI 只按 Canonical Range 创建

不管当前 segment 来自 i64 covered value，PHI 都只能是 segment 类型。

原因：合流后不同 predecessor 可能来自不同 covered value。segment PHI 是唯一稳定表示。

## 2. 多前驱先建 PHI 再递归

`readSegmentEntry(block, range)` 遇到多个 predecessor 时：

1. 创建 `phi rangeType(range)`。
2. 立刻写入 `EntryDef[(block, range)]`。
3. 再递归读取每个 predecessor 的 exit。
4. incoming value 必须在 predecessor terminator 前 materialize 成 segment 类型。
5. 补齐 incoming 后尝试 trivial PHI 删除。

不能先递归再建 PHI。

## 3. PHI Incoming 的插入点

如果 incoming 来自 covered value，比如 `%reg64` 覆盖 `REG[0:64]`，而 PHI 需要 `REG[16:48]`，extract 必须插在 predecessor terminator 前。

不能把 extract 插在 PHI 所在 block 里。PHI operand 必须来自 predecessor 边。

## 4. Trivial PHI 删除要更新所有缓存

删除 trivial PHI 后，必须同步更新：

- `CurrentDef`
- `EntryDef`
- `ExitDef`
- `PendingPhi`
- `Replacement`
- PHI users

并递归检查受影响的 PHI。

self-only PHI 不建议直接 `undef`。更适合生成带 metadata 的 frozen poison，说明这里是没有真实 incoming 的 unknown range。

# Entry Value

函数入口值也要遵守 covered value 规则。

如果需要完整入口寄存器，优先生成：

```llvm
%REG.entry = load i64, ptr @REG
```

并记录：

```text
EntryInputs[REG[0:64]] = { %REG.entry, REG[0:64] }
```

然后把它覆盖的 canonical segments 都映射到这个 covered value。

只有真实需要非完整入口 range，并且没有完整入口值可复用时，才生成 partial entry read。

这保证 full-width 原始读不会因为 planner 切分而默认变成多个 partial entry read。

# Call Effect

Call effect 只进入 range SSA，不在这里推断新接口。

规则：

- call read：按已有 slot range 调用 `readAccessBefore`。
- call return：按 output slot 调用 `writeAccessRange` 或 `writeCoveredValue`。
- call clobber：按 clobber range 写 explicit unknown。
- call preserve：不改 currentDef。
- 未知 effect：写 explicit unknown，不退回 whole-register fallback。

如果 ABI effect 是完整寄存器，可以覆盖完整 range。否则只影响对应 slot range。

# Liveness 和 Cleanup

liveness 必须理解 covered value。

规则：

- full-width store 覆盖多个 canonical range，只有这些 range 全部 dead 时才能删。
- partial write 只影响写入 range，不杀其它 segment。
- unknown value 不能被当作普通定义合并。
- cleanup 删除 helper 前，要确认对应 value 不再被任何 live range 或 rewrite 状态引用。
- `StoresToErase` erase 前必须检查 instruction 仍在 IR 中，不能保存 stale pointer 后盲删。

# 迁移路线

这次按主闭环推进，不做一个小补丁一个小补丁的提交。

## 阶段 A：建立事件层和状态层

目标：

- 收集每个 block 的 register events。
- 引入 `RangedSSAValue` 和 covered range 检查。
- 建立 `CurrentDef / EntryDef / ExitDef / PendingPhi / UnknownDef`。
- 实现 resolve 和 cache 更新规则。

判断标准：

- 不改主要 rewrite 行为也能打印/检查每个 block 的 register events。
- covered range 类型检查能发现不合法绑定。

## 阶段 B：实现 Segment SSA

目标：

- 实现 `readSegmentBefore/Entry/Exit`。
- 实现 segment PHI 创建、incoming materialize、trivial PHI 删除。
- 删除或禁用旧 whole-register entry/exit 主路径。

判断标准：

- loop/header 场景不会递归不止。
- 所有 PHI incoming 完整。
- trivial PHI 删除后 cache 不指向旧 PHI。

## 阶段 C：接入 Access Read/Write

目标：

- 实现 `readAccessBefore`。
- 实现 `writeCoveredValue` 和 `writeAccessRange`。
- full load/store、partial read/write helper 全部走新接口。
- x86-64 低 32 位写清高位作为特殊 full covered write。

判断标准：

- full-width register-to-memory store 不再默认生成多个 entry partial read 拼接。
- partial write 后未写 range 仍能读到旧定义。
- verifier 通过。

## 阶段 D：接入 Call Effect 和 Unknown

目标：

- call read/return/clobber/preserve 全部按 range 进入 SSA。
- unknown effect 生成明确 unknown value 和 metadata。
- 不再整寄存器 kill，除非 ABI effect 本身是完整寄存器。

判断标准：

- unknown/clobber 不静默变成 0。
- call 后读取 preserved range 仍能看到旧定义。
- call 后读取 clobbered range 得到明确 unknown。

## 阶段 E：接入 Liveness 和 Cleanup

目标：

- range liveness 理解 covered value。
- dead store 删除按 covered range 判断。
- cleanup 删除 helper 和 metadata 时不破坏 live range。

判断标准：

- full-width store 只有覆盖 range 全 dead 时才删。
- 不出现 stale instruction erase 崩溃。
- 没有 operandless PHI。

## 阶段 F：验证和记录

目标：

- 跑当前 native smoke 样例。
- 跑 lighttpd-angel。
- 再选一个 Bench2 小二进制。
- 更新本 plan，记录实现文件、函数、验证命令和结果。
- 子模块内提交，再提交顶层指针和日志。

# 风险

## 1. Covered Value 和 Segment PHI 混用

风险：某个 segment cache 指向 full value，但合流后另一个 incoming 是 segment value。

处理：PHI 永远按 segment 建。只有 use 点读 access range 时，才尝试复用同一个 covered value。

## 2. Prefix 状态错位

风险：随机查询 `readBefore(inst)` 时，`CurrentDef` 表示的不是 inst 前缀状态。

处理：第一版按 block instruction order rewrite，避免随机查询。后续如果需要随机查询，再加 prefix cache。

## 3. Extract 插入点错误

风险：PHI incoming 的 extract 插在 PHI block，而不是 predecessor terminator 前。

处理：所有 incoming materialize 必须显式传 predecessor terminator 作为插入点。

## 4. Unknown 被当普通值传播

风险：call clobber 或无法覆盖的 range 被当成普通 SSA value，后续优化误用。

处理：unknown value 必须带 metadata，并和普通 covered value 区分。

## 5. PHI 清理不完整

风险：trivial PHI 删除后，cache 或其它 PHI 还引用旧 PHI。

处理：统一 `Replacement`，并在删除 PHI 后递归检查 PHI users。

# 判断标准

必须满足：

- `llvm-as` 和 `opt -passes=verify` 通过。
- full-width 原始访问不再默认变成多个 entry partial read 拼接。
- 没有 operandless PHI。
- 没有 stale instruction erase 崩溃。
- range 失败不退回 whole-register fallback。
- unknown/clobber 不静默写成 0。

可以暂时接受：

- irreducible CFG 上 PHI 不是 minimal。
- 某些 partial write 后仍有必要的 `lshr/trunc` extract。
- debug metadata 暂时保留，只要最终 cleanup 能清理无用 helper。

# 不做什么

- 不在 lifting 阶段大改 pcode lowering。
- 不把所有 register 固定拆成 8-bit segment。
- 不在本计划里改下游接口恢复策略。
- 不用旧 slot + mem2reg 思路替代 range SSA。
- 不把 unknown/clobber 静默写成 0。

# 预期效果

重写完成后，主线应变成：

```text
原始 register access
  -> register events
  -> canonical range SSA variables
  -> covered value 保留 full-width 来源
  -> segment PHI 保证 CFG 合流正确
  -> access materialization 保持原始访问大小
  -> range liveness / cleanup 删除无用 helper
```

这套 range SSA 应该能独立成立。后续下游接口恢复可以消费它的结果，但不应该和它耦合在一起。

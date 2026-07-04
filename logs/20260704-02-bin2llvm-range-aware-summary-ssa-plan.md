# 原始 prompt

总结一下当前的规划到新的logs/文件，写完规划文件后再启动一个goal，重构SummarySSA

# 背景

当前 bin2llvm native 链路已经引入了：

- `notdec.partial_write.iFULL.iWRITE(ptr @REG, value, i64 bit_offset)`
- `notdec.partial_read.iFULL.iREAD(ptr @REG, i64 bit_offset)`

这两类 helper 解决了 lifting 阶段把 partial register access 伪装成 full-width load/store 的问题。现在窄读不再直接 lift 成 `load i64 @RAX + and/trunc`，而是明确表达成读取某个 bit range。

但 SummarySSA 仍主要把一个完整 register global 当成 Braun SSA 里的 variable。例如 `RAX` 是一个 `i64` variable，`ZMM0` 是一个 `i512` variable。partial read/write 目前是在这个 whole-register SSA 上做特殊处理：

- partial read 先尝试从前面的 partial write 直接取值。
- 否则读完整 register SSA value，再 `lshr/trunc` 抽取位段。
- partial write 仍可能需要旧完整 register value 来保留未写位。

这能明显减少 fortune 残留，但还不完整。剩余 `partial_read` / `partial_write` 多集中在跨 block、PHI、未知入口值和 ZMM lane 搬运上。根因是 SSA 变量粒度太粗，写低位时仍可能被迫引入高位。

# 目标

把 SummarySSA 的 register variable 从 whole-register 改成 range-aware。

核心目标是让 Braun SSA 构建直接支持：

```text
readVariable(RAX[0:32], block)
writeVariable(RAX[0:32], value, block)
readVariable(ZMM0[64:64], block)
```

而不是先构造完整 `RAX[0:64]` 或 `ZMM0[0:512]`，再从中抽取。

最终效果：

- partial read/write 成为 SSA 构建的一等读写操作。
- 低位读写不再引入无关高位 entry input / PHI。
- ZMM/XMM lane 只为实际访问的 lane 构造 SSA。
- `readCoveredPartialWriteBefore()` 这类局部补丁可以被统一 range SSA 逻辑替代或退化为普通 fast path。

# 技术路线

## 1. 动态切分 register range

不要直接按 8bit 全拆。直接拆 byte 对 RAX 可接受，但 ZMM0 会变成 64 个变量，full-width access 也会生成大量拼接，容易让 IR 膨胀。

采用按访问边界动态切分：

1. 每个函数先扫描 register access。
2. 对每个 register global 收集边界：
   - 总是加入 `0` 和 full width。
   - partial read/write 加入 `[offset, offset + width)`。
   - full load/store 加入 `[0, fullWidth)`。
   - ABI input/output/clobber/preserve 加入对应 slot range。
   - summary return/clobber helper 加入对应 register range。
3. 排序后相邻边界形成 canonical segment。

例如：

```text
RAX[0:64], EAX[0:32], AL[0:8], AH[8:8]
=> boundaries: 0, 8, 16, 32, 64
=> segments: [0:8], [8:8], [16:16], [32:32]
```

Braun SSA 的 variable 不直接使用原始 access range，而是使用这些不重叠 segment。这样可以避免 `RAX[0:32]` 和 `AL[0:8]` 两个 variable 互相包含导致定义关系不一致。

## 2. 新增 range key 和 cache

在 `NativeRegisterSummarySSA.cpp` 中引入类似：

```cpp
struct RegisterRangeKey {
  llvm::GlobalVariable *Global;
  uint64_t BitOffset;
  uint32_t BitWidth;
};
```

然后把当前这些 whole-register 状态逐步替换为 range 状态：

```text
EntryValue[(block, global)]
ExitValue[(block, global)]
PendingPhi[(block, global)]
ResolvingEntry[(block, global)]
EntryInputs[global]
```

变成：

```text
EntryRangeValue[(block, range)]
ExitRangeValue[(block, range)]
PendingRangePhi[(block, range)]
ResolvingRangeEntry[(block, range)]
EntryRangeInputs[range]
```

PHI 类型用 segment 宽度，例如 `RAX[0:32]` 是 `i32`，不是 `i64`。

## 3. 实现 range read/write

新增核心接口：

```text
readRangeBefore(block, range, before)
readRangeEntry(block, range)
readRangeExit(block, range)
writeRange(block, range, value)
```

规则：

- partial read：读覆盖它的 segments，必要时 concat。
- partial write：把写入 value 拆成 segments，分别写入。
- full read：读 full range 覆盖的所有 segments，再拼成 full-width value。
- full write：把 full value 拆成所有 segments，分别写入。

这样写低 32 位时只定义低位 segments；只有后面真的读完整 64 位时，才会按需读取和拼接高位 segments。

## 4. Braun SSA 递归保持不变，但变量换成 segment

参考 `docs/analysis/braun-ssa-construction.md`，核心流程仍是：

- `readRangeEntry()` 先查 cache。
- 递归中遇到同一个 `(block, range)`，先创建空 PHI 打断循环。
- 多 predecessor block 先创建 PHI，再递归读 predecessor exit。
- `finalizePendingPhis()` 补齐所有 incoming。
- `simplifyPhi()` 删除 trivial PHI。

不同点只是 variable 从完整 register unit 变成 canonical segment。

## 5. call / ABI effect 改成 range-aware

当前 call effect 主要按 register unit 判断：

- return value
- clobber
- preserve
- unknown

重构后要按 range 表达：

- `RAX` 返回值：定义 `RAX[0:64]` 覆盖的 segments。
- `EAX` 风格返回值：只定义 `RAX[0:32]` 覆盖的 segments。
- `XMM0_Qa`：定义 `ZMM0[0:64]` 覆盖的 segments。
- clobber：对应 range 变成 unknown value。
- preserve：不写 segment，继续往前读。

这一步对 ZMM 残留最关键，否则 float ABI 仍会被 whole `i512` 拖住。

## 6. liveness / dead store 改成 range-aware

当前 liveness 是 whole-register：

```text
set<GlobalVariable *>
```

需要改成 range 集合：

```text
map<GlobalVariable *, interval set>
```

规则：

- partial read 让对应 range live。
- partial write kill/define 对应 range。
- full write kill/define full range。
- 如果 write range 和 live range 没有 overlap，这个 write/helper 可以删。
- 如果只有写入值的部分 bit 被用到，demand 只传播这些 bit。

这可以逐步替代现在的 `zeroDemandReplacement()` 特殊处理。

## 7. 迁移顺序

建议分阶段做，不一次性推翻现有 SummarySSA。

第一阶段：

- 加 `RegisterRangeKey`、segment planner、range cache。
- 只让 `partial_read` 走 range SSA。
- full load 和 full store 暂时继续走旧 whole-register SSA。
- 验证 partial read residual 是否下降，确保签名重写不回归。

第二阶段：

- partial write 改成 range def。
- 删除或弱化 `replacePartialRegisterValue()` 对 partial write 的依赖。
- 验证低位写不再引入高位 entry input / PHI。

第三阶段：

- full load/full store 也转成 range read/write。
- whole-register SSA 退化为 range `[0, fullWidth)` 的一种访问。
- 清理旧的 `(block, global)` cache。

第四阶段：

- call ABI effect、return/clobber helper、liveness/dead store 全部切到 range-aware。
- 处理 ZMM/XMM lane。
- 清理临时 fast path 和旧 partial demand 特判。

# 判断标准

必须新增这些测试：

- 分支两边写不同 `RAX[0:32]`，merge 后读 `RAX[0:32]`，生成 `phi i32`，不生成 `phi i64`。
- 循环里读写 `RAX[0:32]`，只生成低位 PHI。
- 写 `RAX[0:32]` 后读 `RAX[32:32]`，不能依赖低位写。
- 写 `RAX[0:32]` 后读完整 `RAX[0:64]`，才按需读取并拼接高位。
- `AL` / `AH` 访问通过动态边界切分正确处理 `[0:8]` 和 `[8:8]`。
- ZMM lane 搬运只为实际 lane 建 SSA，不为整个 `i512` 建 PHI。
- fortune 当前用例 `llvm-as` 和 `opt -passes=verify` 通过。
- fortune 中 `partial_read`、`partial_write`、raw register load/store、`notdec.register.access` 继续下降。
- 同口径 fortune 运行时间不能明显退化。

# 风险

- range-aware 后 PHI 数量可能增加，尤其是 full-width access 混合很多小段时。需要动态切分，而不是 byte 全拆。
- segment concat/extract 可能生成较多 IR，需要依赖 InstCombine 或本地简化。
- call ABI range 如果建模不准，会误删真实 clobber 或误保留无关寄存器。
- irreducible CFG 仍可能留下非 minimal PHI。第一版可以接受，但不能留下 incomplete PHI。
- 与签名重写、return helper、call arg store 删除耦合较深，最好每阶段都有 fortune smoke。

# 不做什么

- 不把所有 register 默认按 8bit 全拆。
- 不改变 lifting 端 partial read/write helper 语义。
- 不一次性改主 NotDec 中端类型恢复。
- 不用 fallback zero 掩盖未知 range；未知仍应明确 materialize 为 frozen poison 或 helper warning。

---

# 实现记录 2026-07-04

## 已完成范围

本次先完成第一阶段：让 partial read 可以走 range-aware Braun SSA。full load/store、call ABI、return/clobber helper 和 liveness 仍保留 whole-register 主路径，只在 partial read 需要时读 range。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:104` 新增 `RegisterRangeKey` 和 `BlockRangeKey`，用 `(global, bit_offset, bit_width)` 表示 SummarySSA 的窄变量。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1837` 在 `FunctionBuilder::run()` 里 `collectAccesses()` 后调用 `planRegisterRanges()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1887` 新增 range entry/exit/pending PHI/cache，以及每个 register 的动态边界和 canonical range 记录。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2652` 新增 `planRegisterRanges()`，按 register global、full load/store、partial read/write 收集边界，再形成不重叠 segment。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2733` 新增 `plannedRangesCovering()` 和 `assembleRangeRead()`，partial read 覆盖多个 segment 时按低到高拼接。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2802` 修改 `rewritePartialReads()`，优先用 range SSA 替代 partial read，再退回旧的局部 partial-write fast path 和 whole-register fallback。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3142` 新增 `readRangeBefore()` / `readRangeEntry()` / `readRangeExit()` / `ensureRangePhi()` / `completeRangePhi()` / `simplifyRangePhi()`，复用 Braun SSA 递归结构，但 PHI 类型是 segment 宽度。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3606` 把 range PHI 加入 `finalizePendingPhis()`，避免 incomplete PHI 留下。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3665` 新增 `entryRangeInput()`，当前先从 whole entry input 抽取 segment，后续再继续窄化 entry input。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4051` 让 `attachMetadata()` 统计 range PHI。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3993` 新增 `testBranchPartialReadUsesNarrowRangePhi()`，验证分支两边写 `RAX[0:32]` 后 merge 读 `RAX[0:32]` 时会创建 range PHI，且不生成 whole-register `i64` PHI。

## 实现判断

这次改动不是完整 range-aware SummarySSA，只是把最容易产生虚假高位依赖的 partial read 入口切过去。这个阶段的好处是改动风险较低，fortune 已经能看到明显下降；缺点是 entry input、partial write、full load/store、call ABI effect、return/clobber helper 和 liveness 还没有统一 range 化，后续仍会留下 partial helper 和部分 raw register access。

复杂度评分：

- 实现效果：7/10。partial read 残留、partial write 残留、summary clobber、raw register access 都下降，但还不是最终架构。
- 理解成本：6/10。现在同时存在 whole-register SSA 和 range SSA 两套 cache，短期可控，但不能长期保留太久。
- 维护成本：6/10。第一阶段复用旧逻辑较多，后续迁移 full load/store 和 liveness 时要小心删掉重复 fallback。

更好的长期方案仍是把 full load/store、partial write、call ABI、return/clobber helper、liveness 全部切到 range-level，最后删除旧 `(block, global)` SSA cache。

## 验证

单测和构建：

```bash
cmake --build build --target native_register_summary_ssa_test -j4
./build/bin/native_register_summary_ssa_test

cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4
./build/bin/pcode_to_llvm_test
./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
./build/bin/notdec-native-llvm /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-range-20260704050113/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-range-20260704050113/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-range-20260704050113/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-range-20260704050113/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-range-20260704050113/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-range-20260704050113/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-range-20260704050113`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=6.87 user=6.83 sys=0.03 maxrss=170868`
- `partial_read_calls=35`
- `partial_write_calls=21`
- `summary_return_calls=2`
- `summary_clobber_calls=28`
- `register_access_metadata=57`
- `raw_register_loads=2`
- `raw_register_stores=3`
- `warning_lines=34`

对比上一版记录 `/tmp/notdec-bin2llvm-fortune-partial-read-20260704040956/fortune.ll`：partial read 从 60 降到 35，partial write 从 34 降到 21，summary clobber 从 37 降到 28，register access 从 100 降到 57，raw load/store 从 10/6 降到 2/3。运行时间从 `6.94s` 到 `6.87s`，没有看到性能退化。

## 后续

下一步建议按同一个文件继续：

- partial write 直接定义 range，不再通过 full old value 保留高位。
- full load/store 切成 segment read/write。
- call ABI effect 和 return/clobber helper 按 ABI slot range 定义。
- liveness/dead store 改成 range live set。
- entry input 从 whole-register load 抽取，继续改成按需窄 entry input。

---

# 实现记录 2026-07-04 range liveness

## 已完成范围

本次继续推进 partial read/write 后面的 dead store liveness，把原来的 whole-register live set 改成 range live set。full load/store 和 call ABI 仍保守映射为完整 register 的所有 segment；partial read/write 则只让对应 bit range 参与活跃判断。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:115` 给 `RegisterRangeKey` 增加 `operator==`，并新增 `LiveRegisterRanges`，方便 liveness 固定点比较。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2933` 将 `removeDeadStoresByLiveness()` 的 `liveIn/liveOut` 从 `set<GlobalVariable *>` 改成 `LiveRegisterRanges`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2977` 将 `transferBlockLiveness()` 改成传递 range live set。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3004` 修改 `eraseDeadStoresInBlock()`：full store 仍看该 global 是否有任意 live range；partial write 则只看写入 range 是否 live。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3059` 修改 store/load/call 的 liveness transfer：full store kill 所有 segment，full load 让所有 segment live，call return/clobber kill 所有 segment，call read 让所有 segment live。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3083` 修改 partial read/write transfer：partial read 只插入覆盖的 planned ranges，partial write 只 kill 覆盖的 planned ranges。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3142` 新增 `insertGlobalRanges()`、`eraseGlobalRanges()`、`insertPartialRanges()`、`erasePartialRanges()`、`hasLiveGlobalRange()`、`hasLivePartialRange()`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4073` 新增 `testDeadPartialWriteUsesRangeLiveness()`，验证写 `RAX[0:32]` 后只读 `RAX[32:32]` 时，低位 partial write 会被删除。

## 实现判断

这一步不是完整 partial write range SSA；它先解决 dead store 判断太粗的问题。现在 partial write 是否保留由写入 bit range 是否被后续读取决定，不再因为同一个 register 的其他 bit range live 而保留。

复杂度评分：

- 实现效果：8/10。fortune 的 partial write 残留从 21 降到 13，raw register load 从 2 降到 0。
- 理解成本：6/10。range read SSA 和 range liveness 已经共用 `RegisterRangeKey`，但 whole-register SSA 仍存在。
- 维护成本：6/10。full load/store 和 call ABI 仍用完整 segment 保守映射，后续迁移时需要继续收窄。

## 验证

单测和构建：

```bash
cmake --build build --target native_register_summary_ssa_test -j4
./build/bin/native_register_summary_ssa_test

cmake --build build --target pcode_to_llvm_test native_register_summary_test native_register_summary_ssa_test notdec-native-llvm -j4
./build/bin/pcode_to_llvm_test
./build/bin/native_register_summary_test
./build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
./build/bin/notdec-native-llvm /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.06 user=7.03 sys=0.02 maxrss=170920`
- `partial_read_calls=35`
- `partial_write_calls=13`
- `summary_return_calls=2`
- `summary_clobber_calls=28`
- `register_access_metadata=47`
- `raw_register_loads=0`
- `raw_register_stores=3`
- `warning_lines=34`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-range-20260704050113`：partial write 从 21 降到 13，register access metadata 从 57 降到 47，raw register loads 从 2 降到 0。运行时间从 `6.87s` 到 `7.06s`，略有波动但没有明显退化。

## 后续

下一步仍是把定义侧真正切到 range：

- partial write 直接成为 range def，而不是只在 liveness 中按 range 处理。
- full load/store 切成 segment read/write，逐步删除 whole-register SSA cache。
- call ABI effect 和 return/clobber helper 按 ABI slot range 定义，尤其是 XMM/ZMM lane。

---

# 实现记录 2026-07-04 range call arguments

## 已完成范围

本次继续推进 call 参数读取，把 `callArgStoreBindings()` 从先读 whole register 再 cast，改成优先按签名 slot 的 bit range 读取。这样低 32 位参数可以直接从 range SSA 得到，不需要为了构造实参先读完整 `RDI/R9/...`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3895` 修改 `callArgStoreBindings()`：参数绑定现在调用 `readSlotValueBefore()`，返回值已经是 `slotType(slot)`，后续签名重写直接使用该值。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3935` 新增 `readSlotValueBefore()`：先走 `readSlotRangeBefore()`，失败时才退回 whole-register `readValueBefore()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3955` 新增 `readSlotRangeBefore()`：按 `slot.OffsetBits/slot.SizeBits` 查 planned ranges，通过 `assembleRangeRead()` 拼成整数或 float ABI 参数。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3991` 新增 `findNearestStoreBeforeCall()`：只扫描 call 前最近的同寄存器 store，用于保留旧的 ABI argument store 清理证据；不再为了找证据额外构造 whole-register SSA value。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:1643` 新增 `testCallArgUsesPartialRangeRead()`，覆盖 partial write 后直接作为外部 call 参数的场景，确认不生成 whole-register `i64` PHI，partial helper 被消除，call 被正常重写。

## 实现判断

这一步只收窄 call 参数收集，不改 full load/full store 的全局重写策略。这样避免了之前 full-load range 实验中出现的支配关系问题：新值只在 call 前 materialize，只服务当前 call 参数，不跨 block 复用到 PHI。

复杂度评分：

- 实现效果：7/10。fortune 的 `register.access` 从 47 降到 45，真实 raw register global store 从 4 降到 2，带 `register.access` 的 raw store 从 3 降到 1；partial read/write 数量持平。
- 理解成本：6/10。参数读取现在有 slot range 路径和 whole-register fallback 两层，但范围只在 call 参数绑定内。
- 维护成本：6/10。`findNearestStoreBeforeCall()` 是为了保留 ABI store 清理证据的过渡逻辑，后续 full store 切到 range 后可以再统一。

## 验证

单测和构建：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target pcode_to_llvm_test native_register_summary_test \
  native_register_summary_ssa_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.24 user=7.20 sys=0.03 maxrss=169628`
- `partial_read_calls=35`
- `partial_write_calls=13`
- `summary_return_calls=2`
- `summary_clobber_calls=28`
- `register_access_metadata=45`
- `raw_load_all=1`，其中 `raw_load_summary_entry=1`，`raw_load_register_access=0`
- `raw_store_global_register=2`
- `raw_store_register_access=1`
- `warning_lines=33`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-range-liveness-20260704051959`：partial read/write 持平，`register.access` 从 47 降到 45，warning 从 34 降到 33。上一阶段 raw load 统计里有 3 个 `load i64, ptr @...`，其中 1 个是 SummarySSA entry 脚手架；本阶段只剩 1 个 entry 脚手架 load，不带 `notdec.register.access`。运行时间从 `7.06s` 到 `7.24s`，略有波动，未见明显性能退化。

## 后续

下一步建议继续处理定义侧：

- partial write 直接成为 range def，减少对 whole-register fallback 的依赖。
- full load/store 切成 segment read/write，但需要先设计 dominance-safe materialization，避免之前 call return/clobber extract 不支配 PHI use 的问题。
- call ABI effect 和 return/clobber helper 按 ABI slot range 定义，尤其是 XMM/ZMM lane。

---

# 实现记录 2026-07-04 xor zero demand

## 已完成范围

本次处理 fortune 里 ZMM lane 清零残留的一类具体问题：lifting 会生成两个连续的同范围 partial read，然后 `xor` 这两个值，再 partial write 回同一 lane。SummarySSA 把两个 read 替换成同一个 range value 后，这个 `xor` 实际是清零，不应该继续向入口寄存器传播 demand。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummary.cpp:525` 在 `computeValueDemands()` 里识别 `xor x, x`，不再把 demand 传回两个输入，避免把清零前的寄存器值误标为真实 entry input。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2247` 在 partial demand 的 known mask 计算里把 `xor x, x` 视为零 mask。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2470` 在 partial demand 反向传播里遇到 `xor x, x` 直接停止，不再让 zero-demand rewrite 保留无意义输入。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4058` 新增 `testConsecutivePartialReadXorIsZeroedAfterSummarySSA()`，覆盖连续 partial read 被替换后 `xor` 折成零的场景。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4855` 把新增测试接入主测试序列。

## 实现判断

这一步不是完整的 ZMM range ABI 修复，只修正一个明确的需求传播错误。它减少了“清零动作被当成读取入口寄存器”的误判，也让后续 liveness 能删除更多 partial write。

复杂度评分：

- 实现效果：7/10。fortune 的 partial read 从 35 降到 33，partial write 从 13 降到 11，summary helper 从 30 降到 28。
- 理解成本：3/10。规则只针对 LLVM 里明确的 `xor x, x` 清零 idiom，没有引入新的状态。
- 维护成本：3/10。summary 和 SummarySSA 的两个 demand walker 都补同一条规则，后续若抽公共 bit-demand 逻辑可以统一。

## 验证

单测和构建：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target pcode_to_llvm_test native_register_summary_test \
  native_register_summary_ssa_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.19 user=7.16 sys=0.03 maxrss=170800`
- `partial_read_calls=33`
- `partial_write_calls=11`
- `summary_return_calls=1`
- `summary_clobber_calls=27`
- `register_access_metadata=45`
- `raw_load_all=6`，其中 5 个是 SummarySSA entry 脚手架 load，1 个是仍带寄存器访问的 `RDI` raw load
- `raw_store_global_register=2`
- `warning_lines=33`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-range-args-final-20260704064619`：partial read 从 35 降到 33，partial write 从 13 降到 11，summary return 从 2 降到 1，summary clobber 从 28 降到 27。运行时间从 `7.24s` 到 `7.19s`，没有看到性能退化。

## 后续

剩余主要集中在：

- `notdec_native_3eb0`：还有较多 partial read 和 summary helper，需要继续看 call effect / return helper 的 range 化。
- `notdec_native_32e0`：还有 ZMM0/ZMM1 partial write，下一步应继续做 partial write 的真正 range def。
- `notdec_native_5270`、`notdec_native_5040`：还残留少量 summary helper，需要结合签名 shape 和 internal call effect 继续收窄。

---

# 实现记录 2026-07-04 range return collection

## 已完成范围

本次把内部函数返回值收集从 whole-register 读取改成优先按签名 slot 的 bit range 读取。之前 call 参数已经走了 `readSlotRangeBefore()`，但 `collectFunctionReturnValues()` 仍直接 `readValueBefore()` 读取整个 register。对于 XMM/ZMM 低位返回，这会把低 64 位返回又拖回整 `i512`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3944` 把 `readSlotValueBefore()` 抽成基于任意 `Instruction` 的 helper，call 参数路径继续复用它。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3974` 把 `readSlotRangeBefore()` 改成接受任意插入点，用于 call 参数和 return 收集两处。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4179` 让 `collectFunctionReturnValues()` 用 `readSlotValueBefore(*ret, slot, ...)`，优先读取 slot range，再 fallback 到 whole-register。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3041` 新增 `testInternalSignatureRewriteUsesZmmLowLaneReturn()`，覆盖 callee 只写 `ZMM0[0:64]`、caller 只读 `ZMM0[0:64]` 时，内部函数返回类型应重写为 `double`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4953` 把新增测试接入主测试序列。

## 实现判断

这一步是 return 侧和 call 参数侧对齐，不做全局 full-load range 化，所以支配关系风险比较小。fortune 当前残留计数持平，说明这不是这批样例的主瓶颈，但它补上了 XMM/ZMM 低位返回的必要路径。

复杂度评分：

- 实现效果：6/10。补齐 return collection 的 range 读取，单测覆盖低位 ZMM 返回；fortune 指标持平。
- 理解成本：4/10。只把已有 slot range helper 泛化到 `Instruction` 插入点。
- 维护成本：4/10。call 参数和 return 收集复用同一条 helper，后续 ABI slot range 继续收窄时只需维护一处。

## 验证

单测和构建：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target pcode_to_llvm_test native_register_summary_test \
  native_register_summary_ssa_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-return-range-20260704072757/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-return-range-20260704072757`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.31 user=7.25 sys=0.05 maxrss=170008`
- `partial_read_calls=33`
- `partial_write_calls=11`
- `summary_return_calls=1`
- `summary_clobber_calls=27`
- `register_access_metadata=45`
- `raw_load_all=6`
- `raw_store_global_register=2`
- `warning_lines=33`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-xor-zero-20260704070732`：计数持平，运行时间从 `7.19s` 到 `7.31s`，属于同量级波动。

---

# 实现记录 2026-07-04 duplicate partial read xor cleanup

## 已完成范围

本次处理 fortune 里大量残留的相邻同 range `partial_read` 后再 `xor` 的模式。前面已经在 demand walker 里识别 `xor x, x = 0`，但这里两个输入仍是两个 helper call，range SSA 又会被前面的未知 call effect 卡住，所以原先无法合并成同一个 SSA value。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1852`、`external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1874`：在 partial read rewrite 和死 helper 删除之后调用 `foldDuplicatePartialReadXors()`。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2893`：新增 `foldDuplicatePartialReadXors()`，只折叠同一 basic block 内两个相同 `notdec.partial_read` helper 作为 `xor` 两边操作数的场景。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2944`：新增 `samePartialReadRange(...)`，要求 global、full width、read width 和 bit offset 完全一致。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2953`：新增 `hasInterveningWriteToPartialReadRange(...)`，两个 read 之间遇到同 register full store、overlap partial write 或普通 analyzable call 都不折叠。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4188`：新增 `testDuplicatePartialReadXorAfterUnknownCallIsZeroed()`，覆盖未知外部 call 后连续两次读同一 `R9[0:32]` 并 xor 的场景。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5047`：把新增测试接入主测试序列。

## 实现判断

这一步不是完整的 range SSA 重构，只是清掉一种明确恒等式残留。它仍然朝目标前进：减少 range SSA 被 unknown call 阻断后留下的 `partial_read` helper，并让后续 liveness 能继续删掉相关 partial write。

复杂度评分：

- 实现效果：8/10。fortune 中 `partial_read` 从 33 降到 3，`partial_write` 从 11 降到 0，`notdec.register.access` metadata 从 45 降到 3。
- 理解成本：5/10。新增的是一个很窄的 cleanup，规则清楚，但放在 SummarySSA 内会多一个局部 canonicalization 点。
- 维护成本：4/10。只依赖 partial read/write helper 解析和现有 register store 识别；后续如果 full range SSA 覆盖这个模式，可以删掉该 cleanup。

## 验证

构建和单测：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target native_register_summary_ssa_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.22 user=7.19 sys=0.02 maxrss=170032`
- `partial_read_calls=3`
- `partial_write_calls=0`
- `summary_return_calls=1`
- `summary_clobber_calls=27`
- `register_access_metadata=3`
- `raw_load_all=7`
- `raw_store_global_register=0`
- `warning_lines=33`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-return-range-20260704072757`：`partial_read` 从 33 降到 3，`partial_write` 从 11 降到 0，metadata 从 45 降到 3，运行时间从 `7.31s` 到 `7.22s`，没有看到性能退化。`raw_load_all` 从 6 到 7，主要是 `getenv` 后仍残留的 `RAX` full load 和几个 entry load，下一步应看签名重写后 full load 二次消除。

## 后续补充：删除 unused partial read helper

上面 cleanup 后，fortune 剩下 3 个 `partial_read` 都是没有 use 的 dead helper。它们因为不是 `rewritePartialReads()` 成功替换出来的 helper，旧的 `removeDeadReplacedPartialReads()` 不会处理。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1849`、`external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1873`：在 duplicate partial read xor fold 后统一调用 dead partial read 删除，避免先删 helper 后又遍历旧指针。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2883`：把删除逻辑改成 `removeDeadPartialReads()`，候选包括已替换 helper 和所有收集到的 partial read helper，只删除 `use_empty()` 的 call。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4097`：新增 `testDeadPartialReadHelperIsRemovedBySummarySSA()`，覆盖未知 call 后残留的 unused partial read helper。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5081`：把新增测试接入主测试序列。

验证：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target native_register_summary_ssa_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.41 user=7.38 sys=0.02 maxrss=170740`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=1`
- `summary_clobber_calls=27`
- `register_access_metadata=1`
- `raw_load_all=7`
- `raw_store_global_register=0`
- `warning_lines=33`

对比 duplicate partial read xor cleanup 后的 `/tmp/notdec-bin2llvm-fortune-dup-read-xor-final-20260704080807`：`partial_read` 从 3 降到 0，metadata 从 3 降到 1；运行时间从 `7.22s` 到 `7.41s`，仍在当前 fortune 波动范围内。剩余重点是签名重写后的 full register load 和 `summary_clobber`。

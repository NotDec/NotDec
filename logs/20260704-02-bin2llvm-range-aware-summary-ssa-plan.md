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

---

# 实现记录 2026-07-04 dead summary helper cleanup

## 已完成范围

fortune 剩余 `summary_clobber` 里大部分是 use-empty 的 synthetic helper call。它们只是 SummarySSA 为 call clobber 创建的未知值，占位值没有被后续 IR 使用时可以直接删除。之前只删 unused helper declaration，没有删 call 本身，所以 final IR 和 warning 文件里还残留大量无 use 的 `notdec.register.summary_clobber.i64()`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:896`：新增 `eraseDeadSummaryCallValueHelpers(...)`，扫描非 declaration 函数，只删除 use-empty、callee 名字为 `notdec.register.summary_*`，且带 `notdec.register.summary_ssa.call_value` metadata 的 call。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:5087`：在 SummarySSA residue cleanup 尾部、warning 收集前调用 dead helper 删除，并再次清理 unused summary helper declaration。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:4133`：新增 `testDeadSummaryCallValueHelperIsRemovedBySummarySSA()`，覆盖 dead helper 被删、仍被 store 使用的 helper 保留。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5133`：把新增测试接入主测试序列。

## 实现判断

这一步只删没有 use 的 synthetic helper，不改变仍被 PHI、store、return 使用的 clobber/return 值。它减少的是调试噪声和输出残留；真正还被使用的 `summary_clobber` 仍保留，后续需要继续分析其值流。

复杂度评分：

- 实现效果：7/10。fortune 中 `summary_clobber` 从 27 降到 5，warning 从 33 降到 11，`register_access_metadata` 从 1 降到 0。
- 理解成本：3/10。规则直接：SummarySSA 自己创建的 helper，use-empty 就删。
- 维护成本：3/10。只依赖 helper 名字和既有 metadata；如果后续 helper 机制改名，需要同步这一处。

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
  --summary-json-out /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.27 user=7.24 sys=0.02 maxrss=170848`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=1`
- `summary_clobber_calls=5`
- `register_access_metadata=0`
- `raw_load_all=5`
- `raw_store_global_register=0`
- `warning_lines=11`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-dead-partial-read-final-20260704081629`：`summary_clobber` 从 27 降到 5，warning 从 33 降到 11，`register_access_metadata` 从 1 降到 0；运行时间从 `7.41s` 到 `7.27s`，没有看到性能退化。剩余 5 个 `summary_clobber` 都仍有 use，不能按 dead helper 删除。

---

# 实现记录 2026-07-04 float entry fallback

## 已完成范围

fortune 剩下的 raw load 里有 4 个是 `ZMM2.entry` / `ZMM3.entry`。这些值不是 dead：父函数把入口 ZMM 值继续传给内部函数。问题是内部函数 shape 构造时，float/ZMM 输入必须有明确 bit demand mask 才会加入参数；如果 demand walker 没恢复出 lane mask，就会跳过参数，后续只能保留 `load @ZMM*`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1455`：在 `shapeForInternalFunction()` 的 float input 分支里，如果 `ReadEntry=true` 但 demand mask 缺失或为空，退回使用 backing register 类型作为内部函数参数，而不是保留 entry global load。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3041`：新增 `testPreservedZmmEntryIsPassedAsInternalArgument()`，覆盖父函数把入口 ZMM 传给内部子函数时，父函数应获得 `i512 %ZMM0.arg`，call 子函数也应直接使用这个参数。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5169`：把新增测试接入主测试序列。

## 实现判断

这一步没有解决“ZMM 高位是否真的需要”的问题；在没有 demand mask 时仍会退到完整 `i512`。但它避免了更差的结果：IR 中保留对 register global 的 entry load。后续如果 bit demand 能在 internal call 参数上传递得更细，这里自然可以退回 float lane 或窄整数参数。

复杂度评分：

- 实现效果：7/10。fortune 中 raw register load 从 5 降到 1，`partial_read` / `partial_write` 保持 0。
- 理解成本：4/10。规则只在内部函数 shape 构造处，且只处理 `ReadEntry=true` 的 float/ZMM 输入。
- 维护成本：4/10。后续完善 call-arg bit demand 后，可能会减少走这个 fallback 的场景。

## 验证

构建和单测：

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
  --summary-json-out /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.55 user=7.50 sys=0.04 maxrss=170864`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=1`
- `summary_clobber_calls=5`
- `register_access_metadata=0`
- `raw_load_all=1`
- `raw_store_global_register=0`
- `warning_lines=11`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-dead-summary-helper-20260704082318`：raw register load 从 5 降到 1；运行时间从 `7.27s` 到 `7.55s`，仍在当前 fortune 波动范围内。剩余 raw load 是 `notdec_native_4750` 的 `RAX.entry`，属于 integer output register 被函数入口读取，不能按 float fallback 处理。

---

# 实现记录 2026-07-04 read-entry return-register params

## 已完成范围

float entry fallback 后，fortune 只剩 1 个 raw register load：`notdec_native_4750` 里的 `RAX.entry`。这不是死代码；函数入口早退路径会把入口 RAX 拼进返回值。当前 internal signature shape 只允许 ABI input / unaffected register 作为参数，RAX 只在 ABI output 集合里，所以即使 `ReadEntry=true`，也无法变成参数，只能保留 `load @RAX`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1441`：`shapeForInternalFunction()` 的参数构造先看 summary fact 的 `ReadEntry`，然后允许 ABI input / unaffected register，或 ABI output register 进入内部参数 shape。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2963`：新增 `testInternalSignatureRewriteUsesReadEntryReturnRegisterArg()`，覆盖 RAX 作为 ABI output register 但被内部函数入口读取时，应重写成 `i64 %RAX.arg`，不再读 `@RAX`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:5210`：把新增测试接入主测试序列。

## 实现判断

这一步只影响内部函数。外部函数原型仍按 ABI 和原型库处理。对内部函数来说，只要 summary 明确 `ReadEntry=true`，这个寄存器就是函数输入；如果它同时属于 ABI output register，把它作为 internal 参数比保留全局寄存器 load 更直接。

复杂度评分：

- 实现效果：8/10。fortune 中 raw register load 从 1 降到 0。
- 理解成本：4/10。internal 参数集合从 input/unaffected 扩到 read-entry output register。
- 维护成本：4/10。后续如果 internal calling convention 单独建模，可以把这个判断挪到统一的 internal register class 规则里。

## 验证

构建和单测：

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
  --summary-json-out /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-return-reg-entry-param-20260704083954`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.49 user=7.47 sys=0.02 maxrss=169388`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=1`
- `summary_clobber_calls=5`
- `register_access_metadata=0`
- `raw_load_all=0`
- `raw_store_global_register=0`
- `warning_lines=11`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-float-entry-fallback-20260704083427`：raw register load 从 1 降到 0；运行时间从 `7.55s` 到 `7.49s`，没有看到性能退化。剩余问题集中在仍有 use 的 `summary_return` / `summary_clobber` helper。

---

# 实现记录 2026-07-04 full-range load dominance and late cleanup

## 已完成范围

本次继续处理 range-aware SummarySSA 后的 fortune 残留。上一版已经没有 `partial_read` / `partial_write`，但 full register load 和 late canary 形状仍会留下少量 raw register load：

- `FS_OFFSET.entry`：stack canary 比较里，saved canary 一侧被 range rewrite 变成 `and load, 0xffffffff`，旧 canary cleanup 只认比较两边都是直接 `load`。
- `RDI`：`notdec_native_5040` 已经被签名重写成带 `RDI.arg` 的内部函数，但函数体里一个原始 full register load 没在第一次 SummarySSA 中替掉，post-signature cleanup 也没有重新收集 load 并做 rewrite。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1907`：post-signature cleanup 阶段重新 `collectAccesses()` / `planRegisterRanges()`，然后执行 `rewriteLoads()` 和 `removeDeadReplacedLoads()`，让签名重写后仍可清理残留 full register load。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2861`：新增 `valueDominatesUse()`，full-range 拼接前检查 segment value 支配当前 use。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2881`：新增 `assembleRangeReadIfDominating()`，只在覆盖 segments 都支配当前 load/call 时拼接 full value，避免错误复用分支内局部值。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2925`：新增 `readFullRangeValueBefore()`，让 full register load 优先走 range SSA 拼接，失败时才回退到旧 whole-register SSA。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:2988`：`rewriteLoads()` 接入 full-range read。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:3526`、`3594`、`3629`、`3670`：range Braun SSA 读 entry/exit/PHI 时传入 dominator tree；unsafe incoming 会 materialize unknown，而不是留下不支配 use 的值。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4232`：call 参数 slot 读取也改用支配检查后的 range 拼接。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeStackCanaryCleanup.cpp:511`：新增 `savedCanaryLoadFromCompareOperand()`，只支持 `load` 或 `and load, 0xffffffff` 这两种 saved canary compare operand。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeStackCanaryCleanup.cpp:881`：canary predecessor 删除改用 saved-load wrapper 匹配，保留 FS canary 一侧必须是直接 load 的约束。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:384`：`createStackCanaryCheckFunction()` 增加 masked saved canary 构造。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3023`：新增 `testPostSignatureCleanupRewritesInternalEntryRawLoad()`，覆盖签名重写后仍要清理 raw entry register load 的场景。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3711`：新增 `testStackCanaryMaskedSavedLoadIsRemoved()`，覆盖 saved canary 被低 32 位 mask 包装时仍能删除 canary check。

## 实现判断

这次没有扩大 canary 识别到任意表达式，只补了 fortune 里实际出现的 `and load, low32_mask`。post-signature cleanup 也只补 SummarySSA 自己已有的 load rewrite，不引入新的 ABI 推断规则。

复杂度评分：

- 实现效果：8/10。fortune 的 raw register load 从 3 个 `FS_OFFSET.entry` 加 1 个 `RDI`，降到 0；`partial_read` / `partial_write` 继续保持 0。
- 理解成本：5/10。full-range read 多了 dominance guard，但仍沿用现有 range SSA 和 whole-register fallback。
- 维护成本：4/10。canary mask 匹配很窄；post-signature cleanup 复用既有 rewrite 流程，后续 whole-register SSA 移除时可以一起简化。

## 验证

构建和单测：

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
  --summary-json-out /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/fortune.bc \
  -o /tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547/fortune.verified.bc
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.78 user=7.75 sys=0.03 maxrss=170012`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=2`
- `summary_clobber_calls=6`
- `register_access_metadata=0`
- `raw_load_all=0`
- `raw_store_global_register=0`
- `warning_lines=11`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-full-range-skip-special-20260704093656`：`raw_load_all` 从 3 降到 0；对比 canary mask 修复后的 `/tmp/notdec-bin2llvm-fortune-canary-mask-20260704094422`：剩余 `RDI` raw load 也降到 0。运行时间从 `7.75s` 到 `7.78s`，没有看到性能退化。剩余问题集中在仍有 use 的 `summary_return` / `summary_clobber` helper 和 unknown external signature warning。

---

# 实现记录 2026-07-04 clobber argument filtering and entry argument cleanup

## 已完成范围

本次继续收敛 SummarySSA 后的残留 helper 和 raw register load。上一版 fortune 已经做到 `partial_read` / `partial_write` / `notdec.register.access` 为 0，但分析 `summary_clobber` 时发现两个问题：

- 外部调用后的 caller-clobbered RDX 可能被当成下一次未知外部调用的参数证据，导致未知签名推断偏大，或者把 `summary_clobber` helper 继续传下去。
- 内部函数返回值收集时，如果返回 slot 的值来自外部调用 clobber helper，会把这个 helper 暴露到调用者侧。
- post-signature cleanup 会重新通过 range entry 读取函数入口值；对于已经签名重写成 `%RDI.arg` 的函数，不应该再新建 `load @RDI`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:138`：新增 `bindingForIndex()`，签名重写按 ABI 参数 index 查绑定，避免 clobber 证据中断后把稀疏 binding 当成连续数组。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4018`：新增 `entryArgument()`，post-signature cleanup 时从当前函数签名里查对应 register 参数。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4036`：`entryRangeInput()` 在 `PostSignatureCleanup` 下优先从 `%REG.arg` 抽取 range；没有参数时才 fallback 到 entry global load。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4208`：`callArgStoreBindings()` 遇到依赖 `summary_clobber` 的值时停止收集参数证据，不再把 caller-clobbered 值当实参。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4464`：`collectFunctionReturnValues()` 遇到依赖 `summary_clobber` 的返回 slot 时改用 frozen poison，避免内部函数把外部 clobber helper 作为返回值暴露出去。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4956`：`rewriteSignatureShapes()` 使用 `bindingForIndex()`，缺失参数用 frozen poison 补位。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:339`：新增 `valueNameContains()` 测试辅助函数。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2516`：调整 RDX clobber 测试，让 RDX 被真实消费后再检查 warning，避免用死 load 期待 helper 残留。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2579`：新增 `testUnknownExternalClobberArgBecomesUnknown()`，覆盖 clobber-derived value 不作为下一次 unknown external 参数。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2656`：新增 `testInternalReturnDoesNotExposeExternalClobber()`，覆盖内部函数返回不暴露外部 clobber helper。

## 实现判断

这次没有试图直接删除所有 `summary_clobber`。仍然被真实使用的 clobber helper 保留并输出 warning；只阻断两类不应该传播的路径：未知外部参数证据、内部函数返回值。post-signature entry 修复也只在函数已经有签名参数时生效，不改变无签名函数的入口未知值表达。

复杂度评分：

- 实现效果：7/10。fortune 的 `raw_load_all` 和 SummarySSA entry load 回到 0，clobber-derived 参数/返回传播有单测覆盖；fortune 中 `summary_clobber_calls` 仍为 6，说明剩余 helper 是真实使用或还需要后续更细的 ABI/数据流判断。
- 理解成本：4/10。新增逻辑都贴在已有 call arg、return collection、entry range input 路径上，没有新增分析阶段。
- 维护成本：4/10。`entryArgument()` 依赖当前 SignatureShape 参数顺序，和签名重写已有逻辑一致；后续 whole-register fallback 删除时可以一起简化。

## 验证

构建和单测：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target native_register_summary_ssa_test pcode_to_llvm_test \
  native_register_summary_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.71 user=7.68 sys=0.02 maxrss=170640`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=2`
- `summary_clobber_calls=6`
- `register_access_metadata=0`
- `summary_entry_loads=0`
- `raw_load_all=0`
- `raw_store_global_register=0`
- `warning_lines=11`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-final-range-cleanup-20260704095547`：`raw_load_all` 保持 0；中间调试发现的 post-signature `RDI.entry` 在本次输出中消失，`summary_entry_loads=0`。运行时间从 `7.78s` 到 `7.71s`，没有看到性能退化。剩余问题仍是 `summary_return=2`、`summary_clobber=6`，需要后续结合具体 callsite 再判断是否能安全消除。

---

# 实现记录 2026-07-04 return clobber pruning

## 已完成范围

本次继续处理剩余 `summary_clobber`。分析 fortune 后发现，部分 RDX clobber 并不是后续真实参数使用，而是被内部函数返回 tuple 的 RDX slot 拉进来：为了构造返回值，SummarySSA 又去读取外部调用后的 RDX，从而 materialize `notdec.register.summary_clobber.i64()`。

具体改动：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4464`：`collectFunctionReturnValues()` 在读取返回 slot 前先判断该 slot 沿返回路径是否会读到 call clobber；会读到时直接用 frozen poison 表达 unknown 返回值，避免为了构造返回 tuple 生成 clobber helper。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4480`：新增 `returnSlotMayReadCallClobber()`，从 return block 沿 predecessor 回看目标 register range。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:4515`：新增 `blockMayClobberReturnRangesBefore()`，在单个 block 内遇到目标 register store 就停止，遇到会 clobber 目标 range 的 call 就判定该返回 slot 不应 materialize。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:2705`：新增 `testClobberReturnPhiDoesNotMaterializeHelper()`，覆盖返回 PHI 中一条路径来自 clobber、另一条路径是已知值时不保留 `summary_clobber` helper。

## 实现判断

这次仍然不是删除所有 clobber。真实进入后续普通调用参数的 clobber helper 继续保留并输出 warning；只裁掉“为了返回 tuple 读取 clobber”这一类中间产物。这个策略偏保守：如果某个返回 slot 沿路径可能读到 call clobber，就把该 slot 作为 unknown 返回值处理，不把外部调用 clobber 冒充成内部函数的稳定返回。

复杂度评分：

- 实现效果：7/10。fortune 中 `summary_clobber_calls` 从 6 降到 3，warning 从 11 行降到 8 行；raw register load/store 仍保持 0。
- 理解成本：5/10。新增了一个只服务 return collection 的小型反向检查，但范围局限在返回值收集阶段。
- 维护成本：4/10。逻辑复用现有 `callEffect()` 和 range planner；后续如果返回 demand 更精细，可以把这个检查合并到 range liveness。

## 验证

构建和单测：

```bash
cmake --build external/NotDec-bin2llvm/build \
  --target native_register_summary_ssa_test pcode_to_llvm_test \
  native_register_summary_test notdec-native-llvm -j4

external/NotDec-bin2llvm/build/bin/native_register_summary_ssa_test
external/NotDec-bin2llvm/build/bin/pcode_to_llvm_test
external/NotDec-bin2llvm/build/bin/native_register_summary_test
```

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-return-clobber-prune-clean-20260704103031`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=7.71 user=7.67 sys=0.03 maxrss=169904`
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_calls=2`
- `summary_clobber_calls=3`
- `register_access_metadata=0`
- `summary_entry_loads=0`
- `raw_load_all=0`
- `raw_store_global_register=0`
- `warning_lines=8`

对比上一阶段 `/tmp/notdec-bin2llvm-fortune-summaryssa-entry-arg-20260704101350`：`summary_clobber_calls` 从 6 降到 3，warning 从 11 行降到 8 行；`partial_read` / `partial_write` / raw register load/store 继续保持 0。运行时间从 `7.71s` 到 `7.71s`，没有看到性能退化。剩余 helper 为 indirect call 后的 `RAX.summary_return`，以及仍真实进入后续使用的两个 RDX clobber。

---

# 实现记录 2026-07-04 native eh-frame cold block folding

## 已完成范围

本次处理 fortune 里 `0x3470` 被跳过、并派生出 `notdec_native_3d98` declaration 的问题。根因不是寄存器 SSA，而是 `.eh_frame` 给出的非连续冷块被当成独立函数；冷块尾跳回 `0x3470` 内部后，lowering 又把跨 range 目标当成 native tail-call 函数。

具体改动：

- `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:4465`：`FlowFactNormalizer::run()` 在 split 后增加 decoded direct target block 导入，补齐已解码但不在 seed range 内的冷块。
- `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:4596`：`foldEhFrameOnlyBranchTargets()` 增加 eh-frame-only 冷函数尾跳回已确认函数内部块的折回逻辑。
- `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:4641`：新增 `restoreFoldedEhFrameFlowTargets()`，折回后恢复 direct/tail flow 的 CFG successor，并按 terminator 当前所属 block 写边，避免 split 后把旧 successor 写回错误 block。
- `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:4709`：新增 `ensureFunctionBlockStartsAt()`，折回后如果 owner 内部 direct target 落在 copied block 中间，先切出目标 block。
- `external/NotDec-bin2llvm/lib/NativeAnalysis.cpp:4792`：新增 `appendDecodedDirectTargetBlocks()` / `appendDecodedDirectTargetBlockAt()`，只导入 unowned decoded targets，避免 eh-frame-only 冷函数反向吸收主函数块。
- `external/NotDec-bin2llvm/tests/native_analysis_facts_test.cpp:233`：新增 `testFlowNormalizerFoldsEhFrameTailBackIntoOwner()`，覆盖 eh-frame 冷函数尾跳回 owner 内部块时折回 owner，并清掉 tail-flow metadata。
- `external/NotDec-bin2llvm/tests/native_analysis_facts_test.cpp:927`：新增 `testFlowNormalizerImportsDecodedColdDirectTarget()`，覆盖已解码冷 direct target 导入和 successor 保留。

同时保留本轮 SummarySSA 小修：

- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:342`：补 `__errno_location` 已知原型，使其返回值 materialize 为真实 i64 返回，而不是未知 helper。
- `external/NotDec-bin2llvm/lib/passes/summary/NativeRegisterSummarySSA.cpp:1682`：`mayDependOnSummaryClobberValue()` 递归追踪 `BinaryOperator`，避免 unknown external arity 把 clobber 参与的二元表达式当作实参证据。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:1616`：新增 `testKnownErrnoLocationReturnIsMaterialized()`。
- `external/NotDec-bin2llvm/tests/native_register_summary_ssa_test.cpp:3033`：新增 `testUnknownExternalArityStopsAtBinaryClobberArg()`。

## 实现判断

这次没有改 lowering 去容忍假的 `notdec_native_<addr>`，而是在 native facts 侧修正函数/块归属。这样 `0x27b2`、`0x27c0`、`0x27d6`、`0x27e3` 都成为 `0x3470` 的非连续 blocks，`0x3d98` 继续作为 `0x3470` 内部块存在，不再生成独立 declaration。

复杂度评分：

- 实现效果：8/10。fortune 中 `0x3470` 从 declaration/skip 恢复为 definition，`notdec_native_3d98` 消失，`llvm-as` 和 verifier 通过。
- 理解成本：6/10。主要复杂度在 eh-frame 冷块折回后还要恢复 CFG 边，并处理 copied block 再 split 的情况。
- 维护成本：5/10。逻辑仍限制在 `FlowFactNormalizer`，没有扩大到 lowering；后续如果 native discovery 有更正式的 block owner 模型，可以把这些 helper 收敛进去。

## 验证

构建和单测：

```bash
cd external/NotDec-bin2llvm

git diff --check
cmake --build build \
  --target native_analysis_facts_test pcode_to_llvm_test \
  native_register_summary_ssa_test native_register_summary_test \
  notdec-native-llvm -j4

build/bin/native_analysis_facts_test
build/bin/pcode_to_llvm_test
build/bin/native_register_summary_ssa_test
build/bin/native_register_summary_test
```

额外 facts 检查：

```bash
build/bin/notdec-native-discover --block-json 0x27c0 \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune
build/bin/notdec-native-discover --block-json 0x27d6 \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune
```

结果：

- `0x27c0` 属于 `0x3470`，block `[0x27c0, 0x27d6)`，successor 只有 `0x27d6`。
- `0x27d6` 属于 `0x3470`，block `[0x27d6, 0x27e3)`，successor `0x3596`。

fortune smoke：

```bash
external/NotDec-bin2llvm/build/bin/notdec-native-llvm \
  /sn640/NotDec-Exp/Bench2/rootfs/usr/games/fortune \
  --all-confirmed --skip-runtime \
  --summary-json-out /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/summary.json \
  --register-ssa-warning-out /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/register-ssa-warnings.txt \
  -o /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/fortune.ll

/sn640/NotDec/llvm-22.1.0.obj/bin/llvm-as \
  /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/fortune.ll \
  -o /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/fortune.bc

/sn640/NotDec/llvm-22.1.0.obj/bin/opt -passes=verify \
  /tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449/fortune.bc \
  -o /dev/null
```

结果：

- 输出目录：`/tmp/notdec-bin2llvm-fortune-cold-fold-20260704121449`
- `llvm-as` / `opt -passes=verify`：通过
- 时间：`seconds=8.83 user=8.80 sys=0.02 maxrss=171336`
- `notdec_native_3470`：生成 definition
- `notdec_native_3d98`：无残留 declaration/call
- `partial_read_calls=0`
- `partial_write_calls=0`
- `summary_return_refs=2`
- `summary_clobber_refs=0`
- `register_access_metadata=0`
- `raw_load_R=0`
- `raw_store_R=0`
- warnings 只剩 indirect call 的 `RAX.summary_return`，以及未知外部签名推断 warning。

对比上一轮成功 smoke `/tmp/notdec-bin2llvm-fortune-scoped-cold-target-20260704111647`：运行时间从 `10.15s` 到 `8.83s`，没有看到性能退化；`notdec_native_3d98` 从 declaration 消失，`summary_clobber_refs` 从 0 保持 0，raw register load/store 继续保持 0。

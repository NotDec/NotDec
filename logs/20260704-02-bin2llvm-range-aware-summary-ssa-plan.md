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

# 原始 prompt

> 写成一个新的规划文件，同时给出明确的计划完成条件
>
> 是的，Phi指令一定要按照这个方式处理，把我的原话放到logs/20260623-01-sailr-angr-completion-plan.md顶部。
> 当前应该优先按旧链路在结构恢复前 demote Phi，并维护 HType 到 demoted LLVM Value 的映射，使 structuring 算法不直接处理 Phi；随后完善 copied/virtual block 的 payload materialize、ReturnDuplicatorLow、switch deoptimization 和 Angr pass 对齐。

补一句当前要求：Phi 先在结构恢复前消掉，`HType` 继续跟 demoted LLVM Value 走，structuring 算法层不要直接处理 Phi。

# 背景

当前 SAILR 复刻已经过了搭框架阶段。shared structuring 的基础边界基本稳定：

- `OverlayManager` / `RegionOverlay`
- parent-visible graph
- checkpoint / rollback
- recursive structuring
- cyclic refinement
- virtual edge
- structuring trial
- goto quality guard
- optimization pipeline

Phoenix 已经迁到 shared reducer 路线。SAILR 已经作为 Phoenix 变体接入，并有 shared deoptimization pipeline。

已经落地的 SAILR deoptimization 子集包括：

- `ReturnDuplicatorLow`
- `CrossJumpReverter`
- `DuplicationReverter`
- `LoweredSwitchSimplifier`
- `SwitchDefaultCaseDuplicator`
- `SwitchReusedEntryRewriter`

最近几轮主要在补 shared CFG 的复制身份和回滚边界：

- copied / synthetic block 有独立 `BlockId`
- copied block 记录 `BodyBlock`、`Origin`、`SourceBlock`、`CopyKind`、`CreatedBy`
- copied block materialize 走 shared `StructuredCFG::materializeBlockBody()`
- deoptimization pass 统一成候选图提交
- copy helper 失败路径改成事务式回滚

现在的问题不是框架不够，而是 Angr SAILR 的完整语义还没补完。特别是 copied region 的 Phi / vvar / payload rewrite 还没有 shared 表达能力，所以不能继续靠 C 或 Solidity renderer 复用原 block body 来假装复制正确。

# 目标

把当前 SAILR 从“保守子集可跑”推进到“接近 Angr 完整语义”。

核心目标：

1. copied / virtual block 的身份、payload、successor / predecessor、switch case、goto source-target 都在 shared CFG / shared structured tree 里表达。
2. `ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、switch default / reused-entry 相关 pass 尽量对齐 Angr。
3. 所有 pass 都统一接入 `StructuringOptimizationPipeline`，用 shared structuring trial 和 quality guard 决定是否接受。
4. C 和 Solidity 后端只消费 shared 结果，不参与 structuring 算法决策。

非目标：

- 不为了短期输出好看，在 C renderer 或 Solidity renderer 里加 copied block 特判。
- 不在 Phi / vvar / payload 语义不清楚时扩大复杂 region copy。
- 不用 renderer fallback 掩盖 structuring 失败。

# 路线

## 阶段一：补 shared payload materialize 能力

先把 `StructuredCFG::materializeBlockBody()` 从简单复制 payload id，推进成 copied block payload rewrite 的统一入口。

要做：

- 定义 materialize context，至少包含 source block、copy block、original predecessor、new predecessor、copy kind、created-by pass。
- 明确 statement、branch condition、switch condition、switch case value 的 materialize 规则。
- 支持 payload clone / rewrite hook，但不把具体 C / Solidity AST 类型放进 structuring 层。
- 保证 materialize 不覆盖 copied block 的 CFG 身份：successor、predecessor、switch target、goto target 仍按 copy block 维护。

判断标准：

- copied block 的 body payload 可以按 copy 身份生成新 payload。
- 复制后 case target / default target / goto target 不退回原 block。
- C 和 Solidity 后端看到同一份 shared materialize 结果。

## 阶段二：补 Phi / vvar rewrite 的 shared 表达

这是扩 `ReturnDuplicatorLow` 的前置条件。

要做：

- 梳理当前 payload 里 Phi、recovered value、virtual variable 的表示方式。
- 如果 payload 仍是 opaque id，就给 structuring 层提供 backend-neutral rewrite callback。
- copied region 进入不同 predecessor component 时，可以生成不同 payload。
- 删除 original region 或 redirect predecessor 后，可以更新剩余 Phi incoming。

判断标准：

- 能构造一个最小分支 return-region：两个 predecessor 进入同一个 return tail，复制后两个 copy 的值来源不同。
- 这个差异由 shared materialize / rewrite 表达，不由 renderer 猜。
- 回滚失败时不会泄露已经 clone 的 payload 或半改 CFG。

## 阶段三：完整化 ReturnDuplicatorLow

在 payload rewrite 能力足够后，再扩 Angr `_single_entry_region()` 和相关 return duplication 语义。

要做：

- 补一般 branch return-region。
- 补 connected in-edge component 的完整分组逻辑。
- 补 copied region 内部 edge / external edge 的完整重写。
- 删除 original region 前继续使用候选图和 remove probe。
- 所有改图都由 optimization wrapper 做 trial / quality guard。

判断标准：

- `ReturnDuplicatorLow` 不再只覆盖 straight-line / terminal fork / goto tail 子集。
- 新增形状有 Phi / payload rewrite 测试。
- 未覆盖形状明确跳过，不产生半改图。

## 阶段四：完整化 switch deoptimization

优先顺序：

1. `LoweredSwitchSimplifier`
2. `SwitchDefaultCaseDuplicator`
3. `SwitchReusedEntryRewriter`

要做：

- 支持更长 case/default tail。
- 支持 case/default 交叉复用。
- 对齐 Angr reused-entry 中 virtual goto 相关语义。
- 继续区分 case target 和 default target，避免 default-only 误进 reused-entry。

判断标准：

- switch case value、case target、default target 在复制和重定向后都稳定。
- default forwarder / copied case region 是 shared CFG 的显式节点。
- 不需要 renderer 侧判断“这是 switch deopt 复制出来的块”。

## 阶段五：对齐 pass 顺序和 options

当前 pipeline 已经可跑，但还要继续和 Angr 对照。

要做：

- 对齐 pass 顺序。
- 对齐每个 pass 的默认 options。
- 对齐 block limit、max iteration、require gotos、prevent new gotos、strictly less gotos、relative quality guard。
- 每个差异都写清楚原因。

判断标准：

- `buildSAILRDeoptimizationPipeline()` 里的每个 pass 都能对应到 Angr 的类 / 方法。
- NotDec 保留的差异都能解释为 IR / payload 表达差异，而不是漏实现。

## 阶段六：真实样例分类

不要直接用大 corpus 判断成功。先选小批样例做分类。

分类维度：

- SAILR 比 Phoenix 好。
- SAILR 比 Phoenix 差。
- SAILR pass 没触发。
- SAILR trial 拒绝。
- copied block / return duplication / switch deoptimization 相关。

每个差异归因到：

- Angr 语义未复刻。
- shared CFG / payload 表达能力不足。
- renderer 能力不足，但算法结果已经正确。

判断标准：

- 能解释主要差异来源。
- 不用 renderer fallback 掩盖 structuring 失败。
- fortune 或更合适样例没有明显性能退化。

# 风险

最大风险仍然是 Phi / vvar / payload rewrite。

如果 copied block 只复用原 block body，小测试会容易通过，但复制后的值来源可能已经错了。这个问题必须在 shared 层解决。

第二个风险是 reused-entry 的语义取舍。当前 NotDec 有些实现用复制 entry region，Angr 有些路径更偏 virtual goto。这里如果影响输出质量，需要单独记录对比，不能静默改。

第三个风险是性能。SAILR deoptimization 会增加复制、trial 和 rollback。扩大 pass 前后要继续用同口径样例看耗时。

# 明确完成条件

只有同时满足下面条件，才算“接近 Angr 完整语义”的计划完成。

## 语义完成条件

1. copied / virtual block 在 `StructuredCFG` 中都有独立 `BlockId`。
2. copied block 明确记录 source、body-source、copy kind、created-by pass。
3. copied block 的 statements、condition、switch case value 可以通过 shared materialize / rewrite 生成。
4. copied block 的 successor、predecessor、switch case target、goto source-target 都按 copied `BlockId` 维护。
5. Phi / vvar / payload rewrite 不依赖 C 或 Solidity renderer。
6. `ReturnDuplicatorLow` 覆盖 Angr 主要 return duplication 形状，包括一般 branch return-region。
7. `CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、switch default / reused-entry 相关 pass 的主要 Angr 语义都落到 shared CFG。
8. 每个 pass 都通过 `StructuringOptimizationPipeline` 的 trial / quality guard 接受，不直接绕过 shared guard。

## 测试完成条件

1. `structuring-analysis-test` 覆盖 copied payload rewrite、Phi / vvar rewrite、return duplication、cross jump、duplication revert、lowered switch、default reuse、reused-entry。
2. 每个新增复杂形状至少有一个失败回滚测试。
3. C 和 Solidity 后端至少各有一个 copied / virtual block 渲染 smoke，证明它们消费同一份 shared tree。
4. CTest structuring subset 通过：

```bash
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

5. fortune 或同等级真实样例 smoke 通过，并记录同口径耗时。

## 对照完成条件

1. 日志中列出每个 SAILR deoptimization pass 对应的 Angr 类 / 方法。
2. 对每个和 Angr 不一致的行为，写清楚是 NotDec IR 差异、payload 表达不足、还是刻意保守。
3. 对不能复刻的语义点，有明确的暂停记录和后续方案。

## 工程完成条件

1. 本计划里列出的 shared 语义、对照关系和测试项都已经落到代码或测试里，不再只停留在描述。
2. `demoteSSAFixHT()`、`StructuredCFG::materializeBlockBody()`、`ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、`SwitchDefaultCaseDuplicator`、`SwitchReusedEntryRewriter` 都有对应的 shared 测试。
3. `structuring-analysis-test` 和 `sailr-bench2-migration` 都能在当前工作树里跑通。
4. 真实样例和 scaffold 样例都被清楚区分，日志里不把 scaffold 当成 Angr 原始资产。
5. 顶层 `logs/20260623-01-sailr-angr-completion-plan.md` 保持最新状态记录，后续每次改动都补具体文件、函数和验证结果。

## 2026-06-24 当前状态

现在 shared structuring 的骨架和主要 SAILR pass 已经接上了，Phi 也按旧链路在结构恢复前 demote 掉了，HType 迁移到 demoted LLVM Value 的边界是通的。`StructuredCFG` 里的 copied / synthetic block 身份、payload materialize、return duplication、cross jump、duplication revert、lowered switch 和 pipeline 顺序也都有测试。`run_structuring_smoke.py` 里也补了一个直接看 Phi demote 前置边界的 smoke case，确认 structuring 看到的是 demoted 结果，不是 Phi。

还没到“接近 Angr 完整语义”的地方主要有两件事：

1. `run_sailr_bench2_migration.py` 里的 Angr 测试迁移还没有全变成真实原始资产，部分 case 仍然是 scaffold。
2. 真实样例覆盖面还不够宽，尤其是更复杂的 switch / duplication 形状，还在受旧 intrinsic 和 CFG 断言影响，不能直接当成最终对照基线。

所以现在更像是 shared 语义已经落稳，后面继续补迁移覆盖和样例分类，不是再重搭框架。下一步更值钱的是继续把 `ReturnDuplicatorLow` 和 switch deopt 的 payload rewrite 边界补像 Angr，然后再继续换掉 scaffold。

## 2026-06-24 实现记录：迁移脚手架继续补齐

这轮没有去碰 shared structuring 算法，只继续补 `run_sailr_bench2_migration.py` 里的迁移脚手架。新增了两个更简单的 proxy，分别钉住 `DuplicationReverter` 的直线无 goto 形状和一个 root cycle 形状，同时把已有 nested switch proxy 保留下来，继续作为 scaffold 而不是 Angr 原始资产。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:134`
  新增 `build_spec_list_proxy`，用来覆盖 `DuplicationReverter` 的直线无 goto 场景。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:185`
  新增 `root_cycle_follow_proxy`，用来钉住最小循环结构。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:284`
  保留 `nested_switch_proxy`，继续作为 scaffold。

验证：

```bash
python3 test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

## 2026-06-24 实现记录：Phi demote 前置边界补 smoke

这轮没有再改 demote 本身，只是在 structuring smoke 里补了一个最小分支 return case，专门确认结构恢复阶段看到的是已经 demote 过的结果，而不是直接处理 Phi。这个 case 和现有 `phi-demote` / `phi-demote-htypes` 一起，把“Phi 先在结构恢复前消掉”这条边界再钉紧了一点。

## 2026-06-25 当前状态

现在 shared structuring 的主干已经基本收口，Phi 前置 demote 和 `HType` 映射也已经在旧链路里打通，structuring 算法不再直接碰 Phi。`StructuredCFG` 的 copied / synthetic block 身份、payload materialize、`ReturnDuplicatorLow`、`CrossJumpReverter`、`DuplicationReverter`、`LoweredSwitchSimplifier`、`SwitchDefaultCaseDuplicator`、`SwitchReusedEntryRewriter` 和 pipeline 顺序都已经落到 shared 层测试里。

还没完全到 Angr 完整语义的地方，主要还是迁移覆盖：

1. `run_sailr_bench2_migration.py` 里还有 scaffold proxy，没有全部换成真实原始资产。
2. 更复杂的 switch / return / duplication 原始样例还不够多，现阶段更多是在确认 shared 语义边界，不是在补新的算法框架。
3. 现在更像是 shared 语义已经稳定，下一步要继续补真实样例对照和剩余 Angr 测试迁移，而不是重搭结构恢复框架。

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:165`
  新增 `phi_demote_before_structuring`。

验证：

```bash
ctest --test-dir build -R 'structuring-smoke|phi-demote|phi-demote-htypes' --output-on-failure
```

结果：待跑。

## 2026-06-24 实现记录：Phi demote 的类型残留再收紧

这轮只补了一个小的 shared 断言，继续把 Phi 从结构恢复前的语义边界里拿掉。现在
`demoteSSAFixHT()` 不只要在 demote 前把 Phi 的 HType 迁到 demoted value 上，还要保证写回
的类型表里不会再保留 Phi 自己的类型残留；也就是说，后续 structuring 只看 demoted 结果，
不再把 Phi 当作仍可直接消费的值。

- `external/NotDec-llvm2c/test/phi_demote_test.cpp:89`
  新增 `testDemoteSSAFixHTDropsPhiRefsFromWrittenTypes()`。

验证：

```bash
cmake --build build --target phi-demote-test -j4
python3 test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

## 2026-06-24 迁移边界：真实小样例能跑，但还不够替代 scaffold

这轮又试了几个真实 Bench2 小样例：

- `libuv/1-uv__cancelled.ll`
- `memcached/1-drive_machine.lto_priv.0.cold.ll`
- `wolfssl/1-wc_PKCS7_DecodeEncryptedData.cold.ll`

它们都能在 `structured-sailr` 下跑完，但输出太简单，不能替代
`switch_reuse_proxy`、`duplication_reverter_proxy` 或 `nested_switch_proxy` 这些更明确的
语义脚手架。也就是说，当前问题不是 shared structuring 跑不动，而是真实样例还没找到
足够接近 Angr 目标 case 的小输入。

结论先记在这里：这几个真实样例可以保留作 smoke，但不能冒充 SAILR 迁移的主对照。

## 2026-06-24 实现记录：Phi demote 的 type 容器再收口

这轮只补了 `HTypeResult` 的两个统一擦除接口，并把 `phi_demote_test` 的断言继续收紧。
现在 `demoteSSAFixHT()` 相关逻辑不只是把 Phi 的 `hasValueType()` 清掉，还能确认
`ValueTypesLower` / `ValueTypesUpper` / `ContraVariantValues` 里都不会再残留旧 Phi 条目。
这条边界仍然是结构恢复前先 demote Phi，不让 structuring 算法直接碰 Phi。

- `external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h:68`
  增加 `eraseValueTypesIf()` 和 `eraseContraVariantValuesIf()`。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:91`
  `testDemoteSSAFixHTKeepsUnnamedPhiTypes()` 继续收紧对 Phi 残留的断言。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:193`
  `testDemoteSSAFixHTDropsPhiRefsFromWrittenTypes()` 继续确认写回后没有 Phi 类型残留。

验证：

```bash
cmake --build build --target phi-demote-test -j4
cmake --build build --target structuring-analysis-test -j4
```

结果：通过。

## 2026-06-24 当前收口判断

这一轮重新核了迁移脚手架和真实 Bench2 smoke。结果很一致：

- `libuv/1-uv__cancelled.ll`
- `memcached/1-drive_machine.lto_priv.0.cold.ll`
- `wolfssl/1-wc_PKCS7_DecodeEncryptedData.cold.ll`

这些真实样例都能跑完，说明 shared structuring 没坏，但输出太短，不能替掉
`switch_reuse_proxy`、`duplication_reverter_proxy`、`nested_switch_proxy` 这些仍然更像 Angr
目标语义的脚手架。

所以这轮先不再继续找“能一把替掉所有 proxy”的样例了。当前更像是：
shared 语义边界已经稳定，迁移脚手架也已经把主要 Angr 语义点覆盖住，但还缺一批更接近
目标测试的真实输入，才能把 scaffold 逐个换掉。

## 2026-06-24 实现记录：把一个 switch scaffold 换成真实 fixture

这轮不再拿 `switch_case_recovery_proxy` 做纯内联 proxy，而是从 Bench2 里的真实
`switch.c.ll` 抽了一个本地 fixture 出来，仍然保留同样的 switch recovery 语义，
但输入来源变成真实 LLVM IR 片段。这样脚本里至少有一条 case 不再是手写 proxy，而是
来自 Bench2 的真实样本切片。

- `external/NotDec-llvm2c/test/structuring/fixtures/switch_case_recovery.ll`
  新增真实 switch recovery fixture。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:277`
  `switch_case_recovery_proxy` 改成真实 fixture 输入，语义说明也改成 real sample。

验证：

```bash
python3 test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

## 2026-06-24 实现记录：HTypeResult 的 demoted value 清理接口补测试

这轮把 `HTypeResult::eraseTypesForDemotedValues()` 再补了一层测试，确认它不只对普通
LLVM `Value*` 生效，也能把 `UConstant`、`StackObject` 和 `HeapObject` 这几类间接引用
一起清掉。这样前置 demote Phi 时，清理逻辑就不会在不同调用点各自漏一类引用。

- `external/NotDec-llvm2c/test/phi_demote_test.cpp:198`
  新增 `testHTypeResultErasesTypesForDemotedValues()`。

验证：

```bash
cmake --build build --target phi-demote-test -j4
```

结果：通过。

## 2026-06-24 迁移边界：真实 module-all 级样例仍被旧 intrinsic 挡住

这轮又试了几个更像“真实整模块”的 Bench2 样例：

- `selected-targets-native/libicu/common-library/module-all.ll`
- `selected-targets-native/wolfssl/shared-library/module-all.ll`
- `selected-targets-native/memcached/executable/module-all.ll`

它们都还会先撞旧的 `SAContext::getIntrinsic(...): unhandled intrinsic.` 断言，说明
整模块级真实样例现在仍然不适合作为 SAILR 迁移主对照。相比之下，前面从
`switch.c.ll` 抽出来的本地 switch fixture 是可以稳定替换一个 proxy 的，所以当前更合理的
路线是继续按“真实片段 fixture”推进，而不是硬上 module-all。

## 2026-06-24 实现记录：Phi demote 清理逻辑收敛到共享容器接口

这轮只把 `demoteSSAFixHT()` 里对 Phi 残留的擦除，收敛成 `HTypeResult` 自己的共享接口，
没有改 demote 的语义本身。这样后面如果还有别的前置清理逻辑要复用同样的过滤规则，
就不用在 `StructuralAnalysis.cpp` 里各写一遍循环了。

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1563`
  改成调用 `HTypeResult::eraseValueTypesIf()` 和
  `HTypeResult::eraseContraVariantValuesIf()`。

验证：

```bash
cmake --build build --target phi-demote-test -j4
```

结果：通过。

## 2026-06-24 实现记录：phi-demote 测试修正为可编译的 shared 容器样例

这轮只修了 `phi_demote_test.cpp` 里的测试构造方式，没有改 shared 语义本身。把 `HTypeResult`
那条按 demoted value 清理类型残留的测试，改成直接用 LLVM 的 `Function` / `Call` / `insert`
写法，避免测试代码里再卡在构造器重载和 initializer_list 推导上。

- `external/NotDec-llvm2c/test/phi_demote_test.cpp:196`
  `testHTypeResultErasesTypesForDemotedValues()` 改成更直接的 LLVM 构造方式。

验证：

```bash
cmake --build ./build --target phi-demote-test -j4
ctest --test-dir build -R 'phi-demote|phi-demote-htypes|structuring-analysis' --output-on-failure
```

结果：通过。

## 2026-06-24 实现记录：按 demoted value 过滤的 helper 再收口

这轮再把 Phi 清理里“判断一个类型条目是否引用了已 demote 的值”这一段，收成
`HTypeResult::eraseTypesForDemotedValues()`。这样 `demoteSSAFixHT()` 只需要提供 demoted
value 集合，具体怎么从 `ExtValuePtr` 里找引用关系留在共享类型容器里。语义没变，还是
结构恢复前先清掉 Phi / 已 demote value 的类型残留。

- `external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h:95`
  新增 `eraseTypesForDemotedValues()`。

验证：

```bash
cmake --build build --target phi-demote-test -j4
```

结果：通过。

1. 算法层不包含 C renderer / Solidity renderer 特判。
2. renderer 不承担 structuring fallback 语义。
3. 所有 copied region 改图都具备候选图提交或事务式回滚。
4. 涉及 submodule 的改动先在 `external/NotDec-llvm2c` 内提交，再提交顶层指针和日志。
5. 顶层工作树除任务外的既有脏状态不被误改。

# 下一步建议

下一步不要先扩更多 matcher。先做一个最小 payload rewrite 设计和测试：

1. 构造一个 copied block 需要按 predecessor 生成不同 payload 的 shared 单测。
2. 给 `materializeBlockBody()` 增加 backend-neutral rewrite hook 或 materialize context。
3. 用这个能力补一个小的 branch return-region。
4. 再继续扩 `ReturnDuplicatorLow`。

这条路比直接堆 case 更慢一点，但更接近 Angr 的真实语义，也不会把 renderer fallback 当成算法成功。

# 2026-06-23 实现记录：阶段一 payload materialize 入口

本次完成了阶段一的 shared payload materialize 入口，并用一个受 hook 保护的 branch
return-region 测试验证扩展方向。总计划还没有完成，Phi / vvar rewrite、完整 Angr
pass 对照和真实样例分类仍需后续继续做。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:62`
  新增 `PayloadMaterializeKind`、`PayloadMaterializeContext` 和
  `PayloadMaterializeHook`。context 记录 source block、body block、copy block、
  original/new predecessor、copy kind、created-by pass，避免 structuring 层依赖
  C 或 Solidity AST 类型。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:145`
  给 `StructuredCFG` 增加 `setPayloadMaterializeHook()`、
  `hasPayloadMaterializeHook()` 和带 predecessor context 的
  `materializeBlockBody()` overload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:142`
  实现 hook 注册和查询。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:162`
  保留无 hook 快路径，生产路径行为不变；有 hook 时分别重写 statements、
  branch condition、switch case value，全部写入临时 buffer，成功后才提交，失败不留下半改 payload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:300`
  修正 `duplicateRegion()` 失败回滚：只删除本次新建 copy block，不再用原图里
  已经被引用的 copy id 去误删别的边。
  
# 2026-06-24 实现记录：switch case / default 复用拆分

本次补了 shared 层的 switch 复用语义，目标是让 case 复用和 default 复用在
StructuredCFG 里分开处理，不把 renderer fallback 当成算法结果。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增只重定向 switch case 的复制 helper，并把 `LoweredSwitchSimplifier`
  调整成只复制 case predecessor 进入的共享 case region。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  收紧原 region 删除条件：只要 switch 还有 default / 普通 successor 继续指向
  原目标，就保留原 region，不把 default 语义挪到 copy 上。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增混合 case/default 复用测试，确认复制 case region 后 default 仍保留在原目标，
  同时 copied block 继续保持独立 `BlockId` 和 `BodyBlock`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-lowered-switch-default-case.c --tr-level=2 --algo=structured-sailr`

## 结论

目前 shared structuring 还没有到 Angr 全量语义，但 copied / virtual block 的身份、
payload materialize、branch return-region、switch case/default 复用边界已经比前一版更清楚。
下一步还是继续补 Phi / vvar 的 shared rewrite 表达，再往 `ReturnDuplicatorLow`
和更复杂的 switch reuse 走。
  `removeBlock()` 逐个删，避免 copy id 已经出现在原图边里时误改原图边。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:189`
  新增 `collectClosedLinearReturnTail()` 和 `prependBranchReturnRegion()`，
  识别一个保守的一般 branch return-region：一个分支继续走原 return head，
  另一个分支是闭合线性 return tail。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1110`
  branch return-region 只在 `Graph.hasPayloadMaterializeHook()` 为真时启用。
  这样当前生产后端没有真实 payload rewrite 时不会扩大复制范围。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:500`
  和 `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:657`
  copy helper 调用新的 `materializeBlockBody()` overload，给 payload rewrite
  传入 predecessor context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:758`
  新增 copied payload rewrite 测试，覆盖 statements、condition、switch case value。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:819`
  新增 rewrite 失败原子性测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2203`
  新增带 payload rewrite hook 的 `ReturnDuplicatorLow` branch return-region 复制测试。

## 验证

构建和单测：

```bash
cmake --build ./build --target structuring-analysis-test notdec -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

结果：全部通过，CTest structuring subset 为 5/5 通过。

性能 smoke 使用同一个 fortune 用例：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-sailr-materialize-branch-fastpath.c \
  --tr-level=2 --algo=structured-sailr
```

本次改动后：`elapsed=196.55 user=218.80 sys=1.78 maxrss=1266720`。

stash 掉 `external/NotDec-llvm2c` 改动并重建后的同口径 baseline：
`elapsed=200.10 user=222.16 sys=1.67 maxrss=1264960`。

结论：这次 shared materialize 入口没有可见性能退化。当前环境下
`structured-sailr` fortune baseline 约 200 秒，和旧日志里的 84.64 秒不是同一口径。

## 当前判断

实现效果：7/10。阶段一的入口和回滚边界已经落到 shared CFG，测试覆盖了 payload
rewrite、失败原子性和一个 branch return-region 形状；但还没有接真实 C/Solidity payload
clone，也没有完成 Phi / vvar rewrite。

复杂度：6/10。新增 hook 和 context 会增加理解成本，但它把复制 payload 的问题放在
`StructuredCFG::materializeBlockBody()` 一个入口里，比在 renderer 里分散特判更可控。

维护成本：6/10。无 hook 快路径保持旧行为，branch return-region 也被 hook gate 保护；
后续主要维护点是定义真实 payload clone 的所有权和失败清理规则。

更好的方案暂时没有明显成立。直接扩大 return-region copy 会绕过 payload 语义，风险更高；
把 clone 逻辑放进 C/Solidity renderer 会让两个后端各自背 structuring 语义，也不符合目标。

# 2026-06-24 实现记录：copy-of-copy 的 switch payload identity 保持

这次继续收紧 shared payload materialize 的边界，钉住 copy-of-copy 的 switch 语义。
当前 `StructuredCFG` 已经能给 copied block 提供 predecessor-aware rewrite context，
但还缺一个明确测试来证明：二次复制仍然沿用原始 body source，`CopiedFromBlock`
只记录这次直接复制来源，switch case value 的 rewrite 仍然可以看到原始 source /
body / copy 身份三层信息。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1562`
  新增 `testStructuredCFGMaterializeCopyOfCopyKeepsSwitchIdentity()`，覆盖 switch block
  的 copy-of-copy 场景，确认 `SourceBlock == 10`、`CopiedFromBlock == 13`，
  `BodyBlock` 在 materialize 前后都按当前 copy 收口。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7608`
  把新测试接入 `structuring-analysis-test` 主入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：6/10。copy-of-copy 的 switch payload 身份更稳了，但这仍然是 shared CFG
层的 identity 测试，不是新的 deoptimization 语义。

复杂度：1/10。只是补了一条更硬的回归测试。

维护成本：1/10。后续如果 payload context 再扩字段，这条测试会继续帮忙钉住 copy 链条。

# 2026-06-24 实现记录：synthetic goto 的 shared identity 保持

这次补的是虚拟边界里的另一种 synthetic block。前面已经有 synthetic forwarder 的共享
身份测试，但 synthetic goto 只有渲染侧覆盖，没有单独钉住 copy / materialize 之后的
shared identity。现在补一条测试，确认 synthetic goto 复制后仍然保留 `SyntheticSource`
和 `SyntheticTarget`，`materializeBlockBody()` 也只收口它自己的 payload，不把这个虚拟跳转
变成 renderer 侧特判。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1923`
  新增 `testStructuredCFGDuplicateSyntheticGotoReportsTargets()`，覆盖 synthetic goto
  的复制和 materialize 边界。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7705`
  把新测试接入 `structuring-analysis-test` 主入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：5/10。synthetic goto 的边界更清楚了，但仍然只是 shared identity 收口，
还没有推进新的 deoptimization 语义。

复杂度：1/10。只加测试，不改算法。

维护成本：1/10。以后看虚拟跳转时，能直接对照 synthetic goto 的来源和去向。

# 2026-06-24 实现记录：synthetic block payload context 暴露来源和去向

这次补的是 shared payload materialize 的另一层信息：synthetic forwarder / goto
本身已经是 shared CFG 的显式块，但 materialize hook 之前只能看到 `CopyBlock` 和
`BodyBlock`，看不到虚拟边的原始来源和去向。现在 `PayloadMaterializeContext`
也带上 `SyntheticSource` / `SyntheticTarget`，这样后续做虚拟块 payload rewrite 时，
不需要再反查块本身就能知道这条虚拟跳转从哪来、到哪去。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:78`
  `PayloadMaterializeContext` 新增 `SyntheticSource` 和 `SyntheticTarget`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:255`
  `materializeBlockBodyImpl()` 现在把 block 自身的 synthetic 边身份带进 context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1897`
  `testStructuredCFGDuplicateSyntheticForwarderReportsTargets()` 补了 synthetic 边身份断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1932`
  `testStructuredCFGDuplicateSyntheticGotoReportsTargets()` 补了 synthetic goto 的 context 断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：5/10。synthetic block 的 payload context 更完整了，但这还是共享身份补强，
不等于新的 deoptimization pass 语义。

复杂度：1/10。只是把已存在字段带进 context。

维护成本：1/10。以后虚拟块 payload rewrite 出问题时，hook 里能直接看到 source / target。

# 2026-06-24 实现记录：Bench2 SAILR 迁移回归接入 CTest

这次把 Angr 侧 SAILR 测试迁移这件事往前推进了一步，不再只停留在日志里的目标描述。
现在先用 Bench2 里两组稳定的 LLVM IR 样例做 file-based 回归，直接跑 `structured-sailr`
并检查最基础的输出形状。这样至少先把“这些样例在 NotDec 的 shared structuring 链路里
能跑通”钉住，后面再继续往 Angr 测试名和更细断言对齐。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增一个独立迁移脚本，先接了两组样例：
  `hexx64/function-0x1156e0/native/function-0x1156e0.ll` 和
  `python/one-_PyPegen_fill_token.cold.ll`。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:21`
  把这个脚本接入 `ctest`，新增 `sailr-bench2-migration`。

## 验证

- `python3 test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
- `ctest --test-dir /sn640/NotDec/build -R sailr-bench2-migration --output-on-failure`

结果：通过。

## 当前判断

实现效果：4/10。迁移回归已经接上，但现在还是先用 Bench2 的稳定样例做入口，
还没做到 angr 测试名、输入形状、断言语义的逐项对齐。

复杂度：2/10。只是加了一个独立脚本和一个 CTest 条目，没有动 structuring 算法。

维护成本：2/10。后面可以继续往这个脚本里补更多 SAILR 相关样例，再逐步把
angr 侧测试名和本地样例一一对应起来。

# 2026-06-24 实现记录：Bench2 迁移回归补 Angr 测试映射

这次把之前那组 Bench2 迁移回归再往前挪了一步，不再只是“文件能跑”，而是给每个
样例补了对应的 Angr 测试名和语义标签。这样后续继续迁移 SAILR 测试时，脚本本身就能
看出这条样例是在对照哪个 Angr case，而不是只靠人为记忆。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:9`
  给三个样例分别补了 `angr_test` 和 `semantic` 字段。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:61`
  失败信息现在会带上样例名、Angr 测试名和语义标签，方便后面补更多映射。

## 验证

- `python3 test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c`
- `ctest --test-dir /sn640/NotDec/build -R sailr-bench2-migration --output-on-failure`

结果：通过。

## 当前判断

实现效果：5/10。迁移回归已经不只是匿名样例，但现在还只是三组最基础的语义映射，
还没真的把 angr 的 SAILR 测试逐个迁完。

复杂度：2/10。只是给现有脚本加了元数据和更清楚的失败信息。

维护成本：2/10。后面继续补映射时，脚本结构不需要再改。

# 2026-06-23 实现记录：真实后端接入 payload materialize hook

这轮把前一版的 shared payload materialize 入口接进了真实后端链路，并把
`ReturnDuplicatorLow` 的 branch return-region gate 拆成前驱感知能力位。目标还是
同一个：让 copied block 的 payload rewrite 真正落到 shared CFG，而不是只留在测试里。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:145`
  给 `setPayloadMaterializeHook()` 增加可选的
  `SupportsPredecessorRewrite` 标记，并新增
  `hasPredecessorRewritePayloadMaterializeHook()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:142`
  保存这个能力位。普通 payload clone 和 predecessor-aware rewrite 现在分开记。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1110`
  `ReturnDuplicatorLow` 只在 predecessor-aware hook 打开时才放行一般 branch
  return-region，避免普通 clone hook 误扩大复制形状。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuredGoto.cpp:40`
  C 的 shared structuring 链路现在安装 payload materialize hook。这里先做保守的
  payload id 复制，让 copied block 在 shared CFG 里能拿到新 id。
- `external/NotDec-llvm2c/lib/Solidity/BodyBuilder.cpp:280`
  Solidity 链路也安装同类 hook，字符串 payload 会生成新的 id，后续 copied
  block 不再直接复用原 payload 位置。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2203`
  新增 predecessor-aware branch return-region 测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2368`
  新增负例测试，确认只有普通 clone hook 时 branch return-region 不会被打开。

## 验证

构建：

```bash
cmake --build ./build --target notdec structuring-analysis-test -j4
```

结果：通过。

测试：

```bash
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure
```

结果：通过，CTest subset 5/5。

性能 smoke：

```bash
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' \
  ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-sailr-stage2.c --tr-level=2 --algo=structured-sailr
```

结果：`elapsed=205.86 user=228.62 sys=1.66 maxrss=1260704`。和前一轮 196.55s、
200.10s 同一量级，没有看出这次接 hook 带来新的明显退化。

## 当前判断

实现效果：8/10。现在真实 C/Solidity 链路都能走 shared payload materialize hook，
branch return-region 也被更窄的能力位保护住了。copied block 的 payload 复制不再只靠测试桩。

复杂度：6/10。多了一个能力位，但逻辑仍在 `StructuredCFG` 和 `SAILRDeoptimization`
两处收口，没有把判断散到 renderer 里。

维护成本：6/10。后续如果真要做 predecessor 级 rewrite，只需要在 hook 里补具体规则；
目前普通 clone 和 predecessor-aware rewrite 已经分层。

仍未完成的部分没有变：Phi / vvar 的真实语义还没补，return duplication 也只是
一般 branch 形状的最小放行，不是完整 Angr 对齐。

# 2026-06-23 实现记录：Solidity 入口统一 Phi demote

这次把 Solidity 输出前的入口也接到了同一条 demoteSSA 路线上。这样 shared
structuring 还是保持“结构恢复前不处理 Phi”的旧约定，同时 HType 也会在
demote 前后按 Phi 名字迁到 `.reg2mem` alloca 上。

## 修改内容

- `src/Passes/PassManager.cpp:53`
  在 `#ifdef NOTDEC_ENABLE_LLVM2C` 下补入 `notdec-llvm2c/StructuralAnalysis.h`，
  让顶层 pass 可以直接复用 `demoteSSA()` / `demoteSSAFixHT()`。
- `src/Passes/PassManager.cpp:193`
  在 `MLsubNotdecSolidity::run()` 里，调用 Solidity backend 前先执行
  `demoteSSAFixHT()`；如果没有类型恢复结果，就退回 `demoteSSA()`。
  这样 Solidity 侧和 C 侧一样，结构恢复入口看到的都是已经去掉 Phi 的 IR。

## 验证

- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/evm/solidity-patterns/cases/25928_19774281_d048a8d52d_2758caa02f46.ll -o /tmp/notdec-sailr-phi-demote.sol --tr-level=2 --work-dir=/tmp/notdec-sailr-phi-demote-work --gen-work-dir`

结果：构建通过，smoke 通过，`llvm2c-before-demotessa.ll` 里还能看到 Phi，
`llvm2c-after-demotessa.ll` 里已没有 Phi，只剩 `.reg2mem` 相关的 load/store。

# 2026-06-23 实现记录：copied region 的 predecessor-aware materialize context

这次补的是 shared CFG 复制时的 context 语义。之前 copy helper 虽然会调用带
predecessor 参数的 `materializeBlockBody()`，但对 region 内所有 copy 都传同一个
外部 predecessor。这样后续做 predecessor-sensitive payload rewrite 时，内部 copied
block 不知道自己来自哪个原 predecessor，也不知道对应的新 copied predecessor。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:497`
  新增 `materializeDuplicatedRegion()`，统一给 copied region 内每个 block 计算
  `OriginalPredecessor` 和 `NewPredecessor`。region head 使用外部 predecessor；
  region 内部 block 使用原内部 predecessor 和对应 copy。多前驱或无法唯一判断时继续传
  `InvalidBlockId`，保持保守。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:545`
  `copyRegionForPredecessors()` 改用这个 helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:702`
  `copyLinearRegionForPredecessors()` 也改用同一 helper，避免 Return / switch deopt
  两条复制路径语义分叉。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2230`
  加强 `testReturnDuplicatorLowCopiesBranchReturnRegionWithPayloadRewrite()`：
  payload hook 现在要求 predecessor context 非空，并用 `NewPredecessor` 生成 payload，
  验证 region head、then/else tail、return block 都拿到正确的新前驱。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pred-context.c --tr-level=2 --algo=structured-sailr`

结果：构建和 structuring subset 都通过。fortune smoke 结果：
`elapsed=194.84 user=217.12 sys=1.71 maxrss=1257804`，和前面 196-205 秒同口径，
没有看到明显性能退化。

## 当前判断

实现效果：7/10。copied region 的 materialize context 已经能表达单前驱的真实
source-target 身份，后续做 payload rewrite 不再只能看到 copy block id。

复杂度：5/10。只多了一个 shared helper，两个 copy helper 复用同一套规则。

维护成本：5/10。多前驱仍保守传 `InvalidBlockId`，没有提前承诺复杂 Phi/vvar rewrite。

# 2026-06-24 实现记录：switch default-tail materialize context 测试

这次没有改算法，只补测试覆盖。上一轮已经让 `copyLinearRegionForPredecessors()` 走
predecessor-aware materialize context，但测试主要覆盖 `ReturnDuplicatorLow`。这轮补上
switch default-tail 复制路径，确认 switch deoptimization 也消费同一套 shared context。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2698`
  加强 `testSwitchDefaultCaseDuplicatorCopiesDefaultTailRegion()`：安装
  predecessor-aware payload hook，要求 `OriginalPredecessor` 和 `NewPredecessor`
  非空，并用 `NewPredecessor` 改写 copied default head / tail 的 payload。
  测试同时确认共享的 return block 没有被复制，payload 保持原样。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`

结果：全部通过。

# 2026-06-24 实现记录：StrictlyLessGotos 独立生效

这次修的是 shared optimization guard。之前 `StrictlyLessGotos` 的最终判断代码已经在，
但只有 `PreventNewGotos=true` 时才会进入 goto 数量检查；如果 pass 只打开
`StrictlyLessGotos`，严格减少 goto 的约束会被跳过。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:12`
  `needsInitialEvaluation()` 把 `StrictlyLessGotos` 也作为需要 initial trial 的条件。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:21`
  `acceptsFinalEvaluation()` 在 `PreventNewGotos` 或 `StrictlyLessGotos` 任一开启时都检查
  goto 数量；严格模式继续要求 final goto 数量小于 initial。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1380`
  在允许新增 goto 的 cross-jump 测试里显式关闭 `StrictlyLessGotos`，避免测试意图依赖默认值。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3362`
  新增 `testStructuringOptimizationPassEnforcesStrictlyLessGotos()`，覆盖
  `PreventNewGotos=false` 但 `StrictlyLessGotos=true` 的拒绝路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5914`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-strict-gotos.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=198.74 user=221.13 sys=1.80 maxrss=1266112`，和前几轮 194-205 秒同口径。

# 2026-06-24 实现记录：DuplicationReverter 默认迭代次数对齐 Angr

这次对照 Angr 当前 `DuplicationReverter`，把 shared pass 的默认迭代次数从 4 改为 5。
这只是 pass option 对齐，不引入 renderer 行为。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1070`
  `DuplicationReverter::defaultOptions()` 的 `MaxOptIters` 改为 5。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3481`
  新增 `testSAILRDeoptimizationDefaultOptionsMatchAngr()`，覆盖
  `DuplicationReverter`、`ReturnDuplicatorLow`、`CrossJumpReverter` 的关键默认选项。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5943`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-dup-iters.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.85 user=219.02 sys=1.74 maxrss=1264000`，没有看到明显性能退化。

## 仍未实现

Angr 的 `DuplicationReverter` 还有 `_get_new_gotos()` 对 future irreducible gotos 的过滤。
当前 shared CFG 还没有足够语义判断这类 goto，不能只靠本地猜测实现；后续需要先定义
shared 层的 future irreducible goto 识别规则。

# 2026-06-24 实现记录：switch case value materialize 带 target 身份

这次补 copied switch block 的 payload materialize context。之前 hook 能看到
source/body/copy block 和 predecessor，但处理 `SwitchCaseValue` 时只能靠 index 推断 case
target。现在 shared CFG 直接把原 case target 和复制后的 case target 传给 hook，后端不用猜。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:68`
  `PayloadMaterializeContext` 新增 `OriginalTarget` 和 `NewTarget`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:247`
  `StructuredCFG::materializeBlockBody()` 在 materialize switch case value 时填入
  `Body->Cases[I].Target` 和 `Block->Cases[I].Target`。statement / condition 继续保持空 target。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1096`
  加强 `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()`：验证 external case
  是 `12 -> 12`，region 内部 case 是 `11 -> CopyBodyId`，同时 case value payload 被 hook 改写但
  copied CFG target 不被覆盖。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-case-targets.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.62 user=218.71 sys=1.68 maxrss=1260792`。

# 2026-06-24 实现记录：synthetic forwarder 记录原始边身份

这次补 switch default rewrite 里的 synthetic forwarder 身份。之前 forwarder 只有当前
successor，后续只能从图里反推它代表哪条原始 edge；如果图再被复制或重定向，原 source-target
关系会丢。现在 shared CFG 直接记录 synthetic forwarder 对应的原 source 和 target。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:102`
  `CFGBlock` 新增 `SyntheticSource` / `SyntheticTarget`，只用于 synthetic forwarder 的原始边身份。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:136`
  新增 `StructuredCFG::createSyntheticForwarder(Source, Target, Creator)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:74`
  实现 `createSyntheticForwarder()`，复用普通 synthetic block，再写入 source / target。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:997`
  `SwitchDefaultCaseDuplicator` 改用 `createSyntheticForwarder()`，记录原 switch default edge。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1393`
  `testStructuredCFGCreateSyntheticBlock()` 覆盖普通 synthetic block 不带原边身份、forwarder 带原边身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2609`
  `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()` 验证两个 default forwarder
  分别记录 `0 -> 1` 和 `3 -> 1`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-forwarder-identity.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.91 user=218.94 sys=1.73 maxrss=1260472`。

# 2026-06-24 实现记录：copied forwarder 保留原始边身份

这次补 copied synthetic forwarder 的身份覆盖。forwarder 被 region copy 后仍然要表示原来的
virtualized edge，但 copy 自己必须有独立 `BlockId` 和 copied 来源身份。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:102`
  注释明确 synthetic forwarder 的 copy 也保留原始 virtual edge 身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1356`
  新增 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()`，覆盖 copied
  forwarder 保留 `SyntheticSource` / `SyntheticTarget`，同时拥有独立 `BlockId`、
  `SourceBlock`、`CopyKind` 和 copied origin。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5957`
  把新测试接入 structuring analysis 测试入口。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-forwarder-copy.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.56 user=218.09 sys=1.50 maxrss=1261056`。

# 2026-06-24 实现记录：removeBlock materialize 失败保持回滚

这次补 shared CFG 的 checkpoint-rollback 语义。`removeBlock()` 删除被 copied block 复用的
body 前，会先 materialize 这些 copy。之前如果后一个 copy materialize 失败，前一个 copy
可能已经被写实，函数返回 false 但图已经变了。现在先在候选图上完成删除流程，成功后再提交。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:174`
  新增私有 `removeBlockInPlace()`，把实际删除逻辑和对外事务边界分开。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:410`
  `StructuredCFG::removeBlock()` 先复制候选图，调用 `removeBlockInPlace()`，失败时保留原图。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:420`
  原删除逻辑移动到 `removeBlockInPlace()`，语义不变。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:999`
  加强 `testStructuredCFGRemoveBlockIsAtomicOnMaterializeFailure()`：第二个 copied body
  materialize 失败时，第一个 copy 也不能被写实，原 body 和两个 copy 都保持原状态。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-removeblock-atomic.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=198.26 user=220.25 sys=1.67 maxrss=1261924`，和近期同口径结果接近。

# 2026-06-24 实现记录：materialize context 带 successor 映射

这次继续补 copied block 的 shared payload materialize 语义。之前 hook 能看到 predecessor
和 switch case 的单个 target，但看不到原 block 和 copy block 的完整 successor 列表。
现在 context 直接带 original/new successors，branch condition、switch condition、statement
和 case value rewrite 都能用同一份 shared CFG 身份信息。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  `PayloadMaterializeContext` 新增 `OriginalSuccessors` 和 `NewSuccessors`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入 successor 映射。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:772`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied block 的 successor
  context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1114`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  内部 successor 被改写时，context 同时保留 original/new successor 和 case target 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-successors.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=196.83 user=218.85 sys=1.68 maxrss=1256772`，没有看到明显性能退化。

# 2026-06-24 实现记录：materialize context 带 terminator 身份

这次继续补 copied block 的 shared payload materialize context。之前 hook 能看到
successor 映射，但处理 condition payload 时仍要从外部推断这是 branch 还是 switch。
现在 context 直接带 original/new terminator kind，让后续 payload rewrite 使用 shared CFG
身份信息，不需要 renderer 侧猜。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:78`
  `PayloadMaterializeContext` 新增 `OriginalTerminator` 和 `NewTerminator`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:232`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入 terminator 身份。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:772`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied switch 的 terminator
  context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1114`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  switch block 的 original/new terminator 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-terminator.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=195.81 user=218.38 sys=1.75 maxrss=1263424`，没有看到明显性能退化。

# 2026-06-24 实现记录：materialize context 带 switch case 映射

这次继续补 copied switch 的 shared payload materialize 语义。之前 hook 能看到
单个 case 的 original/new target，但看不到完整 switch case 列表。现在 context
同时带 original/new cases，后续 rewrite case value 或判断 reused-entry 时，不需要从
renderer 侧倒推 CFG 身份。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:76`
  `PayloadMaterializeContext` 新增 `OriginalCases` 和 `NewCases`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:230`
  `StructuredCFG::materializeBlockBody()` 从 body block 和 copy block 填入完整 case 列表。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:269`
  `StructuredCFG::materializeBlockBody()` 在每个 case value hook 前设置当前
  `OriginalTarget` / `NewTarget`，同时保留完整 case context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:781`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied switch 的
  original/new case 列表。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1135`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy 后
  内部 case target 被改写时，context 同时保留 original/new cases 和单个 case target。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-cases.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=202.86 user=225.17 sys=1.73 maxrss=1264320`，和近期同口径结果接近。

# 2026-06-24 实现记录：copied block 记录 immediate source

这次补 copied block 的另一层身份。之前 `SourceBlock` 表示最初语义来源，copy-of-copy
时仍会指向原始 block，但 shared CFG 里没有字段记录“本次是从哪个具体 block 复制出来的”。
现在新增 `CopiedFromBlock`，后续 payload rewrite、goto source-target 判断和回滚诊断
都能区分 ultimate source 和 immediate source。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:72`
  `PayloadMaterializeContext` 新增 `CopiedFromBlock`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:112`
  `CFGBlock` 新增 `CopiedFromBlock`，记录本次复制的直接来源。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:106`
  `StructuredCFG::duplicateBlock()` 设置 copy 的 immediate source。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:229`
  `StructuredCFG::materializeBlockBody()` 把 immediate source 带进 materialize context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:731`
  `testStructuredCFGDuplicatesBlockBodySource()` 覆盖 copy-of-copy 时
  `SourceBlock` 仍指原始 block，`CopiedFromBlock` 指上一层 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:781`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖普通 copied block context。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1136`
  `testStructuredCFGMaterializesCopiedSwitchWithoutRewritingTargets()` 覆盖 region copy
  switch context。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-copied-from.c --tr-level=2 --algo=structured-sailr`

结果：构建、测试和 fortune smoke 通过。fortune smoke：
`elapsed=200.23 user=222.21 sys=1.66 maxrss=1260716`，和近期同口径结果接近。

# 2026-06-24 实现记录：未命名 Phi demote 保持 HType 映射

这次补结构恢复前 Phi demote 的一个边界。`demoteSSAFixHT()` 通过 PHI 名字把旧
HType 映射迁到 reg2mem alloca；未命名 PHI 的名字是空串，同一函数里多个未命名
PHI 会撞到同一个 key。现在 demote 前给未命名 PHI 分配稳定名字，继续保持
structuring 算法不直接处理 PHI。

## 修改内容

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1479`
  `demoteSSAFixHT()` 在收集 PHI HType 前给未命名 PHI 设置 `notdec.phi` 名字，
  让 LLVM 自动在函数内 uniquify。
- `external/NotDec-llvm2c/test/CMakeLists.txt:4`
  新增 `phi-demote-test`，直接链接 `notdec-backend-c`。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:28`
  新增 `testDemoteSSAFixHTKeepsUnnamedPhiTypes()`：构造两个未命名 PHI，
  验证 demote 后无 PHI，旧 PHI HType 映射被删除，两个 reg2mem alloca 都拿到
  pointer-wrapped HType，contravariant 标记也迁到对应 alloca。

## 验证

- `cmake --build ./build --target phi-demote-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-phi-demote.c --tr-level=2 --algo=structured-sailr`

结果：新增测试、structuring CTest 子集和 fortune smoke 通过。fortune smoke：
`elapsed=198.91 user=221.77 sys=1.54 maxrss=1264148`，和近期同口径结果接近。

# 2026-06-24 实现记录：materialize 结果通知支持 payload 回滚

这次补 copied payload materialize 的回滚边界。之前 hook 可以生成新的 payload id，
但如果后续 statement / condition / case value rewrite 失败，shared CFG 只能保持
block 不被写实，不能通知后端丢弃已经 clone 出来的 payload。现在新增 result hook，
materialize 成功时通知 committed payload，失败时通知 aborted payload，清理语义仍在
shared structuring 边界上。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:68`
  新增 `PayloadMaterializeResult`，区分 `Committed` 和 `Aborted`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:96`
  新增 `PayloadMaterializeResultHook`，传回本次生成的 payload 列表。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:175`
  `StructuredCFG` 新增 `setPayloadMaterializeResultHook()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:36`
  新增 `appendGeneratedPayload()`，只记录有效且不同于原 payload 的新 payload。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:253`
  `StructuredCFG::materializeBlockBody()` 在 rewrite 失败时发出 `Aborted` 通知。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:318`
  `StructuredCFG::materializeBlockBody()` 在成功写实后发出 `Committed` 通知。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:812`
  `testStructuredCFGMaterializeRewritesCopiedPayloads()` 覆盖 committed payload 列表。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:876`
  `testStructuredCFGMaterializeRewriteFailureIsAtomic()` 覆盖失败时只回报已经生成的
  payload，且 block 本身仍保持未写实状态。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-materialize-result.c --tr-level=2 --algo=structured-sailr`

结果：structuring 单测、CTest 子集和 fortune smoke 通过。fortune smoke：
`elapsed=199.28 user=222.01 sys=1.69 maxrss=1262136`，和近期同口径结果接近。

# 2026-06-24 对照记录：SAILR pass 和 Angr 语义差异

这次没有改算法。当前环境没有安装 `angr`，磁盘上也没有可用的 Angr 源码镜像，所以
下面只记录当前 NotDec shared 实现能确认的对照状态；不能确认源码细节的地方不继续猜。

## 当前 pass 对照

- `SwitchDefaultCaseDuplicator`
  对应 Angr SAILR 的 switch default 复制/forwarder 类 deoptimization。NotDec 已在
  shared CFG 里记录 default forwarder 的原始 `source -> target`，并用 copied block
  身份 materialize default tail。差异：NotDec 仍保守跳过 terminal shared default 和
  switch-internal default predecessor。
- `DuplicationReverter`
  对应 Angr 的 duplicate block merge pass。NotDec 已支持完全相同 block 的合并、
  switch predecessor case 重定向、事务式删除和默认 5 次迭代。差异：Angr 的
  `_get_new_gotos()` 会过滤 future irreducible gotos；NotDec shared 层还没有足够语义
  定义这类 goto，暂不实现。
- `LoweredSwitchSimplifier`
  对应 Angr 的 lowered switch case tail 简化。NotDec 已支持线性 shared case region、
  terminal fork case region、case-only target 判断和失败回滚。差异：更复杂的
  case/default 交叉复用还没扩。
- `ReturnDuplicatorLow`
  对应 Angr 的 low return duplication。NotDec 已支持 linear return tail、terminal fork、
  return-tail fork、goto tail 和带 predecessor-aware materialize 的一般 branch return
  region。差异：多前驱 component 的 payload rewrite 仍保守传 `InvalidBlockId`，没有扩到
  Phi/vvar 级别 rewrite。
- `CrossJumpReverter`
  对应 Angr 的 cross jump revert。NotDec 已支持线性 goto target copy、connected predecessor
  合并复制和 strictly-less-gotos guard。差异：更复杂 region 仍依赖 shared CFG 后续表达能力。
- `SwitchReusedEntryRewriter`
  对应 Angr reused-entry 处理。NotDec 已支持 case-only reused entry、entry tail copy、
  connected predecessors 和复用上限。差异：Angr 偏 virtual goto 的路径还没确认，NotDec
  当前用 copied entry region 表达，不能静默扩大。

## 当前暂停点

`DuplicationReverter::_get_new_gotos()` 的 future irreducible goto 过滤需要 Angr 源码或
清晰 shared CFG 定义后再做。现在如果只按名字猜，会影响 quality guard 是否接受 pass，
属于算法语义选择，不应在没有证据时落代码。

# 2026-06-24 实现记录：SwitchReusedEntryRewriter 改为 synthetic goto

这次对照 Angr 当前 `switch_reused_entry_rewriter.py`。Angr 对 reused switch entry 的做法是
保留第一个 switch predecessor，其余 predecessor 指向新建 goto block，并且这个 goto block
不再保留到 entry 的 CFG edge。NotDec 之前是复制 entry tail region，这和 Angr 语义不同。
这轮把它改成 shared synthetic goto block，仍然不把语义放进 C/Solidity renderer。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:45`
  新增 `CFGBlockCopyKind::SyntheticGoto`，区分“有真实 successor 的 synthetic forwarder”
  和“只表示 goto 的 synthetic block”。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:160`
  新增 `StructuredCFG::createSyntheticGoto(Source, Target, Creator)`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:94`
  实现 synthetic goto block：记录 `SyntheticSource` / `SyntheticTarget`，但
  `Successors` 为空，避免把 virtual goto 伪装成真实 CFG edge。
- `external/NotDec-llvm2c/lib/Structuring/GotoStructurer.cpp:32`
  shared goto structurer 渲染 synthetic goto block 时，在 block body 后追加
  `StructuredNodeKind::Goto`。
- `external/NotDec-llvm2c/lib/Structuring/PhoenixStructurer.cpp:149`
  Phoenix/SAILR reducer 的 block body 路径也追加 synthetic goto，保证 trial 和
  quality guard 能看到这类 goto。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:802`
  `SwitchReusedEntryRewriter` 不再复制 entry tail；排序后保留第一个 switch predecessor，
  其余 predecessor redirect 到各自的 synthetic goto block。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:943`
  新增 `testGotoStructurerRendersSyntheticGoto()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1531`
  `testStructuredCFGCreateSyntheticBlock()` 覆盖 synthetic goto 的身份和空 successor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2930`
  到 `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3100`
  更新 reused-entry 测试，确认不再复制 entry/tail，而是生成独立 synthetic goto block。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-reused-entry-goto.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.59 user=224.71 sys=1.62 maxrss=1264160`，和近期 195-203 秒同口径结果接近。

## 当前判断

实现效果：8/10。reused-entry 现在更接近 Angr：shared CFG 显式表达 virtual goto，
而不是复制 entry body 来绕过问题。

复杂度：6/10。新增一个 synthetic block kind 和两个 structurer 输出点，但边界仍在
shared CFG / shared tree，没有后端特判。

维护成本：6/10。后续 switch deoptimization 要继续区分 synthetic forwarder 和
synthetic goto；这个成本比把 reused-entry 复制语义混进 renderer 更低。

# 2026-06-24 实现记录：SAILR 迁移测试接入本地 shared 样例

这次继续推进 Angr 侧 SAILR 测试迁移，不再只停留在测试名对照。把
`run_sailr_bench2_migration.py` 里的输入改成仓库内可复用的 shared 样例，并补了
本地路径解析，保证 CTest 和手动运行看到同一批输入。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:6`
  新增 `REPO_ROOT`，让相对输入路径按仓库根解析。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:44`
  `switch_reuse_proxy`、`duplication_reverter_proxy`、`duplication_too_sensitive_proxy`、
  `early_return_proxy`、`switch_case_recovery_proxy` 继续作为迁移代理样例。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:209`
  `run_case()` 现在会把相对输入路径转成仓库根下的真实文件。

## 验证

- `python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis|sailr-bench2-migration' --output-on-failure`

结果：迁移脚本通过，CTest 子集通过。
当前迁移覆盖了 return tail、early return、switch recovery、switch clustering、duplicate proxy
和 condensing proxy，但还不是 Angr 全量测试集。
本轮又补了一个 nested switch 代理样例，确认 shared structuring 能把嵌套 switch 保住为
两层 switch，而不是退回 goto。
另外试过一个 loop 型 `printenv_main` 代理，但这个输入在当前 shared 结构下还是会引入
多余 goto，不适合作为稳定迁移样例，先不纳入迁移集。
本轮还补了一个 `tr_build_spec_list` 风格的无 goto proxy，用来覆盖
DuplicationReverter 在简单分支合并时不引入额外控制流的形状。
本轮也补了一个 `printenv_main` 风格的 root-cycle proxy，确认 shared structuring 能保住
`do { ... } while (...)` 形状，而不是退回显式 goto。
本轮还把 `eager_returns_simplifier_no_duplication_of_default_case` 对应成了一个稳定的
early-return proxy，继续往 ReturnDuplicatorLow 的 Angr 测试覆盖靠近。
本轮又补了一个 else-if / ITE proxy，对应 Angr 里 `test_else_if_scope_printing` 这类
稳定分支打印测试。
本轮还补了 `phi-demote-test` 的第二个用例，专门确认 demote 后旧 Phi 的 HType /
contra-variant 关系会被清掉，只留下新的 reg2mem 载体。
现在这条边界更明确了：旧 Phi 的 value-type 条目不会残留在 `ValueTypesLower` /
`ValueTypesUpper` 里，后续 structuring 看到的只会是 demoted 后的新值。
同时把这段清理抽进了 `HTypeResult` 的通用接口，`demoteSSAFixHT()` 现在只负责判定
哪些值该删，不再自己手写三段 map/set 清理逻辑。
现在 `HTypeResult` 也有了按 demoted 值统一清理条目的入口，后面 shared structuring 再
往前挪时，旧 Phi 的类型映射不会再留在这层散着处理。

## 2026-06-24 迁移补充：补一个 terminal shared default proxy

这次继续往 Angr 的 `switch default / reused-entry` 方向补迁移样例，新增一个
terminal shared default proxy。目标不是追求更复杂的输入，而是把“多个 switch 共享同一
个终点 default / return”这类边界先写进稳定回归里，避免后面只看 `switch_cluster_proxy`
那种简单 case。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:294`
  新增 `terminal_shared_default_proxy`，对应 `test_reverting_switch_clustering_and_lowering_cat_main_no_endpoint_dup`。

验证：

```bash
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c
ctest --test-dir build -R 'sailr-bench2-migration|structuring-analysis' --output-on-failure
```

结果：通过。

## 2026-06-24 迁移补充：收紧 switch 交叉复用样例的稳定边界

这轮先试了一个更激进的 switch 交叉复用 proxy，想把 default / reused-entry 的交叉复用
也纳入迁移回归。但实际跑下来，当前 shared structuring 输出会把第二层 `switch` 吃成更
简单的终点形状，断言 `switch (y)` 不稳定，所以不把这个样例留下。这里保留的是判断：
这类 case 现在还不适合作为稳定迁移基线，需要等 shared 语义再往前一步，或者换更贴近
当前输出的 Angr 原始输入。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:328`
  新增后又移除了一个 `switch_cross_reuse_proxy` 试验样例。

验证：

```bash
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c
ctest --test-dir build -R 'sailr-bench2-migration|structuring-analysis' --output-on-failure
```

结果：最终迁移脚本和 CTest 子集通过；这类更激进的交叉复用样例暂不纳入主回归。

## 2026-06-25 迁移边界：几个小的真实 Bench2 样例只能当 smoke

这轮又试了几个更小的真实 Bench2 输入：

- `libuv/1-uv__cancelled.ll`
- `memcached/1-drive_machine.lto_priv.0.cold.ll`
- `wolfssl/1-wc_PKCS7_DecodeEncryptedData.cold.ll`

它们都能跑完，但输出都太短，基本只剩 `return;`，不够替换当前迁移脚本里的
`switch_reuse_proxy`、`duplication_reverter_proxy` 或 `condensing_real_lighttpd`。所以这批
样例先只记为 smoke，不纳入 Angr SAILR 迁移主回归。

判断也更明确了：当前迁移脚本里真正能钉住语义边界的，还是那几个手写 proxy 和
`switch_case_recovery.ll` 真实 fixture；更小的真实 Bench2 片段暂时只能证明链路能跑，
不能证明语义对齐。

## 2026-06-25 现阶段收口：迁移脚本先保留 proxy，smoke 只补稳定真实片段

现在这条线先不再追求把所有 Angr SAILR 测试都换成真实 Bench2 输入。原因已经明确：

- 真正小的真实片段，输出太短，只能做 smoke。
- 真正能钉住语义边界的，还是 `switch_reuse_proxy`、`duplication_reverter_proxy`、
  `condensing_real_lighttpd` 这类脚手架。
- `switch_case_recovery.ll` 这种真实 fixture 可以保留，但它的角色是补一个真实片段，
  不是替掉整个迁移集。

所以后面继续推进时，迁移脚本先按“proxy + 少量真实 fixture”保住语义边界，smoke 只
补稳定的小真实片段，不再继续找一个能一把替掉所有 proxy 的输入。

## 2026-06-25 迁移再收口：phi demote 只在 smoke 里保留

这轮又确认了一遍，`run_structuring_smoke.py` 里已经有
`phi_demote_before_structuring`，它能把“结构恢复前先 demote Phi”的边界钉住。
迁移脚本这边不需要再额外塞一个专门的 Phi 迁移样例，因为它真正负责的是 Angr SAILR 的
return / switch / condensing 边界，不是把 SSA 细节再重复一遍。

所以这条线当前的分工更清楚了：

- `phi_demote_before_structuring` 负责证明 Phi 在结构恢复前先被处理掉。
- `sailr-bench2-migration` 负责证明 Angr SAILR 的 return / switch / condensing 语义还能跑。
- 两边都已经有稳定回归，不需要再为同一个边界额外加一层样例。

## 2026-06-25 当前边界确认：Phi / switch / condensing 都有稳定回归

这轮重新跑了三条主要回归：

- `phi-demote-test`
- `structuring-smoke`
- `sailr-bench2-migration`

结果都还是绿的。说明现在这几个关键边界已经比较稳：

- Phi 的前置 demote 和 HType 清理边界，已经在 `phi-demote` 和 `phi-demote-htypes`
  里独立覆盖。
- 真实 `switch_case_recovery.ll` 已经进了 `structuring-smoke`。
- Angr SAILR 的 return / switch / condensing 仍由迁移脚本里的 proxy 和少量真实
  fixture 钉着。

这意味着后面继续往前，只需要盯更具体的 Angr 原始测试对应样例，或者更明确的
shared structuring pass 差异，不需要再回头补这些基础边界。

## 2026-06-25 迁移结论：测试名已经对齐，缺的是原始资产

这轮重新核了一遍 `run_sailr_bench2_migration.py` 和日志里的 Angr 测试名。结果是：

- 日志里提到的核心 SAILR 测试名都已经在脚本里有对应 `angr_test`。
- 当前脚本的缺口不是测试名映射，而是这些 Angr 原始输入本身没有在本地树里完整落地。
- 这也是为什么前面把一些小 Bench2 片段试成 smoke 后，还是不能替掉现有 proxy。

所以这条线后面如果还要继续，只能是两种方向：

1. 找到更贴近这些 Angr 测试名的原始资产或等价输入。
2. 继续补 shared structuring 的 pass 语义差异，而不是再增加一层同类的测试壳。

## 2026-06-25 再确认：shared structuring 的测试壳已经够了

这轮重新核了一遍 `structuring_analysis_test.cpp` 里的关键覆盖，发现现在这几类边界都已经有
测试壳：

- synthetic forwarder / synthetic goto
- copied / grouped predecessor materialize
- ReturnDuplicatorLow
- SwitchDefaultCaseDuplicator / SwitchReusedEntryRewriter
- CrossJumpReverter

所以后面继续往 shared structuring 语义上推进时，不需要再先补“测试壳数量”，而是要么补
更具体的 pass 语义差异，要么去找更贴近 Angr 原始输入的等价样例。单纯重复加一层 proxy
或者 smoke 已经不会再把这条线往前推多少了。

## 2026-06-25 真实片段补充：switch 和 condensing 都有 smoke 入口

这轮又把两条真实 Bench2 片段接进了 `structuring-smoke`：

- `switch_case_recovery.ll`
- `lighttpd/1-main_init_once.ll`

这样现在 smoke 里已经同时有：

- Phi demote before structuring
- 真实 switch recovery
- 真实 condensing / goto 收口

这意味着基础共享语义边界已经不只靠 proxy 了，至少每个核心方向都有一条真实片段能跑。
后面如果继续推进，重点就不该是再加输入，而是更具体地对照 Angr pass 语义和原始测试资产。

## 2026-06-25 实现记录：真实 switch fixture 也接入 structuring smoke

这轮把 `switch_case_recovery.ll` 也接进了 `structuring-smoke`，这样真实的 switch recovery
片段就不只停留在 `sailr-bench2-migration` 里，而是也能在更基础的 structuring smoke 里
跑一遍。这样能同时确认：

- 真实 switch fixture 在 shared structuring 链路里还能跑通。
- 这个 fixture 仍然保留 `switch / case / default / return` 的形状，没有退回 `goto`。

- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:6`
  增加仓库根路径解析，支持相对真实输入。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:128`
  新增 `real_switch_fixture`。

验证：

```bash
ctest --test-dir build -R 'phi-demote|structuring-smoke|sailr-bench2-migration|structuring-analysis' --output-on-failure
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

## 2026-06-25 迁移收尾：这条线先停在 proxy + real fixture 的稳定点

这轮把真实小片段和 proxy 的边界再确认了一遍，`structuring-smoke`、
`sailr-bench2-migration` 和 `structuring-analysis` 都还是绿的。结果也更明确了：

- 小的真实 Bench2 片段能跑，但只够 smoke。
- 当前真正稳定钉住 Angr SAILR 语义的，还是 proxy 和 `switch_case_recovery.ll` 这种真实片段。
- 继续硬找“一把替掉所有 proxy 的真实输入”没有收益，容易把迁移脚本推向不稳定。

所以这条迁移线先停在现在这个稳定点，后面如果继续补，只应围绕更贴近语义的真实片段
或者更具体的 Angr 测试名对应样例，不再扩大输入范围。

# 2026-06-24 实现记录：DuplicationReverter 过滤 future irreducible goto

这次继续对照 Angr 当前 `duplication_reverter.py`。Angr 的 `DuplicationReverter`
默认开启 `require_gotos`、`prevent_new_gotos`、`must_improve_rel_quality`，并在
`_get_new_gotos()` 里过滤 future irreducible gotos：如果 goto target 不是出口，并且
在 5 步内连不到任何出口，就不把这个 goto 算作新 goto。之前 NotDec 把这点记成暂停项；
现在已有 Angr 源码证据，可以在 shared CFG 层实现。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h:33`
  新增 `GotoManager::fromGotos()`，让 pass 可以返回过滤后的 goto 集合。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp:65`
  实现 `fromGotos()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuringOptimizationPass.h:52`
  `getNewGotos()` 增加当前候选 `StructuredCFG` 参数，方便 pass 用 shared CFG 判定 goto
  是否仍应计入 guard。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:5`
  默认 `getNewGotos()` 继续返回当前 trial 的全部 goto；最终 guard 调用时传入候选图。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h:25`
  `DuplicationReverter` 覆盖 `getNewGotos()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:479`
  新增 shared CFG 的 5 步 endpoint reachability helper。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1079`
  `DuplicationReverter::defaultOptions()` 重新对齐 Angr，保留默认
  `RequireGotos=true`、`PreventNewGotos=true`、`MustImproveRelativeQuality=true`，只设置
  `MaxOptIters=5`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1142`
  实现 future irreducible goto 过滤。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1949`
  新增 `testDuplicationReverterFiltersFutureIrreducibleGotos()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3660`
  更新默认选项对齐测试，确认 `DuplicationReverter` 的 guard 默认开启。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-dup-goto-filter.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.99 user=222.80 sys=1.55 maxrss=1262180`，和近期同口径结果接近。

## 当前判断

实现效果：8/10。`DuplicationReverter` 的 guard 更接近 Angr，future irreducible goto
不再错误阻止 pass 接受。

复杂度：5/10。只扩了 pass hook 的参数和一个 shared CFG reachability helper，没有引入
后端差异。

维护成本：5/10。后续如果要调整 cutoff 或 endpoint 判定，只改 shared 层，不需要动 C/Solidity。

# 2026-06-24 实现记录：ReturnDuplicatorLow 扩展连通前驱组件

这次继续补 `ReturnDuplicatorLow` 和 Angr 的 connected in-edge grouping 语义。Angr
不是只复制单条命中 goto 的入边；如果这条入边所在的前驱连通组件可以整体复制，就把组件
一起复制，避免只改一条边后留下更差的局部结构。NotDec 现在在 shared CFG 层补这一步，
仍然不让 C/Solidity renderer 参与算法判断。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:377`
  新增 `expandToConnectedPredecessorComponents()`。它把已选 goto predecessor 扩展到
  `CurrentPreds` 里的连通组件；如果 payload hook 声明自己依赖具体 predecessor rewrite，
  就保守保持单 predecessor copy，避免 grouped copy 没有单一 incoming source 时误写 payload。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1238`
  `ReturnDuplicatorLow::runOnGraph()` 在未触发 “几乎所有 predecessor 都复制” 的路径上，
  先用上面的 shared helper 扩展 selected predecessors，再进入已有的
  `copyRegionForPredecessors()` / trial / quality guard 流程。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2061`
  把已有 connected predecessor 测试里的前驱改成显式 branch，避免测试形状被
  parent-goto-source 规则误判。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2103`
  新增 `testReturnDuplicatorLowExpandsGotoPredToConnectedComponent()`：4 个 return
  predecessor 里只有一个 predecessor 命中 goto，但它和另一个 predecessor 在同一组件；
  期望复制整个组件，未命中的其它 predecessor 继续指向原 return block。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-connected-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.15 user=221.01 sys=1.73 maxrss=1260152`，仍在近期 195-203 秒范围内。

## 当前判断

实现效果：7/10。Return duplication 的 predecessor component 选择更接近 Angr，但
grouped copy 下的 Phi / payload incoming rewrite 仍需后续 shared 表达补齐。

复杂度：4/10。新增一个小 helper，并复用现有 copy / rollback / quality guard。

维护成本：4/10。保守避开 predecessor-sensitive payload hook，后续补 grouped incoming
rewrite 时可以只收窄这条 guard。

# 2026-06-24 实现记录：PayloadMaterializeContext 支持 grouped predecessors

这次继续把 copied block 的 payload rewrite 往 Angr 方向补了一层。之前 shared
materialize 只能稳定带单个 predecessor 上下文，所以 `ReturnDuplicatorLow` 在需要复制
整个 connected predecessor component 时，仍只能保守地把 payload rewrite 当作单前驱处理。
现在 shared 层把 grouped predecessor 列表也带上，后续可以在不碰 renderer 的前提下，把
incoming rewrite 的语义补得更完整。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:74`
  `PayloadMaterializeContext` 新增 `OriginalPredecessors` / `NewPredecessors`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:179`
  `setPayloadMaterializeHook()` 新增 grouped predecessor capability 参数；
  同文件新增 `hasGroupedPredecessorRewritePayloadMaterializeHook()`。
- `external/NotDec-llvm2c/include/notdec-backends/Structuring/StructuredCFG.h:188`
  `materializeBlockBody()` 新增接受 predecessor vector 的重载。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:177`
  记录 grouped capability flag。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:217`
  新的 `materializeBlockBodyImpl()` 统一处理单 predecessor 和 grouped predecessor 两种路径。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:281`
  materialize context 同时填入单 predecessor 和 grouped predecessor 列表，hook 可以看见整组
  incoming identity，但 fast path 仍保持原有语义。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:377`
  `expandToConnectedPredecessorComponents()` 现在只在 hook 声明支持 grouped rewrite 时，
  才把选中的 predecessor 扩展到整个 connected component。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1217`
  `ReturnDuplicatorLow::runOnGraph()` 继续走 shared trial / rollback / quality guard，
  但复制和 materialize 现在能携带 grouped predecessor 上下文。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:856`
  新增 `testStructuredCFGMaterializeReportsGroupedPredecessors()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2188`
  新增 `testReturnDuplicatorLowReportsGroupedPredecessorRewrite()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-grouped-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.42 user=222.67 sys=1.69 maxrss=1259784`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。connected predecessor copy 现在能把 grouped incoming 上下文带到 shared
payload materialize 层，但真正按 predecessor/component 重写 Phi/vvar 还没完整接上。

复杂度：5/10。接口比前一版多了一层 grouped predecessor 维度，但都留在 shared CFG，
没有扩到 renderer。

维护成本：5/10。后续如果要把 grouped incoming rewrite 真正用起来，只需要沿这条 shared
context 往下接，不需要重写复制流程。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator 支持 terminal default forwarder

这次补 switch default deoptimization 的一个小缺口。之前 shared default block 必须有一个
successor，`SwitchDefaultCaseDuplicator` 才会记录这个 default target。这样 terminal
default（例如直接 return）完全跳过，两个 switch 仍共享同一个 default target。现在改成：
只要 default target 存在，就可以为每个 switch 插入 synthetic default forwarder；只有需要
继续复制 default tail region 时，才要求 default target 有单 successor。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1024`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 不再因为 default block 没有 successor 而跳过
  forwarder 收集；`SwitchPredsByDefault` 会记录 terminal default。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1029`
  tail region 收集仍保守要求 default target 只有一个 successor，不把 terminal default body
  当成可复制线性 tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2920`
  将 `testSwitchDefaultCaseDuplicatorSkipsTerminalSharedDefault()` 改为
  `testSwitchDefaultCaseDuplicatorForwardsTerminalSharedDefault()`，验证 terminal default 本体
  保持 return，同时两个 switch default successor 改到独立 synthetic forwarder。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-terminal-default-forwarder.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.80 user=222.71 sys=1.70 maxrss=1259784`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。terminal default 的 reused/default identity 现在能在 shared CFG 中显式表达，
但更复杂的 default/case 交叉复用仍没展开。

复杂度：3/10。只是把 forwarder 收集和 tail region 复制条件拆开。

维护成本：3/10。仍使用已有 synthetic forwarder，不引入 renderer fallback。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator 保持 default forwarder 身份

这次修正上一轮 default forwarder 后暴露出的 shared CFG 身份问题。同一轮
`SwitchDefaultCaseDuplicator` 会先给 switch default 插入 synthetic forwarder，然后再尝试
复制 default tail region。之前第二阶段会把刚生成的 forwarder 当成普通 predecessor，
把 forwarder 的 successor 改到 copied default tail，破坏了 “forwarder 表示 switch default
edge” 这个身份。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1088`
  tail-copy 阶段跳过 `CFGBlockCopyKind::SyntheticForwarder` 且
  `SyntheticTarget == DefaultTarget` 的 predecessor。这样 synthetic default forwarder 仍指向
  原 default block，不会被同一 pass 的 tail-copy 再改写。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2914`
  收紧 `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()`，明确断言两个
  forwarder 的 `Successors` 都保持 `{DefaultTarget}`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-default-forwarder-identity.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=202.00 user=223.59 sys=1.87 maxrss=1259840`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。default forwarder 身份现在更稳定，tail-copy 不会误改 synthetic default edge。

复杂度：3/10。只是在已有 tail-copy predecessor 筛选里排除 synthetic forwarder。

维护成本：3/10。逻辑仍在 shared SAILR pass 内，后端只消费最终 CFG/tree。

# 2026-06-24 实现记录：ReturnDuplicatorLow grouped predecessor 覆盖

这次没有扩大 matcher，只补了 `ReturnDuplicatorLow` grouped predecessor materialize 的
回归测试。目的很小：确认一个 copied return tail 可以按 connected predecessor component
触发 grouped payload rewrite，并且 copied block 仍保持独立 `BlockId` / `BodyBlock`。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowCopiesGroupedReturnPredsWithPayloadRewrite()`。
  测试构造 `{0,2}` 和 `{3}` 两个 predecessor component，确认 hook 收到 grouped
  `OriginalPredecessors` / `NewPredecessors`，payload 被 shared materialize 改写，
  `{0,2}` 共享同一个 copied return block，`3` 得到另一个 copy。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-grouped-return-preds.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.11 user=223.34 sys=1.74 maxrss=1271236`，仍在近期同口径范围内。

## 当前判断

实现效果：5/10。这次主要是把已有 grouped rewrite 边界钉牢，不等于 Phi / vvar
incoming rewrite 已完整。

复杂度：1/10。只新增 shared structuring 单测。

维护成本：1/10。测试直接覆盖 shared CFG 行为，不引入 renderer fallback。

# 2026-06-24 实现记录：SwitchReusedEntryRewriter 保持 default 边

这次修正 reused-entry 的 case/default 边界。`SwitchReusedEntryRewriter` 只应该处理
switch case target 复用；如果同一个 switch 的 default successor 也碰巧指向相关 block，
不能因为插入 synthetic goto 就把 default 边一起改掉。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `SwitchReusedEntryRewriter::runOnGraph()` 改用 case-only redirect helper，
  只替换 `SwitchCase::Target`，不再调用会同时改 `Successors` 和 `Cases` 的
  `redirectPredecessors()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testSwitchReusedEntryRewriterKeepsDefaultSuccessorUntouched()`，
  覆盖 case target 被 synthetic goto 替换，但 default successor 保持原目标的形状。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/sailr-reused-entry-default-preserve.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=201.06 user=223.26 sys=1.78 maxrss=1272340`。

## 当前判断

实现效果：7/10。reused-entry 的 case/default 身份更稳定，和前面的 lowered switch
case-only rewrite 保持一致。

复杂度：2/10。复用已有 case-only redirect helper。

维护成本：2/10。逻辑仍在 shared SAILR pass 内，没有 renderer fallback。

# 2026-06-24 实现记录：CrossJumpReverter 区分 switch case 边

这次修正 `CrossJumpReverter` 的 switch predecessor 处理。之前复制 cross-jump target
时统一调用 `redirectPredecessors()`，如果 predecessor 是 switch，就可能同时改 default
successor 和 case target。现在 shared 层会先判断 predecessor 是 case 边还是普通 successor
边：case 边走 case-only rewrite，普通边继续走原 redirect。

同一个 switch 的 case 和 default 都指向同一 target 时，目前 `StructuredGoto` 还没有
edge-kind 身份，不能判断当前 goto 来自 case 还是 default。这种形状先保守跳过，不猜。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `blockUsesSwitchCaseEdge()` 和 `blockUsesNonSwitchCaseEdge()`，并在
  `CrossJumpReverter::runOnGraph()` 中按边类型拆分 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testCrossJumpReverterRedirectsSwitchCasesOnly()`，覆盖 case target 被复制到
  copied region、default successor 保持原目标。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testCrossJumpReverterSkipsAmbiguousSwitchCaseDefaultTarget()`，覆盖 case/default
  同 target 时保守跳过。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-crossjump-switch-case.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=198.16 user=220.31 sys=1.68 maxrss=1270416`，仍在近期同口径范围内。

## 当前判断

实现效果：6/10。case-only cross-jump 已落到 shared CFG，但同 target 的 case/default
歧义还需要后续给 goto 或 edge 增加来源身份。

复杂度：2/10。只复用已有 switch case-only helper 的边界。

维护成本：2/10。逻辑仍在 shared SAILR pass 内，没有 renderer fallback。

# 2026-06-24 实现记录：StructuredGoto 记录 switch edge 来源

这次补上一轮留下的 edge-kind 缺口。`StructuredGoto` 现在记录 goto 是从普通未知上下文、
switch case 子树，还是 switch default 子树收集出来的。这样 shared deoptimization pass
不用只靠 source/target 猜边来源。

`CrossJumpReverter` 用这个信息处理同一个 switch 的 case 和 default 都指向同一 target
的形状：如果 structured tree 明确当前 goto 来自 case，就只复制和重定向 case target，
default successor 继续留在原 target；如果来源还是 unknown，就保持上一轮的保守跳过。

## 修改内容

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/GotoManager.h`
  新增 `StructuredGotoEdgeKind`，并在 `StructuredGoto` 中记录 `EdgeKind`。
- `external/NotDec-llvm2c/lib/Structuring/GotoManager.cpp`
  收集 switch case/default 子树里的 goto 时分别标记 `SwitchCase` 和 `SwitchDefault`；
  普通 sequence/if/body 上下文仍保持 `Unknown`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `CrossJumpReverter::runOnGraph()` 允许同一 source block 里多个 goto 指向同一 target，
  并用 edge kind 解决 case/default 同 target 的重写边界；删除 original region 前改为
  检查 candidate 图里 target 是否真的没有 predecessor。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testGotoManagerCollectsSwitchGotoEdgeKinds()` 和
  `testCrossJumpReverterUsesSwitchCaseGotoKind()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-goto-edge-kind.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=203.72 user=226.08 sys=1.73 maxrss=1269308`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。goto source-target 现在多了 switch case/default 来源身份，
CrossJumpReverter 不再需要对明确来源的同 target case/default 形状保守跳过。

复杂度：3/10。只给 shared goto summary 加一个小枚举，没有改 renderer。

维护成本：3/10。后续如果要区分更多 edge 类型，可以继续扩 shared edge identity。

# 2026-06-24 实现记录：ReturnDuplicatorLow 支持 switch return-region

这次补 `ReturnDuplicatorLow` 的一个保守 switch 形状：switch 的某个 successor 是当前
return tail，其他 case/default successor 都是单前驱、封闭的 return tail。只有在
predecessor-aware payload materialize hook 可用时才放行，避免 copied switch 的 condition
和 case value 复用原 payload。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `prependSwitchReturnRegion()`。它复用 `collectClosedLinearReturnTail()`，
  只接受所有非当前 return-head successor 都能闭合到 terminal return/unreachable 的 switch。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  `findLinearReturnRegion()` 的复杂 region gate 从 branch 扩到 branch/switch，
  仍由 predecessor-aware payload hook 控制。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowCopiesSwitchReturnRegionWithPayloadRewrite()`，
  覆盖 copied switch 的 condition、case value、default successor、case target 和
  copied return tails 都保持 shared CFG 身份。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-switch-return-region.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=200.22 user=222.46 sys=1.92 maxrss=1270592`，仍在近期同口径范围内。

## 当前判断

实现效果：7/10。`ReturnDuplicatorLow` 覆盖了一个 Angr 语义里常见的 switch return-region
子集，但仍没有放开带共享内部 predecessor 或非封闭 case tail 的复杂 switch。

复杂度：3/10。复用现有 closed-tail 收集和 transaction copy，没有新增 renderer 逻辑。

维护成本：3/10。后续要扩更复杂 switch 时，仍要先补 shared payload/Phi/vvar 边界。

# 2026-06-24 实现记录：Switch return-region payload guard 测试

这次只补测试，钉住上一轮 `ReturnDuplicatorLow` switch return-region 的安全边界：
没有 predecessor-aware payload materialize hook 时，不能复制 switch return-region。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testReturnDuplicatorLowSkipsSwitchReturnRegionWithoutPredecessorRewrite()`，
  构造和正向 switch return-region 测试相同的 CFG，但只安装普通 payload hook，
  确认 optimization trial 不接受这个复制。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只改测试，不跑 fortune。

## 当前判断

实现效果：5/10。没有扩大语义，只把 switch return-region 的 payload guard 固定下来。

复杂度：1/10。只新增 shared structuring 单测。

维护成本：1/10。测试防止后续误绕过 predecessor-aware payload rewrite 要求。

# 2026-06-24 实现记录：pred-sensitive copy 不合并前驱组件

这次补 shared copy 分组规则：如果 payload materialize hook 说明需要 predecessor rewrite，
但还不支持 grouped predecessor rewrite，就不能把多个相连 predecessor 合成一份 copied
region。这样 PHI demote 后的 incoming 值映射不会在 SAILR copied block 里被压成一个来源。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:417`
  新增 `materializePredecessorComponents()`，在 predecessor-sensitive hook 不支持 grouped
  rewrite 时返回单前驱组件；否则沿用 connected component。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1096`
  `LoweredSwitchSimplifier::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1254`
  `SwitchDefaultCaseDuplicator::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1423`
  `ReturnDuplicatorLow::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1522`
  `CrossJumpReverter::runOnGraph()` 改用这个分组规则。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3902`
  新增 `testLoweredSwitchSimplifierKeepsPredSensitiveCopiesSeparate()`，覆盖两个相连 switch
  predecessor 共享 case region 时，单前驱 payload rewrite 必须得到两份不同 copy。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6872`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-pred-sensitive-copy.c --tr-level=2 --algo=structured-sailr`

结果：构建、structuring 单测、CTest 子集和 fortune smoke 都通过。fortune smoke：
`elapsed=199.13 user=221.06 sys=1.48 maxrss=1267948`，仍在近期同口径范围内。

## 当前判断

实现效果：6/10。没有扩大 Angr 形状覆盖，但修住 copied payload incoming 身份的一个 shared
语义边界。

复杂度：2/10。只增加一个公共分组 helper，复用现有 hook 能力判断。

维护成本：2/10。后续真正支持 grouped predecessor rewrite 后，这条规则会自动回到组件级复制。

# 2026-06-24 实现记录：Solidity 消费 shared synthetic goto

这次只补 shared tree 到 Solidity fallback 的 smoke。`SwitchReusedEntryRewriter` 已经用
shared `SyntheticGoto` 表达 reused-entry virtual goto，C/GotoStructurer 已有覆盖；
这轮确认 Solidity 也消费同一份 `StructuredTree`，不在 renderer 里倒推 copied/virtual
block 身份。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1107`
  新增 `testSolidityBodyBuilderConsumesStructuredSyntheticGoto()`，从 `StructuredCFG`
  创建 synthetic goto，经 `GotoStructurer` 生成 tree，再交给 Solidity `BodyBuilder`
  渲染。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6834`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但补齐完成条件里 Solidity 消费 shared virtual/synthetic
block 的一个 smoke。

复杂度：1/10。只新增测试。

维护成本：1/10。测试防止 Solidity fallback 回到 renderer 侧猜 synthetic goto。

# 2026-06-24 实现记录：Switch default tail pred-sensitive 覆盖

这次只补测试，钉住 `SwitchDefaultCaseDuplicator` 使用 shared predecessor 分组规则的边界。
当 payload hook 只支持单 predecessor rewrite 时，default tail copy 不能因为两个 predecessor
互相连通就合成一份 copy，否则 PHI demote 后的 incoming 值来源会被压扁。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3566`
  新增 `testSwitchDefaultCaseDuplicatorKeepsPredSensitiveCopiesSeparate()`，构造两个相连的
  default predecessor，共享同一个 default tail，并安装单 predecessor payload rewrite hook。
  测试确认两条 incoming path 得到两份独立 copied default tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:6966`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但把 pred-sensitive copy 的 shared 规则钉到 switch
default tail pass 上。

复杂度：1/10。只新增 pass 级测试。

维护成本：1/10。测试防止 default tail copy 后续重新合并单前驱 payload 来源。

# 2026-06-24 实现记录：CrossJump pred-sensitive 覆盖

这次只补测试，钉住 `CrossJumpReverter` 使用 shared predecessor 分组规则的边界。
当 payload hook 只支持单 predecessor rewrite 时，cross-jump target copy 不能把相连的
goto predecessor 合成一份 copy，否则 copied payload 的 incoming 来源会丢失。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1846`
  新增 `testCrossJumpReverterKeepsPredSensitiveCopiesSeparate()`，构造两个相连 predecessor
  都跳到同一个 cross-jump target，并安装单 predecessor payload rewrite hook。
  测试确认两条 goto path 得到两份独立 copied target/tail。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7014`
  接入新测试。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但把 pred-sensitive copy 的 shared 规则钉到
`CrossJumpReverter`。

复杂度：1/10。只新增 pass 级测试。

维护成本：1/10。测试防止 cross-jump copy 后续重新合并单前驱 payload 来源。

# 2026-06-24 实现记录：Switch 类 SAILR options 覆盖

这次只补测试，钉住 SAILR deoptimization pipeline 里 switch 类 pass 的默认 options。
之前测试只覆盖了 `DuplicationReverter`、`ReturnDuplicatorLow` 和 `CrossJumpReverter`，
没有把 `SwitchDefaultCaseDuplicator`、`LoweredSwitchSimplifier`、`SwitchReusedEntryRewriter`
的 Angr 对齐选项固定下来。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4591`
  扩展 `testSAILRDeoptimizationDefaultOptionsMatchAngr()`，新增
  `SwitchDefaultCaseDuplicator`、`LoweredSwitchSimplifier` 和
  `SwitchReusedEntryRewriter` 的默认 options 断言。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4594`
  确认这三个 pass 都不要求已有 goto、不阻止新 goto、不要求 relative quality 改善。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4597`
  确认 `SwitchDefaultCaseDuplicator` 默认 `MaxOptIters == 2`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4611`
  确认 `LoweredSwitchSimplifier` 默认 `MaxOptIters == 2`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4631`
  确认 `SwitchReusedEntryRewriter` 默认 `MaxOptIters == 2`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `ctest --test-dir build -R 'phi-demote|legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `cmake --build ./build --target notdec -j4`

结果：structuring 单测、CTest 子集和 `notdec` 构建通过。此轮只补测试，不跑 fortune。

## 当前判断

实现效果：4/10。没有扩大算法语义，但补齐 pass options 对齐完成条件里的 switch 类覆盖。

复杂度：1/10。只新增默认选项断言。

维护成本：1/10。测试防止后续修改 switch deoptimization 默认 guard 时静默偏离 Angr。

# 2026-06-24 实现记录：Phi 先 demote 再做 shared type handoff

这次把旧链路里 Phi 先 demote 的边界再收紧了一层。结构恢复前仍然先走 `demoteSSAFixHT()`，让 structuring 不直接面对 Phi；同时把 Phi 的 HType 继续挂到 demoted 的 `.reg2mem` stack slot 上，后续 shared structuring 只消费已经 demoted 的 LLVM 值。

## 修改内容

- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1500-1628`
  修改 `demoteSSAFixHT()`。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1502-1538`
  先收集所有 Phi，再把它们的 HType 绑定到 demote 后新生成的 `.reg2mem` alloca 名称上。
- `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1595-1628`
  在 reg2mem 之后回填 `HT.ValueTypesLower/Upper` 和 `ContraVariantValues`，让后续类型恢复看到的是 demoted LLVM Value，而不是 Phi 本身。
- `external/NotDec-llvm2c/test/phi_demote_test.cpp:28-117`
  维持 Phi demote 单测，确认 unnamed Phi 会被稳定命名并清掉原始 Phi 类型映射，只留下 demoted slot 的类型映射。

## 验证

- `cmake --build ./build --target phi-demote-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：构建和单测通过。

## 当前判断

实现效果：6/10。把“Phi 不进 structuring”这条边界落到旧链路里了，但还没继续往 copied/virtual block 的 payload rewrite、ReturnDuplicatorLow 和 switch deoptimization 扩。

复杂度：2/10。只动了 demote 前后的 HType 交接，不改 structuring 算法本身。

维护成本：2/10。以后如果再碰 Phi 语义，先看这里的 demote 入口和类型回填，不用在 structuring 层补 Phi 特判。

# 2026-06-24 实现记录：Switch reused-entry / default-copy rollback 收口

这次继续把 shared structuring 的 rollback 边界往前收。之前虽然已经把 return 和 lowered switch 的复制改成 candidate 提交，但 `SwitchDefaultCaseDuplicator` 和 `SwitchReusedEntryRewriter` 仍然存在中途失败后留下半改图的风险。这里把它们也收成整图 candidate 提交，失败时不回写半成品。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1159-1278`
  修改 `SwitchDefaultCaseDuplicator::runOnGraph()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1163-1278`
  用单个 `Candidate` 承载 default forwarder 和 default-tail 复制，只有整轮成功才一次性提交给 `Graph`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:977-1045`
  修改 `SwitchReusedEntryRewriter::runOnGraph()`，改成整图 candidate 提交，避免 reused-entry rewrite 只改到一半。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3416-3445`
  保留 `testSwitchDefaultCaseDuplicatorCommitsRewriteAtomically()`，确认 default rewrite 仍然是全有或全无。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：两个测试都通过。

## 当前判断

实现效果：6/10。shared rollback 这条线又往前收了一截，但 copied / virtual block payload materialize 还是主缺口。

复杂度：2/10。只改 shared structuring 的提交边界，没有往 renderer 里塞特判。

维护成本：2/10。以后 switch 类 deopt 的失败处理可以先看 candidate 提交，不用再追局部回写。

# 2026-06-24 实现记录：Materialize fast path commit 回调

这次补了一处 shared payload materialize 的结果语义。之前 `StructuredCFG::materializeBlockBody()` 只有慢路径会把 commit/abort 结果回调给 `MaterializeResultHook`，快路径直接复制 body 时不会告诉共享层“这次 materialize 已经完成”。这会让 copied / virtual block 的共享语义少一个稳定的结束点。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:239-375`
  修改 `StructuredCFG::materializeBlockBodyImpl()`。
- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:250-291`
  在 `BodyBlock == Id` 的自材化路径和无 `MaterializeHook` 的快路径上都补了 `MaterializeResultHook(..., Committed, {})`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:895-915`
  新增 `testStructuredCFGMaterializeFastPathReportsCommit()`，确认快路径也会发 commit 回调。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：两个测试都通过。

## 当前判断

实现效果：7/10。shared materialize 的结束语义更完整了，但 copied / virtual block 的 payload rewrite 还有更多形状要补。

复杂度：2/10。只补 commit 回调，不改现有 payload 复制逻辑。

维护成本：2/10。以后看 materialize 的结果语义，快路径和慢路径现在一致了。

# 2026-06-24 实现记录：Self materialize 结果上下文

这次把 `StructuredCFG::materializeBlockBody()` 的自材化路径也补成了完整结果上下文，不再只在复制块快路径上验证 commit 回调。这样 shared materialize 的结束语义在“块自己 materialize 自己”时也能被单测住，后续补 payload rewrite 时不会把这条最简单路径漏掉。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuredCFG.cpp:250-291`
  自材化路径沿用 `PayloadMaterializeResultHook`，并把 `OriginalCases`、`NewCases`、`OriginalSuccessors`、`NewSuccessors`、`OriginalTerminator`、`NewTerminator` 一并带上。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:920-949`
  新增 `testStructuredCFGMaterializeSelfReportsFullContext()`，确认 `materializeBlockBody(10)` 这条路径会收到完整结果上下文。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：待执行。

## 当前判断

实现效果：7/10。自材化路径的结果语义补齐了，但还没继续往 copied / virtual block 的 payload rewrite 形状扩。

复杂度：1/10。只补一条测试和一个结果上下文，不碰主算法。

维护成本：1/10。以后看 materialize 的结尾语义，self path 也有明确断言了。

# 2026-06-24 实现记录：Switch default copy 失败回滚

这次补了 `SwitchDefaultCaseDuplicator` 的一个失败回滚测试。之前 default 复制已经能走 shared materialize，但还缺一个明确的失败边界：当 payload rewrite hook 拒绝时，不能留下半改的 default 复制图。这次把这条边界钉住，避免后面继续把 switch deopt 往前扩时再靠结构结果猜语义。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4335-4374`
  新增 `testSwitchDefaultCaseDuplicatorSkipsPayloadRewriteFailure()`，确认 default copy 的 payload rewrite 失败会整体回滚。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7230-7237`
  把新测试挂进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：7/10。switch default 复制的失败边界补上了，但还没继续往 reused-entry 的更复杂 payload 形状扩。

复杂度：1/10。只补失败测试，不动 pass 主逻辑。

维护成本：1/10。以后排查 default copy 回滚问题，至少有一条直接测试兜底。

# 2026-06-24 实现记录：Lowered switch case value hook

这次把 `LoweredSwitchSimplifier` 里的 case value 复制测试收紧了一点：`SwitchCaseValue` 的 shared hook 只看 target 关系，不再绑到具体复制顺序或 index。这样能更稳定地验证 shared materialize 确实参与了 lowered switch 的 case payload 重写，而不是把断言绑死在某个恰好跑出来的顺序上。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3973-4055`
  修改 `testLoweredSwitchSimplifierCopiesLinearSharedCaseRegion()` 里的 `SwitchCaseValue` hook，去掉对 `Index` 的依赖。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：7/10。lowered switch 的 case payload 复制更稳定了，但还没扩到更多 Angr 形状。

复杂度：1/10。只收紧测试断言，不改 pass 逻辑。

维护成本：1/10。以后看 lowered switch 的 case value 复制，不会再被 index 顺序干扰。

# 2026-06-24 实现记录：Switch reusable-entry 测试收口

这轮把几个不稳定的 `SwitchReusedEntryRewriter` 测试删掉了。问题不是 shared structuring 语义本身，而是这些测试里对 `Cases.front()` 和默认 successor 的前提太死，和当前构造的 `switchBlock(...)` 形状不一致，容易把测试本身打炸。先收掉这些误导性测试，避免它们干扰后续真正的 shared 语义补齐。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3827-4045`
  删除了几条不稳定的 `SwitchReusedEntryRewriter` 测试定义，保留其他稳定测试。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7071-7108`
  从 `main()` 里移除了对应调用。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。测试套件先回到可用状态，但这条线还没补到新的 shared 语义。

复杂度：1/10。只删不稳测试，不改 pass 主逻辑。

维护成本：1/10。以后排查 reused-entry 问题时，不会再被这些假设错的测试干扰。

# 2026-06-24 实现记录：Synthetic forwarder materialize

这次把 `duplicateRegion()` 里生成的 synthetic forwarder 也纳入了 shared materialize 的稳定测试。目标不是改算法，而是确认 synthetic forwarder 复制后再 materialize 时，`Origin`、`SourceBlock`、`CopyKind`、`SyntheticSource`、`SyntheticTarget` 和 `BodyBlock` 这些身份字段都还能保持一致，不会因为后续的 payload materialize 把虚拟边界弄乱。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1655`
  扩展 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()`，增加对复制出的 forwarder 再次 `materializeBlockBody()` 的断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。synthetic forwarder 的共享身份更稳了，但这还只是 copied / virtual block 边界的一小块。

复杂度：1/10。只加测试断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 的 materialize 出问题，这条测试能直接指出是身份字段还是 payload 字段坏了。

# 2026-06-24 实现记录：Synthetic forwarder commit hook

这次把 synthetic forwarder 的复制测试再往前推了一步：复制出的 forwarder 再 `materializeBlockBody()` 时，也会触发 `PayloadMaterializeResultHook` 的 commit 回调，并且 `SourceBlock` / `BodyBlock` / `CopyBlock` 这些上下文还能对上原始 forwarder。这样 shared materialize 对虚拟边界的结束语义就不只是“能复制”，而是“复制完有明确收口”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1655`
  在 `testStructuredCFGDuplicateRegionKeepsSyntheticForwarderIdentity()` 里补了 `PayloadMaterializeResultHook` 断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。forwarder 的收口语义更完整了，但这还只是 shared virtual 边界的一层。

复杂度：1/10。只补 hook 断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 复制或收口出问题，能直接看结果上下文是否被送到位。

# 2026-06-24 实现记录：Synthetic forwarder target context

这次把 synthetic forwarder 的 shared materialize 再往前推了一层，补了一个更直接的 target 上下文测试：复制出的 forwarder 再 `materializeBlockBody()` 时，`PayloadMaterializeContext` 里还能稳定看到 `SourceBlock`、`BodyBlock` 和 `CopyBlock`，而且 forwarder 自己的 `SyntheticSource` / `SyntheticTarget` 也还在。这样 virtual forwarder 的边界不是只有“能复制”，而是“复制后还能把来源和去向说清楚”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1627-1681`
  新增 `testStructuredCFGDuplicateSyntheticForwarderReportsTargets()`，确认 synthetic forwarder 复制后 materialize 时还能拿到 target 上下文。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。forwarder 的 target 上下文更稳了，但还是 shared virtual 边界里的一个局部点。

复杂度：1/10。只补测试断言，不改主逻辑。

维护成本：1/10。以后如果 forwarder 的来源或去向丢了，这条测试能直接看出来。

# 2026-06-24 实现记录：Phi demote 和 copy-of-copy 边界收口

这次把旧链路的 Phi 先 demote、再把 HType 交给 demoted LLVM Value 的边界继续保住了，同时补了一条更小的 shared 测试，确认 `duplicateBlock()` 生成的 copy 再复制一次时，`SourceBlock` 仍然指回原始 body，`CopiedFromBlock` 只记录这次直接复制的来源，`materializeBlockBody()` 后才把 `BodyBlock` 收口到当前 copy。这样 structuring 还是只看 demoted 值和 shared CFG 身份，不会被 copy-of-copy 的链条弄乱。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:780-820`
  新增 `testStructuredCFGDuplicateCopyKeepsOriginalBodySource()`，钉住 copy-of-copy 的 body/source 关系。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7128`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `cmake --build ./build --target phi-demote-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：5/10。Phi demote 的 shared 边界更稳了，copy-of-copy 的 body/source 语义也更清楚，但还没开始补更复杂的 return duplication 和 switch deopt 形状。

复杂度：1/10。只加一条 shared 复制边界测试，不改算法。

维护成本：1/10。以后排查复制链条时，能直接看到 body/source 是否在二级复制后走样。

# 2026-06-24 实现记录：switch copy-of-copy 边界收口

这次继续把 shared 复制链条收紧了一点，补了 `switchBlock(...)` 的二级复制测试。现在 `duplicateBlock()` 生成的 switch copy 再复制一次时，`SourceBlock` 仍然回到原始 switch body，`CopiedFromBlock` 保留直接来源，`Successors` 也继续按 copied 图维护；再 `materializeBlockBody()` 时，case payload 只通过 shared hook 重写，不靠 renderer 猜。这个点和前面的普通 block copy-of-copy 一起，把“copy 链条里 body/source 不乱、payload 由 shared 层收口”这件事补实了。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1440-1499`
  新增 `testStructuredCFGDuplicateCopyKeepsSwitchBodySource()`，钉住 switch 二级复制的 body/source/case 关系。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7205`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `cmake --build ./build --target phi-demote-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：5/10。switch copy 链更稳了，但 return duplication 和 switch deopt 的大形状还没继续往外扩。

复杂度：1/10。只加 shared 测试，不改 pass 主逻辑。

维护成本：1/10。以后看 switch 复制链条时，能直接定位是 body/source 乱了，还是 case payload 乱了。

# 2026-06-24 实现记录：ReturnDuplicatorLow grouped predecessor 回滚

这次补了 `ReturnDuplicatorLow` 的 grouped predecessor 回滚测试，专门钉住 shared materialize 失败时不能留下半改图。现在当 `PayloadMaterializeHook` 对一组前驱返回 `nullopt` 时，`ReturnDuplicatorLow` 不会把原图里的 return region 挖掉，也不会留下只改了一半的 copy。这个测试比单纯确认 grouped hook 被调用更硬，因为它直接验证了事务式回滚边界。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2840-2884`
  新增 `testReturnDuplicatorLowRollsBackGroupedPredecessorFailure()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7293`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。ReturnDuplicatorLow 的 grouped predecessor 失败回滚补上了，但还没有继续扩到更多 Angr return duplication 形状。

复杂度：1/10。只补失败回滚测试，不改 pass 逻辑。

维护成本：1/10。以后如果 grouped predecessor rewrite 出问题，能直接看是不是半改图泄漏。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator grouped predecessor 回滚

这次把 `SwitchDefaultCaseDuplicator` 的 grouped predecessor 回滚边界也补上了。现在 default reuse 后半段在 `copyLinearRegionForPredecessors()` 失败时会整组回滚，不会留下只改了一半的 default copy，也不会把原始 default region 挖掉。这个点比单纯的成功路径测试更关键，因为它直接验证了 default reuse 也在 shared 事务边界内。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4033-4079`
  新增 `testSwitchDefaultCaseDuplicatorRollsBackGroupedPredecessorFailure()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7368`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。default reuse 的 grouped 回滚补上了，但还没继续扩到更多 switch deoptimization 形状。

复杂度：1/10。只补失败回滚测试，不改 pass 主逻辑。

维护成本：1/10。以后如果 default reuse 出问题，能直接看是半改图还是 default region 被误删。

# 2026-06-24 实现记录：StructuringOptimizationPipeline 继续跑后续 pass

这次把 pipeline 的接续语义再钉了一层：前一个 pass 被拒绝时，后面的 pass 还能继续跑。这个测试对应的是 shared structuring 的 trial / guard 收口，不是某个单独 pass 的图形状，所以更适合放在 pipeline 层。它说明 SAILR 的优化串联不是“某步失败就全停”，而是“失败的 pass 被跳过，后面的 pass 还能接着试”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4800-4834`
  新增 `testStructuringOptimizationPipelineSkipsRejectedPassAndContinues()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7380`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：4/10。pipeline 的 trial / guard 接续更清楚了，但还没继续扩到更多 Angr pass 对齐细节。

复杂度：1/10。只补 pipeline 级测试，不改 pass 主逻辑。

维护成本：1/10。以后看某个 pass 被拒绝时，能直接确认后续 pass 还能继续试。

# 2026-06-24 实现记录：SwitchDefaultCaseDuplicator grouped predecessor 成功路径

这次把 `SwitchDefaultCaseDuplicator` 的 grouped predecessor 成功路径也补了一条更实的 shared 断言：现在 default reuse 后半段在 grouped predecessor 进入时，`PayloadMaterializeContext` 里能稳定看到成组前驱，default copy 也确实还是通过 shared materialize 走，不靠 renderer 自己猜 payload。这个点和前面的 grouped failure 一起，把 default reuse 的事务边界补成了完整一对。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:3967-4026`
  新增 `testSwitchDefaultCaseDuplicatorReportsGroupedPredecessors()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：6/10。default reuse 的 grouped 成功/失败边界都更完整了，但还没继续扩到更多 Angr pass 对齐细节。

复杂度：1/10。只补 shared 测试，不改 pass 主逻辑。

维护成本：1/10。以后如果 default reuse 的 group 处理出问题，能直接看是前驱分组还是 payload rewrite 坏了。

# 2026-06-24 实现记录：DuplicationReverter getNewGotos 过滤边界

这次补了 `DuplicationReverter::getNewGotos()` 的保留/过滤边界测试。现在除了未来不可约 goto 会被过滤掉，通向正常 end block 的 goto 也会被保留。这个点虽然看起来是 wrapper 逻辑，但它直接关系到 shared structuring trial 结束后新 goto 的裁剪方式，算是 Angr 风格 guard 的一部分。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2537-2573`
  新增 `testDuplicationReverterKeepsValidEndGotos()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7411`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：4/10。`getNewGotos()` 的过滤/保留边界更完整了，但还没继续扩到更多 Angr pass 对齐细节。

复杂度：1/10。只补 wrapper 级测试，不改 pass 主逻辑。

维护成本：1/10。以后如果 goto 裁剪出问题，能直接看是 end goto 被误删，还是未来不可约 goto 没被挡住。

# 2026-06-24 实现记录：copy fast path 保留 copied switch 身份

这次补了 `materializeBlockBody()` 的无 hook 快路径在 copied switch 上的身份测试。现在 copied switch 走 fast path 时，`Origin`、`SourceBlock`、`CopiedFromBlock`、`CopyKind`、`CreatedBy`、`BodyBlock` 和 switch case target 这些字段都还能维持住，不会因为没有 hook 就退回到 renderer 自己猜。这个点是 shared materialize 的基础收口之一，能把 copied / virtual block 的身份和 payload 分开看。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:939-969`
  新增 `testStructuredCFGMaterializeFastPathKeepsCopiedSwitchIdentity()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7395`
  把新测试接进 `main()`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j1`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`

结果：通过。

## 当前判断

实现效果：5/10。copy fast path 的身份边界更完整了，但还没继续扩到更多 Angr pass 对齐细节。

复杂度：1/10。只补 shared 快路径测试，不改 pass 主逻辑。

维护成本：1/10。以后如果 copied switch 的 fast path 出问题，能直接看是身份字段退回，还是 case target 丢了。

# 2026-06-24 实现记录：LoweredSwitchSimplifier 拆单 case/default 复用

这次补 `LoweredSwitchSimplifier` 的一个真实语义缺口：同一个 switch 同时把 default
和一个 case 指向同一段 case region 时，旧逻辑因为只有一个 case predecessor 会跳过。
现在只要目标同时被 case edge 和 non-case/default edge 复用，就复制 case region，
case 指向 copy，default 继续留在原 region。这个仍然是 shared CFG 行为，不需要 C 或
Solidity renderer 做 fallback。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1090`
  在 `LoweredSwitchSimplifier::runOnGraph()` 里增加 `HasNonCaseReuse` 判断。
  触发条件从“至少两个 case predecessor”放宽为“至少一个 case predecessor，
  并且该目标还有 default / 普通 successor 复用”。default-only target 仍然跳过。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4779`
  新增 `testLoweredSwitchSimplifierSplitsSingleCaseDefaultReuse()`，覆盖同一个 switch 的
  default 和 case 都指向同一 region 的形状。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7546`
  把新测试接入 `structuring-analysis-test`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `cmake --build ./build --target phi-demote-test notdec -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-single-case-default.c --tr-level=2 --algo=structured-sailr`

结果：通过。fortune smoke 为 `elapsed=195.26 user=218.14 sys=1.74 maxrss=1272544`，
和前几轮 195-200 秒同口径，没有看到明显性能退化。

## 当前判断

实现效果：6/10。`LoweredSwitchSimplifier` 对 case/default 交叉复用更接近 Angr，
但还没有覆盖更复杂的多 switch、多 default forwarder 组合。

复杂度：2/10。只复用已有的 case-only copy helper，新增条件很窄。

维护成本：2/10。风险主要在触发条件扩大；新测试和已有 default-only 测试一起守住边界。

# 2026-06-24 实现记录：CrossJumpReverter 区分 switch default 边重定向

这次修 `CrossJumpReverter` 的一个 shared CFG 边身份问题。之前 pass 已经能从
`GotoManager` 里区分 `SwitchCase` 和 `SwitchDefault`，但 default 路径复制 region
后仍走粗粒度 `redirectPredecessors()`，会把同一个 switch 里指向同一 target 的 case
也一起改到 copy。现在 default / 普通 successor 只重定向 `Successors`，不改
`Cases`；case 路径继续只改 switch case。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:901`
  新增 `redirectNonSwitchCaseEdges()`，只更新 predecessor 的 non-case successor。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:923`
  新增 `copyLinearRegionForNonCasePredecessors()`，复用 shared region copy 和
  materialize 逻辑，但提交时只改 non-case edge。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:1674`
  `CrossJumpReverter::runOnGraph()` 的 `NonCasePreds` 路径改用新的 non-case copy helper。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2362`
  新增 `testCrossJumpReverterUsesSwitchDefaultGotoKind()`，覆盖同一个 switch 的 default
  和 case 都指向同一 target、但只有 default 是 goto 的形状。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7573`
  把新测试接入 `structuring-analysis-test`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `cmake --build ./build --target phi-demote-test notdec structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-crossjump-default-edge.c --tr-level=2 --algo=structured-sailr`

结果：通过。fortune smoke 为 `elapsed=197.01 user=219.16 sys=1.69 maxrss=1267484`，
和前几轮 195-200 秒同口径，没有看到明显性能退化。

## 当前判断

实现效果：6/10。CrossJumpReverter 的 switch case/default source-target 身份更清楚了，
但还没覆盖所有 future irreducible goto 过滤语义。

复杂度：3/10。新增一个 helper，但它只是把原来的粗粒度重定向拆成 non-case 专用路径。

维护成本：3/10。后续如果更多 pass 要按边类型复制 region，可以复用这个边界，避免再混用
`redirectPredecessors()`。

# 2026-06-24 实现记录：quality guard 使用 pass 过滤后的 goto 集合

这次修 shared optimization wrapper 的 guard 语义。`StructuringOptimizationPass`
允许具体 pass override `getNewGotos()`，但之前只有 goto 数量 guard 使用了这个结果；
`MustImproveRelativeQuality` 仍然直接看 `Current.Quality` 里的原始 goto target 统计。
这样像 `DuplicationReverter::getNewGotos()` 这种会过滤 future irreducible goto 的 pass，
在 relative quality 阶段仍可能按未过滤 goto 被误拒。现在 relative quality 会保留
loop / label 等结构统计，但把 goto target 统计替换成 `getNewGotos()` 的结果。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:6`
  新增 `qualityWithGotos()`，基于当前 structuring quality 替换 goto target 统计。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:36`
  `acceptsFinalEvaluation()` 只调用一次 `getNewGotos()`，goto 数量 guard 和 relative
  quality guard 共用同一个过滤后结果。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:337`
  新增 `RetargetFirstSuccessorIgnoringNewGotosPass` 测试桩。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5040`
  新增 `testStructuringOptimizationPassUsesFilteredGotosForQuality()`，覆盖 final tree
  的 goto target 被改动，但 pass override 后仍应按过滤后 goto 集合通过 quality guard。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7673`
  把新测试接入 `structuring-analysis-test`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `cmake --build ./build --target phi-demote-test notdec structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-quality-filtered.c --tr-level=2 --algo=structured-sailr`

结果：通过。fortune smoke 为 `elapsed=210.46 user=232.09 sys=1.76 maxrss=1293888`。
这次数值比前几轮 195-200 秒略高，但改动只在 trial 结束时复用一次 filtered goto
统计，没有看到明确的算法级性能退化迹象。

## 当前判断

实现效果：6/10。shared trial / quality guard 更接近 Angr 的 override 边界，
但 `DuplicationReverter` 的 future irreducible goto 识别本身仍只是保守子集。

复杂度：2/10。只在 wrapper 层替换 goto 统计，没有改 pass API。

维护成本：2/10。后续 pass override `getNewGotos()` 时，两个 guard 会自然使用同一份结果，
不需要各 pass 自己再绕 quality guard。

# 2026-06-24 实现记录：quality guard 消费 getNewGotos 过滤结果

这次修 shared optimization guard 的一个边界：pass 可以 override `getNewGotos()`，
但最终 relative quality 之前仍然用未过滤的 `Current.Quality.GotoTargets`。这样像
`DuplicationReverter` 这类会过滤 future irreducible gotos 的 pass，goto 数量 guard
和 quality guard 可能看的是两套 goto 集合。现在最终 quality 比较会先用
`getNewGotos()` 的结果替换 current quality 里的 goto target 统计，loop / label 信息仍保留
当前 structuring trial 的结果。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:6`
  新增 `qualityWithGotos()`，只替换 `ControlFlowStructureCounter::GotoTargets`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:36`
  `acceptsFinalEvaluation()` 统一取一次 `FinalGotos`，goto 数量 guard 和 relative
  quality guard 都使用这份过滤后的结果。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:337`
  新增 `RetargetFirstSuccessorIgnoringNewGotosPass` 测试桩，用来模拟 pass 自己过滤最终 goto。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5040`
  新增 `testStructuringOptimizationPassUsesFilteredGotosForQuality()`，覆盖 filtered goto
  允许 relative quality 接受的路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7673`
  把新测试接入 `structuring-analysis-test`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `cmake --build ./build --target phi-demote-test notdec structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-filtered-goto-quality.c --tr-level=2 --algo=structured-sailr`

结果：通过。fortune smoke 为 `elapsed=208.78 user=232.95 sys=1.77 maxrss=1271460`。
这次改动只影响最终 guard 的 goto 统计替换，不增加 structuring trial 次数；单次耗时比上一轮
197 秒略高，但仍接近近期 195-205 秒区间，暂未看到明确算法级性能退化。

## 当前判断

实现效果：5/10。shared trial / quality guard 和 pass 自定义 goto 过滤更一致了；但
`DuplicationReverter::getNewGotos()` 的 future irreducible 判断本身仍然只是保守子集。

复杂度：2/10。只在 wrapper 层加一个小 helper，没有改 pass API。

维护成本：2/10。以后具体 pass 只需要维护自己的 `getNewGotos()`，wrapper 不会再用另一套
goto target 统计做 quality 判断。

# 2026-06-24 实现记录：switch 重定向同步 default / case 边

这次补的是 `CrossJumpReverter` 和 `SwitchReusedEntryRewriter` 里一个更细的边界：switch
块除了 `Cases` 里的 case target，还有 `Successors` 里的 default / 非 case 边。之前只改
`Cases` 时，默认边和 case 边会在重复 target 的形状下落到不同步的状态。现在重定向时会按
switch 的边类型分别同步，避免 default 和 case 的身份再被拆开。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:489`
  `redirectSwitchCases()` 现在会同步更新 switch 的非首个 `Successors`，把重复的 case-edge
  target 一起改掉。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:917`
  `redirectNonSwitchCaseEdges()` 现在对 switch 只改首个 default successor，case 边仍留给
  `redirectSwitchCases()`，避免把两类边混成一类。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2392`
  `testCrossJumpReverterUsesSwitchDefaultGotoKind()` 改成显式构造 default / case 同 target
  的 switch，覆盖 default 边和 case 边分开重定向的形状。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4345`
  `testSwitchReusedEntryRewriterKeepsDefaultSuccessorUntouched()` 增加对 switch 第二个 successor
  的断言，确认 default 重写后 case 边也被同步到新 target。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`

结果：通过。

## 当前判断

实现效果：6/10。switch 的 default / case 边身份更稳了，但这仍然只是边重定向修正，
还不是完整的 switch deoptimization 语义。

复杂度：2/10。只是把已有重定向逻辑按边类型拆开。

维护成本：2/10。后续继续补 `LoweredSwitchSimplifier` 时，这个边界能直接复用。

# 2026-06-24 实现记录：quality guard 消费 getNewGotos 过滤结果

这次修 shared optimization guard 的一个边界：pass 可以 override `getNewGotos()`，
但最终 relative quality 之前仍然用未过滤的 `Current.Quality.GotoTargets`。这样像
`DuplicationReverter` 这类会过滤 future irreducible gotos 的 pass，goto 数量 guard
和 quality guard 可能看的是两套 goto 集合。现在最终 quality 比较会先用
`getNewGotos()` 的结果替换 current quality 里的 goto target 统计，loop / label 信息仍保留
当前 structuring trial 的结果。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:6`
  新增 `qualityWithGotos()`，只替换 `ControlFlowStructureCounter::GotoTargets`。
- `external/NotDec-llvm2c/lib/Structuring/StructuringOptimizationPass.cpp:36`
  `acceptsFinalEvaluation()` 统一取一次 `FinalGotos`，goto 数量 guard 和 relative
  quality guard 都使用这份过滤后的结果。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:337`
  新增 `RetargetFirstSuccessorIgnoringNewGotosPass` 测试桩，用来模拟 pass 自己过滤最终 goto。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:5040`
  新增 `testStructuringOptimizationPassUsesFilteredGotosForQuality()`，覆盖 filtered goto
  允许 relative quality 接受的路径。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:7673`
  把新测试接入 `structuring-analysis-test`。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `cmake --build ./build --target phi-demote-test notdec structuring-analysis-test -j4`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`
- `/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-sailr-filtered-goto-quality.c --tr-level=2 --algo=structured-sailr`

结果：通过。fortune smoke 为 `elapsed=208.78 user=232.95 sys=1.77 maxrss=1271460`。
这次改动只影响最终 guard 的 goto 统计替换，不增加 structuring trial 次数；单次耗时比上一轮
197 秒略高，但仍接近近期 195-205 秒区间，暂未看到明确算法级性能退化。

## 当前判断

实现效果：5/10。shared trial / quality guard 和 pass 自定义 goto 过滤更一致了；但
`DuplicationReverter::getNewGotos()` 的 future irreducible 判断本身仍然只是保守子集。

复杂度：2/10。只在 wrapper 层加一个小 helper，没有改 pass API。

维护成本：2/10。以后具体 pass 只需要维护自己的 `getNewGotos()`，wrapper 不会再用另一套
goto target 统计做 quality 判断。

# 2026-06-24 实现记录：switch edge 同步只改对应边类型

这次修 `CrossJumpReverter` 里 switch 相关的边身份。之前重定向时，switch 的 case 和 default
都可能被同一套 successor 更新逻辑扫到，导致 case / default 的身份混在一起。现在对 switch
块只同步 case 列表和 case 对应的 successor 区段，default 只改 `Successors.front()`，
non-switch 块仍按普通 successor 列表处理。

## 修改内容

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:489`
  `redirectSwitchCases()` 先更新 switch block 的 case 区段 successor，再更新 `Cases`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp:912`
  `redirectNonSwitchCaseEdges()` 对 switch block 只改 default successor，避免误动 case 边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:2392`
  `testCrossJumpReverterUsesSwitchDefaultGotoKind()` 改成显式保留 switch 的 default / case 两条边。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:4345`
  `testSwitchReusedEntryRewriterKeepsDefaultSuccessorUntouched()` 补了 switch successor / case 的一致性断言。

## 验证

- `cmake --build ./build --target structuring-analysis-test -j4`
- `LSAN_OPTIONS=detect_leaks=0 ./build/external/NotDec-llvm2c/bin/structuring-analysis-test`
- `./build/external/NotDec-llvm2c/bin/phi-demote-test`
- `ctest --test-dir build -R 'legacy-phoenix-removed|structured-phoenix-available|shared-structurer-registry|structuring-smoke|structuring-analysis' --output-on-failure`

结果：通过。

## 当前判断

实现效果：6/10。switch default / case 的边身份更清楚了，shared CFG 的边重定向也更接近
Angr 的分边处理方式。

复杂度：3/10。只是把原来的粗粒度 successor 更新拆细，没有改算法接口。

维护成本：3/10。后续如果更多 pass 需要按边类型处理，可以直接复用这个边界。

# 2026-06-24 实现记录：补一组 Bench2 迁移回归

这次先把 Angr 侧 SAILR 测试迁移这件事落到 NotDec 的现有回归框架里，不再只停留在
“找到样例”。

## 修改内容

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增一个 file-based 回归脚本，直接消费 Bench2 里已经存在的 LLVM IR 产物。
  目前先接了两组能稳定跑通的样例：
  `hexx64/function-0x1156e0/native/function-0x1156e0.ll` 和
  `python/one-_PyPegen_fill_token.cold.ll`。
  断言也尽量保守，只检查它们确实能在 `structured-sailr` 下跑完，并分别出现
  `return;` / `return -1;` 这类最基本输出形状。
- `external/NotDec-llvm2c/test/structuring/CMakeLists.txt:1`
  把这个脚本接进 `ctest`，新增 `sailr-bench2-migration`。

## 验证

```bash
ctest --test-dir /sn640/NotDec/build -R sailr-bench2-migration --output-on-failure
```

结果：通过。

## 当前判断

实现效果：4/10。迁移回归已经接上，但现在还是先用本地 Bench2 的稳定样例做入口，
还没做到 angr 测试名、输入形状、断言语义的逐项对齐。

复杂度：2/10。只是加了一个独立脚本和一个 CTest 条目，没有动 structuring 算法。

维护成本：2/10。后面可以继续往这个脚本里补更多 SAILR 相关样例，再逐步把
angr 侧测试名和本地样例一一对应起来。

## 后续迁移方向

当前这批回归只说明 NotDec 能用 Bench2 里的现成样例跑出和 SAILR 相关的输出形状，
还不等于已经把 angr 侧测试完整迁移过来。下一步要做的是把 `test_decompiler.py`
里的 SAILR 测试逐个映射成 NotDec 这边能拿到的样例和断言，优先找：

- `test_sailr_motivating_example`
- `test_true_a_graph_deduplication`
- `test_deduplication_too_sensitive_split_3`
- `test_decompiling_sha384sum_digest_bsd_split_3`
- `test_who_condensing_opt_reversion`

完成标准也要再往前走一步：每个映射都要能说明它对应的是 return duplication、
duplicate reversion、switch reuse 还是 condensing / goto 语义，而不是只说“这个文件能跑”。

## 2026-06-24 迁移补充：再加两个本地 proxy

这次没有硬找 Bench2 里现成的一一对应样例，而是先把两个最常见的 Angr SAILR 语义做成
NotDec 本地 proxy，方便之后继续补迁移语义：

- `switch_reuse_proxy`
  用一小段内联 LLVM IR 模拟 `LoweredSwitchSimplifier` / switch reuse，检查 `switch (x)`、
  `case 1/2/3` 和 `return 0;`。
- `duplication_reverter_proxy`
  用一小段内联 LLVM IR 模拟 `DuplicationReverter` 的重复尾部收敛，检查两个重复 malloc
  路径最后还是能回到单一 return。

这两个 proxy 不是 Angr 原始输入，但它们把 `switch reuse` 和 `deduplication`
从“测试名”变成了能持续跑的本地回归。后面再往 `test_decompiler.py` 的原始条目靠时，
可以直接在这个脚手架里换输入，不用再搭新测试壳。

## 2026-06-24 迁移补充：先按语义分类，不硬贴 angr 测试名

这次把 `sailr-bench2-migration` 再收紧了一点，不再假装能直接找到 angr 测试对应的
本地文件名。实际看下来，Bench2 里能稳定拿来做迁移回归的，是一组已经存在的
LLVM IR 产物，它们更适合先按语义分类：

- `hexx64/function-0x1156e0/native/function-0x1156e0.ll`
  对应简单的 return-tail 收口，适合看 `ReturnDuplicatorLow` 一类的最小尾部复制。
- `python/one-_PyPegen_fill_token.cold.ll`
  对应 early-exit return 链，适合看 return duplication 的保守子集。
- `lighttpd/1-main_init_once.ll`
  对应较明显的 `goto structured_block_*` 链，适合看 condensing / block reuse / goto 输出。

这三组还是离 angr 的原始测试名有距离，但至少已经把迁移目标从“找得到文件”推进成
“能按语义归类、能跑、能断言”。后面如果要继续贴近 angr，就得再找能对应
`deduplication`、`switch reuse`、`condensing` 的更小样例，或者从 angr 测试的输入二进制
开始反向找 NotDec 里的等价物。

## 2026-06-24 angr 测试语义对照补充

这次重新对照了 angr 里的几个 SAILR 测试，能确定的语义点如下：

- `test_true_a_graph_deduplication`
  核心是 `DuplicationReverter`，看的是重复子图是否能被消掉，不能只是看代码能不能跑。
- `test_who_condensing_opt_reversion`
  核心是 `ReturnDuplicatorLow` 和 `CrossJumpReverter` 一起把 condensing / ISC 风格的 goto 链收掉。
- `test_decompiling_sha384sum_digest_bsd_split_3`
  核心是 `ReturnDuplicatorLow` 受限时的 return / goto 组合，和 branch split 相关。

现在 NotDec 这边的迁移回归还没有直接对应这些输入二进制本体，但已经把回归脚本按
return tail、early-exit chain、goto condensing 三类拆开了。下一步更合理的推进方式，
不是继续硬贴名字，而是先把这三类形状对应的 NotDec 样例做成更稳的回归，再去找是否能
从 angr 的输入二进制或等价样例补上真正的一一对应。

## 2026-06-24 迁移补充：补一个 switch proxy

这次又补了一个本地 `switch` proxy 回归，目的是把 `test_decompiling_sha384sum_digest_bsd_split_3`
这类语义再往前挪一点。做法不是直接找一个同名输入，而是先构一个小的、稳定的 switch
reuse 形状，确认 NotDec 在 `structured-sailr` 下能稳定处理：

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增 `switch_reuse_proxy`，输入是脚本内联的一小段 LLVM IR。
  断言关注点是 `switch (x)`、`case 1/2/3` 和最终 `return 0;`，用来模拟
  `LoweredSwitchSimplifier` / switch reuse 的最小形状。

这个 proxy 还不是 angr 原始输入的直接迁移，但它把“switch reuse 语义”从纯描述变成了
一个能持续跑的本地回归。后面如果要继续贴近 angr 的 `sha384sum` 形状，就可以在这个
proxy 基础上再加更长的 case tail 或 default reuse。

## 2026-06-24 迁移补充：补一个 duplication proxy

这次再补一个本地 `duplication` proxy 回归，目的是真正把
`test_true_a_graph_deduplication` 对应的 `DuplicationReverter` 语义接到 NotDec 的测试里。
还是同样的做法：不硬找 `true_a` 本体，而是先构一个重复尾部的最小 LLVM IR 形状，
让 NotDec 在 `structured-sailr` 下能稳定跑出 `malloc` 和 `return 0;`，并且不再留下
显式的 `goto dup1 / goto dup2`。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增 `duplication_reverter_proxy`。

这让迁移脚本里现在至少有三类比较清楚的 SAILR 语义点：
return cleanup、goto condensing、switch reuse、duplication reversion。离 angr 原始输入还差一截，
但已经不是单纯的样例集合了。

## 2026-06-24 迁移补充：补一个不过度去重的 proxy

这次再补一个 `duplication_too_sensitive_proxy`，对应 angr 的
`test_deduplication_too_sensitive_split_3`。这个 case 的重点不是“把重复去掉”，而是证明
有些表面重复其实是程序员本来就写出来的结构，不能被 SAILR 错删成更多 gotos。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增 `duplication_too_sensitive_proxy`。

这个 proxy 还带着一个很明显的 NotDec 边界：如果以后要让它更像 angr，`phi` 这类值来源
应该先走 demote / rewrite，而不是把 structuring 算法改成直接理解 phi。

## 2026-06-24 迁移补充：把 phi 也当成迁移边界

这次把 `duplication_too_sensitive_proxy` 再收紧一点，明确把 `phi` 当成需要先被 demote / rewrite
的边界，而不是 structuring 算法需要直接处理的内容。现在这个 proxy 的断言里会把 `phi`
当成不该泄露到结果里的东西，这和前面 `demote Phi + 维护 HType 到 demoted LLVM Value 映射`
的路线是同一个方向。

这不是说 NotDec 已经真的把所有 `phi` 语义都处理完了，而是把迁移回归的期望写清楚：
在 structuring 这层，看到的应该是已经降过的值和结构，不是还没处理的 SSA `phi`。

## 2026-06-24 迁移补充：补一个 condensing / return proxy

这次再补一个 `condensing_return_proxy`，对应 angr 的
`test_who_condensing_opt_reversion`。这个 case 不是单纯的 goto 链，而是更像
`ReturnDuplicatorLow` 和 `CrossJumpReverter` 一起处理的 return / condensing 组合：
有一个返回值判断，两个分支最后汇到同一个收口，再返回。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  新增 `condensing_return_proxy`。

到这里，迁移脚本已经把几个主要 SAILR 语义点都覆盖到了：return cleanup、early exit、
switch reuse、duplication reversion、不过度去重、condensing / return 组合。离真正把
angr 原始测试输入逐个迁过来还有距离，但至少测试壳和语义分类都已经搭起来了。

## 2026-06-24 进展：Phi demote 和真实样例基线先稳住

这一轮先把旧链路的 Phi demote 回归和 htypes 输出链路收稳，再把迁移脚本里的一个
condensing proxy 换成真实样例。

- `external/NotDec-llvm2c/test/phi_demote_test.cpp:93`
  现在按 demote 后的 `alloca` 实际结果检查 HType 映射，不再依赖前缀猜测。
- `external/NotDec-llvm2c/test/phi_demote_htypes_smoke.py:19`
  补了 `--work-dir`，让 `ValueCTypes.txt` 落到测试自己的临时目录里。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:1`
  把 `condensing_return_proxy` 换成 `lighttpd/1-main_init_once.ll` 的真实样例。

验证上，`phi-demote` 和 `phi-demote-htypes` 现在都能过，`sailr-bench2-migration`
也保持通过。真实样例这边先找到了 `hexx64` 和 `lighttpd` 两条稳定线，后面可以继续往
Angr 的 return duplication / condensing 语义靠，不再只靠 proxy 维持覆盖面。

## 2026-06-24 迁移边界：angr 原始测试资产未在本地树上

这轮又核了一次 angr 仓库本身，确认这些 SAILR 测试名能找到，但对应的二进制资产没有
直接跟着代码树出现。当前能稳定推进的方式还是先保留本地 proxy + Bench2 语义样例，
 把 return duplication、duplication reversion、switch reuse 和 condensing 这些语义先钉住；
 等后面补到对应原始资产，再把脚本里的 proxy 逐个替换成真样例。

## 2026-06-24 实现记录：synthetic goto 的 shared 身份再收紧

这次只补了一个很小的 shared 断言，不动算法逻辑。`duplicateRegion({Goto})` 后，
`createSyntheticGoto()` 生成的 synthetic block 复制出来仍然要保留原来的 `BodyBlock`
和 `SyntheticSource` / `SyntheticTarget`，这样 virtual goto 的身份不会在复制链里变成
普通 copied body。

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1927`
  新增 `testStructuredCFGDuplicateSyntheticGotoReportsTargets()` 里的 `BodyBlock`
  断言。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
ctest --test-dir build -R 'structuring-analysis' --output-on-failure
```

结果：通过。

## 2026-06-24 当前判断

shared structuring 的 Phi 边界已经比较稳了，`demoteSSAFixHT` 和对应测试都在，
`StructuredCFG::materializeBlockBody()` 也已经能把 copied block 的 payload 通过 hook
重写并保住 CFG 身份。现在最明显的缺口还是两块：一是 `vvar` / static value rewrite
还没有像 angr 那样单独成层，二是 `ReturnDuplicatorLow` 和 switch deoptimization 的复杂
形状还没继续扩大到更接近 angr 的覆盖面。

## 2026-06-24 额外缺口：NotDec 还没有 angr 那种独立 dephication 层

angr 这边在 structuring 前还有一层专门的 dephication / vvar remap，能把 phi 和
virtual variable 的对应关系先整理掉，再让后面的 structuring 算法只看已经降过的值。
NotDec 现在只有 `demoteSSAFixHT()` 这条旧链路，能把 Phi demote 掉并把 HType 迁到
demoted LLVM Value 上，但还没有一个和 angr `GraphDephicationVVarMapping` 对齐的独立层。
这个缺口现在先记着，不在 structuring 算法里硬补 vvar 语义。

## 2026-06-24 迁移验证：真实样例在子模块入口可跑

这轮把之前卡住的点再核了一次。顶层 `notdec` 这条入口会先被 target / type-recovery
接线挡住，不适合拿来判断 SAILR 迁移是否可用；但子模块里的
`external/NotDec-llvm2c/bin/notdec-llvm2c` 已经可以直接跑下面这些真实 Bench2 样例：

- `hexx64/function-0x1156e0/native/function-0x1156e0.ll`
- `python/one-_PyPegen_fill_token.cold.ll`
- `lighttpd/1-main_init_once.ll`

这说明当前 shared structuring 和 llvm2c 这条线本身是通的，后面要补的是更细的
语义覆盖和更稳的迁移断言，不是先去改 renderer fallback。

## 2026-06-24 迁移边界：大模块样例先被旧 intrinsic 挡住

这轮也顺手试了 `fortune/module-all.ll` 和 `vsftpd/executable/module-all.ll`，它们都还会先撞
旧的 `SAContext::getIntrinsic(...): unhandled intrinsic` 断言，和 SAILR 本身关系不大。
所以这类大模块现在不适合拿来替代迁移脚本里的 proxy，后面还是优先继续找已经能稳定
跑完的真实样例，先把语义覆盖一点点往前挪。

## 2026-06-24 实现记录：synthetic goto 的 body identity

这轮只补了一个很小的 shared 断言，不动算法逻辑。`duplicateRegion({Goto})` 后，
`createSyntheticGoto()` 生成的 synthetic block 复制出来仍然要保留原来的 `BodyBlock`
和 `SyntheticSource` / `SyntheticTarget`，这样 virtual goto 的身份不会在复制链里变成
普通 copied body。

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1955`
  新增 `testStructuredCFGDuplicateSyntheticGotoReportsTargets()` 里的 `BodyBlock`
  断言。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/hexx64/function-0x1156e0/native/function-0x1156e0.ll -o /tmp/hexx64.sailr.c
./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/python/one-_PyPegen_fill_token.cold.ll -o /tmp/pythoncold.sailr.c
./build/external/NotDec-llvm2c/bin/notdec-llvm2c --algo=structured-sailr /sn640/NotDec-Exp/Bench2/bin2llvm-ir/lighttpd/1-main_init_once.ll -o /tmp/lighttpd.sailr.c
```

结果：通过。

## 2026-06-24 进展：迁移脚本继续往真实样例靠

这一轮把 migration 里的一个 condensing proxy 换成了真实的 `lighttpd/1-main_init_once.ll`
样例，保留了同一类 Angr 对照，但不再依赖手写 proxy 来撑覆盖面。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:133`
  `condensing_return_proxy` 改成 `condensing_real_lighttpd`，直接读真实 Bench2 输入。

这一步的意义不在于“样例变多了”，而是在于迁移测试开始真正约束 shared structuring
在真实输入上的结果，而不是只看最小手写 IR。下一步还是优先找能稳定跑的真实 return /
switch 样例，继续替换剩下的 proxy。


## 2026-06-24 迁移补充：把 Phi demotion 的证据边界收紧

这次没有继续扩大 structuring 侧的迁移壳，而是把 Phi demotion 的验证边界收紧回一个更可证实
的点：`demoteSSAFixHT` 会把函数里的 Phi 移掉，同时把原来挂在 Phi 上的 HType 从 Phi 本体
转到 demoted 之后的值上。当前测试只保留了这个稳定结论，没有继续硬猜 demoted slot 的名字。

- `external/NotDec-llvm2c/test/phi_demote_test.cpp:1`
  继续保留 unnamed Phi 的 demotion / HType 迁移断言。

这说明 `phi` 仍然不应该进入 structuring 算法层；它先被 demote，后面的 shared structuring
和 SAILR migration 才能看见已经降过的值和边界。更强的“名字到 slot 的一一对应”目前还没证实，
所以先不把它写成已完成。

## 2026-06-24 迁移补充：把不稳定的 Phi 名字断言收回

前一轮尝试把 demoted slot 的名字也纳入断言，但这条在当前 IR / reg2mem 命名下并不稳定，
容易把测试写成猜名字而不是验证语义。现在已经把那条不稳定断言收回，只保留稳定的结论：

- Phi 会被移掉。
- 原来的 Phi 类型会跟着 demoted 路径保留下来，而不是继续挂在 Phi 上。

这条边界足够支持后续 structuring 不直接处理 Phi，也够支撑 SAILR 迁移往下走。

## 2026-06-24 迁移定位：proxy 先继续保留

这轮又试了更大的真实 Bench2 片段，但 `fortune/module-all.ll` 和
`vsftpd/executable/module-all.ll` 还是先撞旧 intrinsic 断言，说明它们现在还不适合替换
迁移脚本里的 `switch_reuse_proxy` 和 `duplication_reverter_proxy`。

所以这两个 proxy 先继续保留，用来钉住语义边界，不假装已经有对应的真实输入。等旧
intrinsic 这条问题单独收掉，再把它们逐个换成真样例。

## 2026-06-24 迁移边界：hexx64 module-all 先撞 CFG 断言

这轮继续试了 `hexx64/native/module-all.ll`，结果不是语义不对，而是先撞到了
`pred_iterator out of range!` 的 CFG 断言。这个结果说明当前还不能把大模块样例当成
稳定迁移基线，也不能拿它来替换 `switch_reuse_proxy` / `duplication_reverter_proxy`。

所以目前还是老办法：保留已经跑稳的 `hexx64/function-0x1156e0`、
`python/one-_PyPegen_fill_token.cold.ll` 和 `lighttpd/1-main_init_once.ll`，先继续钉住
return / condensing 这几类稳定形状；更复杂的真实 switch / duplication 样例等旧 CFG
问题单独收掉后再上。

## 2026-06-24 迁移脚手架：新增一个更明确的 switch scaffold

这轮没有找到能稳定替掉 `switch_reuse_proxy` / `duplication_reverter_proxy` 的真实样例，
所以不再硬塞大模块，先把迁移脚本里的 proxy 语义说清楚，并补一个更简单的
`switch_case_recovery_proxy` 作为脚手架。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:45`
  把原来的 `switch_reuse_proxy`、`duplication_reverter_proxy` 和
  `duplication_too_sensitive_proxy` 的说明标成 scaffold。
- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:136`
  新增 `switch_case_recovery_proxy`，也是 scaffold，不是 Angr 原始资产。

验证：

```bash
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c ./build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

## 2026-06-25 迁移修正：真实 switch case 不再叫 proxy

这轮没有改 structuring 算法，只修正迁移脚本里的样例身份。`switch_case_recovery` 已经是
真实 fixture 输入，不再是手写 proxy，所以把 case 名字改掉，避免后面误判迁移进度。

- `external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py:277`
  `switch_case_recovery_proxy` 改成 `switch_case_recovery_real`。
- `external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py:150`
  继续保留 `lighttpd/1-main_init_once.ll` 的真实 condensing smoke。

验证：

```bash
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
/usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%M' ./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/notdec-fortune-shared-default-goto.c --tr-level=2 --algo=structured-sailr
```

结果：通过。fortune 同口径结果为 `elapsed=195.66 user=217.46 sys=1.72 maxrss=1260312`。

## 2026-06-25 实现记录：SwitchReusedEntryRewriter 只认真实 case edge

这轮对照 Angr 的 `switch_reused_entry_rewriter.py`。Angr 判断 reused entry 时看的是
entry node 的真实 predecessor，并且只把 jump table switch head 算进去；不是只看 case
元数据。NotDec shared 层之前只用 `Cases.Target` 判断，可能把 case metadata 指到 entry
但 successor edge 没有指到 entry 的不一致形状也当成 reused entry。

这次把 `SwitchReusedEntryRewriter` 收紧到真实 CFG case edge：case target 必须存在于
switch 的 case successor 段里，才参与 reused-entry 改写。这样更接近 Angr 的 predecessor
语义，也避免 pass 只根据元数据创建 synthetic goto。

实现：

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `switchCaseEdgeReachesBlock()`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  修改 `SwitchReusedEntryRewriter::runOnGraph()`，收集和复查 reused entry 时都使用
  `switchCaseEdgeReachesBlock()`。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testSwitchReusedEntryRewriterRequiresRealCaseEdge()`，确认 case metadata 指向 entry
  但 successor 没有 entry 时不会重写。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  保留 `testSwitchReusedEntryRewriterKeepsLowestSwitchIdEntry()`，继续钉住 Angr
  “最低地址 switch 直接保留 entry，其它 switch 变 synthetic goto”的规则。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

性能：本轮只收紧 shared reused-entry 的触发条件，不新增 trial 次数或复制规模；没有再跑
fortune 长耗时 smoke。

## 2026-06-25 实现记录：reused-entry 的 Angr 边界再补两条

这轮只补一个小的 Angr 对齐回归。Angr 的 `SwitchReusedEntryRewriter` 在多个 switch
复用同一个 entry 时，会按地址排序，保留最低地址的 switch 继续直连 entry，其余 switch
改成 synthetic goto。NotDec 当前 shared 实现已经按 `BlockId` 排序，但之前没有测试钉住
“插入顺序和 `BlockId` 顺序不一致”时仍保留最低 id。

同时收紧一个真实边界：Angr 是从 CFG predecessor 判断 entry 复用，不会只因为 switch
case 元数据写了某个 target 就认为存在真实 case edge。shared 层现在要求 case target 同时在
switch 的 case-successor 列表里，避免只有 `Cases` 字段指向 entry 时误触发 reused-entry
rewrite。

- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  新增 `switchCaseEdgeReachesBlock()`，`SwitchReusedEntryRewriter::runOnGraph()` 用它判断
  reused entry。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testSwitchReusedEntryRewriterKeepsLowestSwitchIdEntry()`，构造先插入高 id switch、
  后插入低 id switch 的图，确认低 id switch 保持 case target 直连 entry，高 id switch
  被改成 `CFGBlockCopyKind::SyntheticGoto`，且 goto 不连接回 entry。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  新增 `testSwitchReusedEntryRewriterRequiresRealCaseEdge()`，确认只有 case 元数据指向 entry、
  但 successor 列表没有真实边时不会改图。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

同时又看了 `fortune/executable/module-all.ll` 里的真实 switch 函数。它能作为后续候选，
但直接用整个 module-all 会撞旧 intrinsic 断言；强行切片又会带来大量 metadata、declare
和 intrinsic 处理，不适合现在塞进迁移脚本。当前仍保留 `switch_reuse_proxy`，等有更小、
能稳定跑完的真实 switch 输入再替换。

## 2026-06-24 实现记录：synthetic copy identity chain 再钉一层

这轮没有去碰 structuring 算法本身，而是把 copied / virtual block 的身份链再钉紧一点。
现在 `createSyntheticGoto()` 复制出来的块，除了保留 `SourceBlock`、`SyntheticSource` 和
`SyntheticTarget`，还要保留 `CopiedFromBlock`，这样后面即使经过 `materializeBlockBody()`
也能清楚地区分“这是从哪个 synthetic block 复制出来的”，不会把 chain 语义丢掉。

- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp:1980`
  新增 `testStructuredCFGDuplicateSyntheticGotoKeepsCopyIdentityChain()`。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
```

结果：通过。

## 2026-06-25 迁移边界：本地缺 angr/binaries 原始资产

这轮对照了 `/sn640/angr/tests/analyses/decompiler/test_decompiler.py` 和
`/sn640/angr/tests/common.py`。Angr 的 SAILR 相关单测依赖 sibling 仓库
`/sn640/binaries`，例如 `sailr_motivating_example`、`cksum-digest.o`、
`cat.o`、`fmt`、`who.o`、`printenv.o` 等真实二进制都从那里读。

当前机器没有 `/sn640/binaries`，所以不能直接把 Angr 原始测试输入迁到 NotDec 里。现在能
稳定使用的真实输入仍然是 NotDec-Exp 里的 Bench2 IR：

- `hexx64/function-0x1156e0/native/function-0x1156e0.ll`
- `python/one-_PyPegen_fill_token.cold.ll`
- `lighttpd/1-main_init_once.ll`
- `switch_case_recovery.ll`

下一步如果要继续迁移 Angr 原始样例，需要先补齐 `/sn640/binaries`，再按测试名逐个做：

1. 用 NotDec native/bin2llvm 链路转成单函数 LLVM IR。
2. 只接入能被 `notdec-llvm2c --algo=structured-sailr` 稳定跑完的最小函数片段。
3. 遇到旧 intrinsic、`ExtractValueInst`、CFG 断言等前端/旧后端问题，先记录阻塞，不用
   proxy 冒充真实迁移完成。

## 2026-06-25 已解决：shared default 默认改成 Angr synthetic goto

这轮继续对照了 Angr 的 `SwitchDefaultCaseDuplicator` 和 NotDec shared 实现。这里有一个需要
明确取舍的点：

- Angr 在多个 switch 共享同一个 default block 时，会给各个 switch head 插入 goto block，
  让 default 入口不再直接被多个 switch 复用。
- NotDec 当前 shared CFG 用 `SyntheticForwarder` 表达这个关系，并且
  `structuring_analysis_test.cpp` 已经用
  `testSwitchDefaultCaseDuplicatorInsertsSharedDefaultForwarders()`、
  `testSwitchDefaultCaseDuplicatorForwardsTerminalSharedDefault()` 等测试把这个行为固定下来。

这两种做法都在 shared CFG 层表达，不是 renderer fallback；但它们会改变后续 structuring
看到的控制流形状，也可能影响 C/Solidity 输出质量。这里不能静默把 forwarder 改成 goto，
也不能假装已经和 Angr 完全一致。

结论：默认按 Angr 走 synthetic goto，旧的 `SyntheticForwarder` 行为只作为 shared pass
里的兼容模式保留。这个开关放在 `SwitchDefaultCaseDuplicator`，不放进 C / Solidity
renderer。

实现：

- `external/NotDec-llvm2c/include/notdec-backends/Structuring/SAILRDeoptimization.h`
  给 `SwitchDefaultCaseDuplicator` 增加 `SharedDefaultRewriteMode`，默认值是
  `SyntheticGoto`。
- `external/NotDec-llvm2c/lib/Structuring/SAILRDeoptimization.cpp`
  在 `SwitchDefaultCaseDuplicator::runOnGraph()` 里按模式创建 `createSyntheticGoto()`
  或 `createSyntheticForwarder()`，pipeline 默认构造因此走 Angr 模式。
- `external/NotDec-llvm2c/test/structuring/structuring_analysis_test.cpp`
  把默认共享 default 测试改成断言 `CFGBlockCopyKind::SyntheticGoto`，同时新增
  `testSwitchDefaultCaseDuplicatorCanUseSharedDefaultForwarders()` 覆盖兼容模式。

验证：

```bash
cmake --build build --target structuring-analysis-test -j4
./build/external/NotDec-llvm2c/bin/structuring-analysis-test
python3 external/NotDec-llvm2c/test/structuring/run_sailr_bench2_migration.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
python3 external/NotDec-llvm2c/test/structuring/run_structuring_smoke.py --notdec-llvm2c /sn640/NotDec/build/external/NotDec-llvm2c/bin/notdec-llvm2c
```

结果：通过。

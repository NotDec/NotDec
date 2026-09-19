# Solidity 后端：按 calldata.index metadata 补全 public 入口 ABI 形参

## 本次用户原始 prompt

> 还没做的（需要单独决策）
> 含 calldata 读取（ABI 解码 helper）、memory 读写、signextend/sdiv/smod/sar、struct 返回的 helper
> 仍然不输出——helper 体内怎么表达 calldata 切片/内存目前没有现成模型，最自然的方向是让这类
> helper 接收已解码的实参。goal 已按计划完成，这部分建议下次单独开 plan 再推进。
>
> 这一块接下来该做什么，选择一块，然后详细规划一下

## 背景：候选块盘点（103 个 pattern case，Route B 第一阶段之后）

可达 private helper 按函数体内容分类（定义数 / 调用点数，脚本统计）：

| 类别 | 定义数 | 调用点 | 评估 |
| --- | --- | --- | --- |
| clean（已输出） | 1839 | 6197 | Route B 第一阶段已覆盖 |
| memory（mload/mstore/sha3/return/log/call buffer） | 759(+58) | ~1460 | 需要 memory 对象/局部变量模型，独立大块，风险最高 |
| calldata（calldataload/calldatasize） | 250(+57) | ~1100 | Solidity 侧有现成表达，中等 |
| signed（signextend/sdiv/smod/sar） | 4 | 8 | 量小，但现有渲染（`/ % >>`）在 public 函数里本就是无符号语义，属既有正确性问题 |
| struct 返回 | 0 | 0 | 本语料没有 |

在做 calldata helper 输出之前先量化了两件事，结果指向一个**更靠前、更便宜、产出更"高层"**的缺口：

1. 现有 `AbiDecodeResultPass` 已经在 88 个 case 里打出 **1782 个**
   `notdec.solidity.calldata.index` metadata（解码结果 → ABI 参数序号）；
2. 其中 **301 个（47 个 case）** 落在"没有对应 ABI 形参"的 public 入口里。selector 命名的
   入口（`public__0x802fe1da_0x7c`）名字里没有类型拼写、也没有 outlined 的尾随形参，
   `annotatedAbiArgumentExpr` 查不到名字 → 值按 unresolved 处理 →
   **使用它的整条语句被静默丢掉**（`formatStorageStore` 遇到 unresolved 值直接返回 nullopt）。

典型例子 `0011_multi_public`（也是本任务最初的样例）post-pass IR：

```llvm
%private.call = call i256 @abi_decode_word_from_calldata__0x265(..., i256 4, i256 %evm.calldatasize, i256 151), !notdec.solidity.calldata.index !116
%0 = call i256 @evm.storage.slot(i256 5)
call void @evm.storage.store(i256 %0, i256 %private.call)
```

当前输出是空函数体（整条 store 丢失）；补出形参后是：

```solidity
function public_0x802fe1da(uint256 arg0) public {
    // block_0:
    slot_5 = arg0;
}
```

所以本阶段选择：**把已经恢复出来的 `calldata.index` metadata 真正消费掉——按它补全
public 入口的 ABI 形参**。它比 calldata helper 本体渲染更靠前（不需要新的 calldata 表达式
模型）、产出更接近源代码，也为后续 calldata helper 输出扫掉"metadata 已解决但没人用"的浪费。

## 可行性原型（已实测，工作区已还原，补丁存 /tmp/rb/abi_param_proto.patch）

改动只有 `lib/Solidity/Reader.cpp:readFunction` 约 40 行：扫描函数体内
`notdec.solidity.calldata.index`（复用 `BodyBuilder::getStringMetadata`），取最大序号 N，
把 `Result.Parameters` 从当前数量补到 N（`uint256 argI`），同步写入
`ArgumentNames[4 + I]`；`ParameterTypes` 随后统一构造，自动带上新形参。

实测结果（同一构建目录，103 个 case）：

| 指标 | 当前已提交状态 | 原型 |
| --- | --- | --- |
| solc 编译失败 | 0 | **0** |
| unresolved value | 264 | **226** |
| condition TODO | 729 | **654** |
| goto 总数 / real / dangling | 424 / 343 / 69 | **424 / 343 / 69** |
| helper 函数数 | 1339 | 1339 |
| helper 调用点 | 1301 | **1355** |
| 逐 case 对比 | — | **32 个改善，0 个变差** |

四套 suite（pattern / rewrite / source / pattern_compile）全部通过；source golden 无变化
（那些 case 本来就有类型拼写或 outlined 形参）。

## 目标与预期效果

1. selector 命名的 public 入口按 metadata 的最大 ABI 序号补出 `uint256 argN` 形参，
   使 `annotatedAbiArgumentExpr` / `matchCalldataArgumentIndex` 能命中；
2. 受影响的值不再 unresolved，使用它的 store / return / 条件语句重新出现；
3. 有 ABI 拼写或 outlined 形参时保持现有行为，不重复补形参；
4. 量化：47 个 case / 301 个 metadata 命中里的语句恢复；unresolved、condition TODO 预算
   按原型实测下调（226 / 654）；helper 相关下限不下调。

## 技术路线

1. **形参合成（核心）**：`Reader::readFunction` 在 `ArgumentNames`/`ParameterTypes`
   构造前扫描 metadata，补 `uint256 argI` 形参与名字表。原型已验证，直接复用补丁。
2. **类型细化（可选，必须配 oracle）**：第一版全部 `uint256`。若要细化：
   - `bool`：该字被 `zext i1`/比较使用（参考现有 `isAbiBoolWord`）；
   - `address`：该字与 `evm_caller`/160 位掩码比较。
   细化单独一步做，先用 source golden 锁住形态，不在本阶段混入。
3. **测试**：
   - 新增 source golden（`calldata_index_param_public_entry_01`）：selector 命名入口 +
     decoder 调用 + store，期望 `function public_...(uint256 arg0) public { slot_0 = arg0; }`；
   - 现有 87+1 个 golden 全跑，确认无意外变化；
   - compile suite 预算：unresolved / condition TODO 下调；helper 下限按实测调整；
   - solc 全量编译 + pattern/rewrite suite 不退化。
4. **量化与记录**：新增形参数、恢复的语句数（对比原型产出的 .sol）、逐 case 差异；
   更新 `logs/20260918-01-*` 或本日志的"实现记录"一节。

## 风险

- **误判形参**：metadata 由 `AbiDecodeResultPass` 产生，只在解码结果对应 ABI 参数时命中；
  但仍需确认没有把"内部实参"当成 ABI 形参。缓解：只对 `public_*` 入口补；下标必须真实出现
  在 metadata 中；用 pattern suite 的 per-case metadata oracle 交叉核对，并人工抽查 2-3 个
  case 的 IR 位置。
- **签名漂移**：补形参后函数签名变化（`public_0x802fe1da()` → `(uint256 arg0)`），
  需要 golden 锁定；compile suite 只验证能否编译，覆盖不到语义。
- **多入口**：同一 selector 的多个 outlined body 各自补形参，不做跨函数统一，避免互相影响。
- **类型不准**：统一 `uint256` 可能与真实 ABI 类型（bool/address）不一致；本阶段只记录，
  细化时用 usage 推断 + oracle 证明。
- **新组合的 unresolved**：形参补出后，值可能进入此前无法表达的表达式（memory 等）而出现
  "新的" unresolved。原型实测 0 个 case 变差，实现阶段仍然逐 case 复查。

## 判断标准

- `0011_multi_public` 输出 `function public_0x802fe1da(uint256 arg0) public { slot_5 = arg0; }`；
- 新增 golden 通过且 solc 编译通过；
- 四套 suite 全绿，compile suite 0 编译失败，unresolved / condition TODO 不高于原型实测
  （226 / 654），helper 下限不下降；
- 32 个改善 case 不退化，变差的 case 数保持 0（若出现必须逐个解释）；
- 记录暂不处理的形态：ABI 形参类型细化、calldata helper 本体渲染、memory/signed/struct。

## 暂不处理（本阶段之后）

- **calldata helper 本体输出**：helper 体内 `evm_calldataload(%calldata, off)` →
  `abi.decode(msg.data[off:off + 32], (uint256))`（已用本机 solc 0.8.25 验证运行期切片可编译），
  可解锁 250 个纯 calldata helper；需要处理偏移表达式二次求值、revert 数据差异。
- **memory helper**：需要 memory 对象/局部变量模型（759 个定义），独立大块。
- **signed 渲染**：`sdiv/smod/sar/signextend` 目前是无符号渲染，属既有语义问题，单独小任务。

## 计划调整（2026-09-18，按用户意见收窄）

用户意见（原文要点）：

- IR 里把"不直接暴露、仅内部使用、不做 ABI 解码、不从 calldata 取参"的函数标成
  **internal linkage**，这样后端不需要考虑它们的 ABI 语义；
- 当前目标先放窄一点，**不要一步到位转换出"EVM 那种参数形式"**；
- ABI 参数识别应该放到**后处理（post-TR）**：基于类型推理识别出来的 calldata buffer 结构，
  再判断里面的字段是否符合 ABI 布局、对应第几个参数；
- 阶段划分写成一个显眼文档（根目录 `EVM-Arch.md`，含整条 EVM 链路介绍）。

调整后的路线（**替代**本文前面"优先做形参合成"的顺序）：

1. **函数分类/internal linkage**：已落地在**前端生成阶段**（不是后来的 pass）——
   evm2llvm `lib/LlvmLowerer.cpp:createFunctionPrototype` 现在按 `TacFunction::IsPublic`
   直接生成 linkage：ABI 表面（selector 入口 + `0x0` dispatcher）external，
   `private_*` 共享函数 internal（2026-09-18）。记录：
   `external/NotDec-evm2llvm/logs/20260918-01-linkage-classification.md`；
   剩下的是**消费端切换**：pattern/后端的 `private__` 名字判断逐步换成
   `internal || private_ 前缀` 的双条件，并重新生成语料验证；
2. **post-TR ABI 参数识别**：用 `%calldata` 指针的 `HTypeBufferView`
   （`getHTypeValueBufferView`，字段偏移已经是 ABI 布局）识别参数；
   第一版只做"识别 + 标注/oracle"，**不改函数签名**；
3. 后端暂时不做 ABI 形参合成。原型补丁保留在 `/tmp/rb/abi_param_proto.patch`
   （实测：unresolved 264→226、condition TODO 729→654、32 个 case 改善、0 个变差），
   作为"pre-TR 形状匹配 + 后端消费"这条路的实验记录；等 post-TR 识别就绪后再评估
   是否还需要它兜底。

阶段边界、IR 质量契约和 post-TR 职责见新文档 `EVM-Arch.md`（本日志不再重复）。


### 实现记录：消费端分类切到 linkage / 名字修复（2026-09-18）

起因：evm2llvm 的 helper 名字是 `private_` + sanitize(Name + "_" + Id)，`Name` 为空时得到
`private__0x1b7_0x1b7`，Gigahorse 恢复出高层名字时得到 `private_add_internal_0x10`；
NotDec 各处的 `private__`（双下划线）判断会漏掉后者。实测例：`private_call` fixture 的
`private_add_internal_0x10` 在 Solidity 输出里被整条丢弃。

改动（NotDec 侧，配合 evm2llvm `5d11e87` 的 internal linkage）：

- 新增共享判据 `detail::isEvmPrivateHelperFunction(F)`
  （`include/notdec/Passes/evm/SolidityPatternUtils.h`，实现放 `src/Passes/evm/SolidityPatterns.cpp`）：
  非声明、非 `public_` 前缀，且 `hasInternalLinkage() || 名字以 private_ 开头`；
- `EvmCalldataAccessPass` 的 `shouldRewriteFunction` / `shouldMarkPolymorphicFunction` 改用它；
- `AbiDecoderHelperRenamePass` 的 `isSmallPrivateHelper` 改用它，并跳过已带
  `notdec.evm.original_private_helper` metadata 的函数；
- `firstAddressSuffix` 改成 `helperAddressSuffix(Function&)`：从 metadata（若有）或当前名字里
  取最后一个以 `0x` 开头的分量，`private__0x265_0x265` 与 `private_add_internal_0x10` 都取对；
- `isPrivateHelperCall` 的名字判断从 `private__` 放宽到 `private_`（metadata 分支不变）；
- Solidity 后端 `Reader::isHelperRenderCandidate`：`private_` 前缀 **或** internal linkage
  （`public_` 前缀排除，避免把 `public__notdec_solidity_selector_inline.body` 当成 helper）。

验证：

- `ninja -C build-notdec-nothreads notdec`；四套 suite 全绿，compile suite 指标与改动前一致
  （corpus 仍是 external linkage + `private__` 名字，行为不变）；
- 端到端：新 evm2llvm 生成的 `private_call.ll`（internal linkage + `private_add_internal_0x10`）
  现在输出
  `function private_add_internal_0x10(uint256 arg0, uint256 arg1) internal returns (uint256 ret0) { return arg0 + arg1; }`
  和调用语句 `private_add_internal_0x10(2, 3);`；
- 新增 source golden `named_internal_helper_call_public_entry_01`：同一个 case 覆盖
  `private_sum_0x100`（命名 helper）与 `define internal @internal_helper_0x200`（linkage 信号），
  期望输出 `slot_0 = internal_helper_0x200(private_sum_0x100(5, arg0));` + 两个 internal 函数；
  source suite 89/89（含 solc）通过。

暂不处理：语料重新生成（现有 `test/evm/**/*.ll` 仍是旧 evm2llvm 输出，全部 external linkage）。


### 实现记录（二）：彻底去掉旧语料的名字判据，迁移测试用例（2026-09-18）

按用户要求不再保留"external linkage + `private__` 名字"的兼容路径：

- `isEvmPrivateHelperFunction` 收紧为 **只认 internal linkage**（仍然排除声明和 `public_` 前缀，
  因为 `SelectorEntryOutliningPass` 的 `public__notdec_solidity_selector_inline.body` 也是 internal）；
- `isPrivateHelperCall` 改为直接调用该判据（去掉 `private_` 名字分支和
  `original_private_helper` metadata 分支——改名后的 helper 因为仍是 internal 会被自然命中）；
- Solidity 后端 `Reader::isHelperRenderCandidate` 同样改成"非声明 + 非 `public_` + internal linkage"；
- 测试语料迁移：`test/**/*.ll` 共 **86 个文件、3438 个** `define ... @private_*` 补上 `internal`
  （一次性脚本；迁移范围：solidity-patterns/cases 79 个、solidity-rewrite/cases 2 个、
  solidity-source/ir 4 个、type-recovery/evm/cases 1 个）。`helperAddressSuffix` 仍按名字解析
  fact id，因此命名形式（`private_add_internal_0x10`）继续支持，只是不再用于分类。

验证：

- 四套 EVM suite 全绿；compile suite 八项指标与迁移前**完全一致**
  （264 / 729 / 424 / 343 / 69，helper 1339 / 调用点 1301）→ 分类集合不变、判据换成 linkage；
- source suite 89/89（含 solc），`named_internal_helper_call_public_entry_01` 通过；
- `notdec.type_recovery.evm.tr_level_2` 依旧失败：差异只有 `.htypes` 里的 `bytes: 1` 标注
  （HType width 相关，来自工作区里另一个会话未提交的 mlsub/layout-policy 改动），与 linkage/名字无关。


### 实现记录（三）：post-TR ABI 参数识别 + oracle（2026-09-18）

按"先只做识别 + 标注/oracle、不改签名"的要求，新增 post-TR pass：

- `src/Passes/evm/solidity-patterns/AbiParamRecoveryPass.cpp`（类声明在
  `include/notdec/Passes/evm/SolidityPatterns.h`，注册在 `PassManager.cpp` 的 EVM post-TR
  段、`AbiReturnPass` 之前；`src/CMakeLists.txt` 加源文件）：
  - 对每个 public entry 取 `%calldata` 形参 → `getHTypeValueBufferView` 拿类型恢复出的
    calldata 记录 → 统计位于 `4 + 32*i` 的字段数 → 函数 metadata
    `notdec.solidity.abi_param.record`；
  - 同一函数内扫描 pre-TR 的 `notdec.solidity.calldata.index` 标注 → 逐函数参数个数 →
    `notdec.solidity.abi_param.annotated`；
  - 两者不同时打 `notdec.solidity.abi_param.mismatch`（值 `record=N annotated=M`）。
  - 直接 `setMetadata`，**不**走 `addStringMetadata`（否则会顺带插入
    `notdec_solidity_rewrite_*` marker 调用，属于"只测量不改写"的 pass 不该有的副作用）。
- oracle：`test/run_evm_solidity_patterns_suite.py` 的 `METADATA_ONLY_KINDS` 加入三个新 kind；
  manifest 为 `0011_multi_public`、`0031_...`、`0009_...` 三个 case 增加
  `expected_metadata_string_values` 期望（例如 0011：record=1 ×7、annotated 0×5/1×2、
  mismatch "record=1 annotated=0" ×5；0009：record=3 ×28、annotated 0/1/2/3 = 15/6/6/1、
  mismatch 27）。

**实测结论（重要，规划需要调整）**：类型恢复目前把各入口的 `%calldata` 指针统一成
**同一个模块级记录**（87 个 case 里 86 个只有一种 calldata 记录类型），所以记录里的
ABI head 字段数是模块级上界而不是逐函数参数个数。103 个 case、2316 个 public 入口：

| 对比 | 入口数 |
| --- | --- |
| record == annotated（一致） | 177 |
| record > annotated（记录偏多） | 2106 |
| record < annotated（记录偏少） | 33 |
| record > 0 且 annotated == 0 | 1179 |

也就是说：**post-TR 参数识别要能替代 pre-TR 标注，前提是类型恢复先给出逐函数的 calldata
记录**（根因：共享解码 helper 把所有调用点的常量偏移并进了同一个记录类型）。
在 TR 侧解决之前，本 pass 作为"测量 + oracle"保留：pattern suite 会锁住这三项计数，
TR 改善时逐函数 record 应当向 annotated 收敛。

验证：

- `ninja -C build-notdec-nothreads notdec`；
- 四套 EVM suite 全绿（pattern / compile / rewrite / source）；pattern suite 新增 oracle 通过；
- 后端输出不变（compile suite 指标与预算一致）。


### 根因调查：calldata 记录为什么是模块级的（2026-09-18）

**现象**：103 个 case 里 102 个只有一种 calldata 记录类型，所有 public 入口的 `%calldata`
（`::arg1`）都显示同一个 `struct_N*`；对比 pre-TR 逐函数标注，record 偏多 2106 个入口。

**机制（已用最小复现确认）**：每个入口把自己的 `%calldata` 传进被多个入口共享的
outline 解码 helper；类型恢复的调用约束（`visitCallBase` → `deferCallConstraint` →
`addSubtype(F, ActualFunc)`，参数位置逆变 ⇒ 调用方实参 ≤ helper 形参）把**每个调用方
对 calldata 的字段要求**（常量偏移读取产生的 record 要求）汇进 helper 的那个形参，
形成"模块级并集"记录；由于该记录同时是各调用方的上界，所有入口打印出来都是同一份。

最小复现 `/tmp/rb/calldata_unify_repro3.ll`（约 50 行）：

- helper 在 `calldata + 4` 读一个字；
- 入口 A 只通过 helper 解码；入口 B 自己读 `calldata + 36`，同时也调 helper；
- 结果：A、B、helper 三者的 `%calldata` **都是** `struct_1 { field@4, field@36 }`。
- 把 helper 调用去掉（`repro1`）时，A、B 各自只有自己的记录 ⇒ 共享 helper 的调用约束是并集来源。

**尝试的修法（原型，已还原）**：在 `MLsubVisitor::visitCallBase` 里对 calldata 实参
单独处理——实参槽位不参与函数类型约束（用形参节点占位，自边无效果），另加一条反向边
`addSubtype(形参, 实参)`，期望"helper 自己恢复的字段仍能到达调用方、调用方的字段不再上升"。
补丁：`/tmp/rb/calldata_arg_sever.patch`（47 行，仅这两个 hunk）。

实测（103 case、2316 入口，用 `AbiParamRecoveryPass` 的 oracle 对比）：

| 指标 | 现状 | 原型 |
| --- | --- | --- |
| record == annotated | 177 | **1463** |
| record 偏多（并集） | 2106 | **2** |
| record 偏少 | 33 | 851 |
| record>0 且 annotated==0 | 1179 | **0** |

四套 suite：compile / rewrite / source 通过；pattern suite **99/103**。失败的 4 个里 3 个是
本次新增的 `abi_param.*` oracle（数值随修复变化，属预期），但 `0032_...` 是**真回归**：
`notdec.solidity.abi_return` 46 → 42（4 个返回点不再被识别）。

**结论**：
1. 并集确实来自共享 helper 的调用约束，方向判断正确；
2. 但当前"切断实参槽位"的实现把 **helper 自己恢复的字段也一起切断了**（under 33→851），
   因为该约束在实现上是双向生效的，不能用一条反向边补偿；
3. 还影响到 ABI return 分类（helper 内 buffer 记录变化）——说明 calldata/memory 的
   类型对象之间还有耦合，需要更细的隔离。

**下一步候选**（未开始）：
- **按调用点特化解码 helper（IR 层）**：对常量 base 的调用点克隆 helper 并代入常量，
  使每个克隆只服务一个调用点 ⇒ 并集天然收敛到单函数，且不动 TR 核心；代价是 IR 体积
  和输出函数数增加，需要评估 structurer/后端影响；
- **在类型系统里区分"要求(requirement)与证据(evidence)"方向**：让 calldata 实参只把
  自己的要求留在本函数、helper 的要求能下行到调用方；这需要改 lattice/subtype 传播语义，
  必须配套 `test/type-recovery/**` 期望值重生成；
- 或先保留现状，只把本 pass 的测量结果当作"TR 改善的标尺"。

复现命令：

```bash
build-notdec-nothreads/bin/notdec /tmp/rb/calldata_unify_repro3.ll -o /tmp/out.sol \
  --tr-level=2 --dump-htypes=/tmp/out.htypes
grep -E "::arg1 =>" /tmp/out.htypes
```


### 多态标记：并集根因的既有机制（2026-09-18，按用户提示复查）

用户提示：并集可能来自"某些 core data 获取的特殊函数没有标记为多态"；标记多态后不同调用点
就不会混合。复查结果：**方向正确，而且这是仓库里已有的机制**。

- MLsub 侧：`MLsubRecovery::markBuiltinPolymorphicBufferFunctions`
  给 `KIND_MLSUB_POLYMORPHIC_FUNCTION`（`notdec.mlsub.polymorphic_function`）打标；
  SCC 划分里 `hasPolymorphic()` 命中后，"the SCC boundary instantiates each callsite
  independently instead of merging all of them through one function type"。
- EVM 侧：`EvmCalldataAccessPass::markPolymorphicHelpers` 已经会给 calldata helper 打这个标，
  但**加了额外条件**：只有某个 offset 形参在不同调用点取值冲突
  （`hasPolymorphicCallsiteOffset`：同时见到常量与动态）才标记。
  解码 helper 最常见的形态是"所有调用点传同一个常量 base"，因此**不被标记** → 所有调用点
  经同一形参合并 → 模块级并集。

按用户建议做的两组实验（都只用既有 metadata 机制，没有改约束推理框架）：

| 变体 | record==annotated | 偏多 | 偏少 | record>0 & annotated==0 | 单值 case |
| --- | --- | --- | --- | --- | --- |
| 原始（现状） | 177 | 2106 | 33 | 1179 | 102/103 |
| 仅 private helper 无条件标记多态 | 288 | 1911 | 117 | 1079 | 92/103 |
| helper + public entry 都标记 | **1527**（60 case 部分数据） | **104** | 282 | 104 | 10/60 |

关键点：**entry 也必须标记**。ABI entry 是被 dispatcher（`public___function_selector___0x0`）
用它自己的 `%calldata` 调用的，entry 之间通过 dispatcher 的实参节点继续混合；
把 entry 也标成多态后，`0009` 的 record 分布与 annotated **完全一致**
（`{0:15, 1:6, 2:6, 3:1}`）。最小复现 `repro3` 也验证了逐调用点隔离。

但"helper + entry 都标记"会让 `24259_19755445_aefeec2314_4f43187f4106` **确定性不收敛**
（连续两次 `timeout 150` 都是 exit 124），所以该变体没有保留。

当前代码状态（未提交，待后续决定）：

- `EvmCalldataAccessPass::markPolymorphicHelpers` 已去掉 `hasPolymorphicCallsiteOffset` 门控：
  **所有读取 calldata 的 internal helper 都标记多态**（metadata 值按 offset 是否变化区分
  `calldata_offset` / `calldata_buffer`，仅供诊断）；`shouldMarkPolymorphicFunction` 仍限
  internal helper，**不包含 entry**；
- 该变体下 `24259` 连续两次正常结束（exit 0），四套 suite 正在复核；
- "entry 也标记"的实现与实测数字只留在本节，重新实现时注意 24259 的收敛问题
  （优先查它的 SCC 形状：很可能是递归/自环导致逐调用点实例化无界）。

后续建议：单独排查 24259 在 entry 多态下的不收敛原因（是否可对"只被 dispatcher 调用一次、
无递归"的 entry 安全标记），再决定是否放开 entry。


### 多态标记实验的验证结果与当前状态（2026-09-18 收尾）

- "仅 internal helper 无条件标记多态"：四套 suite 里 compile / rewrite / source 通过，
  pattern suite **98/103**（5 个失败；其中 3 个是本次新增的 `abi_param.*` oracle 数值变化，
  另有 2 个未逐个定位）。`24259` 在该变体下正常结束（连续两次 exit 0）。
- "helper + entry 都标记"：效果最好（见上表），但 `24259` 确定性不收敛（两次 timeout 124），
  且未跑 suite。
- 鉴于两个变体分别"收益小/有未解释的 pattern 失败"和"不收敛"，**代码已全部还原到 HEAD**
  （`EvmCalldataAccessPass.cpp` 无 diff），本节只保留实验数据与结论。
- 下次继续时的起点：
  1. 先查 `24259` 在 entry 多态下不收敛的原因（SCC/递归形状、是否可只对无递归的 entry 标记）；
  2. 决定 entry 的标记范围后，同步更新 `0011/0009/0031` 三个 `abi_param.*` oracle；
  3. 若采纳 helper 变体，需要解释 pattern suite 的 2 个非 oracle 失败。


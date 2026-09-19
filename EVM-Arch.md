# NotDec EVM 链路架构（EVM-Arch）

本文回答三件事，也是 EVM 方向后续所有工作的共同前提：

1. 一条 EVM 字节码到 Solidity 的完整链路长什么样，每步的输入/输出和阶段边界在哪；
2. **类型推理前**的 IR 必须满足什么质量契约；
3. 哪些语义识别放在**类型推理后**，为什么，以及当前缺口。

相关文档：

- 前端（bytecode → LLVM IR）：`external/NotDec-evm2llvm/docs/README.md`；
- Solidity 底层模式（"编译器生成了什么"）：`docs/evm/README.md` 及其子目录；
- 功能开发与验证流程：`docs/evm/feature-development-guideline.md`；
- 批量跑真实合约与抽样口径：`docs/evm/apehex-batch-loop.md`；
- 链路的实测复查记录：`logs/20260917-01-ir-and-solidity-backend-audit.md`。

## 1. 全链路总览

```text
EVM bytecode (.hex / runtime bytecode)
   │  ① 事实提取 + 提升
   │     external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py
   │     Gigahorse(Souffle/Datalog) → out/*.csv facts
   ▼
LLVM IR（evm2llvm 产出，i256 EVM word）
   │  ② NotDec EVM pre-TR pipeline（只做形状匹配，不看类型）
   ▼
pre-TR IR（"阶段 A"）        ← --emit-tr-input-ir 可导出 / --frozen-tr-input-ir 可冻结输入
   │  ③ MLsub 类型恢复（--tr-level >= 2）
   ▼
HTypeResult（值类型 + MemoryDecl/StorageDecl + buffer 记录结构）
   │  ④ NotDec EVM post-TR pattern passes（依赖 HType）
   ▼
高层 IR（marker/metadata 表达已恢复语义）
   │  ⑤ 后端
   ▼
.sol（external/NotDec-llvm2c/lib/Solidity/）或 .c（lib/notdec-llvm2c/）
```

### ① bytecode → LLVM IR（前端）

- Gigahorse 产出 SSA/TAC 事实（CSV）；`evm2llvm` 读事实 → TAC/CFG/函数模型 → 合法 LLVM IR。
- 前端约定（后端和 pass 都依赖这些）：
  - 所有 EVM word 是 `i256`，内存/calldata/returndata/env 是显式指针形参
    `(ptr %mem, ptr %calldata, ptr %returndata, ptr %env)`；
  - 每个 selector 一个 `public_*` 入口函数；共享代码被 outline 成 `private__*` 函数；
  - `evm_*` 是运行时 helper（`evm_sload/sstore/calldataload/return/revert/...`），IR 里保留调用；
  - 每条 TAC 指令带 `!notdec.evm` metadata 作为溯源锚点；
  - **linkage 从生成阶段就按函数类别给出**（2026-09-18 起）：selector 入口和 `0x0`
    dispatcher 是 external，outline 出来的 `private_*` 共享函数是 internal
    （`LlvmLowerer.cpp:createFunctionPrototype`，判据 `TacFunction::IsPublic`），见第 4 节。
- 调 PHI/inliner 的约定：查 PHI 来源时默认 `--disable_inline`（见 `README.md`）。

### ② pre-TR：只做形状匹配

实际顺序（`src/Passes/PassManager.cpp`，EVM 分支，`frozenTRInputIR == false`）：

1. `buildFunctionOptimizations()`：Verifier / InstCombine / SimplifyCFG / Promote / GVN /
   SCCP / BDCE / ADCE / InstCombine / SimplifyCFG；
2. `SelectorEntryOutliningPass`：selector dispatch 的 outline/标记；
3. `PayabilityGuardPass`：nonpayable guard 识别 + CFG rewrite；
4. `MemoryBufferRewritePass`：calloc 风格 memory buffer 归一化；
5. `EvmCalldataAccessPass`：`evm_calldataload` → `inttoptr(add(ptrtoint(%calldata), off))` + load；
   常量 `4+32*i` 偏移顺便标注 `calldata.index`；
6. `AbiDecoderHelperRenamePass`：私有 ABI 解码 helper 改名 `abi_decode_*`；
7. `InstCombinePass`；
8. `AbiDecodeResultPass`：追踪解码 helper 的返回值，在调用点标注 `calldata.index`；
9. `VerifierPass`。

共同点：**只做形状匹配**（常量偏移、CFG 形状、调用关系），不产生也不消费类型信息；
产出是"规范化 IR + marker/metadata 锚点"。

### ③ 类型恢复（MLsub）

- 入口 `add_type_recovery_passes(level)` / `MLsubRecoveryMain`；`--tr-level`，EVM 目前
  `level >= 3` 会被夹到 2（`MLsubRecoveryOpt` 仍是 assert stub）。
- 产出 `HTypeResult`（`external/NotDec-llvm2c/include/notdec-backends/Core/HTypeResult.h`）：
  - 值类型：`ValueTypesLower/Upper`、`ContraVariantValues`；
  - `MemoryDecl`：memory buffer 的记录结构（字段 + 偏移）；
  - `StorageDecl`：storage 根记录（`slot_N` 字段 + 类型）；
  - **没有 `CalldataDecl`**：calldata 没有单独的根记录，但 calldata 指针本身会被推成记录指针。
    例如 `0011_multi_public` 的 dump（`--dump-htypes`）：

    ```text
    struct struct_1 { void* field_0; /* at offset: 4 */ };
    [+] @public__0x802fe1da_0x7c => lower=(void (*)(top:256, struct_1*, top:256, top:256))*
    [+] abi_decode_word_from_calldata__0x265::%calldata.ptr.base => upper=struct_1*
    ```

    两参函数会出现两个字段的 ABI 布局记录：

    ```text
    struct struct_2 {
      'y15:256 field_0; /* at offset: 4 */
      i8[28] padding_0; /* at offset: 36 */
      'u17:256 field_1; /* at offset: 64 */
    };
    ```

- 阶段 A/B：`--emit-tr-input-ir=<out.ll>` 导出"pre-TR 规范化后的 IR"（阶段 A），
  `--frozen-tr-input-ir` 把这样的输入直接送进类型恢复（阶段 B），用于把 pre-TR 和 TR 解耦调试。

### ④ post-TR：把"结构"翻译成"Solidity 语义"

实际顺序（同样在 `PassManager.cpp`）：`AbiReturnPass` → `SolidityRevertPass` →
`CheckedBoundsPass` → `EventLogPass` → `EvmStorageHighLevelRewritePass`。

这些 pass 的公共工具是 `include/notdec/Passes/evm/SolidityPatternUtils.h` 里的
`HTypeBufferView`：

```cpp
// Thin view over one ABI/revert/event memory buffer type.  It only exposes the
// record pointee already recovered by MLsub; ...
struct HTypeBufferView {
  notdec::ast::RecordDecl *Record = nullptr;
  notdec::ast::HType *BaseType = nullptr;
  bool HasTransparentOffset0Field = false;
  HTypeBufferGap Gap = HTypeBufferGap::None;
};
```

也就是：**"这个指针指向什么结构"由类型恢复回答；"这个结构对应 Solidity 的哪种语义"由
post-TR pass 回答**，判断依据是字段偏移/类型是否符合 ABI、event、storage 的布局。

### ⑤ 后端

- Solidity：`external/NotDec-llvm2c/lib/Solidity/{Reader,BodyBuilder,Printer}.cpp`，
  由 `MLsubNotdecSolidity` 驱动；
- C：`external/NotDec-llvm2c/lib/notdec-llvm2c/`（clang AST）。

## 2. 类型推理前的 IR 质量契约

**必须满足**（不满足属于前端/pre-TR 的问题，回到 ①/② 修）：

- verifier 和 `llvm-as` 通过，没有未定义值/非法类型；
- 函数形状和 linkage 可分类：`public_*`（ABI 表面，external linkage，每个 selector 一个）、
  dispatcher/fallback/receive、以及 internal linkage 的共享函数（`private_*`）；
- EVM 语义以稳定形状出现：calldata 读取（常量偏移或 base+常量）、memory buffer
  （free memory pointer / 分配 / 连续写）、storage 访问（常量 slot 或 keccak 地址）、
  return/revert/event 的 ABI 编码形状；
- 只允许**形状匹配**，不允许依赖类型推断结果（类型恢复在它之后）；
- 保留后续阶段需要的信息：load/store 的地址链、调用关系、`!notdec.evm` 锚点。

**明确不要求**（不要为了它硬猜，否则会把错误固化到 IR 里）：

- 参数/返回值类型；
- calldata buffer 的字段结构（这是类型恢复的产出）；
- 动态类型（数组/bytes/嵌套结构）的偏移值（属于 post-TR 的结构识别）。

## 3. 哪些放到类型推理后

原则：**"这段字节是什么结构"是类型恢复的问题；"这个结构是什么 Solidity 语义"是 post-TR 的问题。**

已经这么做：ABI return、revert、event、storage 高层访问、checked bounds。

**目标也要这么做：ABI 参数识别。** 现状与目标：

- 现状：pre-TR 用常量偏移硬匹配（`EvmCalldataAccessPass` 的 `4+32*i`、
  `AbiDecodeResultPass` 的解码 helper 追踪）→ 在调用点/加载点打 `calldata.index` 标注。
  这套能覆盖"常量 base"的调用点（88 个 case、1782 处标注），但：
  - 它不经过类型恢复，无法利用已恢复的 calldata 记录结构；
  - 消费端（Solidity 后端）需要函数形参名才能把它变成 `argN`，selector 命名的入口没有
    形参表，47 个 case / 301 处标注因此断链（详见 `logs/20260918-02-*`）。
- 目标：post-TR 用 `%calldata` 指针的 HType（`struct_N*`，字段偏移已是 ABI 布局）判断
  "第几个参数"，再做参数恢复；pre-TR 只保留"calldata 是无类型字节 buffer"的读取形状。
- 为什么：参数识别和 ABI return/event 是同一类问题（结构 → Solidity 语义），应该在同一层、
  用同一套 `HTypeBufferView` 工具做。

**当前目标收窄**：不追求"一步到位输出 ABI 参数签名"，先把
(a) pre-TR 的函数分类/契约 和 (b) post-TR 的结构识别补上，参数形式最后再定。

## 4. 函数分类与 linkage

**规则（已实现，2026-09-18）**：evm2llvm 在**生成阶段**直接给出 linkage
（`lib/LlvmLowerer.cpp:createFunctionPrototype`，判据 `TacFunction::IsPublic`，
由 `FactLoader.cpp` 从 `PublicFunction.csv` 与 `functionId == "0x0"` 得到）：

- **external**：ABI 表面——selector 入口、`0x0` dispatcher；
- **internal**：其余共享代码 outline 出来的 `private_*` 函数。

配套测试 `evm2llvm.fixture.private_call.check-linkage`；全部 fixtures 重新生成后扫描过
`@private_*` 的非调用使用，**0 处地址逃逸**，所以 internal linkage 不会让 LLVM 的 DCE
误删"实际会被间接调用"的函数。改动记录：
`external/NotDec-evm2llvm/logs/20260918-01-linkage-classification.md`。

为什么需要它（旧做法的问题）：

- 分类靠名字前缀：pattern 侧 `isPrivateHelperCall`、`AbiDecoderHelperRenamePass`、
  `EvmCalldataAccessPass`、Solidity 后端都检查 `private__`；
- 名字本身不可靠：`functionName = (IsPublic ? "public_" : "private_") + sanitize(Name + "_" + Id)`，
  `Name` 为空时得到 `private__0x1b7_0x1b7`（能命中），Gigahorse 恢复出高层名字时得到
  `private_add_internal_0x10`（单下划线，命中不了）。

linkage 只回答"是否 ABI 表面"，**不回答内联与否**：函数体渲染/内联仍是独立决策。
Route A（在 IR 里内联 private helper）实测会破坏 structurer：pattern suite 103→32、
`// goto` 423→4755，已回滚，结论保留在 `logs/20260917-04-evm-solidity-next-steps.md`。

**消费端分类（2026-09-18 起只认 linkage）**：

- 统一判据 `notdec::passes::evm::detail::isEvmPrivateHelperFunction`
  （`include/notdec/Passes/evm/SolidityPatternUtils.h`）：非声明、非 `public_` 前缀、
  **internal linkage**。名字（`private__<id>_<id>` / `private_<name>_<id>`）不再参与分类。
- 使用位置：`EvmCalldataAccessPass`（calldata 访问重写、polymorphic 标记）、
  `AbiDecoderHelperRenamePass`（`isSmallPrivateHelper`、改名后的地址后缀提取）、
  `isPrivateHelperCall`（调用点判定）、Solidity 后端 `Reader::isHelperRenderCandidate`。
- `public_` 前缀仍然要排除：`SelectorEntryOutliningPass` 创建的
  `public__notdec_solidity_selector_inline.body` 也是 internal linkage。
- 改名后的 helper 仍通过 `notdec.evm.original_private_helper` metadata 记住原名，地址后缀取
  最后一个以 `0x` 开头的分量，`private__0x265_0x265` 与 `private_add_internal_0x10` 都能取对，
  重复运行不会二次改名。
- source golden `named_internal_helper_call_public_entry_01` 覆盖
  `private_<name>_<id>` 名字（internal linkage）与无前缀 internal helper 两种形式。

**测试语料已迁移**：`test/**/*.ll` 里 86 个文件、3438 个 `define ... @private_*` 全部补上
`internal`（一次性脚本迁移；不是重跑 Gigahorse）。迁移后四套 EVM suite 的八项指标与迁移前
**完全一致**（unresolved 264 / condition TODO 729 / goto 424 / real 343 / dangling 69，
helper 1339 / 调用点 1301），说明分类集合没变，只是判据换成了 linkage。新生成的 IR 由
evm2llvm 直接产出 internal linkage，不再需要迁移。

## 5. 现状与路线图

已交付：

- 前端（evm2llvm）：verifier-clean IR、每个 selector 一个入口、共享代码 outline、
  **按函数类别生成 linkage**（2026-09-18）；
- 函数分类：消费端只按 internal linkage 判定 helper（`isEvmPrivateHelperFunction`），
  测试语料 86 个文件 / 3438 个 helper 已迁移为 internal；
- pre-TR：selector outline、payability guard、memory buffer 归一化、calldata 访问归一化 +
  `calldata.index` 标注（88 case / 1782 处）、解码 helper 改名、ABI 解码结果追踪；
- post-TR：ABI return、revert 分类、checked bounds、event、storage 高层访问；
- Solidity 后端：private helper 输出 + SSA 折叠/局部变量（1339 个 helper 函数 / 1301 个调用点，
  77/103 个 case），四套 suite（pattern / rewrite / source / pattern_compile）与预算棘轮。

下一步（按"先窄后宽"排序）：

1. **post-TR 的 ABI 参数识别**：用 `%calldata` 的 `HTypeBufferView` 判断 ABI 布局字段，
   先只做"识别 + 标注/oracle"，不直接改函数签名；
2. **calldata 解码 helper 的本体输出**：`abi_decode_*` 现在已经是 internal helper，
   判据上已就绪；helper 体内
   `evm_calldataload(%calldata, off)` → `abi.decode(msg.data[off:off + 32], (uint256))`
   （已用 solc 0.8.25 验证运行期切片可编译）；解锁 250 个纯 calldata helper；
3. 其余：memory helper（需要 memory 对象模型）、`signextend/sdiv/smod/sar` 的正确有符号渲染、
   struct 返回。

## 6. 维护约定

- 本文描述"阶段边界与契约"；具体形状和模式细节放 `docs/evm/` 子目录；
- 修改 pre-TR/post-TR 分工、linkage 约定或 IR 质量契约时，必须同步更新本文；
- 新增 pass 时在本文第 ②/④ 节的列表里登记，并说明它属于哪一侧、依赖什么。

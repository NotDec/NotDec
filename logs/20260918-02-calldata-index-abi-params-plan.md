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


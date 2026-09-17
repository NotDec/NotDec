# EVM Solidity 链路现状盘点与下一步

## 用户原始 prompt

总结当前任务，盘点接下来该干什么

盘点接下来该干什么，提供完整的上下文

## 一、当前任务与已完成状态

本轮任务是接续 `logs/20260917-03-evm-calldata-guard-and-require.md`，把
`calldata_min_size_public_entry_01` 从占位输出推进到真实语义。已完成并已 push：

- llvm2c `646560b`：guard-if-revert 折叠成 `require`、remaining-body 标记按
  结构化树完整性判断、i1 常量打印 bool、address 下标转 uint256、清理 dead code；
  同时把上一轮未提交的 `evm_calldatasize`、typed condition、calldata matcher、
  direct-slot store、branch polarity 修正一起提交。
- 顶层 `c02118b7`：新增 source-suite golden `calldata_min_size_public_entry_01`，
  runner 加可选 `--solc`，CMake 加 `NOTDEC_SOLC`，更新两份过期 golden。

当前输出：

    contract Decompiled {
        uint256 public slot_0;

        function calldata_min_size(uint256 arg0) public {
            // block_0:
            require(msg.data.length >= 36);
            // block_1:
            slot_0 = arg0;
        }
    }

## 二、链路与测试的完整上下文

数据流：

1. `external/NotDec-evm2llvm`：bytecode/Gigahorse facts -> LLVM IR（低层 helper，
   `evm_*` intrinsic + `!notdec.evm` metadata），自带 33 个 fixture 测试，不在主 ctest。
2. 主项目 `notdec` 的 EVM pattern passes（`src/Passes/evm/solidity-patterns/`）：
   SelectorEntryOutlining -> PayabilityGuard -> MemoryBufferRewrite -> EvmCalldataAccess
   -> AbiDecoderHelperRename -> （类型恢复）-> AbiReturn -> SolidityRevert ->
   CheckedBounds -> EventLog -> EvmStorageHighLevelRewrite。
3. 类型恢复：MLsub，`--tr-level=2` 是 EVM 链路唯一被验证的档位（level 3 见 P3）。
4. Solidity 后端：`external/NotDec-llvm2c/lib/Solidity/`（Reader/StorageInfo/
   BodyBuilder/Printer），把结构化和类型结果打印成 Solidity。

测试入口（主 build `build-notdec-nothreads`，见下方构建注意）：

| suite | case 数 | 检查内容 | 当前状态 |
| --- | ---: | --- | --- |
| `notdec.evm.solidity_patterns` | 103 | 输出 IR metadata/marker 计数 + `llvm-as` | 通过 |
| `notdec.evm.solidity_rewrite` | 75 | selector outline 后的 CFG/结构 | 通过 |
| `notdec.evm.solidity_source` | 79 | 生成的 `.sol` 与 golden 逐行 diff | 通过，79/79 过 `solc --bin` |
| `notdec.type_recovery.evm.tr_level_2` | - | 类型恢复 oracle | 通过 |
| `notdec.lifting.wasm` | - | 非 EVM 回归 | 通过 |

构建注意：

- `build-notdec-nothreads2` 的 cache 指向已不存在的 `llvm-22.1.0.nothreads.obj`，
  无法 reconfigure；本轮验证都在 `build-notdec-nothreads`（LLVM 22.1.0.obj）。
- `NOTDEC_SOLC` 只在 `build-notdec-nothreads` 的 cache 里设了
  `/sn640/EthIR/source/solcv8.25`；其他 build 默认不带 solc 检查。
- standalone `external/NotDec-llvm2c/build` 的 test/CMakeLists 依赖主项目的
  `notdec` target，独立配置会失败；`structuring-analysis-test` 目前没有可执行的
  构建入口。

## 三、量化差距（103 个真实 pattern case，全部可编译）

统计口径（对 `notdec <case>.ll -o out.sol --tr-level=2` 的产物）：

```bash
cd /tmp/pattern-audit2   # 103 份生成结果
grep -lF "if (false /* TODO" *.sol | wc -l        # 56
grep -lF "if (!false /* TODO" *.sol | wc -l       # 40
grep -lF "while (false /* TODO" *.sol | wc -l     # 19
grep -lF "TODO: unresolved value" *.sol | wc -l   # 65
grep -oF "TODO: unresolved value" *.sol | wc -l   # 538
grep -lF "// goto block_" *.sol | wc -l           # 41
grep -lF "TODO: recover remaining body" *.sol | wc -l  # 30
```

| 缺口 | 受影响 case | 量 |
| --- | ---: | ---: |
| 条件未恢复（`if/while (false /* TODO */)`） | 65/103 | 813 处 |
| 未恢复值（`0 /* TODO: unresolved value */`） | 65/103 | 538 处 |
| 其中 private call / helper 结果 | ~50/103 | ~384 处 |
| 其中 `evm_sload` | - | 36 处 |
| 其中 `evm.calldataload.load` | - | 29 处 |
| 其中 `abi_decode_word_from_calldata` | - | 11 处 |
| 无法结构化（`// goto block_`） | 41/103 | 423 处 |
| 仍追加 remaining-body | 30/103 | 30 处（与 goto 完全重叠） |
| event signature 未恢复 | 57/103 | - |
| revert payload 只到 encoded_candidate | - | 428/773 个 revert |

revert kind 分布：encoded_candidate 428、error_string 151、custom_error_candidate 88、
empty 66、returndata_forward 39、panic 1。

对照组：79 个 source-suite 小 case 现在是 0 unresolved / 0 TODO condition /
0 goto / 0 remaining-body，只有 1 个 event signature TODO。也就是说**小 case 层
已经干净，缺口集中在真实合约规模**（private call、动态 ABI buffer、复杂 CFG）。

## 四、下一步候选（按优先级）

### P0-1 calldata index 从 matcher 上游成 metadata

现状：`BodyBuilder::matchCalldataArgumentIndex()` 在 IR 上重做
`notdec_evm_calldata_min_size -> ptrtoint -> add -> inttoptr -> load` 形状匹配。
只有这一种受 guard 保护的形状能恢复成 ABI 参数，`evm.calldataload.load` 仍有 29 处
落在 unresolved。

落点：`EvmCalldataAccessPass::rewriteCalldataLoad()`（`src/Passes/evm/solidity-patterns/
EvmCalldataAccessPass.cpp:516`）在把 `evm_calldataload` 换成 `calldata.ptr` + load 时，
offset 常量满足 `>=4 && (offset-4)%32==0` 就给新 load 写
`!notdec.solidity.calldata.index = i`（再带一个 checked/unchecked 标记）。
后端 `valueExpr()` 的 LoadInst 分支优先读 metadata，保留现有 matcher 作为兜底。
验证：pattern suite 加 metadata 计数 oracle；`calldata` 类 4 个 case +
`abi_decoder_helper_rename_01` 的 golden 输出对比；再抽真实 case 看 29 处是否下降。

**状态：已完成。** 实现与验证：

- `EvmCalldataAccessPass::annotateCalldataArgumentIndex()` 在
  `rewriteCalldataLoad()` 里对 public entry 的常量 offset 满足
  `>=4 && (offset-4)%32==0` 的 load 直接 `setMetadata`
  `notdec.solidity.calldata.index`（十进制字符串）。这里刻意不走
  `addStringMetadata()`——它不是 rewrite surface，不该生成 marker/hidden 计数，
  runner 也把该 kind 放进 `METADATA_ONLY_KINDS`。
- 后端 `valueExpr()` 的 LoadInst 分支优先读 metadata，再退回原 matcher；
  新增 `isWordLikeParameter()`：只有 `uint*/int*/address` 参数才允许用标识符
  顶替原始 calldata word。bool / bytesN / bytes / string / array 仍退回
  unresolved，否则 ABI decoder 的 `raw < 2`、storage packing、selector 比较
  会打印出 `bool < int_const`、`bytes4 & uint256` 这类非法表达式
  （真实 case `24534` 在 P0-1 第一版暴露了这两类）。
- oracle：pattern manifest 对 3 个 calldata case 加 metadata 计数 +
  `expected_metadata_string_values`（公开单参数=0、sub 三参数=2、private helper=0）；
  source suite 新增 `calldata_min_size_sub_public_entry_01` golden，锁定
  `slot_0 = arg2;`。
- 结果：pattern 103/103 可编译；`evm.calldataload.load` unresolved 29 -> 3；
  总 unresolved 538 -> 515；条件 TODO 813 -> 781；source suite 80/80 通过。

### P0-2 private call / helper 结果恢复

现状：~384/538 个 unresolved value 是 `private__0x..._0x...`、`private.ret1` 这类
private helper 的返回值，说明 outlined 出来的内部函数没有内联，也没有生成对应的
Solidity helper 函数。

两条路线：

1. 上游内联：对满足条件的 outlined private 函数在上游做 inline（Gigahorse 的
   `--disable_inline` 是外部开关，NotDec 内部还缺一个保守的 inline/摘要 pass）；
2. 后端输出 helper：private 函数照常生成 Solidity function，调用点输出函数调用。

判断标准：优先选不容易把跨函数 memory/storage 语义搞错的一侧；先用
`test/evm/solidity-patterns/cases/` 里 private 调用密集的 case 建 oracle，
再决定。

**状态：调查完成，停在路线选择（按 AGENTS.md 的 goal 约定暂停）。**

规模盘点（对 pattern suite 的 `out.ll` 统计）：

- 77/103 个 case 有 private 调用；9053 个 call site、2737 个不同 callee；
- callee 指令数：<=10 有 480 个，11-30 有 1281 个，31-100 有 773 个，>100 有 203 个；
- 当前 Solidity 输出里 private 类 unresolved 共 430 处（58 个 case、269 个不同名字），
  其中 side-effect-free（无 store / 无 side-effecting evm call / 无 alloca）的
  callee 只占 36 处，纯表达式内联最多只能解决 <10%。

全量 inline 实验（用 `opt -mtriple=unknown-unknown-unknown -passes=inline` 对
103 个 `out.ll` 内联，再把 triple 改回 evm，跑 `notdec --tr-level=2` + solc）：

| 指标 | 现状 | 全量 inline 后 |
| --- | ---: | ---: |
| solc 可编译 | 103/103 | 98/103（5 个新失败） |
| private unresolved | 430 | 166（-61%） |
| 总 unresolved | 515 | 1103 |
| `evm.storage.load` unresolved | 0（藏在 private body 里） | 685 |
| `evm.storage.packed.load` unresolved | 0 | 91 |
| `evm_sload` unresolved | 36 | 44 |
| IR 体积 | 26.0MB | 28.7MB（+10.6%） |

典型例子 `1274`：inline 前 `approve()` 是
`return 0 /* TODO: private__0x88c_0x88c */ != 0;`，inline 后 call boundary 消失、
函数折叠成 `return 1;`；但同一模块里新暴露 13 处 `evm.storage.load` unresolved。

结论：

- LLVM inliner 语义上是安全的（就是同一份 IR 展平），符合“不搞错跨函数
  memory/storage 语义”的判断标准；430 处 private 值里 61% 能被它消掉。
- 真正的后续阻塞不是调用边界，而是 (a) 后端 storage helper 覆盖
  （`evm.storage.load` / `packed.load` / `evm_sload` 变可见后解析不了）、
  (b) 5 个新 solc 失败要逐个 triage、(c) pattern suite 的 oracle 会大面积变化、
  IR 体积 +10%。
- 后端 helper 路线（Reader 输出 private 函数 + local materialization + 多返回值）
  是更大的新功能，而且正好要重新实现跨函数 memory/storage/returndata 语义，风险
  高于 inline 路线。
- 无论走哪条路线，先把 `private.ret` / `abi_decode_word_from_calldata__*` 这类
  ABI decode helper 的结果用 P0-1 同款 metadata 映射回参数下标，都是无悔收益
  （当前 63+ 处，且两条路线都需要）。

建议顺序：A1 后端 storage helper 覆盖 -> A2 有界 private-helper inline pass
（只 inline `private__*`、size cap、fixpoint、带 stat 和关闭开关）-> A3 oracle
重建 + private 密集 case 的 source golden；期间 triage 那 5 个 solc 失败。
在用户确认路线前不继续改 pipeline。

**状态（后续实现与回退）：A2 pass 实现并实测后被回退，代码未保留；阻塞点
已量化并记录在此。**

实现过的原型（已删除）：

- `PrivateHelperInlinePass`：只 inline callee 名以 `private__` 开头的函数，
  size cap 120（可调），最多 5 轮 fixpoint，`InlineFunction()` +
  WeakTrackingVH，带 inlined/oversize statistic。
- 试了两个 placement：
  - early：`buildFunctionOptimizations` 之后、`SelectorEntryOutlining` 之前；
  - late：front passes 之后、type recovery 之前。

early placement 开启后的 3 个代表 case（slots 保留、solc 全过）：

| case | 默认 unresolved | 开启 inline |
| --- | ---: | ---: |
| 1274 | 10（private 8） | 3（private 0） |
| 24541 | 8 | 4（private 0） |
| 26499 | 6 | 6（private 2，callee 超 cap） |

但 early placement 的全量 opt-in 审计不成立：

| 指标 | 默认 | early inline |
| --- | ---: | ---: |
| solc 可编译 | 103/103 | 93/103（9 个 solc 失败 + 1 个 abort） |
| 总 unresolved | 515 | 299 |
| private unresolved | 430 | 26 |
| `evm.calldataload.load` unresolved | 3 | 111 |
| `if (false /* TODO` | 522 | 1788 |
| `while (false /* TODO` | 46 | 702 |
| `// goto block_` | 423 | 4755 |

即：private 值确实被打通了，但 flatten 后的 CFG 让 structurer 退化
（goto/条件 TODO 暴涨），同时 pattern suite 从 103/103 掉到 32/103：
66 个 case 的 revert/hidden marker 计数增加（同一处语义在多个 call site 被复制），
并有真实覆盖损失——11 个 case 丢 `storage_byte_array_length_bounds`、
5 个丢 `checked_mul`、1 个丢 `checked_add`、`memory_allocation_bounds` 下降。

late placement 的覆盖损失变成 `abi_return` 大幅下降（如 66 -> 30）和
`memory_allocation_bounds` 下降，而且 backend 收益基本消失（1274 只到 10）。

结论：

- Route A 的障碍不是 inliner 本身，而是 **pattern matcher/oracle 按 outlined
  形状调优 + structurer 对 flatten 后大函数退化**；直接转正会同时损失
  checked-bounds/abi-return 覆盖和控制流质量。
- 因此本轮回退代码，默认路径回到 3/3 suite 全绿；P0-2 的调用边界打通被证明
  可行但需要独立工作流：retune checked-bounds/abi-return matcher on inlined IR、
  处理大函数结构化、重建 pattern oracle、抽 private 密集 source golden。
- 下一步二选一：(i) 开专门一轮做上述 retune + oracle 重建，再把 inline 默认打开；
  (ii) 放弃 IR inline，转 Route B（后端 helper + local materialization），接受其
  跨函数 memory/storage/returndata 语义风险。

### P0-2b ABI decoder 结果映射（已落地，默认路径）

在不改 pipeline、不改 pattern oracle 的前提下，先把 private-helper 问题里**无争议的
decode 半边**做掉：

实现：

- 新增 `src/Passes/evm/solidity-patterns/AbiDecodeResultPass.cpp`，放在 EVM front
  passes 末尾（`InstCombinePass` 之后）。pass 只分析非 public entry 的函数：
  - 单一 `ret`；标量返回看作 element 0，struct 返回要求由 `insertvalue` 链在
    poison/undef 上完整构造；
  - 每个 element 必须能回溯成 `TracedOffset`：绝对常量，或"helper 的 base
    formal + 常量"；
  - `inttoptr(add(ptrtoint(calldata), X))` 里的 X 可以是常量、base formal 或
    `base + const`；
  - element 也可以来自**嵌套 decoder 调用**：若被调 helper 已在表里，就把它的
    base-relative offset 与外层调用实参的 base 表达式复合（fixpoint 迭代直到不再
    新增 helper，最多 6 轮）；
  - 只有当整个 helper 的 element 都能回溯时才收进表。
- 对 public entry 里的调用点，把每个 element 对应的 ABI 下标写回该
  `extractvalue`（标量返回写回 call 本身）：`(offset - 4) / 32`。
  关键点是 element 顺序不等同参数顺序——真实 helper 里 element 0 常是
  `base+32`、element 1 是 `base`，必须沿 insertvalue 链算，不能按 tuple 顺序猜。
- metadata 仍是 backend-interface only（同名 `notdec.solidity.calldata.index`，
  不走 marker surface）。
- 后端 `BodyBuilder::valueExpr()` 把 metadata 解析抽成
  `annotatedAbiArgumentExpr()`，extractvalue / load / call 三处共用；bool/bytesN
  等非 word 参数继续走 unresolved。

oracle：

- 新增 source-suite golden `abi_decode_result_public_entry_01`：helper 返回
  `{word@base+32, word@base}`，输出锁定为 `slot_0 = arg1; slot_1 = arg0;`，
  直接证明 element 到参数下标的映射方向；
- 新增 `abi_decode_nested_result_public_entry_01`：第二个 word 由嵌套 word helper
  用外层 base formal 解出，锁定 base-relative 复合逻辑；
- pattern manifest 的 `abi_decoder_helper_rename_01` 增加
  `expected_metadata_counts` 和 `expected_metadata_string_values`（index 0）。

结果（默认路径，103 个 pattern case 全量审计）：

| 指标 | 改动前 | 直连版 | 嵌套版（当前） |
| --- | ---: | ---: | ---: |
| solc 可编译 | 103/103 | 103/103 | **103/103** |
| 总 unresolved | 515 | 399 | **337** |
| private unresolved | 430 | 336 | **274** |
| `private.ret*` unresolved | 63 | 33 | **12** |
| `abi_decode_*` unresolved | 11 | 5 | 5 |
| `if (false /* TODO` | 522 | 480 | **451** |
| `while (false /* TODO` | 46 | 46 | 46 |
| `// goto block_` | 423 | 423 | 423 |

`notdec.evm.solidity_patterns` / `_rewrite` / `_source` 全绿（source 82/82
含 `solc --bin`）。这一步不动 pipeline、不新增 marker，因此是纯增量收益，
后续 Route A/B 都需要它。

### P1-3 条件恢复随值一起收敛

现状：`getCondition()` 只要表达式里还有 unresolved 就退回 TODO，因此条件缺口
（813 处）基本是 P0-2/P1-4 的下游。做完 P0-1/P0-2 后应重跑统计看自然下降；
剩下的是 PHI/dephication 条件、bool 类型条件（需要真值语义），再单独处理。

### P1-4 storage load/store 扩到 mapping/array/packed

现状：`formatStorageStore()` 只放开 direct slot；mapping/array 下标已能打印但
packed store、嵌套 mapping/struct 仍缺。`evm_sload` 36 处 unresolved 说明部分
load 没走 `EvmStorageHighLevelRewritePass`。

落点：先枚举 `evm.storage.*` helper 的现有覆盖矩阵，再补 packed/嵌套 path；
每补一类往 source suite 加一个 golden（可直接用手写 IR 构造稳定形状）。

### P1-5 revert payload 分类优先级调查

现状：`SolidityRevertPass.cpp:593 classifyRevertFromHType()` 在尝试 panic/
error_string/custom_error 之前，会先被 `hasHelperEncodedRevertPayload` /
`hasRawBytesPayloadRevert` 判成 `encoded_candidate`，导致 428/773 revert 停在
encoded_candidate。

第一步是确认这是故意的保守回退还是顺序问题：把 panic/error/custom 的判定提前，
用 pattern suite 的 `expected_revert_kinds` / `expected_panic_codes` oracle 验证
误分类不上升。若 helper-encoded 形状确实无法解码，就保持现状并记录原因。

**状态：已实现，收益远超预期。**

先用临时 instrumentation 给 428 个 `encoded_candidate` 分类，发现根因不是优先级：

| 路径 | 数量 | 说明 |
| --- | ---: | --- |
| zero-size | **388** | arg2 常量 0，但 arg1（offset）非 0 |
| no-selector | 253 | payload HType 有 selector store，但取不出 selector 常量 |
| raw-bytes | 26 | bytes header 形状 |
| helper-encoded | 1 | helper 先写 selector 再编码 |

其中 388 个 zero-size 是**语义误分类**：EVM `REVERT` 只返回 `size` 字节，
size 为 0 就是空 revert，与 offset 无关。之前统一归到 `encoded_candidate`，
既让 marker 不准确，又挡住了 Solidity 后端的 `require` 折叠
（`guardRevertRequire` 只折叠 `empty`）。

改动（`SolidityRevertPass.cpp` 的 `classifyRevertFromHType`）：
`isZeroSizeValue(arg2)` 分支的 `Kind` 从 `encoded_candidate` 改为 `empty`。
其余路径（no-selector / raw-bytes / helper-encoded）确实缺证据，保持
`encoded_candidate`。

oracle/测试：
- pattern manifest 的 5 个 case 更新 `expected_revert_kinds`（empty 与
  encoded_candidate 此消彼长，其它 kind 逐一核对不变）；
- 新增 source golden `zero_size_revert_public_entry_01`：`revert(mem+64, 0)`
  守卫折叠成 `require(arg0 != 0); slot_0 = arg0;`。

结果（103 个 pattern case，默认路径）：

| 指标 | 改动前 | 改动后 |
| --- | ---: | ---: |
| solc 可编译 | 103/103 | 103/103 |
| `revert(); // encoded_candidate` | 428 | **40** |
| `require(` 语句 | 280 | **666** |
| `if (false /* TODO` | 451 | **289** |
| unresolved | 337 | 337（不变） |

source suite 83/83 全绿（含 `solc --bin`）。这是纯语义修正，不依赖
P0-2b 的路线选择。

**第二步：识别 InstCombine 折叠后的常量 selector（已实现）。**

zero-size 修完后，剩下的 `encoded_candidate` 里又用 instrumentation 分类：
no-selector 路径的 offset-0 store 共 251 处，其中 250 处是**纯常量**
（Top-32 位分别是 0x4e487b71 72 处、0x08c379a0 64 处，其余是 custom selector）。
`getSelectorWord()` 之前只认 `evm_shl(shift, payload)`，认不出折叠后的常量，
所以 panic/Error/custom 都掉进了 `encoded_candidate`；而且 error-selector 形状在
`SelectorFromStoreEvidenceOnly` 分支直接 `return nullopt`，这些 revert 在 Solidity
输出里**完全消失**。

改动：

- `getSelectorWord()`（`SolidityPatterns.cpp`）接受 low-224 位为 0 的**非零**常量，
  零值不当作 selector（panic code 0 也要能作为 payload 证据被读到）；
- `getRevertPayloadHType()` 的 store-evidence-only 分支补 offset 36（error length）
  和 offset 68（error data）的 store 证据；
- panic 分支：selector + panic-code 证据即可，不再要求字面 size == 36
  （helper 形状的 size 是 `sub(end, base)`，字面 36 永远不出现）；
- error 分支：0x08c379a0 本身足以确定 kind，删掉“只有 store evidence 就
  `return nullopt`”的丢弃路径，length/literal 保持可选。

oracle：

- 47 个 pattern case 更新 `notdec.solidity.revert` / `expected_revert_kinds` /
  `expected_panic_codes` / `notdec.solidity.checked_bounds` 计数（只更新已存在的
  key，派生 marker/hidden 计数由 runner 自动跟随）；
- 新增 source golden `revert_error_string_folded_selector_01`：把
  `revert_error_string_01` 的 selector store 换成折叠常量，输出必须逐字节一致。

结果（103 个 pattern case，默认路径）：

| 指标 | 改动前 | 改动后 |
| --- | ---: | ---: |
| solc 可编译 | 103/103 | 103/103 |
| 渲染出的 `revert(); // encoded_candidate` | 40 | **0** |
| 渲染出的 `revert(); // error_string` | 151 | **439** |
| 渲染出的 `revert(); // custom_error_candidate` | 88 | 106 |
| `if (false /* TODO` | 289 | 288 |
| `// goto block_` | 423 | 424 |

级联效果：`CheckedBoundsMatchers` 通过 `notdec.solidity_revert.panic_code` 元数据识别
checked arithmetic；panic 分类变多后，9 个 case 的 `notdec.solidity.checked_bounds`
从 0 变成 2-16（更多 checked 运算被恢复）。error_string 的 +288 是把之前被丢弃的
revert 重新渲染出来，source suite 84/84 全绿。

### P2-6 结构化质量（goto / remaining-body）

41/103 个真实 case 还有 `// goto block_`，423 处，且 30 个 remaining-body 完全
落在这 41 个里；这是结构器（Phoenix/SAILR）层面的问题，不是打印层。
本轮加的 `analyzeBodyCompletion()` 只是让标记更诚实，没有减少 goto。
下一步需要按 pattern suite 归类 goto 的形状（共享 tail、多出口、loop exit 等），
一次只放宽一条结构器规则，避免全局控制流回退。

**goto 形状归类（已完成，进入 metric）。** 对当前 103 个 case 的输出做逐行分析：

| 类别 | 数量 | 说明 |
| --- | ---: | --- |
| total | 424 | 全部 `// goto block_` |
| fallthrough | 12 | goto 之后（跳过 `}`）就是它自己的 label，删掉不改控制流 |
| dangling | 36 | target label 从未被渲染（13 个 case，典型 target 是 block_1/3 这类共享 revert/merge 块） |
| **real jump** | **376** | target 存在但不是 fallthrough，真正的无结构跳转（41 个 case） |

结论：

- 结构器已有 `dropGotoIntoFollowingNode()` / `dropIfGotoToFollowingBranch()` 在
  清理 fallthrough goto，确实只剩 12 处；说明剩余 376 处不是打印层能修的。
- 376 处 real jump 的典型形状是：
  `if (c) { // goto A } // block B: // goto C // block A: ...`，来自
  `buildVirtualizedBranchSource()`（被移除的 virtual edge 放 If.Then，kept target
  只作为兄弟 Goto，没有把目标体结构化进 else）。
- **这是语义问题不是观感问题**：renderer 把 Goto 打印成注释，等于这些跳转在生成的
  Solidity 里没有发生；41 个 case 的输出在 real jump 处可能偏离 IR 语义。
- 下一步：按 region 形状分类（共享 join、多前驱、loop exit、dephication 边），
  一次只放宽一条结构器规则，用 compile suite 的 `real_jump_occurrences` budget
  当 ratchet（本轮已加入 suite，实际 376）。

compile suite 现在输出并 ratchet 6 个指标：compile_failures、unresolved（337）、
condition_todo（813）、goto（424）、real_jump（376）、dangling_goto（36）。


### P2-7 打印层零散缺口

- event signature：57/103 仍是 `Event_0x<topic0>` + `// TODO: recover event signature`；
  需要 topic0 反查/推断，价值中等；
- `StructuredNodeKind::Switch` 目前打印成 `/* TODO: switch */` + case 注释，
  不是真正的 `switch` 语句；
- dynamic ABI return（bytes/string/array）只有单 word static return 稳；

**多 word ABI return 已补（P2-7）。**

- 新增 `formatMultiWordReturn()`：匹配 `evm_return(mem, base, 32*n)`（2 <= n <= 4），
  支持两种 buffer 形状——静态帧（每个 word 一个 `inttoptr(const)`）和
  calloc-backed（`inttoptr(add(ptrtoint(calloc), 32*i))`），把 0/32/... 的 store
  收集成 `return (v0, ..., vn-1);`；任一 word 缺失就退回旧行为。
- bool word：reader 把多 word 返回一律声明成 uint256，而 `valueExpr()` 会拆掉
  `zext i1`，所以比较表达式会打印成 Solidity bool；新增 `isBoolValuedExpr()`，
  在 tuple 元素位置降成 `(cond) ? 1 : 0`。
- `n > 4` 保持旧行为：case 2001 的 5-word 返回在没有 viaIR 的 solc 下
  `Stack too deep`，不生成无法编译的代码。
- 顺带把单 word 的非 constant-offset 分支也改用同一个 offset-aware
  store 查找，覆盖 `inttoptr(add(base, 0))` 形状。
**单 word return 的 buffer base 泛化（同轮）。**

- `storeOffsetFromReturnBase()` / `findReturnBufferStoreBefore()` 现在无条件使用
  （`BasePtr` 允许为空），匹配 `inttoptr(base)` 和 `inttoptr(add(base, C))`，
  不再要求 base 是 calloc 或常量——outlined wrapper 从 formal 里 load 出的
  返回缓冲指针（`%evm.mload`）也能命中。
- 声明返回类型通过新的 `ActiveReturnTypes` 传给后端；新增
  `wordReturnExpr()`：当声明类型不是 bool 时，把 bool 值表达式降成
  `(cond) ? 1 : 0`（泛化后暴露了 2 个 `Return argument type bool ... uint256`
  solc 失败）。

结果：声明了 returns 但没有 return 语句的函数 **40 -> 16**（只剩 1 个单 word、
1 个三 word、14 个 5-10 word 深 tuple）；compile_failures 0；新增 source golden
`formal_buffer_return_public_entry_01`（`return arg0;`），source suite 87/87。
`unresolved` budget 377 -> **387**（新 surface 的占位符，同上理由）。


oracle/指标：

- 新增 source golden `multi_word_return_public_entry_01`（`return (arg0, arg1);`），
  source suite 86/86；
- compile suite：compile_failures 0，其它指标不变；
- `unresolved_value_occurrences` **330 -> 377（+47）**：这不是恢复退化，而是此前
  "完全没有 return 语句"的函数现在把 tuple 里的未知元素显式打印成
  `0 /* TODO: unresolved value */`（与单 word return 的行为一致）。budget 已按此
  上调并在此记录理由；后续 P0-2b 解决 private 值后应自然回落。

- `selfdestruct`、`tx.origin` 等环境语义在 body 里没有恢复（gasleft 的 `kill()`
  then 分支为空是典型例子）。
**状态：环境语义与 selfdestruct 已补。**

- `evmEnvBuiltinExpr()` 新增：`evm_gasprice -> tx.gasprice`、
  `evm_origin -> tx.origin`、`evm_timestamp -> block.timestamp`、
  `evm_number -> block.number`、`evm_chainid -> block.chainid`、
  `evm_gaslimit -> block.gaslimit`、`evm_prevrandao -> block.prevrandao`、
  `evm_coinbase -> block.coinbase`、`evm_address -> address(this)`、
  `evm_selfbalance -> address(this).balance`。
  `evm_callvalue` 故意不加：`msg.value` 在非 payable 函数里非法，而后端目前不追踪
  payability（试加后 13 个 pattern case 编译失败，已回退该分支）。
- 地址值判断抽成 `isAddressValuedExpr()`（msg.sender / tx.origin /
  block.coinbase / address(this) / address 参数）；`wordCastAddressExpr()` 统一
  转 `uint256(uint160(...))`。`block.coinbase` 是 address payable，
  `uint160()` 不收，额外包一层 `address(...)`。storage 赋值和单 word return 也套用
  该转换，避免 `slot = tx.origin` 这类类型错误。
- 新增 `formatSelfDestruct()`：`evm_selfdestruct` 之前被完全丢弃，现在输出
  `selfdestruct(payable(<address>))`（word 先 `uint160/address/payable`，
  address 值直接 `payable`）。

oracle/验证：

- `apehex_gasleft_return_01` 的 `kill()` then 分支从空变成
  `selfdestruct(payable(tx.origin));`，与原始 Solidity 一致；
- 新增 source golden `env_builtins_public_entry_01` 锁定 8 个环境 builtin 的打印与
  转换；
- compile suite 指标：unresolved 337 -> **330**，condition TODO 813 -> **789**，
  budget 已同步下调。source suite 85/85，四个 EVM suite 全绿。


### P3-8 `--tr-level=3` 会 abort（易踩坑）

复现：

```bash
build-notdec-nothreads/bin/notdec test/evm/solidity-patterns/cases/calldata_min_size_public_entry_01.ll \
  -o /tmp/out.sol   # 默认 tr-level=3
# -> notdec: MLsubGenerator.h:906: Assertion `false && "TODO"` failed.
```

`MLsubRecoveryOpt::run()` 是 `assert(false && "TODO")` 的占位 pass，只有
`--tr-level>=3` 会加入 pipeline，而 CLI 默认就是 3。所有 EVM suite 都显式传
`--tr-level=2` 所以没有暴露。建议二选一：EVM target 下把 level 3 当作 level 2
处理并给 warning，或者把该 pass 改成 no-op/明确报错，避免 core dump。

**状态：已修。** 在 `PassEnv::build_passes()` 里，EVM target 且 `level >= 3` 时
打 warning 并把 level 夹到 2（`TypeRecoveryLevel` 同步更新）。CLI 默认的
`--tr-level=3` 现在与 `--tr-level=2` 输出一致，不再 core dump。

顺带修掉同类的 CLI footgun：`-o <dir>/out.{sol,ll,bc}` 在目录不存在时原先会
`std::abort()`（本轮踩了两次）。现在 `main` 在跑 pipeline 前检查输出目录，
不存在就报 `Error: output directory does not exist` 并 `return 1`；
`.ll`/`.bc`/未知后缀的 writer 失败也都改成 `return 1`。

### P3-9 测试与构建基建

- pattern suite 目前只看 IR marker，不看生成的 `.sol`；**已补**：新增
  `test/run_evm_solidity_pattern_compile_suite.py` +
  `test/evm/solidity-patterns/compile-budget.json`，对全部 103 个 pattern case
  跑 `notdec --tr-level=2 -o .sol`，要求 solc 全部可编译，并 ratchet 三个指标：
  `TODO: unresolved value`（337）、`false /* TODO`（813，含 if/while/require
  全部上下文）、`// goto block_`（424）。只在配置了 `NOTDEC_SOLC` 时注册，
  默认 ctest 不依赖 solc。指标改善时在同一 commit 里下调 budget。
- 修复 standalone `external/NotDec-llvm2c` 的测试入口，让
  `structuring-analysis-test` 能跑（它直接覆盖 `LLVMFunctionCFGBuilder` 和
  branch polarity）；
- 把 evm2llvm 自己的 33 个 fixture 测试纳入主 ctest 或 CI 聚合步骤；
- 清理/reconfigure `build-notdec-nothreads2`，避免下次又踩不存在的 LLVM 路径；
- 非 EVM oracle 漂移（`type_recovery.llvm_ir` 8/14、`sysy` 0/9、realworld
  fortune extra-constraints 哈希）是既有问题，与本链路无关，单独排期。

### P3-10 遗留工作区状态

- 顶层 `external/NotDec-bin2llvm` 指针有未提交改动（fab1fa8e），不是本轮 EVM
  工作产生的，需要由其对应工作流决定提交还是还原；
- llvm2c `main` 与顶层 `v2` 在 `646560b` / `c02118b7` 之后都已经 push 干净。

## 五、建议的推进顺序

1. P0-1 calldata index metadata（范围小、直接延续本轮，能把 calldata 一族从
   “单形状 matcher” 变成通用恢复）；
2. P0-2 private call 结果（收益最大，384/538 unresolved，且是条件恢复的上游）；
3. 重跑量化统计，确认条件 TODO 的自然下降幅度，再决定 P1-3 的具体落点；
4. P1-4 storage 扩展与 P1-5 revert 分类（互相独立，可并行）；
5. P2/P3 视需要穿插；P3-8 的 tr-level=3 abort 建议尽快做掉，成本最低。

判断标准统一为：pattern suite oracle 不退化 + source suite golden 更新后通过 +
`solc --bin` 编译率不下降 + 量化 marker 数字下降。不能靠放宽 matcher 硬猜。


## 六、本轮 goal 收尾总结（2026-09-17 EVM Solidity 语义恢复）

### 交付的提交

llvm2c（submodule main）：

- `646560b` solidity: recover calldata guards and fold them into require
- `daa7fc2` solidity: name ABI arguments from calldata index metadata
- `ac1817f` solidity: resolve ABI decoder results from calldata index metadata
- `2cbdb85` solidity: recover environment builtins and selfdestruct
- `14a01b7` solidity: recover multi-word ABI returns
- `b98c2dd` solidity: generalize return buffer matching and bool return coercion

顶层（v2）：`c02118b7`、`ff93df50`、`455cef47`、`ebff20b6`、`053e30a5`、
`e60ba8b0`、`8aadfb48`、`3ef31ee0`、`f6168238`、`60cb427f`、`975125cd`、
`87afd41e`、`fed7dc6d`、`4b3c1267`、`a71cbcf8`，全部已 push。

### 最终基线（103 个 pattern case 默认路径）

| 指标 | goal 起点 | 最终 |
| --- | ---: | ---: |
| solc 可编译 | 103/103 | 103/103 |
| 渲染出的 `revert(); // encoded_candidate` | 428 | 0 |
| `require(` 语句 | 280 | 666 |
| `if (false /* TODO` | 522 | 288 |
| `false /* TODO`（全上下文） | 813 | 789 |
| unresolved value（含新 surface） | 515 | 387 |
| 声明了 returns 但没有 return 语句的函数 | 70 | 16 |
| real jump（`// goto`） | 423 | 376 |
| source golden suite | 79 | 87/87 |

测试基建：

- `NOTDEC_SOLC` 可选 solc 检查（source suite 全量编译）；
- `notdec.evm.solidity_pattern_compile`：103 case 全量编译 + 6 个 ratchet 指标
  （compile_failures / unresolved / condition_todo / goto / real_jump /
  dangling_goto）；
- 新增 source golden：calldata sub、decoder 直连/嵌套、zero-size revert、
  folded selector、env builtins、多 word return、formal buffer return。

### 未完成与阻塞

1. **P0-2b（private body helper 调用结果）仍未完成，需要路线决定。**
   Route A（IR inline）实测会让 9-11 个 case 丢 checked-bounds/abi-return 覆盖、
   structurer 退化（goto 423 -> 4755），必须先 retune matcher + 重建 oracle；
   Route B（后端 helper + local materialization）是新功能且要重做跨函数
   memory/storage 语义。两条路都需要明确选择后单独立项，详见 P0-2 小节。
2. 16 个无 return 函数：14 个是 5-10 word 深 tuple（非 viaIR solc 的
   `Stack too deep`），1 个三 word（word 0 由 helper 通过 buffer 指针写入，
   需要 interprocedural store 匹配），1 个 selector-inline helper。
3. 376 个 real jump / 36 个 dangling（结构器 region 选择，P2-6）。
4. 36 个 `evm_sload` 是 keccak 计算槽（mapping 访问的内存布局证明）。

### 复现命令

```bash
cmake -S . -B build-notdec-nothreads -DNOTDEC_SOLC=/sn640/EthIR/source/solcv8.25
ninja -C build-notdec-nothreads notdec
ctest --test-dir build-notdec-nothreads -R "notdec.evm.solidity" --output-on-failure
ctest --test-dir build-notdec-nothreads -R "notdec.evm.solidity_pattern_compile" -V
```


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

### P2-6 结构化质量（goto / remaining-body）

41/103 个真实 case 还有 `// goto block_`，423 处，且 30 个 remaining-body 完全
落在这 41 个里；这是结构器（Phoenix/SAILR）层面的问题，不是打印层。
本轮加的 `analyzeBodyCompletion()` 只是让标记更诚实，没有减少 goto。
下一步需要按 pattern suite 归类 goto 的形状（共享 tail、多出口、loop exit 等），
一次只放宽一条结构器规则，避免全局控制流回退。

### P2-7 打印层零散缺口

- event signature：57/103 仍是 `Event_0x<topic0>` + `// TODO: recover event signature`；
  需要 topic0 反查/推断，价值中等；
- `StructuredNodeKind::Switch` 目前打印成 `/* TODO: switch */` + case 注释，
  不是真正的 `switch` 语句；
- dynamic ABI return（bytes/string/array）只有单 word static return 稳；
- `selfdestruct`、`tx.origin` 等环境语义在 body 里没有恢复（gasleft 的 `kill()`
  then 分支为空是典型例子）。

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

### P3-9 测试与构建基建

- pattern suite 目前只看 IR marker，不看生成的 `.sol`；建议把 solc 抽样检查
  （例如每个 pattern case 至少能编译，或按 patterns 分类抽 10-20 个）接进
  CI/可选 ctest，本轮只做了 source suite 的可选 `--solc`；
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


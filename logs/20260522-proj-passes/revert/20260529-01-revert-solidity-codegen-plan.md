# Revert 语义恢复的 Solidity codegen 对照计划

## 原始 prompt

尝试规划一下logs/20260522-01-Evm2llvmSolidityPassOrderingPlan.md里面2.3节的部分怎么实现，规划单独写到一个logs/下的文件

## 背景

`logs/20260522-01-Evm2llvmSolidityPassOrderingPlan.md` 的 2.3 节要规划
Revert、Panic、Error、custom error 和 returndata bubble 的恢复顺序。这里补一份更具体的实现计划，重点是直接对照 Solidity 编译器源码生成的形状。

当前 `AGENTS.md` 只保留长期原则：做 EVM Solidity 语义恢复时优先对照 `/sn640/solidity` 的 codegen，不只从样例猜。具体源码位置和实现判断放在这份日志里。

## Solidity 源码依据

主要参考 `/sn640/solidity`：

- Panic code 定义：`libsolutil/ErrorCodes.h`
  - `0x00` generic
  - `0x01` assert
  - `0x11` arithmetic underflow / overflow
  - `0x12` division / modulo by zero
  - `0x21` enum conversion
  - `0x22` invalid storage bytes/string encoding
  - `0x31` empty array pop
  - `0x32` array out of bounds
  - `0x41` resource error
  - `0x51` invalid internal function
- Panic 生成：`libsolidity/codegen/YulUtilFunctions.cpp` 的
  `YulUtilFunctions::panicFunction()`。典型 Yul 形状是
  `mstore(0, selector(Panic(uint256)))`、`mstore(4, code)`、
  `revert(0, 0x24)`。
- returndata bubble：同文件 `YulUtilFunctions::forwardingRevertFunction()`。
  典型形状是申请一段内存，`returndatacopy(pos, 0, returndatasize())`，
  再 `revert(pos, returndatasize())`。旧 codegen 里也会出现
  `CompilerContext::appendConditionalRevert(true, ...)` 生成的
  `returndatacopy(0, 0, returndatasize())` / `revert(0, returndatasize())`。
- `Error(string)` 和 custom error：`CompilerUtils::revertWithStringData()`、
  `CompilerUtils::revertWithError()`、IR 路线的
  `IRGeneratorForStatements::revertWithError()`。共同形状是先写 selector，
  再从 selector 后 4 字节开始 ABI encode 参数，最后按编码长度
  `revert(start, size)`。

## 总体目标

这件事分两步做。

第一步先不急着改 pass。先把当前测试用例里已经出现的 revert 相关低层形状盘清楚，
做成 oracle，并把测试框架补到能检查这些 oracle。否则 pass 改完只能看总数，
不知道命中的是不是对的位置、对的 kind、对的 panic code。

第二步再基于这些 oracle 推进 `SolidityRevertPass`。主要目标放在 rewrite 上，
metadata 只作为内部 debug 和测试 oracle 的辅助，不作为最终成果。当前
`AbiRevertEncodingPass` 的 marker-only 价值不大，后续考虑去掉，把能确认的
revert buffer / selector 识别并入 `SolidityRevertPass` 的 matcher 和 rewrite 结果。

- `Panic(uint256)` 要恢复 selector 和 panic code。
- returndata bubble 要识别 `pos` 和 `returndatasize()` 的一致性。
- `Error(string)` / custom error 先恢复 selector 和 revert buffer，暂不完整解析 ABI 参数。
- `revert(0,0)` 保守保留，只有被 payability、ABI bounds、selector fallback reject 等上游语义认领后才隐藏或改写。

## 第一步：整理当前用例和 oracle

范围先限制在现有 `test/evm/solidity-patterns/cases/`。不要先扩样例。先回答：

- 现有 case 里有多少个 `evm_revert`。
- 每个 `evm_revert` 属于哪种初步形状：
  - `empty`: `revert(0, 0)`
  - `panic`: Panic selector + code + `revert(0, 0x24)`
  - `returndata_bubble`: `returndatacopy(pos, 0, returndatasize())` 后接
    `revert(pos, returndatasize())`
  - `error_string`: `Error(string)` selector + ABI string buffer
  - `custom_error_candidate`: 非 Panic / Error 的 4-byte selector + encoded buffer
  - `encoded_candidate`: 有非空 buffer 但 selector 暂时不明
- 哪些 `empty revert` 已经被 payability CFG rewrite 消费，哪些仍然是 ABI bounds、
  selector fallback reject 或用户逻辑候选。

建议先写一个轻量统计脚本或扩展现有 runner 的 debug 输出，读取 notdec 输出后的
`out.ll`，按 `evm_revert` 周边指令做保守分类，生成每个 case 的明细文件。明细先服务人工确认，
不直接作为最终 oracle。

需要扩展 `test/run_evm_solidity_patterns_suite.py` 的 oracle 能力。当前 runner 主要检查
metadata 总数、rewrite marker 总数和 payability CFG rewrite 总数。revert 这部分需要至少支持：

- `expected_revert_kinds`：按 kind 计数。
- `expected_panic_codes`：按 panic code 计数。
- `expected_returndata_bubbles`：returndata bubble 数量。
- 后续必要时再加“函数名 + block 名 + kind”的位置 oracle，避免总数正确但命中错位。

第一步的产物：

- manifest 里补 revert kind / panic code / returndata bubble oracle。
- runner 能检查这些 oracle。
- 每个新增 oracle 都能回到当前 case 的具体 IR 位置。
- 如果某类形状暂时无法人工确认，就不要写强 oracle，先写到日志的待确认列表。

## 第二步：推进 pass

`SolidityRevertPass` 保持这个名字，不再拆出一堆小 pass。内部先把 revert
matcher 做成一个小的结构化结果。这个结果同时覆盖退出语义和已确认的 ABI revert
buffer 外壳，避免 `SolidityRevertPass`、`AbiRevertEncodingPass`、
`CheckedBoundsPass` 各自重复猜 panic / selector。

`AbiRevertEncodingPass` 先不要继续扩大。当前它主要插入
`notdec_solidity_rewrite_abi_revert_encoding` 和 `notdec_solidity_rewrite_hidden`，
但没有真正合并 buffer、没有恢复参数，也没有被后续 pass 消费。后续更合适的方向是：

- 删除 `AbiRevertEncodingPass`，或者至少从 pipeline 中移除。
- 把它已有的 selector / encoded revert 判断并入 `SolidityRevertPass` 的 matcher。
- 对外只保留 `SolidityRevertPass` 产出的结构化 rewrite 结果。
- metadata 继续保留，但只用于 debug、统计和 oracle。

建议第一版结果至少记录：

- `evm_revert` 调用。
- kind：`empty`、`panic`、`error_string`、`custom_error_candidate`、
  `returndata_bubble`、`encoded_candidate`。
- revert offset 和 length。
- selector store。
- panic code store。
- returndata copy。

同一个 `evm_revert` 只能归到一个主要 kind。matcher 按优先级分类，避免同一处既被当成
encoded revert，又被当成 panic 或 bubble：

1. `returndata_bubble`：先识别外部调用失败后的原样冒泡。它虽然也是
   `revert(pos, size)`，但语义是转发外部 returndata，不应该再进入本合约 ABI
   revert buffer 分类。
2. `panic`：再识别 `Panic(uint256)` selector、panic code 和 `revert(0, 0x24)`。
   这类结果会被 checked arithmetic、bounds check、storage bytes/string 等后续逻辑使用。
3. `error_string`：识别 `Error(string)` selector。第一阶段只确认 selector 和 buffer 外壳，
   不完整解析 string 参数。
4. `custom_error_candidate`：识别其它 4-byte selector 的 encoded revert。第一阶段只作为
   custom error 候选，不猜具体错误名和参数类型。
5. `empty`：最后处理 `revert(0,0)`。默认保留低层退出，只有被 payability、ABI bounds、
   selector fallback reject 等上游语义认领后才 rewrite。
6. `encoded_candidate` / `candidate`：剩下的非空 revert 只作为候选，不做强 rewrite。

实现顺序：

1. 完善 Panic。匹配 `mstore(0, Panic selector)`、`mstore(4, code)`、
   `revert(0, 0x24)`，先支持常量 code。命中后插入真正有语义的 rewrite marker，
   例如 `notdec_solidity_rewrite_revert_panic(code)`；metadata 只同步标出
   panic kind 和 code，方便测试。
2. 完善 returndata bubble。现有代码只看同一 block 和 `revert(0, size)`，
   下一步要支持 `revert(pos, returndatasize())`，并确认前面有
   `returndatacopy(pos, 0, returndatasize())`。命中后插入能表达 bubble 语义的
   rewrite marker，而不是只给 `evm_revert` 加一个通用 hidden marker。
3. 识别 Error / custom error selector。`0x08c379a0` 归为
   `error_string`，`0x4e487b71` 归为 `panic`，其它 4-byte selector 先归为
   `custom_error_candidate`。第一版 rewrite 只表达 selector 和 buffer，不完整恢复
   参数结构。
4. 处理 `revert(0,0)` 归属。默认只标 `empty`，不隐藏。后续由 payability、
   ABI decode bounds、selector fallback reject 等 pass 消费后再改写。

## 风险

- 第一步的自动分类只能辅助做 oracle，不能直接替代人工确认。尤其是
  `encoded_candidate` 和 `custom_error_candidate`，总数容易对，语义可能错。
- 如果一开始只补总数 oracle，后续 pass 可能把命中位置改错但测试仍通过。
  所以 panic code 和 returndata bubble 至少要有 kind 级 oracle，必要时加位置 oracle。
- `evm_mstore` 和 `evm_revert` 可能不在同一 basic block。第一版可以先保守处理同 block 或支配关系很清楚的情况，不要为了命中率猜跨块数据流。
- optimizer 可能把 `0x24`、selector、code 变成 SSA 表达式。第一版先支持常量和简单 `evm_shl(224, selector)`。
- `Error(string)` 和 custom error 的 ABI 结构依赖 memory buffer。revert pass 不要提前变成 ABI decoder。
- `revert(0,0)` 来源太多，不能仅凭形状认定为编译器保护。
- rewrite marker 必须比现在的通用 marker 更有语义。只插入
  `notdec_solidity_rewrite_abi_revert_encoding(i256)` 这种 marker 意义不大，
  因为它既不表达 kind，也不表达 panic code、selector、buffer。
- 删除或停用 `AbiRevertEncodingPass` 会影响现有 `notdec.solidity.abi_revert_encoding`
  oracle。需要先在第一步把新的 revert oracle 补好，再调整旧 oracle，避免测试只是在
  计数变化。

## 判断标准

第一步完成标准：

- 现有 `test/evm/solidity-patterns/cases/` 中 revert 相关形状完成盘点。
- `manifest.json` 能表达 revert kind、panic code、returndata bubble 这类 oracle。
- `test/run_evm_solidity_patterns_suite.py` 能检查这些 oracle，并在失败时输出 expected / actual。
- `notdec.evm.solidity_patterns` 通过，证明新增 oracle 与当前输出一致。
- 日志里记录哪些形状已经确认，哪些暂时只作为 candidate。

第二步完成标准：

- `notdec.evm.solidity_patterns` 通过。
- `notdec.evm.solidity_rewrite` 不退化。
- `SolidityRevertPass` 产出结构化 rewrite marker，至少能区分 panic、
  returndata bubble、error selector / custom error candidate。
- metadata 仍可用于 debug 和 oracle，但不把 metadata-only 当成主要成果。
- 现有 `notdec.solidity.revert` 总命中不下降，除非能解释为分类变细或 oracle 变严。
- Panic case 能稳定提取 panic code，并通过 oracle 检查。
- Returndata bubble 能覆盖 `revert(0, returndatasize())` 和
  `revert(pos, returndatasize())` 两类 Solidity 生成形状。
- `Error(string)` / custom error 第一阶段只增加 selector / buffer 识别，不要求完整恢复参数。
- `AbiRevertEncodingPass` 的去留有明确结论：要么移除并迁移 oracle，要么保留为临时兼容层，
  但不再作为主要语义恢复入口。

## 第一步实现记录（2026-05-29）

本次只完成测试框架和 oracle，不改 pass。

- `test/run_evm_solidity_patterns_suite.py` 新增 revert 形状分类：
  `empty`、`panic`、`returndata_bubble`、`error_string`、
  `custom_error_candidate`、`encoded_candidate`。
- 同文件新增 panic code 统计，当前只提取同一 block 内
  `mstore(0, shl(224, 0x4e487b71))`、`mstore(4, 常量)`、
  `revert(..., 36)` 这种形状。
- 同文件新增 returndata bubble 统计，支持
  `returndatacopy(pos, 0, returndatasize())` 后接
  `revert(pos, returndatasize())`，包括 `pos=0` 和 SSA `pos`。
- `test/evm/solidity-patterns/manifest.json` 为 58 个现有 case 增加：
  `expected_revert_kinds`、`expected_panic_codes`、
  `expected_returndata_bubbles`。

当前 58 个 case 的总量：

- `empty`: 3109
- `panic`: 610
- `returndata_bubble`: 294
- `encoded_candidate`: 1670
- `error_string`: 0
- `custom_error_candidate`: 0

当前 panic code 总量：

- `0x01`: 1
- `0x11`: 166
- `0x12`: 30
- `0x21`: 10
- `0x22`: 25
- `0x31`: 10
- `0x32`: 277
- `0x41`: 91

`encoded_candidate` 暂时只作为候选，没有强行标成 Error(string) 或 custom error。
当前 case 里未稳定识别到 `0x08c379a0` selector 或其它 custom selector 外壳。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 88.61s。

## 第二步实现记录（2026-05-29）

本次实现 `SolidityRevertPass` 的第一版结构化 matcher，暂不移除
`AbiRevertEncodingPass`。

修改内容：

- `src/Passes/evm/SolidityPatterns.cpp:84` 增加 Panic / Error selector 常量。
- `src/Passes/evm/SolidityPatterns.cpp:90` 增加 `SolidityRevertMatch`，记录
  `evm_revert`、selector store、panic code store、returndatacopy、selector、
  panic code 和 kind。
- `src/Passes/evm/SolidityPatterns.cpp:1044` 增加常量和 selector 提取 helper：
  `isSameValue()`、`getUInt64Constant()`、`getSelectorWord()`。
- `src/Passes/evm/SolidityPatterns.cpp:1079` 改进 returndata bubble 匹配，
  `findReturndataBubbleCopy()` 支持 `revert(pos, returndatasize())`，要求前面有
  `returndatacopy(pos, 0, returndatasize())`。
- `src/Passes/evm/SolidityPatterns.cpp:1124` 增加 `matchSolidityRevert()`，按
  `empty`、`returndata_bubble`、`panic`、`error_string`、
  `custom_error_candidate`、`encoded_candidate` 归类。
- `src/Passes/evm/SolidityPatterns.cpp:1187` 增加
  `notdec_solidity_rewrite_revert_panic(code)` marker。
- `src/Passes/evm/SolidityPatterns.cpp:1211` 增加
  `notdec_solidity_rewrite_revert_returndata_bubble(kind)` marker。
- `src/Passes/evm/SolidityPatterns.cpp:1236` 增加
  `addRevertMatchMetadata()`，保留 `notdec.solidity.revert` 主 metadata，同时把
  panic code / selector / returndata copy 写到 `notdec.solidity_revert.*` 辅助
  metadata，避免影响旧 `notdec.solidity.revert` 计数。
- `src/Passes/evm/SolidityPatterns.cpp:1627` 让 `SolidityRevertPass::run()` 使用
  新 matcher，并为 panic / returndata bubble 插入专用 rewrite marker。
- `test/run_evm_solidity_patterns_suite.py:132` 增加 `count_exact_marker()`。
- `test/run_evm_solidity_patterns_suite.py:383` 和 `:429` 增加
  `notdec_solidity_rewrite_revert_panic`、
  `notdec_solidity_rewrite_revert_returndata_bubble` 的 oracle 检查。
- `test/evm/solidity-patterns/manifest.json:1469` 将
  `1775_19507435_1be1a16c5d_eb304133e7c9` 的
  `notdec.solidity.abi_revert_encoding` 从 106 调整为 105。原因是新的
  `revert(pos, returndatasize())` bubble 匹配让 `AbiRevertEncodingPass` 少误标
  1 个 encoded revert。

效果：

- 现有 `notdec.solidity.revert` 总命中不下降。
- panic code 和 returndata bubble 的原有 oracle 保持一致。
- 新增 rewrite marker 已纳入 suite 检查，不再只是 metadata-only。
- `AbiRevertEncodingPass` 本次保留为临时兼容层，后续再决定移除或迁移。

复杂度评价：

- 实现效果：8/10。覆盖了同 block 的 panic、selector 和
  `revert(pos, returndatasize())` bubble；跨 block 数据流还没有做。
- 理解成本：6/10。多了一个 `SolidityRevertMatch`，但比在多个 pass 里重复猜形状更清楚。
- 后期维护成本：6/10。辅助 metadata 和专用 marker 比旧通用 marker 更明确；后续删除
  `AbiRevertEncodingPass` 时还需要整理 oracle。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)' --output-on-failure`
  通过，2 个测试全部通过，耗时 173.66s。

性能：

- 本次只改 EVM Solidity metadata/rewrite matcher，没有改类型恢复、结构体合并、
  pointer analysis 或主 pass pipeline。验证用例同口径耗时：
  `notdec.evm.solidity_patterns` 87.58s，`notdec.evm.solidity_rewrite` 86.08s。

## 第三步实现记录（2026-05-29）

本次按后续决定移除 `AbiRevertEncodingPass`，并补 Error(string) / custom error
最小 IR 覆盖。新增 case 只覆盖 selector + revert 外壳，不解析 ABI 参数，也不是
Solidity 源码端到端样例。

修改内容：

- `src/Passes/PassManager.cpp:293` 从 EVM pipeline 移除
  `AbiRevertEncodingPass`。
- `include/notdec/Passes/evm/SolidityPatterns.h:13` 删除
  `KIND_SOLIDITY_ABI_REVERT_ENCODING` 声明，`:92` 附近删除
  `AbiRevertEncodingPass` 声明。
- `src/Passes/evm/SolidityPatterns.cpp:27` 删除
  `NumAbiRevertEncodings` 统计，`:58` 删除
  `KIND_SOLIDITY_ABI_REVERT_ENCODING` 定义，`:1653` 附近删除
  `AbiRevertEncodingPass::run()` 实现。
- `src/Passes/evm/SolidityPatterns.cpp:1224` 增加
  `insertSelectorRewriteMarker()`，用于 selector 型 revert marker。
- `src/Passes/evm/SolidityPatterns.cpp:1640` 的
  `SolidityRevertPass::run()` 在 `error_string` 时插入
  `notdec_solidity_rewrite_revert_error_string(selector)`，在
  `custom_error_candidate` 时插入
  `notdec_solidity_rewrite_revert_custom_error(selector)`。
- `test/run_evm_solidity_patterns_suite.py:393` 和 `:456` 增加 Error(string) /
  custom error 专用 rewrite marker 计数检查。
- `test/evm/solidity-patterns/manifest.json:10` 开始新增 10 个最小 case oracle：
  4 个 `error_string`，6 个 `custom_error_candidate`。同时移除所有
  `notdec.solidity.abi_revert_encoding` oracle 和 `abi_revert_encoding` pattern。
- 新增 10 个 case：
  - `test/evm/solidity-patterns/cases/revert_error_string_01.ll:10`
  - `test/evm/solidity-patterns/cases/revert_error_string_02.ll:10`
  - `test/evm/solidity-patterns/cases/revert_error_string_03.ll:10`
  - `test/evm/solidity-patterns/cases/revert_error_string_04.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_01.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_02.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_03.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_04.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_05.ll:10`
  - `test/evm/solidity-patterns/cases/revert_custom_error_06.ll:10`

效果：

- `AbiRevertEncodingPass` 已不在 pipeline 中，声明和实现也已删除。
- Error(string) 和 custom error 现在有专用 rewrite marker。
- 当前只确认 selector 和 revert buffer 外壳；错误名、参数类型、string 内容还没有恢复。
- 旧 `notdec.solidity.abi_revert_encoding` marker-only oracle 已从 pattern suite 移除。

复杂度评价：

- 实现效果：7/10。删掉了重复 pass，并把 Error/custom selector 变成可测 marker。
  但还不是 ABI 参数恢复。
- 理解成本：5/10。少了一个 pass，整体更简单；新增 marker 逻辑复用
  `SolidityRevertMatch`。
- 后期维护成本：5/10。后续只需要沿着 `SolidityRevertPass` 扩展，不再维护两套 revert
  判断。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 87.06s。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`
  通过，耗时 84.76s。

性能：

- 本次改动仍只影响 EVM Solidity matcher 和测试，不涉及类型恢复、结构体合并、
  pointer analysis 或主类型恢复 pipeline。`notdec.evm.solidity_patterns` 同口径从
  87.58s 变为 87.06s，`notdec.evm.solidity_rewrite` 从 86.08s 变为 84.76s。

## 第四步实现记录（2026-05-29）

本次把第三步新增的 10 个手写 IR case 全部替换为 Solidity codegen 来源的 IR。
没有把 Solidity 编译接进 CTest：当前本机没有固定 `solc` 二进制，生成链路还依赖
`npx solc@0.8.26`、Gigahorse 和 `evm2llvm`，放进常规测试会让 suite 依赖网络和较重
外部工具。当前保留 `.sol` 源码和转换后的 `.ll` 输入，CTest 仍只跑稳定的 IR。

修改内容：

- 新增 10 个 Solidity 源文件：
  - `test/evm/solidity-patterns/solidity/revert_error_string_01.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_error_string_02.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_error_string_03.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_error_string_04.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_01.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_02.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_03.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_04.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_05.sol:1`
  - `test/evm/solidity-patterns/solidity/revert_custom_error_06.sol:1`
- 覆盖第三步的 10 个 IR 输入：
  - `test/evm/solidity-patterns/cases/revert_error_string_01.ll:1`
  - `test/evm/solidity-patterns/cases/revert_error_string_02.ll:1`
  - `test/evm/solidity-patterns/cases/revert_error_string_03.ll:1`
  - `test/evm/solidity-patterns/cases/revert_error_string_04.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_01.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_02.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_03.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_04.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_05.ll:1`
  - `test/evm/solidity-patterns/cases/revert_custom_error_06.ll:1`
- `src/Passes/evm/SolidityPatterns.cpp:1041` 扩展 `isSameValue()`，支持识别相同
  `evm_mload(ptr, offset)`，用于真实 Solidity free-memory-pointer 形状。
- `src/Passes/evm/SolidityPatterns.cpp:1070` 新增 `getOffsetFromBase()`，支持
  `base`、`base + 常量`、常量 offset 差值。
- `src/Passes/evm/SolidityPatterns.cpp:1099` 改进 `getSelectorWord()`，不再只认
  `shl(224, selector)`，而是解析 Solidity 常见的 `PUSH3/PUSH4 + SHL` selector word。
- `src/Passes/evm/SolidityPatterns.cpp:1169` 的 `matchSolidityRevert()` 改为按
  `revert` 起始地址寻找 selector store 和 panic code store。
- `test/run_evm_solidity_patterns_suite.py:27`、`:170`、`:221` 同步扩展 runner 的
  文本分类逻辑，让 oracle 能识别真实 Solidity IR 里的 selector word 和
  free-memory-pointer offset。
- `test/run_evm_solidity_patterns_suite.py:476` 将 panic rewrite marker 的预期改为
  已提取到具体 panic code 的数量。少数真实 IR 能确认 panic selector 和 kind，
  但 code 不是当前轻量文本分类能稳定提取的常量。
- `test/evm/solidity-patterns/manifest.json:10` 起更新新增 10 个 case 的 oracle。
  真实 runtime 自带 dispatcher、fallback 和非 payable 保护，所以这些 case 不再是
  “只有 1 个 revert”的最小手写形状。
- `test/evm/solidity-patterns/manifest.json` 同步更新旧真实样例的
  `expected_revert_kinds`。这不是旧命中下降，而是 selector 解析变强后，一部分原
  `encoded_candidate` 被细分为 `error_string` 或 `custom_error_candidate`。

生成命令：

```bash
npx --yes solc@0.8.26 --standard-json
python3 external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  <runtime-bytecode.hex> \
  -o <case>.ll \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --evm2llvm build-evm2llvm/bin/evm2llvm \
  --timeout-secs 120 \
  --jobs 1
```

效果：

- 10 个 Error(string) / custom error case 不再是手写 IR，均来自 Solidity
  0.8.26 optimized runtime bytecode 经过 Gigahorse 和 `evm2llvm` 转换后的 IR。
- `SolidityRevertPass` 能识别真实 codegen 里的 selector 写法：
  `shl(229, 0x461bcd)` 这类压缩 `Error(string)` selector，以及
  `revert(freeMemoryPointer, size)` 这类 buffer 起点。
- `encoded_candidate` 的旧真实样例被进一步细分，Error(string) 和 custom error 的
  marker 不再只靠手写最小 IR 覆盖。

复杂度评价：

- 实现效果：8/10。测试输入来自真实 Solidity codegen，且 pass 支持了真实 selector
  和 memory base 形状；还没有接动态 Solidity 编译进 CTest。
- 理解成本：6/10。runner 多了一点轻量表达式跟踪，但和 pass 的 matcher 规则保持一致。
- 后期维护成本：6/10。保留 `.sol` 源码后 case 来源清楚；如果以后固定本地 solc，
  可以再把生成链路脚本化。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 89.21s。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`
  通过，耗时 85.93s。

性能：

- 本次仍只影响 EVM Solidity matcher 和测试，不涉及类型恢复、结构体合并、
  pointer analysis 或主类型恢复 pipeline。`notdec.evm.solidity_patterns` 同口径从
  87.06s 变为 89.21s，`notdec.evm.solidity_rewrite` 从 84.76s 变为 85.93s。

## 第五步实现记录（2026-05-29）

本次继续推进 Error(string) / custom error payload 的第一层支持。范围保持保守：
只记录能从同一 basic block 和 revert 长度里直接确认的信息，不恢复错误名、
参数类型，也不解析字符串内容。

修改内容：

- `src/Passes/evm/SolidityPatterns.cpp:87` 扩展 `SolidityRevertMatch`，新增
  `CustomErrorArgCount` 和 `ErrorStringLength`。
- `src/Passes/evm/SolidityPatterns.cpp:1101` 新增 `getLengthFromBase()`，支持
  常量长度，以及 Solidity 常见的 `end - freeMemoryPointer`。
- `src/Passes/evm/SolidityPatterns.cpp:1184` 的 `matchSolidityRevert()` 在 selector 型
  revert 上记录 revert 总长度。
- `src/Passes/evm/SolidityPatterns.cpp:1228` 附近记录 `base + 36` 位置的常量
  string length，用于 Error(string)。
- `src/Passes/evm/SolidityPatterns.cpp:1248` 附近对 custom error 用
  `(revert_length - 4) / 32` 计算静态 ABI word 个数。
- `src/Passes/evm/SolidityPatterns.cpp:1346` 附近把结果写入
  `notdec.solidity_revert.custom_error_arg_count` 和
  `notdec.solidity_revert.error_string_length` metadata。
- `test/run_evm_solidity_patterns_suite.py:121` 增加 metadata 字符串值计数 helper。
- `test/run_evm_solidity_patterns_suite.py:493`、`:558` 增加
  `expected_custom_error_arg_counts` 和 `expected_error_string_lengths` oracle。
- `test/evm/solidity-patterns/manifest.json:55` 起为 10 个 Solidity-generated case
  增加 payload oracle：
  - Error(string) 长度：`5`、`42`、`18`、`18`
  - custom error 参数个数：`0`、`1`、`2`、`2`、`1`、`1`

效果：

- Error(string) 已能记录 ABI string length。
- custom error 已能记录静态 ABI word 个数。
- 这些信息先作为 metadata 和测试 oracle 暴露，rewrite marker 暂时保持 selector 参数，
  避免过早改对外 marker 形状。

复杂度评价：

- 实现效果：7/10。覆盖当前 Solidity-generated case 的 payload 外壳，但还没有
  字符串内容、错误名和参数类型。
- 理解成本：5/10。只增加两个字段和一个长度 helper，仍沿用 `SolidityRevertMatch`。
- 后期维护成本：5/10。后续可以直接在同一 matcher 上继续扩字符串内容或参数类型，
  不需要再加新 pass。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 89.30s。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`
  通过，耗时 86.29s。

性能：

- 本次仍只影响 EVM Solidity matcher 和测试，不涉及类型恢复、结构体合并、
  pointer analysis 或主类型恢复 pipeline。`notdec.evm.solidity_patterns` 同口径从
  89.21s 变为 89.30s，`notdec.evm.solidity_rewrite` 从 85.93s 变为 86.29s。

## 第六步实现记录（2026-05-29）

本次继续做 Error(string) 的常量字符串内容提取。范围仍然保守：只支持同一 basic
block 内 `mstore(base + 68 + 32*n, word)` 的常量 ABI word，并且只写入不需要转义的
可打印 ASCII 字符串。遇到非 ASCII、控制字符、双引号或反斜杠时不写 literal
metadata。

修改内容：

- `src/Passes/evm/SolidityPatterns.cpp:88` 扩展 `SolidityRevertMatch`，新增
  `ErrorStringLiteral`。
- `src/Passes/evm/SolidityPatterns.cpp:101` 新增 `RevertStringWord`，记录 ABI string
  data word 的序号和 32 字节内容。
- `src/Passes/evm/SolidityPatterns.cpp:1123` 新增 `getAbiWordBytes()`，支持常量 word
  和 `evm_shl(shift, constant)` 两种 Solidity 常见写法。
- `src/Passes/evm/SolidityPatterns.cpp:1177` 新增 `buildAsciiStringLiteral()`，按
  Error(string) length 拼接 ABI word，并做 ASCII / 转义字符过滤。
- `src/Passes/evm/SolidityPatterns.cpp:1253` 的 `matchSolidityRevert()` 收集 string
  data word；`:1312` 只在 offset 可确认时收集，避免真实大样例里空 `optional`
  解引用；`:1325` 在 Error(string) 上生成 literal。
- `src/Passes/evm/SolidityPatterns.cpp:1441` 写入
  `notdec.solidity_revert.error_string_literal` metadata。
- `test/run_evm_solidity_patterns_suite.py:501`、`:576` 增加
  `expected_error_string_literals` oracle。
- `test/evm/solidity-patterns/manifest.json:58` 起为 4 个 Solidity-generated
  Error(string) case 增加 literal oracle：
  - `short`
  - `longer revert message for solidity codegen`
  - `value must be zero`
  - `calldata too short`

实现时发现并修正了一个问题：旧真实样例里有些 `mstore` 的 offset 不能相对
`revert` 起点算出常量。最初代码直接解引用 `Offset`，导致
`0679_19497465_...`、`0651_19497235_...` 等 case 在 notdec 阶段 abort。已改为
只有 `Offset.has_value()` 时才尝试收集 string data word。

效果：

- 4 个 Solidity-generated Error(string) case 都能恢复常量字符串内容。
- rewrite marker 仍保持 selector 参数；字符串内容先作为 metadata 和 oracle 暴露。
- custom error 本次不继续扩参数值恢复。

复杂度评价：

- 实现效果：7/10。覆盖短字符串和跨两个 ABI word 的常量字符串；非 ASCII 和需要转义的
  字符串暂时保守跳过。
- 理解成本：6/10。新增了 ABI word 到 bytes 的局部 helper，但仍集中在
  `SolidityRevertPass` matcher 内。
- 后期维护成本：6/10。后续如果要支持转义或 bytes literal，可以复用当前 word 收集逻辑。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 90.06s。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`
  通过，耗时 86.10s。

性能：

- 本次仍只影响 EVM Solidity matcher 和测试，不涉及类型恢复、结构体合并、
  pointer analysis 或主类型恢复 pipeline。`notdec.evm.solidity_patterns` 同口径从
  89.30s 变为 90.06s，`notdec.evm.solidity_rewrite` 从 86.29s 变为 86.10s。

## 第七步实现记录（2026-05-29）

本次整理 selector 型 revert 的 rewrite marker 形态，把已恢复的 payload 形状放进
marker 参数里。metadata 继续保留为 debug 和 oracle 辅助；marker 作为后续 rewrite /
lowering 更稳定的接口。

修改内容：

- `src/Passes/evm/SolidityPatterns.cpp:1392` 修改
  `insertSelectorRewriteMarker()`，marker function type 从 1 个 `i256` 参数扩成
  2 个 `i256` 参数。
- `src/Passes/evm/SolidityPatterns.cpp:1415` 对未知 payload 使用
  `uint64_t::max()` 作为 sentinel。当前新增 10 个 Solidity-generated case 都能拿到
  具体 payload，不会用到 sentinel。
- `src/Passes/evm/SolidityPatterns.cpp:1853` 在 Error(string) marker 中传
  `(selector, string_length)`。
- `src/Passes/evm/SolidityPatterns.cpp:1857` 在 custom error marker 中传
  `(selector, static_arg_word_count)`。
- `test/run_evm_solidity_patterns_suite.py:162` 增加 marker 参数对计数 helper。
- `test/run_evm_solidity_patterns_suite.py:518`、`:604` 增加
  `expected_error_string_marker_payloads` 和
  `expected_custom_error_marker_payloads` oracle。
- `test/evm/solidity-patterns/manifest.json:61` 起为 10 个 Solidity-generated case
  增加 marker 参数 oracle：
  - Error(string): `147028384:5`、`147028384:42`、`147028384:18`
  - custom error: selector + `0/1/2` 静态 ABI word 个数

效果：

- Error(string) / custom error 的 selector 和 payload 外壳不再只藏在 metadata。
- 后续 consumer 可以直接读 marker 参数；字符串 literal 仍保留在 metadata，因为 marker
  不适合直接传字符串。
- 旧真实样例仍只检查 marker 数量，新 10 个 Solidity-generated case 额外检查 marker
  参数对。

复杂度评价：

- 实现效果：8/10。marker 已表达 selector 和第一层 payload 形状，但仍未恢复 custom
  error 参数类型和值。
- 理解成本：5/10。marker 参数语义更清楚；测试多了一个参数对计数 helper。
- 后期维护成本：5/10。后续扩展时优先改 marker 参数和 metadata，不需要新 pass。

验证：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py`
- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  通过，耗时 90.29s。
- `ctest --test-dir build -R notdec.evm.solidity_rewrite --output-on-failure`
  通过，耗时 86.85s。

性能：

- 本次仍只影响 EVM Solidity matcher 和测试，不涉及类型恢复、结构体合并、
  pointer analysis 或主类型恢复 pipeline。`notdec.evm.solidity_patterns` 同口径从
  90.06s 变为 90.29s，`notdec.evm.solidity_rewrite` 从 86.10s 变为 86.85s。

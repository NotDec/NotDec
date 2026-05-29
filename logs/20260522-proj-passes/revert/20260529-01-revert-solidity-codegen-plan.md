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

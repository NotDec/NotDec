# Revert pass consumes memory markers

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

让 `SolidityRevertPass` 先读取 memory rewrite marker，再退回旧的 raw EVM call 匹配。

PLAN 已经明确：只有 marker 但后续 pass 仍完全绕过 marker、继续猜原始 memory 形状，不算完成。本轮先接最稳的消费者：returndata bubble revert。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- forwarding revert helper：
  - `let pos := allocate_unbounded()`
  - `returndatacopy(pos, 0, returndatasize())`
  - `revert(pos, returndatasize())`

`/sn640/solidity/libsolidity/codegen/ContractCompiler.cpp`：

- fallback re-throw：
  - `returndatacopy(0, 0, returndatasize())`
  - `revert(0, returndatasize())`

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`：

- external call 失败且不是 try-call 时，走 `<forwardingRevert>()`。
- dynamic returndata 成功路径也会 `returndatacopy(pos, 0, size)`，但没有立即 `revert(pos, size)`，不能当 bubble。

判断：returndata bubble 的稳定语义是 copy marker 和 revert consumer marker 同时存在：

- `notdec_solidity_memory_copy_write(base, 0, 0, size, scratch_returndata)`
- `notdec_solidity_memory_consumer(base, size, revert)`

## 计划

1. 把 `MemoryBufferRewritePass` 提前到 `SolidityRevertPass` 前面。
2. `SolidityRevertPass` 的 returndata bubble matcher 先找 memory marker：
   - copy kind = 4。
   - consumer kind = 2。
   - base 相同。
   - size 是同一个值，或两边都是 `evm_returndatasize()`。
3. 如果 marker 不完整，再走旧的 raw `evm_returndatacopy` + `evm_revert` 匹配。
4. 不改现有 rewrite marker 名字和 manifest oracle。

## 风险

- `returndatasize()` 可能出现两次不同 SSA value。marker matcher 不能要求 Value 指针相同，要接受“两边都是 returndatasize”。
- pass 顺序提前 memory marker 后，后续 pattern pass 会看到更多 marker call。它们按 callee 名过滤，预期不影响。
- 这轮只迁一个消费者路径，不代表全部 ABI / event / external-call 都已迁完。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.66s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll -o /tmp/notdec-memory-marker-revert-0011.ll --tr-level=0`。
  - 输出中 `notdec_solidity_memory_copy_write(..., kind 4)` 带上了 `!notdec.solidity_revert.returndata_copy`，说明 RevertPass 读到了 memory copy marker。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-revert-marker-consumer`。
  - 100/100 跑完，总耗时 83.38s。
  - memory marker 数量保持不变：allocation 1041、word write 8902、byte write 7、copy write 783、consumer 5736。
  - `notdec_solidity_rewrite_revert_returndata_bubble`：493。
  - 带 `notdec.solidity_revert.returndata_copy` 的 memory copy marker：493。

## 实现记录

- `src/Passes/PassManager.cpp:290` 把 `MemoryBufferRewritePass` 提前到 `SolidityRevertPass` 前。
- `src/Passes/evm/SolidityPatterns.cpp:1281` 增加 `isSameRevertBufferSize()`，允许两边是不同 SSA value 的 `evm_returndatasize()`。
- `src/Passes/evm/SolidityPatterns.cpp:1464` 增加 `findReturndataBubbleCopyFromMemoryMarkers()`。
- `src/Passes/evm/SolidityPatterns.cpp:1482` 匹配 `notdec_solidity_memory_consumer(..., revert)`。
- `src/Passes/evm/SolidityPatterns.cpp:1492` 匹配 `notdec_solidity_memory_copy_write(..., scratch_returndata)`。
- `src/Passes/evm/SolidityPatterns.cpp:1527` 在旧 raw `evm_returndatacopy` matcher 前优先读取 memory marker。

## 当前限制

- 只迁了 returndata bubble revert。Error(string)、custom error、panic 仍主要读原始 `mstore` 形状。
- 只在同一 basic block 里配对 copy marker 和 consumer marker。
- marker size 现在只特殊处理 `returndatasize()`；其他等价 size 表达式还没归一。

## 评分

- 实现效果：6/10。已经有一个后续消费者优先读取 memory rewrite marker，满足 PLAN 的关键要求；但覆盖范围还只限 returndata bubble。
- 理解成本：4/10。pass 顺序改变需要注意，但 fallback 到旧 matcher，行为风险可控。
- 维护成本：5/10。后续还要把 revert selector / ABI return / event / external call 逐步迁到 marker。

## 下一轮候选

1. 让 `SolidityRevertPass` 的 Error(string) / custom error 读取 `memory_write` marker。
2. 让 `AbiReturnPass` 读取 memory consumer/write marker，先覆盖 return(base, 32)。
3. 给 helper-return base 做 summary，减少 unknown-base fallback。
4. 支持 `base + symbolic` offset，但要先限定在 ABI helper 内。

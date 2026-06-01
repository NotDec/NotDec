# Memory buffer rewrite surface 第一版

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

先实现第一块可验证功能：在 EVM Solidity pipeline 中新增 memory buffer rewrite surface。

本轮只处理确定性高的形状：

- `base = evm_mload(mem, 64)`。
- `evm_mstore(mem, 64, base + size)` 作为 finalize。
- `evm_mstore(mem, base + const, value)` / `evm_mstore(mem, base, value)` 作为 word write。
- `evm_return(mem, base, size)` 和 `evm_revert(mem, base, size)` 作为 consumer。

本轮不删除原始 `mload/mstore/return/revert`，只插入稳定 rewrite marker call，供后续 ABI / revert / event / external-call pass 消费。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp`：

- `freeMemoryPointer = 64`。
- `fetchFreeMemoryPointer()` 生成 `mload(0x40)`。
- `allocateMemory()` / `allocateMemory(size)` 生成“读 free pointer，加 size，写回 `0x40`”。
- `revertWithStringData()` / `revertWithError()` 以 free pointer 为 base 写 selector 和 ABI payload，最后 `revert(start, size)`。

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- `allocate_unbounded()` 只返回 `mload(0x40)`。
- `finalize_allocation(memPtr, size)` 写回 `mstore(0x40, memPtr + roundUp(size))`，失败走 Panic `0x41`。

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp`：

- public entry return 形状是 `memPos := allocate_unbounded()`，`memEnd := abiEncode(memPos, returns...)`，`return(memPos, sub(memEnd, memPos))`。

判断：memory pass 的第一版应围绕 base/finalize/write/consumer 建模，并插入 IR marker。只写 metadata 不够。

## 计划

1. 新增 `MemoryBufferAnalysis`，提取 allocation、write、consumer 事实。
2. 新增 `MemoryBufferRewritePass`，根据分析结果插入：
   - `notdec_solidity_memory_allocation(base, size)`
   - `notdec_solidity_memory_write(base, offset, value)`
   - `notdec_solidity_memory_consumer(base, size, kind)`
3. 接入 EVM pipeline，放在 checked-bounds 之后，避免影响现有 guard / revert matcher。
4. 给 pattern runner 增加 memory marker oracle，并先选一个现有 Solidity-generated case 验证。

## 风险

- marker 插入过早可能影响 checked-bounds / revert matcher，所以本轮放在 checked-bounds 后。
- 只支持常量 offset，不处理 PHI、循环和复杂 symbolic offset。
- external call input/output 复用先不处理，下一轮再做。

## 下一轮候选

1. 接 external call input/output consumer，处理同 base 多 role。
2. 接 event log data buffer。
3. 把 revert Error/custom error 的 selector/payload 匹配迁到 memory analysis。
4. 处理 `base + roundUp(size)` 的 allocation size 归一。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-memory-revert-error.ll --tr-level=0`：输出 4 个 `notdec_solidity_memory_write`，1 个 revert consumer。
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll -o /tmp/notdec-memory-multi.ll --tr-level=0`：输出 return/revert consumer marker，修正后无 verifier dominance 报错。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:18` 定义 memory write / consumer kind；`MemoryAllocation`、`MemoryWrite`、`MemoryConsumer`、`MemoryBufferFacts` 放到 `notdec::passes::evm`。
- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:56` 暴露 `analyzeMemoryBuffers()`；`include/notdec/Passes/evm/MemoryBufferAnalysis.h:58` 暴露 `MemoryBufferRewritePass`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:36` 识别 `evm_mload(mem, 64)` 作为 free memory pointer base。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:43` 只接受 `base` 或 `base + const` 的 offset，避免 symbolic offset 误判。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:98`、`src/Passes/evm/MemoryBufferAnalysis.cpp:113`、`src/Passes/evm/MemoryBufferAnalysis.cpp:129` 分别插入 allocation / write / consumer marker call。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:147` 实现 `analyzeMemoryBuffers()`，提取 finalize allocation、word write、return/revert consumer。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:228` 实现 `MemoryBufferRewritePass::run()`，只要插入 marker 就返回 `PreservedAnalyses::none()`。
- `src/CMakeLists.txt:10` 加入 `Passes/evm/MemoryBufferAnalysis.cpp`。
- `src/Passes/PassManager.cpp:44` include 新 pass；`src/Passes/PassManager.cpp:292` 把 `MemoryBufferRewritePass` 放在 `SolidityRevertPass` / `CheckedBoundsPass` 之后、storage/event/external 之前。
- `test/run_evm_solidity_patterns_suite.py:296` 增加 memory consumer kind 计数；`test/run_evm_solidity_patterns_suite.py:712` 和 `test/run_evm_solidity_patterns_suite.py:849` 接入 expected / actual oracle。
- `test/evm/solidity-patterns/manifest.json:45` 给 `checked_bounds_arithmetic_01` 增加 return buffer oracle；`test/evm/solidity-patterns/manifest.json:444` 给 `revert_error_string_01` 增加 revert buffer oracle。

## 当前限制

- 本轮 rewrite 还是 marker surface：已经是 IR rewrite，不只是 metadata，但还没有删除原始 `evm_mstore` / `evm_return` / `evm_revert`。
- allocation marker 只覆盖直接 `mstore(0x40, base + size)` finalize。很多 Solidity return/revert 样例使用 `allocate_unbounded()`，不会立即 finalize，所以本轮 oracle 先覆盖 write 和 consumer。
- base 判断现在用同一个 SSA value，不把等价的 `mload(0x40)` 混用。这样更保守，避免 marker 参数不支配插入点。
- external call input/output、event data、复杂 symbolic offset、PHI/循环里的 buffer 暂时不处理。

## 评分

- 实现效果：6/10。已能在真实 Solidity-generated IR 上插入稳定 write/consumer marker，但还不是最终 lowering。
- 理解成本：6/10。新增了独立文件和显式 facts，比继续塞进大文件清楚；但 marker 只是中间形态，需要后续 pass 接着消费。
- 维护成本：5/10。当前规则保守，误判风险低；后续要扩展 external call / event / allocation 归一时，需要补更多 oracle。

## 后续更好的方向

下一步不要只扩大 marker 数量。应优先让 ABI return / revert / external call / event 这些 pass 从 memory facts 读取 buffer 结构，逐步减少各自重复解析 `mstore` 序列。

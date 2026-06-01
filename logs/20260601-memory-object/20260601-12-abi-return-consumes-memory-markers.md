# ABI return pass consumes memory markers

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

让 `AbiReturnPass` 先读取 memory consumer marker，再退回旧的 `evm_return` size 判断。

上一轮已经让 revert bubble 读取 memory marker。本轮继续迁一个消费者，覆盖 ABI return：

- `notdec_solidity_memory_consumer(base, size, return)`

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp`：

- public external wrapper：
  - `let memPos := allocateUnbounded()`
  - `let memEnd := abiEncode(memPos, returns...)`
  - `return(memPos, sub(memEnd, memPos))`
- deploy code：
  - `codecopy(codeOffset, dataoffset("<object>"), datasize("<object>"))`
  - `return(codeOffset, datasize("<object>"))`
- fallback(bytes calldata) returns (bytes memory)：
  - `retval := fallback(0, calldatasize())`
  - `return(add(retval, 0x20), mload(retval))`

判断：ABI return 的稳定入口是 `return(base, size)`。第一步不急着完整恢复 ABI payload，先让 pass 读 memory consumer marker，避免继续只从 raw `evm_return` 猜。

## 计划

1. 把 `MemoryBufferRewritePass` 提前到 `AbiReturnPass` 前。
2. `AbiReturnPass` 在同一 block 内找 `notdec_solidity_memory_consumer(..., kind return)`。
3. marker 的 base / size 与 `evm_return` 参数一致时，优先用 marker 判定 return kind。
4. 如果 marker 不存在，保留旧逻辑：
   - size = 32 -> `static_1_word`
   - size = `returndatasize()` -> `returndata_forward`
   - 其他 -> `candidate`
5. 给被消费的 marker 加 `notdec.solidity_abi_return.consumer` metadata，作为测试和 audit 证据。

## 风险

- 当前只在同一 basic block 内找 consumer marker。
- `return(add(retval, 0x20), mload(retval))` 这类 fallback return 还不能绑定到 allocation base，但 marker 仍可作为 fallback base 记录。
- 这轮只迁 consumer 判断，不恢复 ABI return payload。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.68s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll -o /tmp/notdec-memory-marker-abi-return-0011.ll --tr-level=0`。
  - 输出 1 个 `notdec_solidity_abi_return_memory_consumer`，说明 `AbiReturnPass` 读到了 return consumer marker。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-abi-return-marker-consumer`。
  - 100/100 跑完，总耗时 83.50s。
  - `notdec_solidity_abi_return_memory_consumer`：1420。
  - return consumer marker：1420。
  - memory marker 总数保持不变：allocation 1041、word write 8902、byte write 7、copy write 783、consumer 5736。

## 实现记录

- `src/Passes/PassManager.cpp:289` 把 `MemoryBufferRewritePass` 提前到 `AbiReturnPass` 前。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:18` 增加 `classifyAbiReturnSize()`，复用原来的 return size 分类。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:38` 增加 `findReturnConsumerMarker()`，在同一 block 内查找 return consumer marker。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:58` 增加 `insertAbiReturnMemoryConsumerMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:86` 优先使用 memory consumer marker 判定 ABI return kind。
- `test/evm/solidity-patterns/manifest.json:981` 给 `0011_multi_public` 增加 `notdec_solidity_abi_return_memory_consumer = 1` oracle。

## 当前限制

- 只在同一 basic block 内找 marker。
- base / size 现在要求同一个 SSA value，暂不做等价表达式归一。
- marker 只表达 ABI return 已经消费 memory consumer，还没有恢复 return payload。

## 评分

- 实现效果：7/10。固定 100-case 里 1420 个 return consumer 都被 AbiReturn 接上，说明这条 consumer 迁移覆盖面大。
- 理解成本：4/10。新增 marker 很直接，但 pass 顺序又提前了一步，需要注意后续 pass 都会看到 memory marker。
- 维护成本：5/10。后续要继续把 return payload 解析迁到 memory write marker。

## 下一轮候选

1. 让 `SolidityRevertPass` 的 Error(string) / custom error 读取 `memory_write` marker。
2. 让 `AbiReturnPass` 读取 `memory_write` marker，先恢复 `return(base, 32)` 的 word payload。
3. 给 helper-return base 做 summary，减少 unknown-base fallback。
4. 支持 `base + symbolic` offset，但要先限定在 ABI helper 内。

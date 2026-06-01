# External call input word write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮 `ExternalCallPass` 已经能读取 external input 的 copy write。固定 100 例里这类命中只有 4 个，主要是 proxy forwarding 的 `calldatacopy`。

本轮接更常见的 ordinary external call input word write：

- 在 external call 前查找 `notdec_solidity_memory_write(base, 0, value)`。
- 要求 word write 和 external call 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_external_call_input_word_write(base, offset, value, callKind)`。

这个 marker 证明 external call pass 已经读取 input buffer 的 word payload。它不只说明“有 input buffer”，也不只是 metadata。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()` 的普通 external call 生成：

- `let pos := allocate_unbounded()`
- `mstore(pos, shl28(funSel))`
- `let end := encodeArgs(add(pos, 4), args...)`
- `call(gas, address, value, pos, sub(end, pos), pos, staticReturndataSize)`

这里 offset 0 的 word write 是 selector 写入。后续 ABI 参数从 `pos + 4` 开始编码。

判断：`notdec_solidity_memory_write(base, 0, value)` 对 external input 来说是明确的 payload write，应该被 `ExternalCallPass` 消费。第一版不尝试判断 `value` 是否一定是 selector，只证明 external input 读到了 offset 0 word write。

## 实现记录

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:21` 增加 `NumExternalCallInputWordWrites` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:141` 增加 `findExternalCallInputWordWriteMarker()`，查找同 block 内 external call 前的 offset 0 word write marker。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:217` 增加 `insertExternalCallInputWordWriteMarker()`，插入 `notdec_solidity_external_call_input_word_write(base, offset, value, callKind)`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:266` 在处理 external input 时读取 word write marker。
- `test/evm/solidity-patterns/manifest.json:1171` 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_external_call_input_word_write = 6` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，100.81s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-external-input-word`。
  - 100/100 成功。
  - `notdec_solidity_external_call_input_word_write`：308。
  - 其中 `call` 90，`staticcall` 218，`delegatecall` 0，`callcode` 0。

## 当前限制

- 只接 offset 0 word write。
- 只在同一 basic block 内查找，不跨块追踪。
- marker 不区分 selector 和其他 offset 0 word payload；目前只证明 external input 读到了 word write。
- ABI 参数 head/tail 还没恢复，`pos + 4` 后面的写入后续再接。

## 下一轮候选

1. 接 external input 的 offset 4 / 36 / 68 等 ABI head word write，开始恢复参数 head。
2. 把 free pointer reload 等价下沉到 `MemoryBufferAnalysis`，避免 event / external / return 各自重复实现。
3. 审计 ABI return payload 的 write marker，找能稳定接入的 return word/copy proof。

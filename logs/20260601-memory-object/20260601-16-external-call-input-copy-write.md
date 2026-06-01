# External call input copy write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮 `ExternalCallPass` 已经能读 external input / output 的 memory consumer marker，但还没有读 input buffer 的 payload write。

本轮接一个保守的 payload proof：

- 在 external call 前查找 `notdec_solidity_memory_copy_write(base, 0, sourceOffset, copySize, copyKind)`。
- 要求 copy marker 和 external call 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_external_call_input_copy_write(base, sourceOffset, copySize, copyKind, callKind)`。

这个 marker 证明 external call input payload 不是只知道“有一个 buffer”，而是已经读到了 memory copy write。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()` 的普通外部函数调用会：

- `let pos := allocate_unbounded()`
- `mstore(pos, shl28(funSel))`
- `let end := encodeArgs(add(pos, 4), args...)`
- `call(..., pos, sub(end, pos), pos, staticReturndataSize)`

`appendBareCall()` 的 bare call / delegatecall / staticcall 对 bytes memory 参数会直接使用：

- `let pos := add(arg, 0x20)`
- `let length := mload(arg)`
- `call(..., pos, length, 0, 0)`

真实 proxy / fallback forwarding 还常见：

- `calldatacopy(pos, 0, calldatasize())`
- `delegatecall(gas(), target, pos, size, pos, 0)`

判断：`calldatacopy` / `returndatacopy` / `codecopy` / `mcopy` 写入 external input buffer 是明确的 payload write，应该被 external call pass 消费。第一版只接 offset 0 的同 block copy write。

## 实现记录

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:19` 增加 `NumExternalCallInputCopyWrites` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:50` 增加 `isFreeMemoryPointerLoad()`，识别 `evm_mload(mem, 64)`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:57` 增加 `isFreeMemoryPointerStore()`，遇到 `evm_mstore(mem, 64, ...)` 时切断 free pointer reload 等价。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:98` 增加 `findExternalCallInputCopyWriteMarker()`，查找同 block 内 external call 前的 offset 0 copy write marker。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:155` 增加 `insertExternalCallInputCopyWriteMarker()`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:195` 在 external input consumer 后继续读取 input copy write marker。
- `test/evm/solidity-patterns/manifest.json:922` 给 `0014_proxy_like` 增加 `notdec_solidity_external_call_input_copy_write = 1` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- smoke：`0014_proxy_like` 输出 1 个 `notdec_solidity_external_call_input_copy_write`，对应 calldata copy 到 delegatecall input。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，100.51s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-external-input-copy`。
  - 100/100 成功。
  - `notdec_solidity_external_call_input_copy_write`：4。
  - 4 个全部是 calldata copy input。

## 当前限制

- 只接 offset 0 的 copy write。
- 只在同一 basic block 内查找，不跨块追踪。
- free pointer reload 等价只在没有 `mstore(0x40, ...)` 分隔时使用。
- 还没有读取 ordinary external function call 的 selector `memory_write(base, 0, shl28(selector))`；100-case 里这个形状较少，后续再接。

## 下一轮候选

1. 审计并接 external call input 的 selector `memory_write(base, 0, value)`，覆盖普通 external function call。
2. 改进 free pointer reload 等价，把这类逻辑下沉到 `MemoryBufferAnalysis`，避免各 pass 重复写。
3. 审计 ABI return / event payload 的 write marker 覆盖，挑高频且 Solidity codegen 明确的 shape 接入。

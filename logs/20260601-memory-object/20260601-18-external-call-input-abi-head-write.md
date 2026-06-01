# External call input ABI head write

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

上一轮 external call input 已经读取 offset 0 word write，通常对应 selector。普通 external call 的参数 ABI head 写在 `pos + 4` 开始，offset 形如 `4 + 32 * n`。

本轮接这类 ABI head word write：

- 在 external call 前查找 `notdec_solidity_memory_write(base, offset, value)`。
- 只接受 `offset >= 4` 且 `offset % 32 == 4`。
- 要求 word write 和 external call 在同一 basic block。
- base 直接相同，或二者都是没有被 `mstore(0x40, ...)` 分隔的 free pointer reload。
- 命中后插入 `notdec_solidity_external_call_input_abi_head_write(base, offset, value, callKind)`。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp::appendExternalFunctionCall()` 生成：

- `let pos := allocate_unbounded()`
- `mstore(pos, shl28(funSel))`
- `let end := encodeArgs(add(pos, 4), args...)`
- `call(gas, address, value, pos, sub(end, pos), pos, staticReturndataSize)`

`encodeArgs(add(pos, 4), ...)` 说明参数 ABI head 从 offset 4 开始，每个 head word 间隔 32 字节。这里的 writes 是 compiler-generated external input payload，不是业务分支。

## 实现记录

- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:3` 增加 `SmallVector` include。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:23` 增加 `NumExternalCallInputAbiHeadWrites` 统计。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:64` 增加 `getUInt64Constant()`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:72` 增加 `isExternalAbiHeadOffset()`，只接受 `4 + 32 * n`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:182` 增加 `collectExternalCallInputAbiHeadWriteMarkers()`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:268` 增加 `insertExternalCallInputAbiHeadWriteMarker()`。
- `src/Passes/evm/solidity-patterns/ExternalCallPass.cpp:329` 在 external input 处理中插入 ABI head proof marker。
- `test/evm/solidity-patterns/manifest.json:1171` 给 `0448_19495059_065877b669_4f138305be23` 增加 `notdec_solidity_external_call_input_abi_head_write = 10` oracle。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，101.14s。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-external-input-abi-head`。
  - 100/100 成功。
  - `notdec_solidity_external_call_input_abi_head_write`：290。
  - offset 分布：4 有 152 个，36 有 72 个，68 有 17 个，100 有 18 个，132 有 17 个，164 有 14 个。

## 当前限制

- 只接常量 offset 的 ABI head word。
- 只在同一 basic block 内查找，不跨块追踪。
- 不解析动态 tail，也不判断参数类型。
- free pointer reload 等价仍在 `ExternalCallPass` 内部实现，后续应该下沉到 `MemoryBufferAnalysis`。

## 下一轮候选

1. 把 free pointer reload 等价和 marker 查找逻辑下沉到 `MemoryBufferAnalysis`，减少 pass 间重复。
2. 审计 event / ABI return payload 的 ABI head write，挑稳定形状接入。
3. 对 external call input 的动态 tail / copy write 继续做更细的 proof marker。

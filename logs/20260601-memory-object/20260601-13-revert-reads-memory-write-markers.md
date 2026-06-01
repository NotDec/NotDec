# Revert pass reads memory write markers

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

让 `SolidityRevertPass` 的 selector / Error(string) 解析读取 `notdec_solidity_memory_write`，而不是只读 raw `evm_mstore`。

上一轮已经让 returndata bubble 读取 memory copy / consumer marker。本轮继续迁 revert encoding 的 word write：

- `notdec_solidity_memory_write(base, offset, value)`

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- `revertWithError(signature, args...)`：
  - `pos := allocateUnbounded()` 或 `pos := 0`
  - `mstore(pos, selector)`
  - `encode(add(pos, 4), args...)`
  - `revert(pos, sub(end, pos))`
- `revertReasonIfDebugBody()`：
  - `start := allocate`
  - `mstore(start, Error(string))`
  - `mstore(start + 4, 0x20)`
  - `mstore(start + 36, length)`
  - `mstore(start + 68 + i*32, word)`
  - `revert(start, overallLength)`

判断：这些 mstore 是 compiler-generated revert buffer writes。Memory pass 已经把 `base + const` 的 word write 转成 `notdec_solidity_memory_write`，RevertPass 应该优先读这个 rewrite surface。

## 计划

1. 在 `matchSolidityRevert()` 的 mstore 扫描中同时接受 `notdec_solidity_memory_write`。
2. marker 形状：
   - arg0 = base
   - arg1 = constant offset
   - arg2 = value
3. raw `evm_mstore` fallback 保留。
4. 如果本次匹配用到了 memory write marker，插入证明 marker：
   - `notdec_solidity_revert_memory_write_match(selector, kindCode)`
5. 用已有 Error(string) case 增加 oracle。

## 风险

- marker 和 raw `evm_mstore` 会同时存在。匹配时允许重复看到同一 offset，但只要 selector / length / literal 一致，不影响结果。
- 只支持 constant offset marker，不处理 symbolic offset。
- absolute scratch `mstore(4, panicCode)` 仍主要走 raw mstore，因为 memory pass 不把所有 scratch mstore 都当 allocation write。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.54s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/revert_error_string_01.ll -o /tmp/notdec-revert-memory-write-01.ll --tr-level=0`。
  - 输出 4 个 `notdec_solidity_memory_write` 和 1 个 `notdec_solidity_revert_memory_write_match`。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-revert-memory-write`。
  - 100/100 跑完，总耗时 83.51s。
  - `notdec_solidity_revert_memory_write_match`：1123。
  - `notdec_solidity_rewrite_revert_error_string`：1059。
  - `notdec_solidity_rewrite_revert_custom_error`：146。
  - memory marker 总数保持不变：allocation 1041、word write 8902、byte write 7、copy write 783、consumer 5736。

## 实现记录

- `include/notdec/Passes/evm/SolidityPatternUtils.h:40` 在 `SolidityRevertMatch` 增加 `UsedMemoryWriteMarker`。
- `include/notdec/Passes/evm/SolidityPatternUtils.h:128` 声明 `insertRevertMemoryWriteMatchMarker()`。
- `src/Passes/evm/SolidityPatterns.cpp:69` 增加 `RevertMemoryWrite`，统一 raw `evm_mstore` 和 memory write marker。
- `src/Passes/evm/SolidityPatterns.cpp:1516` 增加 `getRevertMemoryWrite()`。
- `src/Passes/evm/SolidityPatterns.cpp:1578` 在 `matchSolidityRevert()` 中同时读取 raw mstore 和 `notdec_solidity_memory_write`。
- `src/Passes/evm/SolidityPatterns.cpp:1583` 记录本次匹配是否用到 memory write marker。
- `src/Passes/evm/SolidityPatterns.cpp:1720` 增加 `notdec_solidity_revert_memory_write_match(selector, kindCode)`。
- `src/Passes/evm/solidity-patterns/SolidityRevertPass.cpp:34` 在匹配成功后插入该证明 marker。
- `test/evm/solidity-patterns/manifest.json:445` 给 `revert_error_string_01` 增加 marker oracle。

## 当前限制

- marker 只处理 constant offset。
- scratch absolute `mstore(4, panicCode)` 仍主要靠 raw mstore，因为 memory pass 没把所有 scratch mstore 标成 allocation write。
- marker 和 raw mstore 都存在时会重复扫描同一语义，但目前 selector / length / literal 一致，不改变结果。

## 评分

- 实现效果：7/10。固定 100-case 中 1123 个 revert 匹配明确读到了 memory write marker，覆盖 error string 和 custom error 的主体路径。
- 理解成本：5/10。统一 raw mstore 和 marker 后逻辑更集中，但需要注意 marker 与 raw 指令共存。
- 维护成本：5/10。后续可以逐步减少 raw mstore fallback，或者让 memory marker 携带更多 ABI payload 信息。

## 下一轮候选

1. 让 `AbiReturnPass` 读取 `memory_write` marker，先处理能绑定 base 的 return payload。
2. 给 helper-return / helper-argument base 做 summary，覆盖更多 `return(base, 32)` 和 ABI encode helper。
3. 支持 `base + symbolic` offset，但要先限定在 ABI helper 内。
4. 审计 fixed 100-case 中剩余没有进入 memory rewrite surface 的 memory shape。

# Mstore8 memory byte write rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

补齐 `evm_mstore8` 的 byte write marker。

`mstore8` 只写 1 字节，不能复用现有 `notdec_solidity_memory_write(base, offset, value)`，否则后续 pass 会误以为这是 32 字节 word write。本轮新增 marker：

- `notdec_solidity_memory_byte_write(base, offset, value)`

如果地址能匹配 free-memory base 或 `base + const`，就写 allocation-relative marker；否则写 fallback marker：

- `notdec_solidity_memory_byte_write(dst, 0, value)`

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/LValue.cpp`：

- `MemoryItem::storeValue()` 写非 padded、大小为 1 字节的 memory value 时使用 `MSTORE8`。
- fixed bytes 会先取 `byte(0, value)`，然后 `MSTORE8`。

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`：

- memory byte array element 赋值会生成：
  - `mstore8(memoryAddress, byte(0, preparedValue))`

`/sn640/solidity/libsolidity/codegen/ContractCompiler.cpp`：

- library deploy code patching 中会：
  - `codecopy(codepos, subOffset, subSize)`
  - `mstore8(codepos, 0x73)`
  - `return(codepos, subSize)`

判断：这些都是 compiler-generated memory byte writes。它们应该进入 IR rewrite surface，但必须和 word write 分开。

## 真实 IR 形状

NotDec IR 声明：

- `declare void @evm_mstore8(ptr, i256, i256)`

manifest 和固定 100-case 里常见形状：

- `evm_mstore8(ptr %mem, i256 %evm.add..., i256 %evm.byte...)`
- `evm_mstore8(ptr %mem, i256 %evm.sub..., i256 %evm.byte...)`

## 计划

1. `MemoryWriteKind` 增加 `MStore8`。
2. `insertWriteMarker()` 对 `MStore8` 写 `notdec_solidity_memory_byte_write(base, offset, value)`。
3. `analyzeMemoryBuffers()` 识别 `evm_mstore8(ptr mem, i256 dst, i256 value)`。
4. 能匹配 `dst = base` 或 `base + const` 时，写 allocation-relative marker。
5. 匹配不到 base 时，写 fallback marker：base 为实际 `dst`，offset 为 0。
6. manifest 增加一个真实 case oracle，检查 byte write marker 数量。

## 风险

- fallback base 是实际目的地址，不代表 allocation base。
- `base - const` 和复杂 symbolic offset 暂不归到 allocation-relative marker，先按 fallback 记录。
- 这轮只表达 byte write，不尝试合并相邻 byte writes。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.60s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0450_19495071_6b6c9447e0_6344565f4b31.ll -o /tmp/notdec-memory-mstore8-0450.ll --tr-level=0`。
  - 输出 3 个 `notdec_solidity_memory_byte_write`。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-mstore8`。
  - 100/100 跑完，总耗时 83.78s。
  - 新增 `notdec_solidity_memory_byte_write`：7。
  - allocation / word write / copy write / consumer 数量不变：1041 / 8902 / 783 / 5736。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:25` 增加 `MemoryWriteKind::MStore8 = 7`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:184` 对 `MStore8` 插入 `notdec_solidity_memory_byte_write(base, offset, value)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:284` 新增 `evm_mstore8` 分支。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:292` 对 allocation-relative byte write 插入 marker。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:298` 对 unknown-base byte write 插入 fallback marker。
- `test/evm/solidity-patterns/manifest.json:1232` 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 memory rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:1235` 检查 `notdec_solidity_memory_byte_write = 3`。

## 当前限制

- `base - const` 和复杂 symbolic offset 现在走 fallback，不归到 allocation-relative marker。
- marker 不合并相邻 byte writes。
- 后续消费者还没读取 byte write marker。

## 评分

- 实现效果：4/10。100-case 只新增 7 个 marker，但补齐了重要的 byte write surface。
- 理解成本：3/10。新 marker 语义直接，不和 word write 混用。
- 维护成本：4/10。后续消费者需要单独处理 byte write。

## 下一轮候选

1. 让 `SolidityRevertPass` 优先读取 memory copy/consumer marker，先从 scratch returndata bubble 入手，满足 PLAN 里“后续消费者要读 marker”的要求。
2. 给 helper-return base 做 summary，减少 unknown-base fallback。
3. 支持 `base + symbolic` offset，但要先限定在 ABI helper 内。
4. 审计 fixed 100-case 中剩余未进入 rewrite surface 的 memory call。

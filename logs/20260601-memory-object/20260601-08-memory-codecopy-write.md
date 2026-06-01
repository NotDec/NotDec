# Codecopy memory write rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

补齐 `evm_codecopy` 的 memory copy-write marker。

当前已有 `calldatacopy` / `returndatacopy` 的 rewrite surface：

- `notdec_solidity_memory_copy_write(base, offset, src, size, kind)`
- calldata kind = 2
- returndata kind = 3
- scratch returndata kind = 4

`codecopy` 同样是编译器生成的 memory write。它的 source 来自 contract code 区，不是 calldata / returndata，但目的地址仍然是 memory buffer。本轮增加 kind = 5。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp`：

- `IRGenerator::deployCode()` 非 EOF 路径：
  - `let codeOffset := allocateUnbounded()`
  - `codecopy(codeOffset, dataoffset("<object>"), datasize("<object>"))`
  - `return(codeOffset, datasize("<object>"))`

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp`：

- `new Contract(...)` 非 EOF 路径：
  - `memPos := allocateUnbounded()`
  - `memEnd := add(memPos, datasize("<object>"))`
  - `datacopy(memPos, dataoffset("<object>"), datasize("<object>"))`
  - 后面把 constructor 参数 ABI encode 到 `memEnd`。
- `type(C).creationCode/runtimeCode`：
  - 分配 `add(size, 32)`
  - `mstore(result, size)`
  - `datacopy(add(result, 32), dataoffset(...), size)`

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- constructor args decode 非 EOF 路径：
  - `programSize := datasize("<object>")`
  - `argSize := sub(codesize(), programSize)`
  - `memoryDataOffset := allocate(argSize)`
  - `codecopy(memoryDataOffset, programSize, argSize)`

判断：这些都是 Solidity compiler-generated memory writes。目的地址可能是 free-memory base，也可能是 helper 参数或表达式。应复用上一轮 unknown-base copy 策略。

## 计划

1. `MemoryWriteKind` 增加 `CodeCopy = 5`。
2. `analyzeMemoryBuffers()` 把 `evm_codecopy` 纳入 copy-write 分支。
3. 参数位置沿用当前 IR 声明：`evm_codecopy(ptr mem, ptr env, i256 dst, i256 src, i256 size)`。
4. 能匹配 `dst = base` 或 `base + const` 时，写 allocation-relative marker。
5. 匹配不到 base 时，写 fallback marker：`notdec_solidity_memory_copy_write(dst, 0, src, size, 5)`。
6. runner 增加 kind 5 的名字，manifest 增加一个真实 case oracle。

## 风险

- `codecopy` 有两类常见来源：部署/runtime object copy，以及 constructor args 从 code 区读取。后续消费者需要结合 consumer 判断它是 return/create buffer 还是 ABI decode buffer。
- fallback marker 的 base 仍然只是实际 dst，不代表 allocation base。
- Yul `datacopy` 在 IR 里会落成 `evm_codecopy`，本轮不需要单独处理 `datacopy` 名字。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.12s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0448_19495059_065877b669_4f138305be23.ll -o /tmp/notdec-memory-codecopy-0448.ll --tr-level=0`。
  - 输出 10 个 copy write，其中 `code` kind 1 个。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-codecopy`。
  - 100/100 跑完，总耗时 83.43s。
  - `notdec_solidity_memory_copy_write` 从 723 增加到 767。
  - code copy 从 0 增加到 44。
  - calldata / returndata / scratch returndata 数量不变：151 / 75 / 497。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:23` 增加 `MemoryWriteKind::CodeCopy = 5`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:45` 增加 `isMemoryCopyWriteCall()`，把 `evm_codecopy` 纳入 copy-write call。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:51` 增加 `getMemoryCopyWriteKind()`，统一返回 calldata / returndata / code kind。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:275` 复用现有 copy-write 分支处理 `evm_codecopy`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:291` 对 allocation-relative `codecopy` 插入 kind 5 marker。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:299` 对 unknown-base `codecopy` 插入 fallback marker。
- `test/run_evm_solidity_patterns_suite.py:320` 增加 copy write kind 5 的名字 `code`。
- `test/evm/solidity-patterns/manifest.json:1166` 给 `0448_19495059_065877b669_4f138305be23` 增加 memory rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:1179` 检查 `memory_copy_write_kind:code = 1`。

## 当前限制

- `codecopy` 的 source 只记录为 code offset，没有区分 deployed object、creation object、constructor args。
- fallback base 仍然是实际目的地址，不代表 allocation base。
- 这轮只补 write surface，还没有把 create / return / ABI decode consumer 迁到读取 marker。

## 评分

- 实现效果：6/10。100-case 增加 44 个真实 rewrite marker，覆盖了 Solidity code/data copy 的一部分。
- 理解成本：3/10。复用现有 copy-write marker，只多一个 kind。
- 维护成本：4/10。后续只需要约定 kind 5 表示 code 区来源。

## 下一轮候选

1. 接 `evm_mcopy`，100-case raw 约 116，属于 memory-to-memory write surface。
2. 接 `evm_mstore8`，补齐 byte write，尤其 bytes/string 构造和小字段写入。
3. 让 `SolidityRevertPass` 优先读取 memory copy/consumer marker，先从 scratch returndata bubble 入手，满足 PLAN 里“后续消费者要读 marker”的要求。
4. 给 helper-return base 做 summary，减少 unknown-base fallback。

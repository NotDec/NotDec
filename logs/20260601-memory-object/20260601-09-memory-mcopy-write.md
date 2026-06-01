# Mcopy memory write rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

补齐 `evm_mcopy` 的 memory-to-memory copy-write marker。

当前 copy-write marker 已覆盖 calldata / returndata / code source。本轮增加 memory source：

- `notdec_solidity_memory_copy_write(base, offset, src, size, kind)`
- memory-to-memory kind = 6

这里 `src` 不再是外部数据 offset，而是源 memory 地址。字段名还沿用 `SourceOffset`，但语义是“copy source”。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- `YulUtilFunctions::copyToMemoryFunction(false, cleanup)` 生成 memory-to-memory copy helper。
- 如果目标 EVM 支持 MCOPY：
  - `mcopy(dst, src, length)`
  - 可选 cleanup：`mstore(add(dst, length), 0)`
- 如果不支持 MCOPY，Solidity 会退回循环 `mload/mstore`。

判断：`mcopy(dst, src, length)` 是 compiler-generated memory write。它常用于 memory array / bytes / string copy。目的地址如果能绑定到 free-memory base，就写 allocation-relative marker；否则沿用 fallback marker。

## 真实 IR 形状

manifest 里目前有一个真实调用：

- `test/evm/solidity-patterns/cases/0870_19499091_69ef351b75_57ed8326bd30.ll`
- 形状：`call void @evm_mcopy(ptr %mem, i256 %evm.add2, i256 %evm.add1, i256 %evm.mload)`

apehex 固定 100-case 中也有多处同形状调用，常见目的地址来自 `base + const`、private helper 返回值或函数参数。

## 计划

1. `MemoryWriteKind` 增加 `MemoryCopy = 6`。
2. `analyzeMemoryBuffers()` 单独处理 `evm_mcopy`，因为它只有 4 个参数，和 calldata/code copy 的 5 参数不同。
3. 参数位置按 IR 声明：`evm_mcopy(ptr mem, i256 dst, i256 src, i256 size)`。
4. 能匹配 `dst = base` 或 `base + const` 时，写 allocation-relative marker。
5. 匹配不到 base 时，写 fallback marker：`notdec_solidity_memory_copy_write(dst, 0, src, size, 6)`。
6. runner 增加 kind 6 的名字，manifest 增加真实 case oracle。

## 风险

- `mcopy` source 是 memory 地址，不是外部 offset。后续消费者必须看 kind，不能把它当 calldata/code offset。
- memory-to-memory copy 可能有重叠语义，marker 只表达“写入 dst buffer”，不表达复制方向细节。
- fallback base 仍然是实际目的地址，不代表 allocation base。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1，100.59s。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0870_19499091_69ef351b75_57ed8326bd30.ll -o /tmp/notdec-memory-mcopy-0870.ll --tr-level=0`。
  - 输出 9 个 copy write，其中 `memory` kind 1 个。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-mcopy`。
  - 100/100 跑完，总耗时 83.85s。
  - `notdec_solidity_memory_copy_write` 从 767 增加到 783。
  - memory copy 从 0 增加到 16。
  - calldata / returndata / scratch returndata / code 数量不变：151 / 75 / 497 / 44。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:24` 增加 `MemoryWriteKind::MemoryCopy = 6`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:306` 新增 `evm_mcopy` 分支。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:314` 对 allocation-relative `mcopy` 插入 kind 6 marker。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:321` 对 unknown-base `mcopy` 插入 fallback marker。
- `test/run_evm_solidity_patterns_suite.py:320` 增加 copy write kind 6 的名字 `memory`。
- `test/evm/solidity-patterns/manifest.json:3382` 给 `0870_19499091_69ef351b75_57ed8326bd30` 增加 memory rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:3395` 检查 `memory_copy_write_kind:memory = 1`。

## 当前限制

- marker 只表达 `dst` 被写入，没有表达 overlap 方向和逐字节复制细节。
- fallback base 是实际目的地址，不代表 allocation base。
- 这轮仍是 write surface，后续消费者还没迁到读取 marker。

## 评分

- 实现效果：5/10。100-case 增加 16 个真实 rewrite marker，数量不大但补齐了一类 EVM memory write。
- 理解成本：3/10。复用 copy-write marker，只新增 kind 6。
- 维护成本：4/10。后续消费者需要区分 kind 6 的 source 是 memory 地址。

## 下一轮候选

1. 接 `evm_mstore8`，补齐 byte write surface。
2. 让 `SolidityRevertPass` 优先读取 memory copy/consumer marker，先从 scratch returndata bubble 入手。
3. 给 helper-return base 做 summary，减少 unknown-base fallback。
4. 支持 `base + symbolic` offset，但要先限定在 ABI helper 内，避免误绑。

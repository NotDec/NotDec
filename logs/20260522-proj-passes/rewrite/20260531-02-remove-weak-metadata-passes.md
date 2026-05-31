# 删除弱 metadata pass

## 原始 prompt

把MemoryObjectPass，ValueCleanupTypeHintPass，StorageBytesStringPass删掉吧

## 背景

这三个 pass 当前只做粗粒度 metadata / rewrite marker：

- `MemoryObjectPass` 标 `mload/mstore(0x40)` free memory pointer。
- `ValueCleanupTypeHintPass` 标 address / uint / int cleanup hint。
- `StorageBytesStringPass` 标 storage bytes/string 的低位编码、`mstore8` 和短 bytes shift 候选。

它们没有被后续 pass 消费，也没有做结构化恢复。保留它们主要是在 suite 里统计覆盖率，
会让 EVM Solidity pipeline 看起来比实际语义恢复能力更完整。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:14` 删除
  `KIND_SOLIDITY_CLEANUP`、`KIND_SOLIDITY_STORAGE_BYTES_STRING`、
  `KIND_SOLIDITY_MEMORY_OBJECT`。
- `include/notdec/Passes/evm/SolidityPatterns.h:72` 删除
  `ValueCleanupTypeHintPass`、`StorageBytesStringPass`、`MemoryObjectPass` 声明。
- `include/notdec/Passes/PassManager.h:61` 删除 value cleanup / memory object pass
  调试显示名注册。
- `src/Passes/PassManager.cpp:292` 从 EVM pipeline 删除三个 pass。
- `src/Passes/evm/SolidityPatterns.cpp:33` 删除三个 pass 的统计项和 metadata kind。
- `src/Passes/evm/SolidityPatterns.cpp:1022` 删除只服务 `MemoryObjectPass` 的
  `isMstoreAt()` 和 `isMloadAt()`。
- `src/Passes/evm/SolidityPatterns.cpp:1514` 删除只服务 value cleanup / storage
  bytes-string 的 helper：`isLowBitMask()`、`hasLowBitMaskOperand()`、
  `classifyMask()`。
- `src/Passes/evm/SolidityPatterns.cpp:1853` 删除三个 pass 的实现。
- `test/evm/solidity-patterns/manifest.json:1` 删除
  `memory_buffer`、`value_cleanup`、`storage_bytes_string` pattern 和对应 metadata
  oracle。

## 通用性

这次不是调低 oracle，也不是针对单个样例绕开失败。删除的是没有后续消费者的候选标注。
free memory pointer 相关真实需求仍由各自 matcher 自己处理，例如
`SolidityRevertPass` 直接按 revert base / offset 识别 Error(string) 和 custom error buffer。

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `cmake --build ./build --target all -j4` 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)' --output-on-failure`
  通过，2 个测试全部通过，耗时 167.80s。

## 性能

本次只删 EVM Solidity metadata pass，不涉及类型恢复、结构体合并、
pointer analysis 或主 Wasm pipeline。

- fortune 同口径命令：
  `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-remove-weak-metadata-passes.c --tr-level=2`
- 结果：退出码 0，`elapsed=16.54`。

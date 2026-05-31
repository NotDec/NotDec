# 去掉 AbiDecodePass

## 原始 prompt

AbiDecodePass是干什么的，是不是主要就只在解析参数的时候用一下，是不是有些返回值也是按照ABI标准存的？那可能不应该单独作为pass，而是类似工具函数，被参数解析的部分调用？

## 背景

`AbiDecodePass` 现在只是在 selector/public entry 里标 `calldataload` / `calldatacopy`
候选，没有恢复参数结构，也没有被后续 pass 消费。真正有用的部分其实是：

- 识别 `calldataload(0)` 这类 selector word。
- 让 selector entry matcher 知道哪些 load 是入口相关的。

这些判断和 selector-entry 的 matcher 已经重叠，更适合留成内部 helper，而不是单独跑一个 pass。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:10` 删除
  `KIND_SOLIDITY_ABI_DECODE`。
- `include/notdec/Passes/evm/SolidityPatterns.h:43` 删除 `AbiDecodePass` 声明。
- `src/Passes/PassManager.cpp:288` 从 EVM pipeline 删除 `AbiDecodePass`。
- `src/Passes/evm/SolidityPatterns.cpp:28` 删除 `NumAbiDecodes`。
- `src/Passes/evm/SolidityPatterns.cpp:50` 删除 `notdec.solidity.abi_decode` kind。
- `src/Passes/evm/SolidityPatterns.cpp:402` 起把 `calldataload(0)` 识别收成
  `isCalldataWordLoadAt()`，供 selector matcher 复用。
- `src/Passes/evm/SolidityPatterns.cpp:1666` 删除 `AbiDecodePass::run()`。
- `test/evm/solidity-patterns/manifest.json:1` 删除所有 case 的
  `abi_decode` pattern 和 `notdec.solidity.abi_decode` oracle。

## 通用性

这次不是把 ABI 相关东西整体删掉，而是把“入口里识别 selector word”这部分留在共用 helper 里。
后续如果真要恢复参数，应该在具体语义 pass 里调用 ABI helper，而不是再起一个只打弱标记的 pass。

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `rg -n "AbiDecodePass|KIND_SOLIDITY_ABI_DECODE|notdec\\.solidity\\.abi_decode|abi_decode|NumAbiDecodes|isAbiDecodeContext" include src test/evm test/run_evm_solidity_patterns_suite.py test/run_evm_solidity_rewrite_suite.py`
  无残留。
- `git diff --check` 通过。
- `cmake --build ./build --target all -j4` 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)' --output-on-failure`
  通过，2 个测试全部通过，耗时 167.50s。

## 性能

本次只删 EVM Solidity metadata pass，不涉及类型恢复、结构体合并、
pointer analysis 或主 Wasm pipeline。

- fortune 同口径命令：
  `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-remove-abidecodepass.c --tr-level=2`
- 结果：退出码 0，`elapsed=16.59`。

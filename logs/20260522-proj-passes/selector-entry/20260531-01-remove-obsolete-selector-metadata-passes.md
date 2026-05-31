# 删除过时 selector metadata pass

## 原始 prompt

SolidityPatternAnnotationPass 感觉没什么用，去掉吧，SelectorInlinedLogicExtractionPass，这个也去掉吧

## 背景

`SelectorEntryOutliningPass` 已经负责 selector entry 的实质处理：按 CFG 边界保守拆出
内联 fallback / receive / selector body helper。相比之下：

- `SolidityPatternAnnotationPass` 只写模块级 `notdec.solidity.patterns` 标记。
- `SelectorInlinedLogicExtractionPass` 只给 selector 函数、prologue 和明显 call/log
  贴粗粒度 metadata。

这些 metadata 不再是后续 pass 的输入，保留会让 selector-entry 链路多一层旧语义。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:8` 删除
  `KIND_SOLIDITY_ENTRY_KIND`、`KIND_SOLIDITY_SELECTOR_PROLOGUE`、
  `KIND_SOLIDITY_SELECTOR_INLINED_BODY` 声明。
- `include/notdec/Passes/evm/SolidityPatterns.h:22` 删除
  `SolidityPatternAnnotationPass` 和 `SelectorInlinedLogicExtractionPass` 声明。
- `src/Passes/evm/SolidityPatterns.cpp:27` 删除 selector prologue / inlined body
  统计项。
- `src/Passes/evm/SolidityPatterns.cpp:52` 删除三个旧 metadata kind 定义。
- `src/Passes/evm/SolidityPatterns.cpp:1554` 删除两个 pass 的实现，入口直接接
  `SelectorEntryOutliningPass::run()`。
- `src/Passes/PassManager.cpp:287` 从 EVM pipeline 移除两个 pass。
- `include/notdec/Passes/PassManager.h:55` 删除两个 pass 的调试显示名注册。
- `test/evm/solidity-patterns/manifest.json:18` 删除旧 selector metadata oracle。
- `test/evm/solidity-rewrite/manifest.json:10` 删除 `selector_inlined_body` rewrite
  oracle。
- `test/run_evm_solidity_rewrite_suite.py:121` 删除 `selector_inlined_body` 计数。

## 通用性

这次不是放宽 matcher，也不是针对样例调计数。删掉的是不再被语义链路消费的旧标注。
selector entry 的判断仍由 `SelectorEntryOutliningPass` 负责，保留
`selector_outlined_body` 和 `selector_outline_skipped` 这两类当前仍有用的 oracle。

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `python3 -m json.tool test/evm/solidity-rewrite/manifest.json` 通过。
- `python3 -m py_compile test/run_evm_solidity_rewrite_suite.py` 通过。
- `cmake --build ./build --target all -j4` 通过。
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)' --output-on-failure`
  通过，2 个测试全部通过，耗时 175.94s。

注意：第一次验证时曾把 build 和 CTest 并行启动，CTest 用到旧二进制，导致
`notdec.evm.solidity_patterns` 中 30 个 case 仍看到旧 selector metadata 而失败。
构建结束后重新单独跑同一条 CTest 已通过。

## 性能

本次只删 EVM Solidity selector metadata pass，不涉及类型恢复、结构体合并、
pointer analysis 或主 Wasm pipeline。

- fortune 同口径命令：
  `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-remove-selector-metadata.c --tr-level=2`
- 结果：退出码 0，`elapsed=16.70`。

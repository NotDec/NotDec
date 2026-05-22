# ABI decode 候选标注

## 背景

收集 Solidity pattern case 时，manifest 已经开始按 pass 类别统计覆盖，但 ABI decode 类别还没有对应 metadata。当前先做保守标注，只记录 calldata 读取点，不恢复参数类型。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:13` 增加 `KIND_SOLIDITY_ABI_DECODE`。
- `include/notdec/Passes/evm/SolidityPatterns.h:51` 增加 `AbiDecodePass` 声明，说明它只标静态 word load 和动态 calldata copy。
- `src/Passes/evm/SolidityPatterns.cpp:20` 增加 `NumAbiDecodes` 统计。
- `src/Passes/evm/SolidityPatterns.cpp:39` 增加 `notdec.solidity.abi_decode` metadata 名。
- `src/Passes/evm/SolidityPatterns.cpp:143` 增加 `isAbiDecodeContext`，只在 selector/public entry 中标注。
- `src/Passes/evm/SolidityPatterns.cpp:316` 实现 `AbiDecodePass::run`：
  - `evm_calldataload(..., 0)` 标为 `selector_word`。
  - 其他 `evm_calldataload` 标为 `static_arg_word`。
  - `evm_calldatacopy` 标为 `dynamic_copy_candidate`。
- `src/Passes/PassManager.cpp:289` 把 `AbiDecodePass` 放在 payability 后、ABI return 前。

## 通用性

这次没有按样例名或常量 selector 特判。规则只看 helper 类型和函数上下文，所以能覆盖当前 batch 中不同大小的 dispatcher/public entry。它仍然是候选标注，动态参数边界和类型要等后续 memory buffer/dataflow 更稳以后再做。

## 验证

- `cmake --build ./build --target all -j4` 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过，14 个 case 全部通过。
- fortune 同口径运行：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-eval.c --tr-level=2`，退出码 0，耗时 16.66s。

## 评分

- 实现效果：7/10。ABI decode 类别有了可测入口，但还没恢复参数结构。
- 复杂度：2/10。只新增一个小 pass，没有引入跨块状态。
- 维护成本：2/10。后续可以在同一 pass 内继续收紧 kind 和 oracle。

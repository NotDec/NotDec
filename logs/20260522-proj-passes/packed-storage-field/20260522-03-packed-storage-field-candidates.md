# packed storage field 候选标注

## 背景

packed storage field 之前没有 pass。当前样例里已经有典型 `sload -> mask/shift -> sstore` 和 `sload -> div/and` 形状，需要先把这些低层 field 访问标出来。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:20` 增加 `KIND_SOLIDITY_PACKED_STORAGE_FIELD`。
- `include/notdec/Passes/evm/SolidityPatterns.h:122` 增加 `PackedStorageFieldPass`。
- `src/Passes/evm/SolidityPatterns.cpp:31` 增加统计 `NumPackedStorageFields`。
- `src/Passes/evm/SolidityPatterns.cpp:57` 增加 metadata 名 `notdec.solidity.packed_storage_field`。
- `src/Passes/evm/SolidityPatterns.cpp:272` 增加 `expressionHasPackedStorageOp`，识别 mask/or/xor/mul 和 EVM shift/div/exp。
- `src/Passes/evm/SolidityPatterns.cpp:673` 实现 `PackedStorageFieldPass::run`：
  - `sstore` 的值依赖旧 `sload` 且经过位运算，标 `packed_store_candidate`。
  - `sload` 的直接 use 进入位运算链，标 `packed_load_candidate`。
- `src/Passes/PassManager.cpp:299` 接入 pass。

## 通用性

规则基于 `sload/sstore` 和位运算链，不依赖具体 slot、变量名或合约样例。它只输出候选，不猜 field offset/width。

## 验证

- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过。
- manifest 中 `packed_storage_field` 覆盖 14 个 case。
- fortune 同口径运行退出码 0，耗时 16.66s。

## 评分

- 实现效果：6/10。能覆盖当前 field load/store 形状，但还没恢复 offset/width。
- 复杂度：3/10。只做深度受限表达式扫描。
- 维护成本：3/10。后续可复用表达式扫描并补结构化 field 描述。

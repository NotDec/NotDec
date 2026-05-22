# Solidity pattern rewrite markers

## 背景

当前 58 个 EVM Solidity pattern case 已经覆盖每个 pass 类别至少 50 个样例，但主链路还停在 metadata-only。计划要求 rewrite 复用各 pass 的识别结果，并且 rewrite 后 IR 仍要能让后续 pass 继续识别。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:109` 新增 `isRewriteMarkerCall`，让 matcher 能识别本轮插入的 rewrite marker。
- `src/Passes/evm/SolidityPatterns.cpp:138` 修改 `isEmptyRevertBlock`，忽略 rewrite marker，避免 payability / revert rewrite 影响后面的 checked-bounds 识别。
- `src/Passes/evm/SolidityPatterns.cpp:160` 新增 `getRewriteMarkerName`，把 `notdec.solidity.*` metadata 名转换成 `notdec_solidity_rewrite_*` helper 名。
- `src/Passes/evm/SolidityPatterns.cpp:169` 新增 `getRewriteKindCode`，把同一类里的 kind 字符串稳定编码成 `i256` 参数。
- `src/Passes/evm/SolidityPatterns.cpp:181` 和 `src/Passes/evm/SolidityPatterns.cpp:205` 新增 instruction / function 两种 `insertRewriteMarker`。
- `src/Passes/evm/SolidityPatterns.cpp:219` 和 `src/Passes/evm/SolidityPatterns.cpp:228` 修改 `addStringMetadata`，所有 pass 复用原识别结果同步插入 rewrite marker。
- `src/Passes/evm/SolidityPatterns.cpp:237` 修改 `markBlock`，先收集原始指令再打 metadata，避免遍历时反复标记新插入的 marker。
- `test/run_evm_solidity_patterns_suite.py:101` 新增 rewrite marker 名转换和计数。
- `test/run_evm_solidity_patterns_suite.py:211` 扩展 oracle，开启后要求每个 metadata 类别都有同数量的 rewrite marker。
- `test/evm/solidity-patterns/manifest.json:3` 开启 `expect_rewrite_markers`。

## 通用性

这版没有给某个 case 写专门规则。rewrite 入口集中在 `addStringMetadata`，所以 selector、payability、revert、checked-bounds、value-cleanup、memory、ABI、storage、event、external-call 等现有 pass 都走同一套规则。当前不删除原始 EVM helper，只插入高层 marker，目的是先建立稳定可测的 rewrite 表达，避免过早破坏后续 matcher。

## 风险和代价

- IR 会多出 marker call，suite 时间从旧 binary 的约 76.44s 增加到 80.91s，约 5.8%。后续如果要压性能，可以改成按类别开关或只对需要 oracle 的类别插入。
- 这还不是最终高层 IR，只是第一阶段 rewrite surface。真正删除低层 helper、合并 buffer / guard 还需要逐类继续做。

复杂度评分：实现效果 7/10，复杂度 4/10，维护成本 4/10。更彻底的方案是每个 pass 输出专门的 typed intrinsic，但现在 memory / ABI / storage 数据流还不够稳，先统一 marker 更稳。

## 验证

- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 结果：58/58 passed，80.91s。

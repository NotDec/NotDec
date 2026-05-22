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

## 补充修改：低层 helper hide 标记

- `src/Passes/evm/SolidityPatterns.cpp:205` 新增 `insertHiddenMarker`，每个被 pass 认领的低层点同步插入 `notdec_solidity_rewrite_hidden(i256 kind)`。
- `src/Passes/evm/SolidityPatterns.cpp:241` 新增 function 版本 `insertHiddenMarker`，处理 `entry_kind`、函数级 ABI return 这类不是直接挂在 helper 上的语义。
- `src/Passes/evm/SolidityPatterns.cpp:254` 和 `src/Passes/evm/SolidityPatterns.cpp:264` 更新 `addStringMetadata`，确保 hidden marker 和原识别结果同源。
- `test/run_evm_solidity_patterns_suite.py:113` 新增 hidden marker 计数。
- `test/run_evm_solidity_patterns_suite.py:220` 和 `test/run_evm_solidity_patterns_suite.py:246` 扩展 oracle，要求 hidden marker 总数等于该 case 所有 `notdec.solidity.*` metadata 期望数之和。
- `test/evm/solidity-patterns/manifest.json:4` 开启 `expect_rewrite_hidden`。

## 补充修改：原指令 hide metadata

- `src/Passes/evm/SolidityPatterns.cpp:169` 新增 `getHiddenMetadataName`，生成 `notdec.solidity.rewrite_hidden.<category>`。这里不用单一 metadata 名，避免同一条指令被多个 pass 认领时互相覆盖。
- `src/Passes/evm/SolidityPatterns.cpp:188` 新增 `getFunctionRewritePoint`，统一函数级 rewrite / hide marker 的插入点。
- `src/Passes/evm/SolidityPatterns.cpp:261` 和 `src/Passes/evm/SolidityPatterns.cpp:267` 新增 instruction / function 两种 `addHiddenMetadata`。
- `src/Passes/evm/SolidityPatterns.cpp:276` 和 `src/Passes/evm/SolidityPatterns.cpp:287` 更新 `addStringMetadata`，在原始低层指令或函数入口点同步写 hide metadata。
- `test/run_evm_solidity_patterns_suite.py:118` 新增 hidden metadata 计数。
- `test/run_evm_solidity_patterns_suite.py:236` 和 `test/run_evm_solidity_patterns_suite.py:251` 扩展 oracle，要求 hidden metadata 数量和 hidden marker 数量、metadata 总数一致。

## 通用性

这版没有给某个 case 写专门规则。rewrite 入口集中在 `addStringMetadata`，所以 selector、payability、revert、checked-bounds、value-cleanup、memory、ABI、storage、event、external-call 等现有 pass 都走同一套规则。当前不删除原始 EVM helper，而是在命中点同时写高层 marker、hide marker 和原指令 hide metadata。这样后端可以直接按 `notdec.solidity.rewrite_hidden.*` 隐藏低层残片，后续 matcher 也还能继续看原始 helper。

## 风险和代价

- IR 会多出 marker call。第一版 rewrite marker 后 suite 时间从旧 binary 的约 76.44s 增加到 80.91s，约 5.8%；加入 hidden marker 后为 84.55s，比旧 binary 增加约 10.6%。后续如果要压性能，可以改成按类别开关或只对需要 oracle 的类别插入。
- 这还不是最终高层 IR，只是第一阶段 rewrite surface。真正删除低层 helper、合并 buffer / guard 还需要逐类继续做。

复杂度评分：实现效果 7/10，复杂度 4/10，维护成本 4/10。更彻底的方案是每个 pass 输出专门的 typed intrinsic，但现在 memory / ABI / storage 数据流还不够稳，先统一 marker 更稳。

## 验证

- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 第一版 rewrite marker 结果：58/58 passed，80.91s。
  - 加入 hidden marker 后结果：58/58 passed，84.55s。
  - 加入原指令 hide metadata 后结果：58/58 passed，84.58s。

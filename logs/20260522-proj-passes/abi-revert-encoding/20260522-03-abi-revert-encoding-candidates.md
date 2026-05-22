# ABI revert encoding 候选标注

## 背景

revert pass 已能标 empty revert、panic 和 returndata bubble，但 ABI revert buffer 本身没有单独 metadata。为了让测试覆盖能按类别统计，这次先标 selector 写入和非空 revert buffer。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:15` 增加 `KIND_SOLIDITY_ABI_REVERT_ENCODING`。
- `include/notdec/Passes/evm/SolidityPatterns.h:83` 增加 `AbiRevertEncodingPass`。
- `src/Passes/evm/SolidityPatterns.cpp:23` 增加统计 `NumAbiRevertEncodings`。
- `src/Passes/evm/SolidityPatterns.cpp:50` 增加 metadata 名 `notdec.solidity.abi_revert_encoding`。
- `src/Passes/evm/SolidityPatterns.cpp:188` 增加 `isPanicSelectorValue`。
- `src/Passes/evm/SolidityPatterns.cpp:199` 增加 `isSelectorWord`。
- `src/Passes/evm/SolidityPatterns.cpp:532` 实现 `AbiRevertEncodingPass::run`：
  - `mstore(..., shl(224, selector))` 标 selector 写入。
  - 非空、非 returndata bubble 的 `evm_revert` 标 encoded revert 候选。
- `src/Passes/PassManager.cpp:292` 接入 pass。

## 通用性

规则只看 ABI selector 写法和 revert 参数，不按 selector 常量表或样例名判断。当前仍是 candidate，不解析 Error(string) 或 custom error 参数结构。

## 验证

- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过。
- manifest 中 `abi_revert_encoding` 覆盖 11 个 case。
- fortune 同口径运行退出码 0，耗时 16.66s。

## 评分

- 实现效果：6/10。能稳定统计 ABI revert buffer 候选，但还没分 Error/custom error 参数。
- 复杂度：2/10。只依赖现有 block 局部扫描。
- 维护成本：2/10。后续可在同一 pass 内补 buffer 参数解析。

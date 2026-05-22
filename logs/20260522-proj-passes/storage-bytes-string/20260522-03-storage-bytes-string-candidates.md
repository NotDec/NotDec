# storage bytes/string 候选标注

## 背景

storage bytes/string 的短长编码还没有 metadata。现有样例里相关逻辑常被拆进 helper，不一定和 `sload/sstore` 在同一函数，所以标注应看编码形状，而不是要求同函数直接访问 storage。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:21` 增加 `KIND_SOLIDITY_STORAGE_BYTES_STRING`。
- `include/notdec/Passes/evm/SolidityPatterns.h:132` 增加 `StorageBytesStringPass`。
- `src/Passes/evm/SolidityPatterns.cpp:33` 增加统计 `NumStorageBytesStrings`。
- `src/Passes/evm/SolidityPatterns.cpp:59` 增加 metadata 名 `notdec.solidity.storage_bytes_string`。
- `src/Passes/evm/SolidityPatterns.cpp:310` 增加低位 mask 判断，覆盖 `1`、`31`、`127`。
- `src/Passes/evm/SolidityPatterns.cpp:709` 实现 `StorageBytesStringPass::run`：
  - 低位 mask 标 `low_bit_encoding_candidate`。
  - `mstore8` 标 `byte_copy_candidate`。
  - `shl/shr 248` 标 `short_bytes_shift_candidate`。
- `src/Passes/PassManager.cpp:301` 接入 pass。

## 通用性

规则不按样例名、不依赖具体 storage slot。它标的是 bytes/string 编码候选，后续还需要和 storage address、memory copy 绑定后再确认短/长分支。

## 验证

- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过。
- manifest 中 `storage_bytes_string` 覆盖 11 个 case。
- fortune 同口径运行退出码 0，耗时 16.66s。

## 评分

- 实现效果：5/10。能收集候选覆盖，但还没有确认 storage 根和短长分支。
- 复杂度：2/10。只看局部编码操作。
- 维护成本：3/10。后续需要减少普通 bytes/memory helper 的误标。

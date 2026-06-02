# ABI return literal payload final word marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续看 Solidity literal 的 32 字节 word。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`storeLiteralInMemoryFunction()` 生成 `mstore(add(memPtr, offset), wordValue)`。
- `/sn640/solidity/libsolutil/CommonData.cpp:187`，`formatAsStringOrNumber()` 对 bytes literal 使用 left-aligned 32 字节 word。

当前 NotDec IR 里，短 literal 常见形状是 `evm_shl(shift, rawWord)`。EVM `SHL` 语义是 256-bit 左移，shift >= 256 时结果为 0。

## 问题

上一轮 `notdec_solidity_abi_return_dynamic_array_literal_payload_shift` 已经把 `shift` 和 `rawWord` 写到 IR 里，但后续如果要恢复最终 bytes/string，仍然要自己做 256-bit 左移。

这轮把折叠后的 final word 也写成 IR marker，减少后续 pass 的重复计算。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取 literal payload shift marker 所需事实，折叠出 final word，并写成更直接的 marker。没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:3`
  - 新增 `llvm/ADT/APInt.h`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:45`
  - 新增 `NumAbiReturnDynamicArrayLiteralPayloadWords` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:665`
  - 新增 `foldEvmShlToI256()`。
  - 规则是 `rawWord` 截断 / 扩展到 256-bit；`shift >= 256` 得 0；否则执行 256-bit 左移。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:860`
  - 新增 `insertAbiReturnDynamicArrayLiteralPayloadWordMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_literal_payload_word(sourceArray, length, dataOffset, shift, rawWord, finalWord, dataWord, returnBase, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1033`
  - 在 literal payload shift 分支里同步插入 final word marker。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 literal payload word oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5274`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 literal payload word oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5294`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 literal payload word oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5313`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 literal payload word oracle，期望 2。

## 判断

这个 marker 比 `literal_payload_shift` 更适合给后续 pass 消费：后续可以直接读 final word，不需要知道 `evm_shl` 的细节。

当前仍没有把 final word 解成字符串文本。原因是这一步还需要按 length 截断成 bytes，并考虑 printable / non-printable 显示策略。先把 256-bit word 固定下来。

## 验证

命令：

```bash
cmake --build ./build --target all -j4
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

结果：

- build 通过。
- manifest JSON 校验通过。
- `notdec.evm.solidity_patterns` 通过，用时 110.36s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-payload-word
mkdir -p /tmp/notdec-memory-fixed100-literal-payload-word/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-payload-word/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
- `notdec_solidity_abi_return_dynamic_array_literal_payload_word`: 9。
- `notdec_solidity_abi_return_dynamic_array_literal_payload_shift`: 9。
- `notdec_solidity_abi_return_dynamic_array_literal_payload`: 9。
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 9。
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 9。
- 对照关键 ABI return markers：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21。
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18。
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2。
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1。
  - `notdec_solidity_abi_return_dynamic_array_storage_source`: 10。

## 下一轮候选

1. 基于 `length + finalWord` 产出 literal bytes/string value marker。
2. 扩大样例找多 word literal，决定是多个 word marker 还是一个聚合 marker。
3. 把 literal payload word marker 和 copy loop / mcopy / helper copy marker 合并，形成完整 ABI return literal bytes/string 语义。

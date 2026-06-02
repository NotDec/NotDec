# ABI return literal bytes marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续看 Solidity literal bytes/string 的 memory 形状。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`storeLiteralInMemoryFunction()` 按 32 字节 word 写 literal。
- `/sn640/solidity/libsolutil/CommonData.cpp:187`，`formatAsStringOrNumber()` 对 bytes literal 使用 left-aligned 32 字节 word。

上一轮已经把 `evm_shl(shift, rawWord)` 折叠成 final 256-bit word。本轮把 ABI return 里真正后续需要的 bytes 事实单独写出来：length、final word、return buffer。

## 问题

`literal_payload_word` 仍保留了 source array、offset、shift、rawWord、dataWord 等较底层信息。后续如果只是想知道 “这个 ABI return 返回了一个单 word literal bytes/string”，不应该继续理解这些 payload 细节。

因此需要更高层的 marker，只表达 literal bytes 的核心事实。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取 literal payload word 的事实，产出更高层的 literal bytes marker。没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:47`
  - 新增 `NumAbiReturnDynamicArrayLiteralBytes` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:890`
  - 新增 `insertAbiReturnDynamicArrayLiteralBytesMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_literal_bytes(sourceArray, length, finalWord, returnBase, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1062`
  - 在 literal payload word 分支里同步插入 literal bytes marker。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 literal bytes oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5275`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 literal bytes oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5296`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 literal bytes oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5316`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 literal bytes oracle，期望 2。

## 判断

这个 marker 是给后续 pass 消费的高层 IR 事实：不再要求后续理解 `evm_shl` 或 payload word 的来源。当前仍只覆盖单 word literal；多 word literal 后续单独处理。

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
- `notdec.evm.solidity_patterns` 通过，用时 110.93s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-bytes
mkdir -p /tmp/notdec-memory-fixed100-literal-bytes/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-bytes/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
- `notdec_solidity_abi_return_dynamic_array_literal_bytes`: 9。
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

1. 把 literal bytes marker 和 copy loop / mcopy / helper copy marker 合并，产出完整 ABI return literal bytes/string 语义。
2. 扩大样例找多 word literal。
3. 给直接常量 data word 的 literal payload 补同等 literal bytes marker。

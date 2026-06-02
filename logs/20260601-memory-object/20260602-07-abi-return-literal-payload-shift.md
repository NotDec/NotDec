# ABI return literal payload shift marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续看 Solidity literal 写入 memory 的数据 word。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`storeLiteralInMemoryFunction()` 对 literal 每 32 字节生成一个 `mstore(add(memPtr, offset), wordValue)`。
- `/sn640/solidity/libsolutil/CommonData.cpp:187`，`formatAsStringOrNumber()` 对不可直接打印的 bytes 返回 left-aligned 32 字节 word。

在当前 EVM IR 中，短 literal 的 left-aligned word 不是直接常量，而是 `evm_shl(shift, rawWord)`。fixed100 中 9 个 literal payload 都是这个形状。

## 问题

上一轮 `notdec_solidity_abi_return_dynamic_array_literal_payload` 已经带出了 data word，但 data word 仍是 `%evm.shl` 这个 SSA 值。后续如果要恢复实际 bytes/string，还要回头追 `evm_shl` 的两个常量参数。

这轮把 `shift` 和 `rawWord` 也写进 ABI return 级别 marker，避免后续 pass 重新追 use-def。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取 literal payload 的 `evm_shl(const, const)`，再产出更具体的 shift payload marker。没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:42`
  - 新增 `NumAbiReturnDynamicArrayLiteralPayloadShifts` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:652`
  - 新增 `getLiteralPayloadShift()`。
  - 只接受 `evm_shl` 且两个参数都是常量。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:826`
  - 新增 `insertAbiReturnDynamicArrayLiteralPayloadShiftMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_literal_payload_shift(sourceArray, length, dataOffset, shift, rawWord, dataWord, returnBase, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:988`
  - 在 literal payload 分支里，如果 data word 是常量 shift，就同步插入 shift marker。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 literal payload shift oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5273`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 literal payload shift oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5292`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 literal payload shift oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5310`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 literal payload shift oracle，期望 2。

## 判断

这个 marker 没有直接折叠出最终 bytes。原因是本轮只把 Solidity codegen 直接暴露的常量事实写入 IR，保持 marker 简单稳定。最终 bytes/string 的恢复可以在下一轮基于 `length + shift + rawWord` 做。

如果以后出现 data word 已经是直接常量的 literal payload，它仍有 `literal_payload` marker，但不会有 `literal_payload_shift` marker。

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
- `notdec.evm.solidity_patterns` 通过，用时 110.65s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-payload-shift
mkdir -p /tmp/notdec-memory-fixed100-literal-payload-shift/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-payload-shift/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
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

1. 基于 `length + shift + rawWord` 产出 final bytes/string value marker。
2. 扩大样例找 data word 是直接常量、或多 word literal 的情况。
3. 把 literal payload shift marker 和 copy loop / mcopy / helper copy 关系合并，形成完整 ABI return literal bytes/string 语义。

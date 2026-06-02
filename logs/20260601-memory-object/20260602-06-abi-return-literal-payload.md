# ABI return literal payload marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续从 Solidity literal helper 对照 IR。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:145`，`copyLiteralToMemoryFunction()` 分配 memory array，并调用 literal store helper 写 `add(memPtr, 32)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`storeLiteralInMemoryFunction()` 按 32 字节 word 生成 `mstore(add(memPtr, offset), wordValue)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3320`，`allocateMemoryArrayFunction()` 对动态 memory array 写 `mstore(memPtr, length)`。
- `/sn640/solidity/libsolutil/CommonData.cpp:187`，`formatAsStringOrNumber()` 把 literal word 格式化成可直接写入的常量，非 printable bytes 使用 left-aligned hex word。

所以 literal return source 的有效 payload 是：

- source array base。
- literal length。
- data offset，第一版只覆盖当前样例里的 32。
- data word，当前可以是常量或 `evm_shl(const, const)`。
- return buffer base / size，用来绑定 ABI return consumer。

## 问题

上一轮的 `notdec_solidity_abi_return_dynamic_array_literal_source` 只说明 source 是 literal，但没有把 length 和 data word 直接作为 ABI return 级别的 IR 事实写出来。后续 pass 如果要恢复具体 bytes/string，还得重新找 source array 上的 `notdec_solidity_memory_write`。

这还不够接近 IR rewrite 目标。本轮把 literal payload 直接写进 ABI return marker。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取已有 memory write marker 和 literal source 判断，产出更具体的 literal payload marker。没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:40`
  - 新增 `NumAbiReturnDynamicArrayLiteralPayloads` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:793`
  - 新增 `insertAbiReturnDynamicArrayLiteralPayloadMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_literal_payload(sourceArray, length, dataOffset, dataWord, returnBase, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:951`
  - 在 literal source 分支里同步插入 payload marker。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 literal payload oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5272`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 literal payload oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5290`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 literal payload oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5307`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 literal payload oracle，期望 2。

## 判断

这个 marker 是对 `literal_source` 的补强。它没有尝试把 `evm_shl(const, const)` 折成最终 bytes，也没有处理多 word literal。当前 fixed100 里命中的 literal return 都是单 data word，先按这个真实形状推进。

后续如果出现多 word literal，需要让 payload marker 支持多个 data word，或者每个 word 插一个 payload-word marker。

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
- `notdec.evm.solidity_patterns` 通过，用时 111.03s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-payload
mkdir -p /tmp/notdec-memory-fixed100-literal-payload/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-payload/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
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

1. 把 `evm_shl(const, const)` 的 literal payload 归一成 final bytes word，产出更容易被后端消费的 marker。
2. 扩大 fixed100 之外的样例，找多 word literal 或非 literal memory builder。
3. 让 copy loop / mcopy / helper copy marker 和 literal payload marker 合并，产出完整的 ABI return literal bytes/string 语义。

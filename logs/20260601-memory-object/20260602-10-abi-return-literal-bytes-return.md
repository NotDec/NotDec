# ABI return literal bytes return marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

Solidity public wrapper 在 `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:766-768` 生成：

- `memPos := allocateUnbounded()`
- `memEnd := abiEncode(memPos, returns...)`
- `return(memPos, sub(memEnd, memPos))`

短 literal bytes / string 的 memory array helper 在 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:145-160` 生成 `copyLiteralToMemoryFunction()`：先分配 memory array，再调用 `storeLiteralInMemoryFunction()` 写 data。后者在 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164-170` 按 word 准备 literal 写入。

所以当前 IR 里的完整语义不是只有 literal bytes 值，还要确认这段 bytes 被 copy loop / mcopy / helper copy 放进 ABI return buffer，并最终由 `return(base, size)` 消费。

## 本轮目标

上一轮已经能插入：

- `notdec_solidity_abi_return_dynamic_array_literal_bytes`

但它只说明 source memory array 里有可折叠的 literal bytes。它还没把 source bytes 和 ABI return consumer 的复制关系合到一个 marker 里。

本轮补一个更完整的 rewrite surface：

- `notdec_solidity_abi_return_dynamic_array_literal_bytes_return(sourceArray, length, finalWord, returnBase, returnSize, copyKind, kind)`

`copyKind` 含义：

- `1`：copy loop
- `2`：mcopy
- `3`：helper copy

这轮仍是 `rewrite surface`，不是最终高层 C 输出改写。它比 metadata 更进一步，因为 marker 直接写入 IR，并把 literal bytes、return buffer、return size、复制方式绑定到同一个 consumer surface 上。

## 修改

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:49-50`
  - 增加 `NumAbiReturnDynamicArrayLiteralByteReturns` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:678-693`
  - 新增 `getLiteralBytesReturnCopyKind()`，把 copy loop / mcopy / helper copy 编成稳定 payload。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:935-955`
  - 新增 `insertAbiReturnDynamicArrayLiteralBytesReturnMarker()`，插入 `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1108-1114`
  - 在 literal bytes marker 后，如果当前 dynamic return source 有复制关系，就继续插入 bytes-return marker。
- `test/evm/solidity-patterns/manifest.json:3932-3934`
  - 给 `0118_19493248_593c688d00_799916048038` 增加 1 个 bytes-return oracle。
- `test/evm/solidity-patterns/manifest.json:5257-5267`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 2 个 mcopy bytes-return oracle。
- `test/evm/solidity-patterns/manifest.json:5279-5288`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 1 个 helper-copy bytes-return oracle。
- `test/evm/solidity-patterns/manifest.json:5301-5310`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 2 个 copy-loop bytes-return oracle。
- `test/evm/solidity-patterns/manifest.json:5322-5331`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 2 个 copy-loop bytes-return oracle。

## 验证

构建：

```bash
cmake --build ./build --target all -j4
```

结果：通过。

manifest JSON：

```bash
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json
```

结果：通过。

Solidity patterns：

```bash
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

结果：通过，`111.13 sec`。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-bytes-return
mkdir -p /tmp/notdec-memory-fixed100-literal-bytes-return/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-bytes-return/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18
- `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
- `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_payload`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_payload_shift`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_payload_word`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_bytes`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`: 9
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 10

命中 `literal_bytes_return` 的 fixed100 样例：

- `25524_19769741_8c9517a9e9_3555426790bd.ll`
- `25544_19769956_6dc86f13a0_e354b4c44525.ll`
- `25563_19770118_498bb800eb_8e79ddef1b42.ll`
- `25619_19770992_6b2850de20_21a0b617860a.ll`
- `25625_19771047_de9c01a012_1459648d68cd.ll`

## 下一步

按价值排序：

1. 让 ABI return 更高层 rewrite 优先读取 `literal_bytes_return`，把 literal bytes return 从 marker surface 推进到 consumer rewrite。
2. 扩大 fixed100 之外的样例，找多 word literal bytes / string。当前只覆盖单 data word。
3. 找不是 literal 的 memory source return，例如 concat / builder 形状，避免当前动态 return 只覆盖 storage source 和 literal source。

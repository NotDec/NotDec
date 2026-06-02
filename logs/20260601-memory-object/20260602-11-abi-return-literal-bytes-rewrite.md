# ABI return literal bytes rewrite marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

Solidity public wrapper 在 `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:766-768` 先拿 `memPos`，再 `abiEncode(memPos, returns...)`，最后 `return(memPos, sub(memEnd, memPos))`。

literal bytes / string 的 memory source 来自 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:145-160` 的 `copyLiteralToMemoryFunction()`：分配 memory array，把 literal data 写到 `memPtr + 32`。`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164-170` 的 `storeLiteralInMemoryFunction()` 负责把 literal 切成 word。

上一轮已经把 literal memory source、copy loop / mcopy / helper copy 和 final `return` 合成了 `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`。这轮继续往 plan 要求的 consumer rewrite 推：插入 ABI return 级别的 rewrite marker，而不是只停在 memory fact marker。

## 本轮目标

新增：

- `notdec_solidity_rewrite_abi_return_literal_bytes(sourceArray, length, finalWord, returnBase, returnSize, copyKind, kind)`

它和 `notdec_solidity_abi_return_dynamic_array_literal_bytes_return` 一一对应，但语义层级更高：表示这个 ABI return 已经可以按 literal bytes return 处理。后续 lowering / C 输出不需要再扫描低层 `mstore`、copy loop、`mcopy` 或 helper copy 才知道这是 literal bytes return。

这轮属于 `consumer rewrite` 的一小步：还没有删除低层指令，也没有直接改 C 输出，但已经产出 ABI return 级别 semantic rewrite marker。

## 修改

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:51-52`
  - 增加 `NumAbiReturnLiteralByteRewrites` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:959-981`
  - 新增 `insertAbiReturnLiteralBytesRewriteMarker()`，插入 `notdec_solidity_rewrite_abi_return_literal_bytes`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1138-1142`
  - 在 `literal_bytes_return` 确认 copy kind 后，同时插入 ABI return 级别 rewrite marker。
- `test/evm/solidity-patterns/manifest.json:3932-3934`
  - 给 `0118_19493248_593c688d00_799916048038` 增加 1 个 `notdec_solidity_rewrite_abi_return_literal_bytes` oracle。
- `test/evm/solidity-patterns/manifest.json:5267-5269`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 2 个 oracle。
- `test/evm/solidity-patterns/manifest.json:5290-5292`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 1 个 oracle。
- `test/evm/solidity-patterns/manifest.json:5312-5314`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 2 个 oracle。
- `test/evm/solidity-patterns/manifest.json:5334-5336`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 2 个 oracle。

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

结果：通过，`111.34 sec`。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-bytes-rewrite
mkdir -p /tmp/notdec-memory-fixed100-literal-bytes-rewrite/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-bytes-rewrite/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`: 9
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 9
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18
- `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
- `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1

命中 `notdec_solidity_rewrite_abi_return_literal_bytes` 的 fixed100 样例：

- `25524_19769741_8c9517a9e9_3555426790bd.ll`
- `25544_19769956_6dc86f13a0_e354b4c44525.ll`
- `25563_19770118_498bb800eb_8e79ddef1b42.ll`
- `25619_19770992_6b2850de20_21a0b617860a.ll`
- `25625_19771047_de9c01a012_1459648d68cd.ll`

## 下一步

按价值排序：

1. 继续把 storage dynamic bytes/string return 推成更具体的 ABI return rewrite marker。
2. 找多 word literal bytes / string，避免当前 literal rewrite 只覆盖一个 data word。
3. 把后端或后续 lowering 接到 `notdec_solidity_rewrite_abi_return_literal_bytes`，确认 C 输出能真正使用这个高层语义。

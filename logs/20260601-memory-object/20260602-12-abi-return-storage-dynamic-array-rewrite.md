# ABI return storage dynamic array rewrite marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

Solidity public getter 对 storage bytes/string 的路径在 `/sn640/solidity/libsolidity/codegen/ir/IRGenerator.cpp:724-729`：生成 `<ret> := <readStorage>(slot, offset)`，其中 `readStorage` 来自 `readFromStorageDynamic()`。

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2796-2821` 的 `readFromStorageDynamic()` 对 reference type 会调用 `readFromStorageReferenceType()`。后者在 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2876-2884` 对 memory array 返回 `copyArrayFromStorageToMemoryFunction(storage array, memory array)`。

storage array 到 memory array 的 helper 在 `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2611-2640` 会分配 memory array，并用 ABI encode / finalize allocation 生成 memory source。public wrapper 随后再把这个 memory source copy 到 ABI return buffer。

所以这个场景的高层语义是：ABI return 的 dynamic array source 来自 storage bytes/string decode helper。当前不能直接恢复具体 bytes 内容，但可以把 ABI return 标成 storage dynamic array return。

## 本轮目标

上一轮已有：

- `notdec_solidity_abi_return_dynamic_array_storage_source(returnBase, sourceArray, length, returnSize, kind)`

它说明 source array 来自 storage helper，但还偏 memory fact。现在新增 ABI return 级别 rewrite marker：

- `notdec_solidity_rewrite_abi_return_storage_dynamic_array(returnBase, sourceArray, length, returnSize, kind)`

这轮属于 `consumer rewrite`：marker 直接表达 ABI return 语义，后续 lowering 可以基于它处理 storage bytes/string getter，不需要重新扫描 storage helper 和 ABI return copy。

## 修改

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:37-38`
  - 增加 `NumAbiReturnStorageDynamicArrayRewrites` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:804-821`
  - 新增 `insertAbiReturnStorageDynamicArrayRewriteMarker()`，插入 `notdec_solidity_rewrite_abi_return_storage_dynamic_array`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1126-1132`
  - 在 `StorageSource` 确认后，同时插入 storage dynamic array 的 ABI return rewrite marker。
- `test/evm/solidity-patterns/manifest.json:1262-1264`
  - 给 `0450_19495071_6b6c9447e0_6344565f4b31` 增加 2 个 `notdec_solidity_rewrite_abi_return_storage_dynamic_array` oracle。

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

结果：通过，`110.61 sec`。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-storage-dynamic-rewrite
mkdir -p /tmp/notdec-memory-fixed100-storage-dynamic-rewrite/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-storage-dynamic-rewrite/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 10
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 10
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18
- `notdec_solidity_abi_return_dynamic_array_mcopy`: 2
- `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1

命中 `notdec_solidity_rewrite_abi_return_storage_dynamic_array` 的 fixed100 样例：

- `25580_19770476_5bc11a3f17_8208dc777fbf.ll`
- `25585_19770594_ae03944f62_01750e6d9f87.ll`
- `25586_19770596_9da1c11cd3_b7356f8ff0dc.ll`
- `25620_19771007_f77cc69480_fe51b87b04c6.ll`

## 下一步

按价值排序：

1. 找多 word literal bytes / string，补 `notdec_solidity_rewrite_abi_return_literal_bytes` 的多 word 形状。
2. 找 calldata decode / concat / builder 形状的 dynamic return source，避免只覆盖 storage source 和 literal source。
3. 把后续 lowering 接到 `notdec_solidity_rewrite_abi_return_storage_dynamic_array` 和 `notdec_solidity_rewrite_abi_return_literal_bytes`。

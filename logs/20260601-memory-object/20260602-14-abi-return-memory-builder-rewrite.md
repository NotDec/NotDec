# ABI return memory builder rewrite marker

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2675-2736` 的 `bytesOrStringConcatFunction()` 生成 bytes/string concat helper：

- `outPtr := allocateUnbounded()`
- `dataStart := add(outPtr, 0x20)`
- `dataEnd := encodePacked(dataStart, ...)`
- `mstore(outPtr, sub(dataEnd, dataStart))`
- `finalizeAllocation(outPtr, sub(dataEnd, outPtr))`

`/sn640/solidity/libsolidity/codegen/ir/IRGeneratorForStatements.cpp:2270-2271` 确认 `.concat` 只允许 bytes/string array type。

所以 concat / encodePacked builder 的 source array 形状是：当前函数内先写 `source[0] = length`，data 区由 packed encoder 或 copy loop 写入，最后 ABI return wrapper 再把这个 source array 复制到 return buffer。

## 问题

fixed100 中 `notdec_solidity_abi_return_dynamic_array_source` 共 21 个：

- 10 个已经是 storage source。
- 9 个已经是 literal source。
- 剩下 2 个都在 `25541_19769941_0129b94c29_ed9ec6332e1e.ll`。

检查 `25541` 的真实 IR 后，这两个 source 都不是 storage，也不是 literal。它们的共同形状是：

- source array 有当前函数内的 length write。
- return buffer 写 head offset 和 length。
- source data 通过 copy loop 复制到 return data。

这符合 Solidity concat / memory builder，而不是业务分支。

## 本轮目标

新增两个 marker：

- `notdec_solidity_abi_return_dynamic_array_memory_builder_source(returnBase, sourceArray, length, builderLength, returnSize, kind)`
- `notdec_solidity_rewrite_abi_return_memory_builder(returnBase, sourceArray, length, returnSize, copyKind, kind)`

这轮属于 `consumer rewrite`：它把未分类 dynamic ABI return source 推到 ABI return 级别的 memory builder 语义，后续 lowering 不需要再回扫 source array length write 和 return copy loop。

## 修改

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:41-44`
  - 增加 memory builder source / rewrite 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:274-282`
  - 新增 `AbiReturnDynamicArrayMemoryBuilderSource`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:659-684`
  - 新增 `findAbiReturnDynamicArrayMemoryBuilderSource()`，只识别 source array offset 0 的 length write。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:895-933`
  - 新增 `insertAbiReturnDynamicArrayMemoryBuilderSourceMarker()` 和 `insertAbiReturnMemoryBuilderRewriteMarker()`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1256-1269`
  - 在没有 storage source、也没有更具体 memory/literal source 时，插入 memory builder marker。
- `test/evm/solidity-patterns/cases/25541_19769941_0129b94c29_ed9ec6332e1e.ll`
  - 新增真实 IR oracle 输入。
- `test/evm/solidity-patterns/manifest.json:5252-5265`
  - 增加 `25541_19769941_0129b94c29_ed9ec6332e1e` case，检查 dynamic source、copy loop、memory builder source 和 memory builder rewrite 各 2 个。

## 验证

构建：

```bash
cmake --build ./build --target all -j4
```

结果：通过。

单例：

```bash
./build/bin/notdec test/evm/solidity-patterns/cases/25541_19769941_0129b94c29_ed9ec6332e1e.ll \
  -o /tmp/notdec-memory-25541-builder.ll --tr-level=3
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_source`: 2
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 2
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 2
- `notdec_solidity_rewrite_abi_return_memory_builder`: 2

manifest JSON：

```bash
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json
```

结果：通过。

Solidity patterns：

```bash
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

结果：通过，`111.27 sec`。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-memory-builder-rewrite
mkdir -p /tmp/notdec-memory-fixed100-memory-builder-rewrite/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-memory-builder-rewrite/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_source`: 21
- `notdec_solidity_abi_return_dynamic_array_storage_source`: 10
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 9
- `notdec_solidity_abi_return_dynamic_array_memory_builder_source`: 2
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 10
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 9
- `notdec_solidity_rewrite_abi_return_memory_builder`: 2

这说明 fixed100 里的 21 个 dynamic return source 现在都有更具体分类。

## 下一步

按价值排序：

1. 继续扩大 apehex 范围，找 fixed100 之外的未分类 dynamic return source。
2. 找多 word literal bytes / string，目前 fixed100 仍只有单 word literal。
3. 把后续 lowering 接到 `notdec_solidity_rewrite_abi_return_literal_bytes`、`notdec_solidity_rewrite_abi_return_storage_dynamic_array` 和 `notdec_solidity_rewrite_abi_return_memory_builder`。

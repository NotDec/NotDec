# ABI return literal dynamic array source

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮继续看 Solidity string / bytes literal 返回。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:145`，`copyLiteralToMemoryFunction()` 生成返回 memory array 指针的 helper。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`storeLiteralInMemoryFunction()` 把 literal 拆成 32 字节 word 后写入 `add(memPtr, offset)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3320`，`allocateMemoryArrayFunction()` 对动态数组会 `mstore(memPtr, length)`。
- `/sn640/solidity/libsolutil/CommonData.cpp:187`，`formatAsStringOrNumber()` 对不可直接打印的 bytes 返回 left-aligned hex word。

NotDec 当前 IR 里，literal data word 常见成两种形状：

- 直接常量 word。
- `evm_shl(const, const)`，用于把短 literal 左对齐到 32 字节 word。

## 问题

上一轮新增了 `notdec_solidity_abi_return_dynamic_array_memory_source`，能说明 dynamic return source 是同函数里构造的 memory array。但它还没有区分：

- Solidity literal helper 构造出来的 bytes/string。
- 更一般的 memory builder，例如 concat、运行时 copy、动态数据拼接。

fixed100 中当前 9 个 memory source 都是 literal 形状，其中 `25625_19771047_de9c01a012_1459648d68cd` 虽然还没进 manifest，但两个 return source 都是 length 常量加 `evm_shl(const, const)` data word。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取上一轮确认的 memory source 和已有 memory write marker，再产出更窄的 literal source marker。没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:38`
  - 新增 `NumAbiReturnDynamicArrayLiteralSources` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:630`
  - 新增 `isLiteralDataWord()`。
  - 接受常量 word，或 `evm_shl(const, const)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:642`
  - 新增 `isAbiReturnDynamicArrayLiteralSource()`。
  - 要求 source array 的 length write 是常量，data write 是 literal word 形状。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:772`
  - 新增 `insertAbiReturnDynamicArrayLiteralSourceMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_literal_source(returnBase, sourceArray, length, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:924`
  - 在已有 memory source 分支里接入 literal source marker。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 literal source oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5271`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 literal source oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5288`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 literal source oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5304`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 literal source oracle，期望 2。

## 判断

这个 marker 比 `memory_source` 更窄，只表达 Solidity literal helper 这类 source。它没有试图反推出字符串内容，也没有删除底层 `mstore/mload/copy`。后续如果要恢复具体 bytes/string 值，可以从 literal marker 再继续读取 length 和 data word。

当前没有把 `25625` 加进 manifest。原因是本轮手上稳定的 `25625` 文件是 `/tmp` 下跑完 pass 后的输出，不适合直接作为输入 oracle。它保留在 fixed100 audit 里覆盖。

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
- `notdec.evm.solidity_patterns` 通过，用时 110.67s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-source
mkdir -p /tmp/notdec-memory-fixed100-literal-source/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-source/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
- `notdec_solidity_abi_return_dynamic_array_literal_source`: 9。
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 9。
- 对照关键 ABI return markers：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21。
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18。
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2。
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1。
  - `notdec_solidity_abi_return_dynamic_array_storage_source`: 10。

## 下一轮候选

1. 从 literal source marker 继续恢复 literal payload 的 length / data word，产出更具体的 bytes/string return marker。
2. 找不是 literal 的 memory source 样例。当前 fixed100 没有，下一轮可扩大 apehex 样例或看 `25541` 这类 concat/builder。
3. 让 copy loop / mcopy / helper copy marker 读取 literal source，产出完整的 “ABI return literal bytes/string” 语义。

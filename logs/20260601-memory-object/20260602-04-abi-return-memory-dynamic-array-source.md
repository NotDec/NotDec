# ABI return memory-built dynamic array source

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮看的是 Solidity string / bytes literal 或内存构造后返回的动态数组 source。

源码位置：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:147`，`YulUtilFunctions::copyLiteralToMemoryFunction()` 生成 `copy_literal_to_memory_*()`，先调用 memory array allocation helper，再把 literal 写到 `add(memPtr, 32)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164`，`YulUtilFunctions::storeLiteralInMemoryFunction()` 按 32 字节 word 生成 `mstore(add(memPtr, offset), wordValue)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.h:83` 到 `:90` 的注释说明这两个 helper 一个返回带 length 的 memory array 指针，一个只负责写 data 区。

对应 IR 形状是：source array 先写 offset 0 的 length，再写 offset >= 32 的 data word。ABI return wrapper 后面会读 `mload(source)` 作为 length，并把 `source + 32` 的数据复制到 return buffer。

## 问题

上一轮已经能标出：

- `notdec_solidity_abi_return_dynamic_array_source`
- copy loop / mcopy / helper copy
- storage source

但 fixed100 里还有一批动态 return source 不是 storage helper，而是同函数里构造出来的 memory array。典型样例：

- `25524_19769741_8c9517a9e9_3555426790bd`：写 `memory_write(source, 0, 5)` 和 `memory_write(source, 32, ...)`，再 helper copy 到 ABI return。
- `25544_19769956_6dc86f13a0_e354b4c44525`：两个 literal source，后面走 copy loop。
- `25619_19770992_6b2850de20_21a0b617860a`：两个 literal source，后面走 mcopy。

这不是业务分支。它来自 Solidity 的 memory array / literal helper codegen。

## 修改

本轮属于 `consumer rewrite`：ABI return pass 读取已有 `notdec_solidity_memory_write` rewrite marker，并产出更具体的 ABI return dynamic source marker；没有新增 metadata。

改动文件：

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:36`
  - 新增 `NumAbiReturnDynamicArrayMemorySources` 统计。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:243`
  - 新增 `AbiReturnDynamicArrayMemorySource`，保存 return base、source array、length、length write 和 data write。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:589`
  - 新增 `findAbiReturnDynamicArrayMemorySource()`。
  - 只在已经确认的 dynamic return source 上继续检查。
  - 要求同函数内、支配 return、同一 `SourceArray` 上有 offset 0 的 length write 和 offset >= 32 的 data write。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:733`
  - 新增 `insertAbiReturnDynamicArrayMemorySourceMarker()`。
  - 插入 `notdec_solidity_abi_return_dynamic_array_memory_source(returnBase, sourceArray, length, returnSize, kind)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:879`
  - 在 `AbiReturnPass::run()` 的 dynamic source 分支里接入 memory source matcher。
- `test/evm/solidity-patterns/manifest.json:5254`
  - 给 `25619_19770992_6b2850de20_21a0b617860a` 增加 memory source oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5270`
  - 给 `25524_19769741_8c9517a9e9_3555426790bd` 增加 memory source oracle，期望 1。
- `test/evm/solidity-patterns/manifest.json:5286`
  - 给 `25544_19769956_6dc86f13a0_e354b4c44525` 增加 memory source oracle，期望 2。
- `test/evm/solidity-patterns/manifest.json:5301`
  - 给 `25563_19770118_498bb800eb_8e79ddef1b42` 增加 memory source oracle，期望 2。

## 判断

这个 marker 的语义比 `dynamic_array_source` 更具体：它说明 source array 是当前函数里由 memory writes 构造出来的，不是 storage helper 返回值。判断保守绑定到已经识别出的 ABI return source，不扫描所有 memory buffer。

暂时没有把 source data word 改写成 literal value 级别，也没有删除低层 `mstore/mload/copy`。这是为了保持当前 ABI return 语义稳定，后续再做更强 rewrite。

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
- `notdec.evm.solidity_patterns` 通过，用时 110.24s。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-memory-source
mkdir -p /tmp/notdec-memory-fixed100-memory-source/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-memory-source/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

结果：

- 输入 100 个，输出 100 个 `.ll`。
- `notdec_solidity_abi_return_dynamic_array_memory_source`: 9。
- 对照关键 ABI return markers：
  - `notdec_solidity_abi_return_dynamic_array_source`: 21。
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 18。
  - `notdec_solidity_abi_return_dynamic_array_mcopy`: 2。
  - `notdec_solidity_abi_return_dynamic_array_helper_copy`: 1。
  - `notdec_solidity_abi_return_dynamic_array_storage_source`: 10。

## 下一轮候选

1. 给 memory-built dynamic source 继续区分 literal source 和一般 memory builder source。先看 `25541`、`25625`，不要只按 `memory_write(source, 32, ...)` 猜。
2. 让 ABI return copy marker 读取 memory source marker，进一步产出“literal bytes/string return”语义。
3. 继续把 ABI return 低层 return buffer write 和 source copy 关系合并，减少后续 pass 重新扫描 `mstore/mload`。

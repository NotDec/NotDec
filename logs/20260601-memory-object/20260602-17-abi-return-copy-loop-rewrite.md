# ABI return copy loop rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

本轮看 `/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp:524` 以后的 `abiEncodingFunctionSimpleArray()`。

对普通 memory array ABI return，Solidity 生成的形状是：

- `pos := storeLength(pos, length)`。
- `baseRef := dataAreaFun(value)`。
- `srcPtr := baseRef`。
- 循环里 `elementValues := mload(srcPtr)`。
- 再 `pos := encodeToMemoryFun(elementValues, pos)`。

所以真实 IR 里的 `returnBase + 64 + index` 写入和 `sourceArray + 32 + index` 读取，是 ABI encoder 把 memory array 编码到 return buffer，不是业务分支。

## 问题

固定 100 个 apehex 样例来自：

- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260602-evm2llvm-train-batch661/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260602-evm2llvm-train-batch662/outputs`
- `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260602-evm2llvm-train-batch663/outputs`

当前 100 个 `.bc` 都能跑完。审计发现：

- 通用 `notdec_solidity_memory_consumer` 都有对应 ABI return / revert / event / external call 专用 consumer marker。
- `26499_19783384_5c97aa67ba_f1db7947fb2c` 有 2 个 `notdec_solidity_abi_return_dynamic_array_source`，也有 2 个 `notdec_solidity_abi_return_dynamic_array_copy_loop`，但没有 ABI return 级别 rewrite marker。

这说明 memory fact 已经找到了，但最后一步 consumer rewrite 没闭环。

## 实现计划

本轮属于 `consumer rewrite`。

做一个窄 marker：

- 新增 `notdec_solidity_rewrite_abi_return_dynamic_array_copy_loop(returnBase, sourceArray, length, returnSize, kind)`。
- 只在已有 `dynamic_array_source` 和纯 `dynamic_array_copy_loop`，且没有 storage source、converted copy、memory source、memory builder rewrite 时插入。
- 不删除低层 `mload/mstore/return`。

这样不会抢已有更具体的 storage / literal / memory builder / converted rewrite。

## 判断标准

- 新增真实 IR oracle 覆盖 `26499_19783384_5c97aa67ba_f1db7947fb2c`。
- `notdec.evm.solidity_patterns` 通过。
- 固定 100 样例里 dynamic source 数量和 ABI return dynamic rewrite 数量对齐。

## 实现

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:31` 增加 `NumAbiReturnDynamicArrayCopyLoopRewrites`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:921` 增加 `insertAbiReturnDynamicArrayCopyLoopRewriteMarker()`，插入 `notdec_solidity_rewrite_abi_return_dynamic_array_copy_loop`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1487` 在 builder source 不成立、且已有纯 copy loop、没有 converted copy loop 时插入新 rewrite。
- `test/evm/solidity-patterns/cases/26499_19783384_5c97aa67ba_f1db7947fb2c.ll` 增加真实 IR case。
- `test/evm/solidity-patterns/manifest.json:5375` 增加 oracle：
  - `notdec_solidity_abi_return_dynamic_array_source`: 1
  - `notdec_solidity_abi_return_dynamic_array_copy_loop`: 1
  - `notdec_solidity_rewrite_abi_return_dynamic_array_copy_loop`: 1

这个 rewrite 不推断元素类型，只表达 ABI return 正在用纯 memory copy loop 编码动态 array。更具体的 storage / literal / memory builder / converted rewrite 仍然优先。

## 验证

已跑：

- `cmake --build ./build --target all -j4`
- `./build/bin/notdec test/evm/solidity-patterns/cases/26499_19783384_5c97aa67ba_f1db7947fb2c.ll -o /tmp/notdec-memory-26499-tr0.ll --tr-level=0`
- `./build/bin/notdec test/evm/solidity-patterns/cases/26499_19783384_5c97aa67ba_f1db7947fb2c.ll -o /tmp/notdec-memory-26499-tr3.ll --tr-level=3`
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 99 个 case 全部通过。
  - 总耗时 116.73 秒。

固定 100 audit，输入来自 batch661-663 的前 100 个 `.bc`，`--tr-level=3`：

- runner 失败：0。
- `notdec_solidity_abi_return_dynamic_array_source`: 21。
- `notdec_solidity_abi_return_dynamic_array_copy_loop`: 21。
- `notdec_solidity_rewrite_abi_return_storage_dynamic_array`: 12。
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 8。
- `notdec_solidity_rewrite_abi_return_dynamic_array_copy_loop`: 1。
- dynamic source > dynamic rewrite 的文件：0。
- generic memory consumer > 专用 consumer 的文件：0。

基线 suite 本轮前耗时 116.77 秒，本轮后 116.73 秒；没有看到性能下降。

## 复杂度和维护

- 实现效果：8/10。补上固定 100 里最后一个明显 dynamic ABI return source 缺口。
- 理解成本：3/10。只新增一个和现有 ABI return rewrite 同形的 marker。
- 后期维护成本：3/10。条件很窄，不影响已有更具体 rewrite。

更激进的方案是把所有 copy loop 都直接归并到 memory dynamic array rewrite，但会混淆“source memory array 已知”和“只确认 ABI encoder copy loop”两件事。本轮保留独立 marker，更保守。

## 下一轮候选

1. 扩大到最近 200 个 apehex，继续找 dynamic source > dynamic rewrite 的样例。
2. 审计 `notdec_solidity_memory_allocation` 与 ABI return / external call 专用 allocation 的差值，优先看同 base reload 或跨块 finalize。
3. 后续 lowering 接入 `notdec_solidity_rewrite_abi_return_dynamic_array_copy_loop`，让 C 输出直接消费高层 marker。

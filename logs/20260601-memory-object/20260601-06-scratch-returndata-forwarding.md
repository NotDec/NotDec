# Scratch returndata forwarding rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

根据 100-case audit 的最高频缺口，建模 scratch returndata forwarding：

- `returndatacopy(0, 0, size)`。
- `revert(0, size)` / `return(0, size)`，但跳过 `size = 0` 的普通 empty revert。

这不是 free-memory allocation。marker 使用 `base = 0, offset = 0` 表示 scratch 区。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp::forwardingRevertFunction()`：

- 常规 helper 是 `pos := allocate_unbounded(); returndatacopy(pos, 0, returndatasize()); revert(pos, returndatasize())`。
- 但真实 IR 和 proxy-like fallback 里还常见 scratch 形状：`returndatacopy(0, 0, returndatasize())` 后接 `revert(0, returndatasize())` 或 `return(0, returndatasize())`。

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp::returnDataSelectorFunction()`：

- 也会生成 `returndatacopy(0, 0, 4)`，再 `mload(0)` 读取 selector。

判断：`returndatacopy(0, 0, size)` 是 compiler scratch buffer 写入。只有后面被 `return/revert(0, size)` 消费时才建 consumer role，避免把 selector scratch 误当成 forwarding consumer。

## 计划

1. 给 copy write 增加 scratch returndata kind。
2. 对 `returndatacopy(..., dst=0, src=0, size)` 插入 `notdec_solidity_memory_copy_write(0, 0, 0, size, kind)`。
3. 对 `return/revert(..., offset=0, size!=0)` 插入 `notdec_solidity_memory_consumer(0, size, kind)`。
4. 在 `0011_multi_public` 上加 oracle。这个 case 有两个 returndata bubble，正好对应 scratch forwarding。

## 风险

- `revert(0, 36)` 也可能是 Panic/custom error buffer，当前会被标成 scratch consumer。它不是 allocation，但确实是低地址 scratch buffer。后续需要按 selector 写入再细分。
- `returndatacopy(0, 0, 4)` 只是读 selector，不应该当 return/revert consumer；本轮只插 copy marker，不插 consumer。
- 这个逻辑不试图恢复 allocation，也不删除原始低层 IR。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0011_multi_public.ll -o /tmp/notdec-memory-scratch-0011.ll --tr-level=0`。
  - 输出 2 个 `notdec_solidity_memory_copy_write(... kind 4)`，对应两个 returndata bubble。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-scratch`。
  - 100/100 跑完，总耗时 83.15s。
  - `notdec_solidity_memory_copy_write` 从 197 增加到 694。
  - 新增 `scratch_returndata` copy 497 个，覆盖上一轮 audit 的最高频缺口。
  - `notdec_solidity_memory_consumer` 从 4436 增加到 5736。

## 实现记录

- `include/notdec/Passes/evm/MemoryBufferAnalysis.h:18` 给 `MemoryWriteKind` 增加 `ScratchReturndataCopy = 4`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:43` 增加 `isZero()` helper。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:263` 识别 `returndatacopy(..., dst=0, src=0, size)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:265` 插入 scratch copy write fact，最终写出 `notdec_solidity_memory_copy_write(0, 0, 0, size, 4)`。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:292` 对 `return(0, size!=0)` 插入 scratch consumer。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:307` 对 `revert(0, size!=0)` 插入 scratch consumer。
- `test/run_evm_solidity_patterns_suite.py:318` 给 copy write kind oracle 增加 `scratch_returndata`。
- `test/evm/solidity-patterns/manifest.json:45` 更新 `checked_bounds_arithmetic_01`，覆盖 `revert(0,36)` scratch consumer。
- `test/evm/solidity-patterns/manifest.json:981` 更新 `0011_multi_public`，覆盖两个 returndata bubble 的 scratch copy / consumer。

## 当前限制

- `revert(0,36)` 现在会标成 scratch consumer。它通常是 Panic/custom error 的低地址 scratch buffer，不是 forwarding returndata；后续应该结合 selector write 再细分。
- `returndatacopy(0,0,4)` 只会产生 scratch copy，不会单独产生 consumer；这是 selector read helper 的合理处理。
- scratch consumer 还没有和具体前驱 copy 做 CFG 配对，只按 call 参数保守插 marker。
- 仍然不删除原始 `returndatacopy` / `revert` / `return`。

## 评分

- 实现效果：8/10。覆盖了 100-case audit 里最大缺口，新增 497 个 scratch returndata copy marker。
- 理解成本：5/10。复用现有 marker，`base=0` 的约定需要文档说明，但没有新增复杂结构。
- 维护成本：6/10。后续需要把 scratch panic/custom error 和 forwarding returndata 进一步区分。

## 下一轮候选

1. 区分 scratch panic/custom error 和 forwarding returndata：结合 `mstore(0, selector)`、`returndatacopy(0,0,size)`、`revert(0,size)`。
2. 支持 helper-return base，覆盖 `calldatacopy` 的 other 高频形状。
3. 接 `evm_codecopy`，覆盖 literal data copy。
4. 接 `evm_mstore8` / `evm_mcopy`，补齐低层 memory 写入 surface。
5. 让 `SolidityRevertPass` 读取 memory facts，先迁 returndata bubble / panic scratch 的重复判断。

# Apehex 100-case memory marker audit

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

完成条件包括：在 `/sn640/NotDecChainExp/evm2llvm_apehex_pilot` 上选固定的 100 个用例，不断改进效果，直到没有明显的问题。

## 本轮范围

开始固定 100 个 apehex 用例的 memory marker audit。这里只统计当前 rewrite surface 的覆盖情况，不改 pass 行为。

固定样例选择规则：

- 目录：
  - `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch639/outputs`
  - `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch640/outputs`
  - `/sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch641/outputs`
- 只取 `.ll`。
- 按完整路径排序后取前 100 个。
- 第一个：`25521_19769720_5f0f4a67ae_61ca7accc9c3.ll`。
- 最后一个：`25629_19771091_03130f3ea6_7ad93f027ac5.ll`。

## 执行命令

每个样例执行：

```bash
./build/bin/notdec <input.ll> -o /tmp/notdec-memory-audit-20260601/outputs/<name>.ll --tr-level=0
```

统计脚本输出放在：

- `/tmp/notdec-memory-audit-20260601/summary.csv`
- `/tmp/notdec-memory-audit-20260601/logs/`
- `/tmp/notdec-memory-audit-20260601/outputs/`

## 结果

- 100/100 样例能跑完。
- 总耗时：83.03s。
- memory marker 总数：
  - `notdec_solidity_memory_allocation`：1041。
  - `notdec_solidity_memory_write`：8902。
  - `notdec_solidity_memory_copy_write`：197。
  - `notdec_solidity_memory_consumer`：4436。
- consumer kind：
  - return：1412。
  - revert：1480。
  - event_log：439。
  - external_call_input：558。
  - external_call_output：547。
- copy write kind：
  - calldata：125。
  - returndata：72。

## 主要观察

当前已经覆盖了高频 free-memory base 的 return / revert / event / external call role。外部调用 input/output 同 base 也能被同时标出来。

明显缺口主要是：

1. `returndatacopy` 到常量 0：
   - 统计里约 497 个。
   - 例子：`returndatacopy(0, 0, returndatasize())` 后接 `revert(0, returndatasize())` 或直接 return。
   - 这不是 free-memory allocation，但它是 compiler forwarding returndata / revert 的重要 scratch buffer。
2. `calldatacopy` 到非当前识别 base：
   - `other` 分类约 85 个。
   - 常见形状是目的地址来自前面函数返回值、私有 helper 返回的 memory pointer，或者 `base + symbolic`。
3. `base + symbolic` copy：
   - 数量少，但在动态 bytes/string payload 里会出现。
   - 当前只接受 `base + const`，所以会漏。
4. `evm_codecopy`、`evm_mstore8`、`evm_mcopy`：
   - raw 统计分别约 144、107、116。
   - 目前还没有进入 memory write surface。

## 判断

下一轮不应该继续只加 consumer kind。现在更明显的问题是 scratch / forwarding buffer 和非简单 base 的 copy write。

`returndatacopy(0, 0, returndatasize())` 要谨慎处理：它不是 allocation buffer，但如果后面立刻 `revert(0, returndatasize())`，这是 Solidity forwarding revert helper，不是业务分支。可以作为 scratch buffer role 单独建模，而不是硬塞进 free-memory allocation。

## 下一轮候选

1. 建模 scratch returndata forwarding：`returndatacopy(0, 0, size)` + `revert(0, size)` / `return(0, size)`，插入 scratch copy/consumer marker。
2. 支持 helper-return base：把私有 helper 返回的 memory pointer 当作 base 候选，覆盖 `calldatacopy` 的 other 高频形状。
3. 支持 `base + symbolic` copy offset，但必须先限制在同一 base 的 ABI encode helper 内，避免误绑。
4. 接 `evm_codecopy`，用于 literal bytes/string/code data copy。
5. 接 `evm_mstore8` / `evm_mcopy`，补齐低层 memory 写入 surface。

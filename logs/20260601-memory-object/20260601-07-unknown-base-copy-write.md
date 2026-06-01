# Unknown-base copy write rewrite

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## 本轮范围

补齐 `calldatacopy` / `returndatacopy` 中目的地址不能还原到 free-memory base 的情况。

前几轮只在 `dst = base` 或 `dst = base + const` 时插 marker。100-case audit 里还有不少 `calldatacopy` 目的地址来自 ABI helper 参数、helper 返回值、多层 add 或 PHI。它们仍然是确定的 memory copy write，只是当前还不能绑定到 allocation。

本轮策略：

- 如果能绑定到 free-memory base，沿用原 marker。
- 如果不能绑定，但是 `evm_calldatacopy` / `evm_returndatacopy`，插入 unknown-base copy marker：
  - `notdec_solidity_memory_copy_write(dst, 0, src, size, kind)`。
- 不把 unknown-base copy 当 allocation，也不插 consumer。

## Solidity codegen 回顾

`/sn640/solidity/libsolidity/codegen/ABIFunctions.cpp`：

- calldata dynamic array / bytes / string encode 到 memory 时，会生成 copy helper：
  - `copyFun(start, pos, length)`。
  - 其中 `pos` 可能来自 ABI encoder 的参数，不一定是本函数内直接的 `mload(0x40)`。

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp`：

- zero memory helper 里会生成 `calldatacopy(dataStart, calldatasize(), dataSizeInBytes)`。
- `dataStart` 是 helper 参数，也不一定能追到当前函数内 free-memory base。

判断：这些 copy 是 compiler-generated memory write。不能因为 base 不完整就完全不写 IR rewrite surface；但也不能伪装成 allocation-relative write。

## 计划

1. 在 copy write 分支里保留现有 free-memory base 匹配。
2. 如果没有匹配到 base，插入 fallback copy write，base 字段直接用 `dst`，offset 为 0。
3. 给 runner 不新增字段，继续按 calldata / returndata kind 统计。
4. 选一个已有 manifest case 加 oracle，覆盖 fallback copy write。

## 风险

- fallback marker 的 base 字段是实际 dst，不是 allocation base。后续 pass 消费时必须看它是否能和 allocation 绑定，不能直接当 ABI object base。
- 对所有 unmatched copy 都插 marker，数量会增加；这是预期的 rewrite surface，不是 metadata。
- PHI / helper-return base 后续还需要更精确的 interprocedural / helper summary。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json`：通过。
- `git diff --check`：通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`：通过，1/1。
- 手工 smoke：
  - `./build/bin/notdec test/evm/solidity-patterns/cases/0002_delegatecall_no_nonpayable.ll -o /tmp/notdec-memory-unknown-copy-0002.ll --tr-level=0`。
  - 输出 2 个 copy write：1 个 calldata fallback copy，1 个 scratch returndata copy。
- 固定 100 个 apehex 样例 rerun：
  - 输出目录：`/tmp/notdec-memory-audit-20260601-unknown-copy`。
  - 100/100 跑完，总耗时 83.57s。
  - `notdec_solidity_memory_copy_write` 从 694 增加到 723。
  - calldata copy 从 125 增加到 151。
  - returndata copy 从 72 增加到 75。

## 实现记录

- `src/Passes/evm/MemoryBufferAnalysis.cpp:271` 在 copy write 分支记录是否匹配到 free-memory base。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:286` 对没有匹配到 base 的 copy 插入 fallback fact。
- `src/Passes/evm/MemoryBufferAnalysis.cpp:290` fallback marker 使用 `dst` 作为 base，offset 固定为 0。
- `test/evm/solidity-patterns/manifest.json:1037` 给 `0002_delegatecall_no_nonpayable` 增加 memory rewrite oracle。
- `test/evm/solidity-patterns/manifest.json:1045` 检查 calldata fallback copy 和 scratch returndata copy。

## 当前限制

- fallback base 是实际目的地址，不代表 allocation base。
- 这只能保证 copy write 进入 IR rewrite surface，还不能回答“这个 copy 属于哪个 ABI object”。
- 对 PHI / helper-return base 的精确归属仍需 helper summary 或跨函数事实。

## 评分

- 实现效果：5/10。补上了所有 unmatched copy 的 rewrite surface，但 100-case 增量只有 29 个，说明大缺口已经被 scratch returndata 那轮解决。
- 理解成本：4/10。规则简单，marker 参数含义需要注意：fallback base 是 dst。
- 维护成本：5/10。后续消费者必须区分 allocation-relative copy 和 unknown-base copy。

## 下一轮候选

1. 接 `evm_codecopy`，100-case 里 raw 约 144，通常对应 literal/code data copy。
2. 接 `evm_mstore8` / `evm_mcopy`，补齐低层 memory write surface。
3. 给 helper-return base 做 summary，而不是只用 fallback base。
4. 让 `SolidityRevertPass` 读取 memory facts，先迁 scratch returndata / panic 的判断。

# 原始 prompt

> <goal_context>
> Continue working toward the active thread goal.
> 持续迭代完善 CheckedBoundsPass，但每轮只能基于真实证据扩展，不做猜测式放宽。完成一轮可验证的改进
> </goal_context>

# 本轮问题

本轮重新统计：

- pattern suite `/tmp/notdec-patterns-cb-round75-suite`：90 个 `.ll`，775 个 checked-bounds，774 个 semantic marker，774 个 CFG rewrite，1 个 skip。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- rewrite suite `/tmp/notdec-rewrite-cb-round75-suite`：65 个 `.ll`，626 个 checked-bounds，624 个 semantic marker，624 个 CFG rewrite，2 个 skip。
  - `0046_19493140_f89daa079d_8aad1d5bf751/out.ll`：`unsupported_condition:1`，`panic_resource_error:1`。
  - `0334_19494307_668d201319_1354ce2e324d/out.ll`：`unrecognized_operands:1`，`panic_assert:1`。
- apehex `20260601-evm2llvm-train-batch634/outputs`：35 个 `.ll`，checked-bounds 为 0。

0046 的 skipped 0x41 guard：

```llvm
%evm.add = add i256 %_0x7c_0x2, 31
%evm.and11 = and i256 %evm.add, -32
%evm.add12 = add i256 %evm.and11, 128
%evm.gt13 = icmp ugt i256 %evm.add12, 18446744073709551615
%evm.lt = icmp ugt i256 %evm.add, -129
%evm.or2 = or i1 %evm.lt, %evm.gt13
br i1 %evm.or2, label %panic, label %success
```

success block 写回：

```llvm
call void @evm_mstore(ptr %mem, i256 64, i256 %evm.add12)
```

Solidity codegen 依据：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3261` 的 `finalize_allocation(memPtr, size)`：
  - `newFreePtr := add(memPtr, roundUp(size))`
  - `if or(gt(newFreePtr, 0xffffffffffffffff), lt(newFreePtr, memPtr)) { panic(0x41) }`
  - `mstore(64, newFreePtr)`
- 0046 这个真实 IR 是 `memPtr = 128` 的首次 free pointer finalize；`%evm.add` 是 `size + 31`，`%evm.and11` 是 `roundUp(size)`，`%evm.add12` 是 `128 + roundUp(size)`。

# 选择

做 1 个窄 matcher：只接受 `memPtr=128` 的初始 free pointer finalize 形状，并且要求 success block 写回 free memory pointer slot 64。

不做：

- 不把任意 `size + 31 > -129` 当作 allocation guard。
- 不支持任意常量 memPtr。
- 不 rewrite `panic_assert` / panic 0x01。

# 修改

- `src/Passes/evm/SolidityPatterns.cpp:4007` 新增 `isRoundedMemoryAllocationSizeFromBase`，确认 size 是从同一个 `size + 31` roundUp 出来的。
- `src/Passes/evm/SolidityPatterns.cpp:5123` 在 `matchMemoryAllocationPointerBounds` 中记录初始 free pointer no-wrap 的 rounded base。
- `src/Passes/evm/SolidityPatterns.cpp:5152` 只在 `matchWrappingAddStrictUpper(...) == 128` 时接受初始 free pointer 128。
- `src/Passes/evm/SolidityPatterns.cpp:5276` 要求最终 `Size` 是同一 rounded base 的 roundUp 结果。
- `test/evm/solidity-rewrite/manifest.json:75` 更新 0046 oracle：`checked_bounds_skip_total 1 -> 0`，semantic marker 和 CFG rewrite `1 -> 2`。

# 验证

- `cmake --build ./build --target all`：通过。
- 单 case 0046：
  - `./build/bin/notdec test/evm/solidity-rewrite/cases/0046_19493140_f89daa079d_8aad1d5bf751.ll -o /tmp/notdec-cb-round76-0046-direct/out.ll --tr-level=0`：通过。
  - `llvm-22.1.0.obj/bin/llvm-as /tmp/notdec-cb-round76-0046-direct/out.ll -o /tmp/notdec-cb-round76-0046-direct/out.bc`：通过。
  - `python3 scripts/audit-checked-bounds.py /tmp/notdec-cb-round76-0046-direct/out.ll --list-skips --fail-on-mismatch`：2 个 `memory_allocation_pointer_bounds`，0 skip，2 个 CFG rewrite。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round76-suite`：90 passed，0 failed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-rewrite-cb-round76-suite`：65 passed，0 failed。
- `python3 scripts/audit-checked-bounds.py /sn640/NotDecChainExp/evm2llvm_apehex_pilot/20260601-evm2llvm-train-batch634/outputs --list-skips --fail-on-mismatch --csv /tmp/notdec-cb-round76-apehex634-post.csv --csv-by-file /tmp/notdec-cb-round76-apehex634-post-nonzero.csv --csv-by-file-nonzero`：35 个 `.ll`，checked-bounds 为 0。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed，179.70 秒。

# 结果

- pattern suite：775 个 checked-bounds，774 个已 rewrite，1 个保留 skip。
- rewrite suite：626 个 checked-bounds，625 个已 rewrite，1 个保留 skip。
- rewrite suite 的 `panic_resource_error` skip 清零。
- 剩余 skip 只有 `0334_19494307_668d201319_1354ce2e324d` 的 `panic_assert` / `unrecognized_operands`。

# 评分

- 实现效果：8/10。清掉真实 rewrite suite 中唯一 0x41 skip，并保留强 oracle。
- 复杂度：3/10。新增一个小 helper 和一个窄路径，复用现有 marker。
- 维护成本：3/10。限制在初始 free pointer 128 和 `mstore(64,newPtr)`，后续如果有更多真实形状再单独扩展。

更好的方案：如果后续出现更多 finalizeAllocation 变体，可以把 `memPtr` 和 rounded size 的关系抽成更统一的小 matcher；当前只有 1 个真实 case，不提前做。

# 下一轮候选

按真实 case 数量、语义确定性、rewrite 风险、测试成本排序：

1. 继续扫 apehex 新 batch 的 checked-bounds 命中。真实 case 数量可能增加；语义确定性取决于命中形状；rewrite 风险未知；测试成本低。
2. 对 `panic_assert` 0x01 做 Solidity codegen 和业务 assert 区分。当前真实 case 1 个；语义确定性低；rewrite 风险高；测试成本中等。
3. 审计 pattern/rewrite manifest 中是否还有 checked-bounds 弱 oracle。真实 case 数量中等；语义确定；rewrite 风险低；测试成本低。

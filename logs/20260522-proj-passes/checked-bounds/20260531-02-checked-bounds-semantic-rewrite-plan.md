# checked-bounds 具体语义识别和 rewrite 计划

## 原始需求

根据 docs/evm/feature-development-guideline.md 里的规范，实现识别和rewrite bound check (docs/evm/entry-control/checked-bounds.md )的Pass。目前有一个CheckedBoundsPass，在它的基础上改进并进一步完善。实现前调研分析solidity生成相关字节码逻辑的相关源码部分作为参考。

功能上：

1. Pass 识别出具体语义：
    - checked arithmetic：区分 add/sub/mul/div/mod，能记录参与检查的关键 value；不能识别操作数时要有 skipped reason
    - array bounds：区分 memory/calldata/storage，能记录 index/length 或明确说明无法恢复
    - Solidity codegen 调研也要分析所有这些情况。

2. 实现完整的rewrite：
  - 仅打标记而不rewrite的方式仅能用于内部测试用例或者debug，对外的接口要直接做到rewrite，以正确的rewrite为目标。不要留下原始的操作的同时还有额外标记，这样语义冗余。通过增加测试用例并不断调整算法，是可以做到rewrite足够保守的同时在数据集上的效果也很好的

判断是否完成前需要检查：
1. 按照docs/evm/feature-development-guideline.md 里的规范实现，初步实现后同时完成测试用例并修复实现中存在的bug或问题。
2. 并针对不同的bound check的细分情况，都创建至少一个Solidity-generated case测试。并增加对现有真实 IR case里已有的这种情况的oracle以及正确处理。
3. 也可以根据需要增加额外的测试用例，但是尽量不要直接生成IR，从数据集中找真实的bytecode生成，或者使用solc编译得到的bytecode生成。

## 背景

上一轮 `CheckedBoundsPass` 只做到 Panic code 分类和专用 marker：

- 能区分 `0x11`、`0x12`、`0x32` 等 Panic code。
- 不能区分 checked add/sub/mul/div/mod。
- 不能区分 memory/calldata/storage array bounds。
- 没有改 CFG，低层 guard 和 panic block 仍留在可达控制流里。

这不满足当前文档开头强调的 rewrite 要求。本轮目标是把稳定形状真正改写成高层语义入口，并把低层 failure edge 从正常输出里拿掉。

## Solidity codegen 依据

已确认的 Solidity 0.8.26 生成规则：

- `YulUtilFunctions::overflowCheckedIntAddFunction`
  - `uint256`: `sum := add(x, y); if gt(x, sum) { panic(0x11) }`
  - 小整数：`if gt(sum, maxValue) { panic(0x11) }`
- `YulUtilFunctions::overflowCheckedIntSubFunction`
  - `uint256`: `diff := sub(x, y); if gt(diff, x) { panic(0x11) }`
  - 小整数：`if gt(diff, maxValue) { panic(0x11) }`
- `YulUtilFunctions::overflowCheckedIntMulFunction`
  - 大整数：`product := mul(x, y); if x != 0 && y != product / x { panic(0x11) }`
  - 小整数：`if product != product_raw { panic(0x11) }`
- `YulUtilFunctions::overflowCheckedIntDivFunction`
  - `if iszero(y) { panic(0x12) }`
  - signed min / -1 也会 `panic(0x11)`。
- `YulUtilFunctions::intModFunction`
  - `if iszero(y) { panic(0x12) }`
- `YulUtilFunctions::memoryArrayIndexAccessFunction`
  - `if iszero(lt(index, arrayLen(baseRef))) { panic(0x32) }`
- `YulUtilFunctions::calldataArrayIndexAccessFunction`
  - `if iszero(lt(index, length)) { panic(0x32) }`
- `YulUtilFunctions::storageArrayIndexAccessFunction`
  - `let arrayLength := arrayLen(array); if iszero(lt(index, arrayLength)) { panic(0x32) }`
- `YulUtilFunctions::panicFunction`
  - `mstore(0, selector); mstore(4, code); revert(0, 0x24)`

## 当前稳定支持范围

先支持 evm2llvm 经过基础优化后的常见直接形状：

- 条件分支的一个 successor 是 Panic block，另一个是正常 successor。
- Panic block 是 `Panic(uint256)`，且能读到 code。
- guard 条件能反推出 success 条件。
- checked add/sub/div/mod 和 memory array bounds 先要求操作数能从当前 function 的 use-def 找到。
- mul 和 calldata/storage array bounds 如果本轮还不能稳定恢复 operands，也必须给 skipped reason，不把它们伪装成已 rewrite。

## rewrite 路线

对已证明安全的 guard：

1. 在 guard block terminator 前插入高层 helper。
2. 把条件分支改成无条件跳到 success block。
3. 不在 panic block 上打对外语义 marker。
4. 暂时不强删 panic block；如果没有 predecessor，后续 cleanup 可以删。对外控制流不再走低层 panic 分支。

建议 helper 形状：

```text
notdec_solidity_rewrite_checked_add(lhs, rhs, result)
notdec_solidity_rewrite_checked_sub(lhs, rhs, result)
notdec_solidity_rewrite_checked_div(lhs, rhs)
notdec_solidity_rewrite_checked_mod(lhs, rhs)
notdec_solidity_rewrite_array_bounds_memory(index, length)
notdec_solidity_rewrite_array_bounds_calldata(index, length)
notdec_solidity_rewrite_array_bounds_storage(index, length)
notdec_solidity_rewrite_checked_bounds_skip(code, reason)
```

metadata 只作为测试锚点和 skipped reason，不作为对外完成状态。

## 测试计划

- 扩展 Solidity-generated case：
  - add/sub/mul/div/mod 各至少一个。
  - memory/calldata/storage array bounds 各至少一个。
- manifest oracle 检查：
  - 具体 helper 数量。
  - helper 参数能稳定检查时检查参数对或参数三元组。
  - skipped reason 数量。
  - CFG rewrite 数量。
- 真实 IR case：
  - 先复用已有 `0651...`、`0648...` 等 Panic 丰富 case，补具体语义 oracle。
  - 只把能确认的类别写强 oracle；不能确认的写 skipped reason。

## 风险

- evm2llvm IR 会有很多 ABI decode / memory allocation 的 Panic，不都是源码层 checked/bounds。
- shared panic block 可能有多个 predecessor，只改 guard edge，不直接删 block。
- PHI incoming 可能依赖 failure edge。只在 guard block 的 failure successor 是 panic/unreachable 且 success edge 不需要 failure value 时 rewrite。
- 完整识别 mul、storage array bounds 可能需要更强 dataflow；本轮不能用宽松猜测硬拆。

## 判断标准

- pattern suite 能检查具体语义，不只检查 `notdec.solidity.checked_bounds` 总数。
- rewrite suite 或等价 oracle 能证明 guard branch 已改成 success-only。
- Solidity-generated case 覆盖 add/sub/mul/div/mod 和 memory/calldata/storage array bounds。
- 真实 IR case 对已有命中的细分类别有 oracle。
- 输出 `.ll` 全部通过项目 LLVM 22 `llvm-as`。
- 记录 suite 时间和 fortune 同口径烟测。

## 实现记录

本次实现范围：

- `src/Passes/evm/SolidityPatterns.cpp:104` 扩展 `CheckedBoundsMatch`，保存具体 kind、skip reason、guard branch、success/failure block、panic code 和 operands。
- `src/Passes/evm/SolidityPatterns.cpp:1458` 新增 guard 条件归一化，只剥离 bool zext / eq-ne 0 形状，避免把除零检查误归一化掉。
- `src/Passes/evm/SolidityPatterns.cpp:1501` 新增 panic revert 线性路径查找，只穿过无 PHI 的无条件跳转，保持 CFG 判断保守。
- `src/Passes/evm/SolidityPatterns.cpp:1605`、`src/Passes/evm/SolidityPatterns.cpp:1631`、`src/Passes/evm/SolidityPatterns.cpp:1667`、`src/Passes/evm/SolidityPatterns.cpp:1747` 分别实现 checked sub、checked mul、checked add/div/mod、array bounds 的具体语义识别。
- `src/Passes/evm/SolidityPatterns.cpp:1873` 新增对外 helper 名称，输出 `notdec_solidity_rewrite_checked_add/sub/mul/div/mod` 和 `notdec_solidity_rewrite_array_bounds_memory/calldata/storage`。
- `src/Passes/evm/SolidityPatterns.cpp:1901` 加 DominatorTree 检查；operand 不支配 guard 时降级为 `operand_not_available_at_guard`，不插非法 marker、不改 CFG。
- `src/Passes/evm/SolidityPatterns.cpp:1928`、`src/Passes/evm/SolidityPatterns.cpp:2000`、`src/Passes/evm/SolidityPatterns.cpp:2035` 分别插入 semantic marker、skip marker、metadata，并把已 rewrite 的 guard 改成 `br label %success`。
- `src/Passes/evm/SolidityPatterns.cpp:2320` 在 `CheckedBoundsPass::run` 中统一收集 guard match，先做 dominance gate，再写 metadata / marker / CFG rewrite。

测试和 oracle：

- 新增 Solidity-generated 源码和 IR：
  - `test/evm/solidity-patterns/solidity/checked_bounds_sub_01.sol`
  - `test/evm/solidity-patterns/solidity/checked_bounds_mul_01.sol`
  - `test/evm/solidity-patterns/solidity/checked_bounds_mod_01.sol`
  - `test/evm/solidity-patterns/solidity/checked_bounds_calldata_array_01.sol`
  - `test/evm/solidity-patterns/solidity/checked_bounds_storage_array_01.sol`
  - 对应 `test/evm/solidity-patterns/cases/*.ll`
- `test/run_evm_solidity_patterns_suite.py:157` 新增 checked-bounds CFG rewrite 计数。
- `test/run_evm_solidity_patterns_suite.py:490` 起调整 checked-bounds oracle：不再要求旧的通用 rewrite marker / hidden marker，改为检查具体 semantic marker、skip reason 和 CFG rewrite。
- `test/evm/solidity-patterns/manifest.json` 更新 checked-bounds 专项 case，并给 `0651_19497235_18ca6cbfa7_017843325734`、`0648_19497230_998d150449_ea367f7fc1ac` 两个真实 IR case 增加具体 helper、skip reason、CFG rewrite oracle。
- 旧真实 case 里 `notdec.solidity.checked_bounds` 计数已从“panic 候选”调整为“已确认 guard”，避免继续把 metadata-only 候选当作完成状态。

验证结果：

- `cmake --build ./build --target all -j4` 通过。
- `python3 -m py_compile test/run_evm_solidity_patterns_suite.py` 通过。
- `python3 -m json.tool test/evm/solidity-patterns/manifest.json >/dev/null` 通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-run3` 通过，78/78。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure` 通过，2/2；`notdec.evm.solidity_patterns` 87.32s，`notdec.evm.solidity_rewrite` 81.33s。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds.c --tr-level=2` 通过，`elapsed=16.58`。这次改动只影响 EVM Solidity pattern pass，fortune 主要作为主 wasm 路径烟测。

实现效果评分：

- 效果：8/10。add/sub/mul/div/mod 和 memory/calldata/storage 都有 Solidity-generated case 和 rewrite oracle；真实 case 里 operand 不可安全使用时会明确 skip。
- 复杂度：5/10。matcher 比 panic code 分类明显复杂，但仍集中在 `CheckedBoundsMatch` 和几个小 matcher，没有扩散到其他 pass。
- 维护成本：4/10。后续扩展 signed overflow、小整数 overflow、更多 array length dataflow 时，可以继续补 matcher 和 oracle，不需要改测试框架主结构。

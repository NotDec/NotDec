# checked-bounds Panic 分类和 rewrite marker

## 原始需求

根据 docs/evm/feature-development-guideline.md 里的规范，实现识别和rewrite bound check (docs/evm/entry-control/checked-bounds.md )的Pass。目前有一个CheckedBoundsPass，在它的基础上改进并进一步完善。实现前调研分析solidity生成相关字节码逻辑的相关源码部分作为参考。
判断是否完成前需要检查：
1. 按照docs/evm/feature-development-guideline.md 里的规范实现，初步实现后同时完成测试用例并修复实现中存在的bug或问题。
2. 并针对不同的bound check的细分情况，都创建至少一个Solidity-generated case测试。并增加对现有真实 IR case里已有的这种情况的oracle以及正确处理。
3. 也可以根据需要增加额外的测试用例，但是尽量不要直接生成IR，从数据集中找真实的bytecode生成，或者使用solc编译得到的bytecode生成。

## Solidity 依据

查了 `/sn640/solidity` 的 codegen：

- `libsolutil/ErrorCodes.h:25` 定义稳定 `PanicCode`：`0x11` 算术溢出/下溢，`0x12` 除零，`0x21` enum 转换，`0x22` storage 编码，`0x31` 空数组 pop，`0x32` 数组越界，`0x41` 资源错误，`0x51` 无效 internal function。
- `libsolidity/codegen/YulUtilFunctions.cpp:737`、`:859`、`:939` 的 checked add/div/sub 都调用 `panicFunction(PanicCode::UnderOverflow/DivisionByZero)`。
- `libsolidity/codegen/YulUtilFunctions.cpp:2429`、`:2500` 的 memory/calldata array index access 形状是 `if iszero(lt(index, length)) { panic(ArrayOutOfBounds) }`。
- `libsolidity/codegen/YulUtilFunctions.cpp:4638` 的 `panicFunction` 统一生成 `mstore(0, Panic selector); mstore(4, code); revert(0, 0x24)`。

因此本轮按 Panic code 分类，不猜复杂 CFG 来源。empty revert 仍只保守标 `empty_revert_guard_candidate`。

## 修改

- `include/notdec/Passes/evm/SolidityPatterns.h:57` 更新 `CheckedBoundsPass` 注释，说明 Panic 按 code 分类，empty revert 仍保守。
- `src/Passes/evm/SolidityPatterns.cpp:101` 新增 `CheckedBoundsMatch`，把 guard branch、panic revert、panic code 放在一个结构里。
- `src/Passes/evm/SolidityPatterns.cpp:1421` 新增 `getCheckedBoundsKindForPanicCode`，把 Solidity `PanicCode` 映射为：
  - `panic_checked_arithmetic`
  - `panic_division_by_zero`
  - `panic_array_out_of_bounds`
  - `panic_enum_conversion`
  - `panic_storage_encoding`
  - `panic_empty_array_pop`
  - `panic_resource_error`
  - 以及其他保守分类。
- `src/Passes/evm/SolidityPatterns.cpp:1448` 新增 `notdec_solidity_rewrite_checked_bounds_panic(code, kind)` marker，作为 checked/bounds rewrite 的稳定接口。
- `src/Passes/evm/SolidityPatterns.cpp:1476` 新增 `addCheckedBoundsMetadata`，同时写：
  - `!notdec.solidity.checked_bounds`
  - `!notdec.solidity_checked_bounds.panic_code`
  - 通用 hidden/rewrite marker
  - checked-bounds 专用 panic marker。
- `src/Passes/evm/SolidityPatterns.cpp:1794` 改 `CheckedBoundsPass::run`：复用 `matchSolidityRevert`，支持 base-relative Panic buffer，不再只看 `mstore(0, selector)` 的窄形状。
- `test/run_evm_solidity_patterns_suite.py:502` 和 `:589` 增加 checked-bounds kind、panic code、专用 marker 参数 oracle。
- `test/evm/solidity-patterns/solidity/checked_bounds_*.sol:1` 新增 5 个 Solidity 0.8.26 源码 case。
- `test/evm/solidity-patterns/cases/checked_bounds_*.ll:1` 新增对应 runtime bytecode 经过 Gigahorse + evm2llvm 生成的 IR。
- `test/evm/solidity-patterns/manifest.json:10` 起新增 5 个 Solidity-generated case oracle。
- `test/evm/solidity-patterns/manifest.json:1512` 和 `:1867` 更新两个现有真实 IR case 的 checked-bounds 计数和分类 oracle。变化来自原来漏掉的 base-relative Panic buffer 被识别，不是 revert 总数变化。

## 生成命令

```bash
npx --yes solc@0.8.26 --standard-json
python3 external/NotDec-evm2llvm/scripts/notdec-evm2llvm.py \
  <runtime-bytecode.hex> \
  -o test/evm/solidity-patterns/cases/<case>.ll \
  --gigahorse-dir /sn640/gigahorse-toolchain \
  --evm2llvm build-evm2llvm/bin/evm2llvm \
  --timeout-secs 120 \
  --jobs 1
```

## 验证

- `cmake --build ./build --target all -j4` 通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-solidity-patterns-checked-bounds`：73 passed。
- `python3 test/run_evm_solidity_rewrite_suite.py --binary build/bin/notdec --manifest test/evm/solidity-rewrite/manifest.json --project-root . --workdir /tmp/notdec-solidity-rewrite-checked-bounds`：65 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_(patterns|rewrite)' --output-on-failure`：2/2 passed，170.85s。
- runner 内部对每个输出 `.ll` 都跑了 `llvm-22.1.0.obj/bin/llvm-as`。
- fortune 同口径烟测：`/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds.c --tr-level=2` 通过，`elapsed=16.61`。这次 pass 只跑 EVM triple，fortune 不经过 checked-bounds pass。

## 评估

- 实现效果：8/10。Panic 类 checked/bounds 已能按 Solidity codegen 的稳定 code 分类，并有专用 rewrite marker；empty revert 仍保守候选。
- 复杂度：4/10。主要复用已有 `matchSolidityRevert`，只增加一个小结构和 code 映射。
- 维护成本：4/10。新增 oracle 能直接暴露分类退化；后续若要删除 guard CFG，需要另写 CFG 安全规则。

暂不处理：

- 不按条件表达式反推具体 `index < length`、`sum < x` 的操作数语义。
- 不删除 panic 分支或重写 CFG。当前先把对外语义稳定成 marker，低层残片仍通过 existing hidden metadata 交给后端隐藏。

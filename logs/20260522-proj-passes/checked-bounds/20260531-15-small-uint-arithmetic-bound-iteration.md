# CheckedBoundsPass small uint arithmetic bound iteration

## 本轮问题

round15 统计：

- pattern suite：111 个 checked-bounds skip，其中 panic `0x11` 有 21 个。
- apehex batch589：2 个 skip，都是 panic `0x11`。

apehex 剩余两个 `0x11` 是常量形状：

```llvm
%x = call i256 @evm_shl(i256 255, i256 1)
%cond = icmp eq i256 %x, 1 或 -1
```

这个分支实际恒不触发，像 checked exponent 的常量边界残留，但不能用现有 `checked_exp(base, exponent)` 准确表达。本轮不处理它。

pattern 里更明确的是小位宽 unsigned checked arithmetic，例如：

```llvm
%x = and i256 %arg0, 255
%y = and i256 %arg1, 255
%sum = add i256 %x, %y
%ok = icmp ult i256 %sum, 256
```

Solidity `YulUtilFunctions::overflowCheckedIntAddFunction/SubFunction/MulFunction` 对非 256-bit unsigned 类型会先 cleanup 到目标位宽，再检查结果是否超过 max。

## 选择

本轮只扩展 small unsigned add/sub/mul bound guard：

- panic code 必须是 `0x11`。
- guard 必须是 `result > max` 或 `result >= max + 1` 的等价形状。
- `max + 1` 必须是 2 的幂，且不是 256-bit 全宽。
- 运算 operands 必须是同一个 `max` 的 cleanup 结果，或者是小于等于 `max` 的常量。
- rewrite helper 带上 `max`，不复用 256-bit `checked_add/sub/mul` helper。

不做：

- 不处理 apehex 的常量 `shl(255, 1)` checked exponent 残留。
- 不处理 signed small integer arithmetic。
- 不把没有 cleanup 证据的业务比较当 compiler guard。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:228`
  - 新增 `isSmallUnsignedMax`、`isConstantWithin`、`isConstantEqual`、`isUnsignedCleanupToMax`。
- `src/Passes/evm/SolidityPatterns.cpp:1904`
  - 新增 `matchSmallUnsignedBoundedResult`，识别 `result > max` / `result >= max + 1`。
- `src/Passes/evm/SolidityPatterns.cpp:1986`
  - 在 `matchCheckedArithmetic` 里优先识别 small unsigned bounded add/sub/mul。
- `src/Passes/evm/SolidityPatterns.cpp:3000`
  - 新增 marker 名：
    - `notdec_solidity_rewrite_checked_add_bound`
    - `notdec_solidity_rewrite_checked_sub_bound`
    - `notdec_solidity_rewrite_checked_mul_bound`
- `test/evm/solidity-patterns/manifest.json`
  - 给 `0112_19493231_e681d3a0e3_c848651ad27d`、`1847_19507643_148f68d102_07c7230a7676`、`2001_19510128_72aa4f70b3_e9ee7c9c0e79` 加强 checked-bounds oracle，检查 bounded marker、skip reason 和 CFG rewrite。
- `docs/evm/entry-control/checked-bounds.md:33`
  - 补充 small unsigned arithmetic 的 `max` guard 形状。

## 效果

pattern suite：

- skip 总数：111 -> 106。
- panic `0x11` skip：21 -> 16。
- 新增 semantic marker：
  - `checked_add_bound`：2。
  - `checked_sub_bound`：1。
  - `checked_mul_bound`：2。

apehex batch589：

- 输出：38 个。
- skip 总数：2 -> 2。
- 剩余两个仍是常量 `shl(255, 1)` 形状。
- bounded marker：0。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round16`：78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round16：38 个输出，bad logs = 0。
- `llvm-22.1.0.obj/bin/llvm-as` 检查 apehex round16 输出：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round16.c --tr-level=2`，通过，`elapsed=16.34`。

## 下一轮候选

1. panic `0x41` fixed-size / pointer guard
   - 真实 case 数量：pattern 还有 49 个 `0x41` skip，apehex batch589 暂无。
   - 语义确定性：中等。需要拆 fixed-size free pointer bump、helper-returned size、业务上限。
   - rewrite 风险：中等。
   - 测试成本：中等。
2. panic `0x21` enum conversion
   - 真实 case 数量：pattern 11 个，apehex 0 个。
   - 语义确定性：较高。
   - rewrite 风险：低到中。
   - 测试成本：低。
3. panic `0x31` empty array pop
   - 真实 case 数量：pattern 11 个，apehex 0 个。
   - 语义确定性：较高。
   - rewrite 风险：低到中。
   - 测试成本：低。
4. panic `0x11` 常量 checked exponent 残留
   - 真实 case 数量：apehex 2 个，pattern 暂未聚出同类。
   - 语义确定性：低到中。需要先对照 Solidity literal exponent codegen，定义准确 helper。
   - rewrite 风险：中等。
   - 测试成本：中等。

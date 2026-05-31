# CheckedBoundsPass empty array pop guard iteration

## 本轮问题

round17 统计：

- pattern suite：97 个 checked-bounds skip，其中 panic `0x31` 有 11 个。
- apehex batch589：2 个 skip，都是 panic `0x11`，没有 `0x31`。

pattern 里的 `0x31` 都是 storage dynamic array pop 的空数组检查：

```llvm
%old_len = call i256 @evm_sload(...)
%is_empty = icmp eq i256 %old_len, 0
br i1 %is_empty, label %panic, label %success
```

success block 里能看到同一个 `old_len` 参与 `old_len - 1` 或 `old_len + base - 1`。

Solidity 侧证据：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:1665` 到 `:1674`：storage array pop 先取 `oldLen`，`iszero(oldLen)` 时触发 `PanicCode::EmptyArrayPop`，然后 `sub(oldLen, 1)`。
- `/sn640/solidity/libsolidity/codegen/ArrayUtils.cpp:556` 到 `:565`：旧 codegen 也是先检查 `oldLength == 0`，再计算 `oldLength - 1`。

## 选择

本轮只扩展 storage array pop empty guard：

- panic code 必须是 `0x31`。
- failure condition 必须是 `length == 0`。
- `length` 必须来自 `evm_sload`。
- success block 必须使用同一个 `length` 计算 `length - 1`，或 `length + base - 1`。
- rewrite helper 是 `notdec_solidity_rewrite_empty_array_pop_storage(length)`。

不做：

- 不处理 memory/calldata，因为 Solidity `pop()` 作用在 storage dynamic array。
- 不处理看不到 `length - 1` 的分支。
- 不把普通业务 `sload(slot) == 0` 放宽成 pop。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2256`
  - 新增 `blockComputesLengthMinusOne`，确认 success block 用同一个 length 做 pop 后的减一。
- `src/Passes/evm/SolidityPatterns.cpp:2290`
  - 新增 `matchEmptyArrayPop`，识别 storage array pop empty guard。
- `src/Passes/evm/SolidityPatterns.cpp:3095`
  - 在 checked-bounds guard 匹配中接入 empty array pop。
- `src/Passes/evm/SolidityPatterns.cpp:3177`
  - 新增 semantic marker `notdec_solidity_rewrite_empty_array_pop_storage`。
- `test/evm/solidity-patterns/manifest.json:344`
  - 更新 `checked_bounds_pop_01` 的 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:1512`
  - 更新 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 的 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:1696`
  - 更新 `0679_19497465_c2187cbc73_f22fac5262f8` 的 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:2461`
  - 更新 `1847_19507643_148f68d102_07c7230a7676` 的 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:2591`
  - 更新 `1636_19506827_f7c7103caa_7898f279a7f3` 的 checked-bounds oracle。
- `docs/evm/entry-control/checked-bounds.md:119`
  - 补充 storage dynamic array `pop()` 的 `old_len == 0` 形状。

## 效果

pattern suite：

- skip 总数：97 -> 86。
- panic `0x31` skip：11 -> 0。
- 新增 empty array pop rewrite：11 个。

apehex batch589：

- 输出：38 个。
- skip 总数：2 -> 2。
- 剩余两个仍是 panic `0x11` 常量 `shl(255, 1)` 形状。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round18b`：78 passed。
- pattern round18b 输出 `llvm-as`：78/78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round18：38 个输出，bad logs = 0。
- apehex batch589 round18 输出 `llvm-as`：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round18.c --tr-level=2`，通过，`elapsed=17.04`。

## 下一轮候选

1. panic `0x41` resource guard
   - 真实 case 数量：pattern 49 个，apehex 0 个。
   - 语义确定性：中等。要继续拆 memory allocation、fixed-size free pointer bump、业务上限。
   - rewrite 风险：中等。
   - 测试成本：中等。
2. panic `0x11` 常量 checked exponent 残留
   - 真实 case 数量：apehex 2 个，pattern 暂未聚出同类。
   - 语义确定性：低到中。需要先对照 Solidity literal exponent codegen，并定义准确 helper。
   - rewrite 风险：中等。
   - 测试成本：中等。
3. panic `0x0` / `0x1` assert-like guard
   - 真实 case 数量：pattern 15 个，apehex 0 个。
   - 语义确定性：低到中。需要区分 compiler assert、user assert 和不可达分支。
   - rewrite 风险：中等。
   - 测试成本：中等。
4. 剩余 panic `0x21` / `0x32`
   - 真实 case 数量：pattern 各 2 个，apehex 0 个。
   - 语义确定性：低到中。当前不是简单 enum conversion 或 array bounds 形状。
   - rewrite 风险：中等。
   - 测试成本：中等。

# CheckedBoundsPass enum conversion guard iteration

## 本轮问题

round16 统计：

- pattern suite：106 个 checked-bounds skip，其中 panic `0x21` 有 11 个。
- apehex batch589：2 个 skip，都是 panic `0x11`，没有 `0x21`。

pattern 里的 `0x21` 主要来自 enum conversion，例如：

```llvm
%ok = icmp ult i256 %value, 5
br i1 %ok, label %success, label %panic
```

失败条件就是 `value >= enum_member_count`。

Solidity 侧证据：

- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp:851` 到 `:861`：enum overflow check 用 `member_count - 1 < value`，失败时触发 `PanicCode::EnumConversionError`。
- `/sn640/solidity/libsolidity/codegen/CompilerUtils.cpp:891` 到 `:900`：integer 转 enum 也是同类检查。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:4089` 到 `:4094`：Yul cleanup 要求 `value < members`，panic code 是 `EnumConversionError`。

## 选择

本轮只扩展 enum conversion guard：

- panic code 必须是 `0x21`。
- failure condition 必须归一成 `value >= member_count` 或 `member_count <= value`。
- `member_count` 必须是小常量，范围是 `1..256`。
- rewrite helper 是 `notdec_solidity_rewrite_enum_conversion(value, member_count)`。

不做：

- 不处理 `0648/0651` 里还没证明的 `0x21` skip。
- 不处理非常量 enum member count。
- 不把其他 `0x21` 或业务比较放宽进来。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2208`
  - 新增 `isSmallEnumMemberCount`。
- `src/Passes/evm/SolidityPatterns.cpp:2218`
  - 新增 `matchEnumConversion`，识别 `0x21` enum member count guard。
- `src/Passes/evm/SolidityPatterns.cpp:3016`
  - 在 checked-bounds guard 匹配中接入 enum conversion。
- `src/Passes/evm/SolidityPatterns.cpp:3093`
  - 新增 semantic marker `notdec_solidity_rewrite_enum_conversion`。
- `test/evm/solidity-patterns/manifest.json:305`
  - 更新 `checked_bounds_enum_01` 的 enum conversion oracle。
- `test/evm/solidity-patterns/manifest.json:1509`
  - 更新 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 的 enum conversion oracle。
- `test/evm/solidity-patterns/manifest.json:2114`
  - 更新 `1775_19507435_1be1a16c5d_eb304133e7c9` 的 enum conversion oracle。
- `test/evm/solidity-patterns/manifest.json:2585`
  - 给 `1636_19506827_f7c7103caa_7898f279a7f3` 补 checked-bounds 强 oracle。
- `docs/evm/entry-control/checked-bounds.md:112`
  - 补充 enum conversion 的 `value >= enum_member_count` 形状。

## 效果

pattern suite：

- skip 总数：106 -> 97。
- 新增 enum conversion rewrite：9 个。
- 仍保留的 `0x21` skip：`0648`、`0651` 各 1 个，暂不处理。

apehex batch589：

- 输出：38 个。
- skip 总数：2 -> 2。
- 剩余两个仍是 panic `0x11` 常量 `shl(255, 1)` 形状。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round17b`：78 passed。
- pattern round17b 输出 `llvm-as`：78/78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round17：38 个输出，bad logs = 0。
- apehex batch589 round17 输出 `llvm-as`：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round17.c --tr-level=2`，通过，`elapsed=17.02`。

## 下一轮候选

1. panic `0x41` resource guard
   - 真实 case 数量：pattern 49 个，apehex 0 个。
   - 语义确定性：中等。要继续拆 memory allocation、fixed-size free pointer bump、业务上限。
   - rewrite 风险：中等。
   - 测试成本：中等。
2. panic `0x31` empty array pop
   - 真实 case 数量：pattern 11 个，apehex 0 个。
   - 语义确定性：较高。
   - rewrite 风险：低到中。
   - 测试成本：低。
3. panic `0x11` 常量 checked exponent 残留
   - 真实 case 数量：apehex 2 个，pattern 暂未聚出同类。
   - 语义确定性：低到中。需要先对照 Solidity literal exponent codegen，并定义准确 helper。
   - rewrite 风险：中等。
   - 测试成本：中等。
4. 剩余 panic `0x21`
   - 真实 case 数量：pattern 2 个，apehex 0 个。
   - 语义确定性：低到中。当前不像简单 `value < member_count`。
   - rewrite 风险：中等。
   - 测试成本：中等。

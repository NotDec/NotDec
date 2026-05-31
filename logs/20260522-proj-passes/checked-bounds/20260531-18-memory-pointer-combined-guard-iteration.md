# CheckedBoundsPass memory pointer combined guard iteration

## 本轮问题

round18 统计：

- pattern suite：86 个 checked-bounds skip，其中 panic `0x41` 有 49 个。
- apehex batch589：2 个 skip，都是 panic `0x11`，没有 `0x41`。

`0x41` 里有一类 free memory pointer bump guard：

```llvm
%new = add i256 %old_free_ptr, %size
%in_range = icmp ule i256 %new, 18446744073709551615
%no_wrap = icmp uge i256 %new, %old_free_ptr
%ok = and i1 %in_range, %no_wrap
br i1 %ok, label %success, label %panic
```

还有固定小 size 的等价形状：

```llvm
%new = add i256 %old_free_ptr, 128
%in_range = icmp ule i256 %new, 18446744073709551615
%no_wrap = icmp ult i256 %old_free_ptr, -128
%ok = and i1 %in_range, %no_wrap
```

success block 里紧接着 `mstore(0x40, new_free_ptr)`。

Solidity 侧证据：

- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:3256` 到 `:3271`：`finalizeAllocation` 计算 `newFreePtr = memPtr + roundUp(size)`，如果 `newFreePtr > 0xffffffffffffffff` 或 `newFreePtr < memPtr` 就触发 `PanicCode::ResourceError`，然后 `mstore(0x40, newFreePtr)`。
- `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2362` 到 `:2383`：array allocation size 会先检查 length 不超过 `uint64`。

## 选择

本轮只扩展 memory allocation pointer guard：

- panic code 必须是 `0x41`。
- branch condition 必须是两个 bool 条件的 `and`，并且 success 分支才继续。
- range check 接受 `new_ptr < 2^64` 或 `new_ptr <= 2^64 - 1`。
- no-wrap check 接受 `new_ptr >= old_ptr`。
- 固定小 size 额外接受 `old_ptr < -size`，但 size 必须是 `1..4096` 且 32-byte 对齐。
- `old_ptr` 必须来自 `mload(0x40)`，success block 必须写回 `mstore(0x40, new_ptr)`。

不做：

- 不处理单独的 `length <= 2^64-1`，除非 success block 能证明 allocation size 计算。
- 不处理没有 free pointer 写回的业务上限。
- 不处理 size 不对齐或过大的固定 bump。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2702`
  - 新增 `isSmallFixedMemoryAllocationSize`。
- `src/Passes/evm/SolidityPatterns.cpp:2715`
  - 新增 `isSupportedMemoryAllocationSize`，复用已有动态 allocation size 识别，并允许小 fixed size。
- `src/Passes/evm/SolidityPatterns.cpp:2931`
  - 扩展 `matchMemoryAllocationPointerBounds`：
    - 支持 `new_ptr <= 2^64 - 1`。
    - 支持 `old_ptr < -size` 的固定 size no-wrap guard。
    - 继续要求 `mstore(0x40, new_ptr)`。
- `test/evm/solidity-patterns/manifest.json:1512`
  - 更新 `2001_19510128_72aa4f70b3_e9ee7c9c0e79` 的 pointer bounds oracle。
- `test/evm/solidity-patterns/manifest.json:1698`
  - 更新 `0679_19497465_c2187cbc73_f22fac5262f8` 的 pointer bounds oracle。
- `test/evm/solidity-patterns/manifest.json:1994`
  - 更新 `1991_19510015_7f60edd6aa_6188d2031ddf` 的 pointer bounds oracle。
- `docs/evm/entry-control/checked-bounds.md`
  - 补充 combined free pointer range/no-wrap guard 形状。

## 效果

pattern suite：

- skip 总数：86 -> 78。
- panic `0x41` skip：49 -> 41。
- 新增 memory allocation pointer rewrite：8 个。

apehex batch589：

- 输出：38 个。
- skip 总数：2 -> 2。
- 剩余两个仍是 panic `0x11` 常量 `shl(255, 1)` 形状。

## 验证

- `cmake --build ./build --target all -j4`：通过。
- `python3 test/run_evm_solidity_patterns_suite.py --binary ./build/bin/notdec --manifest test/evm/solidity-patterns/manifest.json --project-root . --workdir /tmp/notdec-patterns-cb-round19b`：78 passed。
- pattern round19b 输出 `llvm-as`：78/78 passed。
- `ctest --test-dir build -R 'notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite' --output-on-failure`：2/2 passed。
- apehex batch589 round19：38 个输出，bad logs = 0。
- apehex batch589 round19 输出 `llvm-as`：38/38 passed。
- fortune smoke：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-checked-bounds-round19.c --tr-level=2`，通过，`elapsed=17.57`。

## 下一轮候选

1. 剩余 panic `0x41` resource guard
   - 真实 case 数量：pattern 41 个，apehex 0 个。
   - 语义确定性：中等。还要拆 length guard、returndata allocation、业务上限。
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

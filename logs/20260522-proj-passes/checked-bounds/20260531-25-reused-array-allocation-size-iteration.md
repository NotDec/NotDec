# Checked bounds reused array allocation size

## 问题

上一轮后 pattern suite 还有 45 个 `checked_bounds_skip`，panic `0x41`
还有 14 个。继续看 `0x41` 后，`0334_19494307_668d201319_1354ce2e324d`
里有 6 个重复的 array allocation guard，`1991_19510015_7f60edd6aa_6188d2031ddf`
还有 1 个同类 guard。

IR 形状是：

- 前面已经算好 `size = (length << 5) + 32`；
- 后面的 success block 复用这个 `size`，写 `mstore(oldPtr, length)`；
- 再用 `mstore(0x40, oldPtr + size)` 更新 free memory pointer；
- 非空时还会 `calldatacopy(oldPtr + 32, ..., length << 5)`。

Solidity `allocateMemoryArrayFunction` 会调用 `arrayAllocationSizeFunction`，
对非 bytes array 的动态内存数组就是 `length * 32 + 32`。这里的 size
表达式已经在前一块里算好，所以旧 matcher 因为只在当前 success block 找
`length << 5` 漏掉了。

## 本轮选择

只补直接识别 size 表达式本身：如果 `size` 是 `(length << 5) + 32`，即使
`length << 5` 不在当前 success block，也允许通过 allocation size 证明。

不做：

- 不接受没有 header store / free pointer store 的 `0x41`；
- 不接受任意跨 block 算出来的 size，只认 `length << 5` 和常量 `32`；
- 不处理 storage-to-memory copy helper 里的剩余 `0x41`；
- 不处理 apehex 剩余两个 `0x11`。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:1732` 新增 `isMemoryAllocationShift`，
  直接判断一个值是不是当前 `length << 5`。
- `src/Passes/evm/SolidityPatterns.cpp:2498` 的 `isArrayAllocationSize`
  在原有 block-local 查找失败后，直接检查 `size` 表达式是不是
  `(length << 5) + 32`。
- `test/evm/solidity-patterns/manifest.json:1930` 给
  `0334_19494307_668d201319_1354ce2e324d` 增加强 checked-bounds oracle。
- `test/evm/solidity-patterns/manifest.json:2067` 更新
  `1991_19510015_7f60edd6aa_6188d2031ddf` oracle：`memory_allocation_bounds`
  `6 -> 7`，`panic_resource_error` `1 -> 0`，semantic marker `6 -> 7`，
  `unrecognized_operands` `3 -> 2`，CFG rewrite `72 -> 73`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：45 -> 38。
- pattern panic `0x41` skip：14 -> 7。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，169.95s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.75s。

复杂度评分：

- 实现效果：7/10。一次减少 7 个 `0x41` skip，并把 `0334` 补成强 oracle。
- 理解成本：3/10。只是把已有 allocation size 识别从 block-local 扩到表达式本身。
- 维护成本：3/10。仍要求 header store 和 free pointer store，且只接受固定 `length << 5 + 32`。

## 剩余 skip 和下一轮

pattern suite 当前剩余 38 个 skip：

- panic `0x0`: 14
- panic `0x11`: 10
- panic `0x41`: 7
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. apehex batch589 剩余两个 panic `0x11`：真实数据集还剩两个，数量不大但目标直接。下一步要回原始 IR 或 Solidity codegen 确认 signed increment 形状。
2. pattern 剩余 panic `0x41`：还剩 7 个，主要是 storage-to-memory copy helper 和 returndata allocation，不能只按 uint64 guard 放宽。
3. pattern panic `0x0` / `0x1`：共 15 个，数量最高，但要先区分 compiler assert、user assert 和不可达分支。
4. pattern 剩余 panic `0x21` / `0x32`：数量少，语义明确，可以后处理。

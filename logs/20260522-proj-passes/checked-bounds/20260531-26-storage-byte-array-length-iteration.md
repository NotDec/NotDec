# Checked bounds storage byte array length

## 问题

上一轮后 pattern suite 还有 38 个 `checked_bounds_skip`，其中 panic
`0x41` 还有 7 个。继续看 `0x41` 后，6 个 case 里都有同一种
`copyByteArrayToStorageFunction` 形状：

- guard 判断 `newLen <= uint64.max`，失败走 `panic(0x41)`；
- success block 先 `sload(slot)`，再调用 private helper 取旧 bytes/string 长度；
- 同一块里调用另一个 private helper，参数同时带 `slot`、旧长度和 `newLen`；
- 后面马上按 `newLen > 31` 分短/长 storage bytes/string 写回。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:2063`
的 `copyByteArrayToStorageFunction` 明确有：

- `let newLen := <arrayLength>(...)`
- `if gt(newLen, 0xffffffffffffffff) { <panic>() }`
- `let oldLen := <byteArrayLength>(sload(slot))`
- 清理旧尾部 slot，然后按 `newLen > 31` 写 storage。

所以这是编译器生成的 storage bytes/string 长度 guard，不是业务分支。

## 本轮选择

只处理 `copyByteArrayToStorageFunction` 的 `newLen` 上限 guard。收益是 6 个
pattern skip，语义证据稳定，rewrite 风险低。

不做：

- 不把任意 `0x41` + `uint64.max` guard 都当成 storage copy；
- 不接受只有 private helper call、但缺少 `sload(slot)`、旧长度、新长度和
  `newLen > 31` 分支关系的形状；
- 不处理 batch589 剩余两个 `panic 0x11`，当前 pass 位置已经丢了原始加法操作数。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:2862` 新增 `callHasArg`。
- `src/Passes/evm/SolidityPatterns.cpp:2874` 新增 `hasLengthGreaterThan31Branch`。
- `src/Passes/evm/SolidityPatterns.cpp:2894` 新增
  `hasStorageByteArrayCopyToStorageSuccess`，在同一 success block 内同时检查
  storage slot、旧长度 helper、copy helper 和 `newLen > 31` 分支。
- `src/Passes/evm/SolidityPatterns.cpp:3092` 新增
  `matchStorageByteArrayLengthBounds`，只接受 panic `0x41` 的 uint64 上限 guard。
- `src/Passes/evm/SolidityPatterns.cpp:3506` 接入 checked-bounds matcher。
- `src/Passes/evm/SolidityPatterns.cpp:3576` 增加 semantic marker
  `notdec_solidity_rewrite_storage_byte_array_length_bounds`。
- `test/evm/solidity-patterns/manifest.json:1512`、`:1603`、`:2148`、`:2427`、
  `:2506`、`:2640` 更新或补充 6 个 case 的强 checked-bounds oracle。

## 效果

- build：`cmake --build ./build --target all -j4`，通过。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：38 -> 32。
- pattern panic `0x41` skip：7 -> 1。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，170.81s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，16.60s。

复杂度评分：

- 实现效果：7/10。pattern suite 里 6 个 storage bytes/string copy guard 从 skip
  变成 rewrite。
- 理解成本：4/10。新增了一个专用 matcher，但条件都在同一 success block 内。
- 维护成本：3/10。没有按 helper 名字或地址匹配，主要依赖 Solidity codegen 的数据流形状。

## 剩余 skip 和下一轮

pattern suite 当前剩余 32 个 skip：

- panic `0x0`: 14
- panic `0x11`: 10
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1
- panic `0x22`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 和 apehex 剩余 panic `0x11`：数量最多，batch589 也只剩这类。当前两个真实
   case 已确认像 Solidity signed increment guard，但当前 pass 位置缺少原始操作数，下一轮要看能否从更早 IR 或保留的数据流证明。
2. pattern panic `0x0` / `0x1`：数量最高，但要先区分 compiler assert、user assert 和不可达分支，不能直接 rewrite。
3. pattern panic `0x32`：数组越界语义明确，数量少，适合从剩余 2 个具体 case 继续看。
4. pattern 剩余 panic `0x41`：只剩 1 个，优先级低，先确认是不是 compiler allocation/copy guard。

# Checked bounds storage bytes div length

## 问题

上一轮后 pattern suite 还有 27 个 `checked_bounds_skip`。其中
`1636_19506827_f7c7103caa_7898f279a7f3` 还有一个 panic `0x22` skip。

看真实 IR 后，这个 guard 是 Solidity storage bytes/string 编码检查：

- `length := evm_div(data, 2)`；
- `outOfPlaceEncoding := and(data, 1)`；
- short encoding 下 `length := and(length, 127)`；
- 检查 `outOfPlaceEncoding == (length < 32)` 时 `panic(0x22)`。

Solidity `/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:1359`
的 `extractByteArrayLengthFunction` 正是这个形状：先 `div(data, 2)`，再按
低位判断 short/long encoding，最后用 `StorageEncodingError` panic。

## 本轮选择

只把已有 `storage_bytes_encoding` matcher 从 `shr(slot, 1)` 扩到
Solidity 原生的 `div(slot, 2)`。这是同一个无符号 slot 解码长度表达式，而且后续
仍然要求同一个 slot 的 low bit、`and 127` 和 `length < 32` 编码检查。

不做：

- 不新增宽松的任意 `div x, 2` matcher；
- 不处理其他 panic `0x22` 形状；
- 不处理剩余 `0x0`、`0x1`、`0x11`、`0x32` skip；
- 不做 metadata-only，仍然要求输出 semantic marker 并 rewrite CFG。

## 修改

- `src/Passes/evm/SolidityPatterns.cpp:3286` 在
  `matchStorageBytesLength` 里接受 `evm_div(slot, 2)`。
- `src/Passes/evm/SolidityPatterns.cpp:3290` 保留原来的
  `evm_shr(1, slot)` 形状。
- `src/Passes/evm/SolidityPatterns.cpp:3493` 仍要求
  `matchStorageBytesLength` 通过后才返回 `storage_bytes_encoding`。
- `test/evm/solidity-patterns/manifest.json:2805` 更新 `1636...` oracle：
  `panic_storage_encoding` 改为 `storage_bytes_encoding`，新增
  `notdec_solidity_rewrite_storage_bytes_encoding`，skip 清零，CFG rewrite
  `18 -> 19`。

## 效果

- build：`cmake --build ./build --target all -j4`，通过，`ninja: no work to do`。
- pattern suite：78 passed, 0 failed。
- pattern output `llvm-as`：78/78 passed。
- pattern skip：27 -> 26。
- pattern panic `0x22` skip：1 -> 0。
- ctest：`notdec.evm.solidity_patterns|notdec.evm.solidity_rewrite` 2/2 passed，171.90s。
- apehex batch589：38 个输出，bad logs 0，`llvm-as` 38/38 passed，剩余 skip 2 个，都是 panic `0x11`。
- fortune smoke：`test/lifting/wasm/cases/fortune.o3.wasm`，`--tr-level=2`，17.21s。

复杂度评分：

- 实现效果：3/10。只减少 1 个 pattern skip，但消掉了最后一个 `0x22` skip。
- 理解成本：2/10。只是给已有 storage bytes/string length matcher 增加 Solidity codegen 里的等价长度表达式。
- 维护成本：2/10。匹配条件仍绑定同一个 slot、low bit、short length mask 和 encoding check。

## 剩余 skip 和下一轮

pattern suite 当前剩余 26 个 skip：

- panic `0x0`: 14
- panic `0x11`: 5
- panic `0x32`: 2
- panic `0x21`: 2
- panic `0x41`: 1
- panic `0x1`: 1
- panic `0x12`: 1

按“真实 case 数量、语义确定性、rewrite 风险、测试成本”排序：

1. pattern 剩余 panic `0x11`：还剩 5 个。数量中等，可能还有能证明的 unsigned 子类，但 signed 或丢原始操作数的形状不能猜。
2. pattern panic `0x32`：剩 2 个。语义明确，但要先证明是 compiler array bounds guard，不能直接删业务检查。
3. pattern panic `0x0` / `0x1`：数量最高。下一步先分类 assert 来源，区分 compiler assert、user assert 和不可达分支。
4. batch589 两个 panic `0x11`：真实数据仍剩两个。要从更早 IR 或 helper 形状找证据，当前 pass 位置还不能安全 rewrite。

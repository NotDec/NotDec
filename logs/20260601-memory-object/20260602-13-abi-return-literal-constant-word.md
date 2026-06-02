# ABI return literal constant word

## 原始 prompt

阅读 logs/20260601-memory-object/PLAN.md， 实现并迭代完善 Memory Object相关的Pass。日志放到`logs/20260601-memory-object`文件夹内，文件名格式和logs/的日志一致。

注意，一定要做好IR rewrite，而不能仅仅标好metadata。rewrite 是目标，metadata 只辅助调试/oracle。

## Solidity 源码回顾

`/sn640/solidity/libsolidity/codegen/YulUtilFunctions.cpp:164-188` 的 `storeLiteralInMemoryFunction()` 按 word 写 literal：

- `words = (literal.length() + 31) / 32`
- 每个 word 用 `formatAsStringOrNumber(_literal.substr(...))`
- 生成 `mstore(add(memPtr, offset), wordValue)`

所以短 literal 可能有两种 IR 形状：

- `evm_shl(const, const)`：需要左移得到 ABI 左对齐 word。
- 直接 `ConstantInt`：Solidity 已经把 word 写成最终 256-bit 常量，不需要再追 `evm_shl`。

这轮检查 suite 后发现 `0118_19493248_593c688d00_799916048038` 里有长度 25 的 direct constant literal payload。之前只生成 `literal_payload`，没有继续生成 `literal_bytes_return` 和 ABI return 级别 rewrite。

## 本轮目标

补 direct constant literal word：

- 常量 data word 直接作为 final word。
- `evm_shl(const, const)` 继续走原有左移折叠。

这轮仍然只在已经确认 `literal_source` 的 dynamic ABI return 上生效，不扩大 source 匹配范围。

## 修改

- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:682-691`
  - 新增 `getLiteralFinalWord()`，统一处理 direct `ConstantInt` 和 `evm_shl(const, const)`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:944-1005`
  - `insertAbiReturnDynamicArrayLiteralBytesMarker()`、`insertAbiReturnDynamicArrayLiteralBytesReturnMarker()`、`insertAbiReturnLiteralBytesRewriteMarker()` 改为接收已经折叠好的 `FinalWord`。
- `src/Passes/evm/solidity-patterns/AbiReturnPass.cpp:1150-1173`
  - literal payload 识别后先保留原有 `evm_shl` 细节 marker，再用 `getLiteralFinalWord()` 统一插入 `literal_bytes`、`literal_bytes_return` 和 `rewrite_abi_return_literal_bytes`。
- `test/evm/solidity-patterns/manifest.json:3933-3936`
  - 给 `0118_19493248_593c688d00_799916048038` 增加 direct constant word 后的 oracle：
    - `notdec_solidity_abi_return_dynamic_array_literal_bytes`: 2
    - `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`: 2
    - `notdec_solidity_rewrite_abi_return_literal_bytes`: 2

## 验证

构建：

```bash
cmake --build ./build --target all -j4
```

结果：通过。

manifest JSON：

```bash
python3 -m json.tool test/evm/solidity-patterns/manifest.json >/tmp/notdec-memory-manifest.json
```

结果：通过。

Solidity patterns：

```bash
ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure
```

结果：通过，`110.70 sec`。

`0118` 现在有两组 literal bytes return：

- length 25，direct constant word。
- length 5，`evm_shl(const, const)` word。

fixed100 audit：

```bash
rm -rf /tmp/notdec-memory-fixed100-literal-constant-word
mkdir -p /tmp/notdec-memory-fixed100-literal-constant-word/out
# 对 20260601-evm2llvm-train-batch639/640/641 的前 100 个 outputs/*.bc 跑 build/bin/notdec --tr-level=3
rg -o 'call void @notdec_solidity_[A-Za-z0-9_]+\(' /tmp/notdec-memory-fixed100-literal-constant-word/out/*.ll \
  | sed 's/.*call void @//; s/(//' | sort | uniq -c | sort -k2
```

关键结果：

- `notdec_solidity_abi_return_dynamic_array_literal_bytes`: 9
- `notdec_solidity_abi_return_dynamic_array_literal_bytes_return`: 9
- `notdec_solidity_rewrite_abi_return_literal_bytes`: 9

fixed100 数量没有增加，说明固定 100 个样例里暂时没有 direct constant literal return；这轮覆盖的是 suite 里已有的真实 `0118` 缺口。

## 下一步

按价值排序：

1. 扩大 apehex 范围找多 word literal bytes / string；当前 fixed100 仍只有单 word。
2. 找 calldata decode / concat / builder 形状的 dynamic return source。
3. 把后续 lowering 接到 `notdec_solidity_rewrite_abi_return_literal_bytes` 和 `notdec_solidity_rewrite_abi_return_storage_dynamic_array`。

# Evm2llvm apehex 源码 markdown 导出脚本

## 这次做了什么

新增一个脚本，把 apehex parquet 里某一条合约记录的 `creation_sourcecode`
导出成单个 markdown 文件。markdown 里先写样本元信息，再按 Solidity standard JSON
里的 `sources` 顺序列出每个文件名和源码代码块。

## 改动点

- `/sn640/NotDec/external/NotDec-evm2llvm/scripts/export-apehex-source-md.py`
  - 第 16-43 行：解析参数。支持 `--contract-address` 或 `--row-index` 二选一，`-o/--output` 指定 markdown 输出。
  - 第 46-65 行：延迟导入 `pyarrow` 并读取 parquet 的必要列。
  - 第 68-78 行：按合约地址或行号选中记录。
  - 第 81-99 行：解析 `creation_sourcecode`。兼容 apehex 里偶尔出现的外层双大括号 `{{...}}`。
  - 第 102-106 行：选择安全的 markdown code fence，避免源码里出现反引号导致代码块提前结束。
  - 第 109-138 行：写出 markdown，包含 parquet、row index、block、contract address、transaction hash、language 和每个源码文件。
  - 第 141-154 行：主流程。

## 使用方式

按合约地址导出：

```bash
python3 external/NotDec-evm2llvm/scripts/export-apehex-source-md.py \
  /sn640/NotDecChainExp/apehex_evm_contracts/hex/ethereum/train/19494000_to_19494999.parquet \
  --contract-address 261e203d6f794d29a77e77d168358f4a82caf18f \
  -o /tmp/0394-source-export-test.md
```

按 parquet 行号导出：

```bash
python3 external/NotDec-evm2llvm/scripts/export-apehex-source-md.py \
  /path/to/file.parquet \
  --row-index 364 \
  -o /tmp/source.md
```

## 验证

```bash
python3 -m py_compile external/NotDec-evm2llvm/scripts/export-apehex-source-md.py
python3 external/NotDec-evm2llvm/scripts/export-apehex-source-md.py \
  /sn640/NotDecChainExp/apehex_evm_contracts/hex/ethereum/train/19494000_to_19494999.parquet \
  --contract-address 261e203d6f794d29a77e77d168358f4a82caf18f \
  -o /tmp/0394-source-export-test.md
rg -n '^## ' /tmp/0394-source-export-test.md
wc -l /tmp/0394-source-export-test.md
```

结果：

- 导出成功：`/tmp/0394-source-export-test.md`
- 文件标题正常，元信息里 `row_index` 是 `364`
- 识别出 5 个源码文件：
  - `contracts/UCCM.sol`
  - `contracts/manifold/ERC721Creator.sol`
  - `node_modules/@openzeppelin/contracts/proxy/Proxy.sol`
  - `node_modules/@openzeppelin/contracts/utils/Address.sol`
  - `node_modules/@openzeppelin/contracts/utils/StorageSlot.sol`
- 输出文件 570 行。

## 性能影响

这个脚本只在离线导出源码时运行，不接入 evm2llvm lowering、类型恢复、pointer analysis
或主 pass pipeline，不影响 fortune 当前关注用例，所以没有跑 fortune 对比。

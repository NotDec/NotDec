# 补齐剩余 Solidity pattern 类别覆盖

## 背景

上一轮 manifest 已有 14 个 case，但 `abi-revert-encoding`、`checked-bounds`、`packed-storage-field`、`storage-bytes-string` 还没有 metadata 和 oracle，因此不能证明这些 pass 类别达到 10 个 case。

## 修改

- `test/evm/solidity-patterns/manifest.json:1` 更新 14 个 case 的 oracle，新增以下 metadata 计数：
  - `notdec.solidity.abi_revert_encoding`
  - `notdec.solidity.checked_bounds`
  - `notdec.solidity.packed_storage_field`
  - `notdec.solidity.storage_bytes_string`
- 同步更新每个 case 的 `patterns` 字段。

## 当前覆盖

按 manifest 的 `patterns` 统计：

- `abi_decode`: 14
- `abi_return`: 14
- `abi_revert_encoding`: 11
- `checked_bounds`: 12
- `event_log`: 11
- `external_call`: 14
- `memory_buffer`: 14
- `nonpayable_guard`: 11
- `packed_storage_field`: 14
- `payability`: 11
- `revert`: 14
- `selector_entry`: 14
- `storage_addressing`: 10
- `storage_bytes_string`: 11
- `value_cleanup`: 14

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过，14 个 case 全部通过，耗时 9.26s。
- fortune 同口径运行：`./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-eval2.c --tr-level=2`，退出码 0，耗时 16.66s。

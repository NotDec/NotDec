# Solidity pattern batch case 扩充

## 背景

目标是每个 pass 类别至少 10 个测试样例覆盖。原 manifest 只有 3 个固定 case，且没有 `patterns` 字段，无法直接统计类别覆盖。

## 修改

- `test/evm/solidity-patterns/manifest.json:10` 开始为每个 case 增加 `patterns`。
- `test/evm/solidity-patterns/manifest.json:24` 开始把 oracle 扩展到 `notdec.solidity.abi_decode`。
- 新增 11 个来自 `/sn640/NotDecChainExp/evm2llvm_apehex_pilot` 的 `.ll` case：
  - `test/evm/solidity-patterns/cases/0441_19494998_776c03cc9d_8117f350cb9d.ll`
  - `test/evm/solidity-patterns/cases/0448_19495059_065877b669_4f138305be23.ll`
  - `test/evm/solidity-patterns/cases/0450_19495071_6b6c9447e0_6344565f4b31.ll`
  - `test/evm/solidity-patterns/cases/0456_19495158_c7c34e07aa_0e692a3d145c.ll`
  - `test/evm/solidity-patterns/cases/0457_19495180_71d7525532_df21a257bd5c.ll`
  - `test/evm/solidity-patterns/cases/0459_19495198_fc5ac215e3_805f0e6bf43e.ll`
  - `test/evm/solidity-patterns/cases/0460_19495212_3dd362a96f_b915c4f6cb8b.ll`
  - `test/evm/solidity-patterns/cases/0725_19498082_e78beb21f7_98e658f9eae8.ll`
  - `test/evm/solidity-patterns/cases/0726_19498082_c3945ab21b_2b68f9f60fc2.ll`
  - `test/evm/solidity-patterns/cases/0735_19498132_e6671af1f9_f57fe690a6a3.ll`
  - `test/evm/solidity-patterns/cases/0740_19498186_acae9b9760_d235a47b18d5.ll`

## 当前覆盖

按 manifest 的 `patterns` 统计：

- `abi_decode`: 14
- `abi_return`: 14
- `event_log`: 11
- `external_call`: 14
- `memory_buffer`: 14
- `nonpayable_guard`: 11
- `payability`: 11
- `revert`: 14
- `selector_entry`: 14
- `storage_addressing`: 10
- `value_cleanup`: 14

这次只证明当前已实现的类别达到 10 个 case 覆盖。`checked-bounds`、`packed-storage-field`、`storage-bytes-string`、`abi-revert-encoding` 等还需要继续补 matcher 和 oracle。

## 验证

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过，14 个 case 全部通过，耗时 9.08s。

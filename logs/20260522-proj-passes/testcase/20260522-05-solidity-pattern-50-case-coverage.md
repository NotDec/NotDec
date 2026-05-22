# Solidity pattern 50 case coverage

## 背景

目标从每类至少 30 个 case 提高到每类至少 50 个 case。覆盖仍按
`test/evm/solidity-patterns/manifest.json` 的 `patterns` 字段统计：一个 case 对同一类别只算
1 次，不按 metadata 命中次数重复计算。

## 修改

- `test/evm/solidity-patterns/manifest.json:1` 新增 20 个真实 evm2llvm case。
- 新增 case 文件：
  - `test/evm/solidity-patterns/cases/0009_19493025_977804571f_ab93bcf84624.ll`
  - `test/evm/solidity-patterns/cases/0029_19493081_446349df2b_2c2bd78bea7d.ll`
  - `test/evm/solidity-patterns/cases/0030_19493089_e48135f4b6_56a7c8e02323.ll`
  - `test/evm/solidity-patterns/cases/0031_19493095_1fa5f54c3e_32ee79d4db5c.ll`
  - `test/evm/solidity-patterns/cases/0032_19493098_f0dab0bf78_8c9406cb7887.ll`
  - `test/evm/solidity-patterns/cases/0038_19493119_aed19fef21_ea7b432ec4f1.ll`
  - `test/evm/solidity-patterns/cases/0112_19493231_e681d3a0e3_c848651ad27d.ll`
  - `test/evm/solidity-patterns/cases/0114_19493235_fc69a30a99_3aa7e05b781a.ll`
  - `test/evm/solidity-patterns/cases/0118_19493248_593c688d00_799916048038.ll`
  - `test/evm/solidity-patterns/cases/0121_19493259_63ed382f00_e1eb3485de04.ll`
  - `test/evm/solidity-patterns/cases/0128_19493282_117a55c492_4233986babaf.ll`
  - `test/evm/solidity-patterns/cases/0146_19493376_503a732d50_138a4d77d617.ll`
  - `test/evm/solidity-patterns/cases/0147_19493380_262c2cb2b2_c24931e22d66.ll`
  - `test/evm/solidity-patterns/cases/0175_19493428_d314d380f5_900e60ffbb10.ll`
  - `test/evm/solidity-patterns/cases/0176_19493428_12f263f8c9_ce83d7b0b2a7.ll`
  - `test/evm/solidity-patterns/cases/0178_19493463_4da3a6e3d8_c33022fe2f2f.ll`
  - `test/evm/solidity-patterns/cases/0179_19493473_76bc10e48c_fd3041389134.ll`
  - `test/evm/solidity-patterns/cases/0190_19493619_2fea34e2f2_9597b615b1d4.ll`
  - `test/evm/solidity-patterns/cases/0195_19493642_a9eb505e1d_c5345c8e5a60.ll`
  - `test/evm/solidity-patterns/cases/0207_19493703_606c6dc416_58752239cf88.ll`

这次没有改 matcher。筛选时要求新 case 至少覆盖 `payability`、`nonpayable_guard`、
`storage_addressing`、`event_log`，因为这些是 50 目标下的主要缺口。

## 覆盖结果

按 manifest 的 `patterns` 统计：

- `abi_decode`: 58
- `abi_return`: 58
- `abi_revert_encoding`: 50
- `checked_bounds`: 56
- `event_log`: 55
- `external_call`: 57
- `memory_buffer`: 58
- `nonpayable_guard`: 50
- `packed_storage_field`: 58
- `payability`: 50
- `revert`: 58
- `selector_entry`: 58
- `storage_addressing`: 54
- `storage_bytes_string`: 50
- `value_cleanup`: 58

## 验证

已跑：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过，58 个 case 全部通过，耗时 76.28s。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-eval50.c --tr-level=2` 通过，`elapsed=16.35`。

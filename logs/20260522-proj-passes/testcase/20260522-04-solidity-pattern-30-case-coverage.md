# Solidity pattern 30 case coverage

## 背景

目标从每类至少 10 个 case 提高到每类至少 30 个 case。这里的覆盖按
`test/evm/solidity-patterns/manifest.json` 里的 `patterns` 字段统计：一个 case
命中某类只算 1 次，不按 metadata 出现次数重复计算。

## 修改

- `test/evm/solidity-patterns/manifest.json:1` 新增 24 个真实 evm2llvm case，并用当前
  `build/bin/notdec --tr-level=0` 输出重算 `expected_nonpayable_functions` 和
  `expected_metadata_counts`。
- 新增 case 文件：
  - `test/evm/solidity-patterns/cases/2001_19510128_72aa4f70b3_e9ee7c9c0e79.ll`
  - `test/evm/solidity-patterns/cases/0189_19493609_3c0627c9e0_9d16fec0a1c2.ll`
  - `test/evm/solidity-patterns/cases/0258_19493864_78edd9f88e_b098925cbd2d.ll`
  - `test/evm/solidity-patterns/cases/0679_19497465_c2187cbc73_f22fac5262f8.ll`
  - `test/evm/solidity-patterns/cases/0651_19497235_18ca6cbfa7_017843325734.ll`
  - `test/evm/solidity-patterns/cases/0334_19494307_668d201319_1354ce2e324d.ll`
  - `test/evm/solidity-patterns/cases/1274_19502133_51e0372918_736c45f00099.ll`
  - `test/evm/solidity-patterns/cases/1991_19510015_7f60edd6aa_6188d2031ddf.ll`
  - `test/evm/solidity-patterns/cases/1924_19508168_fe1f1c11b0_0b74e6ed0d55.ll`
  - `test/evm/solidity-patterns/cases/0340_19494346_717db37a78_7324081e28fc.ll`
  - `test/evm/solidity-patterns/cases/1775_19507435_1be1a16c5d_eb304133e7c9.ll`
  - `test/evm/solidity-patterns/cases/0648_19497230_998d150449_ea367f7fc1ac.ll`
  - `test/evm/solidity-patterns/cases/1407_19503702_47675ff2ed_71b1710392bd.ll`
  - `test/evm/solidity-patterns/cases/1572_19505998_da275bcf64_0796e2424ba2.ll`
  - `test/evm/solidity-patterns/cases/1988_19508992_89095f0aa3_688f45aabbde.ll`
  - `test/evm/solidity-patterns/cases/1485_19504919_9deb0fc809_62afe2b29485.ll`
  - `test/evm/solidity-patterns/cases/1847_19507643_148f68d102_07c7230a7676.ll`
  - `test/evm/solidity-patterns/cases/0657_19497309_a4d607684e_332c65dbca0f.ll`
  - `test/evm/solidity-patterns/cases/1636_19506827_f7c7103caa_7898f279a7f3.ll`
  - `test/evm/solidity-patterns/cases/0637_19497125_d33ca6fd43_b0988224e0b7.ll`
  - `test/evm/solidity-patterns/cases/1490_19504983_72b68e8dfe_38ab68a7adc2.ll`
  - `test/evm/solidity-patterns/cases/2002_19510139_923ad4a9d1_4e9ed82fcc5b.ll`
  - `test/evm/solidity-patterns/cases/1915_19508046_91cdb18983_844dae639fc0.ll`
  - `test/evm/solidity-patterns/cases/0870_19499091_69ef351b75_57ed8326bd30.ll`

这次没有改 matcher 代码。现有识别已经能覆盖这些 case 的底层模式。

## 覆盖结果

按 manifest 的 `patterns` 统计：

- `abi_decode`: 38
- `abi_return`: 38
- `abi_revert_encoding`: 35
- `checked_bounds`: 36
- `event_log`: 35
- `external_call`: 38
- `memory_buffer`: 38
- `nonpayable_guard`: 30
- `packed_storage_field`: 38
- `payability`: 30
- `revert`: 38
- `selector_entry`: 38
- `storage_addressing`: 34
- `storage_bytes_string`: 34
- `value_cleanup`: 38

## 验证

已跑：

- `python3 -m json.tool test/evm/solidity-patterns/manifest.json` 通过。
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure` 通过，38 个 case 全部通过，耗时 58.24s。
- `/usr/bin/time -f 'elapsed=%e' ./build/bin/notdec test/lifting/wasm/cases/fortune.o3.wasm -o /tmp/notdec-fortune-eval30.c --tr-level=2` 通过，`elapsed=16.56`。

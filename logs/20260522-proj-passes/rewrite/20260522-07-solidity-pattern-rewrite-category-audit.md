# Solidity pattern rewrite category audit

## 背景

本轮目标是 rewrite 阶段。当前 58 个 case 已经让每个 pass 类别至少有 50 个样例；代码也已经把所有 `addStringMetadata` 命中的位置统一写成：

- `notdec_solidity_rewrite_<category>(kind)`：高层 rewrite marker。
- `notdec_solidity_rewrite_hidden(kind)`：低层隐藏 marker。
- `!notdec.solidity.rewrite_hidden.<category>`：挂在原低层指令或函数入口点上的隐藏 metadata。

这说明所有现有 pass 类别都有通用 rewrite / hide 表达。但这还不是逐类删除 CFG 或合并 buffer 的最终形态。

## 覆盖证据

`test/evm/solidity-patterns/manifest.json` 当前有 58 个 case。按 `patterns` 字段统计：

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

metadata 总命中量：

- `notdec.solidity.abi_decode`: 393
- `notdec.solidity.abi_return`: 2387
- `notdec.solidity.abi_revert_encoding`: 3149
- `notdec.solidity.checked_bounds`: 1822
- `notdec.solidity.cleanup`: 1990
- `notdec.solidity.entry_kind`: 58
- `notdec.solidity.event`: 434
- `notdec.solidity.external_call`: 362
- `notdec.solidity.memory_object`: 7989
- `notdec.solidity.packed_storage_field`: 3446
- `notdec.solidity.payability_guard`: 7570
- `notdec.solidity.revert`: 5683
- `notdec.solidity.selector_inlined_body`: 3
- `notdec.solidity.selector_prologue`: 57
- `notdec.solidity.storage_addressing`: 2154
- `notdec.solidity.storage_bytes_string`: 232

runner 已验证每个 case 的 rewrite marker、hidden marker、hidden metadata 数量都和对应 metadata oracle 对齐。

## 按类别判断

- selector-entry：已有 rewrite / hide。真正拆 selector CFG 需要 selector 比较链和 fallback / receive 边界，目前只隐藏已认领 prologue 和内联候选，先不拆函数。
- payability / nonpayable：已有 rewrite / hide。删除 guard CFG 会改变后续 checked-bounds / revert matcher 的输入，当前保留原 helper 并挂 hide metadata。
- revert：已有 rewrite / hide。Panic / Error / custom error 参数还依赖 ABI revert buffer，先不删除 revert 终点。
- checked-bounds：已有 rewrite / hide。panic code、array bounds、enum / conversion check 还没有精分类，先保留 guard CFG。
- value-cleanup：已有 rewrite / hide。address / uintN / intN 仍是类型线索，不强行改成最终类型。
- memory-buffer：已有 rewrite / hide。free memory pointer 可标，但 buffer 分组和跨块写入顺序还没恢复，暂不合并为 allocation / buffer object。
- abi-decode：已有 rewrite / hide。静态 calldata 读取可标，动态参数 offset / length / copy 还没绑定，暂不合并成函数参数。
- abi-return：已有 rewrite / hide。return site 可标，return buffer 的 mstore 序列和动态 head/tail 还没绑定，暂不合并成返回值。
- abi-revert-encoding：已有 rewrite / hide。selector 和 encoded revert candidate 可标，Error/custom error 参数还没恢复，暂不合并成完整高层 revert。
- storage-addressing：已有 rewrite / hide。sha3 长度候选可标，但 scratch memory key/base slot 还没确认，暂不改成 mapping / array slot intrinsic。
- packed-storage-field：已有 rewrite / hide。load/store 候选可标，但 bit offset / width 没有稳定提取，暂不改成 field intrinsic。
- storage-bytes-string：已有 rewrite / hide。短长编码候选可标，但还没绑定 storage 根和 memory copy，暂不改成 bytes/string 高层操作。
- event-log：已有 rewrite / hide。logN 可标，但 topic 常量、匿名事件、data buffer 还没恢复，暂不合并成 emit event。
- external-call：已有 rewrite / hide。call kind 可标，但 target/value/gas/input/output buffer、success check、returndata bubble 还没绑定，暂不合并成完整 external call。

## 结论

当前 rewrite 阶段已经覆盖所有现有 pass 类别：每个被识别的低层点都有高层 rewrite marker、hide marker 和原指令 hide metadata，且 58 个 case 全量验证通过。

逐类真正删除低层 helper 或合并成 typed intrinsic 的难点主要是共享的数据流：memory buffer、ABI buffer、storage scratch、returndata 和 CFG guard 关系。没有这些信息时直接删除 CFG 风险大，容易让后续 pass 识别变差。因此本阶段停止在“可测、可隐藏、后续 pass 不受破坏”的 rewrite surface。

## 验证

- `cmake --build ./build --target all -j4`
- `ctest --test-dir build -R notdec.evm.solidity_patterns --output-on-failure`
  - 结果：58/58 passed，84.58s。

# 2026-04-30 ValueHTypes lower/upper 命名更正和 merged 计划

## 背景

之前把 `pos=true` 的结果写成 `upper`，`pos=false` 的结果写成 `lower`。
这个名字容易误导。

当前统一成：

- `pos=true`：沿 lower bounds 收集，求这些下界的最小公共上界。最终仍命名为
  `lower`
- `pos=false`：沿 upper bounds 收集，求这些上界的最大公共下界。最终命名为
  `upper`

也就是说，`ValueTypes.txt` 和 `ValueHTypes.txt` 都应该打印：

```text
lower=<pos=true result> ; upper=<pos=false result>
```

`llvm2c` 仍保留两边信息。默认选类型时：

- 普通 value 取 lower
- `ContraVariantValues` 里的 value 取 upper

## 后续 merged 目标

下一步再加 `merged` 类型：

- `ValueHTypes.txt` 显示 `lower`、`upper`、`merged`
- `llvm2c` 默认用 merged
- lower/upper 仍可通过接口查询，方便后续特殊逻辑

初始 merged 策略保持简单：

1. 如果 lower 存在且不是根 bottom，选 lower
2. 否则选 upper
3. 如果只有 lower，即使是 bottom 也先保留

这一步本次没有实现，只先把 lower/upper 名字纠正。

## 本次实现

### `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h`

- `48-55`：`HTypeResult` 改为保存 `ValueTypesLower` 和 `ValueTypesUpper`。
  注释明确 `pos=true` 是 lower-side，`pos=false` 是 upper-side。
- `67-83`：`prefersLowerValueType()` 改成 `prefersUpperValueType()`。
  `getDefaultValueType()` 现在普通 value 取 lower，contra value 取 upper。
- `131-132`：snapshot formatter 预热 lower/upper 两张表。
- `153-175`：`ValueHTypes.txt` / `.htypes` 打印时，`lower=` 取
  `getValueType(value, true)`，`upper=` 取 `getValueType(value, false)`。

### `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

- `101-104`：`ConstraintsGenerator` 保存 `ValueTypesLower` 和
  `ValueTypesUpper`，注释说明两张表对应的 polarity。

### `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

- `1420-1426`：`ValueTypes.txt` 改为 `lower=pos=true`、
  `upper=pos=false`。
- `1521-1538`：`type-struct-merge.md` root label 同步改名。
- `1605-1620`：`ImportantHTypes` formatter 预热 lower/upper 两张表。
- `1683`：重要函数列表从 lower 表枚举 value。
- `3074-3143`：`genTypes()` 中 `pos=true` 转成 `ValueTypesLower`，
  `pos=false` 转成 `ValueTypesUpper`。memory 仍用 `pos=false` 的结果。
- `3166-3188`：`genASTTypes()` 合并 lower/upper 两张表，并从 upper 表取
  memory type。

### `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp`

- `1519-1528`：`demoteSSAFixHT()` 删除 demote 前 value 时同时处理
  upper/lower 两张表。
- `1564-1572`：phi demote 到 alloca 后，upper 写入 `ValueTypesUpper`，
  lower 写入 `ValueTypesLower`。

### `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp`

- `90-96`：`ValueCTypes.txt` 从 lower 表枚举 value，再通过默认 helper
  取实际使用类型。
- `612-613`：结构体使用统计也从 lower 表枚举 value。

### `test/tools/compare_htypes_with_wasm_dwarf.py`

- `306-313`：realworld compare 解析 `.htypes` 时，`[+]` 取 lower，
  `[-]` 取 upper。

### snapshot

- 同步 `test/type-recovery/llvm-ir/expected/tr-level-2/*.htypes`
- 同步 `test/type-recovery/sysy/expected/tr-level-2/*.htypes`

## 验证

1. clean build

```bash
cmake --build ./build --target notdec-decompile --clean-first -j4
```

结果：通过。

2. 小 case 检查

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01.bound-name-swap.out.ll --tr-level=2 -g \
  --work-dir=/tmp/notdec-01-bound-name-swap
```

结果：`ValueHTypes.txt` 和 `ValueTypes.txt` 都变成
`lower=pos=true ; upper=pos=false`。

3. `llvm-ir` suite

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-suite-llvm-ir-bound-name-swap-verify
```

结果：`17 passed, 3 xfailed, 0 failed`。

4. `sysy` suite

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/sysy/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-suite-sysy-bound-name-swap-verify
```

结果：`6 passed, 3 xpassed, 0 failed`。runner 返回非零仍是既有 XPASS。

5. fortune 性能

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original \
/usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.bound-name-swap.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-bound-name-swap
```

结果：跑通，`real 16.73s`。当前参考是 `real 16.33s`，慢约 `0.40s`。

6. realworld oracle

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/realworld/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-suite-realworld-bound-name-swap
```

结果：仍失败。失败点仍是 `@File_list`、`free_desc::arg0`、
`get_tbl::arg0`、`matches_in_list::arg0`、`maxlen_in_list::arg0` 等
`fd` 结构恢复问题，属于当前已知剩余问题。

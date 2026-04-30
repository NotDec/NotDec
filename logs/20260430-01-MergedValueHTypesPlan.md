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

## 当前 lower/upper 传递路径

`ConstraintsGenerator::genTypes()` 是现在 lower/upper 结果变成 HType 的出口。

当前流程：

1. 对每个 value 把两个 root 放进 `Tys`：
   - `PolarVar{var, pos=true}`：lower
   - `PolarVar{var, pos=false}`：upper
2. 调 `TypeSimplifier::bulkSimplifyDetailed(Tys, false)`，得到
   `BulkResult.types`
3. `BulkResult.types` 是 `std::map<PolarVar, UTypePtr>`，里面已经有每个 value
   的 lower UType 和 upper UType
4. `genTypes()` 把 lower/upper UType 分别转成 HType，写入
   `ValueTypesLower` / `ValueTypesUpper`
5. `.c` 输出时，这份 `HTypeResult` 直接传到 `llvm2c`

当前先不在 UType 阶段产出 merged。先在 HTypeResult 里做一个更小的实验：
默认选出的类型如果是 bottom，就尝试改用 upper。

## 当前 merged 规划

目标：

- 先不改 `ValueHTypes.txt` 格式
- 后续默认类型如果落到 bottom，优先试 upper
- lower/upper 仍保留，后续需要特殊逻辑时还能直接查两边

基本判断：

- 不重新跑求解，只基于已有 lower/upper HType 做轻量选择
- 本次只处理 bottom 这种明显没有信息的情况
- 不把字段数量、后端可打印性这类现象当作类型偏序

初版策略：

1. 先按当前逻辑取 default：普通 value 取 lower，
   `ContraVariantValues` 里的 value 取 upper。
2. 如果 default 是 root `Bottom`，尝试返回 upper。
3. 如果 upper 不存在，或者 upper 也是 bottom，保留原 default。
4. 其他情况不改。

暂时不做：

- 不按字段数量、结构体覆盖范围等“信息量”打分
- 不用 canonical type 决定 merged
- 不处理 top fallback
- 不在 set union/intersection 上补子类型规则
- 不额外引入一套后端选择逻辑

实现位置：

- 只改 `HTypeResult::getDefaultValueType(Value)`。
- `getValueType(Value, true/false)` 保持不变，继续专门查 lower/upper。

后续如果要更精确：

- 可以考虑在 UType 或更早的 CompactType 阶段做，因为那里更接近求解结果。
- 但当前先不做，避免把这一步变成新的类型偏序实现。

风险：

- 这个 fallback 很保守，可能不会改变某些当前不理想的 C 输出。
- 但它不引入复杂猜测，便于先确认 bottom fallback 是否有实际收益。

判断标准：

- C 输出默认使用 bottom fallback 后的类型。
- lower/upper 结果仍保留，后续需要特殊逻辑时还能查两边。
- `llvm-ir`、`sysy` suite 不退化。
- fortune 当前关注用例同口径计时，不能出现明显性能下降；这一步不重新求解，理论上
  只增加每个 value 的小常数判断。

## 本次追加实现：default bottom fallback

### `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h`

- `82-91`：`HTypeResult::getDefaultValueType()` 保持原来的默认选择逻辑；
  如果默认结果是 bottom，则尝试返回 upper-side 类型。upper 不存在时仍返回原结果。

### 追加验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. 小 `.c` 用例

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01.bottom-fallback.out.c --tr-level=2 -g \
  --work-dir=/tmp/notdec-01-bottom-fallback
```

结果：跑通。`ValueHTypes.txt` 中多个 value 是 `lower=bottom:32 ; upper=top:32`；
`ValueCTypes.txt` 里对应默认类型变成 `top:32`，说明 fallback 生效。

3. fortune `.c` 路径

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.bottom-fallback.out.c \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-bottom-fallback
```

结果：跑通，`real 20.54s`。过程中仍有既有 llvm2c pointer arithmetic /
global offset warning。

4. fortune 当前参考口径 `.ll`

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.bottom-fallback.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-bottom-fallback-ll
```

结果：跑通，`real 16.44s`。当前参考是 `real 16.33s`，基本同档。

5. `llvm-ir` suite

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-suite-llvm-ir-bottom-fallback
```

结果：`17 passed, 3 xfailed, 0 failed`。

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

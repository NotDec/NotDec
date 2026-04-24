# 2026-04-23 双 ValueTypes 映射：upper/lower 同时保留

## 背景

当前 `bulkSimplify` 之前只会给每个 `ExtValuePtr` 选一个 variance，
最后只落一张 `ValueTypes`。这次改成：

- `ValueTypes` 只存协变求出来的上界
- `ValueTypesLower` 新增，存逆变求出来的下界
- `ContraVariantValues` 先保留，继续作为“默认取哪边”的策略标记

同时按要求把 `.htypes` / `ValueTypes.txt` 改成同一行显示 upper/lower。

## 代码修改

### 1. 结果结构与快照输出

1. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:48-188`
   - 涉及结构/函数：
     - `notdec::llvm2c::HTypeResult`
     - `HTypeResult::hasValueType`
     - `HTypeResult::getValueType`
     - `HTypeResult::hasDefaultValueType`
     - `HTypeResult::getDefaultValueType`
     - `HTypeResult::print`
     - `HTypeResult::primeFormatter`
     - `HTypeResult::printValueSection`
   - 修改内容：
     - 新增 `ValueTypesLower`
     - 在 `HTypeResult` 内封装按 `ContraVariantValues` 取默认类型的 helper
     - `.htypes` 的 `[types]` section 改成一行显示
       `upper=... ; lower=...`
     - snapshot formatter 预热时同时收集 upper/lower 两张表里的声明

2. `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:63-70`
   - 涉及结构：
     - `notdec::mlsub::ConstraintsGenerator`
   - 修改内容：
     - 新增 `ValueTypesLower`

### 2. MLsub 求解与 debug 落盘

1. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1392-1445`
   - 涉及函数：
     - `appendDebugValueTypes`
   - 修改内容：
     - `ValueTypes.txt` 现在同一行显示 `upper` 和 `lower`

2. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1519-1615`
   - 涉及函数：
     - `primeSnapshotFormatter`
     - `writeDebugImportantHTypes`
   - 修改内容：
     - snapshot formatter 预热同时吃 upper/lower
     - `ImportantHTypes.txt` 里的函数原始类型改成走默认取值 helper

3. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2761-2858`
   - 涉及函数：
     - `ConstraintsGenerator::genTypes`
     - `MLsubRecovery::genASTTypes`
   - 修改内容：
     - `genTypes()` 对每个 `V2N` 同时请求
       `PolarVar{pos=true}` 和 `PolarVar{pos=false}`
     - `ValueTypes` 落 upper，`ValueTypesLower` 落 lower
     - 为了减少 lower 对 upper/memory 的 `HTypeContext` 污染，转换顺序改成：
       - 先全部 upper
       - 再 memory
       - 最后全部 lower
     - `genASTTypes()` 合并 SCC 结果时同时合并两张 map

### 3. llvm2c 默认取值与 demoteSSA 迁移

1. `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp:65-115,608-618`
   - 涉及函数：
     - `ClangTypeResult::hasType`
     - `ClangTypeResult::getType`
     - `ClangTypeResult::writeValueCTypes`
     - `ClangTypeResult::calcUseRelation`
   - 修改内容：
     - `llvm2c` 侧默认不再直接读 `Result->ValueTypes`
     - 统一改成通过 `HTypeResult::getDefaultValueType()` 取类型

2. `external/NotDec-llvm2c/lib/notdec-llvm2c/StructuralAnalysis.cpp:1468-1580`
   - 涉及函数：
     - `demoteSSAFixHT`
   - 修改内容：
     - phi demote 到 reg2mem alloca 时，同时迁移 upper/lower 两张表
     - 删除 demoted 值时，也同时从 `ValueTypesLower` 里删

### 4. 测试工具与 snapshot golden

1. `test/tools/compare_htypes_with_wasm_dwarf.py:13-16,150-159,305-320`
   - 涉及函数：
     - `parse_function_pointer_type`
     - `parse_htypes`
   - 修改内容：
     - parser 支持新的
       `upper=... ; lower=...`
       行格式
     - 修掉函数指针参数解析末尾多吃一个 `)` 的问题

2. 重新生成 snapshot golden
   - `test/type-recovery/llvm-ir/expected/tr-level-2/*.htypes`
   - `test/type-recovery/sysy/expected/tr-level-2/*.htypes`
   - 修改内容：
     - 将当前 pass snapshot 全量同步到新格式
     - 这批文件是整文件重写，不是局部 patch

## 验证

1. 构建
   - 命令：
     `cmake --build /sn640/NotDec/build --target notdec-decompile --clean-first -j2`
   - 结果：通过

2. `llvm-ir` snapshot suite
   - 命令：
     `python3 test/run_type_recovery_suite.py --binary ./build/bin/notdec --manifest test/type-recovery/llvm-ir/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-suite-llvm-ir-dual-verify`
   - 结果：`16 passed, 3 xfailed, 0 failed`

3. `sysy` snapshot suite
   - 命令：
     `python3 test/run_type_recovery_suite.py --binary ./build/bin/notdec --manifest test/type-recovery/sysy/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-suite-sysy-dual-verify`
   - 结果：
     - `6 passed`
     - 还有 `3 xpass`
   - 说明：
     - 没有新的 fail
     - suite runner 因为 `xpass` 返回非零，这个看起来是仓库当前已有状态

4. realworld `fortune`
   - 命令：
     `python3 test/run_type_recovery_suite.py --binary ./build/bin/notdec --manifest test/type-recovery/realworld/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-suite-realworld-dual3`
   - 结果：仍然失败
   - 当前剩余问题：
     - `@File_list` 的若干字段缺失
     - `free_desc::arg0`
     - `get_tbl::arg0`
     - `matches_in_list::arg0`
     - `maxlen_in_list::arg0`
   - 说明：
     - 现在 `.htypes` / `ValueTypes.txt` 格式和 compare parser 已经匹配
     - 剩下的是 `fortune` 上真实的类型恢复退化，不是文本格式或 parser 问题

## 当前结论

- 这次“两张 map + 默认取值 helper + 同行显示 upper/lower”的主改动已经落完
- `llvm-ir` / `sysy` snapshot 已经跟新格式对齐
- `fortune` 还有真实回归，后续要继续盯 `fd*` 相关结构布局

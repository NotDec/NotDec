# 2026-04-24 workdir type 打印顺序改成 lower 在前

## 背景

昨天把双边界打印接进来后，workdir 里的几类类型输出都是：

- `upper=... ; lower=...`

这次只调整显示顺序，不改求解，不改默认取值逻辑。目标是统一改成：

- `lower=... ; upper=...`

## 代码修改

1. `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1419-1425`
   - 涉及函数：
     - `appendDebugValueTypes`
   - 修改内容：
     - workdir 里的 `ValueTypes.txt` 改成先打印 `lower`
     - 原来：
       - `upper=... ; lower=...`
     - 现在：
       - `lower=... ; upper=...`

2. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h:170-173`
   - 涉及函数：
     - `HTypeResult::printValueSection`
   - 修改内容：
     - `.htypes` / `--dump-htypes` / workdir `ValueHTypes.txt` 的 `[types]`
       section 统一改成先打印 `lower`

3. `test/tools/compare_htypes_with_wasm_dwarf.py:13-16,305-313`
   - 涉及函数：
     - 模块级正则 `TYPE_BOUNDS_LINE_RE`
     - `parse_htypes`
   - 修改内容：
     - 让 realworld compare 工具适配新的
       `lower=... ; upper=...`
       行格式

4. snapshot golden 同步
   - `test/type-recovery/llvm-ir/expected/tr-level-2/*.htypes`
   - `test/type-recovery/sysy/expected/tr-level-2/*.htypes`
   - 修改内容：
     - 只同步打印顺序变化

## 验证

1. 构建
   - 命令：
     `cmake --build /sn640/NotDec/build --target notdec-decompile -j2`
   - 结果：通过

2. workdir 小 case
   - 命令：
     `./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll -o /tmp/01.lower-first.out.ll --tr-level=2 -g --work-dir=/tmp/notdec-01-lower-first`
   - 结果：
     - `/tmp/notdec-01-lower-first/ValueHTypes.txt` 已变成
       `lower=... ; upper=...`
     - `/tmp/notdec-01-lower-first/ValueTypes.txt` 已变成
       `lower=... ; upper=...`

3. `llvm-ir` suite
   - 命令：
     `python3 test/run_type_recovery_suite.py --binary ./build/bin/notdec --manifest test/type-recovery/llvm-ir/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-suite-llvm-ir-lower-first-verify`
   - 结果：`16 passed, 3 xfailed, 0 failed`

4. `sysy` suite
   - 命令：
     `python3 test/run_type_recovery_suite.py --binary ./build/bin/notdec --manifest test/type-recovery/sysy/manifest.json --project-root /sn640/NotDec --workdir /tmp/notdec-suite-sysy-lower-first-verify`
   - 结果：
     - `6 passed`
     - `3 xpassed`
   - 说明：
     - 没有新的 fail
     - runner 非零还是因为仓库当前已有的 `xpass`

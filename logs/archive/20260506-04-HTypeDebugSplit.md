# 2026-05-06 拆分 HType debug 输出

## 背景

上一轮已经把 HType normalize 拆出 `MLsubGenerator.cpp`。剩下的大文件里还有不少
debug 输出逻辑。先继续拆最独立的一块：最终 HType 产物输出。

## 目标

只移动：

- `ValueHTypes.txt`
- `ImportantHTypes.txt`

不移动 `ValueTypes.txt` / `VarOrigins.txt` / `type-struct-merge.md`，因为它们还依赖
`ConstraintsGenerator` 内部状态和 UType origin helper。

## 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/HTypeDebug.h:1-16`
  - 新增头文件
  - 暴露 `writeDebugValueHTypes`
  - 暴露 `writeDebugImportantHTypes`

- `src/TypeRecovery/mlsub/HTypeDebug.cpp:1-186`
  - 新增实现文件
  - 从 `MLsubGenerator.cpp` 移入：
    - `writeDebugValueHTypes`
    - `writeDebugImportantHTypes`
    - `primeSnapshotFormatter`
    - `unwrapFunctionType`
    - `formatJoinedHTypes`
    - `ImportantFunctionEntry`

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:12`
  - include `notdec/TypeRecovery/mlsub/HTypeDebug.h`

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3040-3041`
  - 调用点保持 `writeDebugValueHTypes` / `writeDebugImportantHTypes` 不变

- `src/CMakeLists.txt:23`
  - 将 `TypeRecovery/mlsub/HTypeDebug.cpp` 加入 `notdec` target

## 验证

已运行：

```bash
cmake --build ./build --target notdec-decompile -j4
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-htype-debug-split/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-htype-debug-split
diff -u \
  /tmp/notdec-fortune-htype-normalize-split/ValueHTypes.txt \
  /tmp/notdec-fortune-htype-debug-split/ValueHTypes.txt
diff -u \
  /tmp/notdec-fortune-htype-normalize-split/ImportantHTypes.txt \
  /tmp/notdec-fortune-htype-debug-split/ImportantHTypes.txt
```

结果：

- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通
- fortune 时间：`real 14.98s`、`user 14.57s`、`sys 0.40s`
- 对比参考 `real 16.33s`，没有看到性能回退
- `ValueHTypes.txt` 与拆分前无差异
- `ImportantHTypes.txt` 与拆分前无差异
- `MLsubGenerator.cpp` 降到 4083 行

## 简评

- 实现效果：7/10。继续减少大文件体积，且行为不变。
- 复杂度成本：9/10。只拆两个最终 HType debug 输出入口。
- 维护成本：8/10。后续 HType debug 输出集中在 `HTypeDebug.cpp`。

下一步如果继续拆，可以考虑 `ValueTypes.txt` / `VarOrigins.txt`，但需要先处理
`formatExtValueMappingLabel` 和 UType origin helper 的共享边界。

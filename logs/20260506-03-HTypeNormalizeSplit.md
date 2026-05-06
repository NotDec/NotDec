# 2026-05-06 拆分 HType normalize

## 背景

`src/TypeRecovery/mlsub/MLsubGenerator.cpp` 已经超过 4500 行，并且混了约束生成、
override 解析、debug 输出、HType 后处理等职责。前两次新增的 HType 后处理逻辑
继续放在这里会让文件更难读。

## 目标

先做低风险拆分，只移动 HType 后处理：

- 尾部按值自递归转数组
- 单字段 record 透明化

不改行为，不继续拆 override / debug / PNDiff。

## 实现记录

修改文件：

- `include/notdec/TypeRecovery/mlsub/HTypeNormalize.h:1-12`
  - 新增头文件
  - 对外只暴露 `normalizeHTypeResult(llvm2c::HTypeResult &Result)`

- `src/TypeRecovery/mlsub/HTypeNormalize.cpp:1-321`
  - 新增实现文件
  - 从 `MLsubGenerator.cpp` 移入：
    - `normalizeTailValueRecursiveRecords`
    - `normalizeTransparentSingleFieldRecords`
    - recursive anchor 收集 helper
    - transparent record rewrite helper
  - 这些 helper 都留在匿名 namespace

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:12`
  - include `notdec/TypeRecovery/mlsub/HTypeNormalize.h`

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3191`
  - `MLsubRecovery::genASTTypes` 改为调用 `normalizeHTypeResult(*ResultVal)`

- `src/CMakeLists.txt:23`
  - 将 `TypeRecovery/mlsub/HTypeNormalize.cpp` 加入 `notdec` target

## 验证

已运行：

```bash
cmake --build ./build --target notdec-decompile -j4
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-htype-normalize-split/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-htype-normalize-split
diff -u \
  /tmp/notdec-fortune-transparent-single-field/ValueHTypes.txt \
  /tmp/notdec-fortune-htype-normalize-split/ValueHTypes.txt
```

结果：

- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通
- fortune 时间：`real 15.00s`、`user 14.58s`、`sys 0.42s`
- 对比参考 `real 16.33s`，没有看到性能回退
- fortune `ValueHTypes.txt` 与拆分前无差异
- `MLsubGenerator.cpp` 从约 4532 行降到 4236 行

## 简评

- 实现效果：8/10。把新加的 HType 后处理从大文件里拆出去了。
- 复杂度成本：9/10。只新增一个公开入口，调用点更简单。
- 维护成本：8/10。后续 HType normalize 继续放这个文件，不再堆到
  `MLsubGenerator.cpp`。

下一步如果继续拆，建议拆 debug 输出；override 解析耦合更多，放后面。

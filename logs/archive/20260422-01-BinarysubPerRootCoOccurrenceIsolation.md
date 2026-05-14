# 2026-04-22 binarysub per-root co-occurrence isolation

## 背景

`external/binarysub/src/binarysub.cpp` 里的 `TypeSimplifier::bulkSimplify()` 之前是：

1. 整批 root 先 `canonicalizeType`
2. 整批 root 共享同一个 `coOccurrences`
3. 再逐个 `simplifyType`

这会把不同 root 的 co-occurrence 交叉污染到一起，不符合 simplesub 原始 Scala 版里“每个待简化类型单独做分析”的语义。

## 代码修改

1. 在 `external/binarysub/include/binarysub/binarysub.h:269-314` 新增 `OccurrenceAnalysisState`
   - 涉及结构/函数：
     - `OccurrenceAnalysisState`
     - `TypeSimplifier::analyzeOccurrences`
     - `TypeSimplifier::simplifyType`
     - `TypeSimplifier::isClear`
     - `TypeSimplifier::clear`
   - 修改内容：
     - 把 `coOccurrences`、`processedRecVars` 从 `TypeSimplifier` 成员移到单次分析上下文
     - `analyzeOccurrences` / `simplifyType` 改为显式接收 `OccurrenceAnalysisState &`

2. 在 `external/binarysub/src/binarysub.cpp:908-930` 新增 `simplify_current_root`
   - 涉及函数：
     - `simplify_current_root`
   - 职责：
     - 封装单个 root 的 `canonicalize -> analyzeOccurrences -> simplifyType -> coalesce`
     - 保证一次 root 一套独立 state

3. 在 `external/binarysub/src/binarysub.cpp:1168-1277` 与 `external/binarysub/src/binarysub.cpp:1279-1617` 改造分析/简化实现
   - 涉及函数：
     - `TypeSimplifier::analyzeOccurrences`
     - `TypeSimplifier::simplifyType`
   - 修改内容：
     - 全部改用 `OccurrenceAnalysisState` 中的 `coOccurrences` / `processedRecVars`
     - 不再依赖 `TypeSimplifier` 共享成员保存 co-occurrence 分析结果

4. 在 `external/binarysub/src/binarysub.cpp:1820-1856` 改造对外入口
   - 涉及函数：
     - `TypeSimplifier::bulkSimplify`
     - `TypeSimplifier::simplify`
   - 修改内容：
     - `bulkSimplify` 改为对每个 `PolarVar` 单独新建 `TypeSimplifier` worker
     - `simplify` 也改为内部使用独立 worker，避免同一个 `TypeSimplifier` 实例多次调用时残留分析状态

5. 在 `external/binarysub/src/binarysub-test.cpp:476-523` 新增回归测试，并在 `external/binarysub/include/binarysub/binarysub-test.h:11-13`、`external/binarysub/src/binarysub-test-main.cpp:35-40` 接入测试入口
   - 涉及函数：
     - `test_bulk_simplify_isolation`
   - 校验目标：
     - `bulkSimplify` 的结果必须和“每个 root 单独 `TypeSimplifier().simplify(...)`”一致

## 验证

1. `binarysub` 自测
   - 命令：`./build/binarysub`
   - 结果：通过，新加的 `test_bulk_simplify_isolation` 通过

2. `notdec` 构建
   - 命令：`cmake --build ./build --target notdec-decompile -j2`
   - 结果：通过

3. `fortune` 链路验证
   - 命令：
     `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune.coocc.isolated.out.ll --tr-level=2 -g --work-dir=/tmp/notdec-fortune-coocc-isolated`
   - 结果：通过，成功输出 `/tmp/fortune.coocc.isolated.out.ll`

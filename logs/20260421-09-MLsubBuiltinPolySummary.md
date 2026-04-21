# 2026-04-21 MLsub Builtin Poly Summary

## 背景

`work_dir/SCCs.txt` 里没有把 `malloc` / `free` 识别成需要单独抬 level 的多态库函数。
当前默认 builtin summary 又会直接给它们写死 `args` / `ret` 类型，和“只做多态边界 +
PNDiff 方向约束”的需求不一致。

另外补上 `memcpy`：

- 标成多态函数
- 不再依赖固定 `args` / `ret`
- 用 `constraints` 明确 `src <: dest`

## 代码修改

1. 在 [resources/mlsub_builtin_summaries.json:200](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L200) 到 [resources/mlsub_builtin_summaries.json:276](/sn640/NotDec/resources/mlsub_builtin_summaries.json#L276) 调整 builtin summary 数据
   - 涉及函数：`free`、`malloc`、`memcpy`
   - 修改内容：
     - `free` / `malloc` 删除 `args` / `ret`
     - 新增 `is_polymorphic: true`
     - `memcpy` 新增 builtin summary，保留 `pndiff`，并添加 `constraints`：`arg1 <: arg0`

2. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:359](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L359) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:362](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L359) 新增 `MLsubRecovery::isSummaryOverridePolymorphic()`
   - 涉及函数：`MLsubRecovery::isSummaryOverridePolymorphic`
   - 职责：从已加载 summary override 中读取 `is_polymorphic`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:196](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L196) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:210](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L196) 新增 `hasFunctionTypeOverride()`
   - 涉及函数：`hasFunctionTypeOverride`
   - 职责：统一判断 override 是否提供完整的 `args + ret`，支持 summary “两者都不写”

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:353](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L353) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:395](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L353) 新增 summary target 解析
   - 涉及函数：
     - `resolveSummaryConstraintTarget`
     - `isSummaryConstraintTargetOperand`
   - 职责：让 summary 的 `constraints` 直接引用 `arg` / `ret`

5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1113](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1113) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1156](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1113) 放宽 summary 文件校验
   - 涉及函数：`loadOverrideFileImpl`
   - 职责：
     - summary 不再强制要求 `args` / `ret`
     - 校验 `constraints` 的 `lhs` / `rhs`
     - 校验 `is_polymorphic` 为 boolean

6. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2349](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2349) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2358](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2349) 实现 `MLsubRecovery::isSummaryOverridePolymorphic()`
   - 涉及函数：`MLsubRecovery::isSummaryOverridePolymorphic`
   - 职责：供 `prepareSCC()` 读取 summary 里的多态标记

7. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2505](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2505) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2593](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2505) 修改 summary 应用逻辑
   - 涉及函数：
     - `buildSummaryConstraintOperand`
     - `MLsubRecovery::applySummaryOverride`
   - 职责：
     - 支持“无 `args` / `ret`”的 summary
     - 支持 `constraints` 直接绑定到 `arg` / `ret` 节点
     - 继续保留 `pndiff` 覆盖逻辑

8. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2888](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2888) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2903](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2888) 修改 `MLsubRecovery::prepareSCC()`
   - 涉及函数：`MLsubRecovery::prepareSCC`
   - 职责：把 summary `is_polymorphic` 纳入 SCC level 提升判断

## 验证

1. JSON 格式验证
   - 命令：`python3 -m json.tool resources/mlsub_builtin_summaries.json`
   - 结果：通过

2. 构建验证
   - 命令：`cmake --build ./build --target notdec-decompile -j2`
   - 结果：通过

3. `fortune` 用例验证
   - 命令：
     `./build/bin/notdec test/type-recovery/realworld/cases/fortune.o3.wasm.ll -o /tmp/fortune.poly-summary.out.ll --tr-level=2 -g --work-dir=/tmp/notdec-fortune-poly`
   - 结果：通过
   - `/tmp/notdec-fortune-poly/SCCs.txt` 结果：
     - `SCC1,free (level = 1)`
     - `SCC2,malloc (level = 1)`

4. `memcpy` 最小 IR 验证
   - 临时文件：`/tmp/notdec_memcpy_poly_test.ll`
   - 命令：
     `./build/bin/notdec /tmp/notdec_memcpy_poly_test.ll -o /tmp/notdec_memcpy_poly_test.out.ll --tr-level=2 -g --work-dir=/tmp/notdec-memcpy-poly`
   - 结果：通过
   - `/tmp/notdec-memcpy-poly/SCCs.txt` 结果：
     - `SCC0,main (level = 0)`
     - `SCC1,memcpy (level = 1)`

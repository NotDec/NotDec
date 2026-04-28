# MLsub 冻结 IR 锚点切换为 `sha256` 单格式

日期：2026-04-17

## 1. 本次范围

沿着 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
和 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
继续收尾一小步：把 `MLsub` 冻结 IR 锚点从过渡期的 `md5/content_hash`
彻底切到计划里的单一 `sha256` 方案。

这次明确的取舍是：

1. workdir 导出的 `02-mlsub-input.anchor.json` 只写 `ir_anchor.sha256`
2. `NOTDEC_EXTRA_CONSTRAINTS` 校验时只接受 `ir_anchor.sha256`
3. 不再同时保留 `md5` 和 `sha256` 两套摘要支持

## 2. 具体修改

### 2.1 代码

1. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:259](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L259)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:294](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L259)
   - 去掉旧的 `MD5` helper，只保留 `computeSHA256Hex()`
   - `writeMLsubInputAnchor()` 改成只写 `sha256`
   - 涉及函数：
     - `computeSHA256Hex()`
     - `writeMLsubInputAnchor()`

2. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:891](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L891)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:919](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L891)
   - `validateExtraConstraintsAnchor()` 改成强制要求并校验
     `ir_anchor.sha256`
   - 不再接受 `content_hash/md5`
   - 涉及函数：
     - `validateExtraConstraintsAnchor()`

3. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1375](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1375)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1388](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1375)
   - `MLsubRecovery::run()` 里只计算一份 `ModuleSHA256Hex`
   - 导出 anchor 和校验 extra constraints 都统一走这一个摘要
   - 涉及函数：
     - `MLsubRecovery::run()`

4. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1450](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1450)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1474](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1450)
   - `MLsubRecovery::validateExtraConstraintsFile()` 的参数和调用
     简化为只接收 `ModuleSHA256Hex`
   - 涉及函数：
     - `MLsubRecovery::validateExtraConstraintsFile()`

5. [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:385](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L385)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:386](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L385)
   - `validateExtraConstraintsFile()` 的声明回到只接收一个
     `ModuleSHA256Hex`

### 2.2 文档

1. [DEBUG.md:71](/sn640/NotDec/DEBUG.md#L71)
   到 [DEBUG.md:82](/sn640/NotDec/DEBUG.md#L71)
   - `02-mlsub-input.anchor.json` 小节改为只描述 `sha256`
   - 去掉旧的 `content_hash/md5` 兼容说明

## 3. 验证

1. 编译：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

2. workdir 冒烟：

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-anchor-sha256/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-anchor-sha256/work
```

结果：`02-mlsub-input.anchor.json` 中只保留：

```json
{
  "sha256": "...",
  "stage": "mlsub-input",
  "ir_file": "02-mlsub-input.ll"
}
```

3. 额外约束正例：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "sha256": "..."
  },
  "functions": {}
}
```

结果：`NOTDEC_EXTRA_CONSTRAINTS` 可以正常通过锚点校验。

## 4. 当前判断

这一步之后，冻结 IR 锚点这一层和最初计划已经更一致：

1. 单一摘要算法
2. 单一字段入口
3. selector 与锚点语义都绑定到 `02-mlsub-input.ll`

后续如果继续推进，更适合回到真正使用这套锚点的 recovery / typing 问题，
而不是继续在摘要格式上做兼容层。

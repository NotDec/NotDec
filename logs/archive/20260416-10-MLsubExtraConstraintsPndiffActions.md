# MLsub 额外约束 `pndiff` action 实现

日期：2026-04-16

## 1. 本次范围

这次继续只往前推一小步：让 `NOTDEC_EXTRA_CONSTRAINTS` 里的函数级
`actions` 不再只是占位，而是先支持最小可用子集：

1. `functions.<name>.actions[*].kind = "pndiff"`
2. `target.kind` 当前只支持：
   - `arg`
   - `ret`
3. `state` 当前支持：
   - `ptr`
   - `number`

本次仍未实现：

- `bindings`
- `inst`
- `operand`
- `subtype`
- `equals`
- 任何类型表达式注入

## 2. 具体修改

### 2.1 `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h`

1. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:342](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L342)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:346](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L342)
   为 `MLsubRecovery` 新增：
   - `ExtraConstraintsDoc`
   - `ExtraConstraintsFuncs`
2. 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:384](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L384)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:394](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L384)
   新增声明：
   - `getExtraConstraintsSpec()`
   - `applyExtraConstraints()`

### 2.2 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:206](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L206)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:258](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L206)
   新增 extra-constraints 专用解析 helper：
   - `parseExtraConstraintPNDiffState()`
   - `resolveExtraConstraintPNDiffTarget()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:334](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L334)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:394](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L334)
   新增 `applyExtraConstraintPNDiffs()`，负责把 `actions[*]` 中的
   `pndiff` 动作真正施加到当前 `PNDiff` 节点上。
3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:520](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L520)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:577](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L520)
   将原来的“拒绝全部函数体”的 extra-constraints 校验替换成
   `validateExtraConstraintFunctions()`：
   - 允许 `functions.<name>.actions`
   - 校验 action 只能是 `kind = "pndiff"`
   - 校验重复 target 不能给出冲突状态
   - 把合法 spec 记入 `ExtraConstraintsDoc`
4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:989](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L989)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1014](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L989)
   让 `validateExtraConstraintsFile()` 在校验 `ir_anchor` 后继续加载
   `functions`
5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1381](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1381)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1388](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1381)
   新增 `MLsubRecovery::applyExtraConstraints()`，目前只调用
   `applyExtraConstraintPNDiffs()`
6. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1417](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1417)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1421](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1417)
   修改 `MLsubRecovery::bottomUpPhase()`：
   - 在 `G->run()` 后
   - summary/signature override 前
   - 如果函数命中 `ExtraConstraintsDoc`
   - 则打印 `Applying MLsub extra constraints to ...` 并应用

### 2.3 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   更新 `NOTDEC_EXTRA_CONSTRAINTS` 当前能力描述：
   - 已支持函数级 `actions.kind = "pndiff"`
   - target 目前只支持 `arg` / `ret`

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 正例

对 `test/type-recovery/llvm-ir/cases/01_Simple1.ll` 先导出锚点，再构造：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "content_hash": {
      "algorithm": "md5",
      "value": "1cb71d8840454092a1d3536771dec6e1"
    }
  },
  "functions": {
    "main": {
      "actions": [
        {
          "kind": "pndiff",
          "target": { "kind": "arg", "index": 0 },
          "state": "number"
        }
      ]
    }
  }
}
```

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-extra-actions-pass.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-extra-actions-pass.out.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 成功打印 `Applying MLsub extra constraints to main`
- 流程继续完成

### 3.3 反例

构造：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "content_hash": {
      "algorithm": "md5",
      "value": "1cb71d8840454092a1d3536771dec6e1"
    }
  },
  "functions": {
    "main": {
      "actions": [
        { "kind": "subtype" }
      ]
    }
  }
}
```

结果：

- 在校验阶段报错
- 错误位置为 `functions.main.actions[0]`
- 错误消息明确指出当前只支持 `kind = "pndiff"`

## 4. 后续建议

下一步最自然的是继续接：

1. 把 `target.kind` 从 `arg/ret` 扩到 `inst/operand`
2. 再补 `bindings`
3. 之后再实现 `actions.kind = subtype`

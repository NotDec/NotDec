# MLsub 额外约束 `subtype/equal` action 实现

日期：2026-04-17

## 1. 本次范围

这次继续只往前推一小步：在不引入 `inst/operand/binding` selector 的前提下，
先让 `NOTDEC_EXTRA_CONSTRAINTS` 的函数级 `actions` 支持：

1. `kind = "subtype"`
2. `kind = "equal"`
3. `lhs` / `rhs` operand 允许二选一：
   - `{ "type": ... }`
   - `{ "target": { "kind": "arg" | "ret", ... } }`

本次仍未实现：

- `bindings`
- `target.kind = "inst" | "operand" | "binding"`
- `SelectableValues.txt`

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:316](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L316)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:402](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L402)
   新增 extra-constraints operand 解析 helper：
   - `resolveExtraConstraintTarget()`
   - `validateExtraConstraintOperand()`
   - `buildExtraConstraintOperand()`

   这里把 `actions.lhs/rhs` 的最小 schema 定下来：
   - 必须是 object
   - 必须且只能出现 `type` / `target` 之一
   - `target.kind` 当前只接受 `arg` / `ret`

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:698](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L698)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:783](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L783)
   扩展 `validateExtraConstraintFunctions()`：
   - 保留现有 `pndiff` 校验
   - 新增 `subtype` / `equal` 的 `lhs/rhs` 必填校验
   - 新增 `lhs/rhs` operand 的浅层 schema 校验
   - 未识别 action 改为统一报
     `only actions.kind = 'pndiff', 'subtype', or 'equal' is supported currently`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:785](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L785)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:834](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L785)
   新增 `applyExtraConstraintSubtypeActions()`：
   - 复用 `MLsubRecovery::buildOverrideType()`
   - 对 `subtype` 注入单向约束 `lhs <: rhs`
   - 对 `equal` 注入双向约束
     - `lhs <: rhs`
     - `rhs <: lhs`

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1532](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1532)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1540](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1532)
   修改 `MLsubRecovery::applyExtraConstraints()`：
   - 先应用 `subtype/equal`
   - 再继续应用已有 `pndiff`

5. 同文件 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:365](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L365)
   对应的 `validateExtraConstraintOperand()` 和
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:383](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L383)
   对应的 `buildExtraConstraintOperand()` 目前仍只把 `target` 解析到
   当前函数边界值节点，因此这次提交仍不触碰函数体内 selector。

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   更新 `NOTDEC_EXTRA_CONSTRAINTS` 当前能力描述：
   - `actions.kind` 已支持 `pndiff | subtype | equal`
   - `target` 目前仍只支持 `arg` / `ret`

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 正例

先用 `test/type-recovery/llvm-ir/cases/01_Simple1.ll` 导出锚点，得到：

```json
{
  "content_hash": {
    "algorithm": "md5",
    "value": "1cb71d8840454092a1d3536771dec6e1"
  }
}
```

再构造：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "content_hash": {
      "algorithm": "md5",
      "value": "1cb71d8840454092a1d3536771dec6e1"
    },
    "data_layout": "e-m:e-p:32:32-i64:64-n32:64-S128",
    "target_triple": "wasm32-unknown-wasi"
  },
  "functions": {
    "main": {
      "actions": [
        {
          "kind": "subtype",
          "lhs": {
            "type": { "kind": "primitive", "name": "uint", "bits": 32 }
          },
          "rhs": {
            "target": { "kind": "arg", "index": 0 }
          }
        },
        {
          "kind": "equal",
          "lhs": {
            "target": { "kind": "arg", "index": 0 }
          },
          "rhs": {
            "target": { "kind": "arg", "index": 1 }
          }
        }
      ]
    }
  }
}
```

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-extra-subtype-equal.json \
./build/bin/notdec test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-extra-subtype-equal.out.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 成功打印 `Applying MLsub extra constraints to main`
- 流程继续完成

### 3.3 反例

构造缺少 `rhs` 的 `subtype` action：

```json
{
  "kind": "subtype",
  "lhs": {
    "target": { "kind": "arg", "index": 0 }
  }
}
```

结果：

- 在校验阶段直接报错退出
- 报错位置为 `functions.main.actions[0]`
- 报错消息为 `constraint action requires lhs and rhs`

## 4. 后续建议

下一步最自然的是继续接：

1. 把 `target.kind` 从 `arg/ret` 扩到 `inst/operand`
2. 再补 `bindings`
3. 如果要改善报错一致性，再把 extra-constraints 的 type expr 校验从
   “复用 override builder”进一步抽成独立错误域

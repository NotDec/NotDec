# MLsub 额外约束 `bindings` selector 实现

日期：2026-04-17

## 1. 本次范围

这次继续沿着 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
和 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
往前推一小步：先不实现 `inst/operand`，而是把函数级局部 selector 别名
`bindings` 接到现有的 `arg` / `ret` / `named_value` 之上。

本次新增能力：

1. `functions.<name>.bindings` 允许定义局部 selector 别名
2. `target.kind = "binding"` 可被：
   - `actions.kind = "pndiff"`
   - `actions.kind = "subtype"`
   - `actions.kind = "equal"`
   三条路径共同复用
3. `binding` 支持引用另一个 `binding`
4. 对循环引用会在校验阶段直接报错

本次仍未实现：

1. `target.kind = "inst"`
2. `target.kind = "operand"`
3. `SelectableValues.txt`

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:146](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L146)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:165](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L165)
   新增 `getExtraConstraintBindings()`：
   - 校验 `functions.<name>.bindings` 如果存在则必须是 object
   - 校验每个 binding entry 也必须是 object

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:312](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L312)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:442](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L312)
   重构 extra-constraints target 解析链路：
   - 新增 `resolveExtraConstraintTargetSelector()`
   - 统一处理 `arg` / `ret` / `named_value` / `binding`
   - `binding` 通过递归展开到底层 target
   - 用 `ResolvingBindings` 检测环，并在命中时报
     `binding cycle detected for '...'`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:444](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L444)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:488](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L444)
   修改：
   - `validateExtraConstraintOperand()`
   - `buildExtraConstraintOperand()`

   让 `subtype/equal` 的 `lhs/rhs` operand 在解析 `target` 时能访问当前函数
   的 `bindings`。

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:545](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L545)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:611](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L545)
   修改 `applyExtraConstraintPNDiffs()`：
   - 先取函数级 `bindings`
   - 再让 `pndiff` 的 target 也能走 `binding`

5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:786](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L786)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:872](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L786)
   修改 `validateExtraConstraintFunctions()`：
   - 校验 `bindings`
   - 让 `pndiff/subtype/equal` 在 schema 校验阶段就能解析 `binding`

6. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:874](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L874)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:924](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L874)
   修改 `applyExtraConstraintSubtypeActions()`：
   - 让 `subtype/equal` 真正应用约束时也复用同一份 `bindings`

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   更新 `NOTDEC_EXTRA_CONSTRAINTS` 当前能力描述：
   - `target` 已支持 `arg` / `ret` / `named_value` / `binding`
   - `binding` 明确表示引用函数级 `bindings` 段中的局部 selector 别名

### 2.3 规划文档同步

1. 在 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:42](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   到 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:53](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   把当前进展更新为：
   - `target.kind` 已包含 `binding`
   - 未实现项收敛为 `inst` / `operand` / `SelectableValues.txt`

2. 在 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:48](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   到 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:54](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   把 anchor 方案当前状态更新为：
   - `target.kind` 已支持 `binding`
   - 当前仍沿着轻量 selector 路线推进

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 正例

构造最小 LLVM IR：

```llvm
define internal i32* @main(i32* %p, i32 %b) {
entry:
  %next = getelementptr i32, i32* %p, i32 1
  ret i32* %next
}
```

并构造：

```json
{
  "functions": {
    "main": {
      "bindings": {
        "next_ptr": { "kind": "named_value", "name": "next" },
        "next_alias": { "kind": "binding", "name": "next_ptr" },
        "ret_ptr": { "kind": "ret" },
        "arg_ptr": { "kind": "arg", "index": 0 }
      },
      "actions": [
        {
          "kind": "equal",
          "lhs": { "target": { "kind": "binding", "name": "next_alias" } },
          "rhs": { "target": { "kind": "binding", "name": "ret_ptr" } }
        },
        {
          "kind": "subtype",
          "lhs": { "target": { "kind": "binding", "name": "arg_ptr" } },
          "rhs": { "target": { "kind": "binding", "name": "next_ptr" } }
        },
        {
          "kind": "pndiff",
          "target": { "kind": "binding", "name": "next_alias" },
          "state": "ptr"
        }
      ]
    }
  }
}
```

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-binding-check-ptr/extra-pass.json \
./build/bin/notdec /tmp/notdec-binding-check-ptr/input.ll \
  -o /tmp/notdec-binding-check-ptr/out-pass.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 成功打印 `Applying MLsub extra constraints to main`
- 流程继续完成

### 3.3 反例

构造循环 binding：

```json
{
  "bindings": {
    "a": { "kind": "binding", "name": "b" },
    "b": { "kind": "binding", "name": "a" }
  }
}
```

并让 action 引用 `a`。

结果：

- 在校验阶段直接报错退出
- 报错位置为 `bindings.b`
- 报错消息为 `binding cycle detected for 'a'`

## 4. 后续建议

下一步最自然的是继续接：

1. 把 `binding` 的底层 selector 从当前轻量版继续扩到 `inst`
2. 再补 `operand`
3. 如果需要改善可发现性，再补 `SelectableValues.txt`

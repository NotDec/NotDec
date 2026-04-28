# MLsub 额外约束 `named_value` 轻量 selector 实现

日期：2026-04-17

## 1. 本次范围

这次继续只往前推一小步：先不实现长期正式版的
`inst/operand/binding` selector，而是先把一个更轻量的函数内 value
定位方案接起来：

1. `target.kind = "named_value"`
2. 只在当前函数内查找
3. 只匹配有名字的、非 `void` 的 `Instruction`
4. 如果有多个同名值，按函数内遍历顺序取第一个

这次的目标是先把“函数体内具体 value 注入”真正打通，并让它能被：

1. `pndiff`
2. `subtype`
3. `equal`

三条 action 共享。

本次仍未实现：

- `target.kind = "inst"`
- `target.kind = "operand"`
- `target.kind = "binding"`
- `SelectableValues.txt`

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:146](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L146)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:162](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L146)
   新增 `findFirstNamedInstruction()`：
   - 按函数内基本块/指令顺序线性扫描
   - 跳过 `void` 指令
   - 跳过无名指令
   - 返回第一个 `I.getName() == name` 的结果值

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:291](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L291)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:352](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L291)
   修改 `resolveExtraConstraintPNDiffTarget()`：
   - 新增 `named_value` 分支
   - 读取 `target.name`
   - 解析成当前函数内的命名 instruction result
   - 解析失败时报：
     `named_value target '...' not found as a non-void instruction in function`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:354](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L354)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:415](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L354)
   修改 `resolveExtraConstraintTarget()`：
   - 为 `subtype/equal` 共用的 target 解析路径新增 `named_value`
   - 这样 `buildExtraConstraintOperand()` 就能直接复用这一入口

4. 因为 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:417](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L417)
   的 `validateExtraConstraintOperand()` 和
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:698](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L698)
   的 `validateExtraConstraintFunctions()` 本来就是通过
   `resolveExtraConstraintTarget()` /
   `resolveExtraConstraintPNDiffTarget()` 做 target schema 校验，所以这次不需要再额外复制一套 `named_value` 校验逻辑。

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   更新当前能力描述：
   - `target` 除了 `arg` / `ret` 外，开始支持轻量级 `named_value`
   - 语义明确为“当前函数里第一个同名非 `void` instruction result”

### 2.3 规划文档

1. 在 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:42](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   到 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:54](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   把当前进展更新为：
   - `target.kind = "arg" | "ret" | "named_value"`
2. 在 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:230](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L230)
   到 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:294](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L230)
   明确：
   - 近期轻量版走 `named_value`
   - 长期正式版 `inst/operand/binding` 先不实现
3. 在 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:48](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   到 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:53](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   把锚点规划更新为：
   - 当前已开始支持轻量级 `named_value`
   - 长期正式 selector 暂不实现
   - 即便先走名字匹配，`ir_anchor` 仍然保留

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
define internal i32 @main(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
```

先导出锚点，再构造：

```json
{
  "functions": {
    "main": {
      "actions": [
        {
          "kind": "subtype",
          "lhs": {
            "type": { "kind": "primitive", "name": "uint", "bits": 32 }
          },
          "rhs": {
            "target": { "kind": "named_value", "name": "sum" }
          }
        },
        {
          "kind": "equal",
          "lhs": {
            "target": { "kind": "named_value", "name": "sum" }
          },
          "rhs": {
            "target": { "kind": "ret" }
          }
        },
        {
          "kind": "pndiff",
          "target": { "kind": "named_value", "name": "sum" },
          "state": "number"
        }
      ]
    }
  }
}
```

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-named-value/extra-pass.json \
./build/bin/notdec /tmp/notdec-named-value/input.ll \
  -o /tmp/notdec-named-value/out-pass.ll \
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
  "kind": "pndiff",
  "target": { "kind": "named_value", "name": "missing" },
  "state": "number"
}
```

结果：

- 在校验阶段直接报错退出
- 报错位置为 `functions.main.actions[0].target`
- 报错消息为
  `named_value target 'missing' not found as a non-void instruction in function`

## 4. 后续建议

下一步最自然的是继续接：

1. 给 `named_value` 加最基础的调试导出，比如在 workdir 列出当前函数可选名字
2. 之后再决定是否值得继续做 `inst`
3. 如果 `named_value` 很快暴露出局限，再转去实现 `operand` / `binding`

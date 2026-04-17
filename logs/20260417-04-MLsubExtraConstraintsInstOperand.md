# MLsub 额外约束 `inst/operand` selector 实现

日期：2026-04-17

## 1. 本次范围

这次继续沿着 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
和 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
往前推一小步：不引入新的 stable-id parser，而是在当前函数内线性扫描
instruction，把 JSON 中的 stable id 直接和 `toStableString(...)` 做比较，
先把正式 selector 里的 `inst/operand` 路径接起来。

本次新增能力：

1. `target.kind = "inst"`
2. `target.kind = "operand"`
3. `binding` 现在也可以递归引用 `inst/operand`
4. `pndiff/subtype/equal` 三条 action 全部共享这条 selector 解析路径

本次仍未实现：

1. `SelectableValues.txt`
2. 更完整的 selector discoverability / 调试导出

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:167](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L167)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:177](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L167)
   新增 `findInstructionByStableId()`：
   - 遍历当前函数全部 basic block / instruction
   - 对每条 instruction 计算 `toStableString(ExtValuePtr(&I))`
   - 和 JSON 里的 stable id 做字符串比较

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:375](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L375)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:423](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L375)
   扩展 `resolveExtraConstraintTargetSelector()`：
   - 新增 `inst` 分支
   - 读取 `target.id`
   - 要求该 instruction 存在且非 `void`
   - 新增 `operand` 分支
   - 读取 `target.inst` 与 `target.index`
   - 先按 stable id 找到 instruction
   - 再把 operand 折叠成 `getExtValuePtr(Operand, Inst, Index)`，这样常量
     operand 也能保留 use-site 语义

3. 同一函数 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:429](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L429)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:446](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L429)
   的 `resolveExtraConstraintPNDiffTarget()` 与
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:448](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L448)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:463](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L448)
   的 `resolveExtraConstraintTarget()` 不需要单独再加新逻辑，因为两者都已
   统一走 `resolveExtraConstraintTargetSelector()`。

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:441](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L441)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:446](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L441)
   修改未识别 selector 的报错：
   - `pndiff` 路径现在会明确列出
     `arg/ret/named_value/inst/operand/binding`
   - `subtype/equal` 路径也同步列出同一组 kind

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   更新 `NOTDEC_EXTRA_CONSTRAINTS` 当前能力说明：
   - `target` 已支持 `inst` / `operand`
   - `inst.id` 与 `operand.inst` 使用 `toStableString()` 风格的 instruction id
   - `operand` 明确表示某条 instruction 的第 `index` 个 operand use-site

### 2.3 规划文档同步

1. 在 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:42](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   到 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md:52](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md#L42)
   把当前进展更新为：
   - `target.kind` 已包含 `inst` / `operand`
   - 当前剩余未实现项收敛为 `SelectableValues.txt` 和 discoverability

2. 在 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:48](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   到 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md:54](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md#L48)
   把 anchor 方案当前状态更新为：
   - 轻量 selector 已从 `named_value` 扩到 `inst/operand/binding`
   - 下一步更适合补 selector discoverability，而不是继续改 anchor 语义

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
define internal i32* @main(i32* %p, i32 %n) {
entry:
  %0 = sext i32 %n to i64
  %next = getelementptr i32, i32* %p, i64 %0
  ret i32* %next
}
```

其中 `02-mlsub-input.ll` 中对应的 stable id 为：

1. `main::%next`
2. `operand(inst = "main::%next", index = 0/1)`

再构造：

```json
{
  "functions": {
    "main": {
      "actions": [
        {
          "kind": "equal",
          "lhs": { "target": { "kind": "inst", "id": "main::%next" } },
          "rhs": { "target": { "kind": "ret" } }
        },
        {
          "kind": "subtype",
          "lhs": { "target": { "kind": "arg", "index": 0 } },
          "rhs": {
            "target": {
              "kind": "operand",
              "inst": "main::%next",
              "index": 0
            }
          }
        },
        {
          "kind": "pndiff",
          "target": {
            "kind": "operand",
            "inst": "main::%next",
            "index": 1
          },
          "state": "number"
        }
      ]
    }
  }
}
```

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-inst-operand/extra-pass.json \
./build/bin/notdec /tmp/notdec-inst-operand/input.ll \
  -o /tmp/notdec-inst-operand/out-pass.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 成功打印 `Applying MLsub extra constraints to main`
- 流程继续完成

### 3.3 反例

构造越界 operand：

```json
{
  "kind": "pndiff",
  "target": {
    "kind": "operand",
    "inst": "main::%next",
    "index": 9
  },
  "state": "number"
}
```

结果：

- 在校验阶段直接报错退出
- 报错位置为 `functions.main.actions[0].target`
- 报错消息为 `operand target index out of range`

## 4. 后续建议

下一步最自然的是继续接：

1. 导出 `SelectableValues.txt`
2. 在 workdir 里把当前函数可用的 stable instruction id / operand selector 列出来
3. 如果后续再需要，再考虑做更强的 selector 解析或专门 parser

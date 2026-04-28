# MLsub 名字版 operand selector 与 `SelectableValues.txt`

日期：2026-04-17

## 1. 本次范围

这次把 selector 这一段再往前收一小步：

1. 在现有“按名字定位 instruction result”的轻量路径上，补 `operand(name=..., index=...)`
2. 在 workdir 中导出 `SelectableValues.txt`

做完这两点之后，当前这条 selector 线的基础可用性就差不多够用了：

1. `named_value`
2. `inst`
3. `operand`
4. `binding`
5. `SelectableValues.txt`

本次没有继续做新的 selector 种类，而是把“怎么更方便地写现有 selector”
这部分补完整。

## 2. 具体修改

### 2.1 `src/TypeRecovery/mlsub/MLsubGenerator.cpp`

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:49](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L49)
   新增 workdir 调试文件常量：
   - `kSelectableValuesFile = "SelectableValues.txt"`

2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:198](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L198)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:241](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L241)
   新增 `resolveOperandInstructionSelector()`：
   - `operand` 现在要求二选一：
     - `inst`
     - `name`
   - `inst` 继续走 stable-id
   - `name` 复用 `findFirstNamedInstruction()`
   - 如果两者同时存在或同时缺失，直接报：
     `operand target must contain exactly one of 'inst' or 'name'`

3. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:490](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L490)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:508](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L508)
   修改 `resolveExtraConstraintTargetSelector()` 的 `operand` 分支：
   - 不再强制只接受 `inst`
   - 改为调用 `resolveOperandInstructionSelector()`
   - 继续把 operand 折叠成 `getExtValuePtr(Operand, Inst, Index)`，保留 use-site 语义

4. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:304](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L304)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:349](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L349)
   新增 `writeSelectableValues()`：
   - 遍历当前模块内的已定义函数
   - 输出 `arg` / `ret`
   - 输出所有 instruction 的 stable id
   - 对有名非 `void` instruction 额外输出 `named_value(name) -> stable-id`
   - 对每条 instruction 输出：
     - `operand(inst="...", index=N)`
     - 如果该 instruction 有名字，再额外输出
       `operand(name="...", index=N)`

5. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1390](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1390)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1396](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1390)
   修改 `MLsubRecovery::run()`：
   - 在导出 `02-mlsub-input.ll` 与 `02-mlsub-input.anchor.json` 后
   - 同步导出 `SelectableValues.txt`

### 2.2 `DEBUG.md`

1. 在 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L77)
   补充 `operand` 当前两种写法：
   - `operand(inst="...", index=N)`
   - `operand(name="...", index=N)`

2. 在 [DEBUG.md](/sn640/NotDec/DEBUG.md)
   新增 `SelectableValues.txt` 小节，说明它的来源、用途和当前格式。

### 2.3 规划文档同步

1. 在 [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
   把当前进展更新为：
   - `operand` 已支持 `inst` / `name`
   - `SelectableValues.txt` 已落地

2. 在 [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)
   把当前状态更新为：
   - selector 基础链路已落地
   - 现在主要剩 discoverability / polish

## 3. 验证

### 3.1 编译

执行：

```bash
cmake --build ./build --target notdec-decompile
```

结果：通过。

### 3.2 `SelectableValues.txt`

对最小输入：

```llvm
define internal i32* @main(i32* %p, i32 %n) {
entry:
  %0 = sext i32 %n to i64
  %next = getelementptr i32, i32* %p, i64 %0
  ret i32* %next
}
```

执行：

```bash
./build/bin/notdec /tmp/notdec-named-operand/input.ll \
  -o /tmp/notdec-named-operand/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-named-operand/work
```

结果：workdir 中成功出现：

- `/tmp/notdec-named-operand/work/SelectableValues.txt`

并包含：

```text
main::%next
named_value(next) -> main::%next
operand(inst="main::%next", index=0) -> main::arg0
operand(name="next", index=0) -> main::arg0
```

### 3.3 正例

构造：

```json
{
  "kind": "operand",
  "name": "next",
  "index": 1
}
```

并把它用于 `pndiff` / `subtype` action。

执行：

```bash
NOTDEC_EXTRA_CONSTRAINTS=/tmp/notdec-named-operand/extra-pass.json \
./build/bin/notdec /tmp/notdec-named-operand/input.ll \
  -o /tmp/notdec-named-operand/out-pass.ll \
  --tr-level=2
```

结果：

- 成功打印 `Loading MLsub extra constraints from: ...`
- 成功打印 `Applying MLsub extra constraints to main`
- 流程继续完成

### 3.4 反例

构造：

```json
{
  "kind": "operand",
  "name": "missing",
  "index": 0
}
```

结果：

- 在校验阶段直接报错退出
- 报错位置为 `functions.main.actions[0].target`
- 报错消息为
  `operand target name 'missing' not found as a non-void instruction in function`

## 4. 后续建议

selector 这一段当前已经比较完整，后续更适合转去做别的方向；如果还要继续补，
优先级更高的也应该是：

1. `SelectableValues.txt` 的格式 polish
2. 更一致的 selector 规范化报错
3. 真正需要时再考虑更正式的 stable-id parser

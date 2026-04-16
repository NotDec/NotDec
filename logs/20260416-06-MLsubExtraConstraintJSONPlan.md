# MLsub 额外 JSON 约束注入方案

日期：2026-04-16

## 1. 结论

当前 `MLsub` 已经有两类函数边界 override：

1. `NOTDEC_SUMMARY_OVERRIDE`
   - 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:345](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L345) 持有文档与函数集合
   - 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:895](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L895) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:956](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L956) 应用
   - 方向是 `OverrideTy <: F`
2. `NOTDEC_SIGNATURE_OVERRIDE`
   - 在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:341](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L341) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:345](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L345) 持有文档与函数集合
   - 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:958](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L958) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1019](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1019) 应用
   - 方向是 `F <: OverrideTy`

但这两条路径当前都只适合“函数边界”的类型约束与 `pndiff` 边界状态约束，不适合继续硬扩成“任意 SSA value / 任意 use-site 的额外约束注入”。

我建议新增第三条独立入口：

- `NOTDEC_EXTRA_CONSTRAINTS`

并单独设计一个 JSON schema，专门表达：

1. 某个函数里的具体 value / instruction / operand selector
2. 对这些 selector 之间补充的额外 subtype / equality / `pndiff` 约束
3. 必要时对 selector 起局部别名，避免重复写长路径

这样可以同时满足：

1. 保持现有 `summary/signature` 语义清晰
2. 支持“具体 value，比如某个指令结果或某个 operand”的约束注入
3. 复用当前 `buildOverrideType()` 已经实现的 type expr 解析能力

## 1.1 当前进展

截至 2026-04-16，这份方案里已经有一部分最小实现落地：

1. 已新增环境变量入口 `NOTDEC_EXTRA_CONSTRAINTS`
2. 已支持在 `MLsubRecovery::run()` 中读取 JSON 并校验 `ir_anchor`
3. 已支持最小函数级 action 子集：
   - `functions.<name>.actions[*].kind = "pndiff"`
   - `target.kind = "arg" | "ret"`
   - `state = "ptr" | "number"`
4. 已在 `MLsubRecovery::bottomUpPhase()` 中于 `G->run()` 之后真正应用这批
   extra constraints

当前仍未实现的部分包括：

1. `bindings`
2. `target.kind = "inst" | "operand" | "binding"`
3. `actions.kind = "subtype" | "equal"`
4. `SelectableValues.txt`

相关实现日志：

1. [logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md](/sn640/NotDec/logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md)
2. [logs/20260416-10-MLsubExtraConstraintsPndiffActions.md](/sn640/NotDec/logs/20260416-10-MLsubExtraConstraintsPndiffActions.md)

## 2. 当前代码证据

### 2.1 现有 override 的类型表达式只会构造“抽象类型”，不会定位 IR value

当前 type expr builder 在：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:756](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L756) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:885](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L885)
  - `MLsubRecovery::buildOverrideType()`

它当前支持的 `kind` 只有：

1. `primitive`
2. `var`
3. `ref`
4. `ptr`
5. `record`

这说明它适合构造“右侧或左侧的类型项”，但还没有“把 JSON 目标解析到现有图节点”的能力。

### 2.2 现有 summary / signature 应用点天然是“函数边界后置注入”

当前 bottom-up 里：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1021](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1021) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1045](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1045)
  - `MLsubRecovery::bottomUpPhase()`
- 同一函数后续会先 `G->run()`，再应用 summary/signature override

这意味着“额外约束注入”的最佳时机仍然是：

1. `G->run()` 之后
2. `unhandledCalls` 实例化之前

因为这时函数内已有：

1. 参数节点
2. 返回节点
3. 指令结果节点
4. PNDiff 图节点

### 2.3 当前仓库已经有稳定 value id，可以作为 JSON selector

`ExtValuePtr` 的稳定字符串格式定义在：

- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:45](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp#L45)
  - `formatFunctionId()`
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:81](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp#L81)
  - `formatBlockId()`
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:99](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp#L99)
  - `formatInstructionId()`
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:122](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp#L122)
  - `formatValueRef()`
- [external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp:285](/sn640/NotDec/external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp#L285)
  - `toStableString()`

这里已经能稳定输出：

1. `main::arg0`
2. `main::<ret>`
3. `main::%bb.entry.i7`
4. `const(i32 4)@main::%bb.entry.i7:1`

所以新 schema 不应该依赖 `ValueNamer` 生成的运行期临时名，而应该直接复用 `toStableString()` 一致的外显格式。

### 2.4 operand/use-site 是必须支持的

当前 `ConstraintsGenerator` 内部不是直接用 `llvm::Value *` 做 key，而是用：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:123](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L123) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:142](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L142)
  - `createNode(ExtValuePtr Val, llvm::User *User, long OpInd)`
- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:145](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L145) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:161](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L161)
  - `getNodeOrNull(...)`
- [external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:72](/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h#L72) 到 [external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h:104](/sn640/NotDec/external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h#L72)
  - `ExtValuePtr`
  - `llvmValue2ExtVal(...)`

这意味着：

1. 某些常量的类型节点是按 use-site 区分的
2. “只按常量值定位”是不够的
3. 第一版 selector 必须支持 `operand`

## 3. 设计目标

### 3.1 目标

新入口需要能表达下面三类需求：

1. 给函数边界外的某个 value 直接加类型约束
2. 给某个具体 operand/use-site 加 `pndiff` 状态
3. 给两个具体 value 之间补充显式关系

### 3.2 非目标

第一版不建议支持：

1. 跨函数的全局 alias 解析
2. 模糊匹配指令文本
3. 基于 `ValueNamer` 的临时名 selector
4. union / intersection / recursive type expr
5. 自动重写 IR

## 4. 推荐的顶层结构

我建议新增一份独立文档，而不是继续把字段塞进 `summary/signature`：

```json
{
  "version": 1,
  "functions": {
    "main": {
      "bindings": {
        "len_add": {
          "kind": "inst",
          "id": "main::%bb.entry.i7"
        },
        "arg_n": {
          "kind": "arg",
          "index": 1
        }
      },
      "actions": [
        {
          "kind": "subtype",
          "lhs": {
            "type": { "kind": "primitive", "name": "uint", "bits": 32 }
          },
          "rhs": {
            "target": { "kind": "binding", "name": "len_add" }
          }
        },
        {
          "kind": "pndiff",
          "target": { "kind": "binding", "name": "len_add" },
          "state": "number"
        }
      ]
    }
  }
}
```

推荐理由：

1. 顶层仍然按 `functions` 划作用域，和现有 override 习惯一致
2. `bindings` 专门给 selector 起别名
3. `actions` 专门表达“要注入什么约束”
4. `type` 和 `target` 显式分离，不会把“抽象类型项”和“现有 IR value”混在一个字段里

## 5. selector 设计

### 5.1 第一版支持的 `target.kind`

第一版建议只支持：

1. `arg`
2. `ret`
3. `inst`
4. `operand`
5. `binding`

分别表示：

1. 当前函数的第 `index` 个参数
2. 当前函数返回值
3. 某条非 `void` 指令的结果值
4. 某条指令的第 `index` 个 operand，在内部解析成 use-site 级 `ExtValuePtr`
5. 引用 `bindings` 里的局部别名

### 5.2 推荐格式

#### 参数和返回值

```json
{ "kind": "arg", "index": 0 }
{ "kind": "ret" }
```

#### 指令结果

```json
{ "kind": "inst", "id": "main::%bb.entry.i7" }
```

这里的 `id` 应当和 `toStableString()` 输出对齐。

#### operand/use-site

```json
{ "kind": "operand", "inst": "main::%bb.entry.i7", "index": 1 }
```

它表示：

1. 先定位这条 instruction
2. 再取第 `index` 个 operand
3. 最终通过 `llvmValue2ExtVal(Val, Inst, index)` 折叠成内部 `ExtValuePtr`

这是第一版支持具体常量约束的关键。

#### 绑定别名

```json
{ "kind": "binding", "name": "len_add" }
```

### 5.3 为什么不建议用 `ValueNamer`

`ValueNamer` 的名字来自运行期补名，适合打印，不适合长期 JSON 配置。

相比之下，`toStableString()` 这一路已经显式考虑了：

1. 无名函数
2. 无名 block
3. 无名 instruction
4. use-site 包装常量

所以 selector 应统一以 `toStableString()` 风格为权威外显 id。

## 6. action 设计

### 6.1 第一版支持的 `action.kind`

建议第一版只支持：

1. `subtype`
2. `equal`
3. `pndiff`

### 6.2 `subtype`

```json
{
  "kind": "subtype",
  "lhs": { "type": { "kind": "primitive", "name": "uint", "bits": 32 } },
  "rhs": { "target": { "kind": "binding", "name": "len_add" } }
}
```

语义：

- `lhs <: rhs`

其中 `lhs/rhs` 都是一个 `ConstraintOperand`，只能二选一：

1. `{ "type": ... }`
2. `{ "target": ... }`

### 6.3 `equal`

```json
{
  "kind": "equal",
  "lhs": { "target": { "kind": "arg", "index": 0 } },
  "rhs": { "target": { "kind": "binding", "name": "len_add" } }
}
```

语义：

- 双向约束
  - `lhs <: rhs`
  - `rhs <: lhs`

这样可以避免在 schema 层引入 binarysub 当前没有直接公开的“等式”原语。

### 6.4 `pndiff`

```json
{
  "kind": "pndiff",
  "target": { "kind": "operand", "inst": "main::%bb.entry.i7", "index": 1 },
  "state": "number"
}
```

语义：

1. 先解析 `target`
2. 再对其 PNDiff 节点设置：
   - `ptr`
   - `number`

这里可以直接复用当前：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:145](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L145)
  - `parsePNDiffState()`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:192](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L192)
  - `applyPNDiffOverrides()`

只是把它从“仅支持函数边界 target”扩成“通用 resolved target”。

## 7. 类型表达式复用策略

新 schema 不应再发明第二套 type expr。

应直接复用当前：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:756](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L756)
  - `MLsubRecovery::buildOverrideType()`

也就是说：

1. `primitive`
2. `var`
3. `ref`
4. `ptr`
5. `record`

这一套仍然保留不变。

区别只在于：

1. 旧 `summary/signature` 用它来构造函数边界类型
2. 新 `extra constraints` 用它来构造 action 里的 `type` operand

因此第一版推荐新增：

1. `ResolvedConstraintTarget`
2. `ConstraintOperandRecipe`

其中：

1. `ConstraintOperandRecipe`
   - 要么持有 `SimpleType`
   - 要么持有 `ExtValuePtr`
2. 当 operand 是 `target` 时，再调用 `G.getNodeOrNull(...)`
3. 当 operand 是 `type` 时，直接走现有 `buildOverrideType()`

## 8. 推荐实现位置

### 8.1 头文件

建议在：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340) 到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:396](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L396)

附近新增：

1. `const char *ExtraConstraintFile = std::getenv("NOTDEC_EXTRA_CONSTRAINTS");`
2. `llvm::json::Value ExtraConstraintDoc = nullptr;`
3. `std::set<llvm::Function *> ExtraConstraintFuncs;`
4. `ResolvedConstraintTarget`
5. `ConstraintOperandRecipe`
6. `loadExtraConstraintFile(...)`
7. `getExtraConstraintSpec(...)`
8. `resolveConstraintTarget(...)`
9. `buildConstraintOperand(...)`
10. `applyExtraConstraintActions(...)`

当前已完成其中一部分：

1. 已在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:342](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L342)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:348](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L342)
   新增：
   - `const char *ExtraConstraintsFile`
   - `llvm::json::Value ExtraConstraintsDoc`
   - `std::set<llvm::Function *> ExtraConstraintsFuncs`
2. 已在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:384](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L384)
   到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:403](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L384)
   新增：
   - `validateExtraConstraintsFile(...)`
   - `getExtraConstraintsSpec(...)`
   - `applyExtraConstraints(...)`

### 8.2 loader 初始化

建议在：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:663](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L663) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:676](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L676)

附近加入：

1. 初始化 `ExtraConstraintDoc`
2. 读取 `NOTDEC_EXTRA_CONSTRAINTS`
3. 执行和现有 override 同风格的严格校验

当前已完成：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1066](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1066)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1094](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1066)
   实现 `MLsubRecovery::validateExtraConstraintsFile()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:885](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L885)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:899](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L885)
   的 `MLsubRecovery::run()` 中接入：
   - 当前模块文本摘要计算
   - `NOTDEC_EXTRA_CONSTRAINTS` 加载与严格校验

### 8.3 应用点

建议在：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1021](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1021)
  - `MLsubRecovery::bottomUpPhase()`

里保持当前顺序：

1. `G->run()`
2. 应用 `summary override`
3. 应用 `signature override`
4. 应用 `extra constraints`
5. 再处理 `unhandledCalls`

这样有几个好处：

1. 目标 value 节点大多已经建立
2. 额外约束能看到 summary/signature 已经注入后的边界信息
3. 不会改变现有两类 override 的优先级

当前实现与原文顺序略有差异：

1. 已在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1416](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1416)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1421](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1416)
   的 `MLsubRecovery::bottomUpPhase()` 中接入 `extra constraints`
2. 当前顺序是：
   - `G->run()`
   - `extra constraints`
   - `summary override`
   - `signature override`
   - `unhandledCalls`
3. 之所以先放在 summary/signature 前，是因为当前只实现了函数内 `pndiff`
   action，这部分不依赖边界 type expr 注入

## 9. 推荐的解析与应用流程

### 9.1 loader 阶段

对每个 `functions.<name>`：

1. 校验 `bindings` 为对象，可选
2. 校验 `actions` 为数组，必选
3. `bindings` 只做 schema 检查，不提前解析到 LLVM value
4. `actions` 只做结构检查，不提前依赖具体 SCC 图

当前已完成：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:602](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L602)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:671](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L602)
   实现 `validateExtraConstraintFunctions()`
2. 当前 loader 已支持：
   - `functions` 顶层对象
   - `functions.<name>.actions` 数组
   - `actions[*].kind = "pndiff"`
   - `target.kind = "arg" | "ret"`
3. 当前仍未支持：
   - `bindings`
   - `inst`
   - `operand`
   - `binding`
   - `subtype`
   - `equal`

### 9.2 应用阶段

在 `applyExtraConstraintActions(ConstraintsGenerator &G, llvm::Function &Func, const llvm::json::Value &Spec)` 中：

1. 构造单函数局部上下文
   - selector bindings map
   - type vars map
2. 先解析 `bindings`
3. 再遍历 `actions`
4. 对 `subtype/equal`：
   - 左右各自解析成 `ConstraintOperandRecipe`
   - 必要时取 `G.getNodeOrNull(...)`
   - 调用 `G.addSubtype(...)`
5. 对 `pndiff`：
   - 解析 target
   - 取 `G.PG.getPNIVarOrNull(...)`
   - 设置 `ptr/number`

当前已完成这条最小子集：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:362](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L362)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:434](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L362)
   实现 `applyExtraConstraintPNDiffs()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1370](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1370)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1377](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1370)
   实现 `MLsubRecovery::applyExtraConstraints()`
3. 当前应用阶段只会处理 `pndiff`

### 9.3 selector 解析失败策略

建议第一版保持和现有 override 一致的严格风格：

1. 找不到函数时 warning 并 skip
2. 找到函数但 selector 无法解析时直接报错并终止
3. selector 解析到了值，但对应图节点不存在时直接报错
4. 对 `void` 指令使用 `inst` selector 时直接报错

## 10. 调试与可观测性

为了让用户能写这份 JSON，我建议顺手补一个调试文件：

- `SelectableValues.txt`

推荐在每个 workdir 输出：

1. 当前函数参数
2. 当前函数返回值
3. 当前函数所有指令结果的 stable id
4. 对每条指令额外列出 operand selector 示例

例如：

```text
main::arg0
main::<ret>
main::%bb.entry.i7
main::%bb.entry.i7:operand:0 -> const(i32 4)@main::%bb.entry.i7:0
main::%bb.entry.i7:operand:1 -> main::arg1
```

这能显著降低用户手写 selector 的成本，也便于回归测试。

## 11. 分阶段实施建议

### 阶段 1：最小可用版

先支持：

1. 新环境变量 `NOTDEC_EXTRA_CONSTRAINTS`
2. `target.kind`
   - `arg`
   - `ret`
   - `inst`
   - `operand`
   - `binding`
3. `action.kind`
   - `subtype`
   - `equal`
   - `pndiff`
4. 复用现有 `buildOverrideType()`

这是最有价值、也最贴合当前架构的一版。

当前状态：

1. 这一阶段已完成其中一半左右
2. 已落地：
   - `NOTDEC_EXTRA_CONSTRAINTS`
   - `pndiff`
   - `target.kind = "arg" | "ret"`
3. 尚未落地：
   - `inst`
   - `operand`
   - `binding`
   - `subtype`
   - `equal`
   - `buildOverrideType()` 在 extra constraints 中的复用

### 阶段 2：调试辅助

增加：

1. `SelectableValues.txt`
2. selector 冲突与解析失败的更明确报错
3. 在 stderr 中打印 `Applying MLsub extra constraints to ...`

### 阶段 3：再考虑扩展 selector

只有在第一版证明不够用时，再考虑：

1. `global`
2. `stack_object`
3. `heap_object`
4. callsite-specific shortcut selector

这些都不应该进入第一版。

## 12. 风险与取舍

### 12.1 最大风险不是 type expr，而是 selector 稳定性

如果 selector 用的是 `ValueNamer` 或指令文本模糊匹配：

1. 优化一变就会失效
2. 报错难以理解
3. JSON 很难长期维护

所以第一版必须统一用 stable id。

### 12.2 operand selector 是必要但实现要谨慎

因为常量会被包装成 use-site 相关的 `ExtValuePtr`，所以必须支持 operand。

但也意味着：

1. selector 解析逻辑必须明确依赖 `inst + index`
2. 不能偷懒只按 operand 的 printed value 匹配

### 12.3 不建议把这套 schema 混进 `summary/signature`

如果继续把“具体 value 约束”塞回 `summary/signature`：

1. 一个文件里会同时混有函数边界语义和函数内局部语义
2. `OverrideTy <: F` / `F <: OverrideTy` 两种方向会和局部 `value-to-value` 约束混在一起
3. 文档与报错都更难解释

所以我更推荐：

1. `summary` 负责 lower-bound 函数边界
2. `signature` 负责 upper-bound 函数边界
3. `extra constraints` 负责函数内或具体 value 约束

## 13. 推荐验证方式

第一版落地后，建议至少覆盖下面几类样例：

1. 给某个 `add` 指令结果补 `uint32 <: value`
2. 给某个 `icmp` operand 补 `pndiff = number`
3. 给某个 use-site 常量补 `pndiff = ptr`
4. 用 `equal` 把某条指令结果和 `arg0` 绑定
5. selector 指向 `void call` 时报错
6. selector 指向不存在的 stable id 时报错

推荐把这些都放进：

- `test/type-recovery/llvm-ir/`

并优先验证：

1. stderr 报错信息
2. `03-pndiff-final.ll`
3. `ValueTypes.txt`
4. 新增的 `SelectableValues.txt`

## 14. 本次文档变更

本次先新增方案文档，后续已有对应实现落地：

- [logs/20260416-06-MLsubExtraConstraintJSONPlan.md](/sn640/NotDec/logs/20260416-06-MLsubExtraConstraintJSONPlan.md)
- [logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md](/sn640/NotDec/logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md)
- [logs/20260416-10-MLsubExtraConstraintsPndiffActions.md](/sn640/NotDec/logs/20260416-10-MLsubExtraConstraintsPndiffActions.md)

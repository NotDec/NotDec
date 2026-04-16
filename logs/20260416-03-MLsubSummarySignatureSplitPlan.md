# MLsub Summary / Signature Override Split Plan

日期：2026-04-16

## 1. 结论

当前 `MLsubRecovery::applySignatureOverride()` 里这条：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:776](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L776)
  - `G.addSubtype(binarysub::make_function(Args, RetRecipe.Root), FuncNode);`

语义上更接近“函数边界下界约束”或“summary override”，而不是“修改函数自身签名的 upper-bound signature override”。

因此把当前这一路拆成两种 override，我认为是对的：

1. `NOTDEC_SUMMARY_OVERRIDE`
   - 用于外部库函数、声明函数、以及其它“把某个函数当成 summary / lower-bound 来用”的场景
   - 约束方向保持：
     - `OverrideTy <: F`
2. `NOTDEC_SIGNATURE_OVERRIDE`
   - 用于有函数体的函数边界，尤其是入口函数、`main`、导出 API 等“想直接收紧函数自身边界”的场景
   - 约束方向改为：
     - `F <: OverrideTy`

我建议后续文档和实现都明确把这两种语义分开，不再把当前 lower-bound 语义继续叫作 `SIGNATURE_OVERRIDE`。

## 2. 当前代码证据

### 2.1 当前函数节点的默认关系本身就是下界建模

在 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:84](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L84)
到 [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:100](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L100) 的 `ConstraintsGenerator::run()` 里：

1. 先为每个函数建立：
   - 函数节点 `F`
   - 参数节点
   - 返回节点
2. 然后执行：
   - `addSubtype(binarysub::make_function(Args, Ret), F);`

这表示当前 MLsub 里，函数节点 `F` 接受一个来自“函数结构”的下界约束。

所以 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:776](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L776)
的当前 override 本质上是在给 `F` 再补一个 lower bound。

### 2.2 当前 call 约束也说明 `F` 更像被调用时的被约束对象

在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1545](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1545)
到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1589](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1589)
的 `ConstraintsGenerator::MLsubVisitor::visitCallBase()` 里：

1. 调用点构造：
   - `ActualFunc = binarysub::make_function(Args, Ret)`
2. 若目标函数在当前 SCC：
   - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1585](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1585)
     - `cg.addSubtype(F, ActualFunc);`
3. 若目标函数不在当前 SCC：
   - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1588](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1588)
     - `cg.unhandledCalls.insert({&I, ActualFunc});`

这说明：

1. 对“有函数体并参与当前求解”的函数，`F` 会被调用点 usage 约束
2. 对“没有可用函数体 / 当前不能内联求解”的函数，调用点只留下一个 `ActualFunc`

从这个角度看，当前 override 若直接作用在 `F` 上，确实更像“把外部函数 / summary 的最小能力灌给它”。

### 2.3 当前 loader 名字和语义已经开始错位

现在 MLsub 里：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:340](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L340)
  - `SigFile = std::getenv("NOTDEC_SIGNATURE_OVERRIDE")`
- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:375](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L375)
  - `loadSignatureFile(...)`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:490](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L490)
  - `MLsubRecovery::loadSignatureFile(...)`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:717](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L717)
  - `MLsubRecovery::applySignatureOverride(...)`

但应用点依然是：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:776](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L776)
  - `OverrideTy <: F`

所以当前名字已经暗示“函数签名 override”，但实现语义仍是“summary lower bound”。

## 3. 旧路径里的对应分工

旧 retypd 路径里其实已经有这两类东西的分离：

- [include/notdec/Passes/ConstraintGenerator.h:138](/sn640/NotDec/include/notdec/Passes/ConstraintGenerator.h#L138)
  - `SummaryOverride`
- [include/notdec/Passes/ConstraintGenerator.h:140](/sn640/NotDec/include/notdec/Passes/ConstraintGenerator.h#L140)
  - `SignatureOverride`

并且它们的使用位置也不同：

1. bottom-up summary 侧：
   - [src/Passes/ConstraintGenerator.cpp:661](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L661)
     到
     [src/Passes/ConstraintGenerator.cpp:739](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L739)
   - 外部函数 / external call 主要走 `SummaryOverride`
2. top-down signature 侧：
   - [src/Passes/ConstraintGenerator.cpp:804](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L804)
     到
     [src/Passes/ConstraintGenerator.cpp:819](/sn640/NotDec/src/Passes/ConstraintGenerator.cpp#L819)
   - `SignatureOverride` 影响的是“函数最终 signature”的聚合结果

所以你现在提出的拆分，其实和旧路径的大方向是一致的，只是需要用 MLsub 的约束方向重新表述。

## 4. 推荐的新语义

### 4.1 `NOTDEC_SUMMARY_OVERRIDE`

推荐用途：

1. 外部库函数
2. declaration-only 函数
3. 默认 builtin libc / POSIX / regex JSON
4. 其它“我只想告诉求解器这个函数至少满足这个边界”的场景

推荐约束：

- `OverrideTy <: F`

推荐实现位置：

1. 继续放在 bottom-up
2. 放在当前 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:779](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L779)
   的 `MLsubRecovery::bottomUpPhase()`
3. 继续在 `G->run()` 之后、`unhandledCalls` 实例化之前灌入

推荐实现函数：

- 当前 `MLsubRecovery::applySignatureOverride()` 改名为：
  - `applySummaryOverride()`
- 当前 `loadSignatureFile()` / `SigFile` 的 lower-bound 这一路，迁到：
  - `SummaryFile`
  - `loadSummaryFile()`

### 4.2 `NOTDEC_SIGNATURE_OVERRIDE`

推荐用途：

1. `main`
2. 程序入口导出函数
3. 有函数体、但希望直接收紧参数/返回边界的函数
4. 调试某个函数恢复结果时，希望给它显式 upper-bound 的场景

推荐约束：

- `F <: OverrideTy`

原因：

1. 这才更符合“函数自己的签名不能比 override 更宽”的直觉
2. 对入口函数尤其成立，因为入口函数往往没有足够 caller usage 去约束参数
3. 这条约束和当前 summary lower-bound 方向正好相反，因此更应该单独命名

推荐实现位置：

1. 仍然可以放在 bottom-up
2. 但应用目标应限定为“有函数体的函数”
3. 第一版可只对：
   - `!Func.isDeclaration()`
   - 或者更窄一点，只对 `main` / 明确配置的入口函数

推荐实现函数：

- 新增：
  - `applyUpperBoundSignatureOverride()`
  - 或命名成：
    - `applyEntrySignatureOverride()`

## 5. 推荐的实现切分

### 阶段 1：仅重命名与语义拆分，不改 JSON schema

建议先做最小切分：

1. 当前 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:776](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L776)
   的 lower-bound 路径改名为 summary
2. 环境变量层面：
   - 当前 lower-bound 路径改读 `NOTDEC_SUMMARY_OVERRIDE`
3. 默认 builtin JSON 资源文件：
   - 也归到 summary 语义
4. 保留同一份 JSON schema：
   - 先不为 summary / signature 引入两套格式

这样可以先把名字和语义对齐。

### 阶段 2：新增 upper-bound signature override

新增一条平行路径：

1. `SigFile = std::getenv("NOTDEC_SIGNATURE_OVERRIDE")`
2. 新 loader：
   - 可先复用当前相同 JSON schema
3. 新应用点：
   - `G.addSubtype(FuncNode, binarysub::make_function(Args, RetRecipe.Root));`
4. 限制适用对象：
   - 第一版只对非 declaration 函数生效
   - 更保守的话只对入口函数生效

### 阶段 3：再决定是否需要 summary / signature 分开缓存

等第 1、2 阶段跑通后，再考虑数据结构层面是否分开：

1. `SummaryOverrideDoc`
2. `SignatureOverrideDoc`
3. `SummaryOverrideFuncs`
4. `SignatureOverrideFuncs`

我建议不要一开始就做太多抽象，先把约束方向拆开最重要。

## 6. 需要特别注意的风险

### 6.1 upper-bound 比 lower-bound 更容易和函数体内部约束冲突

`F <: OverrideTy` 的副作用会更强，因为它是在“收紧函数自身”。

如果 override 过窄，可能会导致：

1. 参数节点和函数体内部 usage 冲突
2. 返回值被压得过窄
3. 某些本来可解的 case 直接变成不可满足

所以我建议：

1. 第一版仅用于入口函数或少量白名单函数
2. 出错时打印出具体函数名和 override 来源

### 6.2 `main` 的 argc/argv 要注意不要把 `argv` 过度具体化

如果对 `main` 做 upper-bound signature override，常见直觉是：

- `argc: i32`
- `argv: char**`

这个方向是合理的，但 `argv` 的 store/load 形状会比普通 `char*` 更敏感，第一版最好只做：

- `ptr<load=ptr<load=i8, store=i8>, ...>`

不要一开始再叠过多 record / semantic primitive 约束。

## 7. 建议的后续文档动作

如果后面按这个方案实现，我建议同步更新：

1. [logs/20260414-02-MLsubSummarySignatureOverridePlan.md](/sn640/NotDec/logs/20260414-02-MLsubSummarySignatureOverridePlan.md)
   - 明确这是“旧第一版计划”，当前 lower-bound 语义应改名为 summary
2. [logs/20260416-02-MLsubBuiltinLibcSignatureFortune.md](/sn640/NotDec/logs/20260416-02-MLsubBuiltinLibcSignatureFortune.md)
   - 把“默认 builtin signature JSON”改表述为默认 builtin summary JSON
3. `AGENTS.md`
   - 调试环境变量说明里明确区分：
     - `NOTDEC_SUMMARY_OVERRIDE`
     - `NOTDEC_SIGNATURE_OVERRIDE`

## 8. 本轮结论

这轮不建议直接修改代码，先把语义定下来：

1. 当前 `OverrideTy <: F` 应归类为 summary，而不是 signature
2. 入口函数 / `main` 这种“想改函数自身边界”的需求，应新增反向约束路径 `F <: OverrideTy`
3. 这两种 override 不应该继续共用同一个名字

如果后续正式开改，我建议顺序是：

1. 先把当前 lower-bound 路径改名为 `NOTDEC_SUMMARY_OVERRIDE`
2. 再新增 upper-bound 的 `NOTDEC_SIGNATURE_OVERRIDE`
3. 第一版只对 `main` 这类入口函数开放 upper-bound signature override

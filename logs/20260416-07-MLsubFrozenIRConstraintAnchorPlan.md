# MLsub 约束锚点 IR 与 Provenance 方案

日期：2026-04-16

## 1. 结论

你提的方向我认为是对的：如果后续要让用户通过 JSON 给 `MLsub` 注入“具体 value / 指令”级约束，那么这份 JSON 最好不要只依赖函数名和 selector，还应该绑定一份“约束写作时所针对的 LLVM IR 身份”。

最直接的做法就是在 JSON 里保存：

1. 约束所针对的 IR 阶段名
2. 该阶段 IR 的 `sha256`
3. 必要时补充 `data_layout` / `target_triple`

而不是只看原始输入路径。

当前代码里，`MLsub` 入口前确实已经会把一份 IR 落到 workdir：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:681](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L681)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:683](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L683)

本次我把它从：

- `01-Optimized.ll`

改成了更明确的：

- `02-mlsub-input.ll`

因为从约束注入视角看，这份 IR 已经不是泛泛的 “optimized IR”，而是“`MLsub` 真正消费的输入 IR”。

## 1.1 当前进展

截至 2026-04-16，这份计划里已有两块基础设施落地：

1. `MLsub` 入口 IR dump 已统一更名为 `02-mlsub-input.ll`
2. `MLsubRecovery::run()` 已同步导出
   `02-mlsub-input.anchor.json`
3. `NOTDEC_EXTRA_CONSTRAINTS` 已开始消费这份锚点信息，并会在 `MLsub`
   开始前严格校验：
   - `ir_anchor.stage`
   - `ir_anchor.content_hash.algorithm`
   - `ir_anchor.content_hash.value`
   - `ir_anchor.data_layout`
   - `ir_anchor.target_triple`

同时结合后续实现，当前还需要补的一点是：

1. 具体函数内 value selector 仍未完整落地
2. 当前 `extra constraints` 的 `target.kind` 已支持 `arg` / `ret` /
   `named_value` / `inst` / `operand` / `binding`
3. 如果近期还要继续往前推，一条更轻量的路线是继续补 selector discoverability，比如导出 `SelectableValues.txt`
4. 即便走这条轻量路线，`ir_anchor` 仍然是必要的

和原计划不同的是，当前实现为了先把接口打通，摘要字段暂时不是
`sha256`，而是：

```json
{
  "content_hash": {
    "algorithm": "md5",
    "value": "..."
  }
}
```

也就是说，当前已经把“锚点校验链路”打通，但 hash 算法还保留升级空间。

## 2. 当前代码证据

### 2.1 当前 workdir 里已经有两份关键 IR

主 pass pipeline 启动前会写：

- [src/Passes/PassManager.cpp:404](/sn640/NotDec/src/Passes/PassManager.cpp#L404)
  到
  [src/Passes/PassManager.cpp:408](/sn640/NotDec/src/Passes/PassManager.cpp#L408)
  - `00-lifted.ll`

它表示：

1. 输入刚被载入/提升后的 LLVM IR
2. 还没有进入中端 decompilation / recovery pass

而 `MLsub` 启动前会写：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:681](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L681)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:683](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L683)
  - `02-mlsub-input.ll`

它表示：

1. 已经过了 `build_passes(tr-level>=2)` 前段的预处理
2. 是 `MLsub` 真正开始分析时的模块状态

### 2.2 当前 `MLsub` 之前仍然会改写 IR

在：

- [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
  到
  [src/Passes/PassManager.cpp:365](/sn640/NotDec/src/Passes/PassManager.cpp#L365)

的 `PassEnv::build_passes()` 里，`tr-level>=2` 的 `MLsub` 之前会执行：

1. `buildFunctionOptimizations()`
2. `LinearAllocationRecovery`
3. `MemsetMatcher`
4. `MemcpyMatcher`
5. `UndoInstCombine`
6. `BDCE`
7. `ReorderBlocksPass`

所以如果 JSON selector 绑定的是“原始输入 IR”，那它在进入 `MLsub` 前就可能失效。

这也是为什么我更倾向于把 selector 的锚点定义成：

- `02-mlsub-input.ll`

而不是原始输入文件。

### 2.3 当前输入格式不只 LLVM IR

在：

- [src/NotDec.cpp:169](/sn640/NotDec/src/NotDec.cpp#L169)
  到
  [src/NotDec.cpp:215](/sn640/NotDec/src/NotDec.cpp#L215)

当前前端支持：

1. `.ll`
2. `.bc`
3. `.wasm`
4. `.wat`

因此如果不额外限制，JSON 约束写作者要面对两个不稳定源：

1. 前端 lift 的变化
2. 中端 pre-MLsub pass 的变化

## 3. 我建议绑定哪一份 IR

### 3.1 最合理的锚点：`02-mlsub-input.ll`

如果目标是“给当前 `MLsub` 约束系统提供稳定 selector”，最佳锚点不是：

1. 原始 `.wasm/.wat`
2. 原始 `.ll/.bc`
3. `00-lifted.ll`

而是：

- `02-mlsub-input.ll`

原因：

1. selector 最终解析到的就是 `MLsub` 当前图中的 value
2. 这份 IR 已经过了当前 `MLsub` 必需的预处理
3. 用户只要对这一份 IR 写 selector，就不必关心 earlier pass 细节

### 3.2 推荐的 provenance 字段

我建议第一版 JSON 就预留：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "sha256": "7b2d...c1",
    "data_layout": "e-m:e-p:32:32-...",
    "target_triple": "wasm32-unknown-unknown"
  },
  "functions": {}
}
```

其中：

1. `stage`
   - 第一版固定只能是 `mlsub-input`
2. `sha256`
   - 对 workdir 中 `02-mlsub-input.ll` 的文本内容求 hash
3. `data_layout`
   - 用于更快发现 pointer size / ABI 已变
4. `target_triple`
   - 辅助定位 lift 差异

当前实现状态：

1. provenance/anchor 字段已落地，但命名略有调整
2. 当前不是单独的 `sha256` 字段，而是：

```json
{
  "ir_anchor": {
    "stage": "mlsub-input",
    "content_hash": {
      "algorithm": "md5",
      "value": "..."
    },
    "data_layout": "...",
    "target_triple": "..."
  }
}
```

3. 这样做是为了先把锚点校验和算法标识一起打通，后续再把
   `algorithm/value` 升级为 `sha256` 或兼容双格式

### 3.3 `sha256` 应该 hash 什么

不建议 hash：

1. 原始输入文件字节流
2. 原始 `.bc`
3. 原始 `.wasm`

更建议 hash：

1. `printModule()` 输出后的 `02-mlsub-input.ll`

也就是：

1. 先经过当前 pre-MLsub pipeline
2. 再以稳定文本形式导出
3. 对导出的文本求 `sha256`

这样 hash 对 selector 才有真正约束意义。

这里补一个我现在更明确的判断：

1. 即使近期 selector 不直接使用 `toStableString()`
2. 而是先用“函数内有名字的 value”
3. 它依然是在某一份冻结 IR 上解释的
4. 因此 anchor 方案并不会因为 selector 先走轻量版而失去意义

当前实现已经完成“对 `MLsub` 当前输入模块文本求摘要”这一步，只是算法暂时为
`md5`：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:153](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L153)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:170](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L153)
   新增：
   - `computeMD5Hex()`
   - `renderModuleToString()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:885](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L885)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:893](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L885)
   的 `MLsubRecovery::run()` 中：
   - 先把当前模块打印成文本
   - 再对文本求 `md5`

## 4. 应该怎样重新切分阶段

我现在更倾向于把整个流程明确拆成两步，而不是继续把“冻结 IR”和“类型恢复”混在一个命令里。

### 4.1 阶段 A：生成 `MLsub` 冻结 IR

这一阶段的目标不是做类型恢复，而是把输入统一整理成一份适合 `MLsub` 消费、且可长期固定的 LLVM IR。

这一阶段应当包含当前 `MLsub` 之前的结构整理 pass，也就是：

1. [src/Passes/PassManager.cpp:337](/sn640/NotDec/src/Passes/PassManager.cpp#L337)
   - `buildFunctionOptimizations()`
2. [src/Passes/PassManager.cpp:348](/sn640/NotDec/src/Passes/PassManager.cpp#L348)
   - `LinearAllocationRecovery`
3. [src/Passes/PassManager.cpp:353](/sn640/NotDec/src/Passes/PassManager.cpp#L353)
   - `MemsetMatcher`
4. [src/Passes/PassManager.cpp:354](/sn640/NotDec/src/Passes/PassManager.cpp#L354)
   - `MemcpyMatcher`
5. [src/Passes/PassManager.cpp:357](/sn640/NotDec/src/Passes/PassManager.cpp#L357)
   - `UndoInstCombine`
6. [src/Passes/PassManager.cpp:358](/sn640/NotDec/src/Passes/PassManager.cpp#L358)
   - `BDCE`
7. [src/Passes/PassManager.cpp:363](/sn640/NotDec/src/Passes/PassManager.cpp#L363)
   - `ReorderBlocksPass`

也就是说，这些 pass 不应该再被视为“约束模式下最好关掉的噪音”，而应该被重新归类为：

- 冻结 `MLsub` 输入 IR 的标准预处理步骤

从这个角度看，它们和 JSON 约束注入不是打架关系，而是前后两个阶段的关系。

### 4.2 阶段 B：只在冻结 IR 上做类型恢复与约束注入

第二步才是：

1. 读入一份已经冻结好的 `.ll/.bc`
2. 校验它和 JSON 里的 `ir_anchor` 是否一致
3. 执行 `MLsubRecoveryMain`
4. 应用 JSON 约束注入
5. 继续 top-down / dump-htypes / llvm2c 等后续流程

在这一阶段里，原则上不应再运行任何会改写 selector 锚点结构的 pre-MLsub pass。

如果近期 selector 先采用更轻量的“函数内有名字的 instruction result”方案，
这一点反而更重要，因为：

1. 名字同样会受 IR 改写影响
2. 甚至比 stable-id 更依赖“当前看到的正是当时写 JSON 的那份 IR”
3. 所以锚点校验和冻结 IR 仍然应当先于 selector 解析

## 5. “只允许 LLVM IR 输入”应该怎么理解

### 5.1 对阶段 B，我认为应该强制

如果流程已经明确分成两步，那么“只允许 LLVM IR 输入”就不应该只是一个可选安全门槛，而应该成为阶段 B 的接口前提：

1. 启用 JSON 约束注入时
2. 输入必须是 `.ll` / `.bc`
3. 而且语义上应当是“已经冻结好的 `MLsub` 输入 IR”

这时 JSON selector 写作者不再面对：

1. 前端 lift 漂移
2. 中端预处理 pass 漂移

而只需要面对一份固定下来的 IR。

### 5.2 对阶段 A，则不需要限制输入来源

阶段 A 反而应该继续接受：

1. `.wasm`
2. `.wat`
3. `.ll`
4. `.bc`

因为它的职责本来就是：

1. 把各种来源统一转成冻结后的 LLVM IR
2. 让用户有机会先检查并修补 IR
3. 再把这份 IR 当成后续类型恢复和约束注入的权威输入

## 6. 当前 `tr-level=1` 不足以代替阶段 A

用户提到“当前的 `opt level==1` 好像就是这样”，这里我觉得要在文档里明确纠正一下。

在：

- [src/NotDec.cpp:80](/sn640/NotDec/src/NotDec.cpp#L80)
  到
  [src/NotDec.cpp:83](/sn640/NotDec/src/NotDec.cpp#L83)

当前 `tr-level` 的语义是：

1. `0`
   - disable
2. `1`
   - simple opt
3. `2`
   - type recovery without breaking stack
4. `3`
   - full type recovery

而在：

- [src/Passes/PassManager.cpp:334](/sn640/NotDec/src/Passes/PassManager.cpp#L334)
  到
  [src/Passes/PassManager.cpp:365](/sn640/NotDec/src/Passes/PassManager.cpp#L365)

可以看到 `tr-level=1` 只会执行：

1. `buildFunctionOptimizations()`

它不会执行：

1. `LinearAllocationRecovery`
2. `MemsetMatcher`
3. `MemcpyMatcher`
4. `UndoInstCombine`
5. `BDCE`
6. `ReorderBlocksPass`

因此：

1. `tr-level=1` 不是“冻结 `MLsub` 输入 IR”
2. 它只能算“早期简化优化”
3. 不能直接拿来替代阶段 A

如果要把阶段 A 做成正式接口，最好新增显式命令行，而不是继续借用 `tr-level=1`。

## 7. 推荐的 CLI 方向

### 7.1 新增“只生成冻结 IR，不做类型恢复”的模式

我建议新增一个显式 CLI，而不是让用户用 `tr-level` 猜：

1. `--emit-mlsub-input-ir=<path>`
   - 运行前端和全部 pre-MLsub 标准化 pass
   - 写出冻结后的 LLVM IR
   - 不进入 `MLsubRecoveryMain`
2. 或者 `--stop-before-mlsub`
   - 和正常输出 `.ll/.bc` 组合使用
   - 语义上同样是“停在 `MLsub` 前”

这两种里，我更偏向前者，因为它表达得更直接。

### 7.2 冻结 IR 的推荐产物

阶段 A 的推荐输出应当就是：

- `02-mlsub-input.ll`

如果用户指定了显式输出路径，则可以把这份内容写到：

- `<user-output>.ll`

同时在 workdir 中仍保留同名快照，方便比对。

### 7.3 阶段 B 的推荐 CLI

阶段 B 则是：

1. 输入必须是 `.ll/.bc`
2. 输入内容应当就是阶段 A 导出的冻结 IR
3. 启用：
   - `NOTDEC_EXTRA_CONSTRAINTS`
   - `NOTDEC_SUMMARY_OVERRIDE`
   - `NOTDEC_SIGNATURE_OVERRIDE`
4. 不再重复执行阶段 A 的结构改写 pass

换句话说，阶段 B 更像：

- “对冻结好的 `MLsub` 输入 IR 做类型恢复”

而不是：

- “从任意输入重新生成一遍 `MLsub` 输入 IR，再顺手做类型恢复”

## 8. provenance 字段怎样更合理

在两阶段架构下，JSON 中的 provenance 字段就更明确了：

```json
{
  "version": 1,
  "ir_anchor": {
    "stage": "mlsub-input",
    "sha256": "7b2d...c1",
    "data_layout": "e-m:e-p:32:32-...",
    "target_triple": "wasm32-unknown-unknown"
  },
  "functions": {}
}
```

这里的含义变成：

1. 这份 JSON 不是绑定原始输入
2. 而是绑定阶段 A 产出的冻结 IR

因此 `sha256` 的计算对象仍然应该是：

1. `printModule()` 导出的 `02-mlsub-input.ll`

这一点比之前更确定，不再需要把它视为“暂时的折中锚点”。

## 9. 对现有文档方案应怎样改口

原文里关于“关闭各种优化 pass”的犹豫，我觉得可以直接去掉，替换成更明确的说法：

1. `MemsetMatcher`、`MemcpyMatcher`、`LinearAllocationRecovery` 等 pass
   不属于阶段 B
2. 它们应被视为阶段 A 的 IR 规范化 / 冻结流程
3. 阶段 B 的职责不是再跑这些 pass，而是消费阶段 A 的产物

这样文档里的冲突点就从：

- “这些 pass 会不会和 strict mode 打架”

改成：

- “这些 pass 属于前一步，不属于约束注入那一步”

我觉得这个表达更符合你现在想收敛的架构。

## 10. 推荐落地顺序

### 10.1 第一步：先把锚点文件名改正

本次已完成：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:681](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L681) 到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:683](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L683)
   把 workdir 输出从 `01-Optimized.ll` 改成 `02-mlsub-input.ll`
2. 在 [DEBUG.md:64](/sn640/NotDec/DEBUG.md#L64) 到 [DEBUG.md:69](/sn640/NotDec/DEBUG.md#L64)
   更新文件说明
3. 在 [DEBUG.md:128](/sn640/NotDec/DEBUG.md#L128)
   更新推荐阅读顺序

这一步后续又继续补完成了：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:182](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L182)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:197](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L182)
   新增 `writeMLsubInputAnchor()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:889](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L889)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:893](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L889)
   的 `MLsubRecovery::run()` 中，除了 `02-mlsub-input.ll` 外还会同步导出：
   - `02-mlsub-input.anchor.json`
3. 在 [DEBUG.md:71](/sn640/NotDec/DEBUG.md#L71)
   到 [DEBUG.md:77](/sn640/NotDec/DEBUG.md#L71)
   补充了该锚点文件的用途说明

### 10.2 第二步：给 JSON 增加 provenance

建议让 `NOTDEC_EXTRA_CONSTRAINTS` 第一版就支持：

1. `ir_anchor.stage`
2. `ir_anchor.sha256`
3. `ir_anchor.data_layout`
4. `ir_anchor.target_triple`

行为：

1. 阶段 B 中应当强制校验
2. 不匹配则直接报错

这一步已经部分完成：

1. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:425](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L425)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:476](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L425)
   实现 `validateExtraConstraintsAnchor()`
2. 在 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1066](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1066)
   到 [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1094](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1066)
   的 `MLsubRecovery::validateExtraConstraintsFile()` 中接入严格校验
3. 当前已严格校验：
   - `ir_anchor.stage`
   - `ir_anchor.content_hash.algorithm`
   - `ir_anchor.content_hash.value`
   - `ir_anchor.data_layout`
   - `ir_anchor.target_triple`
4. 当前未完成的只剩“把摘要算法从 `md5` 升级到计划里的 `sha256`”

### 10.3 第三步：新增阶段 A 专用 CLI

建议新增显式接口，例如：

1. `--emit-mlsub-input-ir=<path>`
2. 或 `--stop-before-mlsub`

并在实现上明确：

1. 跑完 pre-MLsub 标准化 pass
2. 写出冻结 IR
3. 不进入 `MLsubRecoveryMain`

### 10.4 第四步：让阶段 B 只接受冻结 IR

此时应当正式收紧为：

1. 启用 JSON 约束注入时只接受 `.ll/.bc`
2. 并要求该输入语义上就是阶段 A 的冻结产物
3. 阶段 B 不再重新跑阶段 A 的结构改写 pass

## 11. 我当前的建议

如果只看近期实现复杂度和收益，我建议按下面顺序走：

1. 把 `MLsub` 真正消费的 IR 明确命名为 `02-mlsub-input.ll`
2. 把 `MemsetMatcher`、`MemcpyMatcher`、`LinearAllocationRecovery` 等 pass
   明确定义为阶段 A 的标准化流程
3. 新增“只导出冻结 IR，不做类型恢复”的显式 CLI
4. JSON selector 与 `ir_anchor.sha256` 统一绑定 `02-mlsub-input.ll`
5. 启用 JSON 约束注入时，阶段 B 只接受冻结后的 `.ll/.bc`
6. 不再把 `tr-level=1` 视为阶段 A 的替代品

如果把“近期能更快落地”也考虑进去，我会把第 4 步再细分成两层：

1. 近期先支持函数内有名字的 value selector
   - 例如只匹配当前函数内第一个 `I.getName() == name` 的非 `void`
     instruction result
2. 长期再把 selector 统一收敛到 stable-id / operand / binding
   - 这一层当前先不实现，只作为后续正式方向保留
3. 但这两层都继续绑定同一份 `02-mlsub-input.ll` 的 `ir_anchor`

这样既能把长期方向定住，也能把阶段职责切得更清楚。

## 12. 本次修改

### 12.1 源码

文件：

- [src/TypeRecovery/mlsub/MLsubGenerator.cpp:681](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L681)
  到
  [src/TypeRecovery/mlsub/MLsubGenerator.cpp:683](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L683)

涉及函数：

- `MLsubRecovery::run()`

修改内容：

1. workdir 中 `MLsub` 入口 IR dump 文件名从 `01-Optimized.ll` 改为 `02-mlsub-input.ll`

### 12.2 文档

文件：

- [DEBUG.md:64](/sn640/NotDec/DEBUG.md#L64)
  到
  [DEBUG.md:69](/sn640/NotDec/DEBUG.md#L69)
- [DEBUG.md:128](/sn640/NotDec/DEBUG.md#L128)

修改内容：

1. 将 `01-Optimized.ll` 的说明更新为 `02-mlsub-input.ll`
2. 明确这份 IR 更接近未来 JSON 约束的实际锚点

### 12.3 方案文档

新增：

- [logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md](/sn640/NotDec/logs/20260416-07-MLsubFrozenIRConstraintAnchorPlan.md)

### 12.4 后续已落地实现

后续基于这份计划又继续实现了：

1. [logs/20260416-08-MLsubInputAnchorImplementation.md](/sn640/NotDec/logs/20260416-08-MLsubInputAnchorImplementation.md)
   - 导出 `02-mlsub-input.anchor.json`
2. [logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md](/sn640/NotDec/logs/20260416-09-MLsubExtraConstraintsAnchorValidation.md)
   - `NOTDEC_EXTRA_CONSTRAINTS` 校验 `ir_anchor`

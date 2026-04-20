# 2026-04-20 ValueTypes / VarOrigins readability

## 目标

基于 [logs/20260420-01-BinarysubUTypeVariableOriginTracking.md](/sn640/NotDec/logs/20260420-01-BinarysubUTypeVariableOriginTracking.md)
里新增的 `UTypeVariable::{id, originIds}`，让 workdir 里的类型 dump 更容易回答两类问题：

1. `ValueTypes.txt` 里的某个 `'a` / `'b` 到底对应哪个最终 `ut#id`
2. 这个 `ut#id` 最早是从哪些 `ExtValuePtr` 创建出来的

## 修改

### 1. 在约束生成阶段保存 `variable state id -> original ExtValuePtr`

- 文件：
  - [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:63](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L63)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1883](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1883)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2560](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2560)
- 涉及：
  - `notdec::mlsub::ConstraintsGenerator::OriginalVariableSources`
  - `notdec::mlsub::ConstraintsGenerator::createNode`
  - `notdec::mlsub::ConstraintsGenerator::releaseBinarysubState`

处理方式：

- 在 `ConstraintsGenerator` 上新增 `OriginalVariableSources`，键为原始
  `VariableState::id`，值为创建该变量时对应的 `ExtValuePtr` 集合
- `createNode()` 在 `convertSimpleType()` 返回 `VariableState` 时立刻记录来源
- `releaseBinarysubState()` 结束后清空该映射，避免跨 SCC 残留

### 2. 收紧 `ValueTypes.txt` 行内摘要，只保留可扫读的 `ut# / vs#` 信息

- 文件：
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1312](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1312)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1463](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1463)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1551](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1551)
- 涉及：
  - `appendDebugValueTypes`
  - `formatExtValueMappingLabel`
  - `formatInlineUTypeVariableSummary`
  - `collectUTypeVariableDetailsImpl`
  - `collectUTypeVariableDetails`
  - `formatOriginIdSummary`

当前输出策略：

- 左侧 value label 改成 `stable id + verbose ExtValuePtr`
- 右侧仍保留原始 `binarysub::printType(UType)`
- 若该 `UType` 内含变量，行尾追加紧凑摘要：
  - `name=ut#finalId<-vs#originIds`

这样 `ValueTypes.txt` 仍能在一屏里扫读，不会把完整 origin value 列表塞回每一行。

### 3. 新增 `VarOrigins.txt`，把详细 origin 展开到单独文件

- 文件：
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:54](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L54)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1372](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1372)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1954](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1954)
  - [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2552](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2552)
- 涉及：
  - `kVarOriginsFile`
  - `appendDebugVarOrigins`
  - `appendVarOriginEntries`
  - `MLsubRecovery::run`
  - `ConstraintsGenerator::genTypes`

`VarOrigins.txt` 当前按 SCC 输出：

- `ut#id name ; size=... ; origins=vs#...`
- `roots: ...`
- 每个 `vs#originId => ExtValuePtr`

如果某个 `originId` 来自 summary/override/其他非 value 创建路径，当前打印
`<unknown>`，避免误装成真实 `ExtValuePtr`。

### 4. 更新 workdir 文档

- 文件：
  - [DEBUG.md:119](/sn640/NotDec/DEBUG.md#L119)
- 涉及：
  - `ValueTypes.txt` 段落
  - `VarOrigins.txt` 段落

文档现在明确说明：

- `ValueTypes.txt` 里新的行内摘要格式
- `VarOrigins.txt` 的用途和与 `ValueTypes.txt` 的互补关系

## 验证

### 1. 小样例冒烟

执行：

```bash
./build/bin/notdec \
  test/type-recovery/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/notdec-varorigins-smoke/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-varorigins-smoke
```

结果：

- 生成 `/tmp/notdec-varorigins-smoke/ValueTypes.txt`
- 生成 `/tmp/notdec-varorigins-smoke/VarOrigins.txt`
- `ValueTypes.txt` 行尾已出现 `ut# / vs#` 摘要
- `VarOrigins.txt` 可从 `ut#2 'c` 追到 `main::arg0`

### 2. 当前关注 case

执行：

```bash
./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-varorigins-fortune2/out.ll \
  --tr-level=2 -g --work-dir=/tmp/notdec-varorigins-fortune2
```

结果：

- 生成 `/tmp/notdec-varorigins-fortune2/ValueTypes.txt`
- 生成 `/tmp/notdec-varorigins-fortune2/VarOrigins.txt`
- `ValueTypes.txt` 在大 case 上保持为紧凑摘要，而不是把完整 origin value 列表塞进每一行
- `VarOrigins.txt` 能把 `ut#id` 展开到 `roots` 和每个 `vs#originId` 的来源值

## 目前边界

当前 `VarOrigins.txt` 里仍可能看到：

- `vs#... => <unknown>`

这通常不是丢数据，而是该 origin 来自：

1. summary / signature override 里内部新建的变量
2. 其他没有直接对应 `ExtValuePtr` 的非 value 创建路径

如果后续还想继续提升解释性，我建议下一步不要再把更多长文本塞进
`ValueTypes.txt`，而是把 `VarOrigins.txt` 再细分出 source kind，例如：

- `extvalue`
- `summary-override`
- `signature-override`
- `synthetic/extruded`

这样比继续拉长 `ValueTypes.txt` 更利于排查。

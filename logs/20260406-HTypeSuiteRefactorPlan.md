# HType 类型恢复测试改造实施清单

## 目标

将当前基于 `.c` 反编译结果比较的 `tr-level=2` 集成测试，改造为基于 `Value -> HType` 结果比较的类型恢复测试。

总体方向：

1. 增加独立的命令行选项导出 HType 快照
2. 规范化 `HTypeResult` 的稳定输出格式
3. 将现有 `notdec.decompile.llvm_ir.tr_level_2` 直接改造成类型恢复 suite
4. 删除旧的 `.c` golden 比较逻辑，改为 `.htypes` golden 比较

---

## 实现顺序

## 第 1 步：新增稳定的 HType 导出入口

### 目标

让 `notdec` 可以在正常跑 pass 的同时，把最终 `HTypeResult` 稳定写到文件中，作为测试产物。

### 建议改动

#### 1.1 在 `src/NotDec.cpp` 增加命令行参数

新增类似：

```cpp
--dump-htypes=/path/to/file
```

建议形式：

- 类型：`cl::opt<std::string>`
- 默认空字符串
- 仅在 `tr-level >= 2` 且类型恢复可用时生效

#### 1.2 把导出路径传入 pass / driver 层

当前 `DecompileConfig` 只持有主输出文件路径 `OutFilePath`。
建议增加一个字段，例如：

- `std::string HTypeDumpPath;`

涉及文件：

- `include/notdec/Passes/PassManager.h`
- `src/NotDec.cpp`

#### 1.3 在 pass 跑完后拿到 `HTypeResult`

当前 mlsub 类型恢复结果保存在：

- `mlsub::MLsubRecovery::getResult(...)`

建议在 `PassEnv` / `DecompileConfig` 层增加一个导出辅助函数，负责：

1. 判断 `HTypeDumpPath` 是否非空
2. 判断 `TR` 是否已初始化
3. 调用 `TR->getResult(Mod, MAM)` 拿到结果
4. 将结果打印到指定文件

### 设计要求

- 不依赖 `.c` 输出
- 可以和 `-o out.ll` 一起使用
- 不要复用 `NOTDEC_TYPE_RECOVERY_DEBUG_DIR` 的 `ValueTypes.txt`
- 导出失败时应明确报错

### 第 1 步完成情况（2026-04-07）

已完成的改动：

- 在 `src/NotDec.cpp` 增加了 `--dump-htypes=/path/to/file`
- 在 `DecompileConfig` 中新增了 `HTypeDumpPath`
- 在 `PassEnv` 中新增了 `dump_htypes(const std::string &)` 导出辅助函数
- `run_passes()` 结束后如果指定了 `--dump-htypes`，会主动取回 mlsub 的 `HTypeResult` 并写入文件
- 导出逻辑与主输出解耦，可和 `-o out.ll` 一起使用
- 已完成本地构建，并用 `01_Simple1.ll` 验证 `.htypes` 文件可实际生成

当前仍是过渡态：

- 输出格式暂时沿用接近 `HTypeResult.dump()` 的文本风格
- 还没有做稳定排序、stable key、canonical HType 文本
- 还不能直接作为长期 golden，需要继续完成第 2～4 步

补充处理（2026-04-07）：

- 修复了 `--dump-htypes` 路径上一处稳定复现的 LSAN 泄漏
- 复现命令：

```bash
./build/bin/notdec test/decompile/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01_Simple1.pass1.ll \
  --tr-level=2 \
  --dump-htypes /tmp/01_Simple1.pass1.htypes
```

- 根因是 `binarysub::TypeNode` 约束图中 `VariableState.lowerBounds/upperBounds`
  等强引用边会形成环；`genTypes()` 完成后虽然只保留 `HType` 结果，但旧的
  binarysub 图仍留在 `ConstraintsGenerator` 里，导致进程退出时被 LSAN 识别
- 当前修复策略是在所有 SCC 完成 `genTypes()` 之后，统一递归释放
  binarysub 类型图中的强引用边，再丢弃 `V2N` / `unhandledCalls` 等中间状态
- 这一步只针对类型恢复中间图的生命周期管理，不改变 HType 推理结果
- 已完成本地验证：上述命令现在可稳定退出且不再报 LSAN

建议本批提交 message：

```text
Add initial --dump-htypes export path for recovered HTypes
```

---

## 第 2 步：把 `HTypeResult` 变成稳定 snapshot 输出接口

### 目标

让 `HTypeResult` 的输出可直接作为测试 golden。

### 建议改动

#### 2.1 在 `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h` 扩展接口

当前只有：

- `void dump() const`

建议改成：

- `void print(llvm::raw_ostream &OS) const;`
- `void dump() const { print(llvm::errs()); }`

这样 CLI 导出和调试复用同一份稳定格式。

#### 2.2 在 `external/NotDec-llvm2c/lib/notdec-llvm2c/...` 实现稳定输出

建议输出分段固定为：

```text
# HTypeResult

[decls]
...

[upper]
...

[lower]
...

[memory]
...
```

其中：

- `[decls]`：打印 `HTCtx` 中声明的类型定义
- `[upper]`：打印 `ValueTypes`
- `[lower]`：打印 `ValueTypesLowerBound`
- `[memory]`：打印 memory type

#### 2.3 明确排序规则

输出必须稳定排序，避免 golden 波动。

建议：

- values：按 stable key 的字典序排序
- decls：按 decl 名称或稳定打印文本排序

#### 2.4 空段也显式保留

即使某段为空，也建议输出段头，例如：

```text
[lower]
```

这样格式固定，便于 diff。

### 第 2 步完成情况（2026-04-07）

已完成的改动：

- 在 `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h` 中为 `HTypeResult` 增加 `print(llvm::raw_ostream &OS) const`
- 保留 `dump()`，但改为直接复用 `print(llvm::errs())`
- 将输出格式固定为：
  - `# HTypeResult`
  - `[decls]`
  - `[upper]`
  - `[lower]`
  - `[memory]`
- `decls` 与 value/type 条目现在都会先转成文本，再按字典序稳定排序
- 空段也会保留段头，避免快照格式随内容缺失而漂移
- `PassEnv::dump_htypes()` 已改为复用 `HTypeResult::print()`，CLI 导出与调试导出走同一份格式

当前仍是过渡态：

- 左侧 key 仍暂时依赖现有 `ExtValuePtr::toString(..., true)`，还不是长期稳定 key
- 这部分会在第 3 步单独替换为 `toStableString(...)`
- 右侧 `HType` 文本仍沿用 `getAsString()`，是否需要 canonical print 留给第 4 步评估

建议本批提交 message：

```text
Stabilize HTypeResult snapshot formatting
```

---

## 第 3 步：为 `ExtValuePtr` 增加 stable 文本格式

### 目标

让测试左侧 key 不依赖当前偏 debug 的 `toString()` 文本。

### 原因

现有：

- `toString(ExtValuePtr, true)`

更偏人类调试阅读，不适合长期作为 golden key。

### 建议改动

#### 3.1 在 `ExtValuePtr` 接口中新增 stable formatter

例如新增：

- `std::string toStableString(const ExtValuePtr &Val);`

涉及文件：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp`

#### 3.2 推荐格式

建议统一成简洁、稳定、适合 diff 的形式，例如：

- 参数：`main::arg0`
- 返回值：`main::<ret>`
- 指令值：`main::%foo`
- 未命名值：使用 `ValueNamer` 保证稳定名
- 常量操作数：`main::const(i32 0)@ret:0`
- constant addr：`main::addr(0x10)` 或 `addr(0x10)`
- memory：`<memory>`

#### 3.3 保留现有 `toString()`

不要直接拿旧 `toString()` 改成 stable 格式。

建议区分：

- `toString(...)`：继续用于调试
- `toStableString(...)`：专供测试/快照

这样以后调整 debug 文本不会把 golden 全部打爆。

### 第 3 步完成情况（2026-04-07）

已完成的改动：

- 在 `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h`
  中新增 `toStableString(const ExtValuePtr &)`
- 在 `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp`
  中为 `llvm::Value *`、`ReturnValue`、`UConstant`、`ConstantAddr`、
  `StackObject`、`HeapObject` 实现了独立的 stable formatter
- `HTypeResult::print()` 的左侧 key 已切换为 `toStableString(...)`，
  不再复用偏调试用途的 `toString(..., true)`
- 本次 stable key 的主要规则为：
  - 函数参数：`main::arg0`
  - 返回值：`main::<ret>`
  - 命名指令值：`main::%foo`
  - 未命名指令值：`main::%bb0.i3`
  - 常量 use-site：`const(i32 0)@main::%bb0.i3:1`
  - 常量地址：`addr(0x400)`
  - 全局/函数值：`@g`、`@main`
- 这一步刻意没有修改 `toString()`，调试文本与测试快照格式已解耦

当前状态说明：

- unnamed value 的 stable key 现在优先依据函数内 block/instruction 位置生成，
  避免因 map 指针顺序不同而通过 `ValueNamer` 产生漂移
- 右侧 `HType` 文本仍沿用 `getAsString()`，canonical type print 留给第 4 步

本地验证：

- 已重新构建 `build/bin/notdec`
- 已用以下命令验证快照左侧 key 生效：

```bash
./build/bin/notdec test/decompile/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01_Simple1.step3.ll \
  --tr-level=2 \
  --dump-htypes /tmp/01_Simple1.step3.htypes

./build/bin/notdec test/decompile/llvm-ir/cases/02_ConstantAddr1.ll \
  -o /tmp/02_ConstantAddr1.step3.ll \
  --tr-level=2 \
  --dump-htypes /tmp/02_ConstantAddr1.step3.htypes
```

- 观察到输出 key 已变为 `main::arg0`、`main::<ret>`、`addr(0x400)`、
  `const(i32 0)@...` 等 stable 形式

建议本批提交 message：

```text
Add stable ExtValuePtr snapshot keys for HType dumps
```

---

## 第 4 步：检查 HType 文本表示是否足够稳定

### 目标

确保右侧 `HType` 字符串也适合作为 golden。

### 检查点

#### 4.1 评估 `HType::getAsString()`

重点确认：

1. 结构体/递归类型命名是否稳定
2. union / intersection 打印是否稳定
3. pointer / dual-pointer / function type 打印是否稳定

#### 4.2 如果 decl 文本能覆盖结构体细节，则 value 侧尽量保持简洁

例如 value 行：

```text
main::%p => struct_0*
```

而详细字段放到 `[decls]` 中。

这样 golden 更好维护。

#### 4.3 必要时增加 canonical print

如果 `getAsString()` 不够稳定，建议单独加：

- `getAsCanonicalString()`

但如果当前 `getAsString()` 已经足够稳定，则尽量少改。

### 第 4 步完成情况（2026-04-07）

结论：

- `HType::getAsString()` 不足以直接作为长期 golden
- 主要不稳定源不是基础类型语法本身，而是它会直接引用类型恢复阶段通过
  `ValueNamer` 生成的声明名 / 字段名，例如 `struct_7`、`field_8`
- 这会让右侧 type 文本与 `[decls]` 一起受到中间构造顺序影响，不适合作为
  snapshot oracle

已完成的改动：

- 在 `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h`
  与 `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp`
  中新增 `HTypeSnapshotFormatter`
- snapshot formatter 会为声明分配独立 canonical 名：
  - `struct_0`
  - `union_0`
  - `typedef_0`
- 字段名也改为打印期现算的 canonical 名：
  - `field_0`
  - `padding_0`
- `HTypeResult::print()` 已改为复用同一个 formatter 来同时打印：
  - `[decls]`
  - `[upper]`
  - `[lower]`
  - `[memory]`
- value 侧 record/union/typedef type 现在不再泄漏内部声明文本，而是引用
  canonical 名，例如：
  - `main::%p => struct_0*`
- `SetUnionType` / `SetInterType` 的打印已按左右子表达式文本排序，
  避免 `(A | B)` 与 `(B | A)` 在 snapshot 中来回翻转
- 指针 / dual-pointer / function type 继续沿用现有语法，但其子类型会先经过
  canonical print

当前取舍：

- `getAsString()` 仍保留原行为，继续服务调试和其他非 snapshot 路径
- canonical 化只作用于 HType snapshot 导出路径，不主动扩大影响面
- `TypeVariableType` 当前仍沿用现有名字（如 `'c:32`），本步未额外重命名

本地验证：

- 已重新构建 `build/bin/notdec`
- `02_ConstantAddr1.ll` 的导出已从：
  - `struct struct_7*`
  变为：
  - `struct_0*`
- `[decls]` 中对应声明现在固定为：

```text
struct struct_0 {
  ptr<load=void, store=void, psize=32> field_0; /* at offset: 1024 */
};
```

- 对 `02_ConstantAddr1.ll`、`11_SimpleRecursive1.ll` 各重复跑两次，
  `.htypes` diff 结果为空，确认当前 canonical print 至少在本地重复执行下稳定

补充说明：

- `06_SimpleRecursive2.ll` 曾在本地验证时出现长时间未结束现象；后续定位发现
  实际卡点在 `HTypeSnapshotFormatter` 对递归声明做 snapshot collect 时无限递归，
  修复后已可稳定导出 `.htypes`

建议本批提交 message：

```text
Canonicalize HType snapshot printing
```

---

## 第 5 步：实现 CLI 导出文件写入

### 目标

把第 1～4 步串起来，真正输出 `.htypes` 文件。

### 建议改动

#### 5.1 在 `src/NotDec.cpp` 主流程末尾增加导出逻辑

当前流程是：

1. parse input
2. `conf.build_passes(trLevel)`
3. `conf.run_passes()`
4. 根据 `-o` 后缀输出 `.ll/.bc/.c`

建议新增：

5. 如果 `--dump-htypes` 非空，则导出 HType snapshot

#### 5.2 注意 `.c` 输出场景

如果主输出是 `.c`，当前 llvm2c pass 里面也会调用 `TR.getResult(...)`。

为避免重复计算，建议：

- 优先复用已有 `ResultVal`
- 若 `getResult()` 已缓存则无需额外担心

#### 5.3 处理无类型恢复场景

如果：

- `tr-level < 2`
- 或类型恢复未初始化

建议：

- 明确报错并退出非零
- 不要写空文件冒充成功

### 第 5 步完成情况（2026-04-07）

已完成的改动：

- `--dump-htypes` 现在会在主流程结束后稳定写出 `.htypes` 文件
- 对 `.ll/.bc` 输出路径，仍然直接复用 `PassEnv::dump_htypes()` 从
  `MLsubRecovery` 中取回 `HTypeResult` 并打印
- 对 `.c` 输出路径，`MLsubNotdecLLVM2C` 在把 `HTypeResult` 交给 llvm2c 之前，
  会先把 snapshot 文本缓存到 `PassEnv`，后续 `dump_htypes()` 直接写这个缓存，
  不再在 llvm2c 改写 IR 之后重新跑一遍 `getResult()`
- 因此 `.c + --dump-htypes` 现在满足：
  - 不重复 materialize HType 结果
  - 不依赖 llvm2c 之后被 demote SSA 改写过的模块重新推类型
  - 导出的 snapshot 与 llvm2c 实际消费的那份 HTypeResult 保持一致
- 若 `tr-level < 2` 或类型恢复未初始化，仍会明确报错并终止，不会生成空文件

本地验证：

```bash
cmake --build ./build --target notdec-decompile

./build/bin/notdec test/decompile/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01_Simple1.step5.ll \
  --tr-level=2 \
  --dump-htypes /tmp/01_Simple1.step5.ll.htypes

./build/bin/notdec test/decompile/llvm-ir/cases/01_Simple1.ll \
  -o /tmp/01_Simple1.step5.c \
  --tr-level=2 \
  --dump-htypes /tmp/01_Simple1.step5.c.htypes
```

- 两条命令都可成功生成非空 `.htypes`
- `.c` 路径下导出的 snapshot 与 `.ll` 路径一致，确认没有因 llvm2c 的
  demote SSA 改写而产生额外漂移

建议本批提交 message：

```text
Finalize CLI HType snapshot export for all output modes
```

---

## 第 6 步：把测试 runner 从 `.c` 比较改成 `.htypes` 比较

### 目标

直接把现有 suite runner 改造成类型恢复 runner。

### 建议改动

#### 6.1 修改 `test/tools/run_decompile_suite.py`

建议直接重命名成更通用或更贴切的名字，例如：

- `run_type_recovery_suite.py`
- 或 `run_notdec_suite.py`

但如果想少动路径，也可以先保留文件名，内部逻辑改掉。

#### 6.2 修改核心行为

当前逻辑：

- 运行 `notdec input.ll -o out.c --tr-level=2`
- 检查 `.c`

新逻辑改成：

- 运行 `notdec input.ll -o out.ll --tr-level=2 --dump-htypes out.htypes`
- 检查 `.htypes`

#### 6.3 修改成功条件

当前 `command_succeeded()` 依赖：

- 返回码为 0
- 输出文件存在且非空
- stdout 不包含 `IR parsing failed:`

改为：

- 返回码为 0
- `.htypes` 文件存在且非空
- stdout/stderr 无明显解析失败

#### 6.4 artifact 文件命名

建议：

- 主输出：`<case>.out.ll`
- HType 输出：`<case>.out.htypes`
- 日志：`<case>.log`

这样遇到失败时还能顺手看最终 IR。

### 第 6 步完成情况（2026-04-07）

已完成的改动：

- 将 `test/tools/run_decompile_suite.py` 重命名为
  `test/tools/run_type_recovery_suite.py`
- runner 现在统一执行：
  - `notdec input.ll -o <case>.out.ll --tr-level=2 --dump-htypes <case>.out.htypes`
- 成功条件已切换为：
  - 返回码为 0
  - `.out.ll` 存在且非空
  - `.out.htypes` 存在且非空
  - 日志中不包含 `IR parsing failed:`
- golden 比较对象已从 `.c` 输出切换为 `.htypes`
- 失败时会保留：
  - `<case>.out.ll`
  - `<case>.out.htypes`
  - `<case>.log`

当前取舍：

- runner 仍保留“manifest + pass/xfail/skip”语义，尽量减少测试框架层面的额外改造
- 只切换比较目标，不引入新的测试 DSL，便于继续沿用现有回归工作流

---

## 第 7 步：直接把现有 suite 改名并迁移用途

### 目标

把当前 suite 从“反编译结果回归”正式切换成“类型恢复回归”。

### 建议改动

#### 7.1 修改 `test/CMakeLists.txt`

将测试名：

- `notdec.decompile.llvm_ir.tr_level_2`

改为：

- `notdec.type_recovery.llvm_ir.tr_level_2`

同时修改 workdir 路径。

#### 7.2 修改 manifest 文件名

建议把：

- `test/decompile/llvm-ir/tr-level-2.json`

改成更准确的名字，例如：

- `test/decompile/llvm-ir/type-recovery-tr-level-2.json`

#### 7.3 修改 manifest 元信息

例如：

```json
{
  "suite": "notdec type recovery llvm ir tr-level=2",
  ...
}
```

#### 7.4 `expected` 目录改名

建议从：

- `expected/tr-level-2/*.c`

迁移到：

- `expected/type-recovery-tr-level-2/*.htypes`

### 第 7 步完成情况（2026-04-07）

已完成的改动：

- `test/CMakeLists.txt` 中的 CTest 名称已改为：
  - `notdec.type_recovery.llvm_ir.tr_level_2`
- manifest 已改名为：
  - `test/decompile/llvm-ir/type-recovery-tr-level-2.json`
- suite 元信息已同步改成 “notdec type recovery llvm ir tr-level=2”
- 新 golden 目录已切换为：
  - `test/decompile/llvm-ir/expected/type-recovery-tr-level-2/`

说明：

- 旧的 `.c` 期望文件不再被当前 suite 引用
- 当前权威 oracle 已正式切换为 `.htypes` snapshot

---

## 第 8 步：生成第一批 `.htypes` golden

### 目标

先让新 suite 跑起来，再逐步收紧预期。

### 建议改动

#### 8.1 先迁现有可运行 case

优先迁移当前 manifest 里原本就是 `pass` 的 case：

- `01_Simple1`
- `02_ConstantAddr1`
- `03_LoadUpdateGood`
- `04_LoadUpdate`
- `05_MultiOffset`
- `09_OffsetLoop`
- `12_Stack1`

#### 8.2 用新导出接口批量生成 snapshot

命令形式类似：

```bash
notdec case.ll -o /tmp/x.ll --tr-level=2 --dump-htypes /tmp/x.htypes
```

确认输出稳定后，拷贝为 golden。

#### 8.3 再评估原来的 xfail

原来很多 xfail 原因是：

- `llvm2c constant-user assertion`

迁移后这些 case 可能会变成：

- 可直接 `pass`
- 或暴露真正的类型恢复失败原因

所以不要原样保留旧的 xfail reason，要重新分类。

### 第 8 步完成情况（2026-04-07）

已导入第一批 `.htypes` golden：

- `01_Simple1`
- `02_ConstantAddr1`
- `03_LoadUpdateGood`
- `04_LoadUpdate`
- `05_MultiOffset`
- `07_PassStack1`
- `08_Memory1`
- `09_OffsetLoop`
- `10_BottomUp1`
- `11_SimpleRecursive1`
- `12_Stack1`
- `16_Poly1`

其中原本因为 `llvm2c` 路径失败而标成 `xfail`，但迁移到 HType snapshot 后
已转为 `pass` 的 case 包括：

- `07_PassStack1`
- `08_Memory1`
- `10_BottomUp1`
- `11_SimpleRecursive1`
- `16_Poly1`

保留 `xfail` 的 case 现已按类型恢复层面的真实失败重分类：

- `13_stack_variable_alloc`
- `15_signed1`
  - `MLsubGenerator` 尚不能处理字符串 `getelementptr` 常量
- `14_Equality1`
- `17_StackArray`
- `18_offset1`
  - `PNDiff` 在 `getPNIVar` 上触发断言
- `19_PtrPtr`
  - 输入 LLVM IR 本身无法通过 parser，因而不会生成 `.htypes`

---

## 第 9 步：调整 manifest 字段与 case 语义

### 目标

让 manifest 语义匹配“类型恢复 suite”。

### 建议改动

#### 9.1 `expected` 指向 `.htypes`

每个 case 的 `expected` 从 `.c` 改成 `.htypes`。

#### 9.2 保留 `status`

仍保留：

- `pass`
- `xfail`
- `skip`

#### 9.3 重写 `reason`

将旧的 backend 原因替换为类型恢复相关原因，例如：

- type recovery assertion
- unstable recursive htype snapshot
- missing lower bound for stack object

避免继续保留“llvm2c assertion”这类已经不相关的说明。

### 第 9 步完成情况（2026-04-07）

已完成的改动：

- manifest 中所有 `pass` case 的 `expected` 已改为指向 `.htypes`
- `status` 仍保留 `pass` / `xfail` / `skip`
- 旧的 `llvm2c constant-user assertion`、`type recovery TODO assertion`
  等原因已被替换为当前真实可复现的失败描述

结果：

- manifest 语义已与 “类型恢复 snapshot suite” 对齐
- 后续再处理失败 case 时，可以直接依据现有 `reason` 和日志定位到
  类型恢复链路中的具体断点

---

## 第 10 步：更新文档与说明

### 目标

让仓库内说明与新的测试目标一致。

### 建议改动

#### 10.1 更新 `AGENTS.md`

当前优先任务描述里，应把：

- `notdec.decompile.llvm_ir.tr_level_2`

改成：

- `notdec.type_recovery.llvm_ir.tr_level_2`

并明确：

- 当前 suite 检查的是 `Value -> HType`
- 不再检查 `.c` 输出

#### 10.2 更新相关测试说明

涉及：

- `test/` 目录说明
- `README` 中如果提到旧 suite，也应同步

#### 10.3 记录 snapshot 格式约定

建议在文档或日志中写清：

- `[decls] / [upper] / [lower] / [memory]` 的含义
- stable key 的生成规则
- golden 更新方式

### 第 10 步完成情况（2026-04-07）

已完成的文档同步：

- 更新 `AGENTS.md`
  - 当前 CTest suite 名称
  - manifest 路径
  - runner 路径
  - `expected/type-recovery-tr-level-2/` 目录约定
  - `ctest` 运行命令
- 更新 `test/decompile/llvm-ir/README.md`
  - 明确该目录当前承载的是 LLVM IR 类型恢复回归
  - 说明 golden 现为 `.htypes` snapshot

当前文档口径已经切换为：

- 当前 suite 检查的是 `Value -> HType`
- 不再以 `.c` 输出作为权威 oracle

---

## 第 11 步：回归检查与稳定性清理

### 目标

确保 snapshot 真正稳定，不会因为小改动频繁抖动。

### 检查清单

#### 11.1 连续运行同一 case 两次，输出必须完全一致

重点关注：

- decl 排序
- unnamed value 命名
- map 遍历顺序
- struct / recursive type 名称

#### 11.2 检查 ASAN 泄漏对测试结果的影响

当前 suite 已设置：

```json
"ASAN_OPTIONS": "detect_leaks=0"
```

新的类型恢复 suite 应继续保留这一设置，避免“产物正确但 leak 失败”干扰功能回归。

#### 11.3 确认 `.htypes` 文件没有掺杂非稳定日志

例如：

- 地址
- 临时编号
- 调试 trace
- 输出时机相关文本

### 第 11 步完成情况（2026-04-07）

已完成的稳定性检查：

- 对以下 12 个 `pass` case 各重复运行两次：
  - `01_Simple1`
  - `02_ConstantAddr1`
  - `03_LoadUpdateGood`
  - `04_LoadUpdate`
  - `05_MultiOffset`
  - `07_PassStack1`
  - `08_Memory1`
  - `09_OffsetLoop`
  - `10_BottomUp1`
  - `11_SimpleRecursive1`
  - `12_Stack1`
  - `16_Poly1`
- 两次导出的 `.htypes` 文本均完全一致
- 新 suite 继续保留：
  - `ASAN_OPTIONS=detect_leaks=0`
- 当前 `.htypes` 文件中未观察到地址、临时编号、trace 日志等非稳定噪声

最终回归结果：

- 已重新配置工程并运行：

```bash
ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure
```

- 当前结果为：
  - `12 passed`
  - `7 xfailed`
  - `0 skipped`
  - `0 xpassed`
  - `0 failed`

---

## 第 12 步：提交策略

### 建议拆分提交

#### Commit 1

- CLI: `--dump-htypes`
- `HTypeResult::print()`
- stable formatter

建议 message 类似：

```text
Add stable HType snapshot export for type recovery
```

#### Commit 2

- runner 改造
- suite 改名
- CMake 改名

建议 message 类似：

```text
Switch tr-level-2 LLVM IR suite to HType-based type recovery checks
```

#### Commit 3

- 导入/更新 `.htypes` golden
- 调整 xfail/pass 状态

建议 message 类似：

```text
Add HType golden snapshots for LLVM IR tr-level-2 cases
```

如果中间存在未完全打通的提交，按仓库约定：

- commit message 以 `WIP:` 开头
- 并在对应日志文档中说明边界

---

## 附：建议优先修改的文件

### 代码

- `src/NotDec.cpp`
- `include/notdec/Passes/PassManager.h`
- `src/Passes/PassManager.cpp`
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface.h`
- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/ExtValuePtr.h`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/ExtValuePtr.cpp`
- `external/NotDec-llvm2c/lib/notdec-llvm2c/...`（实现 `HTypeResult::print` 的位置）

### 测试

- `test/CMakeLists.txt`
- `test/tools/run_decompile_suite.py`（或其重命名后的新文件）
- `test/decompile/llvm-ir/tr-level-2.json`（或新名字）
- `test/decompile/llvm-ir/expected/...`

### 文档

- `AGENTS.md`
- `README.md`（若涉及）
- 本文件

---

## 2026-04-08 Follow-up

后续清理中，测试布局进一步统一为：

- `test/type-recovery/llvm-ir/`
- `test/type-recovery/sysy/`
- `test/run_type_recovery_suite.py`
- `test/legacy/wasm/`

其中 `sysy` suite 通过 manifest 驱动，先把 `cases/*.c` 编译为 LLVM IR，
再复用同一套 HType snapshot runner。旧的 `test/wasm/` 工作区迁入
`test/legacy/wasm/`，避免继续与当前权威回归布局混放。

### 2026-04-08 HOWARD O3 Corpus Import

新增：

- `test/type-recovery/howard-o3-split/`

该 suite 全量导入：

- `/sn640/NotDec-Exp/ICSE-HOWARD/splited/splitted_irs`

中所有 `*.o3*.ll` 输入，共 92 个 case，覆盖：

- `fortune`
- `grep`
- `gzip`
- `lighttpd`
- `wget`

当前策略：

- 先全量入库到 `cases/`
- 先接入 `CTest`
- `manifest.json` 中统一标记为 `skip`
- 后续再逐步从 `skip` 提升为 `xfail/pass`，并补 `expected/tr-level-2/*.htypes`

初步人工 baseline 探测表明，前若干个 O3 case 在 `--tr-level=2` 下大多会触发：

- `PNDiff assertion while looking up PNI variable`

因此本批提交不尝试一次性打通该语料，只保证：

- 语料完整入库
- suite 结构与主测试布局一致
- 现有 `ctest` 不被新语料阻塞

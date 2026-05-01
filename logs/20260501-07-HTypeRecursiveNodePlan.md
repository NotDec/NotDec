# 2026-05-01 HType 显式递归类型节点方案计划

## 背景

当前 `fortune frozen` 的 `.c` 路径会卡在 Clang record layout。

已经确认的问题不是：

- `NOTDEC_BINARYSUB_TRACE`
- stable string / `llvm::Value::print()`

而是某些字段在 `llvm2c` 降级时被当成按值聚合类型，最后形成了
`struct/union` 之间的按值环。

现状里：

- `UType` 有显式 `URecursiveType`
- 但到 `HType` 阶段，这个信息会被消掉
- 后续只能靠 `RecordDecl/UnionDecl` 的字段图重新推断递归

这会带来两个问题：

1. “这是递归绑定” 这个语义在 `HType` 之后丢了
2. 后端很难区分：
   - 合法的 pointer 递归
   - 非法但语义上有意义的 by-value 递归

## 目标

考虑在 `HType` 里增加显式递归类型节点，保留从 `URecursiveType`
带下来的递归信息，让后续阶段不用只靠 decl 图猜。

希望最终能支持两件事：

1. 类型恢复阶段保留“这里本来就是递归类型”的语义
2. `llvm2c` 可以基于这个信息决定：
   - 什么时候输出正常的 pointer 递归
   - 什么时候打印 warning，并输出非法但更贴近语义的 by-value 递归 C

## 当前判断

这个方向比单纯在 `TypeManager` 里打补丁更系统，因为它解决的是“信息丢失”
的问题，不只是某一个用例的后端卡死。

但它也比后端局部修复更重，风险主要在于：

- `HType` 的 canonical / print / compare 逻辑要一起想清楚
- set/inter/union/typedef/pointer 和递归节点怎么组合，需要统一规则
- 现有很多地方默认 `RecordDecl/UnionDecl` 就足够表达递归，改完可能会影响
  其他 case 的输出和调试信息

所以这条路线适合作为一个明确方案先设计清楚，再决定是否实现。

## 方案设想

### 1. 在 HType 中显式表示递归

新增一种递归类型节点，表达“这里引用的是某个递归 binder”，而不是立刻把它
摊平成 `RecordDecl/UnionDecl` 的字段自引用。

这样 `HType` 里就能直接保留：

- 递归绑定名
- 递归体
- 某个字段是在“递归体内部回指 binder”

而不是等到后面只剩一个普通的 decl 图。

### 2. UType -> HType 转换时不要彻底抹掉 URecursiveType

当前 `TypeBuilder` 在处理 `URecursiveType` 时，会先建 anchor，再把 body 塞回
去，最后只留下普通的 `RecordDecl/UnionDecl` 引用关系。

如果要保留递归信息，这一步需要调整为：

- 仍然可以保留 anchor，方便已有结构复用
- 但不能把“递归绑定”这层语义完全吃掉
- 至少要让后续还能判断：某个字段是普通嵌套，还是递归回边

### 3. llvm2c 根据递归节点决定输出策略

有了显式递归节点后，`llvm2c` 不必再靠 Clang layout 失败后倒推。

可以提前分流：

- pointer 递归：正常降成合法 C
- by-value 递归：打印 warning，并走“仅用于输出语义”的路径

这样就可以把“非法但更贴近语义的 C 输出”做成一个明确策略，而不是依赖 Clang
AST 是否能接受。

## 进一步规划

### 当前假设

1. 先不改 binarysub 的递归求解结果，只在 `UType -> HType` 这层保留递归信息。
2. 现有 `RecordDecl::Field.Type` 存的是字段地址类型，llvm2c 声明字段前会剥一层指针类型。
   所以判断递归是否合法时，必须看剥一层指针类型的“字段值类型”，不能直接看 `Field.Type`。
3. 递归节点先服务 HType dump 和 llvm2c 降级，不先引入新的 HType 子类型规则。

### 递归类型节点

建议不要只加一个 `RecursiveType`。至少需要两个概念：

1. `RecursiveBinder`
   - 类似一个轻量 decl，不直接等同于 `RecordDecl`
   - 保存名字、body、可选 size bits、`AnchorDecl`
   - `AnchorDecl` 是后端 C 输出用的 `RecordDecl/UnionDecl` 壳
   - body 后填充，因为转换 body 时会先遇到回边

2. `RecursiveBindingType`
   - 表示 `mu name. body`
   - 只保存 `RecursiveBinder *`
   - 类型 identity 来自 binder，不来自 body 结构

3. `RecursiveRefType`
   - 表示递归体内部对 binder 的回指
   - 也只保存 `RecursiveBinder *`
   - 和普通 `TypeVariableType` 分开，避免把“未知类型变量”和“已知递归回边”混在一起

这样后续遍历 HType 时可以直接看出哪里是回边，不需要再从 `RecordDecl` 图里猜。

### factory / canonical 规则

递归类型不要按 body 做结构化 interning。body 会后填充，也可能成环，拿它当
factory key 或 canonical key 都容易把问题做复杂。

建议规则：

1. `HTypeContext` 管理 `RecursiveBinder`，就像现在管理 decl 一样。
2. `RecursiveBindingType` / `RecursiveRefType` 的 factory key 都用：

```text
(isConst, RecursiveBinder *)
```

3. canonical 只处理 const，不展开 body：
   - const binding 的 canonical 是非 const binding
   - const ref 的 canonical 是非 const ref
   - 非 const binding/ref 的 canonical 是自己
4. set union/intersection 比较 canonical 时，递归 ref 不会被折成 body 或普通
   `RecordPtrType`，这样不会把“这里是递归回边”的信息抹掉。

这个设计和 `RecordDecl/UnionDecl` 当前规则一致：decl 类型的 identity 是 decl，
不是字段结构。递归 binder 的 identity 也是 binder，不是 body 结构。

### TypeBuilder 转换路线

`convertRecursive()` 可以改成三步：

1. 进入 `URecursiveType` 时创建一个 binding，占位放进递归名字表。
2. 转换 body。`convertVariable()` 遇到当前递归名字时，返回 `RecursiveRefType`，
   不再直接返回普通 `RecordPtrType` anchor。
3. body 转换完后，把 body 挂回 binding，再根据 body 的外形决定 C anchor：
   - body 是 record/union：anchor 就是这个 decl
   - body 是 pointer/set/其他：保留 binding 本身，后端再决定如何降级

为了少改现有代码，可以第一版仍保留 `getOrCreateStruct()` 的 anchor，但 anchor
只作为 C 输出名字和前置声明用，不能再把回边吞成普通 record 字段。

### 两类递归的判定

需要在 HType 层做一个递归 occurrence 分类。遍历 `RecursiveBindingType::body`，
遇到 `RecursiveRefType` 时，看从 binding body 到这个 ref 的路径。

分类规则：

1. pointer 递归
   - 路径上至少经过一层真实 C 指针
   - 注意字段 storage pointer 不算。也就是说，先从字段地址类型剥一层，再看字段值。
   - 例子：`struct S { struct S *next; }`
   - 这是合法 C，正常输出。

2. embedded 递归
   - 路径上没有真实 C 指针，回边按值出现
   - 例子：`struct S { struct S child; }` 或 `struct S { struct S child[]; }`
   - 这不能直接交给 Clang record layout，否则会形成非法按值环。
   - 这类更像数组/重复访问模式，应优先回到 `OffsetRange.access` / stride 信息，
     尝试打印成数组；如果缺少 stride 信息，就 warning 并降级成 `char[N]`
     或语义注释，不能生成自嵌 struct。

这个判定最好做成一个独立 helper，例如：

```text
classifyRecursiveOccurrences(binding) -> { pointerRefs, embeddedRefs }
```

不要把判断散在 `TypeManager::convertStruct()` 和 `defineDecls()` 里。

### embedded 递归的处理策略

embedded 递归不要先尝试输出非法 C。更稳的策略是：

1. 如果回边在 `ArrayType` 下，或者 TypeBuilder 当前正在处理 stride 子问题：
   - 把它标记为 array-like recursion
   - 优先保留数组外壳，但数组元素不能再是同一个完整 recursive ref
   - 第一版可以用一层展开后的元素类型；如果仍然回到自身，就降级为 `char[stride]`

2. 如果回边完全裸露在字段值里：
   - 直接标记为 invalid embedded recursion
   - llvm2c 不创建这个按值字段的 Clang record type
   - 输出 warning，并用 `char[field_size]` 占位，字段注释里写原始递归类型

这样能先保证 `.c` 路径不再被 Clang layout 卡住。后面如果要更像源码，再基于
stride 信息恢复更漂亮的数组类型。

### llvm2c 降级路线

`TypeManager` 需要先加一个递归安全的降级入口：

1. `convertType(RecursiveBindingType)`
   - 如果有 C anchor，降成 anchor 对应 record/union
   - 否则按 body 的可降级结果处理

2. `convertType(RecursiveRefType)`
   - 只有在真实 pointer 下才允许降成 anchor
   - 如果不是 pointer 下出现，交给 embedded 处理逻辑，不直接降级

3. `defineDecls()`
   - 依赖遍历时，pointer 下的 recursive ref 不继续递归展开
   - embedded ref 不参与普通 DFS 定义顺序，避免按值环

4. `convertStruct()` / `convertUnion()`
   - 字段先剥 storage pointer，得到字段值类型
   - pointer recursion 正常声明
   - embedded recursion 走占位字段和 warning

### HType dump

`ValueHTypes.txt` 需要能看出递归边，例如：

```text
struct rec_R {
  rec_R* field_0;
};

struct rec_R_bad {
  <embedded rec_R_bad> field_0;
};
```

具体打印可以模仿结构体类型：给 `RecursiveBinder::AnchorDecl` 一个 `rec_`
开头的名字，把递归绑定打印成一个只有一个成员的结构体壳。壳的成员类型是
body，body 内部遇到 `RecursiveRefType` 时直接打印这个 `rec_` 结构体名。

不要在 dump 里把递归无限展开。`HTypeSnapshotFormatter` 遍历时需要维护当前
binder 栈，遇到 `RecursiveRefType` 只打印 `AnchorDecl` 的名字。

### 实现顺序

1. 先加 HType 节点和 formatter 支持，保证 dump 能表达递归，不接 llvm2c。
2. 改 `TypeBuilder::convertRecursive()` / `convertVariable()`，让回边生成
   `RecursiveRefType`。
3. 加 occurrence 分类 helper，并在 dump 或 debug log 里打印 pointer/embedded 数量。
4. 只让 pointer recursion 进入 llvm2c 正常降级。
5. embedded recursion 先用 warning + `char[N]` 占位，避免 Clang layout 卡死。
6. 最后再考虑基于 stride 信息把 array-like embedded recursion 打印得更准。

### 风险和判断标准

风险：

- HType factory/canonical 逻辑会多一种可递归节点，不能在 canonical 里递归展开。
- `collectType()` / `defineDecls()` 这类 DFS 必须显式处理 recursive ref，否则容易
  不是漏 decl，就是无限递归。
- embedded 递归降级成 `char[N]` 会损失语义，但比输出非法 self-embed struct 更稳。

判断标准：

- `ValueHTypes.txt` 能稳定打印递归绑定和回边。
- pointer 递归输出合法 C，例如自指针字段不触发 record layout 失败。
- embedded 递归不会生成 `struct S { struct S field; }` 这类 C。
- fortune 当前关注用例 `.ll` 同口径运行时间不能明显变慢；`.c` 路径不能卡在
  Clang record layout。

## 本次实现：HType 节点和 UType 转换

这次只实现 HType 表达和 `UType -> HType` 转换，没有接 llvm2c C 降级。

### 修改点

1. `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h`
   - `226-250`：新增 `RecursiveBinder`，保存名字、body、size bits、anchor decl。
   - `286-319`：新增 `TK_RecursiveBinding` / `TK_RecursiveRef` 和判断 helper。
   - `560-586`：新增 `RecursiveBindingType` / `RecursiveRefType`，都只保存 binder 指针。
   - `622-627`、`908-940`：`HTypeContext` 负责持有 binder，并按
     `(isConst, RecursiveBinder*)` factory 出 binding/ref。canonical 只处理 const，
     不展开 body。

2. `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp`
   - `84-90`：新增递归 binder 展示名 helper。
   - `290-295`：`HType::getAsString()` 支持 binding/ref。
   - `351-357`：snapshot formatter 保留 `rec_` 开头的 anchor decl 名字。
   - `403-419`：`collectType()` 支持递归 binding/ref，遇到 anchor decl 后停止展开。
   - `600-613`：`formatType()` 支持递归 binding/ref。

3. `include/notdec/TypeRecovery/mlsub/TypeBuilder.h`
   - `44-46`：递归名字表从 `HType*` 改成 `RecursiveBinder*`。

4. `src/TypeRecovery/mlsub/TypeBuilder.cpp`
   - `728-766`：`convertRecursive()` 创建 `rec_` anchor decl 和 binder，把 body 后填回 binder，
     并生成单字段递归壳。
   - `1639-1643`：`convertVariable()` 在递归体内返回 `RecursiveRefType`，不再返回普通
     record anchor。

### 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. fortune 当前关注 `.ll` 口径

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.htype-recursive-node.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-htype-recursive-node
```

结果：通过，`real 15.12s`。当前参考是 `real 16.33s`，没有性能下降。

`ValueHTypes.txt` 里已经能看到 `struct rec_...` 壳，例如：

```text
struct rec_3326 {
  (ptr<load=void, store=void, psize=32> & struct_3*)* field_0; /* recursive body at offset: 0 */
};
```

3. llvm-ir suite

```bash
python3 test/run_type_recovery_suite.py \
  --binary ./build/bin/notdec \
  --manifest test/type-recovery/llvm-ir/manifest.json \
  --project-root /sn640/NotDec \
  --workdir /tmp/notdec-suite-llvm-ir-htype-recursive-node
```

结果：`11 passed, 3 xfailed, 6 failed`。

失败都是 `.htypes` golden mismatch。主要原因是递归类型现在显式打印成 `rec_`
anchor，不再被摊平成普通 `struct_N`。例如 `06_SimpleRecursive2` 从
`struct_0` 自指针变成 `struct rec_13` 加递归 body 字段。

另有 `12_Stack1` 出现 `void* -> void**` 的既有 pointer-storage 语义差异，这个
和本次递归节点不是同一类问题，先不在这里处理。

4. 小递归 `.c` 路径

```bash
./build/bin/notdec test/type-recovery/llvm-ir/cases/06_SimpleRecursive2.ll \
  -o /tmp/06.rec-node.out.c --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-06-rec-node-c
```

结果：失败在 `TypeManager::convertType()` 的 unhandled type 断言。原因是 llvm2c
还没有接 `RecursiveBindingType` / `RecursiveRefType`，符合本次实现边界。

### 当前评分

- 实现效果：7/10。HType 已能保留递归 binder/ref，dump 能看出回边。
- 复杂度：6/10。新增两个 HType 节点和一个 binder，identity 规则清楚，但后端还没接。
- 维护成本：6/10。factory/canonical 没展开 body，风险可控；后续主要工作在 llvm2c
  降级和 embedded 递归分类。

更好的后续方案：下一步不要直接在 `convertType()` 里硬降级所有递归节点，而是先加
递归 occurrence 分类 helper，明确区分 pointer recursion 和 embedded recursion。

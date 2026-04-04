# HType / UType 设计盘点

## 目标

本文先盘点 `binarysub` 里的 `UType` 类型体系，再盘点当前 `llvm2c` 里的 `HType` 类型体系，最后总结当前更合适的设计方向，尤其是：

- 如何表达类型变量和类型不确定性
- 是否需要递归类型
- 如何处理 union / intersection
- 如何兼顾 Clang AST / C/C++ 输出与 richer type display

## 一、UType 盘点

`UType` 位于：

- `external/binarysub/include/binarysub/binarysub.h`

它是 `binarysub` 面向用户展示、同时也保留推理语义的一套类型表示。当前的节点包括：

### 1. 顶底类型

- `UTop { size }`
- `UBot { size }`

含义：

- `UTop` 表示“最泛”的类型
- `UBot` 表示“最精确/最空”的类型
- 两者都带有 `size`

这里的 `size` 是重要语义，不只是打印信息。当前 `binarysub::get_size()` 也会依赖它。

### 2. 类型变量

- `UTypeVariable { name, size }`

含义：

- 表示未知类型变量
- 这是当前表达“不确定类型”“多态感”的核心载体
- `name` 可以是普通变量名，也可能是递归变量名，例如 `μ0`

### 3. 基础类型

- `UPrimitiveType { name, size }`

当前用于表达例如：

- `sint`
- `uint`
- `float`
- `double`
- `char`
- `bool`

它既保留 primitive 种类，也保留 bit size。

### 4. set-theoretic 类型

- `UUnion { lhs, rhs }`
- `UInter { lhs, rhs }`

含义：

- `UUnion` 表示类型并
- `UInter` 表示类型交

这部分直接来自 MLsub / Simple-sub 风格的输出语义。它们不是 C 语言里的 `union`，而是“类型层面的并/交”。

### 5. 函数类型

- `UFunctionType { args, result }`

含义：

- 参数列表 `args`
- 返回类型 `result`

这是标准的函数类型节点。打印时当前是 `arg1 -> arg2 -> result` 风格。

### 6. record 类型

- `URecordType { fields }`

含义：

- 表示字段集合
- 字段名当前是字符串
- 在 binary recovery 里，字段字符串很多时候其实是偏移/访问路径编码，不是源码级字段名

这更接近“结构化内存对象的抽象类型”，而不是直接等价于 C struct。

### 7. 递归类型

- `URecursiveType { name, body }`

含义：

- 显式的 `μ` 风格递归绑定
- 用于把类型图中的环重新表达为有名递归类型

这是 `UType` 很关键的能力，因为 binarysub 推理结果天然可能出现环，例如自引用结构体、递归函数类型、通过变量界形成的递归约束。

### 8. 指针类型

- `UPointerType { load, store, psize }`

含义：

- `load` 表示从该指针读取出来时的类型
- `store` 表示写入该指针时约束的类型
- `psize` 表示访问大小

这不是 C 里普通的 `T*`，而是带有 variance / load-store 双向语义的 richer pointer type。

### 9. UType 的总体特征

`UType` 的设计目标不是“直接生成 C/C++ 类型”，而是：

- 保留类型推理中的精确信息
- 保留 unknown / polymorphic / recursive 信息
- 保留 set-theoretic 信息
- 保留指针 load/store 双向信息

所以它本质上是：

- semantic type
- inference type
- display type

而不只是 codegen type。

## 二、当前 HType 盘点

`HType` 位于：

- `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h`

它是当前 `llvm2c` 侧的类型表示。其主要目的是：

- 进入 `ClangTypeResult`
- 转成 `clang::QualType`
- 驱动 AST 构造
- 参与最终 C 风格输出

### 1. HType 主体

当前 `HTypeKind` 包括：

- `TK_Integer`
- `TK_Float`
- `TK_Pointer`
- `TK_Function`
- `TK_Record`
- `TK_Union`
- `TK_Array`
- `TK_Typedef`
- `TK_TypeVariable`

此外有 canonical type、const qualifier 等基础机制。

### 2. 简单类型

#### IntegerType

- bit size
- signed / unsigned

#### FloatingType

- bit size

#### PointerType

- `PointeeType`

这里的指针已经退化为普通 C 风格的单 pointee 指针，无法表达 `load/store` 双向语义。

### 3. 函数类型

#### FunctionType

- `ReturnType`
- `ParamTypes`

这个节点更接近 C/C++ 函数签名结构。

### 4. 复合类型

#### RecordType

- 指向 `RecordDecl`

#### UnionType

- 指向 `UnionDecl`

#### ArrayType

- 元素类型
- 元素数量

注意：这里的 `UnionType` 是 **C union / layout union**，不是 `A | B` 这种 set-theoretic union。

### 5. 别名与变量

#### TypedefType

- 指向 `TypedefDecl`

#### TypeVariableType

- 只有一个 `Name`

这一点说明 `HType` 已经意识到“类型变量”有需求，但目前只做到了占位，没有形成完整的 lower / AST / printer 方案。

### 6. HType 当前缺失的语义

相比 `UType`，当前 `HType` 缺失：

- `Top`
- `Bottom`
- set-theoretic `Union`
- set-theoretic `Intersection`
- 显式 `RecursiveType`
- richer pointer 的 `load/store`
- 类型变量的 bound / scheme / size

换言之，`HType` 现在更接近：

- concrete carrier type
- backend type

而不是完整的推理语义类型。

### 7. HType 当前和 Clang 的关系

当前 `ClangTypeResult::convertType(HType *T)` 只稳定处理：

- integer
- float
- pointer
- array
- record
- union
- typedef

它并没有完整支持：

- `TypeVariableType`
- 任意 richer symbolic type

所以如果直接把 `HType` 扩展成完整 `UType` 语义层，再强行 lower 到 `clang::QualType`，会把现有 `llvm2c` 后端复杂度显著拉高。

## 三、UType -> HType 的转换与需要重构的地方

当前转换主要在：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp`

### 1. 类型变量

`UTypeVariable` 当前会转成：

- `TypeVariableType`

保留名字

### 2. 递归类型

`URecursiveType` 当前基本是：

- 继续转 `body`
- 并没有形成单独的 `HType` 递归节点

循环检测主要依赖 `InProgress + TypeCache`，并在遇到环时临时造一个 `struct*` 占位。

这说明当前实现已经在“工程上”支持一类 recursive shape。

TODO，这里的实现需要修改，对于 URecursiveType 直接强制让它变成struct类型，并提前放到TypeMap里面。后续不要去考虑遇到环然后再强制把类型弄成结构体的逻辑（需要去掉这一部分逻辑），而是assert不会出现这种情况。如果发现具体的类型是比如常量类型，或者指针类型（包括结构体指针）时，同样看TypeMap里是否已经赋了结构体类型，此时将具体类型作为结构体类型的唯一成员。

### 3. 指针类型

`UPointerType` 当前会被近似成普通 `PointerType`：

- 优先取 `store`
- 如果 `store` 不足够，再回退 `load`
- `load/store` 的双向信息会丢失

TODO，这里也需要重构。HType也需要引入额外的DualPointer类型，类似PointerType，但是同时有load和store两个指向的类型。完全保留UPointerType的信息。后续转语法树考虑创建一个叫做Ptr的template struct，打印为类似`Ptr<LoadTy, StoreTy>`这种形式。

### 4. UUnion / UInter

当前 `UUnion` / `UInter` 并不会映射成新的 `HTypeKind`。

而是：

- `doUnion`
- `doInter`

做近似合并。

其中：

- `doUnion` 目前大致返回共同超类型，实在不行退成 `void*`
- `doInter` 目前并没有真正实现

TODO，这里需要重构，在HType里面加入Union类型和Inter类型，直接完整保留这一块。由于类型之间的互斥，如果是指针类型就肯定不是primitive类型，所以理论上大概Union或Inter有两部分，其中一部分是类型变量部分，另外一部分是其他已知类型部分。其中这里其他部分应该都可以merge起来，其中HType这里主要处理Primitive类型的merge，因为其他的类型应该在binarysub里面合并过了（可以assert合并过）。

### 5. layout union

另一方面，在结构恢复时，如果字段发生重叠，当前会专门构造：

- `UnionDecl`
- `HType::UnionType`

这表达的是 **布局层的 C union**，是合理且有用的。

但它与 `UType` 里的 `UUnion` 语义不同，不能混为一谈。

## 输出策略：合法 C/C++ + symbolic 注释

当前 `llvm2c` 已经有 comment 挂载和打印机制：

- `TypedDecl::Comment`
- `FieldDecl::Comment`
- `DeclPrinter::printDeclComments`

因此可以直接用注释承载 richer type info。特别是，对于union了或者inter了类型变量的，用注释标注出这些类型变量。

TODO，设计一下打印的格式。

## Commits规划

建议不要把这次重构作为一个“大提交”推进，而是按“先扩 HType 语义层，再改 UType -> HType 转换，最后改 Clang/C 输出策略”的顺序拆开。这样每一批提交的风险边界更清晰，也更容易在 `notdec.decompile.llvm_ir.tr_level_2` 上定位回归。

### commit 1: 扩 HType 节点与基础工厂，不改转换语义

目标：

- 先把 `HType` 本身补成能承载 richer type 的数据结构
- 暂时不改 `TypeBuilder` 的主逻辑，只让新节点可以被创建、缓存、打印字符串

建议改动：

- 在 `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h` 增加新的 `HTypeKind`
  - `TK_DualPointer`
  - `TK_SetUnion`
  - `TK_SetInter`
- 明确把现有 `TK_Union` 保留为 layout union，避免和 set-theoretic union 混淆
- `UTop` / `UBottom` 这一轮先不要引入独立 `HType`
  - 后续在 `UType -> HType` 转换时直接映射成 primitive 占位类型
  - 先避免把 `HType` 扩成一个过大的 lattice
- 给 `TypeVariableType` 补充必要元信息
  - 至少考虑 `size`
  - 如果后面需要注释输出，可预留 bound / origin comment 的扩展位
- 在 `HTypeContext` 里增加对应 factory 和 uniquing key
- 更新 `HType.cpp` 的 `getAsString()` / 调试打印，让新类型至少能稳定显示

这一提交不要做的事：

- 不改 `ClangTypeResult::convertType`
- 不改 `TypeBuilder::convert()` 的语义
- 不改 AST printer

验收标准：

- 新 `HType` 节点可以单独构造并打印
- 现有 `llvm2c` 代码路径不受影响

当前完成情况：

- 已完成
  - `HType` 新增 `TK_DualPointer`、`TK_SetUnion`、`TK_SetInter`
  - 已增加对应的 `DualPointerType`、`SetUnionType`、`SetInterType`
  - `HTypeContext` 已补上对应 factory 和 uniquing key
  - `TypeVariableType` 已补充 `size` 元信息
  - `HType::getAsString()` 已支持上述新节点和带 size 的类型变量调试打印
  - `mlsub::TypeBuilder::convertVariable()` 已把 `UTypeVariable::size` 传入 `TypeVariableType`

这样拆的原因：

- 先把 carrier type 层补齐，后面每一步都可以直接落到明确的数据结构上，而不是边改 builder 边发明接口

### commit 2: 重构递归类型承载，去掉“遇环时临时造 struct*”的兜底逻辑

目标：

- 把 `URecursiveType` 的处理从“遇到环再修补”改成“显式递归节点先占位，再填充 body”
- 先解决最容易把后续逻辑搞乱的缓存和递归建模问题

建议改动：

- 重构 `src/TypeRecovery/mlsub/TypeBuilder.cpp` 中的 `convert(UTypePtr Ty)`
- `URecursiveType` 进入时立即分配对应的 struct carrier，并写入 `TypeCache`
- 后续统一通过 `TypeCache` 找回这类 recursive anchor
- 删除或收紧当前 `InProgress` 命中后直接创建匿名 `struct*` 的逻辑
  - 理想状态是只允许显式 `URecursiveType` 形成环
  - 如果还遇到未预期的环，直接 `assert`，不要继续 silently degrade
- 对“递归体最终是 primitive / pointer / struct pointer”的场景，按文档里的方案把具体类型收束成该 struct 的单一成员

这一提交不要做的事：

- 不引入 `DualPointer`
- 不引入 set-theoretic union/inter lower
- 不改 Clang 输出层

验收标准：

- 递归类型的缓存路径稳定
- 不再依赖 “InProgress 命中后临时造 struct*” 的隐式行为
- 至少能稳定覆盖当前几个已知 recursive shape

当前完成情况：

- 已完成
  - `mlsub::TypeBuilder` 新增 `convertRecursive()` 和 `finalizeRecursiveType()`，把递归类型处理拆成“先建 anchor，再转 body，最后收束”的显式路径
  - `URecursiveType` 进入转换时会立即创建并缓存对应的 struct carrier，后续统一通过 `TypeCache` 复用这类 recursive anchor
  - `RecursiveTypeNames` 现在在递归转换期间显式绑定到 anchor，`UTypeVariable` 命中同名递归变量时可以直接回到该 anchor
  - `convert(UTypePtr Ty)` 不再在 `InProgress` 命中时静默创建匿名 `struct*` 兜底；现在要求环必须由显式 `URecursiveType` 锚定，否则直接报错
  - `convertPointer()` 末尾原先那段“如果缓存里已经有 struct，就把当前具体类型强制塞成该 struct 唯一成员”的补丁式逻辑已经移除，统一交给 `finalizeRecursiveType()` 处理
  - 对“递归体最终不是 struct*”的情况，会把最终具体类型作为 anchor struct 的唯一字段收束进去；如果 body 本身已经是同一个 struct pointer，则直接复用，不再额外包一层

这样拆的原因：

- 递归类型和缓存策略是 `TypeBuilder` 的地基。如果它还在变，后面的 pointer / union/inter 重构都很容易反复返工

在继续推进 commit 3 之前，先记录一个更直接的结构体建模方向，后续实现优先评估是否能按这套方式收敛：

- 尽量让 `HType` 的结构体语义更贴近当前 `UType`
- `HType` 里的“结构体类型”直接表示最终会交给 Clang/llvm2c 的结构体指针语义
  - 也就是说，不再强依赖“`PointerType<RecordType>` 才表示结构体指针”这层额外 carrier
- 如果某个结构体成员本身仍然是“结构体类型”，则把它解释为内嵌结构体
  - 这里的含义更接近 layout / field tree，而不是再次额外解引用一层
- 真正的叶子节点上，如果出现普通 `PointerType`，则表示在这个位置 `load/store` 之后得到的成员值类型
  - 也就是字段值本身是指针，而不是外层结构体 carrier
- 按这个方向，结构体层和叶子 pointer 层的职责会更清楚：
  - 结构体类型负责描述内存布局和字段树
  - 叶子指针负责描述字段值在解引用后的类型
- 如果这条路可行，后续应优先减少“先把结构体表示成 pointer carrier，再从 pointer carrier 反推出字段值类型”的中间层
- commit 3 的实现需要重点重新评估下面几处：
  - `TypeBuilder::getStructOrNull()` / `getOrCreateStruct()` / `finalizeRecursiveType()` 对 recursive anchor 的假设
  - `craftStruct()` 对字段类型的处理是否还能直接使用结构体类型本身，而不是先假设 offset edge 一定是 pointer-to-field
  - `HType::RecordType`、`DualPointerType`、普通 `PointerType` 三者的边界
  - `llvm2c` 当前 `convertType()` 对 `RecordType` 的解释是否需要配套调整

如果最后发现“让 `RecordType` 直接承担结构体指针语义”会把现有 Clang type 约定打穿，再退回到当前的双层方案；但实现时应优先验证这条更简单的模型，而不是默认维持现有复杂投影层。

### commit 3: 引入 DualPointer，完整保留 `UPointerType { load, store, psize }`

目标：

- 让 `HType` 侧先不丢失 `binarysub` 的 pointer 双向语义

建议改动：

- 在 `HType` 中新增 `DualPointerType`
  - 保存 `load`
  - 保存 `store`
  - 保存 `psize`
- `TypeBuilder::convertPointer()` 不再优先 `store`、回退 `load`
- 对 `UPointerType` 统一生成 `DualPointerType`
- 只在明确需要退化成普通 C 指针的边界，才做受控降级

这一提交可以顺手做的事：

- 提供 `DualPointerType -> comment string` 或 `getAsString()` 规则，方便后面输出注释

这一提交不要做的事：

- 不把 `DualPointerType` 直接塞进 `ClangTypeResult::convertType`
- 不在这一批里同时设计 `Ptr<LoadTy, StoreTy>` 模板输出

验收标准：

- `UPointerType` 信息在 `HType` 中不丢失
- `TypeBuilder` 里不再有“优先取 store，不行再取 load”的近似策略

这样拆的原因：

- DualPointer 是语义保真最核心的一块，但它对 C backend 的影响也最大，所以要先完成内部表示，再单独做输出策略

当前完成情况：

- WIP
  - `TypeBuilder::convertPointer()` 已改为对顶层 `UPointerType` 统一生成 `DualPointerType`
  - `TypeBuilder::convertFieldType()` 也已开始对叶子位置的 `UPointerType` 保留 `DualPointerType`，不再在 builder 阶段优先选 `store/load` 并拍扁成单一类型
  - 当前仍未打通的边界：
    - `llvm2c::TypeManager::convertType()` 还没有为 `DualPointerType` 提供受控降级
    - 因此这一步目前仍是“语义层已保留，但后端输出未完全接上”的中间态
  - 这一阶段如果单独提交，应继续以 `WIP:` 前缀标记，直到 `DualPointerType` 至少能在当前测试路径上被稳定降级或绕开

### commit 4: 引入 set-theoretic `Union/Inter`，先保留语义，不急于落 C 类型

目标：

- 让 `UUnion` / `UInter` 能在 `HType` 中完整存在，而不是在 builder 阶段被拍扁

建议改动：

- 在 `HType` 中加入 set-theoretic union / intersection 节点
- 把当前 `doUnion()` / `doInter()` 的职责从“强行合并成某个 backend type”调整为：
  - 只处理少数确定可 merge 的 primitive 情况
  - 其他情况直接构造 `SetUnionType` / `SetInterType`
- 明确 layout union 和 set union 的分层：
  - layout overlap 仍然走 `UnionDecl` / `UnionType`
  - MLsub 语义并/交走新的 `SetUnionType` / `SetInterType`

建议同时加的约束：

- 如果 binarysub 理论上已经把非 primitive 的结构合并过了，这里可以用 `assert` 守住假设，避免 silent fallback

这一提交不要做的事：

- 不要求 `convertType()` 支持这些类型
- 不要求最终打印成合法 C 声明

验收标准：

- `UUnion` / `UInter` 不再在 `TypeBuilder` 里丢语义
- 调试输出里可以直接看见 union/inter 的结构

这样拆的原因：

- 这一步本质上是“语义层保真”，和“如何打印成 C”是两件事，强行捆在一起会让问题来源不清楚

### commit 5: 建立 `HType -> Clang/C 输出` 的降级边界与注释策略

目标：

- 明确哪些 `HType` 能直接转成 `clang::QualType`
- 明确哪些 `HType` 只能降级成 carrier type，并把 richer info 挂到注释里

建议改动：

- 重构 `external/NotDec-llvm2c/lib/notdec-llvm2c/TypeManager.cpp` 的 `ClangTypeResult::convertType(HType *T)`
- 增加一个“可直接 lower 到 Clang”的判定层
  - integer / float / pointer / array / record / layout union / typedef 仍走现有逻辑
  - `TypeVariable` / `DualPointer` / `SetUnion` / `SetInter` 统一走受控降级
- 降级策略单独集中，不要散在各个 case 里
  - 例如降级成 `void *`
  - 或降级成承载它的 record / typedef
- 同时定义注释格式，把 richer type 信息挂到
  - `TypedDecl::Comment`
  - `FieldDecl::Comment`
  - 必要时某些 value decl comment

建议的输出格式可以先从简单版本开始：

- `mlsub: 'a`
- `mlsub: load=..., store=...`
- `mlsub: (A | B)`
- `mlsub: (A & B)`

不要在这一提交一开始就追求特别复杂的人类可读格式，先保证：

- 稳定
- 可 grep
- 能和 golden 一起回归

验收标准：

- 新增 semantic HType 后，`llvm2c` 仍能产出合法 C/C++
- richer info 不再靠隐式丢失，而是显式降级 + comment 保留

这样拆的原因：

- 这是 backend 行为变化最大的提交，单独拿出来最利于回归测试和 golden 更新

### commit 6: 清理遗留近似逻辑，补测试与 golden

目标：

- 把前面几批留下的旧 fallback 彻底收口
- 用测试把新分层固定下来

建议改动：

- 删除旧的近似分支
  - 指针优先 `store/load` 的 fallback
  - `doUnion()` / `doInter()` 里无依据的拍扁逻辑
  - 非显式递归导致的隐式 struct 兜底
- 增加或更新测试
  - `notdec.decompile.llvm_ir.tr_level_2`
  - 如果已有针对 `llvm2c` 的更小粒度测试入口，也补上针对新注释/新打印的回归
- 只在这里统一更新 golden，避免前几个提交每次都大面积刷新输出

验收标准：

- 旧近似逻辑只剩明确保留的受控降级点
- 集成测试能够反映新的输出策略

这样拆的原因：

- 清理和 golden 更新通常噪音最大，单独放最后一批，便于审查前面几次提交真正的语义变化

## 一个更现实的提交合并方案

如果实际开发过程中发现 6 个提交过细，可以合并成下面 4 批，仍然比较稳：

1. `HType` 扩展 + 基础打印
2. 递归类型重构 + `DualPointer`
3. set-theoretic `Union/Inter` 接入 `TypeBuilder`
4. Clang/C 降级输出 + comments + 测试/golden

不建议再往更粗合并，尤其不要把“语义层扩展”和“backend 输出变更”放进同一个首提交里；那样一旦测试炸掉，很难判断问题出在类型表示、builder 缓存，还是 C backend 降级策略。

## 文档记录约定

- 每次某个 commit 规划项有实质实现后，都在对应小节下追加“当前完成情况”
- 如果该提交刻意允许暂时不可运行或 backend 尚未打通，明确标注 `WIP`
- 提交代码时把对应文档更新一起提交，避免代码状态和设计记录脱节

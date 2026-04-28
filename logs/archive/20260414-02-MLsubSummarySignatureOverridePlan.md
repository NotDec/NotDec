# MLsub `NOTDEC_SIGNATURE_OVERRIDE` 迁移计划

日期：2026-04-14

## 1. 结论

这次只关注 `NOTDEC_SIGNATURE_OVERRIDE`。

截至当前代码，MLsub 路线还没有接入 `NOTDEC_SIGNATURE_OVERRIDE`。最合适的第一版落点不是恢复旧路径的 top-down signature graph，而是直接在 MLsub 的函数边界上注入一个显式函数类型约束。

当前推荐语义：

- signature override 是“函数边界类型的下界约束”
- 实现上对应 `OverrideTy <: F`
- 其中 `F` 是 MLsub 中该函数自己的函数节点
- `OverrideTy` 的构造过程中允许引入函数内局部类型变量，以及额外的显式子类型约束
- primitive 节点应直接支持 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 里定义并已注册到 binarysub registry 的 semantic lattice canonical name

这样和当前 MLsub 代码最贴近，改动也最小。

## 2. 当前代码证据

### 2.1 MLsub 目前没有 signature override 入口

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h)
  - 当前 `class MLsubRecovery` 位于 328-374 行
  - 这里没有：
    - `SigFile`
    - `SignatureOverride`
    - `loadSignatureFile()`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
  - `MLsubRecovery::run()` 位于 289-400 行
  - 当前只做 workdir、call graph、SCC、bottom-up、top-down
  - 没有读取 `NOTDEC_SIGNATURE_OVERRIDE`

### 2.2 MLsub 已经具备“函数边界注入”的天然落点

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h)
  - `ConstraintsGenerator::run()` 位于 76-103 行
  - 这里会先为每个函数构造：
    - 参数节点
    - 返回节点
    - 函数节点
  - 然后执行：
    - `addSubtype(binarysub::make_function(Args, Ret), F);`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
  - `MLsubRecovery::bottomUpPhase()` 位于 402-455 行
  - `G->run()` 之后、`unhandledCalls` 实例化之前，正好是对函数节点追加 override 约束的最佳位置
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
  - `ConstraintsGenerator::MLsubVisitor::visitCallBase()` 位于 1158-1204 行
  - 调用点会构造：
    - `ActualFunc = binarysub::make_function(Args, Ret)`
  - SCC 内调用走：
    - `cg.addSubtype(F, ActualFunc);`
  - 这进一步说明 MLsub 现在本来就是围绕“函数节点 + 函数类型约束”工作的

### 2.3 primitive semantic lattice 已经有现成接入面

- [src/NotDec.cpp](/sn640/NotDec/src/NotDec.cpp)
  - 91-165 行已经提供 `--primitive-semantic-lattice`
  - `configurePrimitiveSemanticRegistry()` 会在主流程启动时把 `.dot` family 文件注册进 `binarysub::globalPrimitiveSemanticRegistry()`
- [external/binarysub/include/binarysub/binarysub-primitive-semantics.h](/sn640/NotDec/external/binarysub/include/binarysub/binarysub-primitive-semantics.h)
  - 20-190 行定义了：
    - `PrimitiveSemanticNodeDescriptor::canonicalName`
    - `PrimitiveSemanticRegistry::findNodeByCanonicalName()`
    - `PrimitiveSemanticRegistry::findFamilyByCanonicalName()`
    - `PrimitiveSemanticRegistry::joinByCanonicalName()`
    - `PrimitiveSemanticRegistry::meetByCanonicalName()`
- [external/binarysub/src/binarysub-core.cpp](/sn640/NotDec/external/binarysub/src/binarysub-core.cpp)
  - 188-230 行的 `constrain_semantic_primitive_subtype()` 已经会在 primitive 名命中 registry 时走 semantic lattice 约束逻辑
- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)
  - 158-175 行的 `TypeBuilder::parsePrimitiveName()` 已经能把 semantic primitive canonical name 还原成 typedef 风格的高级类型输出

这意味着 signature override 不需要再发明一套 primitive semantic alias 机制，而是应该直接消费 registry 当前已经使用的完整 canonical name。

## 3. 推荐实现方案

### 3.1 第一版仍然只在函数边界生效，但需要把局部变量和附加约束一起带上

不要第一版就恢复旧路径那套：

- caller 聚合 signature graph
- top-down graph merge
- graph-level summary / signature clone

MLsub 当前更自然的做法是：

1. 从 JSON 读取某个函数的目标签名
2. 在“单函数 override 构造上下文”里解析：
   - 参数 / 返回值 TypeExpr
   - 函数内局部类型变量
   - signature 级别的显式附加约束
3. 构造 `binarysub::SimpleType OverrideTy`
4. 在 `MLsubRecovery::bottomUpPhase()` 中先灌入额外约束，再执行：
   - `G->addSubtype(OverrideTy, F);`

这里 `F` 是 `G->getNodeOrNull(Func, nullptr, -1)` 取到的函数节点。

### 3.2 具体建议修改的文件、行和函数

建议本批次只动下面这些位置：

- [include/notdec/TypeRecovery/mlsub/MLsubGenerator.h](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h)
  - 328-374 行的 `class MLsubRecovery`
  - 新增：
    - `const char *SigFile`
    - `SignatureOverride` 缓存
    - `loadSignatureFile()` 声明
    - `buildOverrideType()` / `buildOverrideOperand()` 一类 helper 声明
    - `applyOverrideRecipe()` 一类 helper 声明
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
  - 289-400 行的 `MLsubRecovery::run()`
  - 在主流程开始处读取 `NOTDEC_SIGNATURE_OVERRIDE`
- [src/TypeRecovery/mlsub/MLsubGenerator.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp)
  - 402-455 行的 `MLsubRecovery::bottomUpPhase()`
  - 在 `G->run()` 之后、处理 `unhandledCalls` 之前应用 override
- [src/TypeRecovery/mlsub/TypeBuilder.cpp](/sn640/NotDec/src/TypeRecovery/mlsub/TypeBuilder.cpp)
  - 158-175 行的 `TypeBuilder::parsePrimitiveName()`
  - 这里当前已经能消费 semantic primitive canonical name；本批次文档和 loader 设计都应对齐这一路径，不再额外引入 override 私有别名

涉及函数：

- `MLsubRecovery::run()`
- `MLsubRecovery::bottomUpPhase()`
- 新增 `MLsubRecovery::loadSignatureFile()`
- 新增 `MLsubRecovery::buildOverrideType()`
- 新增 `MLsubRecovery::buildOverrideOperand()`
- 新增 `MLsubRecovery::applyOverrideRecipe()`

## 4. MLsub-only 的 signature 文件应该怎么设计

### 4.1 设计原则

如果这个文件只服务 MLsub，不需要兼容旧 retypd 的 signature JSON，那么推荐直接面向 MLsub / binarysub 当前已有的构造能力来设计。

优先目标：

1. 能稳定映射到当前代码里已经在用的类型构造器
   - `binarysub::make_function(...)`
   - `binarysub::make_primitive(...)`
   - `binarysub::make_record(...)`
   - `binarysub::make_ptr_load(...)`
   - `binarysub::make_ptr_store(...)`
2. 支持“同一函数签名内的类型相关性”
   - 例如参数和返回值共享同一个类型变量
3. 支持“signature 级别的显式附加子类型约束”
   - 例如某个局部变量受限于某个具体类型
   - 或两个局部变量之间有额外 `lhs <: rhs` 关系
4. primitive 节点要能直接引用已注册的 semantic lattice canonical name
   - 例如 `prim.uint32.win.HWND`
   - 不再支持在 override 文件里临时起简写别名
5. 不依赖 `printType()` 的文本格式
   - 因为当前仓库有打印器，但没有稳定 parser
6. 第一版避免 union / intersection / recursive type 这些高复杂度特性

### 4.2 推荐顶层结构

推荐使用：

```json
{
  "version": 1,
  "functions": {
    "strlen": {
      "ret": { "kind": "primitive", "name": "uint", "bits": 32 },
      "args": [
        {
          "kind": "ptr",
          "load": { "kind": "primitive", "name": "uint", "bits": 8 },
          "store": { "kind": "primitive", "name": "uint", "bits": 8 }
        }
      ],
      "constraints": []
    }
  }
}
```

理由：

1. `version` 方便后续扩格式
2. `functions` 用函数名做 key，和当前 override 习惯一致
3. 每个函数只保留：
   - `args`
   - `ret`
   - `constraints`
4. 这和 MLsub 当前的“函数类型主约束 + 若干附加 `addSubtype()`”模型对齐

### 4.3 推荐的 TypeExpr 语法

建议第一版只支持下面几种节点：

#### 1. primitive

```json
{ "kind": "primitive", "name": "bool", "bits": 1 }
{ "kind": "primitive", "name": "sint", "bits": 32 }
{ "kind": "primitive", "name": "uint", "bits": 32 }
{ "kind": "primitive", "name": "float", "bits": 32 }
{ "kind": "primitive", "name": "prim.uint32.win.HWND", "bits": 32 }
```

说明：

- primitive 名建议严格跟 MLsub 当前代码保持一致
- 当前 `MLsubGenerator.cpp` 实际已经在用：
  - `bool`
  - `sint`
  - `uint`
  - `float`
- 如果要指定 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 里新增的 semantic primitive：
  - `name` 必须直接写完整 canonical name
  - 必须包含底层类型前缀和 namespace，例如 `prim.uint32.win.HWND`
  - 不接受只写 `HWND` 这种短名
- semantic primitive 的 `bits` 必须和 registry 中该 family 的 `bits` 一致
- override loader 不负责加载 lattice 文件；它只消费已经由 `--primitive-semantic-lattice` 注册好的 canonical name

#### 2. var

```json
{ "kind": "var", "id": 0, "bits": 32 }
```

说明：

- `id` 推荐直接用非负整数，作用域只在单个函数签名内部
- 同一个 `id` 多次出现，表示它们共享同一个 `SimpleType` 变量
- 这是表达“入参与返回值相关联”的关键
- `bits` 要么每次显式写且保持一致，要么在首次定义处写明、后续复用时由 loader 校验一致性

例子：

```json
{
  "ret": { "kind": "var", "id": 0, "bits": 32 },
  "args": [
    { "kind": "var", "id": 0, "bits": 32 }
  ]
}
```

表示一个近似的 identity 风格签名。

#### 3. ref

```json
{ "kind": "ref", "id": 0 }
```

说明：

- `ref` 只用于引用同一函数签名中已经出现过的 `var`
- 它不会新建 `SimpleType`，只是取回已有的局部变量绑定
- 推荐在 `constraints` 里用 `ref`，避免把“声明变量”和“引用变量”混写在一起

#### 4. ptr

```json
{
  "kind": "ptr",
  "load": { "kind": "primitive", "name": "uint", "bits": 8 },
  "store": { "kind": "primitive", "name": "uint", "bits": 8 }
}
```

说明：

- `ptr` 节点不要直接要求 loader 构造单个现成的 `SimpleType`
- 更自然的做法是：
  1. 先创建一个 fresh variable 作为这个位置的主体类型
  2. 如果有 `load`，再加约束 `PtrTy <: make_ptr_load(LoadTy, bits)`
  3. 如果有 `store`，再加约束 `PtrTy <: make_ptr_store(StoreTy, bits)`
- 也就是说，`ptr` 更像“一个带附加约束的类型配方”，而不是一个单独构造器

建议默认规则：

- 若 `load` / `store` 里没有显式位宽，就沿用子类型本身的位宽
- 第一版不单独暴露 `access_bits`

#### 5. record

```json
{
  "kind": "record",
  "fields": [
    {
      "offset": "0",
      "type": { "kind": "primitive", "name": "uint", "bits": 32 }
    },
    {
      "offset": "4",
      "type": { "kind": "primitive", "name": "uint", "bits": 32 }
    }
  ]
}
```

说明：

- `offset` 直接用字符串，和当前 `make_record` 的 field key 对齐
- 第一版建议只支持常量 offset
- 不支持 range / stride / array 语法

#### 6. void

```json
null
```

说明：

- 只用于函数返回值
- 映射到 `make_function(args, nullptr)`

### 4.4 signature 级别附加约束

推荐每个函数对象额外支持：

```json
{
  "constraints": [
    {
      "kind": "subtype",
      "lhs": { "kind": "ref", "id": 0 },
      "rhs": {
        "kind": "primitive",
        "name": "prim.uint32.win.HWND",
        "bits": 32
      }
    },
    {
      "kind": "subtype",
      "lhs": { "kind": "ref", "id": 1 },
      "rhs": { "kind": "ref", "id": 0 }
    }
  ]
}
```

说明：

- `constraints` 的语义直接对应 `ConstraintsGenerator::addSubtype(lhs, rhs)`
- 也就是最终落到 `binarysub::constrain(lhs, rhs, ...)`
- `lhs` / `rhs` 允许是：
  - `ref`
  - 任意完整 TypeExpr
- 因而它可以表达：
  - 某个局部变量和具体 primitive / ptr / record 之间的约束
  - 两个局部变量之间的约束
- 约束作用域仍然只在单个函数 override 内，不支持跨函数引用

### 4.5 第一版不建议支持的特性

为了控制复杂度，建议第一版先不支持：

- union
- intersection
- recursive type
- function-typed field
- callsite-specific override
- vararg
- offset range / stride field
- 跨函数 `var` / `ref` 共享
- 直接复用 `binarysub::printType()` 的文本格式

这些都可以以后再扩。

## 5. 推荐的 loader 语义

### 5.1 不要让 loader 只返回一个 `SimpleType`

因为 `ptr` 节点和显式 `constraints` 都天然需要“主体变量 + 附加约束”，所以更推荐 loader 维护“函数内局部变量表 + recipe”两层状态，并返回：

- 一个主体类型 `RootTy`
- 一组待应用约束

可以抽象成：

```cpp
struct OverrideTypeRecipe {
  binarysub::SimpleType Root;
  std::vector<std::pair<binarysub::SimpleType, binarysub::SimpleType>> Constraints;
};
```

同时还需要一个函数内局部上下文，至少要能记录：

- `var id -> SimpleType`
- 每个 `var id` 的位宽信息
- 当前函数里已经解析出的待应用附加约束

其中：

- `var` 首次出现时创建 fresh variable 并登记到表里
- `ref` 只能从这张表里查，不允许隐式创建
- `primitive` 若命中 semantic lattice canonical name，需立即向 registry 校验名字和位宽

语义是对每个 pair 执行：

```cpp
G->addSubtype(LHS, RHS);
```

这样：

1. primitive / var 只需要返回 `Root`
2. `ref` 只需要解析成已有 `Root`
3. ptr / record 可以额外附带约束
4. signature 级别的 `constraints` 也可以统一追加到同一个 recipe
5. 最后函数签名仍然统一拼成 `make_function(args, ret)`

### 5.2 函数级应用方式

推荐应用流程：

1. `loadSignatureFile()` 只负责 parse JSON 和基本校验
2. `bottomUpPhase()` 中遇到命中函数时：
   - 为该函数创建局部变量上下文
   - 先解析 `args` / `ret`，得到 `OverrideFuncTy`
   - 再解析并展开 `constraints`
   - 先把 recipe 里的附加约束灌进当前 `ConstraintsGenerator`
   - 最后执行 `G->addSubtype(OverrideFuncTy, F)`

这样 override 的类型变量和当前 SCC 的 level、pointer size 都由当前 MLsub 上下文统一决定，不需要在读 JSON 时就固化太多实现细节。

## 6. 一个更完整的示例

```json
{
  "version": 1,
  "functions": {
    "CreateWindowLike": {
      "ret": { "kind": "var", "id": 0, "bits": 32 },
      "args": [
        {
          "kind": "primitive",
          "name": "prim.uint32.win.HINSTANCE",
          "bits": 32
        },
        {
          "kind": "ptr",
          "load": {
            "kind": "primitive",
            "name": "prim.char8.cchar.ascii_char",
            "bits": 8
          },
          "store": {
            "kind": "primitive",
            "name": "prim.char8.cchar.ascii_char",
            "bits": 8
          }
        }
      ],
      "constraints": [
        {
          "kind": "subtype",
          "lhs": { "kind": "ref", "id": 0 },
          "rhs": {
            "kind": "primitive",
            "name": "prim.uint32.win.HWND",
            "bits": 32
          }
        }
      ]
    },
    "memcpy_like": {
      "ret": { "kind": "var", "id": 0, "bits": 32 },
      "args": [
        { "kind": "var", "id": 0, "bits": 32 },
        { "kind": "var", "id": 1, "bits": 32 },
        { "kind": "primitive", "name": "uint", "bits": 32 }
      ],
      "constraints": [
        {
          "kind": "subtype",
          "lhs": { "kind": "ref", "id": 1 },
          "rhs": {
            "kind": "ptr",
            "load": {
              "kind": "primitive",
              "name": "prim.char8.cchar.ascii_char",
              "bits": 8
            },
            "store": {
              "kind": "primitive",
              "name": "prim.char8.cchar.ascii_char",
              "bits": 8
            }
          }
        },
        {
          "kind": "subtype",
          "lhs": { "kind": "ref", "id": 0 },
          "rhs": { "kind": "ref", "id": 1 }
        }
      ]
    }
  }
}
```

这个格式的优点是：

1. 直接面向函数签名
2. 能表达函数内编号变量
3. 能表达变量和具体类型、变量和变量之间的额外约束
4. 能直接引用 semantic primitive canonical name
5. 能表达 pointer 的 load/store 能力
6. 不依赖旧路径的 graph schema

## 7. 推荐的实施顺序

### 阶段 1

补入口，不改求解：

- 在 `MLsubRecovery` 里增加 `SigFile` 和缓存成员
- 在 `MLsubRecovery::run()` 里读取 `NOTDEC_SIGNATURE_OVERRIDE`
- 做 JSON parse、函数存在性检查、格式校验

### 阶段 2

补 loader：

- 把 `TypeExpr` 解析成 `OverrideTypeRecipe`
- 至少支持：
  - `primitive`
  - `var`
  - `ref`
  - `ptr`
  - `record`
  - `null` 返回值
  - `constraints`
- 对 `primitive` 里的 semantic canonical name 做 registry 校验

### 阶段 3

在 `MLsubRecovery::bottomUpPhase()` 应用：

- `G->run()` 后
- `unhandledCalls` 实例化前
- 先灌附加约束
- 再对命中函数执行 `OverrideFuncTy <: F`

## 8. 完成标准

完成后应满足：

1. 设置 `NOTDEC_SIGNATURE_OVERRIDE` 时，MLsub 路线不再静默忽略
2. override 文件能稳定表达 MLsub 关心的函数边界类型
3. override 通过 `bottomUpPhase()` 直接影响后续求解和 `.htypes` 输出
4. 单函数内可通过编号变量和 `constraints` 表达变量-具体类型、变量-变量之间的附加子类型约束
5. override 中可以直接写 semantic primitive canonical name，并复用现有 primitive semantic lattice 求解逻辑
6. 不需要为了第一版功能去恢复旧路径的 top-down signature graph

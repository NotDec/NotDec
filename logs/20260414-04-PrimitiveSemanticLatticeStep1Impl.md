# Primitive Semantic Lattice Step 1 实现记录

日期：2026-04-14

## 完成情况

已完成 `logs/20260414-03-PrimitiveSemanticLatticeDesign.md` 中的 Step 1：

- 实现 primitive semantic lattice 的 descriptor 与 registry
- 实现受限 `.dot` 输入解析
- 实现 Step 1 所需校验
- 补充正反例测试并接入现有 `binarysub` 测试主程序

当前状态：

- 只负责读取、构造、注册、校验 family 定义
- 还没有接入 `external/binarysub/src/binarysub-core.cpp` 的 primitive 求解逻辑

## 具体修改

### 1. 新增 primitive semantic lattice 公共接口

文件：

- `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:15`

涉及结构/类：

- `binarysub::PrimitiveSemanticNodeDescriptor`
- `binarysub::PrimitiveSemanticFamilyDescriptor`
- `binarysub::PrimitiveSemanticRegistry`

说明：

- 在 `external/binarysub/include/binarysub/binarysub-primitive-semantics.h:15-72` 新增了 Step 1 所需的数据结构
- `PrimitiveSemanticFamilyDescriptor` 提供了 `familyTag()`、`familyKey()`、`findNodeById()`、`findNodeByCanonicalName()`、`root()` 查询接口
- `PrimitiveSemanticRegistry` 提供了 `registerFamilyFromDot()`、`registerFamilyFromDotFile()`、`findFamily()`、`findNodeByCanonicalName()` 等注册/查询入口

### 2. 新增 DOT 解析与 family 校验实现

文件：

- `external/binarysub/src/binarysub-primitive-semantics.cpp:53`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:294`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:575`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:753`
- `external/binarysub/src/binarysub-primitive-semantics.cpp:808`

涉及函数：

- `Lexer::tokenize()`
- `Parser::parse()`
- `build_family_descriptor(const ParsedFamily &parsed)`
- `PrimitiveSemanticRegistry::registerFamilyFromDot(std::string_view, std::string)`
- `PrimitiveSemanticRegistry::registerFamilyFromDotFile(const std::string &)`
- `PrimitiveSemanticRegistry::registerFamily(PrimitiveSemanticFamilyDescriptor)`

说明：

- `Lexer::tokenize()` 负责把受限 DOT 文本切成 token，支持字符串、节点 ID、属性表、`->` 边、行注释和块注释
- `Parser::parse()` 负责读取单个 `digraph` family，收集 graph 属性、节点属性和边关系
- `build_family_descriptor()` 在构造 descriptor 时完成 Step 1 校验：
  - `base` / `bits` / `namespace` 必填
  - root 必须且只能有一个
  - canonical name 必须唯一
  - 图必须无环
  - 每个节点必须能到达 root
- `registerFamilyFromDotFile()` 提供了从 `.dot` 文件读取并注册 family 的入口

### 3. 接入 binarysub 构建与测试入口

文件：

- `external/binarysub/CMakeLists.txt:12-18`
- `external/binarysub/include/binarysub/binarysub-test.h:11`
- `external/binarysub/src/binarysub-test-main.cpp:38`
- `external/binarysub/src/binarysub-test.cpp:408-542`

涉及函数：

- `expect_registry_error(const char *dot, const char *expectedSubstring)`
- `test_primitive_semantic_lattice()`

说明：

- 在 `external/binarysub/CMakeLists.txt:12-18` 把 `src/binarysub-primitive-semantics.cpp` 加入 `binarysub_lib`
- 在 `external/binarysub/src/binarysub-test.cpp:426-542` 新增 `test_primitive_semantic_lattice()`，覆盖：
  - 内存字符串注册
  - `.dot` 文件注册
  - root 重复报错
  - 有环报错
  - 不可达 root 报错
  - canonical name 冲突报错
- 在 `external/binarysub/src/binarysub-test-main.cpp:38` 把新测试接到现有 `binarysub` 测试主程序

## 验证

执行命令：

```bash
cmake --build build --target binarysub -j4
./build/binarysub
```

结果：

- `binarysub` 构建成功
- 现有测试与新增 `test_primitive_semantic_lattice()` 全部通过

## 后续建议

Step 2 可以直接在当前 registry/family descriptor 上继续补：

- 祖先闭包或到 root 路径缓存
- `same family` 判断
- `A <: B` 判断

这样后续实现 Step 3/4 的 `join` / `meet` 时不需要再改解析层。

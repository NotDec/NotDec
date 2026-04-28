# Primitive Semantic Lattice CLI 接入记录

日期：2026-04-15

## 完成情况

这一步把 primitive semantic lattice 继续接入到了 `notdec` 主流程，重点是为后续“函数参数类型可指定自定义 lattice type”先把配置链路和后端兜底打通：

- `notdec` CLI 现在可以通过命令行加载一批 primitive semantic lattice `.dot` 文件
- 上述配置已经进入 `notdec::Options`
- `TypeBuilder` 现在能把注册过的 `prim.*` semantic primitive 安全转换成 typedef 风格的 `HType`

当前边界：

- 这次还没有把 semantic lattice 直接施加到函数参数约束生成逻辑
- 这次的目标是先把“CLI -> Options -> registry -> HType fallback”链路稳定下来，为后续参数类型接入留出稳定入口

## 具体修改

### 1. 为 `Options` 增加 primitive semantic lattice CLI 配置承载位

文件：

- `include/notdec/DecompilerContext.h:14-21`

涉及结构：

- `notdec::Options`

说明：

- 在 `include/notdec/DecompilerContext.h:20-21` 新增：
  - `primitiveSemanticLatticeFiles`
- 这样 `src/NotDec.cpp` 解析出来的 lattice 文件配置可以进入 `DecompilerContext`

### 2. 在主程序启动阶段加载并校验 primitive semantic lattice 配置

文件：

- `src/NotDec.cpp:91-95`
- `src/NotDec.cpp:121-132`
- `src/NotDec.cpp:144-162`

涉及对象/函数：

- `primitiveSemanticLatticeFiles`
- `configurePrimitiveSemanticRegistry(const notdec::Options &)`
- `main(int, char **)`

说明：

- 在 `src/NotDec.cpp:91-95` 新增命令行选项：
  - `--primitive-semantic-lattice=<path.dot>`
- 在 `src/NotDec.cpp:121-132` 新增 `configurePrimitiveSemanticRegistry()`：
  - 每次启动先 `clearGlobalPrimitiveSemanticRegistry()`
  - 按顺序加载 `primitiveSemanticLatticeFiles`
- 在 `src/NotDec.cpp:144-162`，`main()` 构造 `notdec::Options` 时保存上述配置，并在真正进入反编译流程前执行 registry 初始化和错误检查

### 3. 为 semantic primitive 增加 HType 层的 typedef 兜底

文件：

- `src/TypeRecovery/mlsub/TypeBuilder.cpp:104-125`
- `src/TypeRecovery/mlsub/TypeBuilder.cpp:129-177`

涉及函数：

- `hexDigit(unsigned)`
- `makeSemanticPrimitiveTypedefName(const std::string &)`
- `notdec::mlsub::TypeBuilder::parsePrimitiveName(const std::string &, std::uint32_t)`

说明：

- 在 `src/TypeRecovery/mlsub/TypeBuilder.cpp:104-125` 新增 semantic primitive typedef 名编码 helper：
  - 把 `prim.uint32.win.HWND` 这类 canonical name 编码成稳定的 C typedef 标识符
- 在 `src/TypeRecovery/mlsub/TypeBuilder.cpp:158-175` 扩展 `parsePrimitiveName()`：
  - 若 primitive 名字能在 global primitive semantic registry 里查到 family
  - 则先按 family 的 `base/bits` 解析出底层 primitive
  - 再创建或复用一个 `TypedefDecl`
  - 使 semantic primitive 在最终 `HType` 层表现为“基于底层 primitive 的命名语义别名”
- 这样后续如果函数参数、summary 或其他约束源引入了 `prim.*` 节点，不会在 `TypeBuilder` 阶段因为 “Unknown primitive type” 断言

### 4. 补充单测覆盖 semantic primitive 到 HType typedef 的转换

文件：

- `unittests/Retypd/GraphTest.cpp:53-96`

涉及测试：

- `TEST(Retypd, TypeBuilderSemanticPrimitiveAliasTest)`

说明：

- 在 `unittests/Retypd/GraphTest.cpp:53-96` 新增测试：
  - 先注册一个最小的 `uint32/win` primitive semantic lattice family
  - 再把 `prim.uint32.win.HWND` 转成 `binarysub::UPrimitiveType`
  - 调用 `TypeBuilder::convert()`
  - 断言结果是 `typedef`，且底层类型为 `uint32`

## 验证

执行命令：

```bash
cmake --build build --target notdec-decompile GraphTest binarysub -j4
./build/binarysub
./build/bin/GraphTest --gtest_filter=Retypd.TypeBuilderSemanticPrimitiveAliasTest
tmpdir=$(mktemp -d)
cat >"$tmpdir/in.ll" <<'EOF'
; ModuleID = 'cli-smoke'
source_filename = "cli-smoke"

define i32 @main() {
entry:
  ret i32 0
}
EOF
cat >"$tmpdir/win.dot" <<'EOF'
digraph win_uint32 {
  graph [base="uint", bits="32", namespace="win"];
  root [special_role="root"];
  HANDLE;
  HWND;
  HWND -> HANDLE;
  HANDLE -> root;
}
EOF
./build/bin/notdec "$tmpdir/in.ll" -o "$tmpdir/out.ll" --tr-level=1 \
  --primitive-semantic-lattice "$tmpdir/win.dot" \
  --default-primitive-semantic-type prim.uint32.win.HWND
```

结果：

- `notdec` / `binarysub` / `GraphTest` 均构建成功
- `binarysub` 现有 primitive semantic lattice 测试继续通过
- 新增 `TypeBuilderSemanticPrimitiveAliasTest` 通过
- `notdec` CLI smoke test 能成功加载 `.dot` family 并校验默认 semantic type，然后继续完成 `.ll -> .ll` 主流程

## 后续建议

下一步可以直接沿用这次接好的 `Options` 字段推进：

1. 在函数参数建图或外部 signature 注入入口消费已加载的 semantic lattice registry
2. 在真正施加约束前检查 bitwidth 是否与目标值一致
3. 对需要显式指定多个参数语义类型的场景，再补更细粒度的 CLI / JSON 配置，而不是重新设计 registry 初始化链路

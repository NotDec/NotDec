# 2026-05-06 单字段结构体透明化

## 背景

尾部按值自递归转数组后，会出现这种结构：

```text
struct struct_4 {
  i8[] field_0;
};
```

它只是给单个成员套了一层 record。后续如果 value type 仍打印为 `struct_4*`，
可读性不如直接打印 `i8[]*`。

## 目标

先做保守透明化：

- 只处理整个 record 正好一个字段
- 字段不是 padding
- 字段 offset 必须是 0
- 不处理 memory root
- 不处理递归 binder anchor
- 不删除旧 decl，只把可达 HType 引用改成字段类型

## 实现记录

修改文件：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1672-1746`
  - 新增 `collectRecursiveAnchorDecls`
  - 从 value types、memory type、decl 字段、union member、typedef 中收集
    `RecursiveBindingType` / `RecursiveRefType` 的 anchor decl
  - 后续透明化时跳过这些 decl，避免破坏递归类型锚点

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1748-1846`
  - 新增 `TransparentRecordMap`
  - 新增 `rewriteTransparentRecordType`
  - 支持重写 `RecordType`、pointer、function、dual pointer、set union、
    set inter、array
  - 不改写 recursive binding/ref、typedef 本身
  - 用 `Resolving` 防止 record replacement 链成环

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1848-1897`
  - 新增 `normalizeTransparentSingleFieldRecords`
  - 收集单字段 record 替换表
  - 重写 `ValueTypesLower`、`ValueTypesUpper`、`MemoryType`、record 字段类型
  - 重新计算 `MemoryDecl`
  - 输出透明化数量

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3485`
  - 在 `MLsubRecovery::genASTTypes` 设置 memory type / decl 后调用
    `normalizeTransparentSingleFieldRecords`

- `test/type-recovery/llvm-ir/expected/tr-level-2/17_StackArray.htypes:4-12,62`
  - `struct_1*` 透明化为 `top:32[]*`
  - `struct_0.field_0` 从 `struct_1` 改成 `top:32[]`

- `test/type-recovery/llvm-ir/expected/tr-level-2/18_offset1.htypes:4-15,49,57-58,68,78-79,93-94`
  - 单字段 `i8[4][]` record 对 value type 透明化为 `i8[4][]*`
  - decl 编号随 snapshot 可达顺序变化

## 验证

已运行：

```bash
cmake --build ./build --target notdec-decompile -j4
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-transparent-single-field/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-transparent-single-field
```

结果：

- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通
- fortune 时间：`real 15.18s`、`user 14.78s`、`sys 0.40s`
- 对比参考 `real 16.33s`，没有看到性能回退
- fortune 输出：
  - 尾部 value recursion 规整 39 个 record
  - 单字段透明化 2 个 record

## 残留问题

- 旧 `RecordDecl` 不从 `HTypeContext` 删除，只依赖 snapshot 可达性不打印。
- `UnionDecl` 当前没有 mutable member 接口，所以只在收集递归 anchor 时读取
  union member，没有改写 union member 类型。
- 有 padding 的单字段 record 暂时不处理。
- 有语义名字的单字段 record 也可能被透明化；当前恢复出来的名字是自动生成的
  `struct_N`，风险可接受。后续如果引入源级名字，需要加白名单或标记。

## 简评

- 实现效果：7/10。解决当前单字段 wrapper 的主要可读性问题。
- 复杂度成本：6/10。引入了一套小型 HType 重写函数，比尾递归转数组复杂。
- 维护成本：6/10。后续若 HType 新增类型，需要扩展 rewrite 逻辑。

更完整的方案是给 `HTypeContext` 加 decl 删除/替换 API，并统一处理 record/union
member；当前先不加这个抽象。

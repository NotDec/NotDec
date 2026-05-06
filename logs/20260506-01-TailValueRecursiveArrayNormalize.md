# 2026-05-06 尾部按值自递归结构转数组

## 背景

`fortune` 的 `ValueHTypes.txt` 出现这种结构：

```text
struct struct_4 {
  i8 field_0;
  struct_4 field_1;
};
```

这不是合法 C 结构。它表示按 value 无限重复的尾部形状，更适合打印成数组。
当前已有 `RecursiveBindingType` / `RecursiveRefType`，但普通 HType 打印会落到
anchor decl 名字，所以文本上看不出 self ref。

## 目标

先只处理最保守的形态：

```text
struct R { T prefix; R tail; }
```

改成：

```text
struct R { T[] prefix; }
```

不处理互递归、union 里藏 self、无前缀 self、多字段 prefix。多字段 prefix 需要
新建数组元素 struct，会影响命名和 struct merge，暂时不做。

## 实现记录

修改文件：

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1605-1621`
  - 新增 `isDirectValueSelfRecursiveField`
  - 识别直接按 value 回到当前 `RecordDecl` 的字段
  - 支持 `RecordType`、`RecursiveBindingType`、`RecursiveRefType`
  - 显式跳过 pointer / dual pointer，避免误伤链表或普通指针

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1623-1628`
  - 新增 `isUnsizedArrayType`
  - 避免把已经是无界数组的 prefix 再包一层数组

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:1630-1676`
  - 新增 `normalizeTailValueRecursiveRecords`
  - 遍历 `HTypeContext::getDecls()`
  - 只匹配整个 record 正好两个字段的形态：第一个字段是 prefix，第二个字段是
    self tail
  - 将 prefix 类型改成无界数组，删除 self tail 字段
  - 输出规整数量日志

- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:3257`
  - 在 `MLsubRecovery::genASTTypes` 合并 HTypeResult 后、写 debug HTypes 前调用
    `normalizeTailValueRecursiveRecords`

- `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes:4-14`
  - 同步 golden
  - `top:8 + self tail` 现在打印为 `top:8[]`

## 验证

已运行：

```bash
cmake --build ./build --target notdec-decompile -j4
ctest --test-dir build -R 'notdec.type_recovery.(llvm_ir|sysy).tr_level_2' --output-on-failure
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-tail-rec-array/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-tail-rec-array
```

结果：

- `llvm_ir` / `sysy` tr-level=2 通过
- fortune 当前关注口径跑通
- fortune 时间：`real 15.56s`、`user 15.22s`、`sys 0.34s`
- 对比参考 `real 16.33s`，没有看到性能回退
- fortune 输出中规整了 39 个尾部按值自递归 record
- 选中的结构变为：
  - `struct_3 { i8[4][] field_0; }`
  - `struct_4 { i8[] field_0; }`

## 残留问题

- `struct R { R tail; }` 这种无前缀 self 没处理。
- union / set 中藏 self 没处理。
- 多字段 prefix 没处理。
- 当前注释会覆盖 prefix 原来的 offset 注释，后续如果需要保留原 offset，可以再把
  comment 改成同时包含 offset 和 normalized 信息。

## 简评

- 实现效果：8/10。解决当前关注的尾部 value self recursion。
- 复杂度成本：8/10。只是一个本地遍历函数，没有新增 pass 类。
- 维护成本：7/10。规则保守，后续扩展多字段 prefix 时需要小心命名稳定性。

更通用的方案是给 HType 增加显式的 normalized array marker 或专门的后处理文件，
但当前先不加抽象。

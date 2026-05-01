# 2026-05-01 ValueHTypes 字段 offset 注释补全

## 背景

`work_dir/ValueHTypes.txt` 里有不少声明已经带了 `/* at offset: ... */`，但还有一批单字段 shell 和 union 成员没有。看文件时会断层，不方便直接核对布局。

这次目标很简单：

- `ValueHTypes.txt` 里所有 field 都要带 offset 注释
- 只补注释，不改类型恢复、不改布局、不改 merge 逻辑

## 判断

先看生成链路后，问题不在 `ValueHTypes.txt` 的打印，而在少数 `FieldDecl` 构造点没有填 `Comment`。

实际漏掉的入口只有两处：

1. `finalizeRecursiveType()` 里给递归 shell 补的单字段，范围固定从 `0` 开始
2. `convertStruct()` 里构造 union 成员时，成员范围也固定从 `0` 开始

所以最小方案就是只在这两处补 `Comment = "at offset: 0"`。

## 实现

### `src/TypeRecovery/mlsub/TypeBuilder.cpp`

- `643-648`，函数 `finalizeRecursiveType()`
  - 给递归 shell 的单字段补上 `.Comment = "at offset: 0"`
- `1443-1449`，函数 `convertStruct()`
  - 给 union 成员补上 `.Comment = "at offset: 0"`

这两个位置之外没动。`craftStruct()` 原来就会给普通字段和 padding 写 offset 注释，这次不改它。

## 验证

1. build

```bash
cmake --build ./build --target notdec-decompile -j4
```

结果：通过。

2. fortune 当前关注用例，参考口径 `.ll`

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.offset-comments.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-offset-comments
```

结果：

- 运行通过
- `rg -n "field_[0-9]+;$" /tmp/notdec-fortune-offset-comments/ValueHTypes.txt`
  无命中，说明不再有裸 field 行
- `real 16.73s`

对比当前参考 `16.33s`，差 `0.40s`，这个改动只补字符串注释，属于同档波动，没有看到明确性能退化信号。

3. 重新生成仓库里的调试目录

```bash
NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/fortune.offset-comments.repo-workdir.out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/sn640/NotDec/work_dir
```

结果：通过，`work_dir/ValueHTypes.txt` 已刷新。

## 效果

- `ValueHTypes.txt` 里的递归单字段 shell 现在会显示 `/* at offset: 0 */`
- `ValueHTypes.txt` 里的 union 成员现在也会显示 `/* at offset: 0 */`
- 没引入新的类型规则，也没改变结构体/union 布局

## 评分

- 实现效果：9/10
- 理解成本：9/10
- 维护成本：9/10

原因：

- 改动很小，直接打在漏注释的源头
- 不碰打印器和类型逻辑，副作用最小
- 如果以后还有别的 `FieldDecl` 构造点漏注释，还是要继续从源头补

## 有没有更好的方案

有一个更兜底的方案：在 snapshot formatter 打印字段时，如果 `Comment` 为空，就按 `FieldDecl.R.Start` 自动补注释。

这方案覆盖面更大，但会把“调试导出层兜底”和“类型层本来就该带的信息”混在一起。当前这次只有两处漏点，直接在构造点补更干净。

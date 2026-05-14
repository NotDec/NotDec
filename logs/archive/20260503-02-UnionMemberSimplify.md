# 2026-05-03 union 成员里的 top 噪音简化

## 背景

`ValueHTypes.txt` 里有不少 union 成员只是重复的 `top:32`，或者
`struct/rec + top:32 + top:32` 这种明显噪音。

这轮只做 HType lowering 末端的最小简化，不改 binarysub solver，不改
field overlap 的布局切分。

目标就是三件事：

1. 完全相同的 union member 去重
2. 如果同一个 union 已经有更具体的成员，删掉纯 `top/bottom`
3. 简化后只剩 1 个成员时，不再硬造单成员 union

## 实现记录

修改文件：`src/TypeRecovery/mlsub/TypeBuilder.cpp`

- 第 278-315 行，新增 helper：
  - `isVagueUnionMemberType`
  - `simplifyUnionMembers`
- 第 1609-1629 行，函数 `TypeBuilder::convertStruct`
  - 在 union member 全部收集完后，先跑 `simplifyUnionMembers`
  - 如果简化后只剩 1 个成员，直接把这个成员回填到 `Fields`
  - 只有成员数仍然大于 1 时，才继续创建 `UnionDecl`

实现细节：

- 去重按 `HType::getCanonicalType()` 做
- `top` / `bottom` 都按 vague member 处理
- 如果 union 里没有 concrete member，就只做去重，不会把所有 vague member 全删掉

## 验证

已运行：

```bash
cmake --build /sn640/NotDec/build --target bin/notdec -j1
/usr/bin/time -p env NOTDEC_POINTER_ANALYSIS_MODE=original \
  ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-union-member-simplify/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-union-member-simplify
```

结果：

- 编译通过
- fortune 当前口径运行通过
- 时间：`real 15.13s`、`user 14.81s`、`sys 0.31s`
- 对比参考时间 `real 16.33s`（2026-04-29），没有看到性能回退

输出观察：

- 旧 `work_dir/ValueHTypes.txt` 里，`top:32 field_* /* at offset: 0 */`
  这种 union offset-0 top 成员有 25 个
- 新 `/tmp/notdec-fortune-union-member-simplify/ValueHTypes.txt` 里只剩 1 个
- 之前关注的
  `main::%bb.blk_exit39.i1`
  从 `[ValueHTypes.txt](/sn640/NotDec/work_dir/ValueHTypes.txt:2323)` 的
  `struct_51*`
  变成了
  `[/tmp/notdec-fortune-union-member-simplify/ValueHTypes.txt](/tmp/notdec-fortune-union-member-simplify/ValueHTypes.txt:2202)`
  的 `struct_48*`
- 这次新结果里对应 decl：
  `[/tmp/notdec-fortune-union-member-simplify/ValueHTypes.txt](/tmp/notdec-fortune-union-member-simplify/ValueHTypes.txt:332)`
  已经收成：
  - `struct_48 { i8; union_0 }`
  - `union_0 { rec_3850; struct_48 }`
  - 原先那两个 `top:32` 成员已经消失

## 说明

这轮按要求先没补单测，只做了构建和 fortune 口径验证。

## 简评

- 实现效果：8/10
- 复杂度成本：8/10
- 维护成本：8/10

当前方案够小，也确实把 union 里的明显 `top` 噪音压下去了。
后面如果还要继续收紧，比较合适的方向是把“vague member 能不能删”再细分成：

- 纯 primitive vague
- pointer vague
- 递归壳旁边的 vague

现在这版规则还是偏粗，只是先把最明显的一批噪音拿掉。

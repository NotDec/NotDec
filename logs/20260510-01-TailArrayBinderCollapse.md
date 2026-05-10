# 2026-05-10 tail-array 单字段壳继续折叠

## 背景

`ValueHTypes.txt` 里有不少这类结构：

```text
struct struct_N {
  i8[] field_0; /* tail value recursion normalized to array at offset: 0 */
};
```

前面已经做过两步：

- 尾部按值自递归 `struct R { T; R; } -> struct R { T[]; }`
- 单字段 record 透明化

但 `fortune` 里还会残两类问题：

1. 一部分 value type 还挂着 `RecursiveBindingType` / `RecursiveRefType`，所以
   仍打印成 `struct_N*`，没有继续变成 `i8[]*`
2. 即使 rewrite 成功，`ValueHTypes.txt` 的 `[decls]` 仍把 `HTypeContext`
   里的全部 decl 都打印出来，旧壳还会继续留在文件里

## 目标

- 只处理“刚被 tail-array 规整过”的那批 anchor record
- 如果对应 binder 的 body 已经不再引用自己，就把递归 binder 降成普通
  `RecordType(anchor)`
- 让后面的单字段透明化继续生效，最终把 `struct_N*` 变成成员类型指针
- `[decls]` 只打印可达 decl，不再把已经失去引用的旧壳重新扫出来

## 实现

### 已完成：tail-array 后只折叠对应的非递归 binder

- 修改文件：
  - `src/TypeRecovery/mlsub/HTypeNormalize.cpp:40-83`
  - `src/TypeRecovery/mlsub/HTypeNormalize.cpp:162-567`
  - `src/TypeRecovery/mlsub/HTypeNormalize.cpp:721-727`
- 涉及函数：
  - `normalizeTailValueRecursiveRecords`
  - `collectRecursiveBinders`
  - `containsRecursiveBinder`
  - `rewriteCollapsedRecursiveType`
  - `normalizeCollapsedRecursiveBinders`
  - `normalizeHTypeResult`
- 修改内容：
  - `normalizeTailValueRecursiveRecords()` 现在返回本轮刚规整过的
    `RecordDecl*` 集合
  - 新增 binder 收集、递归检测、type rewrite 三个辅助步骤
  - `normalizeCollapsedRecursiveBinders()` 只处理：
    - anchor decl 是本轮 tail-array record
    - binder body 已经不再引用自己的 binder
  - 替换目标不是 `binder->getBody()`，而是普通 `RecordType(anchor)`，避免把
    value 侧外层地址层数多保留一层，出现 `i8[]**`
  - 然后再接现有的 `normalizeTransparentSingleFieldRecords()`，让单字段壳继续
    折成成员类型

### 已完成：允许 rewrite 回写 union / typedef

- 修改文件：
  - `external/NotDec-llvm2c/include/notdec-llvm2c/Interface/HType.h:188-225`
- 涉及类型：
  - `UnionDecl`
  - `TypedefDecl`
- 修改内容：
  - 给 `UnionDecl` 增加可写 `getMembers()`
  - 给 `TypedefDecl` 增加 `setType()`
  - 这样 binder collapse 的 rewrite 可以把 decl 内部残留的 recursive type
    一起改掉，不只改 value map

### 已完成：`[decls]` 改成只打印可达 decl

- 修改文件：
  - `external/NotDec-llvm2c/lib/notdec-llvm2c/Interface/HType.cpp:711-713`
- 涉及函数：
  - `HTypeSnapshotFormatter::getOrderedDecls`
- 修改内容：
  - 去掉按 `Ctx->getDecls()` 全量补扫
  - 现在只返回 `primeFormatter()` / `collectType()` 已经收集到的 decl
  - 这样失去引用的单字段 tail-array 壳不会继续出现在 `ValueHTypes.txt`

### 已完成：同步 llvm-ir golden

- 修改文件：
  - `test/type-recovery/llvm-ir/expected/tr-level-2/20_PointerAnalysisFieldCycle.htypes:22-30`
  - `test/type-recovery/llvm-ir/expected/tr-level-2/21_PointerAnalysisBranchingFieldCycle.htypes:18-43,92-99`
- 修改内容：
  - `struct_*` 指针改成直接的数组指针
  - `21` 的 decl 顺序随可达 decl 输出顺序变化一起更新

## 验证

已运行：

```bash
cmake --build ./build --target notdec-decompile -j4
ctest --test-dir build -R '^notdec.type_recovery.llvm_ir.tr_level_2$' --output-on-failure
env NOTDEC_POINTER_ANALYSIS_MODE=original /usr/bin/time -p ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-collapse-rec-binder/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/tmp/notdec-fortune-collapse-rec-binder
mkdir -p /tmp/notdec-fortune-collapse-rec-binder-workdir
env NOTDEC_POINTER_ANALYSIS_MODE=original ./build/bin/notdec \
  test/type-recovery/realworld/cases/fortune.o3.wasm.ll \
  -o /tmp/notdec-fortune-collapse-rec-binder-workdir/out.ll \
  --tr-level=2 --frozen-tr-input-ir -g \
  --work-dir=/sn640/NotDec/work_dir
```

结果：

- `notdec.type_recovery.llvm_ir.tr_level_2` 通过
- `fortune` 当前关注口径跑通
- 运行日志显示：
  - `Info: normalized 44 tail value-recursive record(s) to arrays`
  - `Info: collapsed 43 non-recursive binder(s)`
  - `Info: normalized 45 transparent single-field record(s)`
- `/tmp/notdec-fortune-collapse-rec-binder/ValueHTypes.txt` 与
  `work_dir/ValueHTypes.txt` 中：
  - `add_file::%bb.blk_exit7.i4 => ... upper=i8[]*`
  - 不再出现“只有一个 `tail value recursion normalized to array` 字段”的
    `struct struct_N`
- `fortune` 时间：`real 17.19s`、`user 16.86s`、`sys 0.32s`
- 对比参考 `real 16.33s`，当前慢 `0.86s`

补充：

- `ctest --test-dir build -R '^notdec.type_recovery.sysy.tr_level_2$'` 仍失败在
  `73_int_io`
- 差异是 `i32/u32` 和 block 名变化，和这次改动路径无关；这轮没有改它

## 简评

- 实现效果：8/10。value type 和 `ValueHTypes.txt` 可达 decl 都收紧了，目标点
  已经从 `struct_*` 变成 `i8[]*`
- 复杂度成本：6/10。新增一层 binder 扫描和 rewrite，但仍然留在
  `HTypeNormalize` 本地，没有扩散到 pass 或 solver
- 维护成本：6/10。后续如果 HType 再增新种类，需要补两套 rewrite/遍历分支

更彻底的方案还是给 `HTypeContext` 提供 decl 删除能力，把失效 decl 真删掉。
这次先只做可达输出和局部 rewrite，够解决当前 `ValueHTypes.txt` 的主问题。

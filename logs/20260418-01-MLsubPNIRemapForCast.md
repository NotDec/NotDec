# MLsub cast remap 缺失 PNI alias 修复

日期：2026-04-18

## 1. 问题

调试 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll` 时，
`MLsubVisitor::visitCastInst()` 在处理 `inttoptr` 时会走
`ConstraintsGenerator::addRemapType()`，随后在
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:177](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L177)
到
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:194](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L194)
对应的旧 remap 尾路径里直接解引用 `PG.getPNIVar(Val)`，触发
`PNIGraph::getPNIVar()` 断言。

触发链路：

1. [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2707](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2707)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:2725](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2725)
   的 `ConstraintsGenerator::MLsubVisitor::visitCastInst()`
   会把 `bitcast` / `ptrtoint` / `inttoptr` 视作 remap。
2. `addRemapType()` 会给 `V2N` 新增 `Val -> Target` 的 MLsub type alias，
   但旧实现默认 `PG` 里已经存在 `Val` 的 `PNIVar`。
3. 对 `fortune.o3.wasm.4.ll`，触发点是
   `%30 = inttoptr i32 %_local_3.6 to i8*`，此时 `Target` 已有 `PNIVar`，
   但 `Val` 还没有单独建图，所以 `getPNIVar(Val)` 直接断言。

## 2. 修改

### 2.1 在 `PNIGraph` 增加 remap 语义

在
[include/notdec/TypeRecovery/mlsub/PNDiff.h:249](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/PNDiff.h#L249)
到
[include/notdec/TypeRecovery/mlsub/PNDiff.h:267](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/PNDiff.h#L267)
新增 `PNIGraph::remapPNIVar(ExtValuePtr Val, ExtValuePtr Target)`：

1. 若 `Val` 已经映射到 `TargetNode`，直接返回
2. 若 `Val` 已有其他 `PNINode`，则调用 `TargetNode->unify(*ValNode)`
3. 若 `Val` 尚未建图，则直接把 `Val` alias 到 `TargetNode`

这样 `PG` 侧就和 `V2N` 一样，支持“新增别名但不强制预建节点”的 remap。

### 2.2 `addRemapType()` 改为统一走 `PG.remapPNIVar()`

在
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:177](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L177)
到
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:194](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L194)
的 `ConstraintsGenerator::addRemapType()` 中：

1. `if (N == ty)` 的早返回路径现在也会先执行 `PG.remapPNIVar(Val, Target)`
2. 新插入 `V2N` alias 的路径不再直接 `getPNIVar(Val)`，而是调用
   `PG.remapPNIVar(Val, Target)`

这样可以保证：

1. `V2N` 与 `PG` 的 alias 语义保持一致
2. 即便 `Val` 还没有 `PNIVar`，cast/remap 也不会再触发空节点断言
3. 若后续别的路径已经提前给 `Val` 建过 PNI，也仍然会被正常 unify

## 3. 验证

执行：

```bash
./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/fortune.out.ll \
  --tr-level=2 \
  --dump-htypes /tmp/fortune.htypes \
  -g \
  --work-dir=/sn640/NotDec/work_dir
```

结果：

1. 不再触发 `PNIGraph::getPNIVar()` assertion
2. 进程成功跑到 `Constraint generation done! SCC count:1`
3. 成功输出 `/tmp/fortune.out.ll`

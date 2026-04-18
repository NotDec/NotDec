# MLsub cast 在无 V2N 节点时补做 PNI remap

日期：2026-04-18

## 1. 问题

在前一轮修复后，
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:177](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L177)
到
[include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:194](/sn640/NotDec/include/notdec/TypeRecovery/mlsub/MLsubGenerator.h#L194)
的 `ConstraintsGenerator::addRemapType()` 已经不会因为 cast result 尚未预建
`PNIVar` 而断言。

但继续调 `test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll`
时，`work_dir/03-pndiff-final.ll` 里的

```llvm
%414 = inttoptr i32 %calcOffset526 to i32*; pndiff: result=ptr#1145, op0=unknown#1142
```

仍然说明 `inttoptr` 的结果和参数没有 unify 到同一个 `PNINode`。

根因不在 `addRemapType()`，而在
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:2735](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2735)
到
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:2746](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2746)
的 `ConstraintsGenerator::MLsubVisitor::visitCastInst()`：

1. 这里原先只有 `cg.getNodeOrNull(getExtValuePtr(Src, &I, 0))` 命中时才会调
   `cg.addRemapType(&I, ...)`
2. `getNodeOrNull()` 查的是 `V2N`，不是 `PNDiff` 图
3. 像 `%calcOffset526 = add i32 %413, 76` 这种值，当前常常只有
   `PG.getPNIVarOrNull(...)` 命中，而没有对应 `V2N` 节点
4. 于是 `%414 = inttoptr i32 %calcOffset526 to i32*` 这一条根本不会执行
   remap，后续被 `load` 使用时又会单独创建新的 result PNI，导致
   `result` / `op0` 分裂

## 2. 修改

本次只改了
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:2735](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2735)
到
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:2746](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L2746)
的 `ConstraintsGenerator::MLsubVisitor::visitCastInst()`：

1. 把 `bitcast` / `ptrtoint` / `inttoptr` 合并为同一条 cast-alias 路径
2. 先看 `cg.PG.getPNIVarOrNull(SrcVal)` 是否存在
3. 只要源值已经有 `PNDiff` 节点，就先执行 `cg.PG.remapPNIVar(&I, SrcVal)`
4. 原有 `cg.getNodeOrNull(SrcVal)` 命中时再继续执行 `cg.addRemapType(&I, SrcVal)`，
   保持 MLsub type alias 的旧行为不变

这样拆开以后：

1. `PNDiff` 的 alias 传播不再依赖 `V2N` 是否已经 materialize
2. `MLsub` 类型 remap 仍然只在已有 type node 时发生，不扩大原来的类型约束面
3. `inttoptr` / `ptrtoint` / `bitcast` 的 result-op0 PNI 一致性可以单独保证

## 3. 验证

执行：

```bash
env NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  NOTDEC_DISABLE_INTERPROC=1 \
  NOTDEC_SAT_DISABLE=1 \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/notdec-inttoptr-check/out.ll \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-inttoptr-check
```

验证结果：

1. `/tmp/notdec-inttoptr-check/03-pndiff-final.ll:287`：
   `%30 = inttoptr i32 %_local_3.6 to i8*; pndiff: result=ptr#2801, op0=ptr#2801`
2. `/tmp/notdec-inttoptr-check/03-pndiff-final.ll:1235`：
   `%412 = inttoptr i32 %calcOffset525 to i32*; pndiff: result=ptr#2955, op0=ptr#2955`
3. `/tmp/notdec-inttoptr-check/03-pndiff-final.ll:1238`：
   `%414 = inttoptr i32 %calcOffset526 to i32*; pndiff: result=unknown#1080, op0=unknown#1080`

第三条说明这次关注的 `%414` 已经把参数和值结果 unify 到同一个 `PNINode`，
只是该节点当前状态仍然是 `unknown`，不再是之前那种 `result/op0` 分裂。

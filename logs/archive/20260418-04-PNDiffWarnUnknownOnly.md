# PNDiff.warn.txt 只导出含 unknown 的残余约束

日期：2026-04-18

## 1. 问题

调试 `work_dir/PNDiff.warn.txt` 时发现，当前 residual warning 导出会把
`PG.Constraints` 中剩余的 Add/Sub 约束全部打印出来，即便它们的状态已经是
类似：

```text
state: result=ptr#2964, op0=ptr#2964, op1=num#1514
```

这种全已知 `ptr/num` 状态。

对当前调试目的来说，这类条目噪音较大；更希望 `PNDiff.warn.txt` 只保留
至少一个位置仍然是 `unknown` 的约束，方便聚焦尚未解开的点。

## 2. 修改

本次只改了
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:1449](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1449)
到
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:1520](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1520)
这一段 `PNDiff.warn.txt` 导出逻辑，涉及函数如下：

1. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1466](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1466)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1475](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1475)
   新增 `constraintHasUnknownState(ConstraintsGenerator &, const ConsNode &)`：
   - 遍历 `result/op0/op1`
   - 只要某个位置 `getPNDiffStateInfo()` 为空，或返回 `"unknown"`，就判定该约束应保留在 warning 输出中
2. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1477](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1477)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1520](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1520)
   的 `writePNDiffWarnings()` 中：
   - 对每条 `ConsNode` 先调用 `constraintHasUnknownState()`
   - 全已知 `ptr/num` 的条目直接跳过
   - 只有该 SCC 真正写出了至少一条 `unknown` 约束时，才打印 `## SCC: ...` 头
   - 若最终一条都没写出，则改成输出
     `No residual Add/Sub constraints with unknown state after solve.`

## 3. 验证

执行：

```bash
env NOTDEC_POLY_FUNCS=/sn640/NotDec-Exp/ICSE-HOWARD/polymorphic_funcs.json \
  NOTDEC_DISABLE_INTERPROC=1 \
  NOTDEC_SAT_DISABLE=1 \
  ./build/bin/notdec \
  test/type-recovery/howard-o3-split/cases/fortune.o3.wasm.4.ll \
  -o /tmp/notdec-pndiff-warn-filter/out.ll \
  --tr-level=2 \
  -g \
  --work-dir=/tmp/notdec-pndiff-warn-filter
```

随后检查：

```bash
rg -n -P '^state: (?!.*unknown)' /tmp/notdec-pndiff-warn-filter/PNDiff.warn.txt
```

结果为空，说明新的 `PNDiff.warn.txt` 中不再包含全已知状态的 residual 条目。

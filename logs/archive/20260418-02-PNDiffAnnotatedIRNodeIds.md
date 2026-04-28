# PNDiff 注释输出节点编号

日期：2026-04-18

## 1. 目的

为了在 `work_dir/03-pndiff-final.ll` 里快速判断多处 `ptr/num/unknown`
是不是同一个 `PNINode`，需要让现有 `pndiff:` 注释在输出状态时同时带上
PDIff 节点编号。

## 2. 修改

本次只改了
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:1350](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1350)
到
[src/TypeRecovery/mlsub/MLsubGenerator.cpp:1463](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1463)
这一段注释格式化逻辑，涉及函数如下：

1. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1350](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1350)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1381](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1381)
   新增 `PNDiffStateInfo`、`formatPNDiffStateInfo()`、
   `getPNDiffStateInfo()`，把原先只返回 `"ptr" / "num" / "unknown"` 的
   逻辑改成返回 `{State, NodeId}`。
2. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1394](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1394)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1415](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1415)
   的 `formatInstructionPNDiffComment()` 里，把指令结果和操作数注释改成
   `ptr#3028` / `num#2375` / `unknown#484` 这类格式。
3. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1418](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1418)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1439](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1439)
   的 `formatFunctionPNDiffComment()` 里，把函数 `ret/argN` 注释也同步改成
   带节点编号的格式。
4. 在
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1449](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1449)
   到
   [src/TypeRecovery/mlsub/MLsubGenerator.cpp:1463](/sn640/NotDec/src/TypeRecovery/mlsub/MLsubGenerator.cpp#L1449)
   的 `formatConstraintStateSummary()` 里，同步让 `PNDiff.warn.txt`
   里的 residual constraint state 摘要也带上节点编号。

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

验证点：

1. `work_dir/03-pndiff-final.ll` 里的函数声明注释已变成
   `ret=ptr#3028, arg0=ptr#1799` 这类格式。
2. `fortune` 主流程里，原先关注的 `inttoptr` remap 位置现在显示为：
   `%30 = inttoptr i32 %_local_3.6 to i8*; pndiff: result=ptr#3028, op0=ptr#3028`
3. `add i32 %82, 76` 这类 ptr arithmetic 位置现在显示为：
   `%calcOffset409 = add i32 %82, 76; pndiff: result=ptr#3028, op0=ptr#3028, op1=num#481`
   可以直接判断 `%82` 和加法结果共享同一个 `PNINode #3028`。

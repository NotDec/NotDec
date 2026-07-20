# 20260720-01 merge policy DebugInfo eval 实现记录

## 原始 prompt

继续按照/sn640/NotDec/binarysub-private/cons_gen/docs/MergePolicyEvaluationMetrics.md的话，考虑基于LLVM IR上的DebugInfo统计一些节点合并策略的评估指标要怎么做，看看有没有什么变动。是不是有debuginfo类型的结构体指针类型的值都可以统计起来，另外也可以直接生成一个类似HType的txt结果的那种，有类型的值的类型的ground truth。

merge 统计这一块要不直接在类型推理结束后，单独做一块分析和盘点？比如把映射都收集起来，包括V2N以及SimpleType变量节点合并后也会存在DSU的合并逻辑，直接都盘点分析一遍映射就行？没必要再过程中去维护什么EvalDSU？还是说过程中维护有好处？另外方便用一个flag控制评估逻辑的开关吗

继续推进吧

## 实现概述

本次把 DebugInfo merge-policy 评估做成可选逻辑，默认关闭。开启 `--merge-eval-dir=<dir>` 后，类型恢复开始时收集 DebugInfo oracle，推理过程中只接收已有 value/node 映射和 merge 回调，类型恢复结束、释放 binarysub 图之前统一盘点最终 component。

## 代码修改

- `include/notdec/DecompilerContext.h:17`：新增 `Options::mergeEvalDir`，保存 CLI 传入的评估输出目录。
- `src/NotDec.cpp:89`、`src/NotDec.cpp:244`、`src/NotDec.cpp:291`：新增 `--merge-eval-dir`，禁止和 `--emit-tr-input-ir` 同用，并写入 `Options`。
- `include/notdec/Passes/PassManager.h:76`、`include/notdec/Passes/PassManager.h:127`、`src/Passes/PassManager.cpp:342`：把 merge eval 目录传给 `MLsubRecovery`，并要求 `tr-level >= 2`。
- `include/notdec/TypeRecovery/mlsub/MergePolicyEval.h:21`：新增 `MergePolicyEval` 接口，包含 value 映射、V2N merge、binarysub merge、addSubtype 计数和 final 输出。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:64`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:101`：实现 DebugInfo 类型规范化，只把去掉 typedef/const/volatile 后的 `struct*` 作为 strict oracle。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:294`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:310`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:348`：收集 global、函数参数/返回值、`dbg.value/dbg.declare` 直接位置的 DebugInfo 类型。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:428`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:460`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:472`：记录 V2N merge 和 binarysub variable merge 的 first bad union witness。
- `src/TypeRecovery/mlsub/MergePolicyEval.cpp:483`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:520`、`src/TypeRecovery/mlsub/MergePolicyEval.cpp:542`：输出 `DebugInfoValueTypes.txt`、`bad_unions.jsonl`、`merge-eval-summary.json`。
- `include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:133`、`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:257`、`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h:479`：在 `ConstraintsGenerator` / `MLsubRecovery` 中保存可选评估对象，`addSubtype()` 传入 binarysub `ConstraintContext`。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2595`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:2678`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:2714`：在 V2N merge、create node、remap 时通知评估对象。
- `src/TypeRecovery/mlsub/MLsubGenerator.cpp:2722`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3305`、`src/TypeRecovery/mlsub/MLsubGenerator.cpp:3555`：在 `MLsubRecovery::run()` 创建评估对象，传给各 SCC generator，并在 `topDownPhase()` 释放图前 finish。
- `src/CMakeLists.txt:34`：把 `MergePolicyEval.cpp` 加入 `notdec-core`。

## 验证

- `cmake --build ./build --target notdec -j4`：通过。
- `ctest --test-dir build -R notdec.type_recovery.llvm_ir.tr_level_2 --output-on-failure`：通过，1/1 passed。
- fortune eval 命令：`./build/bin/notdec /sn640/NotDec-Exp/Bench2/source-ir/ir/fortune/fortune.ll -o /tmp/notdec-fortune-eval-1784534115.ll --tr-level=2 --merge-eval-dir=/tmp/notdec-merge-eval-fortune-1784534115`。
- fortune 输出：生成 `DebugInfoValueTypes.txt`、`bad_unions.jsonl`、`merge-eval-summary.json`；summary 中 `target_nodes=1153`、`typed_target_nodes=22`、`bad_unions=0`、`polluted_components=0`。

## 评估

- 实现效果：8/10。已经能基于 LLVM DebugInfo 输出 ground truth txt，并在类型恢复结束后统计 coverage、wrong merge、fragmentation、performance。
- 复杂度：6/10。主要逻辑集中在新 cpp，主流程只接少量 hook；但 DebugInfo 和 SimpleType 映射本身有一定理解成本。
- 维护成本：6/10。后续扩展 target node 分类、DIExpression fragment、更多 witness 字段时，基本只改 `MergePolicyEval.cpp`。

## 后续注意

fortune 当前优化 IR 的本地 `dbg.value` 覆盖较低，strict struct pointer oracle 主要来自全局、函数参数和返回值。后续如果要提高覆盖率，优先补 DIExpression fragment 和更多局部变量位置，而不是放宽 strict oracle 判定。

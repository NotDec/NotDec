# PNDiff union 接口重设计：mergePNINodes 返回真实代表，内置 union by size

## 背景

PNDiff 的 union 接口（`PNINode::unify` / `mergePNINodes(To, From)` / `mergePNVarTo(Var, Target)`）
全部假设"谁被留下"：`this`/`To`/`Target` 保留，`other`/`From`/`Var` 被吸收并立即
`eraseFromParent()` 删除节点对象。为降低 DSUMap key 搬运量尝试 union by size
（swap To/From 后把大集合当代表），但 swap 反转了"被删节点"，使重入路径
（`solve → eraseConstraint → OnPtrAdd 回调 → setAsPtrAdd → unifyPNIValues → unify`）
解引用已删除节点，tmux/redis 在 PNDiff solve 阶段 use-after-free 崩溃（signal 11）。
此前尝试"不立即 erase + solve 末尾延迟清理"（stash 中），后改为更根本的方案：
重设计接口，让"谁被留下"成为内部实现细节，调用方只依赖返回值。

## 改动

`external/binarysub/include/binarysub/PNDiff.h`：
- `PNINode::unify(PNINode &other)`：注释更新为"this 不保证留下，只返回真代表"。
- `mergePNINodes(PNINode *To, PNINode *From)` → `mergePNINodes(PNINode *A, PNINode *B)`：
  声明语义改为"合并两个节点，返回被留下的代表，调用方不得假设 A/B 谁留下"。
- 删除 `mergePNVarTo(PNINode *Var, PNINode *Target)` 声明（职责并入 mergePNINodes）。
- `remapPNIVar(PNIValue, PNIValue)` 返回类型 `PNINode&` → `void`（所有调用方均忽略
  返回值），内部改调 `mergePNINodes`，不再有"返回 Target 代表"的假设。

`external/binarysub/src/PNDiff.cpp`：
- 新增 `PNIGraph::mergePNINodes` 实现：按 `PNIMap.rev()` 的 value 数（等价类大小）
  union by size 选代表；`Rep->Ty.merge(Absorbed->Ty)`（PNTy 可交换，代表选择不影响
  类型语义）；`PNIMap.merge` 迁移正反索引；`Absorbed->eraseFromParent()`；`markChanged(Rep)`；
  返回 Rep。原 `mergePNVarTo` 的 size/同一 graph 断言上移至此。
- 删除 `PNIGraph::mergePNVarTo` 实现。

`include/notdec/TypeRecovery/mlsub/MLsubGenerator.h` / `src/TypeRecovery/mlsub/MLsubGenerator.cpp`：
- `remapPNINode` 返回类型 `PNINode&` → `void`（调用方 addRemapType 等均忽略返回值）。

## 验证

- 构建：`cmake --build build-relwithdebinfo-20260731 --target notdec -j8` 通过。
- ctest `notdec.type_recovery.llvm_ir.tr_level_2`：19 passed / 3 failed
  （06_SimpleRecursive2 / 17_StackArray / 20_PointerAnalysisFieldCycle）；
  失败列表与改动前基线完全一致，为既有失败，非本次引入。
- tmux 全量跑（4 线程，`NOTDEC_BINARYSUB_THREADS=4`，新 work/eval 目录）：
  PNDiff solve 阶段通过（wall 22:14 无崩溃；此前带 union-by-size 实验的
  tmux-fix1 在 8:54 崩溃）；最终 EXIT=137（OOM killer，峰值 62.3GB）——
  内存爆炸在 PNDiff 之后的 binarysub simplify/coalesce 阶段，是已知独立问题
  （此前 tmux/lighttpd 同样 ~56GB OOM），与本次改动无关。

## 后续

tmux/lighttpd 的 bulkSimplify 内存爆炸（~60GB）仍未解决，待单独处理。
